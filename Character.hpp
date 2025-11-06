#pragma once

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/vector_angle.hpp>

#include "Scene.hpp"
#include "DynamicMeshBuffer.hpp"
#include "Mesh.hpp"

#include <vector>
#include <deque>
#include <cmath>

// Forward declaration to break circular dependency
struct PlayMode;

struct Character
{
    Character(PlayMode *_game);

    virtual void update(float elapsed);

    // Gamestate
    PlayMode *game;

    // Circular collision shape
    Scene::Transform *collision = nullptr;
    // Model transform
    Scene::Transform *model = nullptr;

    // Moving
    glm::vec3 speed = glm::vec3(0.0f);
    // Acceleration and max speed of the player, accounting for the smaller parent node of the mesh
    const float acceleration = 7.5f * 4.0f;
    const float maxSpeed = 10.0f;

    // Jumping
    const float height = 6.24f;
    const float jumpHeight = height * 2.0f;
    const float jumpAirTime = 0.8f;
    const float gravity = (2 * jumpHeight) / (pow(jumpAirTime / 2.0f, 2.0f));

    bool jumping = false;

    bool noclip = false;

    // Platform on which the player is
    Scene::Transform *platform = nullptr;

    // Collision between a character and an object
    bool collide(Scene::Transform *object, bool isTrigger);

    // Make the character jump to jump_height
    void charJump(float char_jump_height, float jump_time, float jump_grav);

    void applySpeed(float elapsed);
};