#pragma once

#include "Character.hpp"

struct Rat : public Character
{
    Rat(PlayMode *_game);

    void update(float elapsed) override;

    const float aggroRange = 40.0f;
    const float jumpRange = 1.0f;

    // Jumping
    const float height = 6.24f;
    const float jumpHeight = height * 2.0f;
    const float jumpAirTime = 0.8f;
    const float gravity = (2 * jumpHeight) / (pow(jumpAirTime / 2.0f, 2.0f));

    // Moving
    // Acceleration and max speed of the player, accounting for the smaller parent node of the mesh
    const float acceleration = 7.5f * 4.0f;

    // Player's maximum speed (want a nice arc, so should travel 2x jump height in horizontal direction)
    // const float cheeseMaxSpeed = 10.0f * 2.0f;
    const float maxSpeed = ((jumpHeight * 2) / (jumpAirTime)) / 2.5f ;
};