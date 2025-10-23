#include "PlayMode.hpp"

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
		if (transform.name == "Cheese_Wheel")
			cheese_wheel = &transform;
		else if (transform.name == "Hot_Plate")
			hot_plate = &transform;
		else if (transform.name == "Cold_Plate")
			cold_plate = &transform;
		else if (transform.name.substr(0, 9) == "Collision")
		{
			collision_platforms.emplace_back(&transform);
			if (transform.name == "Collision_CounterTop") {
				counter_top = &transform;
			}
			else if (transform.name == "Collision_Gate") {
				gate = &transform;
			}
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

	{ //set up drawable for wave:
		//make vao:
		wave_for_lit_color_texture_program = wave.make_vao_for_program(lit_color_texture_program->program);

		//make scene entry for the vao:
		scene.transforms.emplace_back();
		scene.drawables.emplace_back(&scene.transforms.back());
		wave_drawable = &scene.drawables.back();
		wave_drawable->pipeline = lit_color_texture_program_pipeline;
		wave_drawable->pipeline.vao = wave_for_lit_color_texture_program;
		wave_drawable->pipeline.type = GL_TRIANGLE_STRIP;
		wave_drawable->pipeline.start = 0;
		wave_drawable->pipeline.count = 0; //will set dynamically, later, based on generated geometry
	}
}

PlayMode::~PlayMode()
{
	glDeleteVertexArrays(1, &wave_for_lit_color_texture_program);
	wave_for_lit_color_texture_program = 0;
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
		else if (evt.key.key == SDLK_J) {
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
		else if (evt.key.key == SDLK_J) {
			debug_heat.pressed = false;
			return true;
		}
	}
	else if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
	{
		if (SDL_GetWindowRelativeMouseMode(Mode::window) == false)
		{
			SDL_SetWindowRelativeMouseMode(Mode::window, true);
			return true;
		}
	}
	else if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
	{
		if (SDL_GetWindowRelativeMouseMode(Mode::window) == false)
		{
			SDL_SetWindowRelativeMouseMode(Mode::window, true);
			return true;
		}
	}

	return false;
}

bool PlayMode::collide_platform_top(Scene::Transform *platform)
{
	glm::vec3 &cheese_pos = cheese_wheel->position;
	glm::vec3 cheese_size = cheese_wheel->scale;

	glm::vec3 platform_pos = platform->position;
	glm::vec3 platform_size = platform->scale;

	bool overlapXY =
		(cheese_pos.x - cheese_size.x <= platform_pos.x + platform_size.x) &&
		(cheese_pos.x + cheese_size.x >= platform_pos.x - platform_size.x) &&
		(cheese_pos.y - cheese_size.y <= platform_pos.y + platform_size.y) &&
		(cheese_pos.y + cheese_size.y >= platform_pos.y - platform_size.y);

	if (!overlapXY) return false;

	//Check the x and y coordinates
	if (cheese_pos.x <= platform_pos.x + platform_size.x && cheese_pos.x >= platform_pos.x - platform_size.x &&
		cheese_pos.y <= platform_pos.y + platform_size.y && cheese_pos.y >= platform_pos.y - platform_size.y &&
		//Check if the elevation is correct
		cheese_pos.z - cheese_size.z < platform_pos.z + platform_size.z && previous_cheese_pos.z - cheese_size.z >= platform_pos.z + platform_size.z)
	{
		cheese_pos.z = platform_pos.z + platform_size.z + cheese_size.z;
		cheeseSpeed.z = 0.0f;
		cheese_platform = platform;
		return true;
	}

	return false;
}


