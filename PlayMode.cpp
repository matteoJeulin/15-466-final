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

GLuint level_meshes_for_lit_color_texture_program = 0;
Load<MeshBuffer> level_meshes(LoadTagDefault, []() -> MeshBuffer const *
							  {
	MeshBuffer const *ret = new MeshBuffer(data_path("level.pnct"));
	level_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret; });

Load<Scene> level_scene(LoadTagDefault, []() -> Scene const *
						{ return new Scene(data_path("level.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name)
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
		if (transform.name == "Player")
			player = &transform;
		else if (transform.name == "DeathPlane")
			deathPlane = &transform;
		else if (transform.name.substr(0, 4) == "Plat")
		{
			platforms.emplace_back(&transform);
			if (transform.name == "PlatEnd")
				goal = &transform;
		}
	}
	if (player == nullptr)
		throw std::runtime_error("Player not found.");
	if (deathPlane == nullptr)
		throw std::runtime_error("Death plane not found.");
	if (goal == nullptr)
		throw std::runtime_error("Goal not found.");

	// get pointer to camera for convenience:
	if (scene.cameras.size() != 1)
		throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
}

PlayMode::~PlayMode()
{
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
		else if (evt.key.key == SDLK_W || evt.key.key == SDLK_UP)
		{
			up.downs += 1;
			up.pressed = true;
			return true;
		}
		else if (evt.key.key == SDLK_S || evt.key.key == SDLK_DOWN)
		{
			down.downs += 1;
			down.pressed = true;
			return true;
		}
		else if (evt.key.key == SDLK_SPACE)
		{
			jump.downs += 1;
			jump.pressed = true;
			return true;
		}
		else if (evt.key.key == SDLK_M)
		{
			mute.downs += 1;
			mute.pressed = true;
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
		else if (evt.key.key == SDLK_W || evt.key.key == SDLK_UP)
		{
			up.pressed = false;
			return true;
		}
		else if (evt.key.key == SDLK_S || evt.key.key == SDLK_DOWN)
		{
			down.pressed = false;
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
	else if (evt.type == SDL_EVENT_MOUSE_MOTION)
	{
		if (SDL_GetWindowRelativeMouseMode(Mode::window) == true)
		{
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				evt.motion.yrel / float(window_size.y));
			// Make sure the rotation does not roll the character by multiplying yaw on one side
			// and pitch on the other (source: https://stackoverflow.com/questions/46738139/prevent-rotation-around-certain-axis-with-quaternion)
			player->rotation = glm::normalize(
				glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 0.0f, 1.0f)) * player->rotation * glm::angleAxis(-motion.y * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f)));
			return true;
		}
	}

	return false;
}

bool PlayMode::collide_platform_top(Scene::Transform *platform)
{
	glm::vec3 &player_pos = player->position;
	glm::vec3 player_size = player->scale;

	glm::vec3 platform_pos = platform->position;
	glm::vec3 platform_size = platform->scale;
	// Check the x and y coordinates
	if (player_pos.x <= platform_pos.x + platform_size.x && player_pos.x >= platform_pos.x - platform_size.x &&
		player_pos.y <= platform_pos.y + platform_size.y && player_pos.y >= platform_pos.y - platform_size.y &&
		// Check if the elevation is correct
		player_pos.z - player_size.z < platform_pos.z + platform_size.z && previous_player_pos.z - player_size.z >= platform_pos.z + platform_size.z)
	{
		player_pos.z = platform_pos.z + platform_size.z + player_size.z;
		playerSpeed.z = 0.0f;
		player_platform = platform;
		return true;
	}

	return false;
}

