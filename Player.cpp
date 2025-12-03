#include "Player.hpp"
#include "PlayMode.hpp"
#include "MenuMode.hpp"

#include "Mode.hpp"
#include "iostream"
#include <algorithm>
#include "AudioManager.hpp"

Player::Player(PlayMode *_game) : Character(_game)
{
	drawable = nullptr;
	set_heat_level(0);

}

void Player::update(float elapsed)
{
	float last_melet = melt_level;
	prev_melt_level = last_melet;
	// combine inputs into a move:
	// std::cout << chomped << std::endl;
	if (chomped) {
		if (platform == nullptr || chompedTimer >= 0) {
			applyKnockbackSpeed(elapsed);
			chompedTimer -= elapsed;
		}
		else {
			chomped = false;
			speed.y = 0.0f;
		}
	}
	else {
		if (locomotionState & PlayerLocomotion::Grappling) {
			assert(this->grapple_point != nullptr);
			// get vector from cheese to grapple
			glm::vec2 rope_vector = glm::vec2(grapple_point->position.y - collision->position.y,
											grapple_point->position.z - collision->position.z);

			float grapple_distance = glm::length(rope_vector);
			if (grapple_distance >= MIN_ROPE_LENGTH) {
				// Thanks to Grace Daja (MCS '28), this PhysicsStackExchange thread
				// (https://physics.stackexchange.com/questions/469046/how-does-tension-work-for-a-simple-pendulum-what-force-is-at-play-to-keep-a-rig)
				// and this simulation (https://www.myphysicslab.com/pendulum/pendulum-en.html)
				// for helping me figure out the tension acceleration here
				// (T = (w^2 * L + gcos(theta))

				// glm::vec2 rope_dir = glm::normalize(rope_vector);
				// float vel_squared = std::powf(glm::length(speed), 2.0f);
				// float swing_theta = glm::angle(glm::vec2(0.0, 1.0f), rope_dir);
				// float tension_strength = (vel_squared / grapple_length) + (gravity * std::cos(swing_theta));
				// glm::vec2 tension_accel = rope_dir * tension_strength;

				grapple_length = std::max(grapple_distance, MIN_ROPE_LENGTH);

				float angular_accel = -gravity/grapple_length * std::sin(grapple_angle);
				grapple_angular_velocity += angular_accel * elapsed;
				grapple_angle += grapple_angular_velocity * elapsed;

				// std::cout << "Current angle = " << grapple_angle << "\n";
				// std::cout << "Current ang. vel = " << grapple_angular_velocity << "\n";

				// cheese directly below hook = grapple_angle is 0
				// counterclockwise = positive angle 
				glm::vec2 hook_to_cheese = grapple_length * glm::vec2(std::sin(grapple_angle),
																	-std::cos(grapple_angle));
				collision->position = glm::vec3(collision->position.x,
												grapple_point->position.y + hook_to_cheese[0],
												grapple_point->position.z + hook_to_cheese[1]);

				// apply tension force
			}
			else {
				// TODO: change angle as I fall
				attach_grapple(grapple_point);
				applySpeed(elapsed);
			}
		}
		else if (locomotionState & PlayerLocomotion::WallClinging) {
			if (jump.pressed) { // wall jump
				wall_jump();
				applySpeed(elapsed);
			}
			else if ((wallDir < 0 && !left.pressed) ||
					 (wallDir > 0 && !right.pressed) ||
					 slideDistance > stickHeight ||
					 melt_level / MELT_MAX < MELT_FOR_CLING) { // let go
				release_wall();
				applySpeed(elapsed);
			}
			else {
				applySlideSpeed(elapsed);
				stickTimer = std::clamp(stickTimer - elapsed, 0.0f, STICK_TIME);
			}
		}
		else if (locomotionState & PlayerLocomotion::WallJumping) {
			// no inertia from lack of input until the wall cling is over
			if (speed.y != 0.0f) this->locomotionState = (PlayerLocomotion)(this->locomotionState | PlayerLocomotion::Rolling);
			else this->locomotionState = (PlayerLocomotion)(this->locomotionState & ~PlayerLocomotion::Rolling);
			applySpeed(elapsed);

			if (wallJumpTimer <= 0.0f) locomotionState = (PlayerLocomotion)(locomotionState & ~PlayerLocomotion::WallJumping);
			wallJumpTimer -= elapsed;
		}
		else {
			if (left.pressed && !right.pressed && speed.y > -maxSpeed)
				speed.y = std::max(speed.y - acceleration * elapsed, -maxSpeed);
			if (!left.pressed && right.pressed && speed.y < maxSpeed)
				speed.y = std::min(speed.y + acceleration * elapsed, maxSpeed);

			if (jump.pressed && !this->jumping && platform != nullptr)
			{
				jump.pressed = false;
				charJump(jumpHeight, jumpAirTime, gravity);
				locomotionState = (PlayerLocomotion)(locomotionState | PlayerLocomotion::Jumping);
				AudioManager::play_event(AudioManager::Event::CheeseRegularJump);
			}

			// Apply inertia to get the player down to 0 speed.
			if ((!left.pressed && !right.pressed) || (left.pressed && speed.y > 0) || (right.pressed && speed.y < 0))
			{
				speed.y -= speed.y * elapsed * 10;
				if (abs(speed.y) < maxSpeed) wasGrappling = false;
			}

			if (speed.y != 0.0f) this->locomotionState = (PlayerLocomotion)(this->locomotionState | PlayerLocomotion::Rolling);
			else this->locomotionState = (PlayerLocomotion)(this->locomotionState & ~PlayerLocomotion::Rolling);
			applySpeed(elapsed);
		}
	}

	// Resolve collisions with the player
	if (!noclip)
	{
		platform = nullptr;

		if (mercyInvincTimer <= 0.0f) {
			chomped = false;
			for (Rat *rat : game->rats)
			{
				if (collide(rat->collision, true))
				{
					// std::cout << "DEAD" << std::endl;
					// std::cout.flush();
					// dead = true;
					std::cout << rat->collision->name << std::endl;
					std::cout << "CHOMPED" << std::endl;
					chomped = true;

					if (locomotionState & Player::PlayerLocomotion::Grappling) {
						assert(grapple_point != nullptr);
						release_grapple();
					}
					if (locomotionState & Player::PlayerLocomotion::WallClinging) {
						assert(wall != nullptr);
						release_wall();
					}

					speed.y = copysign(maxSpeed / 2.0f, collision->position.y - rat->collision->position.y);
					speed.z = CHOMP_VERT_KB_SPEED;
					chompedTimer = CHOMP_AIR_TIME;
					mercyInvincTimer = MERCY_INVINC;
					platform = nullptr;

					return;
				}
			}
		}
		else {
			if (!chomped) mercyInvincTimer = std::clamp(mercyInvincTimer - elapsed, 0.0f, MERCY_INVINC);
		}
		bool on_any_plate = false;
		int max_plate_level = 0;

		// Plate collision
		for (Scene::Transform *plate : game->collision_plates)
		{
			if (collide(plate, false))
			{
				on_any_plate = true;

				int level = game->stove.get_level_for_plate(plate);
				max_plate_level = std::max(max_plate_level, level);
			}
		}

		if (on_any_plate) {
			set_heat_level(max_plate_level);
			melt_level += melt_delta * elapsed;
			melt_level = std::clamp(melt_level, MELT_MIN, MELT_MAX);
		}
		else {
			set_heat_level(0);
		}

		for (Scene::Transform *grate : game->grates)
		{
			// Go throught the grate if melted enough
			collide(grate, melt_level > (MELT_MIN + MELT_MAX) / 2);
		}

		for (Scene::Transform *bouncy : game->bouncy_weak_platforms)
		{
			if (collide(bouncy, true))
			{
				release_wall();
				charJump(4.0f * height, jumpAirTime, gravity);
				locomotionState = (Player::PlayerLocomotion)(locomotionState | Player::PlayerLocomotion::Jumping);
				//AudioManager::play_cheese_jump(2.0f, 0.0f);
				AudioManager::play_event(AudioManager::Event::CheeseWeakJump);
				chomped = false;
				mercyInvincTimer = 0.0f;
			}
		}

		for (Scene::Transform *bouncy : game->bouncy_strong_platforms)
		{
			if (collide(bouncy, true))
			{
				release_wall();
				charJump(8.0f * height, jumpAirTime, gravity);
				locomotionState = (Player::PlayerLocomotion)(locomotionState | Player::PlayerLocomotion::Jumping);
				AudioManager::play_event(AudioManager::Event::CheeseStrongJump);
				chomped = false;
				mercyInvincTimer = 0.0f;
			}
		}

		for (Scene::Transform *plat : game->collision_platforms)
		{
			if (collide(plat, false))
			{
				// std::cout << "Landed\n";
				if (platform != nullptr)
				{
					this->locomotionState = (PlayerLocomotion)(this->locomotionState & ~(PlayerLocomotion::Jumping | PlayerLocomotion::WallJumping));
					if (chomped) {
						speed.y = 0.0f;
						mercyInvincTimer = MERCY_INVINC;
						chomped = false;
					}
				}
				else if (wall && (melt_level / MELT_MAX > MELT_FOR_CLING) && game->current_level >= 1) {
					wall_cling(wall);
				}
			}
		}

		for (Scene::Transform *plat : game->wine_bottles)
		{
			if (collide(plat, true))
			{
				if (game->current_level < game->num_levels - 1) Mode::set_current(std::make_shared<MenuMode>(MenuMode::LevelClearMenu));
				else Mode::set_current(std::make_shared<MenuMode>(MenuMode::WinMenu));
			}
		}
	}



	// Melt Logic
	{
		if (!chomped) { // don't cool down if I've been damaged
			// DEBUG
			if (debug_heat.pressed)
			{
				melt_delta *= -1;
				debug_heat.pressed = false;
			}

			if (!(locomotionState & PlayerLocomotion::Grappling)) { // if grappling, don't cool down! (it's ok if I'm clinging, though)
				melt_level += -0.1f * std::abs(melt_delta) * elapsed;
				melt_level = std::clamp(melt_level, MELT_MIN, MELT_MAX);
			}
		}
	}

	const float MELT_START = 2.0f;
	const float FULL_MELT = 3.0f; 
	const float RESOLIDIFY_THRESHOLD = 4.5f;

	float current = melt_level;

	// track if reached the melt range this cycle
	if ((prev_melt_level < FULL_MELT) && (current >= FULL_MELT)) {
		had_full_melt = true;
	}

	// melt sound when melt level from 2->3:
	if ((prev_melt_level < MELT_START) && (current >= MELT_START)) {
		AudioManager::play_event(AudioManager::Event::CheeseMelt, 1.0f, 0.0f);
	}

	// resolidify:
	if (had_full_melt && (prev_melt_level >= RESOLIDIFY_THRESHOLD) && (current < RESOLIDIFY_THRESHOLD)) {
		AudioManager::play_event(AudioManager::Event::CheeseResolidify, 1.0f, 0.0f);
		had_full_melt = false; // reset for next melt cycle
	}

	float rotation_angle = speed.y * elapsed;

	glm::quat rotation = glm::angleAxis(rotation_angle * 0.5f, glm::vec3(1, 0.0f, 0.0f));

	if (melt_level <= FULL_MELT){
		theta = theta * rotation;
	}
	


	//----------------------------------------
	{ //
		float cheese_base = mesh->min.z;
		float cheese_top = mesh->max.z;
		float height_range = cheese_top - cheese_base;
		wave_acc += elapsed / 5.0f; // 5 second wave animation cycle
		wave_acc -= std::floor(wave_acc);
		float cheese_spread = 1.0f;

		// Target Brown (e.g., RGB: 139, 69, 19)
		constexpr glm::vec4 TARGET_BROWN = glm::vec4(60.0f, 10.0f, 2.0f, 255.0f);

		verticesCpu = initialVerticesCpu;
		initialMeshBuffer.set(initialVerticesCpu.data(), verticesCpu.size(), GL_DYNAMIC_DRAW);

		float melt_percentage_level = 0.5f + (MELT_MAX - melt_level) / MELT_MAX;
		melt_percentage_level = std::clamp(melt_percentage_level, 0.0f, 1.0f);

		float melt_factor = (1.0f - melt_percentage_level);
		float flow = (1.0f + melt_factor * cheese_spread);

		verticesCpu = initialVerticesCpu;
    
    {
    // // Call soft body update
    // cheese_body.update(elapsed, 
    //                    collision->position, 
    //                    melt_factor, // Using the computed melt factor
    //                    gravity, 
    //                    platform != nullptr, 
    //                    game->collision_platforms, 
    //                    game->grates);
                       
    // // Retrieve the new positions from the soft body (assuming a new getter exists)
    // const auto& new_softbody_positions = cheese_body.get_mass_point_positions();

    
    // // 3. APPLY ROTATION, MELT, AND SOFTBODY DEFORMATION
    
    // // Ensure the size matches before iterating
    // if (new_softbody_positions.size() == verticesCpu.size()) {
    //     for (size_t i = 0; i < verticesCpu.size(); ++i)
    //     {
    //         auto &vertex = verticesCpu[i];
            
    //         // A. Apply Character Rotation (theta) to the initial position (or softbody position)
    //         // It's cleaner to have the softbody work in the local frame and then apply rotation here.
            
    //         // B. Apply Melt-Based Color/Radial Expansion (still needed for melt effect)
            
    //         // --- Melt and Color Logic (Copied from your existing code) ---
    //         glm::vec3 pos = new_softbody_positions[i]; // Use softbody position for melt logic below
    //         glm::vec4 original_color_f = glm::vec4(vertex.Color);

    //         // Re-calculate melt parameters based on the new, deformed position (pos)
    //         float melt_z_percent = ((pos.z - cheese_base) / (height_range));

    //         // Radial wave/flow logic (This should ideally be handled by the SoftBody spring physics)
    //         // Since SoftBody is handling deformation, we only keep the color and rotation here.
            

    //         if (melt_z_percent < melt_factor)
    //         {
    //             // This radial expansion is now redundant/conflicting with the SoftBody physics
    //             // Instead, we only handle the coloring and use the SoftBody position.
    //             glm::vec4 final_color_f = glm::mix(original_color_f, TARGET_BROWN, (1.0f - (melt_factor * melt_factor)));
    //             vertex.Color = glm::u8vec4(final_color_f);
    //         }
    //         // --- End Melt and Color Logic ---
            
            
    //         // C. Final Position Assignment (Integration)
    //         // Apply the softbody position and character rotation (theta)
    //         // The softbody position is relative to the character center (0,0,0)
            
    //         // Apply SoftBody Position (which is relative to the character's local origin)
    //         glm::vec3 softbody_local_pos = new_softbody_positions[i];
            
    //         // The final vertex position in the mesh buffer:
    //         // 1. Take the SoftBody position (local deformation).
    //         // 2. Apply the character's rotational quaternion (theta).
    //         vertex.Position = softbody_local_pos * theta; 
            
            
    //         // D. Normal Recalculation (Requires using the new deformed position)
    //         // Since the vertex position is now determined by the soft body, the normal should be too.
    //         // For a soft body mesh, normals are usually recalculated by averaging the face normals
    //         // of the adjacent faces *after* all vertex positions are set, but for this code's wave logic:
            
    //         // If the SoftBody handles all position changes, you need a proper mesh normal recalculation algorithm.
    //         // Since the code seems to have abandoned the wave amplitude (0.0f), we'll skip the derivative-based normal.
    //         // Instead, the SoftBody needs to update the Normals as well, or you need to recalculate them from the mesh.
            
    //         // For now, let's simplify and rely on the SoftBody to provide a position.
    //         // If the mesh is dynamic, a dedicated normal calculation pass is needed after the loop.

    //     }
    // } else {
    //     // Handle error: mismatch between mass points and vertices
    //     std::cerr << "Warning: SoftBody mass point count does not match mesh vertex count.\n";
    // }
	}

		// // Gemnin chat
		for (auto &vertex : verticesCpu)
		{
			vertex.Position = vertex.Position * theta;
			glm::vec3 pos = vertex.Position;
			glm::vec4 original_color_f = glm::vec4(vertex.Color); // Already 0-255 range

			float melt_level_z = ((pos.z - cheese_base) * melt_percentage_level) + cheese_base;

			float melt_z_percent = ((pos.z - cheese_base) / (height_range));

			float r = std::hypot(pos.x, pos.y) + 0.01f;
			float sin_arg = float((r * 0.25f + wave_acc) * (2.0f * M_PI));
			float h = std::sin(sin_arg);
			float wave_amplitude = 0.00f; // Adjust this value to change the wave height

			float dh_dr = float(0.25f * 2.0f * M_PI * std::cos(sin_arg));
			if (melt_z_percent < melt_factor)
			{
				vertex.Position.x = (1.0f + flow) * vertex.Position.x;
				vertex.Position.y = (1.0f + flow) * vertex.Position.y;
				// Lerp (Interpolate): new_color = (1.0 - factor) * start_color + factor * end_color
				glm::vec4 final_color_f = glm::mix(original_color_f, TARGET_BROWN, (1.0f - (melt_factor * melt_factor)));

				// Assign the result back to the vertex (rounding the floats to integers)
				vertex.Color = glm::u8vec4(final_color_f);
				vertex.Position.z = cheese_base + 0.1f + (melt_percentage_level)*std::abs(0 * h * wave_amplitude);

				// Apply deformation to the Z component (vertical axis for the cheese wheel)
				// Adjust the multiplier for the desired wave intensity0
			}
			else
			{

				// Deform the position:
				vertex.Position.z = melt_level_z + 0.1f + (melt_percentage_level)*std::abs(0 * h * wave_amplitude);
			}

			// Deform the normal (assuming the wave is propagating in the XY plane):

			// Recalculate derivative parts for the new normal vector:
			// dr/dx = x / r; dr/dy = y / r (from r = sqrt(x^2 + y^2))
			float dr_dx = pos.x / r;
			float dr_dy = pos.y / r;

			// Tangent vectors (dp_dx, dp_dy) for the surface:
			glm::vec3 dp_dx = glm::vec3(1.0f, 0.0f, dh_dr * dr_dx * wave_amplitude);
			glm::vec3 dp_dy = glm::vec3(0.0f, 1.0f, dh_dr * dr_dy * wave_amplitude);

			// New normal is the cross product of the tangent vectors:
			vertex.Normal = glm::normalize(glm::cross(dp_dx, dp_dy));
		}
		// cheese_body.update(elapsed, collision->position, melt_factor, gravity, platform != nullptr,  game->collision_platforms, game->grates);
		initialMeshBuffer.set(verticesCpu.data(), verticesCpu.size(), GL_DYNAMIC_DRAW);
	}

	

	// pause.pressed = false;
	//updating shadow 
	applyBlobShadow();
}

