#include "PlayMode.hpp"
#include "MenuMode.hpp"
#include "Mode.hpp"
#include "RayCast.hpp"
#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "read_write_chunk.hpp"
#include "AudioManager.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <fstream>
#include <sstream>
#include <iostream>

#include <cmath>
#include <string>
#include <algorithm>

GLuint level_meshes_for_lit_color_texture_program = 0;
Load<MeshBuffer> level_meshes(LoadTagDefault, []() -> MeshBuffer const *
							  {
	MeshBuffer const *ret = new MeshBuffer(data_path("Cheese.pnct"));
	level_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret; });

Load<Scene> level_scene(LoadTagDefault, []() -> Scene const *
						{ return new Scene(data_path("Cheese.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name)
										   {
												if (( transform->name == "Cheese_Wheel")) {
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
												 drawable.pipeline.count = mesh.count; 

												 float roughness = 1.0f; //where can we get roughness of the material 
												 drawable.pipeline.set_uniforms = [roughness](){
													glUniform1f(lit_color_texture_program->ROUGHNESS_float, roughness);
		};
												
												}); });

Sound::Sample kitchen_first = Sound::Sample(data_path("kitchen_music_first.wav"));
Sound::Sample kitchen_loop = Sound::Sample(data_path("kitchen_music_loop.wav"));
Sound::Sample kitchen_pause_first = Sound::Sample(data_path("kitchen_pause_music_loop.wav"));
Sound::Sample kitchen_pause_loop = Sound::Sample(data_path("kitchen_pause_music_loop.wav"));

// PlayMode::PlayMode() : scene(*level_scene), kitchen_music(data_path("kitchen_music_first.wav"), data_path("kitchen_music_loop.wav")),
// 											pause_music(data_path("kitchen_pause_music_first.wav"), data_path("kitchen_pause_music_loop.wav"))

void resume(void)
{
	((PlayMode *)Mode::current.get())->paused = false;
	((PlayMode *)Mode::current.get())->vol_fade_rate = -2.0f;
}

PlayMode::PlayMode() : scene(*level_scene), kitchen_music(&kitchen_first, &kitchen_loop),
					   pause_music(&kitchen_pause_first, &kitchen_pause_loop)
{
	// Import code based on Kenechukwu's Game 3 code
	levels.clear();
	std::ifstream levelFile;
	levelFile.open(data_path("ranks_and_spawns.lvl"), std::ios::binary);

	Level levelTemp;
	while (levelFile.read(reinterpret_cast<char*>(&levelTemp), sizeof(Level))) {
		levels.emplace_back(levelTemp);
	}
	levelFile.close();

	foundLevel = false;
	if (current_level >= 0 && current_level < levels.size()) {
		S_RANK_TIME = levels[current_level].s_rank_time;
		A_RANK_TIME = levels[current_level].a_rank_time;
		B_RANK_TIME = levels[current_level].b_rank_time;
		C_RANK_TIME = levels[current_level].c_rank_time;
		D_RANK_TIME = levels[current_level].d_rank_time;

		spawnLocName = std::string(levels[current_level].spawnLocation);

		std::cout << "Loaded Level " << current_level + 1 << "\n";
	}

	std::cout << "=============================================================================================" << std::endl;
	AudioManager::init();

	player = new Player(this);

	UIElement resumeButton;
	resumeButton.load_image_data(data_path("resume_button.png"), OriginLocation::UpperLeftOrigin);

	buttons.push_back(Button(&resume, resumeButton, glm::vec2(0.0f, 0.7f), 0.2f));
	buttons.push_back(Button::MainMenu);
	buttons.push_back(Button::QuitGame);

	for (auto &transform : scene.transforms)
	{
		std::cout << transform.name << std::endl;
		if (transform.name == "Wheel_Prototype")
			player->model = &transform;
		else if (transform.name == "Cheese_Wheel")
			player->collision = &transform;
		else if (transform.name.substr(0, 5) == "Grate")
		{
			grates.emplace_back(&transform);
		}
		else if (transform.name.substr(0, 9) == "Collision")
		{
			collision_platforms.emplace_back(&transform);
		}
		else if (transform.name.substr(0, 3) == "Rat")
		{
			Rat *rat = new Rat(this);
			rat_map[&transform] = rat;
			rat->model = &transform;
			rat->collision = &transform;
			rats.emplace_back(rat);
		}
		else if (transform.name.substr(0, 9) == "Model_Rat")
		{
			auto it = rat_map.find((transform.parent));
			if (it != rat_map.end())
			{
				Rat *found_rat = it->second;
				found_rat->model = &transform;
			}
		}
		else if (transform.name.substr(0, 10) == "BounceWeak")
		{
			bouncy_weak_platforms.emplace_back(&transform);
		}
		else if (transform.name.substr(0, 12) == "BounceStrong")
		{
			bouncy_strong_platforms.emplace_back(&transform);
		}
		if (transform.name.substr(0, 5) == "Plate")
		{
			collision_plates.emplace_back(&transform);
		}
		else if (transform.name.substr(0, 16) == "GrapplingCracker")
		{
			grapple_crackers.emplace_back(&transform);
		}
		else if (transform.name.substr(0, 11) == "Slider_start") {
			MovingWall *wall = nullptr;
			if (transform.name.substr(17, 19) == "up")
				wall = new MovingWall(-player->height * 5, 1.0f, transform.position.z);
			else
				wall = new MovingWall(player->height * 5, 1.0f, transform.position.z);
			assert (wall != nullptr);
			wall->collision = &transform;
			moving_walls.emplace_back(wall);
		}
		else if (transform.name.substr(0, 5) == "Spawn") {
			spawn_locations.emplace_back(&transform);

			if (transform.name.substr(6, 1)[0] == std::to_string(current_level + 1)[0]) {
				foundLevel = true;
				spawnPos = glm::vec3(0.0f, transform.position.y, transform.position.z);
			}
		}
		else if (transform.name.substr(0, 4) == "Wine") {
			wine_bottles.emplace_back(&transform);
		}
	}
	Scene::Transform *spawnTransform = spawn_positions[0];
	spawnPos = glm::vec3(0.0f, spawnTransform[0].position.y, spawnTransform[0].position.z);
	if (player->model == nullptr)
		throw std::runtime_error("Cheese not found.");
	if (foundLevel) {
		player->collision->position = spawnPos;
		std::cout << "Level found!\n";
	}

	// get pointer to camera for convenience:
	if (scene.cameras.size() != 1)
		throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	//shadow map camera
	// scene.transforms.emplace_back();
	// scene.cameras.emplace_back(&scene.transforms.back());
	// light_camera = &scene.cameras.back();
	// light_camera->fovy = 60.0f / 180.0f * 3.1415926f;
	// light_camera->near = 0.01f;


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

	stove.init(scene);
}

PlayMode::~PlayMode()
{
	glDeleteVertexArrays(1, &player->cheese_lit_color_texture_program);
	player->cheese_lit_color_texture_program = 0;

	if (stove_tint_lvl0)
		glDeleteTextures(1, &stove_tint_lvl0);
	if (stove_tint_lvl1)
		glDeleteTextures(1, &stove_tint_lvl1);
	if (stove_tint_lvl2)
		glDeleteTextures(1, &stove_tint_lvl2);
	if (stove_tint_lvl3)
		glDeleteTextures(1, &stove_tint_lvl3);

	Sound::stop_all_samples();
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
		// TODO: Refactor mouse code to be inside the Player class
		// if (SDL_GetWindowRelativeMouseMode(Mode::window) == false)
		// {
		// 	SDL_SetWindowRelativeMouseMode(Mode::window, true);
		// 	return true;
		// }

		if (evt.button.button == SDL_BUTTON_LEFT)
		{
			if (paused)
			{
				for (Button &button : buttons)
				{
					if (button.handle_click(evt, window_size, last_drawable_px))
						return true;
				}
			}

			/*auto tex_for = [&](int lvl)->GLuint {
				switch (lvl) {
				case 1: return stove_tint_lvl1;
				case 2: return stove_tint_lvl2;
				case 3: return stove_tint_lvl3;
				default: return stove_tint_lvl0;
				}
				};*/

			glm::vec2 mouse_win(float(evt.button.x), float(evt.button.y));
			glm::vec2 scale = glm::vec2(last_drawable_px) / glm::vec2(window_size);
			// glm::vec2 mouse_px = glm::vec2(float(evt.button.x), float(evt.button.y));
			glm::vec2 mouse_px = mouse_win * scale;

			Ray r = screen_point_to_world_ray(camera, mouse_px, last_drawable_px);
			/*	std::cout << "[Sizes] window=(" << window_size.x << "," << window_size.y
					<< ") drawable=(" << last_drawable_px.x << "," << last_drawable_px.y << ")\n";*/
			// std::cout << "[Ray] O=(" << r.origin.x << "," << r.origin.y << "," << r.origin.z
			// 		  << ") D=(" << r.dir.x << "," << r.dir.y << "," << r.dir.z << ")\n";
			last_ray = r;
			has_last_ray = true;

			// int new_level = 0;
			if (stove.try_toggle(r, nullptr))
			{
				// player->set_heat_level(new_level);
				AudioManager::play_event(AudioManager::Event::StoveClick);
				return true;
			}

			// if (player->melt_level > (player->MELT_MIN + player->MELT_MAX) / 2) {
			if (player->melt_level / player->MELT_MAX > player->MELT_FOR_GRAPPLE)
			{
				player->try_grapple(r, grapple_crackers);
			}
		}
	}
	else if (evt.type == SDL_EVENT_MOUSE_BUTTON_UP)
	{
		if (evt.button.button == SDL_BUTTON_LEFT)
		{
			if (player->grapple_point)
				player->release_grapple();
		}
	}

	return false;
}

void PlayMode::update(float elapsed)
{
	if (player->pause.downs > 0)
	{
		paused = !paused;
		if (paused)
		{
			vol_fade_rate = 2.0f;
		}
		else
		{
			vol_fade_rate = -2.0f;
		}
	}

	if (!paused)
	{
		player->update(elapsed);

		if (player->dead)
		{
			reset();
			return;
		}

		for (Rat *rat : rats)
			rat->update(elapsed);

		camera->transform->position.y = player->collision->position.y; // need to change this
		camera->transform->position.z = player->collision->position.z + 30.0f; // need to change this
		float last_wine = wine_remaining;
		wine_remaining = std::clamp(wine_remaining - elapsed, 0.0f, D_RANK_TIME);

		int last_rank, wine_rank;

		if (D_RANK_TIME - wine_remaining < S_RANK_TIME) {
			wine_rank = 5;
			last_rank = 5;
		}
		else if (D_RANK_TIME - wine_remaining < A_RANK_TIME) {
			wine_rank = 4;
			last_rank = D_RANK_TIME - last_wine < S_RANK_TIME ? 5 : 4;
		}
		else if (D_RANK_TIME - wine_remaining < B_RANK_TIME) {
			wine_rank = 3;
			last_rank = D_RANK_TIME - last_wine < A_RANK_TIME ? 4 : 3;
		}
		else if (D_RANK_TIME - wine_remaining < C_RANK_TIME) {
			wine_rank = 2;
			last_rank = D_RANK_TIME - last_wine < B_RANK_TIME ? 3 : 2;
		}
		else if (D_RANK_TIME - wine_remaining < D_RANK_TIME) {
			wine_rank = 1;
			last_rank = D_RANK_TIME - last_wine < C_RANK_TIME ? 2 : 1;
		}
		else {
			wine_rank = 0;
			last_rank = D_RANK_TIME - last_wine < D_RANK_TIME ? 1 : 0;
		}

		if (wine_rank != last_rank)
		{
			wine_bottle_ui.load_image_data(data_path("wine_bottle_" + std::to_string(wine_rank) + ".png"), OriginLocation::UpperLeftOrigin);
			wine_bottle_ui.create_mesh(Mode::window, bottle_ui_pos_x, bottle_ui_pos_y, bottle_ui_height);
		}

		if (wine_remaining <= 0.0f)
		{
			Mode::set_current(std::make_shared<MenuMode>(MenuMode::LoseMenu));
			return;
		}

	}

	player->pause.downs = 0;

	// music
	float base_kitchen_vol = 0.5f;
	pause_vol = std::clamp(pause_vol + (vol_fade_rate * elapsed), 0.0f, 1.0f);
	kitchen_music.set_volume(base_kitchen_vol * (1.0f - pause_vol), 1.f / 60.f);
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

	// set light camera transform and aspect ratio
	glm::vec3 eye = camera->transform->make_world_from_local()[3];
	glm::mat4 light_camera_view = camera->make_projection() * glm::mat4(camera->transform->make_local_from_world());

	//compute light uniforms:
	uint32_t lights = uint32_t(scene.lights.size());

	//clamp lights to maximum lights allowed by shader:
	lights = std::min< uint32_t >(lights, LitColorTextureProgram::MaxLights);

	// std::cout << "Number of lights: " << lights << std::endl;

	std::vector< int32_t > light_type; light_type.reserve(lights);
	std::vector< glm::vec3 > light_location; light_location.reserve(lights);
	std::vector< glm::vec3 > light_direction; light_direction.reserve(lights);
	std::vector< glm::vec3 > light_energy; light_energy.reserve(lights);
	std::vector< float > light_cutoff; light_cutoff.reserve(lights);

	for (auto const &light : scene.lights) {
		glm::mat4 light_to_world = light.transform->make_world_from_local();
		//set up lighting information for this light:
		light_location.emplace_back(glm::vec3(light_to_world[3]));
		light_direction.emplace_back(glm::vec3(-light_to_world[2]));
		light_energy.emplace_back(light.energy);

		if (light.type == Scene::Light::Point) {
			light_type.emplace_back(0);
			light_cutoff.emplace_back(1.0f);
		} else if (light.type == Scene::Light::Hemisphere) {
			light_type.emplace_back(1);
			light_cutoff.emplace_back(1.0f);
		} else if (light.type == Scene::Light::Spot) {
			light_type.emplace_back(2);
			light_cutoff.emplace_back(std::cos(0.5f * light.spot_fov));
		} else if (light.type == Scene::Light::Directional) {
			light_type.emplace_back(3);
			light_cutoff.emplace_back(1.0f);
		}

		//skip remaining lights if maximum light count reached:
		if (light_type.size() == lights) break;
	}

	GL_ERRORS();
	
	//--- actual drawing ---
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	//upload light uniforms:
	glUseProgram(lit_color_texture_program->program);

	glUniform3fv(lit_color_texture_program->EYE_vec3, 1, glm::value_ptr(eye));

	glUniform1ui(lit_color_texture_program->LIGHTS_uint, lights);

	if (lights > 0) {
		glUniform1iv(lit_color_texture_program->LIGHT_TYPE_int_array, lights, light_type.data());
		glUniform3fv(lit_color_texture_program->LIGHT_LOCATION_vec3_array, lights, glm::value_ptr(light_location[0]));
		glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3_array, lights, glm::value_ptr(light_direction[0]));
		glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3_array, lights, glm::value_ptr(light_energy[0]));
		glUniform1fv(lit_color_texture_program->LIGHT_CUTOFF_float_array, lights, light_cutoff.data());
	}



	scene.draw(light_camera_view);

	assert(wine_bottle_ui.data_created);
	if (wine_bottle_ui.data_created)
	{
		wine_bottle_ui.draw_mesh();
	}
	if (paused)
	{
		for (auto &button : buttons)
		{
			button.draw(drawable_size);
		}
	}
	{
		if (player->grapple_point)
		{
			// DEBUG
			float aspect = float(drawable_size.x) / float(drawable_size.y);
			float scale = std::min(
				// 2.0f * aspect / (Game::ArenaMax.x - Game::ArenaMin.x + 2.0f * Game::PlayerRadius),
				// 2.0f / (Game::ArenaMax.y - Game::ArenaMin.y + 2.0f * Game::PlayerRadius)
				2.0f * aspect, 2.0f * aspect);
			// glm::vec2 offset = -0.5f * (Game::ArenaMax + Game::ArenaMin);

			glm::mat4 world_to_clip = glm::mat4(
				scale / aspect, 0.0f, 0.0f, player->collision->position.x,
				0.0f, scale, 0.0f, player->collision->position.y,
				0.0f, 0.0f, 1.0f, player->collision->position.z,
				0.0f, 0.0f, 0.0f, 1.0f);

			DrawLines lines(world_to_clip);
			lines.draw(player->collision->position, player->grapple_point->position, glm::u8vec4(0xff, 0xff, 0x00, 0xff));
			// lines.draw(glm::vec3(0, 0, 0), glm::vec3(1, 1, 0), glm::u8vec4(0xff, 0xff, 0x00, 0xff));
		}
	}

	GL_ERRORS();
}

void PlayMode::reset()
{
	// player->collision->position = glm::vec3(0.0f, 77.41f, 30.301f);
	// player->locomotionState = (Player::PlayerLocomotion)0;
	// player->dead = false;
	Mode::set_current(std::make_shared<PlayMode>());
}

void PlayMode::load_level(int lvl) {
	current_level = lvl;
	Mode::set_current(std::make_shared<PlayMode>());
}

void PlayMode::load_next_level() {
	PlayMode::load_level(current_level + 1);
}