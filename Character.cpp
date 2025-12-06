#include "Character.hpp"
#include "PlayMode.hpp"
#include "RayCast.hpp"
#include "BlobShadowPipeline.hpp"
#include "DynamicMeshBuffer.hpp"
#include "LitColorTextureProgram.hpp"


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
            if (collision->name.compare("Player") == 0)
                std::cout << collision->name <<  " moving into surface " << object->name << "... actual normal = (" << actualNormal.x << ", " << actualNormal.y << ", " << actualNormal.z << ")\n";
            speed -= velocityAlongNormal * actualNormal;
            speed.x = 0.0f;
        }

        // Only set platform if the normal is pointing upward (player landed on top)
        // Check if the world normal has a significant upward (positive Z) component
        if (actualNormal.z > 0.7f)
        { // cos(45°) ≈ 0.707, so steeper than 45° upward
            platform = object;
            jumping = false;
        }
        else
        {
            platform = nullptr;
        }

        if (game->current_level > 0 && actualNormal.z == 0.0f) {
            // cos(90°) = cos(270°) = 0
            wall = object;
        }

        // if (collision == game->player->collision) {
        //     std::cout << "Player is touching " << object->name << "\n";
        // }

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
    if (platform == nullptr) {
        speed.z -= gravity * elapsed;
    }

    // y-axis is the forward/backward direction and the x-axis is the right/left direction
    collision->position += speed.y * glm::vec3(0.0f, 1.0f, 0.0f) * elapsed + speed.z * glm::vec3(0.0f, 0.0f, 1.0f) * elapsed;
}


GLuint Character::createBlobShadow(){

    // Static storage for blob mesh:
    DynamicMeshBuffer &blob_mesh = this->game->blob_mesh;
    GLuint blob_vao = 0;

    // Make VAO for the *lit_color_texture_program* 
    blob_vao = blob_mesh.make_vao_for_program(blob_shadow_program->program);
    std::cout << "Created blob shadow VAO: " << blob_vao << std::endl;
    //2. Create a transform for the shadow:
    this->game->scene.transforms.emplace_back();
    shadow_form = &this->game->scene.transforms.back();
    shadow_form->name = "BlobShadow_" + collision->name;
    std::cout << "shadow name" << shadow_form << std::endl;

    // initial placement; will be overwritten each frame by applyBlobShadow:
    shadow_form->position = collision->position;
    shadow_form->rotation = glm::quat();
    shadow_form->scale    = glm::vec3(2.0f, 2.0f, 1.0f); // blob radius in world units
    std::cout << "start drawable" << std::endl;
    // 3. Create a drawable attached to that transform:
    this->game->scene.drawables.emplace_back(shadow_form);
    Scene::Drawable &drawable = this->game->scene.drawables.back();
    drawable.pipeline[2] = blob_shadow_pipeline;
    drawable.pipeline[2].vao   = blob_vao;
    drawable.pipeline[2].type  = GL_TRIANGLES;
    drawable.pipeline[2].start = 0;
    drawable.pipeline[2].count = 6;

    // set blob color to a dark translucent “shadow”:
    drawable.pipeline[2].set_uniforms = []() {
        glUniform4f(blob_shadow_program->BLOB_COLOR_vec4,
                    1.0f, 1.0f, 1.0f, 0.5f); // black, 50% alpha
    };
    shadow_valid = false;
    return blob_vao;
}
void Character::applyBlobShadow()
{
    if (!shadow_form) {
        shadow_vao = createBlobShadow();
    }

    glm::vec3 origin = collision->position+glm::vec3(0.0f, 0.0f, 1.0f);
    
    glm::vec3 dir = glm::vec3(0.0f, 0.0f, -1.0f);

    Ray r;
    r.origin = origin;
    r.dir = dir;

    float best_t = std::numeric_limits<float>::max();
    bool found = false;
    Scene::Transform *hit_transform = nullptr;
    glm::vec3 hit_pos;

    //loop through all collison objects and check what objects we are hitting: 
    auto test_transform = [&](Scene::Transform *t) {
            if (!t) return;
            float tval;
            glm::vec3 hit;
            if (ray_vs_rotated_platform_box(r, t, &tval, &hit) && tval < best_t) {
                best_t = tval;
                found = true;
                hit_transform = t;
            }
        };

    // test against collision platforms + grates + plates etc:
    for (auto *t : this->game->collision_platforms) test_transform(t);
    for (auto *t : this->game->bouncy_weak_platforms) test_transform(t);
    for (auto *t : this->game->bouncy_strong_platforms) test_transform(t);
    for (auto *t : this->game->collision_plates) test_transform(t);
    for (auto *t : this->game->grates) test_transform(t);

    if (found && shadow_form) {

        // local normal on the mesh:
        glm::vec3 local_normal(0.0f, 0.0f, 1.0f);
        glm::mat4x3 world_from_local = hit_transform->make_world_from_local();
        glm::mat3 M = glm::mat3(world_from_local);
        glm::mat3 normal_matrix = glm::transpose(glm::inverse(M));
        glm::vec3 world_normal = glm::normalize(normal_matrix * local_normal);

        hit_pos = origin + best_t * dir;

        // assume ground normal is roughly +Z (0,0,1) for now
        glm::vec3 normal = world_normal;

        // place the shadow just above the ground:
        shadow_form->position = hit_pos + normal * 0.01f;
        // orient so its local +Z points along the normal:
        shadow_form->rotation = -glm::rotation(glm::vec3(0,0,1), world_normal);
        // scale blob size:
        shadow_form->scale = glm::vec3(5.0f * (2.2/std::sqrt(best_t)), 5.0f *(2.2/std::sqrt(best_t)), 5.0f);

        shadow_valid = true;
        // std::cout << hit_transform->name <<" position:" << hit_pos.x <<","<< hit_pos.y <<","<< hit_pos.z <<" with normals "<< shadow_form->position.x <<","<< shadow_form->position.y <<","<< shadow_form->position.z << std::endl;
        // std::cout << "distance:" << best_t << std::endl;
    } else if (shadow_form) {
        shadow_valid = false;
    }
}


