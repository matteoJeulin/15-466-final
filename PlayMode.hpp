#pragma once

#include "Mode.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>

#include "Scene.hpp"
#include "Sound.hpp"
#include "DynamicSoundLoop.hpp"
#include "TextManager.hpp"
#include "UIElement.hpp"
#include "DynamicMeshBuffer.hpp"
#include "RayCast.hpp"
#include "Player.hpp"
#include "Rat.hpp"
#include "Mesh.hpp"
#include "Stove.hpp"
#include "MovingWall.hpp"
#include "Button.hpp"
#include "Level.hpp"


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
	std::vector<Scene::Transform *> collision_platforms;
	std::vector<Scene::Transform *> collision_plates;
	std::vector<Scene::Transform *> grates;
	std::vector<Scene::Transform *> bouncy_weak_platforms;
	std::vector<Scene::Transform *> bouncy_strong_platforms;
	std::vector<Scene::Transform *> grapple_crackers;
	std::vector<Scene::Transform *> spawn_locations;
	std::vector<MovingWall *> moving_walls;
	std::vector<Scene::Transform *> wine_bottles;
	std::vector<Rat *> rats;
	std::unordered_map<Scene::Transform*, Rat*> rat_map;
	StoveSystem stove;

	Player *player = nullptr;

	// camera:
	Scene::Camera *camera = nullptr;
	Scene::Camera *light_camera = nullptr;

	bool paused = false;

	// mouse:
	glm::uvec2 last_drawable_px = glm::uvec2(1, 1);
	Ray last_ray;
	bool has_last_ray = false;
	//static Ray screen_point_to_world_ray(Scene::Camera* cam, glm::vec2 mouse_px, glm::uvec2 window_size);

	// Text to display on screen
	std::string screen_text = "";

	// stove:
	// int knob_state_1 = 0;
	// int knob_state_2 = 0;
	GLuint stove_tint_lvl0 = 0, stove_tint_lvl1 = 0, stove_tint_lvl2 = 0, stove_tint_lvl3 = 0;
	Scene::Drawable* stove_drawable = nullptr; 

	/*********************
	 * Level Information:
	 *********************/
	// Level contains rank information, spawn location, and camera blocks
	std::vector<Level> levels = {};
	static inline size_t current_level = 0;
	static inline size_t num_levels = 2;

	// Should put the mode into play mode.
	static void load_level(int lvl); // 0-indexed
	static void load_next_level();
	std::vector<Scene::Transform *> spawn_positions;
	// Game Timer (+UI)
	// To be set when the level is loaded
	float S_RANK_TIME = 60.0f;
	float A_RANK_TIME = 80.0f;
	float B_RANK_TIME = 100.0f;
	float C_RANK_TIME = 120.0f;
	float D_RANK_TIME = 140.0f;

	bool foundLevel = false;
	std::string spawnLocName;
	glm::vec3 spawnPos = glm::vec3();
	static float totalScore;
	static float totalSRankTime;
	static float totalARankTime;
	static float totalBRankTime;
	static float totalCRankTime;
	static float totalDRankTime;

	// level camera blocks blocks
	int numberOfCameraBlocks = 0;
	CameraBlock cameraBlocks[64];
	float CAMERA_CORRECTION_SPEED = 6.24f / 0.4f; // per second

	void camera_update(float elapsed);

	float wine_remaining = D_RANK_TIME;
	UIElement wine_bottle_ui;
	float bottle_ui_pos_x = 0.9f;
	float bottle_ui_pos_y = 0.6f;
	float bottle_ui_height = 0.8f;

	// Pause menu displayables
	std::vector<Button> buttons;

	// Music
	DynamicSoundLoop kitchen_music;
	DynamicSoundLoop pause_music;
	float pause_vol = 0.0f;
	float vol_fade_rate = -2.0f; // per second


	//shadow
	DynamicMeshBuffer blob_mesh;
};