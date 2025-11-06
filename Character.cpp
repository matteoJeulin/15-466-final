#include "Character.hpp"
#include "PlayMode.hpp"

Character::Character(PlayMode *_game) : game(_game)
{
}

void Character::update(float elapsed)
{
    // Default implementation - does nothing
    // Derived classes should override this
}

bool Character::collide(Scene::Transform *object, bool isTrigger)
{
    glm::vec3 object_pos = glm::vec3(object->position);
    object_pos.x = 0.0f;
    glm::vec3 object_size = object->scale;

    glm::vec3 &charPos = collision->position;
    charPos.x = 0.0f;
    glm::vec3 charSize = collision->scale;

    // Transform player position from world space to object's local space
    glm::quat invRotation = glm::inverse(object->rotation);
    glm::vec3 playerCenter = invRotation * (charPos - object_pos);
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

    std::pair<bool, glm::vec3> intersection = intersects(playerCenter, charSize.y, boxMin, boxMax);

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
        glm::vec3 collisionToPlayer = charPos - closestPointWorld;
        float distance = glm::length(collisionToPlayer);

        // Calculate penetration and collision normal
        float penetration = charSize.y - distance;

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
            glm::vec3 centerToPlayer = charPos - boxCenterWorld;
            actualNormal = (glm::length(centerToPlayer) > 0.0001f)
                               ? glm::normalize(centerToPlayer)
                               : glm::vec3(0.0f, 0.0f, 1.0f); // Default upward if all else fails
        }

        // Move player along the actual collision normal
        // Apply the advanced sliding resolution only if there is penetration
        if (penetration > 0.0001f)
        {
            charPos += actualNormal * penetration;
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

void Character::charJump(float char_jump_height, float jump_time, float jump_grav) // "jump_time" is up and down
{
    // float jumpSpeed = (2 * jump_height) / (pow(jumpAirTime / 2.0f, 2.0f)) * (jumpAirTime / 2.0f);
    float jumpSpeed = (char_jump_height + (0.5f * jump_grav * pow((jump_time / 2.0f), 2.0f))) / (jump_time / 2.0f); 

    jumping = true;
    speed.z = jumpSpeed;
}

void Character::applySpeed(float elapsed)
{
    if (platform == nullptr)
        speed.z -= gravity * elapsed;

    // y-axis is the forward/backward direction and the x-axis is the right/left direction
    collision->position += speed.y * glm::vec3(0.0f, 1.0f, 0.0f) * elapsed + speed.z * glm::vec3(0.0f, 0.0f, 1.0f) * elapsed;
}