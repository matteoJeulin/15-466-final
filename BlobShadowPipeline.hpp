#pragma once

#include "GL.hpp"
#include <glm/glm.hpp>
#include "Load.hpp"
#include "Scene.hpp"

struct BlobShadowProgram {
    BlobShadowProgram();
    ~BlobShadowProgram();

    GLuint program = 0;

    // attributes:
    GLint Position_vec4 = -1;
    GLint TexCoord_vec2 = -1;

    // uniforms:
    GLint CLIP_FROM_OBJECT_mat4 = -1;
    GLint BLOB_COLOR_vec4 = -1;
};

extern Load< BlobShadowProgram> blob_shadow_program;

//For convenient scene-graph setup, copy this object:
// NOTE: by default, has texture bound to 1-pixel white texture -- so it's okay to use with vertex-color-only meshes.
extern Scene::Drawable::Pipeline blob_shadow_pipeline;