void Player::set_heat_level(int level) {
	heat_level = std::clamp(level, 0, 3);
	// knob to melt rate multipliers
	float rate_by_level[4] = { -1.0f, 0.5f, 1.0f, 2.0f };
	melt_delta = base_melt_rate * rate_by_level[heat_level];
}

void Player::applyKnockbackSpeed(float elapsed) {
	if (platform == nullptr)
        speed.z -= CHOMP_GRAVITY * elapsed;

    // y-axis is the forward/backward direction and the x-axis is the right/left direction
    collision->position +=  glm::vec3(0.0f, speed.y * elapsed, speed.z * elapsed);
}

/********************
 * Grapple Mechanics
 ********************/
// Based on try_toggle from Stove.cpp
bool Player::try_grapple(const Ray& ray, std::vector<Scene::Transform*> points) {
    // if (!scene_) return false;
	// std::cout << "Checking grapple...\n";

    // Find nearest Grapple Point* AABB hit
    Scene::Transform* best_point = nullptr;
    float best_t = std::numeric_limits<float>::max();

    for (auto& p : points) {
		// std::cout << "Checking point " << p->name << "...\n";
        glm::vec3 c, h;
        world_box(p, c, h);
        float tval;
        if (ray_box_intersect(ray, c, h, &tval) && tval < best_t) {
            best_t = tval;
            best_point = p;
        }
    }
    if (!best_point) {
		return false;
		locomotionState = (Player::PlayerLocomotion)(locomotionState & ~Player::PlayerLocomotion::Grappling);
	}

	std::cout << "Found grapple_point " << best_point->name << "!\n";
	attach_grapple(best_point);

    // // Set grapple_point
	// grapple_point = best_point;
	// locomotionState = (Player::PlayerLocomotion)(locomotionState | Player::PlayerLocomotion::Grappling);

	// glm::vec2 rope_vector = glm::vec2(grapple_point->position.y - collision->position.y,
	// 								  grapple_point->position.z - collision->position.z);
	// grapple_length = std::max(glm::length(rope_vector), MIN_ROPE_LENGTH);
	// glm::vec2 rope_dir = grapple_length > 0 ? glm::normalize(rope_vector) : glm::vec2(0.0f, 1.0f);
	// grapple_angle = glm::angle(glm::vec2(0.0f, -1.0f), -rope_dir);
	// grapple_angular_velocity = glm::length(speed) / grapple_length;

	// assert(grapple_angle >= 0.0f && grapple_angle <= std::numbers::pi);
	// assert(grapple_angular_velocity >= 0.0f);

	// // determine which side I'm on so I know if my angle is positive or negative
	// if (rope_dir[0] > 0) grapple_angle *= -1;

	// // likewise, is my speed giving me positive or negative torque
	// if ((speed.y * rope_vector[1]) - (speed.z * rope_vector[0]) < 0) grapple_angular_velocity *= -1;

	// std::cout << "Attach angle = " << grapple_angle * 180.0f / std::numbers::pi << "\n";
	// std::cout << "Attach ang. vel = " << grapple_angular_velocity << "\n";


	// // speed.y = 0.0f;

    // std::cout << "[Grappling] Attached to Grapple " << best_point->name << " (updated best point)\n";
    return true;
}

