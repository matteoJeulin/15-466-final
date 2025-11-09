/************************************************************************************************************
 * The following is a modified version of Kenechukwu's
 * "TextMeshNovice.hpp" file from 15-466 Game 5 (https://github.com/kechezon/15-466-f25-base5),
 * itself inspired by Jim McCann's provided XOR/Circle renderer code from 15-466 Game 4
 ************************************************************************************************************/
#pragma once

#include "Load.hpp"
#include "LitColorTextureProgram.hpp"
#include "ColorTextureProgram.hpp"
#include "load_save_png.hpp"

#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/glm.hpp>

#include <vector>

struct UIElement {

    // Data
    std::vector< uint32_t > data = {}; // data that will be passed as OpenGL texture data
    int data_width = 0;
    int data_height = 0;
    bool data_created = false;

    // OpenGL Variables
    GLuint buffer_for_color_texture_program = 0;
    GLuint tex = 0;
    GLuint vertex_buffer = 0;
    //format for the mesh data:
    struct Vertex {
        glm::vec2 Position;
        glm::u8vec4 Color;
        glm::vec2 TexCoord;

        // Vertex() : Position(0), Color(0), TexCoord(0) {};
    };
    std::vector< Vertex > attribs = {};

    // Calls load_png to get data
    void load_image_data(std::string filename, OriginLocation origin) {
        glm::uvec2 size;

        std::vector<glm::u8vec4> vec_data;
        load_png(filename, &size, &vec_data, origin);

        data_width = size.x;
        data_height = size.y;

        data.clear();
        for (int y = 0; y < data_height; y++) {
            for (int x = 0; x < data_width; x++) {
                int i = (data_height - y - 1) * data_width + x;
                assert(i >= 0 && i < data_height * data_width);

                data.emplace_back((((uint32_t)vec_data[i][0]) << 24) + // r
                                  (((uint32_t)vec_data[i][1]) << 16) + // g
                                  (((uint32_t)vec_data[i][2]) << 8)  + // b
                                  (((uint32_t)vec_data[i][3]))         // a
                                  );
            }
        }

        data_created = true;
    }

    /*****************************************
     * The next four functions are based on
     * Jim McCann's XOR/Circle Code
     *****************************************/
    void create_mesh(SDL_Window *window, float clip_center_x, float clip_center_y, float clip_height) {//,
                    //  uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        //---------------  create and upload texture data ----------------

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        // This code, up to "draw mesh" (exclusive) should be called only when creating new text
        // need a name for the texture object:
        glGenTextures(1, &tex); // store 1 texture name in tex1
        //attach texture object to the GL_TEXTURE_2D binding point:
        glBindTexture(GL_TEXTURE_2D, tex);

        //upload data: (see: https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml )
        glTexImage2D(
            GL_TEXTURE_2D, //target -- the binding point this call is uploading to
            0, //level -- the mip level; 0 = the base level
            // GL_RGBA8,  //internalformat -- channels and storage used on the GPU for the texture; GL_R8 means one channel, 8-bit fixed point
            GL_SRGB_ALPHA,// SRGB_ALPHA reset the gamma, internalformat -- channels and storage used on the GPU for the texture; GL_RGBA8 means four channels, 8-bit fixed point
            data_width, //width of texture
            data_height, //height of texture
            0, //border -- must be 0
            GL_RGBA, //format -- how the data to be uploaded is structured; GL_RGBA means... RGBA
            GL_UNSIGNED_INT_8_8_8_8, //type -- how each element of a pixel is stored
            data.data() //data -- pointer to the texture data
        );
        //set up texture sampling state:
        //clamp texture coordinate to edge of texture: (GL_REPEAT can also be useful in some cases)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        //use linear interpolation to magnify:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        //use trilinear interpolation (linear interpolation of linearly-interpolated mipmaps) to minify:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        //ask OpenGL to make the mipmaps for us:
        glGenerateMipmap(GL_TEXTURE_2D);

        //de-attach texture:
        glBindTexture(GL_TEXTURE_2D, 0);

        //----------- set up place to store mesh that references the data -----------

        //create a buffer object to store mesh data in:
        glGenBuffers(1, &vertex_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); //(buffer created when bound)
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        //create a vertex array object that references the buffer:
        buffer_for_color_texture_program = 0;
        glGenVertexArrays(1, &buffer_for_color_texture_program);
        glBindVertexArray(buffer_for_color_texture_program);

        //configure the vertex array object:

        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); //will take data from 'buffer'

        //set up Position to read from the buffer:
        //see https://registry.khronos.org/OpenGL-Refpages/gl4/html/glVertexAttribPointer.xhtml
        glVertexAttribPointer(
            color_texture_program->Position_vec4, //attribute
            2, //size
            GL_FLOAT, //type
            GL_FALSE, //normalized
            sizeof(Vertex), //stride
            (GLbyte *)0 + offsetof(Vertex, Position) //offset
        );
        glEnableVertexAttribArray(color_texture_program->Position_vec4);