bool PlayMode::collide_platform_side(Scene::Transform *platform)
{
	// TODO: inside_gate is not being set properly because
	// 		 collision seems to check for initial impact

	glm::vec3 &cheese_pos = cheese_wheel->position;
	glm::vec3 cheese_size = cheese_wheel->scale;

	glm::vec3 platform_pos = platform->position;
	glm::vec3 platform_size = platform->scale;

	// Check the collision with each of the sides individually
	if (cheese_pos.x <= platform_pos.x + platform_size.x && cheese_pos.x >= platform_pos.x - platform_size.x &&
		cheese_pos.z <= platform_pos.z + platform_size.z && cheese_pos.z >= platform_pos.z - platform_size.z &&
		cheese_pos.y - cheese_size.y < platform_pos.y + platform_size.y && previous_cheese_pos.y - cheese_size.y >= platform_pos.y + platform_size.y)
	{
		if (platform == gate && melt_level > (MELT_MIN + MELT_MAX) / 2) {
			return true;
		}

		cheese_pos.y = platform_pos.y + platform_size.y + cheese_size.y;
		cheeseSpeed.y = 0.0f;

		return true;
	}

	if (cheese_pos.x <= platform_pos.x + platform_size.x && cheese_pos.x >= platform_pos.x - platform_size.x &&
		cheese_pos.z <= platform_pos.z + platform_size.z && cheese_pos.z >= platform_pos.z - platform_size.z &&
		cheese_pos.y + cheese_size.y > platform_pos.y - platform_size.y && previous_cheese_pos.y + cheese_size.y <= platform_pos.y - platform_size.y)
	{
		if (platform == gate && melt_level > (MELT_MIN + MELT_MAX) / 2) {
			return true;
		} 

		cheese_pos.y = platform_pos.y - platform_size.y - cheese_size.y;
		cheeseSpeed.y = 0.0f;
		
		return true;
	}

	if (cheese_pos.z <= platform_pos.z + platform_size.z && cheese_pos.z >= platform_pos.z - platform_size.z &&
		cheese_pos.y <= platform_pos.y + platform_size.y && cheese_pos.y >= platform_pos.y - platform_size.y &&
		cheese_pos.x - cheese_size.x < platform_pos.x + platform_size.x && previous_cheese_pos.x - cheese_size.x >= platform_pos.x + platform_size.x)
	{
		if (platform == gate && melt_level > (MELT_MIN + MELT_MAX) / 2) {
			return true;
		}
		cheese_pos.x = platform_pos.x + platform_size.x + cheese_size.x;
		cheeseSpeed.x = 0.0f;

		return true;
	}

	if (cheese_pos.z <= platform_pos.z + platform_size.z && cheese_pos.z >= platform_pos.z - platform_size.z &&
		cheese_pos.y <= platform_pos.y + platform_size.y && cheese_pos.y >= platform_pos.y - platform_size.y &&
		cheese_pos.x + cheese_size.x > platform_pos.x - platform_size.x && previous_cheese_pos.x + cheese_size.x <= platform_pos.x - platform_size.x)
	{
		if (platform == gate && melt_level > (MELT_MIN + MELT_MAX) / 2) {
			return true;
		}

		cheese_pos.x = platform_pos.x - platform_size.x - cheese_size.x;
		cheeseSpeed.x = 0.0f;

		return true;
	}

	return false;
}

