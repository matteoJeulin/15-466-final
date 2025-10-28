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
#include <string>


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
		if (transform.name == "Wheel_Prototype")
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
	
	Scene::Drawable *cheese_drawable = nullptr;
    for (auto &drawable : scene.drawables) {
        if (drawable.transform == cheese_wheel) {
            cheese_drawable = &drawable;
			break;
        }
    }

	level_meshes->print_all_meshes();

	if (cheese_drawable == nullptr) throw std::runtime_error("Cheese wheel drawable not found.");

    cheese_mesh = &(level_meshes->lookup("Wheel_Prototype")); 
	
	theta= cheese_wheel->rotation;

	const size_t vertex_stride = sizeof(DynamicMeshBuffer::Vertex);

	const GLintptr offset_bytes = (GLintptr)cheese_mesh->start * vertex_stride;
	const GLsizeiptr size_bytes = (GLsizeiptr)cheese_mesh->count * vertex_stride;

	std::vector<DynamicMeshBuffer::Vertex> initial_vertices(cheese_mesh->count );
	glBindBuffer(GL_ARRAY_BUFFER, level_meshes->buffer);

	glGetBufferSubData(
		GL_ARRAY_BUFFER,            
		offset_bytes,               // Byte offset to start reading
		size_bytes,                 // Total bytes to read
		initial_vertices.data()     // Destination pointer in CPU memory
	);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	initial_cheese_vertices_cpu = initial_vertices;
	cheese_vertices_cpu = initial_vertices;

	initial_cheese.set(initial_vertices.data(), initial_vertices.size(), GL_DYNAMIC_DRAW); 
    melted_cheese.set(initial_vertices.data(), initial_vertices.size(), GL_DYNAMIC_DRAW);

	//change static to dynamic mesh 
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
		// 	inside_gate = true;
			printf("Inside the gate (A) %f\n", melt_level);
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
		// 	inside_gate = true;
			printf("Inside the gate (B) %f\n", melt_level);
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
		// 	inside_gate = true;
			printf("Inside the gate (C) %f\n", melt_level);
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
		// 	inside_gate = true;
			printf("Inside the gate (D) %f\n", melt_level);
			return true;
		}

		cheese_pos.x = platform_pos.x - platform_size.x - cheese_size.x;
		cheeseSpeed.x = 0.0f;

		return true;
	}

	// if (platform == gate) {
	// 	inside_gate = false;
	// 	printf("Outside %f\n", melt_level);
	// }

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

		constexpr float ROTATION_FACTOR = 1.0f; 
		float rotation_angle = cheeseSpeed.x * elapsed * (-ROTATION_FACTOR);


		//previous_player_pos = player->position;
		previous_cheese_pos = cheese_wheel->position;

		// y-axis is the forward/backward direction and the x-axis is the right/left direction
		cheese_wheel->position += cheeseSpeed.x * frame_right * elapsed + cheeseSpeed.y * frame_forward * elapsed + cheeseSpeed.z * glm::vec3(0.0f, 0.0f, 1.0f) * elapsed;

		camera->transform->position += cheeseSpeed.x* frame_right*0.9f*elapsed ;//need to change this


		glm::quat rotation = glm::angleAxis(rotation_angle, frame_forward);
		theta = theta * rotation;



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

			// if (!inside_gate || melt_delta > 0)
			melt_level += melt_delta * elapsed;
			melt_level = std::clamp(melt_level, MELT_MIN, MELT_MAX);
			// std::cout << melt_level << std::endl;
		}

			//----------------------------------------
		{ //generate some waves:

			//advance wave animation:
			// DEBUG: (speed based on melt level)
			float cheese_base = cheese_mesh->min.z;
			float cheese_top = cheese_mesh->max.z;
			float height_range = cheese_top - cheese_base;
			wave_acc += elapsed / 5.0f; //5 second wave animation cycle
			wave_acc -= std::floor(wave_acc);
			float cheese_spread = 1.0f;

			// Target Brown (e.g., RGB: 139, 69, 19)
			constexpr glm::vec4 TARGET_BROWN = glm::vec4(60.0f, 10.0f, 2.0f, 255.0f);
			
			cheese_vertices_cpu = initial_cheese_vertices_cpu;
			initial_cheese.set(initial_cheese_vertices_cpu.data(), cheese_vertices_cpu.size(), GL_DYNAMIC_DRAW);
			glm::vec3 center = cheese_wheel->position;
			float melt_percentage_level = (MELT_MAX-melt_level)/ MELT_MAX;
			float melt_factor = (1.0f-melt_percentage_level);
			float flow = (1.0f+melt_factor*cheese_spread);


			for (auto &vertex : cheese_vertices_cpu) {
				vertex.Position = vertex.Position*theta;
				glm::vec3 pos = vertex.Position;
				glm::vec4 original_color_f = glm::vec4(vertex.Color); // Already 0-255 range
				
				float melt_level_z = ((pos.z-cheese_base) * melt_percentage_level)+cheese_base;

				float melt_z_percent = ((pos.z-cheese_base)/(height_range));


				float r = std::hypot(pos.x, pos.y) + 0.01f; 
				float sin_arg = (r * 0.25f + wave_acc) * (2.0f * PI);
				float h = std::sin(sin_arg);
							float wave_amplitude = 0.00f; // Adjust this value to change the wave height
				
				float dh_dr = 0.25f * 2.0f * PI * std::cos(sin_arg);
				if (melt_z_percent < melt_factor){
					vertex.Position.x  = (1.0f+flow)*vertex.Position.x ;
					vertex.Position.y = (1.0f+flow)*vertex.Position.y ;
					 // Lerp (Interpolate): new_color = (1.0 - factor) * start_color + factor * end_color
					glm::vec4 final_color_f = glm::mix(original_color_f, TARGET_BROWN, (1.0f-(melt_factor*melt_factor)));

					// Assign the result back to the vertex (rounding the floats to integers)
					vertex.Color = glm::u8vec4(final_color_f);
					vertex.Position.z = cheese_base + 0.1f +(melt_percentage_level)*std::abs(0*h*wave_amplitude); 

									// Apply deformation to the Z component (vertical axis for the cheese wheel)
				// Adjust the multiplier for the desired wave intensity0
	
				
			
				}
				else{

					// Deform the position:
					vertex.Position.z = melt_level_z + 0.1f+(melt_percentage_level)*std::abs(0*h*wave_amplitude);
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

	// { //use DrawLines to overlay some text:
	// 	glDisable(GL_DEPTH_TEST);
	// 	float aspect = float(drawable_size.x) / float(drawable_size.y);
	// 	DrawLines lines(glm::mat4(
	// 		1.0f / aspect, 0.0f, 0.0f, 0.0f,
	// 		0.0f, 1.0f, 0.0f, 0.0f,
	// 		0.0f, 0.0f, 1.0f, 0.0f,
	// 		0.0f, 0.0f, 0.0f, 1.0f
	// 	));

	// 	constexpr float H = 0.09f;
	// 	lines.draw_text("heat level: " + std::to_string(melt_level),
	// 		glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
	// 		glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
	// 		glm::u8vec4(0x00, 0x00, 0x00, 0x00));
	// }
	
	
	scene.draw(*camera);

	GL_ERRORS();
}
