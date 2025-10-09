#include "StateMachine.hpp"

#include <assert.h>
#include <iostream>
#include <sstream>
#include <algorithm>

#include "gl_compile_program.hpp"

// Shaders taken from https://github.com/jialand/TheMuteLift#
const GLchar *vertexSrc =
    R"GLSL(
        #version 330
        layout(location=0) in vec2 aPos;
        layout(location=1) in vec2 aUV;
        out vec2 vUV;
        uniform vec2 uScreen; // in pixels
        void main(){
            vUV = aUV;
            // pixel -> NDC. Note: NDC y goes up; here (0,0) = top-left corner
            float x = (aPos.x / uScreen.x) * 2.0 - 1.0;
            float y = 1.0 - (aPos.y / uScreen.y) * 2.0;
            gl_Position = vec4(x, y, 0.0, 1.0);
        }
    )GLSL";

const GLchar *fragmentSrc =
    R"GLSL(
        #version 330
        in vec2 vUV;
        out vec4 FragColor;
        uniform sampler2D uTex; // R8, red channel as alpha
        uniform vec3 uColor;
        void main(){
            float a = texture(uTex, vUV).r;
            FragColor = vec4(uColor, a);
        }
    )GLSL";

TextManager::TextManager()
{
    FT_Error ft_error;

    if ((ft_error = FT_Init_FreeType(&ft_library)))
    {
        std::cout << "No library" << std::endl;
        abort();
    }
    if ((ft_error = FT_New_Face(ft_library, font_file, 0, &ft_face)))
    {
        std::cout << "No Face " << ft_error << std::endl;
        abort();
    }
    if ((ft_error = FT_Set_Char_Size(ft_face, font_size * 64, font_size * 64, 0, 0)))
    {
        std::cout << "No Char size" << std::endl;
        abort();
    }

    hb_font = hb_ft_font_create(ft_face, NULL);
    hb_ft_font_set_funcs(hb_font); // use FT-provided metric functions

    // Taken from https://github.com/jialand/TheMuteLift#
    program = gl_compile_program(vertexSrc, fragmentSrc);
    Position = glGetUniformLocation(program, "uScreen");
    Colour = glGetUniformLocation(program, "uColor");
    TexCoord = glGetUniformLocation(program, "uTex");

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void *)(sizeof(float) * 2));
    glBindVertexArray(0);
}

TextManager::~TextManager()
{
    for (auto &ch : character_atlas)
    {
        if (ch.second.tex_id)
            glDeleteTextures(1, &ch.second.tex_id);
    }
    hb_font_destroy(hb_font);
    FT_Done_Face(ft_face);
    FT_Done_FreeType(ft_library);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);
}

void TextManager::load_glyph(hb_codepoint_t gid)
{
    FT_Load_Glyph(ft_face, gid, FT_LOAD_DEFAULT);

    FT_GlyphSlot slot = ft_face->glyph;
    FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);

    FT_Bitmap bitmap = slot->bitmap;

    Glyph g;
    g.width = bitmap.width;
    g.height = bitmap.rows;
    g.bearing_x = slot->bitmap_left;
    g.bearing_y = slot->bitmap_top;
    g.advance = slot->advance.x / 64.0f;

    // Texture loading taken from https://github.com/jialand/TheMuteLift#
    glGenTextures(1, &g.tex_id);
    glBindTexture(GL_TEXTURE_2D, g.tex_id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, g.width, g.height, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap.buffer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    character_atlas.emplace(std::pair(gid, g));
}

void TextManager::draw_text(std::string str, glm::vec2 window_dimensions, glm::vec2 anchor, glm::vec3 colour)
{
    glUseProgram(program);
    glUniform2f(Position, float(window_dimensions.x), float(window_dimensions.y));
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(TexCoord, 0);

    // Position of the cursor that is writing the text
    float pen_x = anchor.x;
    float pen_y = anchor.y;

    std::vector<std::string> wrapped_text = wrap_text(str, window_dimensions, anchor);
    for (std::string line : wrapped_text)
    {
        hb_buffer_t *hb_buffer;
        hb_buffer = hb_buffer_create();
        hb_buffer_add_utf8(hb_buffer, line.c_str(), -1, 0, -1);
        hb_buffer_guess_segment_properties(hb_buffer);

        hb_feature_t features[] = {
            {HB_TAG('k', 'e', 'r', 'n'), 1, 0, ~0u},
            {HB_TAG('l', 'i', 'g', 'a'), 1, 0, ~0u},
        };

        hb_shape(hb_font, hb_buffer, features, sizeof(features) / sizeof(features[0]));

        unsigned int len = hb_buffer_get_length(hb_buffer);
        hb_glyph_info_t *info = hb_buffer_get_glyph_infos(hb_buffer, NULL);
        hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(hb_buffer, NULL);

        glUniform3f(Colour, colour.r, colour.g, colour.b);
        glBindVertexArray(vao);

        // Enable alpha blending for text rendering
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        for (unsigned int i = 0; i < len; i++)
        {
            hb_codepoint_t gid = info[i].codepoint;

            if (gid == 0)
            {
                throw std::invalid_argument("File contained characters not defined in the given font");
            }

            // Get the current glyph
            const auto &found = character_atlas.find(gid);
            Glyph glyph;
            if (found == character_atlas.end())
            {
                load_glyph(gid);
            }
            glyph = character_atlas[gid];

            // Adapted from https://github.com/tangrams/harfbuzz-example
            float x_advance = pos[i].x_advance / 64.0f;
            float y_advance = pos[i].y_advance / 64.0f;
            float x_offset = pos[i].x_offset / 64.0f;
            float y_offset = pos[i].y_offset / 64.0f;

            float x0 = pen_x + x_offset + glyph.bearing_x;
            float y0 = pen_y - y_offset - glyph.bearing_y;
            float x1 = x0 + glyph.width;
            float y1 = y0 + glyph.height;

            float quad[] = {
                x0, y0, 0.0f, 0.0f,
                x1, y0, 1.0f, 0.0f,
                x1, y1, 1.0f, 1.0f,

                x0, y0, 0.0f, 0.0f,
                x1, y1, 1.0f, 1.0f,
                x0, y1, 0.0f, 1.0f};

            glBindTexture(GL_TEXTURE_2D, glyph.tex_id);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_DYNAMIC_DRAW);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            pen_x += x_advance;
            pen_y += y_advance;
        }

        pen_x = anchor.x;
        pen_y += font_size;

        hb_buffer_destroy(hb_buffer);
        glBindVertexArray(0);
    }

    glDisable(GL_BLEND);
    glUseProgram(0);
}

