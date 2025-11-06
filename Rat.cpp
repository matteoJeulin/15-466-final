#include "Rat.hpp"
#include "PlayMode.hpp"
#include "Player.hpp"

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
    if (std::abs(deltaPos) < aggroRange)
    {
        // Direction the rat should move in
        float dir = copysign(1.0f, deltaPos);

        speed.y = std::max(speed.y/2.5f + dir * acceleration * elapsed, dir * maxSpeed / 2.5f);

        // // Jump if close to player
        // if (deltaPos < jumpRange && playerPos.z - ratPos.z > 2 * height) 
        //     charJump(jumpHeight);

    } else {
        // Slow down to a stop
        speed.y -= speed.y * elapsed * 10;
    }

    applySpeed(elapsed);

    // Resolve collisions with the player
    if (!noclip)
    {
        platform = nullptr;

        // for (Scene::Transform *bouncy : game->bouncy_weak_platforms)
        // {
        //     if (collide(bouncy, true))
        //     {
        //         charJump(4.0f * height, jumpAirTime, gravity);
        //     }
        // }

        // for (Scene::Transform *bouncy : game->bouncy_strong_platforms)
        // {
        //     if (collide(bouncy, true))
        //     {
        //         charJump(8.5f * height, jumpAirTime, gravity);
        //     }
        // }

        for (Scene::Transform *plat : game->collision_platforms)
        {
            if (collide(plat, false))
            {
                jumping = false;
            }
        }
    }
}