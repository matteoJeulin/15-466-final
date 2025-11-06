#include <glm/glm.hpp>

#include "Scene.hpp"
#include "DynamicMeshBuffer.hpp"
#include "Mesh.hpp"

#include <vector>
#include <deque>
#include <cmath>

struct Player
{
    Player();

    // input tracking:
    struct Button
    {
        uint8_t downs = 0;
        uint8_t pressed = 0;
    } left, right, down, up, jump, mute, debug_heat; // debug_temp flips temp delta between -MELT_MAX and MELT_MAX

    Scene::Transform *player = nullptr;
    Scene::Transform *playerCollision = nullptr;
	Scene::Drawable *drawable = nullptr;

    // Player physics
    // Jumping
    float height = 6.24f;
    float jumpHeight = height * 2.0f;
    float jumpAirTime = 0.8f;
    float gravity = (2 * jumpHeight) / (pow(jumpAirTime / 2.0f, 2.0f));
    // float jumpSpeed = (jumpHeight - (0.5f * (-gravity) * pow(jumpAirTime / 2.0f, 2.0f)))/(jumpAirTime/2);
    float jumpSpeed = gravity * (jumpAirTime / 2.0f);

    // Moving
    glm::vec3 speed = glm::vec3(0.0f);
    // Acceleration and max speed of the player, accounting for the smaller parent node of the mesh
    const float acceleration = 7.5f * 4.0f;

    // Player's maximum speed (want a nice arc, so should travel 2x jump height in horizontal direction)
    // const float cheeseMaxSpeed = 10.0f * 2.0f;
    const float maxSpeed = (jumpHeight * 2) / jumpAirTime;

    bool noclip = false;
    bool won = false;
    bool dead = false;

    bool jumping = false;

    // Melt Properties
    const float MELT_MIN = 0;
    const float MELT_MAX = 5;
    float melt_level = 0;
    float melt_delta = MELT_MAX; // positive means melting, negative means cooling

    // Angle to rotate the player
	glm::quat theta;

    // Platform on which the player is
    Scene::Transform *platform = nullptr;

    //dynamic mesh data:
	DynamicMeshBuffer initialMeshBuffer;
	DynamicMeshBuffer meltedMeshBuffer;
	Mesh const *mesh = nullptr;
	std::vector<DynamicMeshBuffer::Vertex> initialVerticesCpu;
	std::vector<DynamicMeshBuffer::Vertex> verticesCpu;

	//vao mapping wave data for lit_color_texture_program:
	GLuint cheese_lit_color_texture_program = 0;
	GLuint melted_cheese_lit_color_texture_program = 0;
	//drawable (in scene) associated with the wave data:
	Scene::Drawable *waveDrawable = nullptr;
	float wave_acc = 0.0f;

    // Checks the collision between the player and an object with a rectangular hitbox
    bool collide(Scene::Transform *object, bool isTrigger);

    void update(float elapsed);
};