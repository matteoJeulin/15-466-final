#pragma once

#include "Character.hpp"
#include "RayCast.hpp"
#include "DrawLines.hpp"
#include <numbers>
#include "SoftBody.hpp"

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

    /****************
     * Player physics
     ****************/
    enum PlayerLocomotion {
        Rolling = 0b1,
        Jumping = 0b10,
        Grappling = 0b100,
        WallClinging = 0b1000,
        WallJumping = 0b10000
    } locomotionState = (PlayerLocomotion)0;

    /********************
     * Vertical Movement
     ********************/
    const float height = 6.24f;
    const float jumpHeight = height * 2.0f;
    const float jumpAirTime = 0.8f;
    const float gravity = (2 * jumpHeight) / (pow(jumpAirTime / 2.0f, 2.0f));
    // float jumpSpeed = (jumpHeight - (0.5f * (gravity) * pow(jumpAirTime / 2.0f, 2.0f)))/(jumpAirTime/2);

    /*******************
     * Lateral Movement
     *******************/
    // Player's maximum speed (want a nice arc, so should travel 2x jump height in horizontal direction in a single bound)
    // const float cheeseMaxSpeed = 10.0f * 2.0f;
    const float maxSpeed = (jumpHeight * 2) / jumpAirTime;
    // Acceleration and max speed of the player, accounting for the smaller parent node of the mesh
    const float acceleration = maxSpeed * 4.0f;

    bool won = false;
    bool dead = false;

    // Rat interactions (getting chomped sends the cheese wheel up)
    bool chomped = false;
    float chompedTimer = 0.0f;
    const float CHOMP_VERT_KB = height * 2.5f; // knocks the cheese wheel upwards
    const float CHOMP_AIR_TIME = jumpAirTime * 1.5f;
    const float CHOMP_GRAVITY = (2 * CHOMP_VERT_KB) / (pow(CHOMP_AIR_TIME / 2.0f, 2.0f));
    const float CHOMP_VERT_KB_SPEED = CHOMP_GRAVITY * CHOMP_AIR_TIME * 0.5f;
    const float MERCY_INVINC = 1.0f;
    float mercyInvincTimer = 0.0f;
    void applyKnockbackSpeed(float elapsed);

    /******************
     * Melt Properties
     ******************/
    const float MELT_MIN = 0;
    const float MELT_MAX = 5;
    float melt_level = 0;
    float melt_delta = MELT_MAX; // positive means melting, negative means cooling
    float prev_melt_level = 0.0f;
    bool had_full_melt = false;

    const float MELT_FOR_GRAPPLE = 0.3f; // >=30% melt to grapple
    const float MELT_FOR_GRATE = 0.5f; // >=50% melt to pass through
    const float MELT_FOR_CLING = 0.7f; // >=70% melt to cling

    // Stove Heat
    void set_heat_level(int level);
    int heat_level = 0;
    float base_melt_rate = 2.0f;

    /************
     * Grappling
     ************/
    Scene::Transform *grapple_point = nullptr;
    float grapple_angle = 0.0f;
    float grapple_angular_velocity = 0.0f;
    float grapple_length = 0.0f;
    const float MIN_ROPE_LENGTH = height * 2;
    bool wasGrappling = false; // allows me to maintain momentum while preventing weird things when bonking on a corner
    bool try_grapple(const Ray& ray, std::vector<Scene::Transform*> points);
    void attach_grapple(Scene::Transform *best_point);
    void release_grapple();

    /******************
     * Wall Cling/Jump
     ******************/
    // Wall on which the player is clinging to
    // See Character.cpp Scene::Transform *wall = nullptr;
    const float STICK_TIME = 0.6f; // time until the player starts sliding
    const float SLIDE_MAX_SPEED = maxSpeed / 2.0f;
    const float SLIDE_ACCEL = SLIDE_MAX_SPEED * 4.0f;

    float wallDir = 0.0f; // -1 means wall is on the left, 1 means wall is on the right
    float stickHeight = 0.0f; // the height along the wall where I started sticking

    float stickTimer = 0.0f;
    float slideSpeed = 0.0f;
    float slideDistance = 0.0f; // how much the cheese has slid down the wall (if > stickHeight, release)
    
    float wallJumpTimer = 0.0f;
    
    void wall_cling(Scene::Transform *target);
    void applySlideSpeed(float elapsed);
    void wall_jump();
    void release_wall();

    // Angle to rotate the player
	glm::quat theta;

    //dynamic mesh data:
	DynamicMeshBuffer initialMeshBuffer;
	DynamicMeshBuffer meltedMeshBuffer;
	Mesh const *mesh = nullptr;
	std::vector<DynamicMeshBuffer::Vertex> initialVerticesCpu;
	std::vector<DynamicMeshBuffer::Vertex> verticesCpu;
    std::vector<SoftBodyMassPoint> massPoints;
    SoftBody cheese_body;



	//vao mapping wave data for lit_color_texture_program:
	GLuint cheese_lit_color_texture_program = 0;
	GLuint melted_cheese_lit_color_texture_program = 0;
	//drawable (in scene) associated with the wave data:
	Scene::Drawable *waveDrawable = nullptr;
	float wave_acc = 0.0f;

    void update(float elapsed) override;
};
