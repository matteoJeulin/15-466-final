#include "PlayMode.hpp"
#include "Mode.hpp"
#include "RayCast.hpp"
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

// texture block set for stove, chatGPT
static GLuint make_solid_tex(glm::u8vec4 rgba) {
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

PlayMode::PlayMode() : scene(*level_scene), kitchen_music(data_path("kitchen_music_first.wav"), data_path("kitchen_music_loop.wav")),
											pause_music(data_path("kitchen_pause_music_first.wav"), data_path("kitchen_pause_music_loop.wav"))
{
	player = new Player(this);

	for (auto &transform : scene.transforms)
	{
		std::cout << transform.name << std::endl;
		if (transform.name == "Wheel_Prototype")
			player->model = &transform;
		else if (transform.name == "Cheese_Wheel")
			player->collision = &transform;
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
			grates.emplace_back(&transform);
		}
		else if (transform.name.substr(0, 9) == "Collision")
		{
			if (transform.name != "Collision_Hot_Plate" && transform.name != "Collision_Cold_Plate")
			{
				collision_platforms.emplace_back(&transform);
			}

			if (transform.name == "Collision_CounterTop")
			{
				counter_top = &transform;
			}
		}
		else if (transform.name == "Rat")
		{
			Rat *rat = new Rat(this);
			rat->model = &transform;
			rat->collision = &transform;
			rats.emplace_back(rat);
		}
		else if (transform.name.substr(0, 11) == "BounceWeak")
		{
			bouncy_weak_platforms.emplace_back(&transform);
		}
		else if (transform.name.substr(0, 13) == "BounceStrong")
		{
			bouncy_strong_platforms.emplace_back(&transform);
		}
		else if (transform.name.substr(0, 16) == "GrapplingCracker") {
			grapple_crackers.emplace_back(&transform);
		}
		else if (transform.name == "Cube.001")
		{
			stove_1 = &transform;
			collision_plates.emplace_back(&transform);
		}
		else if (transform.name == "Cube"){
			stove_2 = &transform;
			collision_plates.emplace_back(&transform);
		}
	}
	if (player->model == nullptr)
		throw std::runtime_error("Cheese not found.");
	if (hot_plate == nullptr)
		throw std::runtime_error("Hot plate not found.");
	if (cold_plate == nullptr)
		throw std::runtime_error("Cold plate not found.");

	// get pointer to camera for convenience:
	if (scene.cameras.size() != 1)
		throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	for (auto &drawable : scene.drawables)
	{
		if (drawable.transform == player->model)
		{
			player->drawable = &drawable;
			break;
		}
	}

	level_meshes->print_all_meshes();

	if (player->drawable == nullptr)
		throw std::runtime_error("Cheese wheel drawable not found.");

	player->mesh = &(level_meshes->lookup("Wheel_Prototype"));

	player->theta = player->model->rotation;
	const size_t vertex_stride = sizeof(DynamicMeshBuffer::Vertex);
	const GLintptr offset_bytes = (GLintptr)player->mesh->start * vertex_stride;
	const GLsizeiptr size_bytes = (GLsizeiptr)player->mesh->count * vertex_stride;

	std::cout << player->collision->position.x <<" " << player->collision->position.y << " " << player->collision->position.z << std::endl;

	std::vector<DynamicMeshBuffer::Vertex> initial_vertices(player->mesh->count);
	glBindBuffer(GL_ARRAY_BUFFER, level_meshes->buffer);

	glGetBufferSubData(
		GL_ARRAY_BUFFER,
		offset_bytes,			// Byte offset to start reading
		size_bytes,				// Total bytes to read
		initial_vertices.data() // Destination pointer in CPU memory
	);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	player->initialVerticesCpu = initial_vertices;
	player->verticesCpu = initial_vertices;

	player->initialMeshBuffer.set(initial_vertices.data(), initial_vertices.size(), GL_DYNAMIC_DRAW);
	player->meltedMeshBuffer.set(initial_vertices.data(), initial_vertices.size(), GL_DYNAMIC_DRAW);

	// change static to dynamic mesh
	player->cheese_lit_color_texture_program = player->initialMeshBuffer.make_vao_for_program(lit_color_texture_program->program);
	player->melted_cheese_lit_color_texture_program = player->initialMeshBuffer.make_vao_for_program(lit_color_texture_program->program);
	player->drawable->pipeline.vao = player->cheese_lit_color_texture_program;
	player->drawable->pipeline.type = player->mesh->type;
	player->drawable->pipeline.start = 0; // Starts from 0 in the new buffer
	player->drawable->pipeline.count = player->mesh->count;

	wine_bottle_ui.load_image_data(data_path("wine_bottle_5.png"), OriginLocation::UpperLeftOrigin);
	wine_bottle_ui.create_mesh(Mode::window, bottle_ui_pos_x, bottle_ui_pos_y, bottle_ui_height);

	// kitchen_music = DynamicSoundLoop::DynamicSoundLoop();
	kitchen_music.play(1.0f, 0.0f);
	pause_music.play(0.0f, 0.0f);

	// Find the stove drawable for stove_1
	for (auto& d : scene.drawables) {
		if (d.transform == stove_1) { stove_drawable = &d; break; }
	}
	if (!stove_drawable) throw std::runtime_error("Hot plate drawable not found.");

	// ensure VAO is compatible with the lit program 
	GLuint stove_vao = level_meshes->make_vao_for_program(lit_color_texture_program->program);
	Mesh const& stove_mesh = level_meshes->lookup("Cube.001"); 
	stove_drawable->pipeline = lit_color_texture_program_pipeline;
	stove_drawable->pipeline.vao = stove_vao;
	stove_drawable->pipeline.type = stove_mesh.type;
	stove_drawable->pipeline.start = stove_mesh.start;
	stove_drawable->pipeline.count = stove_mesh.count;

	// 1�1 solid textures:
	stove_tint_lvl0 = make_solid_tex({ 255,255,255,255 }); // off
	stove_tint_lvl1 = make_solid_tex({ 255, 90, 60,255 }); // warm
	stove_tint_lvl2 = make_solid_tex({ 255, 40, 15,255 }); // hot
	stove_tint_lvl3 = make_solid_tex({ 255,  0,  0,255 }); // very hot

	// start with no tint
	stove_drawable->pipeline.textures[0].texture = stove_tint_lvl0;
	stove_drawable->pipeline.textures[0].target = GL_TEXTURE_2D; // important


}

