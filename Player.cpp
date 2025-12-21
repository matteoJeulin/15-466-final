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
			if (jump.downs > 0) { // wall jump
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
				// platform = plate;

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
				else if (wall && (melt_level / MELT_MAX > MELT_FOR_CLING) && game->current_level >= 1 && wall->scale.z > height) {
					wall_cling(wall);
				}
			}
		}

		for (Scene::Transform *plat : game->wine_bottles)
		{
			if (collide(plat, true))
			{
				MenuMode::Rank rank;
				if (game->D_RANK_TIME - game->wine_remaining < game->S_RANK_TIME) rank = MenuMode::S;
				else if (game->D_RANK_TIME - game->wine_remaining < game->A_RANK_TIME) rank = MenuMode::A;
				else if (game->D_RANK_TIME - game->wine_remaining < game->B_RANK_TIME) rank = MenuMode::B;
				else if (game->D_RANK_TIME - game->wine_remaining < game->C_RANK_TIME) rank = MenuMode::C;
				else if (game->D_RANK_TIME - game->wine_remaining < game->D_RANK_TIME) rank = MenuMode::D;
				else rank = MenuMode::F; // Should be impossible

				game->totalScore += game->wine_remaining;
				AudioManager::play_event(AudioManager::Event::WineHit);

				Mode::set_current(std::make_shared<MenuMode>(MenuMode::LevelClearMenu, rank, static_cast<int>(game->wine_remaining * 1000)));
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

	float rotation_angle = 0.0f;
	glm::quat rotation = glm::angleAxis(rotation_angle * 0.5f, glm::vec3(1, 0.0f, 0.0f));

	if (locomotionState & PlayerLocomotion::Grappling) {
		rotation_angle = -grapple_angular_velocity * elapsed;
		rotation = glm::angleAxis(rotation_angle , glm::vec3(1, 0.0f, 0.0f));
	} else {
		// Fallback: rolling from ground speed
		rotation_angle = speed.y * elapsed;
		rotation = glm::angleAxis(rotation_angle * 0.5f, glm::vec3(1, 0.0f, 0.0f));
	}

	if (melt_level <= FULL_MELT || (locomotionState & PlayerLocomotion::Grappling)){
		theta = theta * rotation;
	}
	


	//----------------------------------------
	{ //
		// float height_range = cheese_top - cheese_base;
		wave_acc += elapsed / 5.0f; // 5 second wave animation cycle
		wave_acc -= std::floor(wave_acc);
		// float cheese_spread = 1.0f;

		// Target Brown (e.g., RGB: 139, 69, 19)
		// constexpr glm::vec4 TARGET_BROWN = glm::vec4(60.0f, 10.0f, 2.0f, 255.0f);

		verticesCpu = initialVerticesCpu;
		initialMeshBuffer.set(initialVerticesCpu.data(), verticesCpu.size(), GL_DYNAMIC_DRAW);

		float melt_percentage_level = 0.5f + (MELT_MAX - melt_level) / MELT_MAX;
		melt_percentage_level = std::clamp(melt_percentage_level, 0.0f, 1.0f);

		float melt_factor = (1.0f - melt_percentage_level);
		// // float flow = (1.0f + melt_factor * cheese_spread);

		verticesCpu = initialVerticesCpu;
    

                       
    // // Retrieve the new positions from the soft body (assuming a new getter exists)
    // const auto& new_softbody_positions = cheese_body.get_mass_point_positions();

    
	// grapple deform
	std::vector<float> grapple_influence(verticesCpu.size(), 0.0f);
    glm::vec3 grapple_target_local(0.0f); // cheese-local position of the hook
	std::vector<glm::vec3> world_rest(verticesCpu.size());

	glm::vec3 hook_world(0.0f);
	glm::vec3 ray_dir_world(0.0f);
	float ray_len_world = 0.0f;

	bool do_grapple_deform =
		(locomotionState & PlayerLocomotion::Grappling) &&
		(grapple_point != nullptr);

	if (do_grapple_deform && !verticesCpu.empty()) {
		// 1) define the ray in WORLD space: from cheese center to hook
		glm::vec3 origin_world = collision->position;
		glm::vec3 to_hook_world = grapple_point->position - origin_world;
		ray_len_world = glm::length(to_hook_world);

		if (ray_len_world > 1e-4f) {
			ray_dir_world = to_hook_world / ray_len_world;
			hook_world = grapple_point->position;

			// 2) precompute each vertex's WORLD rest position:
			//    world_rest = origin_world + (local * theta)
			for (std::size_t i = 0; i < verticesCpu.size(); ++i) {
				glm::vec3 local_rest = initialVerticesCpu[i].Position; // cheese-local
				glm::vec3 rotated    = local_rest * theta;             // same way you rotate elsewhere
				world_rest[i]        = origin_world + rotated;
			}

			// 3) Find 100 vertices closest to the ray near the HOOK end
			struct Candidate { std::size_t idx; float dist; };
			std::vector<Candidate> candidates;
			candidates.reserve(verticesCpu.size());

			// ---- parameters controlling how "gooey" it is ----
			float rope_radius = ray_len_world ;   // thickness of influence tube
			rope_radius = std::max(rope_radius, 0.1f);


			float hook_region_start = 0.0f;
			float hook_region_end   = ray_len_world*0.7f;

			for (std::size_t i = 0; i < verticesCpu.size(); ++i) {
				glm::vec3 p = world_rest[i];
				glm::vec3 rel = p - origin_world;

				// position along the rope in world:
				float t = glm::dot(rel, ray_dir_world);
				float t_clamped = std::clamp(t, hook_region_start, hook_region_end);

				glm::vec3 closest = origin_world + t_clamped * ray_dir_world;
				float dist = glm::length(p - closest);

				if (dist > rope_radius) continue; // too far from rope cylinder
				candidates.push_back({i, dist});
			}

			const std::size_t MAX_COUNT = 500;
			std::size_t keep = std::min<std::size_t>(MAX_COUNT, candidates.size());
			if (keep > 0) {
				std::nth_element(
					candidates.begin(),
					candidates.begin() + keep,
					candidates.end(),
					[](const Candidate &a, const Candidate &b) {
						return a.dist < b.dist;
					}
				);
				candidates.resize(keep);

				float maxDist = 0.0f;
				for (auto &c : candidates) maxDist = std::max(maxDist, c.dist);
				if (maxDist < 1e-4f) maxDist = 1e-4f;

				float sigma = maxDist * 0.5f;
				float twoSigma2 = 2.0f * sigma * sigma;
				const float MAX_PULL_STRENGTH = 0.35f; // <=1; how far toward hook we pull

				for (auto &c : candidates) {
					float d = c.dist;
					float w = std::exp(-(d * d) / twoSigma2); // Gaussian radial falloff
					w *= MAX_PULL_STRENGTH;
					grapple_influence[c.idx] = w;
				}
			}
		}
	}
	// glm::vec3 world_gravity_dir = glm::vec3(0.0f, 0.0f, -1.0f);
	float cheese_base = mesh->min.z;
	float cheese_top = mesh->max.z;
	// cheese_body.update(elapsed, collision->position, melt_factor, gravity, platform != nullptr,  game->collision_platforms, game->grates);
	if (melt_level >= 1 ){
		cheese_body.update_metaballs(elapsed, collision->position, -1.0f,melt_factor,platform != nullptr, game->collision_platforms, game->grates, cheese_base, cheese_top);
		cheese_body.metaball_timer += elapsed;
	}
	
	// // World gravity direction (e.g., -Z):
	// glm::vec3 world_gravity_dir = glm::vec3(0.0f, 0.0f, -1.0f);

	// // Convert to cheese-local space using theta:
	// glm::quat inv_T = glm::inverse(theta);
	// glm::vec3 gravity_dir_local = inv_T * world_gravity_dir;

	// // Run soft body:
	// cheese_body.apply_physics(
	// 	elapsed,
	// 	melt_percentage_level,
	// 	gravity,           // same scalar you use in character motion
	// 	gravity_dir_local
	// );
	float height_range = cheese_top - cheese_base;
	
	
				// // Gemnin chat
	for (std::size_t i = 0; i < verticesCpu.size(); ++i)
    {
        auto &vertex = verticesCpu[i];



			// If we're grappling, pull some vertices toward the hook along a Gaussian falloff.
			// local rest:
		glm::vec3 local_rest = initialVerticesCpu[i].Position;

		// base rotated local position (no goo):
		glm::vec3 rotated = local_rest * theta;


			float melt_level_z = ((rotated.z - cheese_base) * melt_percentage_level) + cheese_base;

			float melt_z_percent = ((rotated.z - cheese_base) / (height_range));

				if (melt_z_percent < melt_factor)
			{
				rotated.z = cheese_base;
			}
			else
			{
				// Deform the position:
				rotated.z = melt_level_z;
			}
		if (locomotionState & PlayerLocomotion::WallClinging) cheese_body.applyWallClingDeform(rotated);
		glm::vec3 world_pos = collision->position + rotated;

				// start from local rest pose:
    // glm::vec3 local = initialVerticesCpu[i].Position;

    // glm::vec3 rotated;
    // glm::vec3 world_pos;

    // if (!do_grapple_deform) {
    //     // --------------------------------------
    //     // CASE 1: MELT IN WORLD VERTICAL (GROUND)
    //     // --------------------------------------
    //     // First rotate the cheese so it's in its current orientation:
    //     rotated   = local * theta;
    //     world_pos = collision->position + rotated;

    //     // Use world Z to decide how much to squash:
    //     float world_bottom = collision->position.z + cheese_base;
    //     float world_top    = collision->position.z + cheese_top;
    //     float world_height = std::max(0.01f, world_top - world_bottom);

    //     float melt_z_percent = (world_pos.z - world_bottom) / world_height;
    //     melt_z_percent = std::clamp(melt_z_percent, 0.0f, 1.0f);

    //     // Example: lower part stays at bottom, upper part moves downward
    //     float t = melt_factor; // how melted we are
    //     if (melt_z_percent < t) {
    //         world_pos.z = world_bottom; // stuck to the bottom
    //     } else {
    //         // compress toward the bottom as we melt:
    //         float local_h = (world_pos.z - world_bottom) / world_height;
    //         float squashed_h = glm::mix(local_h, 0.5f, t); // tweak 0.5f for how tall the blob stays
    //         world_pos.z = world_bottom + squashed_h * world_height;
    //     }

    //     // Now convert the *melted* world position back into object/local:
    //     glm::vec3 rel = world_pos - collision->position;
    //     glm::quat inv_theta = glm::inverse(theta);
    //     glm::vec3 new_local = inv_theta * rel;
    //     rotated = new_local * theta; // rotated local again for consistency

    // } else {
    //     // --------------------------------------
    //     // CASE 2: MELT IN LOCAL CHEESE-Z (GRAPPLING)
    //     // --------------------------------------
    //     float melt_z_percent = (local.z - cheese_base) / height_range;
    //     melt_z_percent = std::clamp(melt_z_percent, 0.0f, 1.0f);

    //     if (melt_z_percent < melt_factor) {
    //         local.z = cheese_base;
    //     } else {
    //         // Simple compression along local z:
    //         float local_h = (local.z - cheese_base) / height_range;
    //         float squashed_h = glm::mix(local_h, 0.5f, melt_factor);
    //         local.z = cheese_base + squashed_h * height_range;
    //     }

    //     // THEN rotate the locally-melted shape:
    //     rotated   = local * theta;
    //     world_pos = collision->position + rotated;
    // }

		// If grappling, pull some vertices toward the hook in WORLD space:
		if (do_grapple_deform && grapple_influence[i] > 0.0f && ray_len_world > 1e-4f) {
			float w = grapple_influence[i]; // 0..MAX_PULL_STRENGTH
			glm::vec3 pulled_world = glm::mix(world_pos, hook_world, w);

			// Convert back to local so the GPU transform still works:
			glm::vec3 rel = pulled_world - collision->position;
			glm::quat inv_theta = glm::inverse(theta);
			glm::vec3 new_local = rel * inv_theta;   // inverse of (local * theta)

			// overwrite rotated/pos based on this new local:
			rotated   = new_local * theta;
			world_pos = collision->position + rotated;
		}

		// At this point, 'rotated' is your deformed local position
		glm::vec3 pos = rotated;    // object-space (no translation)
		vertex.Position = pos;
		// glm::vec4 original_color_f = glm::vec4(vertex.Color); // Already 0-255 range

		

			float r = std::hypot(pos.x, pos.y) + 0.01f;
			float sin_arg = float((r * 0.25f + wave_acc) * (2.0f * M_PI));
		// 	float h = std::sin(sin_arg);
			// float wave_amplitude = 0.00f; // Adjust this value to change the wave height

			float dh_dr = float(0.25f * 2.0f * M_PI * std::cos(sin_arg));
	
	


		float dr_dx = pos.x / r;
		float dr_dy = pos.y / r;

		glm::vec3 dp_dx = glm::vec3(1.0f, 0.0f, dh_dr * dr_dx );
		glm::vec3 dp_dy = glm::vec3(0.0f, 1.0f, dh_dr * dr_dy );

		glm::vec3 wave_normal = glm::normalize(glm::cross(dp_dx, dp_dy));

		// --- Grapple-adjusted normal ---
		glm::vec3 final_normal = wave_normal;

		// If this vertex is being pulled by the grapple, tilt its normal:
		if (do_grapple_deform && grapple_influence[i] > 0.0f) {
			// "Stretch direction" – roughly outward from center of cheese
			glm::vec3 radial_local = glm::normalize(pos); // from (0,0,0) to vertex

			// Blend wave normal and radial normal, using influence as weight (clamped)
			float n_w = std::clamp(grapple_influence[i], 0.0f, 1.0f); // 0..1
			final_normal = glm::normalize(glm::mix(wave_normal, radial_local, n_w));
		}

		vertex.Normal = final_normal;
		}
		initialMeshBuffer.set(verticesCpu.data(), verticesCpu.size(), GL_DYNAMIC_DRAW);
	}
	if(metaBallCube){
		// metaBallCube->position = collision->position + glm::vec3(0.0f, 0.0f, -1.0f);
		// std::cout<< metaBallCube->position.x << ", " << metaBallCube->position.y << ", " << metaBallCube->position.z << "\n"<<std::endl;
	}
	
	// pause.pressed = false;
	//updating shadow 
	applyBlobShadow();
}

void Player::set_heat_level(int level) {
	heat_level = std::clamp(level, 0, 3);
	// knob to melt rate multipliers
	// float rate_by_level[4] = { -1.0f, 0.5f, 1.0f, 2.0f };
	float rate_by_level[4] = { -1.0f, 2.0f, 1.0f, 2.0f };
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

	assert(grapple_angle >= 0.0f && grapple_angle <= M_PI);
	glm::vec3 to_hook_ws = grapple_point->position - collision->position; 
	glm::quat inv_theta   = glm::inverse(theta);                          // cheese rotation inverse
	grapple_attach_local  = inv_theta * to_hook_ws;                      
	grapple_attach_length = glm::length(grapple_attach_local);            //
	if (grapple_attach_length > 1e-4f) {
		grapple_attach_local /= grapple_attach_length; 
	} else {
		grapple_attach_local = glm::vec3(0, 0, 1); 
		grapple_attach_length = 1.0f;
	}

	assert(grapple_angular_velocity >= 0.0f);

	// determine which side I'm on so I know if my angle is positive or negative
	if (rope_dir[0] > 0) grapple_angle *= -1;

	// likewise, is my speed giving me positive or negative torque
	if ((speed.y * rope_vector[1]) - (speed.z * rope_vector[0]) < 0) grapple_angular_velocity *= -1;

	std::cout << "Attach angle = " << grapple_angle * 180.0f / M_PI << "\n";
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
	cheese_body.setWallCling(wallDir);
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
	cheese_body.clearWallCling();
	locomotionState = (Player::PlayerLocomotion)((locomotionState & ~Player::PlayerLocomotion::WallClinging) |
												  PlayerLocomotion::WallJumping);
					
}

void Player::release_wall()
{
	locomotionState = (Player::PlayerLocomotion)(locomotionState & ~Player::PlayerLocomotion::WallClinging);
	// collision->position.y += copysign(RELEASE_DIST, -wallDir); // nope lol
	speed.z = -slideSpeed;
	cheese_body.clearWallCling();
}

glm::vec3 Player::get_speed()
{
	if (locomotionState & PlayerLocomotion::WallClinging)
	{
		// unknown: have no (simple) way of knowing if wall is moving
		return glm::vec3(0.0f, 0.0f, 0.0f);
	}

	if (locomotionState & PlayerLocomotion::Grappling)
	{
		glm::vec3 return_speed = glm::vec3(0.0f, 0.0f, 0.0f);
		float lin_speed = grapple_angular_velocity * grapple_length; // v = wr
		return_speed.y = std::cos(grapple_angle) * lin_speed;
		return_speed.z = std::sin(grapple_angle) * lin_speed;

		return return_speed;
	}

	return glm::vec3(speed.x, speed.y, speed.z);
}