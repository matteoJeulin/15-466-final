#include "Player.hpp"
#include "PlayMode.hpp"

#include "Mode.hpp"
#include "iostream"
#include <algorithm>
Player::Player(PlayMode *_game) : Character(_game)
{
	drawable = nullptr;
	set_heat_level(0);
}

void Player::update(float elapsed)
{
	// combine inputs into a move:
	if (left.pressed && !right.pressed)
		speed.y = std::max(speed.y - acceleration * elapsed, -maxSpeed);
	if (!left.pressed && right.pressed)
		speed.y = std::min(speed.y + acceleration * elapsed, maxSpeed);

	if (jump.pressed && !this->jumping && platform != nullptr)
	{
		jump.pressed = false;
		charJump(jumpHeight, jumpAirTime, gravity);
	}

	// Apply inertia to get the player down to 0 speed.
	if ((!left.pressed && !right.pressed) || (left.pressed && speed.y > 0) || (right.pressed && speed.y < 0))
	{
		speed.y -= speed.y * elapsed * 10;
	}

	applySpeed(elapsed);

	// Resolve collisions with the player
	if (!noclip)
	{
		platform = nullptr;

		for (Rat *rat : game->rats)
		{
			if (collide(rat->collision, true))
			{
				std::cout << "DEAD" << std::endl;
				std::cout.flush();
				game->reset();
				return;
			}
		}

		// Plate collision
		for (Scene::Transform *plate : game->collision_plates)
		{
			if (collide(plate, false))
			{
				melt_level += melt_delta * elapsed;
				melt_level = std::clamp(melt_level, MELT_MIN, MELT_MAX);
			}
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
				charJump(4.0f * height, jumpAirTime, gravity);
			}
		}

		for (Scene::Transform *bouncy : game->bouncy_strong_platforms)
		{
			if (collide(bouncy, true))
			{
				charJump(8.5f * height, jumpAirTime, gravity);
			}
		}

		for (Scene::Transform *plat : game->collision_platforms)
		{

			if (collide(plat, false))
			{
				jumping = false;
			}
		}
	}

	float rotation_angle = speed.y * elapsed;

	glm::quat rotation = glm::angleAxis(rotation_angle * 0.5f, glm::vec3(1, 0.0f, 0.0f));
	theta = theta * rotation;

	// Melt Logic
	{

		// DEBUG
		if (debug_heat.pressed)
		{
			melt_delta *= -1;
			debug_heat.pressed = false;
		}

		melt_level += -0.1f * std::abs(melt_delta) * elapsed;
		melt_level = std::clamp(melt_level, MELT_MIN, MELT_MAX);
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

		// Gemnin chat
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
		initialMeshBuffer.set(verticesCpu.data(), verticesCpu.size(), GL_DYNAMIC_DRAW);
	}
}

void Player::set_heat_level(int level) {
	heat_level = std::clamp(level, 0, 3);
	// knob to melt rate multipliers
	float rate_by_level[4] = { -1.0f, 0.5f, 1.0f, 2.0f };
	melt_delta = base_melt_rate * rate_by_level[heat_level];
}