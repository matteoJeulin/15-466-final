#pragma once

#include "Character.hpp"

struct Player : public Character
{
    Player(PlayMode* _game);

    // input tracking:
    struct Button
    {
        uint8_t downs = 0;
        uint8_t pressed = 0;
    } left, right, down, up, jump, mute, pause, debug_heat; // debug_temp flips temp delta between -MELT_MAX and MELT_MAX

	Scene::Drawable *drawable = nullptr;

    // Player physics
    // Jumping
    const float height = 6.24f;
    const float jumpHeight = height * 2.0f;
    const float jumpAirTime = 0.8f;
    const float gravity = (2 * jumpHeight) / (pow(jumpAirTime / 2.0f, 2.0f));
    // float jumpSpeed = (jumpHeight - (0.5f * (-gravity) * pow(jumpAirTime / 2.0f, 2.0f)))/(jumpAirTime/2);

    // Moving
    // Acceleration and max speed of the player, accounting for the smaller parent node of the mesh
    const float acceleration = 7.5f * 4.0f;

    // Player's maximum speed (want a nice arc, so should travel 2x jump height in horizontal direction)
    // const float cheeseMaxSpeed = 10.0f * 2.0f;
    const float maxSpeed = (jumpHeight * 2) / jumpAirTime;

    bool won = false;
    bool dead = false;

    enum PlayerLocomotion {
        Rolling = 0b1,
        Jumping = 0b10,
        Grappling = 0b100
    } locomotionState = (PlayerLocomotion)0;

    // Grappling
    Scene::Transform *grapple_point = nullptr;

    // Melt Properties
    const float MELT_MIN = 0;
    const float MELT_MAX = 5;
    float melt_level = 0;
    float melt_delta = MELT_MAX; // positive means melting, negative means cooling

    // Stove Heat
    void set_heat_level(int level);
    int heat_level = 0;
    float base_melt_rate = 2.0f;

    // Angle to rotate the player
	glm::quat theta;

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

    void update(float elapsed) override;
};