void Player::attach_grapple(Scene::Transform *best_point) {
	// Set grapple_point
	grapple_point = best_point;
	locomotionState = (Player::PlayerLocomotion)(locomotionState | Player::PlayerLocomotion::Grappling);

	glm::vec2 rope_vector = glm::vec2(grapple_point->position.y - collision->position.y,
									  grapple_point->position.z - collision->position.z);
	grapple_length = std::max(glm::length(rope_vector), MIN_ROPE_LENGTH);
	glm::vec2 rope_dir = grapple_length > 0 ? glm::normalize(rope_vector) : glm::vec2(0.0f, 1.0f);
	grapple_angle = glm::angle(glm::vec2(0.0f, -1.0f), -rope_dir);
	grapple_angular_velocity = glm::length(speed) / grapple_length;

	assert(grapple_angle >= 0.0f && grapple_angle <= std::numbers::pi);
	assert(grapple_angular_velocity >= 0.0f);

	// determine which side I'm on so I know if my angle is positive or negative
	if (rope_dir[0] > 0) grapple_angle *= -1;

	// likewise, is my speed giving me positive or negative torque
	if ((speed.y * rope_vector[1]) - (speed.z * rope_vector[0]) < 0) grapple_angular_velocity *= -1;

	std::cout << "Attach angle = " << grapple_angle * 180.0f / std::numbers::pi << "\n";
	std::cout << "Attach ang. vel = " << grapple_angular_velocity << "\n";

	// speed.y = 0.0f;

    std::cout << "[Grappling] Attached to Grapple " << best_point->name << " (updated best point)\n";
}

