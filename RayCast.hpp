#pragma once
#include <glm/glm.hpp>
#include <vector>

#include "Scene.hpp"

struct Ray {
	glm::vec3 origin;
	glm::vec3 dir; // normalized
};

Ray screen_point_to_world_ray(Scene::Camera* cam, glm::vec2 mouse_px, glm::uvec2 drawable_px);
void world_box(Scene::Transform* t, glm::vec3& center, glm::vec3& half);
bool ray_vs_rotated_platform_box(const Ray &world_ray, Scene::Transform *object, float *t_hit_out,glm::vec3 *hit_pos_out);
bool ray_box_intersect(const Ray& r, const glm::vec3& c, const glm::vec3& h, float* t_out = nullptr);