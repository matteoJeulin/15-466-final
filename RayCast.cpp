#include "RayCast.hpp"

#include <string>
#include <algorithm>
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

	half = (ax + ay + az) * 0.5f;
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