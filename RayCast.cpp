#include "RayCast.hpp"

#include <string>
#include <algorithm>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

// code adapted from https://medium.com/@logandvllrd/how-to-pick-a-3d-object-using-raycasting-in-c-39112aed1987
Ray screen_point_to_world_ray(Scene::Camera* cam, glm::vec2 mouse_px, glm::uvec2 drawable_px)
{
	// NDC for the mouse
	float x = (mouse_px.x / float(drawable_px.x)) * 2.0f - 1.0f;
	float y = 1.0f - (mouse_px.y / float(drawable_px.y)) * 2.0f;

	glm::vec4 clip_near(x, y, -1.0f, 1.0f);
	glm::vec4 clip_far(x, y, 1.0f, 1.0f);

	glm::mat4 cam_l2w = glm::mat4(cam->transform->make_parent_from_local());
	// glm::mat4 w2c = glm::inverse(cam_l2w);
	glm::mat4 proj = cam->make_projection();
	glm::mat4 inv_proj = glm::inverse(proj);

	// near point in eye space
	glm::vec4 eye_near = inv_proj * clip_near;
	if (std::abs(eye_near.w) > 1e-6f)
		eye_near /= eye_near.w;
	glm::vec3 p_near_world = glm::vec3(cam_l2w * eye_near);

	// far point
	glm::vec4 eye_far = inv_proj * clip_far;

	glm::vec3 dir_world;
	if (std::abs(eye_far.w) > 1e-6f)
	{
		eye_far /= eye_far.w;
		glm::vec3 p_far_world = glm::vec3(cam_l2w * eye_far);
		dir_world = glm::normalize(p_far_world - p_near_world);
	}
	else
	{
		// w == 0 => homogeneous direction. No divide, just rotate into world
		glm::vec3 dir_eye = glm::normalize(glm::vec3(eye_far));
		dir_world = glm::normalize(glm::mat3(cam_l2w) * dir_eye);
	}

	Ray r;
	r.origin = p_near_world;
	r.dir = dir_world;
	return r;
}

void world_box(Scene::Transform* t, glm::vec3& center, glm::vec3& half)
{
	glm::mat4x3 M = t->make_parent_from_local();
	glm::vec3 X = glm::vec3(M[0]);
	glm::vec3 Y = glm::vec3(M[1]);
	glm::vec3 Z = glm::vec3(M[2]);
	center = glm::vec3(M[3]);

	glm::vec3 ax = glm::abs(X);
	glm::vec3 ay = glm::abs(Y);
	glm::vec3 az = glm::abs(Z);

	half = (ax + ay + az) * 0.95f;
}

bool ray_vs_rotated_platform_box(
    const Ray &world_ray,
    Scene::Transform *object,
    float *t_hit_out,
    glm::vec3 *hit_pos_out
) {
    if (!object) return false;

    // ----- 1. Build object transform pieces (same basis as your collision) -----
    glm::vec3 object_pos = glm::vec3(object->position);
    glm::quat rotation   = object->rotation;
    glm::quat invRotation = glm::inverse(rotation);
    glm::vec3 size       = object->scale;

    // Local-space AABB in the object's frame (like your boxMin/boxMax, but symmetric):
    float halfY = size.y;
    float halfZ = size.z;
    glm::vec3 boxMin(0.0f, -halfY, -halfZ);
    glm::vec3 boxMax(0.0f,  halfY,  halfZ);

    // ----- 2. Transform ray into object-local space -----
    // origin_local = R^{-1} * (origin_world - object_pos)
    glm::vec3 origin_local = invRotation * (world_ray.origin - object_pos);
    glm::vec3 dir_local    = invRotation * world_ray.dir;

    // We'll do 2D slab intersection in Y/Z, ignoring X (since box is x=0 plane):
    // Parametric form: P(t) = origin_local + t * dir_local
    // We want t such that P(t).y,z is within [boxMin.y,z, boxMax.y,z].

    float tmin = 0.0f;
    float tmax = std::numeric_limits<float>::infinity();

    // Helper to intersect 1D slabs:
    auto intersect_slab = [&](float ray_o, float ray_d, float minb, float maxb, float &tmin, float &tmax) -> bool {
        const float eps = 1e-30f;
        if (std::abs(ray_d) < eps) {
            // Ray parallel to axis: must be inside slab to hit
            if (ray_o < minb || ray_o > maxb) return false;
            return true; // tmin,tmax unchanged
        }
        float invd = 1.0f / ray_d;
        float t0 = (minb - ray_o) * invd;
        float t1 = (maxb - ray_o) * invd;
        if (t0 > t1) std::swap(t0, t1);
        tmin = std::max(tmin, t0);
        tmax = std::min(tmax, t1);
        return (tmax >= tmin);
    };

    // Y slab:
    if (!intersect_slab(origin_local.y, dir_local.y, boxMin.y, boxMax.y, tmin, tmax)) {
        return false;
    }
    // Z slab:
    if (!intersect_slab(origin_local.z, dir_local.z, boxMin.z, boxMax.z, tmin, tmax)) {
        return false;
    }

    if (tmax < 0.0f) return false; 

    float t_hit = (tmin >= 0.0f) ? tmin : tmax;
    if (t_hit < 0.0f) return false;

    glm::vec3 local_hit  = origin_local + t_hit * dir_local;
    glm::vec3 world_hit  = rotation * local_hit + object_pos;

    if (t_hit_out)  *t_hit_out  = t_hit;
    if (hit_pos_out) *hit_pos_out = world_hit;
    return true;
}
bool ray_box_intersect(const Ray& r, const glm::vec3& c, const glm::vec3& h, float* t_out)
{
	glm::vec3 minb = c - h, maxb = c + h;
	glm::vec3 invd = 1.0f / glm::vec3(
		(r.dir.x != 0.0f ? r.dir.x : 1e-30f),
		(r.dir.y != 0.0f ? r.dir.y : 1e-30f),
		(r.dir.z != 0.0f ? r.dir.z : 1e-30f));
	glm::vec3 t0 = (minb - r.origin) * invd;
	glm::vec3 t1 = (maxb - r.origin) * invd;
	glm::vec3 tmin3 = glm::min(t0, t1);
	glm::vec3 tmax3 = glm::max(t0, t1);
	float tmin = std::max(std::max(tmin3.x, tmin3.y), tmin3.z);
	float tmax = std::min(std::min(tmax3.x, tmax3.y), tmax3.z);

	/*std::cout << "    [intersection] t0=(" << t0.x << "," << t0.y << "," << t0.z << ")"
		<< " t1=(" << t1.x << "," << t1.y << "," << t1.z << ")"
		<< " -> tmin=" << tmin << " tmax=" << tmax << "\n";*/

	if (tmax >= glm::max(tmin, 0.0f))
	{
		if (t_out)
			*t_out = tmin;
		return true;
	}
	return false;
}