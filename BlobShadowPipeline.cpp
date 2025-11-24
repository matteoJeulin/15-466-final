#include "BlobShadowPipeline.hpp"
#include "gl_compile_program.hpp"
#include "gl_errors.hpp"
#include "Scene.hpp"
#include "Load.hpp"

Scene::Drawable::Pipeline blob_shadow_pipeline;

// global loader like your other programs:
Load< BlobShadowProgram > blob_shadow_program(LoadTagEarly, []() -> BlobShadowProgram const * {
    BlobShadowProgram *ret = new BlobShadowProgram();

    // build pipeline template:
    blob_shadow_pipeline.program = ret->program;
    blob_shadow_pipeline.CLIP_FROM_OBJECT_mat4 = ret->CLIP_FROM_OBJECT_mat4;
    blob_shadow_pipeline.LIGHT_FROM_OBJECT_mat4x3 = -1;
    blob_shadow_pipeline.LIGHT_FROM_NORMAL_mat3 = -1;

    // no textures needed, but pipeline.textures[0] can stay default

    return ret;
});

BlobShadowProgram::BlobShadowProgram() {
    program = gl_compile_program(
        // vertex shader:
        "#version 330\n"
        "uniform mat4 CLIP_FROM_OBJECT;\n"
        "in vec4 Position;\n"
        "in vec2 TexCoord;\n"
        "out vec2 vUV;\n"
        "void main() {\n"
        "    gl_Position = CLIP_FROM_OBJECT * Position;\n"
        "    vUV = TexCoord;\n"
        "}\n",
        // fragment shader:
        "#version 330\n"
        "in vec2 vUV;\n"
        "out vec4 fragColor;\n"
        "uniform vec4 BLOB_COLOR; // rgb = shadow color, a = max alpha\n"
        "void main() {\n"
        "    // UV [0,1] -> [-1,1] with center at 0\n"
        "    vec2 p = vUV * 2.0 - 1.0;\n"
        "    float r = length(p);\n"
        "    // soft edge: inside radius ~0.9 opaque, fade to 0 at 1.0\n"
        "    float edge_inner = 0.0;\n"
        "    float edge_outer = 1.0;\n"
        "    float alphaFactor = 1.0 - smoothstep(edge_inner, edge_outer, r);\n"
        "    float alpha = BLOB_COLOR.a * alphaFactor;\n"
        "    if (alpha <= 0.001) discard;\n"
        "    fragColor = vec4(BLOB_COLOR.rgb, alpha);\n"
        "}\n"
    );

    // attributes:
    Position_vec4 = glGetAttribLocation(program, "Position");
    TexCoord_vec2 = glGetAttribLocation(program, "TexCoord");

    // uniforms:
    CLIP_FROM_OBJECT_mat4 = glGetUniformLocation(program, "CLIP_FROM_OBJECT");
    BLOB_COLOR_vec4       = glGetUniformLocation(program, "BLOB_COLOR");
}

BlobShadowProgram::~BlobShadowProgram() {
    glDeleteProgram(program);
    program = 0;
}