PlayMode::~PlayMode()
{
	glDeleteVertexArrays(1, &player->cheese_lit_color_texture_program);
	player->cheese_lit_color_texture_program = 0;

	if (stove_tint_lvl0) glDeleteTextures(1, &stove_tint_lvl0);
	if (stove_tint_lvl1) glDeleteTextures(1, &stove_tint_lvl1);
	if (stove_tint_lvl2) glDeleteTextures(1, &stove_tint_lvl2);
	if (stove_tint_lvl3) glDeleteTextures(1, &stove_tint_lvl3);

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
			player->left.downs += 1;
			player->left.pressed = true;
			return true;
		}
		else if (evt.key.key == SDLK_D || evt.key.key == SDLK_RIGHT)
		{
			player->right.downs += 1;
			player->right.pressed = true;
			return true;
		}
		else if (evt.key.key == SDLK_SPACE)
		{
			player->jump.downs += 1;
			player->jump.pressed = true;
			return true;
		}
		else if (evt.key.key == SDLK_J)
		{
			player->debug_heat.downs += 1;
			player->debug_heat.pressed = true;
			return true;
		}
		else if (evt.key.key == SDLK_TAB)
		{
			player->pause.downs += 1;
			player->pause.pressed = true;
			return true;
		}
	}
	else if (evt.type == SDL_EVENT_KEY_UP)
	{
		if (evt.key.key == SDLK_A || evt.key.key == SDLK_LEFT)
		{
			player->left.pressed = false;
			return true;
		}
		else if (evt.key.key == SDLK_D || evt.key.key == SDLK_RIGHT)
		{
			player->right.pressed = false;
			return true;
		}
		else if (evt.key.key == SDLK_SPACE)
		{
			player->jump.pressed = false;
			return true;
		}
		else if (evt.key.key == SDLK_M)
		{
			player->mute.pressed = false;
			return true;
		}
		else if (evt.key.key == SDLK_J)
		{
			player->debug_heat.pressed = false;
			return true;
		}
		else if (evt.key.key == SDLK_TAB)
		{
			player->pause.pressed = false;
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

			auto tex_for = [&](int lvl)->GLuint {
				switch (lvl) {
				case 1: return stove_tint_lvl1;
				case 2: return stove_tint_lvl2;
				case 3: return stove_tint_lvl3;
				default: return stove_tint_lvl0;
				}
				};

			glm::vec2 mouse_win(float(evt.button.x), float(evt.button.y));
			glm::vec2 scale = glm::vec2(last_drawable_px) / glm::vec2(window_size);
			// glm::vec2 mouse_px = glm::vec2(float(evt.button.x), float(evt.button.y));
			glm::vec2 mouse_px = mouse_win * scale;

			Ray r = screen_point_to_world_ray(camera, mouse_px, last_drawable_px);
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
				// switch rotation
				hit->rotation = hit->rotation * glm::angleAxis(glm::radians(90.f), glm::vec3(0, 0, 1));

				int &knob_state = (hit == switch_1) ? knob_state_1 : knob_state_2;
				knob_state = (knob_state + 1) % 4;
				player->set_heat_level(knob_state);

				if (hit == switch_1) {
					stove_drawable->pipeline.textures[0].texture = tex_for(knob_state);
					stove_drawable->pipeline.textures[0].target = GL_TEXTURE_2D;
				}

				std::cout << "Switch toggled:" << hit->name.c_str() << " heat level " << knob_state << " melt_delta now " << player->melt_delta << std::endl;
				return true;
			}

			if (player->melt_level > (MELT_MIN + MELT_MAX) / 2) {}
				for (auto cracker : grapple_crackers) {
					try_hit(cracker);

					if (hit) {
						player->grapple_point = cracker;
						player->locomotionState = (Player::PlayerLocomotion)(player->locomotionState | Player::PlayerLocomotion::Grappling);
					}
				}
			}
		}
	}
	else if (evt.type == SDL_EVENT_MOUSE_BUTTON_UP) {
		player->grapple_point = nullptr;
		player->locomotionState = (Player::PlayerLocomotion)(player->locomotionState & ~Player::PlayerLocomotion::Grappling);
	}

	return false;
}

