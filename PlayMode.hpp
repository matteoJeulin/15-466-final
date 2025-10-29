#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <cmath>

#include "TextManager.hpp"
#include "DynamicMeshBuffer.hpp"
#include "Mesh.hpp"
#include "RayCast.hpp"
struct PlayMode : Mode
{
	PlayMode();
	virtual ~PlayMode();

	// functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	// input tracking:
	struct Button
	{
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, jump, mute, debug_heat; // debug_temp flips temp delta between -MELT_MAX and MELT_MAX

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

	Scene::Transform* cheese_wheel = nullptr;
	Scene::Transform* collision_cheese_wheel = nullptr;
	Scene::Transform* hot_plate = nullptr;
	Scene::Transform* cold_plate = nullptr;
	Scene::Transform* counter_top = nullptr;
	Scene::Transform* gate = nullptr;
	Scene::Transform* switch_1 = nullptr;
	Scene::Transform* switch_2 = nullptr;
	std::vector<Scene::Transform *> collision_platforms;
	std::vector<Scene::Transform *> collision_plates;

	// camera:
	Scene::Camera *camera = nullptr;

	// mouse:
	glm::uvec2 last_drawable_px = glm::uvec2(1, 1);
	Ray last_ray;
	bool has_last_ray = false;
	//static Ray screen_point_to_world_ray(Scene::Camera* cam, glm::vec2 mouse_px, glm::uvec2 window_size);

	//glm::vec3 playerSpeed = glm::vec3(0.0f);



	// Player and shark speeds have different units because of their scale in blender
	// float jumpSpeed = 20.0f;

	// Player physics
	// Jumping
	float cheeseHeight = 6.24f;
	float jumpHeight = cheeseHeight * 2.0f;
	float jumpAirTime = 0.8f;
	// float gravity = 19.62f;
	float gravity = (2 * jumpHeight) / (pow(jumpAirTime/2.0f, 2.0f));
	// float jumpSpeed = (jumpHeight - (0.5f * (-gravity) * pow(jumpAirTime / 2.0f, 2.0f)))/(jumpAirTime/2);
	float jumpSpeed = gravity * (jumpAirTime / 2.0f);

	// Moving
	glm::vec3 cheeseSpeed = glm::vec3(0.0f);
	// Acceleration and max speed of the player, accounting for the smaller parent node of the mesh
	const float cheeseAcceleration = 7.5f * 4.0f;

	// Player's maximum speed (want a nice arc, so should travel 2x jump height in horizontal direction)
	// const float cheeseMaxSpeed = 10.0f * 2.0f;
	const float cheeseMaxSpeed = (jumpHeight * 2) / jumpAirTime;

	bool noclip = false;
	bool won = false;
	bool dead = false;

	bool jumping = false;

	// Platform on which the player is
	Scene::Transform *cheese_platform = nullptr;

	// Melt Properties
	const float MELT_MIN = 0;
	const float MELT_MAX = 5;
	float melt_level = 0;
	float melt_delta = MELT_MAX; // positive means melting, negative means cooling

	std::string screen_text = "";

	// Checks if the player is colliding with the top of a given platform and applies collision
	bool collide_platform_top(Scene::Transform *platform);

	// Checks if the player is colliding with the side of a given platform and applies collision
	bool collide_platform_side(Scene::Transform *platform);

		// Checks the collision between the player and an object with a rectangular hitbox
	bool collide(Scene::Transform *object);

	void ResolveCollisionAndSlide(Scene::Transform *object, glm::vec3& position, glm::vec3& current_velocity, 
                             const glm::vec3& collision_normal, float penetration_depth);


	//dynamic mesh data:
	DynamicMeshBuffer initial_cheese;
	DynamicMeshBuffer melted_cheese;
	Mesh const *cheese_mesh = nullptr;
	std::vector<DynamicMeshBuffer::Vertex> initial_cheese_vertices_cpu;
	std::vector<DynamicMeshBuffer::Vertex> cheese_vertices_cpu;
	glm::quat theta; 

	//vao mapping wave data for lit_color_texture_program:
	GLuint cheese_lit_color_texture_program = 0;
	GLuint melted_cheese_lit_color_texture_program = 0;
	//drawable (in scene) associated with the wave data:
	Scene::Drawable *wave_drawable = nullptr;
	float wave_acc = 0.0f;
};