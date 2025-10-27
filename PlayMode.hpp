#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

#include "TextManager.hpp"
#include "DynamicMeshBuffer.hpp"

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

	// local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	/*Scene::Transform *player = nullptr;
	Scene::Transform *goal = nullptr;
	Scene::Transform *deathPlane = nullptr;
	std::vector<Scene::Transform *> platforms;*/

	Scene::Transform* cheese_wheel = nullptr;
	Scene::Transform* hot_plate = nullptr;
	Scene::Transform* cold_plate = nullptr;
	Scene::Transform* counter_top = nullptr;
	Scene::Transform* gate = nullptr;
	std::vector<Scene::Transform *> collision_platforms;

	// camera:
	Scene::Camera *camera = nullptr;

	//glm::vec3 playerSpeed = glm::vec3(0.0f);

	glm::vec3 cheeseSpeed = glm::vec3(0.0f);
	// Acceleration and max speed of the player, accounting for the smaller parent node of the mesh
	const float cheeseAcceleration = 7.5f;

	// Player's maximum speed
	const float cheeseMaxSpeed = 10.0f;

	// Player and shark speeds have different units because of their scale in blender
	float jumpSpeed = 10.0f;

	// Gravitational force
	float gravity = 19.62f;

	bool noclip = false;
	bool won = false;
	bool dead = false;

	bool jumping = false;

	// Position of the player in the previous frame
	glm::vec3 previous_cheese_pos;
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

	//dynamic mesh data:
	DynamicMeshBuffer initial_cheese;
	DynamicMeshBuffer melted_cheese;
	Mesh const *cheese_mesh = nullptr;
	std::vector<DynamicMeshBuffer::Vertex> initial_cheese_vertices_cpu;
	std::vector<DynamicMeshBuffer::Vertex> cheese_vertices_cpu;
	//vao mapping wave data for lit_color_texture_program:
	GLuint cheese_lit_color_texture_program = 0;
	GLuint melted_cheese_lit_color_texture_program = 0;
	//drawable (in scene) associated with the wave data:
	Scene::Drawable *wave_drawable = nullptr;
	float wave_acc = 0.0f;
	float min_z_value = 0.0;
};
