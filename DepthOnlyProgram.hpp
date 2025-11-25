#include "GL.hpp"
#include "Load.hpp"
#include "Scene.hpp"
#include "ShadowedColorTextureProgram.hpp"

struct DepthOnlyProgram {
	//opengl program object:
	GLuint program = 0;

	//uniform locations:
	GLuint CLIP_FROM_OBJECT_mat4 = -1U;

	DepthOnlyProgram();
	
	// void render_shadow_atlas(
	// 	Framebuffers const &framebuffers,
	// 	std::vector<Scene::Light> const &lights,
	// 	Scene const &scene
	// ); 
};

extern Load< DepthOnlyProgram > depth_only_program;

extern Scene::Drawable::Pipeline depth_only_program_pipeline;