void Player::release_grapple() {
	float lin_speed = grapple_angular_velocity * grapple_length; // v = wr
	speed.y = std::cos(grapple_angle) * lin_speed;
	speed.z = std::sin(grapple_angle) * lin_speed;

	std::cout << "Release horizontal speed: " << speed.y << "/" << maxSpeed << "\n";

	grapple_point = nullptr;
	locomotionState = (Player::PlayerLocomotion)((locomotionState & ~Player::PlayerLocomotion::Grappling) |
												  Player::PlayerLocomotion::Jumping);

	wasGrappling = true;
}

/*************
 * Wall Cling
 *************/
void Player::wall_cling(Scene::Transform *target) {
	wall = target;
	locomotionState = (Player::PlayerLocomotion)(locomotionState | Player::PlayerLocomotion::WallClinging);
	locomotionState = (Player::PlayerLocomotion)(locomotionState & ~Player::PlayerLocomotion::WallJumping);

	slideSpeed = 0.0f;
	stickTimer = STICK_TIME;

	// Get the top of the wall: (height/2) + wall->position.z
	float wall_top = (wall->scale.z / 2.0f) + wall->position.z;

	// Get stick height along wall: wall->scale->z - (top of wall - collision->position.z)
	stickHeight = wall->scale.z - (wall_top - collision->position.z);
	slideDistance = 0.0f;

	assert(wall->position.y - collision->position.y != 0.0f);
	wallDir = copysign(1.0f, wall->position.y - collision->position.y);
}

