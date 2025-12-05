#include "Metaball.hpp"

#include <array>
#include <algorithm>
#include "glm/gtc/type_ptr.hpp"
#include "gl_errors.hpp"

void draw_cheese_metaballs(
    SoftBody const &soft,
    Scene::Camera const &camera,
    Scene::Transform const &cheese_xform,
    float metaball_time
) {
    //    std::cout << "[metaball] draw, count = " << soft.metaballs.size() << "\n";
    if (soft.metaballs.empty()) return;

    // 1. Compute matrices:
    glm::mat4 world_from_object = cheese_xform.make_world_from_local();
    glm::mat4 object_from_world = glm::inverse(world_from_object);

    glm::mat4 clip_from_world = camera.make_projection() * glm::mat4(camera.transform->make_local_from_world());
    glm::mat4 clip_from_object = clip_from_world * world_from_object;

    // 2. Build a pipeline instance from the global template:
    Scene::Drawable::Pipeline pipe = metaball_program_pipeline;

    // EYE in object space:
    glm::vec3 eyeWS = camera.transform->position;
    glm::vec3 eyeOS = glm::vec3(object_from_world * glm::vec4(eyeWS, 1.0f));

    // 3. Use program & set standard uniforms:
    glUseProgram(metaball_program->program);
    metaball_program->set_uniforms(clip_from_object,metaball_time, eyeOS);

    // 4. Pack metaballs:
    std::array<glm::vec4, MetaballProgram::MaxMetaballs> packed;
    int count = std::min<int>(int(soft.metaballs.size()), MetaballProgram::MaxMetaballs);

    
    // std::cout<< "cube"<< " pistiony" << cheese_xform.make_world_from_local()[3].y<< std::endl;
    // std::cout<< "cube"<< " pistionz" << cheese_xform.make_world_from_local()[3].z<< std::endl;

    for (int i = 0; i < count; ++i) {
        const auto &mb = soft.metaballs[i];

        // std::cout<< "metaball"<<i<< " pistion" << mb.position.y<< std::endl;
        // std::cout<< "metaball"<<i<< " pistion" << mb.position.z<< std::endl;
        txt

        // If mb.position is WORLD space:
        glm::vec3 posOS = glm::vec3(object_from_world * glm::vec4(mb.render_position, 1.0f));


        // If mb.position is already OBJECT space, just do:
        // glm::vec3 posOS = mb.position;

        packed[i] = glm::vec4(posOS, mb.radius);
    }

    glUniform1i(metaball_program->METABALL_COUNT_int, count);
    glUniform4fv(metaball_program->METABALLS_vec4, count, glm::value_ptr(packed[0]));
    glUniform1f(metaball_program->ISO_LEVEL_float, 0.67f); 

    // 5. Bind cube VAO and draw:

    glBindVertexArray(cheese_cube_vao);
    glDrawElements(GL_TRIANGLES, cheese_cube_index_count,GL_UNSIGNED_SHORT, (void*)0);
    glBindVertexArray(0);

    glUseProgram(0);
    GL_ERRORS();
}