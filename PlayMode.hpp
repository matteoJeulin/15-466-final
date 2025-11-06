#pragma once

#include "Mode.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <cmath>

#include "Scene.hpp"
#include "Sound.hpp"
#include "TextManager.hpp"
#include "RayCast.hpp"
#include "Player.hpp"
#include "Rat.hpp"

struct PlayMode : Mode
{
	PlayMode();
	virtual ~PlayMode();

	// functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	// Resets the game state
	void reset();

	//----- game state -----

	//struct Ray {
	//	glm::vec3 origin;
	//	glm::vec3 dir; // normalized
	//};

	// local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	/*Scene::Transform *player = nullptr;
	Scene::Transform *goal = nullptr;
	Scene::Transform *deathPlane = nullptr;
	std::vector<Scene::Transform *> platforms;*/

	Scene::Transform* hot_plate = nullptr;
	Scene::Transform* cold_plate = nullptr;
	Scene::Transform* counter_top = nullptr;
	Scene::Transform* switch_1 = nullptr;
	Scene::Transform* switch_2 = nullptr;
	std::vector<Scene::Transform *> collision_platforms;
	std::vector<Scene::Transform *> collision_plates;
	std::vector<Scene::Transform *> grates;
	std::vector<Scene::Transform *> bouncy_weak_platforms;
	std::vector<Scene::Transform *> bouncy_strong_platforms;
	std::vector<Rat *> rats;


	Player *player = nullptr;

	// camera:
	Scene::Camera *camera = nullptr;

	// mouse:
	glm::uvec2 last_drawable_px = glm::uvec2(1, 1);
	Ray last_ray;
	bool has_last_ray = false;
	//static Ray screen_point_to_world_ray(Scene::Camera* cam, glm::vec2 mouse_px, glm::uvec2 window_size);

	// Text to display on screen
	std::string screen_text = "";

	// stove knobs:
	int knob_state_1 = 0;
	int knob_state_2 = 0;
};