std::vector<std::string> TextManager::wrap_text(std::string str, glm::vec2 window_dimensions, glm::vec2 anchor)
{
    std::stringstream ss(str);

    std::vector<std::string> words;

    std::string token;
    while (getline(ss, token, ' '))
        words.emplace_back(token);

    std::vector<std::string> output;
    std::string line;
    float line_length = 0.0f;
    float acc = 0.0f;
    for (std::string word : words)
    {
        if (word.length() == 0)
            continue;

        hb_buffer_t *hb_buffer;
        hb_buffer = hb_buffer_create();
        hb_buffer_add_utf8(hb_buffer, word.c_str(), -1, 0, -1);
        hb_buffer_guess_segment_properties(hb_buffer);

        hb_feature_t features[] = {
            {HB_TAG('k', 'e', 'r', 'n'), 1, 0, ~0u},
            {HB_TAG('l', 'i', 'g', 'a'), 1, 0, ~0u},
        };

        hb_shape(hb_font, hb_buffer, features, sizeof(features) / sizeof(features[0]));

        unsigned int glyph_count_after;
        unsigned int len = hb_buffer_get_length(hb_buffer);
        hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(hb_buffer, NULL);
        hb_glyph_info_t *infos = hb_buffer_get_glyph_infos(hb_buffer, &glyph_count_after);

        unsigned int break_index = 0;

        unsigned int token_length = 0;
        unsigned int string_index = 0;
        unsigned int break_string_index = 0;

        bool word_fit = false;
        bool no_break = true;
        uint32_t current_cluster = infos[0].cluster;

        while (!word_fit && break_index < len)
        {
            for (unsigned int i = break_index; i < len; i++)
            {
                uint32_t cluster = infos[i].cluster;
                if (cluster != current_cluster)
                {
                    string_index++;
                    current_cluster = cluster;

                    if (line_length + anchor.x >= window_dimensions.x - margin)
                    {
                        token_length = string_index - break_string_index;
                        break_string_index = string_index;
                        break_index = i;
                        no_break = false;
                        break;
                    }
                    line_length += acc;
                    acc = 0.0f;
                }
                acc += pos[i].x_advance / 55.0f;
            }

            if (no_break)
            {
                line.append(word.substr(break_string_index, word.length() - break_string_index) + ' ');
                word_fit = true;
            }
            else if (line.empty())
            {
                line.append(word.substr(break_string_index - token_length, token_length));
                output.emplace_back(line);
                line = std::string();
                line_length = 0;
                token_length = 0;
                break_index++;

                if (break_string_index >= word.length() - 1)
                {
                    line.append(word.substr(break_string_index, word.length() - break_string_index));
                    output.emplace_back(line);
                    line = std::string();
                    line_length = 0;

                    word_fit = true;
                }
            }
            else
            {
                output.emplace_back(line);
                line = std::string();
                break_index = 0;
                token_length = 0;
                break_string_index = 0;
                string_index = 0;
                word_fit = false;
                no_break = true;
                line_length = 0;
            }
        }
    }
    output.emplace_back(line);
    return output;
}

StateMachine::StateMachine()
{
    states = std::vector<State>();
}

StateMachine::StateMachine(std::vector<State> _states)
{
    states = std::vector(_states);
    assert(states.size() != 0 && "Cannot pass an empty state vector to the constructor");
    current_state = states[0];
    text_to_display = current_state.text;
}

StateMachine::StateMachine(const StateMachine *machine)
{
    states = std::vector(machine->states);
    current_state = machine->current_state;
    text_to_display = current_state.text;
}

void StateMachine::add_state(State state)
{
    if (states.size() == 0)
    {
        current_state = state;
        text_to_display = state.text;
    }

    states.emplace_back(state);
}

std::vector<StateMachine::State> StateMachine::get_states()
{
    return states;
}

void StateMachine::switch_state(Transition transition)
{
    if (transition.dst_id != -1U)
    {
        text_to_display = std::string(transition.text);
        current_state = states[transition.dst_id];
        text_to_display.append(" ");
        text_to_display.append(current_state.text);
    }
}

void StateMachine::reset() {
    current_state = states[0];
    text_to_display = current_state.text;
}

std::string StateMachine::to_string()
{
    std::string out = "";
    for (State state : states)
    {
        out.append("State ");
        out.append(std::to_string(state.id));
        out.append(" ");
        out.append(state.text);
        out.append("\n");
        for (Transition transition : state.transitions)
        {
            out.append("Transition ");
            out.append(std::to_string(transition.dst_id));
            out.append(" ");
            out.append(transition.text);
            out.append("\n");
        }
    }
    return out;
}

void StateMachine::draw_current_state(glm::vec2 window_dimensions, glm::vec2 anchor, glm::vec3 colour)
{
    tm.draw_text(text_to_display, window_dimensions, anchor, colour);
    // std::cout << text_to_display << std::endl;
}