#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>

#include "Scene.hpp"
#include "RayCast.hpp" 

class StoveSystem {
public:
    StoveSystem();
    ~StoveSystem();

    // call once after the Scene is ready
    void init(Scene& scene);

    bool try_toggle(const Ray& ray, int* out_level = nullptr);

    void set_level_for_plate(Scene::Transform* plate_t, int level);

    void set_level(int level);

    int get_level_for_plate(Scene::Transform* plate_t) const;


private:
    struct Knob {
        Scene::Transform* t = nullptr;
        int state = 0;             // 0..3
        int plate_index = -1;      // nearest Plate* at init
    };
    struct Plate {
        Scene::Transform* t = nullptr;
        Scene::Drawable* d = nullptr; // drawable that references t
        int level = 0;                 // remember its own level
    };

    int* find_knob_state(Scene::Transform* t);
    void rotate_knob(Scene::Transform* t);

    // tint one plate
    void apply_plate_tint_for_level(int plate_index, int level);

    // helpers
    static GLuint make_solid_tex(glm::u8vec4 rgba);
    int  find_nearest_plate_index(Scene::Transform* from) const;

private:
    Scene* scene_ = nullptr;
    std::vector<Knob>  knobs_;
    std::vector<Plate> plates_;

    // 1x1 tint textures:
    GLuint tint_lvl_[4] = { 0,0,0,0 };
};

