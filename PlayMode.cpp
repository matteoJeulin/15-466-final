#include "PlayMode.hpp"
#include "MenuMode.hpp"
#include "Mode.hpp"
#include "RayCast.hpp"
#include "LitColorTextureProgram.hpp"
#include "ShadowedColorTextureProgram.hpp"
#include "DepthOnlyProgram.hpp"
#include "BlobShadowPipeline.hpp"
#include "SoftBody.hpp"
#include "Framebuffers.hpp"

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
// define the global instance:
Framebuffers_shadows fbs;


Load<MeshBuffer> level_meshes(LoadTagDefault, []() -> MeshBuffer const *
							  {
	MeshBuffer const *ret = new MeshBuffer(data_path("Cheese.pnct"));
	level_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret; });

// Load< GLuint > meshes_for_shadowed_color_texture_program(LoadTagDefault, [](){
// 	return new GLuint(level_meshes->make_vao_for_program(shadowed_color_texture_program->program));
// });

// Load< GLuint > meshes_for_depth_only_program(LoadTagDefault, [](){
// 	return new GLuint(level_meshes->make_vao_for_program(depth_only_program->program));
// });


Load<Scene> level_scene(LoadTagDefault, []() -> Scene const *
						{ return new Scene(data_path("Cheese.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name)
										   {


        // static Scene::Drawable::Pipeline texture_pipeline = [&]() {
        //     Scene::Drawable::Pipeline p = shadowed_color_texture_program_pipeline;
        //     p.vao = *meshes_for_shadowed_color_texture_program;
        //     return p;
        // }();

        // static Scene::Drawable::Pipeline depth_pipeline = [&]() {
        //     Scene::Drawable::Pipeline p = depth_only_program_pipeline;
        //     p.vao = *meshes_for_depth_only_program;
        //     return p;
        // }();

												if (( transform->name == "Cheese_Wheel") ||(transform->name.substr(0, 5) == "Grate")) {
												// NOTE: Do NOT create a Scene::Drawable for collision meshes.
												// The transforms will still be loaded into scene.transforms.
												return; // Skip the rest of the function for this transform
												}
												 Mesh const &mesh = level_meshes->lookup(mesh_name);

												 scene.drawables.emplace_back(transform);
												 Scene::Drawable &drawable = scene.drawables.back();

												 drawable.pipeline[0] = lit_color_texture_program_pipeline;
												 drawable.pipeline[0].vao = level_meshes_for_lit_color_texture_program;
												 drawable.pipeline[0].type = mesh.type;
												 drawable.pipeline[0].start = mesh.start;
												 drawable.pipeline[0].count = mesh.count; 

												
											// // COLOR (lit) pipeline:
											// drawable.pipeline[Scene::Drawable::PipelineTypeDefault] = texture_pipeline;
											// drawable.pipeline[Scene::Drawable::PipelineTypeDefault].type  = mesh.type;
											// drawable.pipeline[Scene::Drawable::PipelineTypeDefault].start = mesh.start;
											// drawable.pipeline[Scene::Drawable::PipelineTypeDefault].count = mesh.count;

											// // SHADOW (depth) pipeline:
											// drawable.pipeline[Scene::Drawable::PipelineTypeShadow] = depth_pipeline;
											// drawable.pipeline[Scene::Drawable::PipelineTypeShadow].type  = mesh.type;
											// drawable.pipeline[Scene::Drawable::PipelineTypeShadow].start = mesh.start;
											// drawable.pipeline[Scene::Drawable::PipelineTypeShadow].count = mesh.count;


												 float roughness = 1.0f; //where can we get roughness of the material 
												 drawable.pipeline[0].set_uniforms = [roughness](){
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
	player->drawable->pipeline[0].vao = player->cheese_lit_color_texture_program;
	player->drawable->pipeline[0].type = player->mesh->type;
	player->drawable->pipeline[0].start = 0; // Starts from 0 in the new buffer
	player->drawable->pipeline[0].count = player->mesh->count;

	player->cheese_body.init(initial_vertices);


	//blob shadow mesh
	 // Build a unit quad in the XY plane at z = 0, centered at origin.
    // Local +Z is up (0,0,1) so your rotation-from-normal logic works.
    std::vector<DynamicMeshBuffer::Vertex> verts;
    verts.reserve(6);

    auto make_vertex = [](float x, float y, float u, float v) -> DynamicMeshBuffer::Vertex {
        DynamicMeshBuffer::Vertex vert;
        vert.Position = glm::vec3(x, y, 0.0f);          // in X-Y plane
        vert.Normal   = glm::vec3(0.0f, 0.0f, 1.0f);    // up
        vert.Color    = glm::u8vec4(0xff, 0xff, 0xff, 0xff); // white (ignored by blob branch if you want)
        vert.TexCoord = glm::vec2(u, v);                // [0,1] range
        return vert;
    };

    // Quad corners: (-1,-1), (1,-1), (1,1), (-1,1)
    DynamicMeshBuffer::Vertex v0 = make_vertex(-1.0f, -1.0f, 0.0f, 0.0f);
    DynamicMeshBuffer::Vertex v1 = make_vertex( 1.0f, -1.0f, 1.0f, 0.0f);
    DynamicMeshBuffer::Vertex v2 = make_vertex( 1.0f,  1.0f, 1.0f, 1.0f);
    DynamicMeshBuffer::Vertex v3 = make_vertex(-1.0f,  1.0f, 0.0f, 1.0f);

    // Two triangles: (0,1,2) and (0,2,3)
    verts.push_back(v0);
    verts.push_back(v1);
    verts.push_back(v2);

    verts.push_back(v0);
    verts.push_back(v2);
    verts.push_back(v3);

    // Upload once:
    blob_mesh.set(verts.data(), verts.size(), GL_STATIC_DRAW);

	//UI seteup 
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

	glDeleteVertexArrays(1, &player->shadow_vao);
	player->shadow_vao = 0;
	player->shadow_form = nullptr;

	if (stove_tint_lvl0) glDeleteTextures(1, &stove_tint_lvl0);
	if (stove_tint_lvl1) glDeleteTextures(1, &stove_tint_lvl1);
	if (stove_tint_lvl2) glDeleteTextures(1, &stove_tint_lvl2);
	if (stove_tint_lvl3) glDeleteTextures(1, &stove_tint_lvl3);

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

	//make sure framebuffers are the same size as the window:
	framebuffers.realloc(drawable_size);
	// update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	// for mouse
	last_drawable_px = drawable_size;


// 	glm::uvec2 shadow_atlas_size = glm::uvec2(2048, 2048);
//     glm::uvec2 shadow_tiles = glm::uvec2(4, 4); // must be >= MAX_LIGHTS tiles total

// 	fbs.allocate(drawable_size, shadow_atlas_size, shadow_tiles);

//     auto tileSize = fbs.tile_size();
//     glm::uvec2 tiles = fbs.shadow_tiles;

//     // glUseProgram(depth_only_program->program);

//     // --- build list of shadow-casting lights from scene.lights ---
//     static constexpr uint32_t MAX_SHADOW_LIGHTS = 8;

//     std::vector<Scene::Light const*> shadow_lights;
// 	shadow_lights.reserve(MAX_SHADOW_LIGHTS);


//    for (auto const &L : scene.lights) {
//         if (L.type != Scene::Light::Spot) continue;       // only spots get shadows (you can relax this)
//         // if you have a "casts_shadows" flag, check it here too.
//         shadow_lights.push_back(&L);
//         if (shadow_lights.size() >= MAX_SHADOW_LIGHTS) break;
//     }

//     uint32_t shadow_light_count = uint32_t(shadow_lights.size());

//     // --- SHADOW PASS: render all these lights into atlas ---
//     glBindFramebuffer(GL_FRAMEBUFFER, fbs.shadow_fb);
//     glViewport(0, 0, fbs.shadow_atlas_size.x, fbs.shadow_atlas_size.y);

//     glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
//     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//     glEnable(GL_DEPTH_TEST);
//     glDisable(GL_BLEND);

//     glCullFace(GL_FRONT);
//     glEnable(GL_CULL_FACE);

//     for (uint32_t idx = 0; idx < shadow_light_count; ++idx) {

//         if (idx >= tiles.x * tiles.y) break; // safety: ran out of tiles
// 		Scene::Light const *light = shadow_lights[idx];

//         uint32_t tile_x = idx % tiles.x;
//         uint32_t tile_y = idx / tiles.x;

//         GLint x = tile_x * tileSize.x;
//         GLint y = tile_y * tileSize.y;

//         glViewport(x, y, tileSize.x, tileSize.y);

//         scene.draw(*light, Scene::Drawable::PipelineTypeShadow);
//     }

//     glDisable(GL_CULL_FACE);
//     glBindFramebuffer(GL_FRAMEBUFFER, 0);

//     GL_ERRORS();

// 	glBindFramebuffer(GL_FRAMEBUFFER, 0); // (or fbs.fb if you want offscreen)
//     glViewport(0, 0, drawable_size.x, drawable_size.y);
	
// 	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
// 	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

// 	//set up basic OpenGL state:
// 	glEnable(GL_DEPTH_TEST);
// 	glEnable(GL_BLEND);
// 	glBlendEquation(GL_FUNC_ADD);
// 	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

// 	//set up light positions:
// 	glUseProgram(shadowed_color_texture_program->program);

// 	glm::mat4 world_to_clip =
//         camera->make_projection() * glm::mat4(camera->transform->make_local_from_world());

//     // --- use shadowed lighting shader ---
//     glUseProgram(shadowed_color_texture_program->program);

//     // sun/sky (you can tweak these)
//     glUniform3fv(shadowed_color_texture_program->sun_color_vec3, 1,
//                  glm::value_ptr(glm::vec3(0.0f, 0.0f, 0.0f)));
//     glUniform3fv(shadowed_color_texture_program->sun_direction_vec3, 1,
//                  glm::value_ptr(glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f))));
//     glUniform3fv(shadowed_color_texture_program->sky_color_vec3, 1,
//                  glm::value_ptr(glm::vec3(0.2f, 0.2f, 0.3f)));
//     glUniform3fv(shadowed_color_texture_program->sky_direction_vec3, 1,
//                  glm::value_ptr(glm::vec3(0.0f, 0.0f, 1.0f)));

//     // bind shadow atlas depth texture to unit 1 for sampler2DShadow
//     glActiveTexture(GL_TEXTURE1);
//     glBindTexture(GL_TEXTURE_2D, fbs.shadow_depth_tex);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
//     glActiveTexture(GL_TEXTURE0);

// 	glm::mat4 spot_from_light[MAX_SHADOW_LIGHTS];
// glm::vec3 spot_position[MAX_SHADOW_LIGHTS];
// glm::vec3 spot_direction[MAX_SHADOW_LIGHTS];
// glm::vec3 spot_color[MAX_SHADOW_LIGHTS];
// glm::vec2 spot_outer_inner[MAX_SHADOW_LIGHTS];


// glm::vec2 atlas_scale(1.0f / float(tiles.x), 1.0f / float(tiles.y));

// glm::mat4 bias(
//     0.5f, 0.0f, 0.0f, 0.5f,
//     0.0f, 0.5f, 0.0f, 0.5f,
//     0.0f, 0.0f, 0.5f, 0.5f,
//     0.0f, 0.0f, 0.0f, 1.0f
// );

// int spot_count = static_cast<int>(shadow_light_count);

// for (int i = 0; i < spot_count; ++i) {
//     Scene::Light const *light = shadow_lights[i];

//     glm::mat4 light_clip_from_world =
//         light->make_projection() *
//         glm::mat4(light->transform->make_local_from_world());

//     uint32_t tile_x = uint32_t(i) % tiles.x;
//     uint32_t tile_y = uint32_t(i) / tiles.x;

//     glm::vec2 atlas_offset(
//         tile_x * atlas_scale.x,
//         tile_y * atlas_scale.y
//     );

//     glm::mat4 atlas(
//         atlas_scale.x, 0.0f,         0.0f, atlas_offset.x,
//         0.0f,         atlas_scale.y, 0.0f, atlas_offset.y,
//         0.0f,         0.0f,          1.0f, 0.0f,
//         0.0f,         0.0f,          0.0f, 1.0f
//     );

//     spot_from_light[i] = atlas * bias * light_clip_from_world;

//     glm::mat4 world_from_light = light->transform->make_world_from_local();
//     spot_position[i] = glm::vec3(world_from_light[3]);
//     spot_direction[i] = glm::vec3(-world_from_light[2]);
//     spot_color[i] = light->energy;
//     spot_outer_inner[i] = glm::vec2(
//         std::cos(0.5f * light->spot_fov),
//         std::cos(0.85f * 0.5f * light->spot_fov)
//     );
// }

// glUniform1i(shadowed_color_texture_program->spot_count_int, spot_count);

//  if (spot_count > 0) {
//         glUniformMatrix4fv(
//             shadowed_color_texture_program->SPOT_FROM_LIGHT_mat4,
//             spot_count,
//             GL_FALSE,
//             glm::value_ptr(spot_from_light[0])
//         );

//         glUniform3fv(
//             shadowed_color_texture_program->spot_position_vec3,
//             spot_count,
//             glm::value_ptr(spot_position[0])
//         );

//         glUniform3fv(
//             shadowed_color_texture_program->spot_direction_vec3,
//             spot_count,
//             glm::value_ptr(spot_direction[0])
//         );

//         glUniform3fv(
//             shadowed_color_texture_program->spot_color_vec3,
//             spot_count,
//             glm::value_ptr(spot_color[0])
//         );

//         glUniform2fv(
//             shadowed_color_texture_program->spot_outer_inner_vec2,
//             spot_count,
//             glm::value_ptr(spot_outer_inner[0])
//         );
//     }

//     // now draw the scene with shadowed_color_texture_program_pipeline:
//     scene.draw(world_to_clip); // make sure Scene uses shadowed_color_texture_program_pipeline for lit objects

//     glUseProgram(0);
	/* Utilied boilerplate code for forwardlighting materials 
	FowardDrawMode
	https://github.com/15-466/15-466-lighting
	*/
	
	// // set light camera transform and aspect ratio
	glm::vec3 eye = camera->transform->make_world_from_local()[3];
	glm::mat4 light_camera_view = camera->make_projection() * glm::mat4(camera->transform->make_local_from_world());

	//compute light uniforms:
	uint32_t lights = uint32_t(scene.lights.size());

	//clamp lights to maximum lights allowed by shader:
	lights = std::min< uint32_t >(lights, LitColorTextureProgram::MaxLights);

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

	//--- actual drawing ---
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffers.hdr_fb);

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(light_camera_view);

	//draw shadows blobs 
if (player->shadow_valid && player->shadow_form) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE); // don't overwrite depth buffer

    glUseProgram(blob_shadow_program->program);

    // compute CLIP_FROM_OBJECT for the blob:
    glm::mat4 world_from_object = glm::mat4(player->shadow_form->make_world_from_local());
    glm::mat4 clip_from_object  = light_camera_view * world_from_object;

    glUniformMatrix4fv(blob_shadow_program->CLIP_FROM_OBJECT_mat4,
                       1, GL_FALSE, glm::value_ptr(clip_from_object));

    // simple dark color:
    glUniform4f(blob_shadow_program->BLOB_COLOR_vec4,
                0.0f, 0.0f, 0.0f, 1.0f); // black, 50% alpha

    glBindVertexArray(player->shadow_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6); // our quad

    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//apply a bloom effect:
	framebuffers.add_bloom();

	//copy scene to main window framebuffer:
	framebuffers.tone_map();



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

	PlayMode::load_level((int)current_level + 1);
}