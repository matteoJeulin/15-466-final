#include "Player.hpp"

Player::Player()
{
	drawable = nullptr;
}

bool Player::collide(Scene::Transform *object, bool isTrigger)
{
	glm::vec3 object_pos = glm::vec3(object->position);
	object_pos.x = 0.0f;
	glm::vec3 object_size = object->scale;

	glm::vec3 &playerPos = playerCollision->position;
	playerPos.x = 0.0f;
	glm::vec3 playerSize = playerCollision->scale;

	// Transform player position from world space to object's local space
	glm::quat invRotation = glm::inverse(object->rotation);
	glm::vec3 playerCenter = invRotation * (playerPos - object_pos);
	playerCenter.x = 0.0f;

	// Taken from https://developer.mozilla.org/en-US/docs/Games/Techniques/3D_collision_detection
	// Check the collision between a circle and a bounding box
	auto intersects = ([](glm::vec3 sphereCenter, float sphereRadius, glm::vec3 boxMin, glm::vec3 boxMax)
					   {
		// get box closest point to sphere center by clamping
		float y = std::max(boxMin.y, std::min(sphereCenter.y, boxMax.y));
		float z = std::max(boxMin.z, std::min(sphereCenter.z, boxMax.z));

		float distance = std::sqrt(
					(y - sphereCenter.y) * (y - sphereCenter.y) +
					(z - sphereCenter.z) * (z - sphereCenter.z)
				);

		std::pair<bool, glm::vec3> solution(distance <= sphereRadius,
											glm::vec3(0.0f, y, z));

		return solution; });

	// Bounding box of the object
	glm::vec3 boxMin = glm::vec3(0.0f, -object_size.y, -object_size.z);
	glm::vec3 boxMax = glm::vec3(0.0f, object_size.y, object_size.z);

	std::pair<bool, glm::vec3> intersection = intersects(playerCenter, playerSize.y, boxMin, boxMax);

	// If the collision box is only a trigger, do not change the position/speed of the player
	if (isTrigger)
	{
		return intersection.first;
	}

	if (intersection.first)
	{
		// Inspiration taken from https://www.toptal.com/game/video-game-physics-part-ii-collision-detection-for-solid-objects
		glm::vec3 closestPoint = intersection.second;

		// Transform clamped point to world space - this is the actual collision point
		glm::vec3 closestPointWorld = object->rotation * closestPoint + object_pos;

		// Calculate vector from collision point to player center
		// This is the correct direction for the collision normal
		glm::vec3 collisionToPlayer = playerPos - closestPointWorld;
		float distance = glm::length(collisionToPlayer);

		// Calculate penetration and collision normal
		float penetration = playerSize.y - distance;

		// The collision normal points from the closest point toward the player center
		// If distance is too small, we're at the center and need a fallback
		glm::vec3 actualNormal;
		if (distance > 0.0001f)
		{
			actualNormal = glm::normalize(collisionToPlayer);
		}
		else
		{
			// Fallback: use the direction from box center to player
			glm::vec3 boxCenterWorld = object_pos;
			boxCenterWorld.x = 0.0f;
			glm::vec3 centerToPlayer = playerPos - boxCenterWorld;
			actualNormal = (glm::length(centerToPlayer) > 0.0001f)
							   ? glm::normalize(centerToPlayer)
							   : glm::vec3(0.0f, 0.0f, 1.0f); // Default upward if all else fails
		}

		// Move player along the actual collision normal
		// Apply the advanced sliding resolution only if there is penetration
		if (penetration > 0.0001f)
		{
			playerPos += actualNormal * penetration;
		}

		// Cancel velocity component along the collision normal (world space)
		float velocityAlongNormal = glm::dot(speed, actualNormal);

		if (velocityAlongNormal < 0.0f)
		{ // Only if moving into the surface
			speed -= velocityAlongNormal * actualNormal;
			speed.x = 0.0f;
		}

		// Only set platform if the normal is pointing upward (player landed on top)
		// Check if the world normal has a significant upward (positive Z) component
		if (actualNormal.z > 0.7f)
		{ // cos(45°) ≈ 0.707, so steeper than 45° upward
			platform = object;
		}
		else
		{
			platform = nullptr;
		}

		return true;
	}

	return false;
}

void Player::playerJump(float jump_height)
{
	float jumpSpeed = (2 * jump_height) / (pow(jumpAirTime / 2.0f, 2.0f)) * (jumpAirTime / 2.0f);
	
	jump.pressed = false;
	jumping = true;
	speed.z = jumpSpeed;
}

void Player::update(float elapsed)
{
	// combine inputs into a move:
	if (left.pressed && !right.pressed)
		speed.y = std::max(speed.y - acceleration * elapsed, -maxSpeed);
	if (!left.pressed && right.pressed)
		speed.y = std::min(speed.y + acceleration * elapsed, maxSpeed);

	if (jump.pressed && !jumping && platform != nullptr)
	{
		playerJump(jumpHeight);
	}

	// Apply inertia to get the player down to 0 speed.
	if ((!left.pressed && !right.pressed) || (left.pressed && speed.y > 0) || (right.pressed && speed.y < 0))
	{
		speed.y -= speed.y * elapsed * 10;
	}

	if (platform == nullptr)
		speed.z -= gravity * elapsed;

	float rotation_angle = speed.y * elapsed;

	// y-axis is the forward/backward direction and the x-axis is the right/left direction
	playerCollision->position += speed.y * glm::vec3(0.0f, 1.0f, 0.0f) * elapsed + speed.z * glm::vec3(0.0f, 0.0f, 1.0f) * elapsed;

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
			float sin_arg = (r * 0.25f + wave_acc) * (2.0f * M_PI);
			float h = std::sin(sin_arg);
			float wave_amplitude = 0.00f; // Adjust this value to change the wave height

			float dh_dr = 0.25f * 2.0f * M_PI * std::cos(sin_arg);
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