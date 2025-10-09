#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <stdint.h>

#include "GL.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// FreeType
#include <ft2build.h>
#include FT_FREETYPE_H

// HarfBuzz
#include <hb.h>
#include <hb-ft.h>

constexpr uint32_t max_line_length = 1024;
constexpr uint32_t max_transition = 2;

struct TextManager
{
    struct Glyph
    {
        GLuint tex_id;
        uint32_t width, height;
        float advance;
        float bearing_x, bearing_y;
    };

    void load_glyph(hb_codepoint_t gid);

    void draw_text(std::string str, glm::vec2 window_dimensions, glm::vec2 anchor, glm::vec3 colour);

    TextManager();
    ~TextManager();

    TextManager operator=(const TextManager other)
    {
        if (this == &other)
            return *this;

        this->ft_library = other.ft_library;
        this->ft_face = other.ft_face;
        this->hb_font = other.hb_font;
        this->character_atlas = std::unordered_map(other.character_atlas);
        this->program = other.program;
        this->Position = other.Position;
        this->Colour = other.Colour;
        this->TexCoord = other.TexCoord;
        this->vao = other.vao;
        this->vbo = other.vbo;

        return *this;
    }

private:
    // Libraries and fonts to draw the text
    FT_Library ft_library;
    FT_Face ft_face;
    hb_font_t *hb_font;

    // Font file and size
    const char *font_file = "FreeSans.otf";
    const int font_size = 36;
    const int margin = font_size / 2;

    // Map of all previously seen characters and their corresponding texture
    std::unordered_map<hb_codepoint_t, Glyph> character_atlas;

    // GL properties
    GLuint program;
    GLuint Position;
    GLuint Colour;
    GLuint TexCoord;
    GLuint vao;
    GLuint vbo;

    std::vector<std::string> wrap_text(std::string str, glm::vec2 window_dimensions, glm::vec2 anchor);
};

// State machine
struct StateMachine
{
    // Transition struct for the state machine
    struct Transition
    {
        uint32_t dst_id = -1;             // Id of the state to go to
        char text[max_line_length] = {0}; // Text to display on transition
    };

    // State struct for the state machine
    struct State
    {
        uint32_t id = -1;                       // Index of the current state in the state machine
        char text[max_line_length] = {0};       // Text to display on arriving to this state
        Transition transitions[max_transition]; // Transitions from this state
    };

    StateMachine();
    StateMachine(std::vector<State> states);
    StateMachine(const StateMachine *machine);

    void switch_state(Transition transition);

    void add_state(State state);

    void reset();

    std::vector<State> get_states();

    std::string to_string();

    void draw_current_state(glm::vec2 window_dimensions, glm::vec2 anchor, glm::vec3 colour);

    State current_state;

    StateMachine operator=(const StateMachine other)
    {
        if (this == &other)
            return *this;

        this->current_state = other.current_state;
        this->text_to_display = other.text_to_display;
        this->states = std::vector(other.states);
        
        return this;
    }

private:
    std::vector<State> states;
    std::string text_to_display;

    TextManager tm = TextManager();
};