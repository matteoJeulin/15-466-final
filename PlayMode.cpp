#include "PlayMode.hpp"
#include "Mode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "read_write_chunk.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <fstream>
#include <cmath>
#include <string>
#include <algorithm>

static PlayMode::Ray screen_point_to_world_ray(Scene::Camera *cam, glm::vec2 mouse_px, glm::uvec2 drawable_px)
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

	PlayMode::Ray r;
	r.origin = p_near_world;
	r.dir = dir_world;
	return r;
}

static void world_box(Scene::Transform *t, glm::vec3 &center, glm::vec3 &half)
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

static bool ray_box_intersect(const PlayMode::Ray &r, const glm::vec3 &c, const glm::vec3 &h, float *t_out = nullptr)
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

constexpr float PI = 3.14159265358979323846f;

GLuint level_meshes_for_lit_color_texture_program = 0;
Load<MeshBuffer> level_meshes(LoadTagDefault, []() -> MeshBuffer const *
							  {
	MeshBuffer const *ret = new MeshBuffer(data_path("Cheese.pnct"));
	level_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret; });

Load<Scene> level_scene(LoadTagDefault, []() -> Scene const *
						{ return new Scene(data_path("Cheese.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name)
										   {
												if ((transform->name.substr(0, 9) == "Collision"|| transform->name == "Cheese_Wheel")) {
												// NOTE: Do NOT create a Scene::Drawable for collision meshes.
												// The transforms will still be loaded into scene.transforms.
												return; // Skip the rest of the function for this transform
												}
												 Mesh const &mesh = level_meshes->lookup(mesh_name);

												 scene.drawables.emplace_back(transform);
												 Scene::Drawable &drawable = scene.drawables.back();

												 drawable.pipeline = lit_color_texture_program_pipeline;

												 drawable.pipeline.vao = level_meshes_for_lit_color_texture_program;
												 drawable.pipeline.type = mesh.type;
												 drawable.pipeline.start = mesh.start;
												 drawable.pipeline.count = mesh.count; }); });

PlayMode::PlayMode() : scene(*level_scene)
{
	for (auto &transform : scene.transforms)
	{
		std::cout << transform.name << std::endl;
		if (transform.name == "Wheel_Prototype")
			cheese_wheel = &transform;
		else if (transform.name == "Cheese_Wheel")
			collision_cheese_wheel = &transform;
		else if (transform.name == "Hot_Plate")
			hot_plate = &transform;
		else if (transform.name == "Cold_Plate")
			cold_plate = &transform;
		else if (transform.name == "Switch")
			switch_1 = &transform;
		else if (transform.name == "Switch2")
			switch_2 = &transform;
		else if (transform.name == "Gate")
		{
			gate = &transform;
		}
		else if (transform.name.substr(0, 9) == "Collision")
		{
			if (transform.name != "Collision_Hot_Plate" && transform.name != "Collision_Cold_Plate"&& transform.name != "Collision_Hot_Plate" && transform.name != "Collision_Cold_Plate")
			{
				collision_platforms.emplace_back(&transform);
			}

			if (transform.name == "Collision_CounterTop")
			{
				counter_top = &transform;
			}

			
		}
		else if (transform.name == "Cube" || transform.name == "Cube.001")
			{
				collision_plates.emplace_back(&transform);
			}
	}
	if (cheese_wheel == nullptr)
		throw std::runtime_error("Cheese not found.");
	if (hot_plate == nullptr)
		throw std::runtime_error("Hot plate not found.");
	if (cold_plate == nullptr)
		throw std::runtime_error("Cold plate not found.");

	// get pointer to camera for convenience:
	if (scene.cameras.size() != 1)
		throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	Scene::Drawable *cheese_drawable = nullptr;
	for (auto &drawable : scene.drawables)
	{
		if (drawable.transform == cheese_wheel)
		{
			cheese_drawable = &drawable;
			break;
		}
	}

	level_meshes->print_all_meshes();

	if (cheese_drawable == nullptr)
		throw std::runtime_error("Cheese wheel drawable not found.");

	cheese_mesh = &(level_meshes->lookup("Wheel_Prototype"));

	theta = cheese_wheel->rotation;

	const size_t vertex_stride = sizeof(DynamicMeshBuffer::Vertex);

	const GLintptr offset_bytes = (GLintptr)cheese_mesh->start * vertex_stride;
	const GLsizeiptr size_bytes = (GLsizeiptr)cheese_mesh->count * vertex_stride;

	std::vector<DynamicMeshBuffer::Vertex> initial_vertices(cheese_mesh->count);
	glBindBuffer(GL_ARRAY_BUFFER, level_meshes->buffer);

	glGetBufferSubData(
		GL_ARRAY_BUFFER,
		offset_bytes,			// Byte offset to start reading
		size_bytes,				// Total bytes to read
		initial_vertices.data() // Destination pointer in CPU memory
	);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	initial_cheese_vertices_cpu = initial_vertices;
	cheese_vertices_cpu = initial_vertices;

	initial_cheese.set(initial_vertices.data(), initial_vertices.size(), GL_DYNAMIC_DRAW);
	melted_cheese.set(initial_vertices.data(), initial_vertices.size(), GL_DYNAMIC_DRAW);

	// change static to dynamic mesh
	cheese_lit_color_texture_program = initial_cheese.make_vao_for_program(lit_color_texture_program->program);
	melted_cheese_lit_color_texture_program = initial_cheese.make_vao_for_program(lit_color_texture_program->program);
	cheese_drawable->pipeline.vao = cheese_lit_color_texture_program;
	cheese_drawable->pipeline.type = cheese_mesh->type;
	cheese_drawable->pipeline.start = 0; // Starts from 0 in the new buffer
	cheese_drawable->pipeline.count = cheese_mesh->count;
}

PlayMode::~PlayMode()
{
	glDeleteVertexArrays(1, &cheese_lit_color_texture_program);
	cheese_lit_color_texture_program = 0;
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size)
{

	if (evt.type == SDL_EVENT_KEY_DOWN)
	{
		if (evt.key.key == SDLK_ESCAPE)
		{
			SDL_SetWindowRelativeMouseMode(Mode::window, false);
			return true;
		}
		else if (evt.key.key == SDLK_A || evt.key.key == SDLK_LEFT)
		{
			left.downs += 1;
			left.pressed = true;
			return true;
		}
		else if (evt.key.key == SDLK_D || evt.key.key == SDLK_RIGHT)
		{
			right.downs += 1;
			right.pressed = true;
			return true;
		}
		else if (evt.key.key == SDLK_SPACE)
		{
			jump.downs += 1;
			jump.pressed = true;
			return true;
		}
		else if (evt.key.key == SDLK_J)
		{
			debug_heat.downs += 1;
			debug_heat.pressed = true;
			return true;
		}
	}
	else if (evt.type == SDL_EVENT_KEY_UP)
	{
		if (evt.key.key == SDLK_A || evt.key.key == SDLK_LEFT)
		{
			left.pressed = false;
			return true;
		}
		else if (evt.key.key == SDLK_D || evt.key.key == SDLK_RIGHT)
		{
			right.pressed = false;
			return true;
		}
		else if (evt.key.key == SDLK_SPACE)
		{
			jump.pressed = false;
			return true;
		}
		else if (evt.key.key == SDLK_M)
		{
			mute.pressed = false;
			return true;
		}
		else if (evt.key.key == SDLK_J)
		{
			debug_heat.pressed = false;
			return true;
		}
	}
	else if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
	{
		// if (SDL_GetWindowRelativeMouseMode(Mode::window) == false)
		// {
		// 	SDL_SetWindowRelativeMouseMode(Mode::window, true);
		// 	return true;
		// }

		if (evt.button.button == SDL_BUTTON_LEFT)
		{
			glm::vec2 mouse_win(float(evt.button.x), float(evt.button.y));
			glm::vec2 scale = glm::vec2(last_drawable_px) / glm::vec2(window_size);
			// glm::vec2 mouse_px = glm::vec2(float(evt.button.x), float(evt.button.y));
			glm::vec2 mouse_px = mouse_win * scale;

			PlayMode::Ray r = screen_point_to_world_ray(camera, mouse_px, last_drawable_px);
			/*	std::cout << "[Sizes] window=(" << window_size.x << "," << window_size.y
					<< ") drawable=(" << last_drawable_px.x << "," << last_drawable_px.y << ")\n";*/
			std::cout << "[Ray] O=(" << r.origin.x << "," << r.origin.y << "," << r.origin.z
					  << ") D=(" << r.dir.x << "," << r.dir.y << "," << r.dir.z << ")\n";
			last_ray = r;
			has_last_ray = true;

			Scene::Transform *hit = nullptr;
			float best_t = std::numeric_limits<float>::max();

			auto try_hit = [&](Scene::Transform *t)
			{
				if (!t)
					return;
				glm::vec3 c, h;
				world_box(t, c, h);
				/*std::cout << "[bounding box] " << t->name << " C=(" << c.x << "," << c.y << "," << c.z
					<< ") H=(" << h.x << "," << h.y << "," << h.z << ")\n";*/
				float tval;
				if (ray_box_intersect(r, c, h, &tval) && tval < best_t)
				{
					std::cout << "  -> HIT at t=" << tval << "\n";
					if (tval < best_t)
					{
						best_t = tval;
						hit = t;
					}
				}
				else
				{
					std::cout << "  -> miss\n";
				}
			};

			try_hit(switch_1);
			try_hit(switch_2);

			if (hit)
			{
				// Toggle stove state
				melt_delta *= -1.0f;

				// switch rotation
				hit->rotation = hit->rotation * glm::angleAxis(0.2f, glm::vec3(0, 0, 1));

				std::cout << "Switch toggled:" << hit->name.c_str() << "melt_delta now " << melt_delta << std::endl;
				return true;
			}
		}
	}

	return false;
}

bool PlayMode::collide(Scene::Transform *object)
{
	glm::vec3 object_pos = glm::vec3(object->position);
	object_pos.x = 0.0f;
	glm::vec3 object_size = object->scale;

	glm::vec3 &cheese_pos = collision_cheese_wheel->position;
	cheese_pos.x = 0.0f;
	glm::vec3 cheese_size = collision_cheese_wheel->scale;

	// Transform player position from world space to object's local space
	glm::quat invRotation = glm::inverse(object->rotation);
	glm::vec3 playerCenter = invRotation * (cheese_pos - object_pos);
	playerCenter.x = 0.0f;

	// Taken from https://developer.mozilla.org/en-US/docs/Games/Techniques/3D_collision_detection
	// Check the collision between a circle and a bounding box
	auto intersects = ([](glm::vec3 sphereCenter, float sphereRadius, glm::vec3 boxMin, glm::vec3 boxMax)
					   {
		// get box closest point to sphere center by clamping
		float y = std::max(boxMin.y, std::min(sphereCenter.y, boxMax.y));
		float z = std::max(boxMin.z, std::min(sphereCenter.z, boxMax.z));

		float distance = std::sqrt(
					(y - sphereCenter.y) * (y - sphereCenter.y) +
					(z - sphereCenter.z) * (z - sphereCenter.z)
				);

		std::pair<bool, glm::vec3> solution(distance <= sphereRadius,
											glm::vec3(0.0f, y, z));

		return solution; });

	// Bounding box of the object
	glm::vec3 boxMin = glm::vec3(0.0f, -object_size.y, -object_size.z);
	glm::vec3 boxMax = glm::vec3(0.0f, object_size.y, object_size.z);

	std::pair<bool, glm::vec3> intersection = intersects(playerCenter, cheese_size.y, boxMin, boxMax);

	if (intersection.first)
	{
		// Inspiration taken from https://www.toptal.com/game/video-game-physics-part-ii-collision-detection-for-solid-objects
		glm::vec3 closestPoint = intersection.second;

		// If we are in a gate and melted enough, pass through it
		if (object == gate && melt_level > (MELT_MIN + MELT_MAX) / 2)
			return true;

		// Transform clamped point to world space - this is the actual collision point
		glm::vec3 closestPointWorld = object->rotation * closestPoint + object_pos;

		// Calculate vector from collision point to player center
		// This is the correct direction for the collision normal
		glm::vec3 collisionToPlayer = cheese_pos - closestPointWorld;
		float distance = glm::length(collisionToPlayer);

		// Calculate penetration and collision normal
		float penetration = cheese_size.y - distance;

		// The collision normal points from the closest point toward the player center
		// If distance is too small, we're at the center and need a fallback
		glm::vec3 actualNormal;
		if (distance > 0.0001f)
		{
			actualNormal = glm::normalize(collisionToPlayer);
		}
		else
		{
			// Fallback: use the direction from box center to player
			glm::vec3 boxCenterWorld = object_pos;
			boxCenterWorld.x = 0.0f;
			glm::vec3 centerToPlayer = cheese_pos - boxCenterWorld;
			actualNormal = (glm::length(centerToPlayer) > 0.0001f)
							   ? glm::normalize(centerToPlayer)
							   : glm::vec3(0.0f, 0.0f, 1.0f); // Default upward if all else fails
		}

		// std::cout << "Cheese pos: " << cheese_pos.x << " " << cheese_pos.y << " " << cheese_pos.z << std::endl;
		// std::cout << "Cheese speed: " << cheeseSpeed.x << " " << cheeseSpeed.y << " " << cheeseSpeed.z << std::endl;
		// std::cout << "Actual Normal: " << actualNormal.x << " " << actualNormal.y << " " << actualNormal.z << std::endl;

		// Move player along the actual collision normal
		// Apply the advanced sliding resolution only if there is penetration
		if (penetration > 0.0001f)
		{
			cheese_pos += actualNormal * penetration;
		}

		// Cancel velocity component along the collision normal (world space)
		float velocityAlongNormal = glm::dot(cheeseSpeed, actualNormal);

		if (velocityAlongNormal < 0.0f)
		{ // Only if moving into the surface
			cheeseSpeed -= velocityAlongNormal * actualNormal;
			cheeseSpeed.x = 0.0f;
		}

		// Only set platform if the normal is pointing upward (player landed on top)
		// Check if the world normal has a significant upward (positive Z) component
		if (actualNormal.z > 0.7f)
		{ // cos(45°) ≈ 0.707, so steeper than 45° upward
			cheese_platform = object;
		}
		else
		{
			cheese_platform = nullptr;
		}

		return true;
	}

	return false;
}

void PlayMode::update(float elapsed)
{

	// combine inputs into a move:
	if (left.pressed && !right.pressed)
		cheeseSpeed.y = std::max(cheeseSpeed.y - cheeseAcceleration * elapsed, -cheeseMaxSpeed);
	if (!left.pressed && right.pressed)
		cheeseSpeed.y = std::min(cheeseSpeed.y + cheeseAcceleration * elapsed, cheeseMaxSpeed);

	if (jump.pressed && !jumping && cheese_platform != nullptr)
	{
		cheeseSpeed.z = jumpSpeed;
		jump.pressed = false;
		jumping = true;
	}

	// Apply inertia to get the player down to 0 speed.
	if ((!left.pressed && !right.pressed) || (left.pressed && cheeseSpeed.y > 0) || (right.pressed && cheeseSpeed.y < 0))
	{
		cheeseSpeed.y -= cheeseSpeed.y * elapsed * 10;
	}

	cheeseSpeed.z -= gravity * elapsed;

	float rotation_angle = cheeseSpeed.y * elapsed;

	// y-axis is the forward/backward direction and the x-axis is the right/left direction
	collision_cheese_wheel->position += cheeseSpeed.y * glm::vec3(0.0f, 1.0f, 0.0f) * elapsed + cheeseSpeed.z * glm::vec3(0.0f, 0.0f, 1.0f) * elapsed;

	camera->transform->position += cheeseSpeed.y * glm::vec3(0.0f, 1.0f, 0.0f) * 0.9f * elapsed; // need to change this
	camera->transform->position.z = collision_cheese_wheel->position.z + 30.0f; // need to change this


	glm::quat rotation = glm::angleAxis(rotation_angle, glm::vec3(1, 0.0f, 0.0f));
	theta = theta * rotation;

	if (!noclip)
	{
		cheese_platform = nullptr;

		// // plate collision
		for (Scene::Transform *plate : collision_plates)
		{
			if(collide(plate)){
				melt_level += melt_delta * elapsed;
				melt_level = std::clamp(melt_level, MELT_MIN, MELT_MAX);
			}
		}

		for (Scene::Transform *platform : collision_platforms)
		{
			
			if (collide(platform))
			{
				std::cout<<platform->name<<std::endl;
					if (platform->name == "Collision_Rat") {
					Mode::set_current(std::make_shared<PlayMode>());
					return;
				}
				jumping = false;
			}
		}
	}

	// Melt Logic
	{

		// DEBUG
		if (debug_heat.pressed)
		{
			melt_delta *= -1;
			debug_heat.pressed = false;
		}

		melt_level += -0.1f *std::abs(melt_delta) * elapsed;
		melt_level = std::clamp(melt_level, MELT_MIN, MELT_MAX);

		//remove this code b
		bool found = std::find(collision_platforms.begin(), collision_platforms.end(), gate)!= collision_platforms.end();
		
		if (melt_level == MELT_MIN &&(!found)){
				collision_platforms.push_back(gate);
		}
		else if (melt_level == MELT_MAX && found){
			collision_platforms.pop_back();
		}
		// std::cout << melt_level << std::endl;
	}

	//----------------------------------------
	{ //
		float cheese_base = cheese_mesh->min.z;
		float cheese_top = cheese_mesh->max.z;
		float height_range = cheese_top - cheese_base;
		wave_acc += elapsed / 5.0f; // 5 second wave animation cycle
		wave_acc -= std::floor(wave_acc);
		float cheese_spread = 1.0f;

		// Target Brown (e.g., RGB: 139, 69, 19)
		constexpr glm::vec4 TARGET_BROWN = glm::vec4(60.0f, 10.0f, 2.0f, 255.0f);

		cheese_vertices_cpu = initial_cheese_vertices_cpu;
		initial_cheese.set(initial_cheese_vertices_cpu.data(), cheese_vertices_cpu.size(), GL_DYNAMIC_DRAW);

		float melt_percentage_level = 0.5f + (MELT_MAX - melt_level) / MELT_MAX;
		melt_percentage_level = std::clamp(melt_percentage_level, 0.0f, 1.0f);

		float melt_factor = (1.0f - melt_percentage_level);
		float flow = (1.0f + melt_factor * cheese_spread);

		// Gemnin chat
		for (auto &vertex : cheese_vertices_cpu)
		{
			vertex.Position = vertex.Position * theta;
			glm::vec3 pos = vertex.Position;
			glm::vec4 original_color_f = glm::vec4(vertex.Color); // Already 0-255 range

			float melt_level_z = ((pos.z - cheese_base) * melt_percentage_level) + cheese_base;

			float melt_z_percent = ((pos.z - cheese_base) / (height_range));

			float r = std::hypot(pos.x, pos.y) + 0.01f;
			float sin_arg = (r * 0.25f + wave_acc) * (2.0f * PI);
			float h = std::sin(sin_arg);
			float wave_amplitude = 0.00f; // Adjust this value to change the wave height

			float dh_dr = 0.25f * 2.0f * PI * std::cos(sin_arg);
			if (melt_z_percent < melt_factor)
			{
				vertex.Position.x = (1.0f + flow) * vertex.Position.x;
				vertex.Position.y = (1.0f + flow) * vertex.Position.y;
				// Lerp (Interpolate): new_color = (1.0 - factor) * start_color + factor * end_color
				glm::vec4 final_color_f = glm::mix(original_color_f, TARGET_BROWN, (1.0f - (melt_factor * melt_factor)));

				// Assign the result back to the vertex (rounding the floats to integers)
				vertex.Color = glm::u8vec4(final_color_f);
				vertex.Position.z = cheese_base + 0.1f + (melt_percentage_level)*std::abs(0 * h * wave_amplitude);

				// Apply deformation to the Z component (vertical axis for the cheese wheel)
				// Adjust the multiplier for the desired wave intensity0
			}
			else
			{

				// Deform the position:
				vertex.Position.z = melt_level_z + 0.1f + (melt_percentage_level)*std::abs(0 * h * wave_amplitude);
			}

			// Deform the normal (assuming the wave is propagating in the XY plane):

			// Recalculate derivative parts for the new normal vector:
			// dr/dx = x / r; dr/dy = y / r (from r = sqrt(x^2 + y^2))
			float dr_dx = pos.x / r;
			float dr_dy = pos.y / r;

			// Tangent vectors (dp_dx, dp_dy) for the surface:
			glm::vec3 dp_dx = glm::vec3(1.0f, 0.0f, dh_dr * dr_dx * wave_amplitude);
			glm::vec3 dp_dy = glm::vec3(0.0f, 1.0f, dh_dr * dr_dy * wave_amplitude);

			// New normal is the cross product of the tangent vectors:
			vertex.Normal = glm::normalize(glm::cross(dp_dx, dp_dy));
		}
		initial_cheese.set(cheese_vertices_cpu.data(), cheese_vertices_cpu.size(), GL_DYNAMIC_DRAW);
	}
}

void PlayMode::draw(glm::uvec2 const &drawable_size)
{

	// update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	// for mouse
	last_drawable_px = drawable_size;

	// set up light type and position for lit_color_texture_program:
	//  TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, -1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f); // 1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); // this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	GL_ERRORS();
}