void PlayMode::update(float elapsed)
{
	if (player->pause.downs > 0) {
		paused = !paused;
		if (paused) {
			vol_fade_rate = 2.0f;
		}
		else {
			vol_fade_rate = -2.0f;
		}
	}

	if (!paused) {
		player->update(elapsed);

		for (Rat *rat : rats)
			rat->update(elapsed);

		camera->transform->position.y = player->collision->position.y; // need to change this
		camera->transform->position.z = player->collision->position.z + 30.0f;						   // need to change this
		float last_wine = wine_remaining;
		wine_remaining = std::clamp(wine_remaining - elapsed, 0.0f, MAX_LEVEL_TIME);
		
		int last_rank = (int)(std::ceil(5 * ((last_wine / MAX_LEVEL_TIME))));
		int wine_rank = (int)(std::ceil(5 * ((wine_remaining / MAX_LEVEL_TIME))));

		// std::cout << wine_rank << std::endl;

		if (wine_rank != last_rank) {
			wine_bottle_ui.load_image_data(data_path("wine_bottle_" + std::to_string(wine_rank) + ".png"), OriginLocation::UpperLeftOrigin);
			wine_bottle_ui.create_mesh(Mode::window, bottle_ui_pos_x, bottle_ui_pos_y, bottle_ui_height);
		}
	}

	player->pause.downs = 0;

	// music
	pause_vol = std::clamp(pause_vol + (vol_fade_rate * elapsed), 0.0f, 1.0f);
	kitchen_music.set_volume(1.0f - pause_vol, 1.f / 60.f);
	pause_music.set_volume(pause_vol * 2.0f, 1.f / 60.f);

	kitchen_music.update();
	pause_music.update();
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

	assert(wine_bottle_ui.data_created);
	if (wine_bottle_ui.data_created)
		wine_bottle_ui.draw_mesh();

	GL_ERRORS();
}

void PlayMode::reset()
{
	player->collision->position = glm::vec3(0.0f, -27.2722f, 11.0663f);
	player->locomotionState = (Player::PlayerLocomotion)0;
	player->grapple_point = nullptr;
	// Mode::set_current(std::make_shared<PlayMode>());
}