        //set up Color to read from the buffer:
        glVertexAttribPointer( color_texture_program->Color_vec4, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (GLbyte *)0 + offsetof(Vertex, Color));
        glEnableVertexAttribArray(color_texture_program->Color_vec4);

        //set up TexCoord to read from the buffer:
        glVertexAttribPointer( color_texture_program->TexCoord_vec2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLbyte *)0 + offsetof(Vertex, TexCoord));
        glEnableVertexAttribArray(color_texture_program->TexCoord_vec2);

        //done configuring vertex array, so unbind things:
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        //----------- create and upload a mesh that references the data -----------

        attribs.reserve(4);
        //if drawn as a triangle strip, this will be a square with the lower-left corner at (0,0) and the upper right at (1,1):
        
        // set_position(window, clip_center_x, clip_center_y, clip_height, r, g, b, a);
        set_position(window, clip_center_x, clip_center_y, clip_height);

        GL_ERRORS();

    };

    void set_position(SDL_Window *window, float clip_center_x, float clip_center_y, float clip_height) { //,
                        //  uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        // get ratio
        // if (data_height == 0) {
        //     printf("Glyphs of string %s all had height of 0! Don't try to print just an empty character, please.\n", mytext);
        //     abort();
        // }

        assert(data_height != 0);

        int window_w, window_h;

        if (!SDL_GetWindowSize(window, &window_w, &window_h)) {
            printf("Failed to get window size?!\n");
            abort();
        }
        
        float clip_width = ((float)window_h / window_w) * clip_height * data_width / data_height;
        float left_clip = clip_center_x - (clip_width / 2); 
        float right_clip = clip_center_x + (clip_width / 2); 
        float top_clip = clip_center_y + (clip_height / 2); 
        float bottom_clip = clip_center_y - (clip_height / 2); 
        
        attribs.clear();
        attribs.reserve(4);

        attribs.emplace_back(Vertex {
            .Position = glm::vec2(left_clip, bottom_clip),
            // .Color = glm::u8vec4(r, g, b, a),
            .Color = glm::u8vec4(0xff, 0xff, 0xff, 0xff),
            .TexCoord = glm::vec2(0.0f, 0.0f),
        });

        attribs.emplace_back(Vertex {
            .Position = glm::vec2(left_clip, top_clip),
            // .Color = glm::u8vec4(r, g, b, a),
            .Color = glm::u8vec4(0xff, 0xff, 0xff, 0xff),
            .TexCoord = glm::vec2(0.0f, 1.0f),
        });

        attribs.emplace_back(Vertex {
            .Position = glm::vec2(right_clip, bottom_clip),
            // .Color = glm::u8vec4(r, g, b, a),
            .Color = glm::u8vec4(0xff, 0xff, 0xff, 0xff),
            .TexCoord = glm::vec2(1.0f, 0.0f),
        });

        attribs.emplace_back(Vertex {
            .Position = glm::vec2(right_clip, top_clip),
            // .Color = glm::u8vec4(r, g, b, a),
            .Color = glm::u8vec4(0xff, 0xff, 0xff, 0xff),
            .TexCoord = glm::vec2(1.0f, 1.0f),
        });

        //upload attribs to buffer:
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER, attribs.size() * sizeof((attribs)[0]), attribs.data(), GL_STREAM_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        GL_ERRORS();

    }

    void draw_mesh() {
        glUseProgram(color_texture_program->program);
		//draw with attributes from our buffer, as referenced by the vertex array:
		glBindVertexArray(buffer_for_color_texture_program);
		//draw using texture stored in tex:
		glBindTexture(GL_TEXTURE_2D, tex);

		
		//this particular shader program multiplies all positions by this matrix: (Jim: hmm, old naming style; I should have fixed that)
		// (just setting it to the identity, so Positions are directly in clip space)
		glUniformMatrix4fv(color_texture_program->OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

		//draw without depth testing (so will draw atop everything else):
		glDisable(GL_DEPTH_TEST);
		// //draw with alpha blending (so transparent parts of the texture look transparent):
		glEnable(GL_BLEND);
		//standard 'over' blending:
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		//actually draw:
		glDrawArrays(GL_TRIANGLE_STRIP, 0, (GLsizei)attribs.size());

		//turn off blending:
		glDisable(GL_BLEND);
		//...leave depth test off, since code that wants it will turn it back on

		//unbind texture, vertex array, program:
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindVertexArray(0);
		glUseProgram(0);

        GL_ERRORS();

    }

    ~UIElement() {
        // ----------- free allocated buffers / data -----------
        glDeleteVertexArrays(1, &buffer_for_color_texture_program);
        buffer_for_color_texture_program = 0;
        glDeleteBuffers(1, &vertex_buffer);
        vertex_buffer = 0;
        glDeleteTextures(1, &tex);
        tex = 0;
    }
 };