void PlayMode::update(float elapsed)
{

		// combine inputs into a move:
		if (left.pressed && !right.pressed)
			cheeseSpeed.x = std::max(cheeseSpeed.x - cheeseAcceleration * elapsed, -cheeseMaxSpeed);
		if (!left.pressed && right.pressed)
			cheeseSpeed.x = std::min(cheeseSpeed.x + cheeseAcceleration * elapsed, cheeseMaxSpeed);
		if (jump.pressed && !jumping && cheese_platform != nullptr)
		{
			cheeseSpeed.z = jumpSpeed;
			jump.pressed = false;
			jumping = true;
		}

		// Apply inertia to get the player down to 0 speed.
		if ((!left.pressed && !right.pressed) || (left.pressed && cheeseSpeed.x > 0) || (right.pressed && cheeseSpeed.x < 0))
		{
			cheeseSpeed.x -= cheeseSpeed.x * elapsed * 10;
		}

		cheeseSpeed.z -= gravity * elapsed;

		//glm::mat4x3 frame = player->make_parent_from_local();
		glm::mat4x3 frame = cheese_wheel->make_parent_from_local();
		glm::vec3 frame_forward = -frame[0];
		glm::vec3 frame_right = frame[1];

		frame_forward.z = 0;
		frame_forward = glm::normalize(frame_forward);
		frame_right.z = 0;
		frame_right = glm::normalize(frame_right);


		//previous_player_pos = player->position;
		previous_cheese_pos = cheese_wheel->position;

		// y-axis is the forward/backward direction and the x-axis is the right/left direction
		cheese_wheel->position += cheeseSpeed.x * frame_right * elapsed + cheeseSpeed.y * frame_forward * elapsed + cheeseSpeed.z * glm::vec3(0.0f, 0.0f, 1.0f) * elapsed;

		if (!noclip)
		{
			cheese_platform = nullptr;
			for (Scene::Transform *platform : collision_platforms)
			{
				collide_platform_side(platform);
				if (collide_platform_top(platform))
				{
					jumping = false;
				}
			}
			collide_platform_side(gate);
		}

		// Melt Logic
		{

			// DEBUG
			if (debug_heat.pressed) {
				melt_delta *= -1;
				debug_heat.pressed = false;
			}

			melt_level += melt_delta * elapsed;
			melt_level = std::clamp(melt_level, MELT_MIN, MELT_MAX);
			// std::cout << melt_level << std::endl;
		}

			//----------------------------------------
		{ //generate some waves:

			//advance wave animation:
			// DEBUG: (speed based on melt level)
			wave_acc += melt_level * elapsed / 5.0f; //5 second wave animation cycle
			wave_acc -= std::floor(wave_acc);

			//make geometry:
			size_t size = 100; //will use a size x size grid
			std::vector< DynamicMeshBuffer::Vertex > vertices;
			size_t expected_size = (size-1) * (2 * size) + (size - 2) * 2;
			vertices.reserve(expected_size);

			auto attrib_at = [&](size_t xi, size_t yi) -> DynamicMeshBuffer::Vertex {
				//rescale indices to a [-10,10]^2 grid:
				glm::vec2 pos = (glm::vec2(xi, yi) / float(size-1) - 0.5f) * 20.0f;

				float r = std::hypot(pos.x, pos.y) + 0.01f; //never quite reach the center -- avoid divide-by-zero
				float h = std::sin((r * 0.25f + wave_acc) * (2.0f * PI));

				DynamicMeshBuffer::Vertex vertex;
				vertex.Position = glm::vec3(pos, h);

				// calculus :-/
				// dh/dx = std::cos(...) * 0.25 * 2.0 * M_PI * dr/dx
				// dr/dx = 
				float dh_dr = 0.25f * 2.0f * PI * std::cos((r * 0.25f + wave_acc) * (2.0f * PI));
				float dr_dx = 0.5f * (1.0f / r) * 2.0f * pos.x;
				float dr_dy = 0.5f * (1.0f / r) * 2.0f * pos.y;
				glm::vec3 dp_dx = glm::vec3(1.0f, 0.0f, dh_dr * dr_dx);
				glm::vec3 dp_dy = glm::vec3(0.0f, 1.0f, dh_dr * dr_dy);
				vertex.Normal = glm::normalize(glm::cross(dp_dx, dp_dy));


				vertex.Color = glm::u8vec4(0xff); //glm::u8vec4(0xff, std::clamp< int32_t >(256.0f * (h * 0.5f + 0.5f), 0, 255), 0xff, 0xff);
				vertex.TexCoord = glm::vec2(xi, yi) / float(size-1);

				return vertex;
			};

			for (size_t yi = 0; yi + 1 < size; ++yi) {
				for (size_t xi = 0; xi < size; ++xi) {
					if (xi == 0 && yi != 0) vertices.emplace_back(vertices.back());
					vertices.emplace_back(attrib_at(xi, yi));
					if (xi == 0 && yi != 0) vertices.emplace_back(vertices.back());
					vertices.emplace_back(attrib_at(xi, yi+1));
				}
			}
			assert(vertices.size() == expected_size);

			//upload to the GPU:
			wave.set(vertices, GL_STREAM_DRAW);

			//make sure scene has the info to draw it:
			wave_drawable->pipeline.count = wave.count;

			//lift it up above the car:
			wave_drawable->transform->position = cheese_wheel->position;
		}
}

void PlayMode::draw(glm::uvec2 const &drawable_size)
{

	// update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

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