bool PlayMode::collide_platform_side(Scene::Transform *platform)
{
	glm::vec3 &player_pos = player->position;
	glm::vec3 player_size = player->scale;

	glm::vec3 platform_pos = platform->position;
	glm::vec3 platform_size = platform->scale;

	// Check the collision with each if the sides individually
	if (player_pos.x <= platform_pos.x + platform_size.x && player_pos.x >= platform_pos.x - platform_size.x &&
		player_pos.z <= platform_pos.z + platform_size.z && player_pos.z >= platform_pos.z - platform_size.z &&
		player_pos.y - player_size.y < platform_pos.y + platform_size.y && previous_player_pos.y - player_size.y >= platform_pos.y + platform_size.y)
	{
		player_pos.y = platform_pos.y + platform_size.y + player_size.y;
		playerSpeed.y = 0.0f;
		return true;
	}

	if (player_pos.x <= platform_pos.x + platform_size.x && player_pos.x >= platform_pos.x - platform_size.x &&
		player_pos.z <= platform_pos.z + platform_size.z && player_pos.z >= platform_pos.z - platform_size.z &&
		player_pos.y + player_size.y > platform_pos.y - platform_size.y && previous_player_pos.y + player_size.y <= platform_pos.y - platform_size.y)
	{
		player_pos.y = platform_pos.y - platform_size.y - player_size.y;
		playerSpeed.y = 0.0f;
		return true;
	}

	if (player_pos.z <= platform_pos.z + platform_size.z && player_pos.z >= platform_pos.z - platform_size.z &&
		player_pos.y <= platform_pos.y + platform_size.y && player_pos.y >= platform_pos.y - platform_size.y &&
		player_pos.x - player_size.x < platform_pos.x + platform_size.x && previous_player_pos.x - player_size.x >= platform_pos.x + platform_size.x)
	{
		player_pos.x = platform_pos.x + platform_size.x + player_size.x;
		playerSpeed.x = 0.0f;
		return true;
	}

	if (player_pos.z <= platform_pos.z + platform_size.z && player_pos.z >= platform_pos.z - platform_size.z &&
		player_pos.y <= platform_pos.y + platform_size.y && player_pos.y >= platform_pos.y - platform_size.y &&
		player_pos.x + player_size.x > platform_pos.x - platform_size.x && previous_player_pos.x + player_size.y <= platform_pos.x - platform_size.x)
	{
		player_pos.x = platform_pos.x - platform_size.x - player_size.x;
		playerSpeed.x = 0.0f;
		return true;
	}

	return false;
}

void PlayMode::update(float elapsed)
{
			// combine inputs into a move:
		if (left.pressed && !right.pressed)
			playerSpeed.x = std::max(playerSpeed.x - playerAcceleration * elapsed, -playerMaxSpeed);
		if (!left.pressed && right.pressed)
			playerSpeed.x = std::min(playerSpeed.x + playerAcceleration * elapsed, playerMaxSpeed);
		if (down.pressed && !up.pressed)
			playerSpeed.y = std::max(playerSpeed.y - playerAcceleration * elapsed, -playerMaxSpeed);
		if (!down.pressed && up.pressed)
			playerSpeed.y = std::min(playerSpeed.y + playerAcceleration * elapsed, playerMaxSpeed);
		if (jump.pressed && !jumping && player_platform != nullptr)
		{
			playerSpeed.z = jumpSpeed;
			jump.pressed = false;
			jumping = true;
		}

		// Apply inertia to get the player down to 0 speed.
		if ((!left.pressed && !right.pressed) || (left.pressed && playerSpeed.x > 0) || (right.pressed && playerSpeed.x < 0))
		{
			playerSpeed.x -= playerSpeed.x * elapsed * 10;
		}
		if ((!up.pressed && !down.pressed) || (up.pressed && playerSpeed.y < 0) || (down.pressed && playerSpeed.y > 0))
		{
			playerSpeed.y -= playerSpeed.y * elapsed * 10;
		}

		playerSpeed.z -= gravity * elapsed;

		glm::mat4x3 frame = player->make_parent_from_local();
		glm::vec3 frame_forward = -frame[0];
		glm::vec3 frame_right = frame[1];

		frame_forward.z = 0;
		frame_forward = glm::normalize(frame_forward);
		frame_right.z = 0;
		frame_right = glm::normalize(frame_right);


		previous_player_pos = player->position;

		// y-axis is the forward/backward direction and the x-axis is the right/left direction
		player->position += playerSpeed.x * frame_right * elapsed + playerSpeed.y * frame_forward * elapsed + playerSpeed.z * glm::vec3(0.0f, 0.0f, 1.0f) * elapsed;

}

void PlayMode::draw(glm::uvec2 const &drawable_size)
{

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f); // 1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDepthFunc(GL_LESS); // this is the default depth comparison function, but FYI you can change it.

	GL_ERRORS();
}
