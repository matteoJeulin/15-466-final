#pragma once

#include "GL.hpp"
#include "Scene.hpp"
#include "Load.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "SoftBody.hpp"

// Metaball raymarching program that renders a metaball volume
// inside a bounding box mesh (e.g. a cube) in object space.

struct MetaballProgram {
	MetaballProgram();
	~MetaballProgram();

	static constexpr int MaxLights     = 0;
	static constexpr int MaxMetaballs  = 64; // tweak as needed

	GLuint program = 0;

	// Uniform locations:
	GLint CLIP_FROM_OBJECT_mat4 = -1;  // used to compute final position & depth
	GLint TIME_float            = -1;  // optional wobble / animation
	GLint EYE_vec3              = -1;  // camera in *object space*

	GLint METABALL_COUNT_int    = -1;  // active metaball count
	GLint METABALLS_vec4        = -1;  // vec4(position.xyz, radius) array
    GLint ISO_LEVEL_float = -1;

	// Attributes:
	GLint Position_vec4 = -1;         // cube vertex position
	// (no TexCoord needed; we use object-space position)

	// // Standard uniform helper (you still need to set metaballs separately):
	// void set_uniforms(Scene::Drawable::Pipeline const &pipeline) const {
	// 	glUniformMatrix4fv(CLIP_FROM_OBJECT_mat4, 1, GL_FALSE, glm::value_ptr(pipeline.CLIP_FROM_OBJECT));
	// 	glUniform1f(TIME_float, pipeline.time);
	// 	// IMPORTANT: for this shader, pipeline.EYE must be eye position
	// 	// in *object space* of the metaball object
	// 	glUniform3fv(EYE_vec3, 1, glm::value_ptr(pipeline.EYE));
	// }

     void set_uniforms(glm::mat4 const &clip_from_object,
                      float time,
                      glm::vec3 const &eyeOS) const;


};

extern Load< MetaballProgram > metaball_program;
extern Scene::Drawable::Pipeline metaball_program_pipeline;

extern GLuint cheese_cube_vao;
extern GLsizei cheese_cube_index_count;