#include "Rat.hpp"
#include "PlayMode.hpp"
#include "Player.hpp"
#include "AudioManager.hpp"
#include <algorithm>
#include <random>

namespace {
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> squeak_chance(0.0f, 1.0f);
    std::uniform_real_distribution<float> squeak_interval(0.4f, 5.0f);
}

Rat::Rat(PlayMode *_game) : Character(_game)
{
}

void Rat::update(float elapsed)
{
    glm::vec3 playerPos = game->player->collision->position;
    glm::vec3 ratPos = collision->position;

    // Signed distance between the rat and the player
    float deltaPos = playerPos.y - ratPos.y;

    // Only move if in range
    if (!game->player->chomped) {
        if (glm::distance(playerPos, ratPos) < aggroRange)
        {
            // Direction the rat should move in
            float dir = copysign(1.0f, deltaPos);

            speed.y += dir * acceleration * elapsed;
            speed.y = std::clamp(speed.y, -maxSpeed, maxSpeed);

            // // Jump if close to player
            // if (deltaPos < jumpRange && playerPos.z - ratPos.z > 2 * height) 
            //     charJump(jumpHeight);

        } else {
            // Slow down to a stop
            speed.y -= speed.y * elapsed * 10;
        }
    }
    else {
        // move away
        // Direction the rat should move in
        float dir = copysign(1.0f, -deltaPos);

        speed.y += dir * acceleration * elapsed;
        speed.y = std::clamp(speed.y, -maxSpeed, maxSpeed);

        // // Jump if close to player
        // if (deltaPos < jumpRange && playerPos.z - ratPos.z > 2 * height) 
        //     charJump(jumpHeight);
    }

    squeak_timer -= elapsed;
    float horizontal_speed = std::abs(speed.y);
    bool is_moving = horizontal_speed > 0.1f;

    if (is_moving && squeak_timer <= 0.0f) {
        if (squeak_chance(rng) < 0.35f) {
            AudioManager::play_event(AudioManager::Event::MouseSqueak);
        }
        squeak_timer = squeak_interval(rng);
    }

    applySpeed(elapsed);

    // Resolve collisions with the player
    if (!noclip)
    {
        platform = nullptr;

        for (Scene::Transform *plate : game->collision_plates)
		{
			collide(plate, false);
		}

        for (Scene::Transform *grate : game->grates)
		{
			// Go throught the grate if melted enough
			collide(grate, false);
		}

        for (Scene::Transform *bouncy : game->bouncy_weak_platforms)
		{
			collide(bouncy, false);
		}

		for (Scene::Transform *bouncy : game->bouncy_strong_platforms)
		{
			collide(bouncy, false);
		}

        for (Scene::Transform *plat : game->collision_platforms)
        {
            if (collide(plat, false))
            {
                jumping = false;
            }
        }
    }
}