void Player::applySlideSpeed(float elapsed) {
	if (stickTimer <= 0.0f) {
		slideSpeed = std::clamp(slideSpeed + (SLIDE_ACCEL * elapsed), 0.0f, SLIDE_MAX_SPEED);
		std::cout << "Slide speed: " << slideSpeed << "\n";
    }

    // y-axis is the forward/backward direction and the x-axis is the right/left direction
	slideDistance += slideSpeed * elapsed;

	float wall_bottom = wall->position.z - (wall->scale.z / 2.0f);
	float offset_z = wall_bottom + stickHeight - slideDistance;
    collision->position = glm::vec3(collision->position.x, collision->position.y, offset_z);
}

void Player::wall_jump() // "jump_time" is up and down
{
    // float jumpSpeed = (2 * jump_height) / (pow(jumpAirTime / 2.0f, 2.0f)) * (jumpAirTime / 2.0f);
    float jumpSpeed = (jumpHeight + (0.5f * gravity * pow((jumpAirTime / 2.0f), 2.0f))) / (jumpAirTime / 2.0f); 

    jumping = true;
	speed.y = copysign(maxSpeed, -wallDir);
    speed.z = jumpSpeed;
	wall = nullptr;
	wallJumpTimer = jumpAirTime * 0.6f;

	locomotionState = (Player::PlayerLocomotion)((locomotionState & ~Player::PlayerLocomotion::WallClinging) |
												  PlayerLocomotion::WallJumping);
}

void Player::release_wall()
{
	locomotionState = (Player::PlayerLocomotion)(locomotionState & ~Player::PlayerLocomotion::WallClinging);
	// collision->position.y += copysign(RELEASE_DIST, -wallDir); // nope lol
	speed.z = -slideSpeed;
}