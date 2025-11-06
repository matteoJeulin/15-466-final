#include "Stove.hpp"
#include "LitColorTextureProgram.hpp"

#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <limits>
#include <iostream>

StoveSystem::StoveSystem() {}
StoveSystem::~StoveSystem() {
    for (auto& tex : tint_lvl_) {
        if (tex) { glDeleteTextures(1, &tex); tex = 0; }
    }
}

// texture block set for stove, chatGPT
GLuint StoveSystem::make_solid_tex(glm::u8vec4 rgba) {
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &rgba);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

void StoveSystem::init(Scene& scene) {
    scene_ = &scene;
    knobs_.clear();
    plates_.clear();

    // collect transforms by name prefix:
    for (auto& t : scene.transforms) {
        if (t.name.size() >= 6 && t.name.rfind("Switch", 0) == 0) {
            knobs_.push_back(Knob{ &t, 0, -1 });
        }
        else if (t.name.size() >= 5 && t.name.rfind("Plate", 0) == 0) {
            plates_.push_back(Plate{ &t, nullptr, 0 });
        }
    }

    // match each Plate transform to its drawable :
    for (auto& d : scene.drawables) {
        for (auto& p : plates_) {
            if (d.transform == p.t) { p.d = &d; break; }
        }
    }

    // pair each knob to the nearest plate
    for (auto& k : knobs_) {
        k.plate_index = find_nearest_plate_index(k.t);
    }

    // 1 x 1 tint textures:
    tint_lvl_[0] = make_solid_tex({ 255,255,255,255 }); // off
    tint_lvl_[1] = make_solid_tex({ 255, 90, 60,255 }); // warm
    tint_lvl_[2] = make_solid_tex({ 255, 40, 15,255 }); // hot
    tint_lvl_[3] = make_solid_tex({ 255,  0,  0,255 }); // very hot

    // all plates appear level 0 by default
    for (int i = 0; i < (int)plates_.size(); ++i) {
        apply_plate_tint_for_level(i, 0);
        plates_[i].level = 0;
    }

    /*std::cout << "[StoveSystem] (names-only) "
        << "Switches=" << knobs_.size() << ", Plates=" << plates_.size() << "\n";*/
}

int StoveSystem::get_level_for_plate(Scene::Transform* plate_t) const {
    if (!plate_t) return 0;
    for (const auto& p : plates_) {
        if (p.t == plate_t) return std::clamp(p.level, 0, 3);
    }
    return 0; // default to 0
}

int StoveSystem::find_nearest_plate_index(Scene::Transform* from) const {
    if (plates_.empty() || !from) return -1;
    int best = -1;
    float best_d2 = std::numeric_limits<float>::max();

    glm::vec3 fp = from->position;
    for (int i = 0; i < (int)plates_.size(); ++i) {
        const auto* t = plates_[i].t;
        if (!t) continue;
        glm::vec3 p = t->position;
        float d2 = glm::dot(p - fp, p - fp);
        if (d2 < best_d2) { best_d2 = d2; best = i; }
    }
    return best;
}

int* StoveSystem::find_knob_state(Scene::Transform* t) {
    for (auto& k : knobs_) if (k.t == t) return &k.state;
    return nullptr;
}

void StoveSystem::rotate_knob(Scene::Transform* t) {
    t->rotation = t->rotation * glm::angleAxis(glm::radians(90.0f), glm::vec3(0, 0, 1));
}

void StoveSystem::apply_plate_tint_for_level(int plate_index, int level) {
    if (plate_index < 0 || plate_index >= (int)plates_.size()) return;
    Plate& p = plates_[plate_index];
    if (!p.d) return; // non-rendered plate

    int L = std::clamp(level, 0, 3);
    p.d->pipeline.textures[0].texture = tint_lvl_[L];
    p.d->pipeline.textures[0].target = GL_TEXTURE_2D;
    p.level = L;
}

void StoveSystem::set_level_for_plate(Scene::Transform* plate_t, int level) {
    if (!plate_t) return;
    for (int i = 0; i < (int)plates_.size(); ++i) {
        if (plates_[i].t == plate_t) {
            apply_plate_tint_for_level(i, level);
            return;
        }
    }
}

void StoveSystem::set_level(int level) {
    int L = std::clamp(level, 0, 3);
    for (auto& k : knobs_) k.state = L;
}

bool StoveSystem::try_toggle(const Ray& ray, int* out_level) {
    if (!scene_) return false;

    // Find nearest Switch* AABB hit
    Scene::Transform* best_switch = nullptr;
    float best_t = std::numeric_limits<float>::max();

    for (auto& k : knobs_) {
        glm::vec3 c, h;
        world_box(k.t, c, h);
        float tval;
        if (ray_box_intersect(ray, c, h, &tval) && tval < best_t) {
            best_t = tval;
            best_switch = k.t;
        }
    }
    if (!best_switch) return false;

    // change knob state
    rotate_knob(best_switch);
    int* state = find_knob_state(best_switch);
    if (!state) return false;
    *state = (*state + 1) % 4;

    // find the knob object to get its paired plate
    for (auto& k : knobs_) {
        if (k.t == best_switch) {
            if (k.plate_index >= 0) {
                apply_plate_tint_for_level(k.plate_index, *state);
            }
            break;
        }
    }

    if (out_level) *out_level = *state;

    //std::cout << "[StoveSystem] Toggled " << best_switch->name
    //    << " -> level " << *state << " (paired plate updated)\n";
    return true;
}
