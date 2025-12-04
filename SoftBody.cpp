#include "SoftBody.hpp"
#include <iostream>


// --- SoftBody Methods ---

void SoftBody::init(std::vector<DynamicMeshBuffer::Vertex> & initial_vertices) {
    mass_points.clear();
    // Initialize mass points from the initial mesh vertices
    for (const auto& initial_vertex : initial_vertices) {
        mass_points.push_back({
            initial_vertex.Position, // initial_position (relative to cheese center, unrotated)
            initial_vertex.Position  // position (starting same as initial)
        });
    }
    std::cout << "[SoftBody] Initialized " << mass_points.size() << " mass points.\n";
}

void SoftBody::update(float elapsed, const glm::vec3& center_pos, float melt_factor, float gravity, bool on_platform,std::vector<Scene::Transform*>& collision_platforms,
                      std::vector<Scene::Transform*>& grates) {
    apply_physics(elapsed, melt_factor, gravity);

}

void SoftBody::apply_physics(float elapsed,
    float melt_factor,  float gravity) {

}

void SoftBody::update_metaballs(float elapsed, const glm::vec3& center_pos, float melt_factor, float gravity, bool on_platform, 
                              std::vector<Scene::Transform*>& collision_platforms,
                               std::vector<Scene::Transform*>& grates,   float collision_min,  float collision_max ) {
    
    // --- Spawning Logic ---
    metaball_timer += elapsed;
    constexpr int MAX_ACTIVE_METABALLS = 20;



        // Only spawn if we haven't hit our cap:
        if (metaballs.size() < MAX_ACTIVE_METABALLS &&
            metaball_timer >= METABALL_SPAWN_INTERVAL) {

            Metaball new_ball;

            // Estimate cheese "height" from mass points:
            float radius_z = (collision_max -collision_min)+0.4f;

            // Ring radius around cheese center (tweak to taste):
            float ring_radius = 4.2f; // world-space radius of the circle

            // Place metaballs evenly spaced around a circle in the X–Y plane.
            // Use current count as the slot index.
            int idx = static_cast<int>(metaballs.size());
            float angle_step =6.28f/ static_cast<float>(MAX_ACTIVE_METABALLS);
            float angle = angle_step * static_cast<float>(idx);

            float ox = std::cos(angle) * ring_radius;
            float oy = std::sin(angle) * ring_radius;

            new_ball.position = center_pos + glm::vec3(
                ox,
                oy,
                -radius_z // start near the base of the cheese
            );

            new_ball.radius = float(std::abs(0.01f*std::cos(elapsed)))+ 0.05f;  // tweak for blob size

            new_ball.render_position = new_ball.position;
            new_ball.velocity = glm::vec3(0.0f);
       
            metaballs.push_back(new_ball);

            metaball_timer = 0.0f;
    }

    // --- Physics + Culling ---
         
    metaballs.erase(
       std::remove_if(
            metaballs.begin(), metaballs.end(),
            [&center_pos, gravity, elapsed](Metaball &ball) {
                // simple gravity on z:
                ball.position.z -= gravity * elapsed;


                // --- smoothing / lerp for rendering ---
                // how quickly render_position catches up to position
                const float smooth_speed = 15.0f; // try 8–20 and tweak
                float alpha = 1.0f - std::exp(-smooth_speed * elapsed);
                // clamp alpha in [0,1] just in case
                alpha = glm::clamp(alpha, 0.0f, 1.0f);

                glm::vec3 desired_offset = ball.position - center_pos;
                glm::vec3 spring_target  = center_pos + desired_offset * 0.8f; 
                // 0.8f pulls it slightly toward center → stretchy look

                ball.render_position = glm::mix(ball.render_position, spring_target, alpha);
                // cull if too far away:
                //  ball.radius-= 0.02f* elapsed;
                if (glm::distance(ball.position, center_pos) > METABALL_MAX_DISTANCE) {
                    return true;
                }
             
                if (ball.radius <= 0.0f) return true;
                return false;
            }
        ),
        metaballs.end()
    );
    
}

 const std::vector<glm::vec3> SoftBody::get_mass_point_positions() const {
    // 1. Initialize a new vector to hold only the position data.
    std::vector<glm::vec3> positions;
    
    // Reserve memory upfront for efficiency to avoid reallocations
    positions.reserve(mass_points.size());

    // 2. Iterate through all mass points and extract the position.
    for (const auto& mp : mass_points) {
        positions.push_back(mp.position);
    }

    // 3. Return the new vector.
    // The 'const' ensures the SoftBody state isn't modified.
    return positions;
}

void SoftBody::applyWallClingDeform(glm::vec3 &p) const {
    if (!wallClingActive) return;

    // Normalize height: 0 at bottom, 1 at top
    float z01 = 0.0f;
    if (height_range > 0.0f) {
        z01 = glm::clamp((p.z - cheese_base) / height_range, 0.0f, 1.0f);
    }
    float tail = 1.0f - z01; // 1 at bottom, 0 at top


    // --- 2) Shape towards / off the wall along y using wallClingDir ---
    // wallClingDir is the direction *toward* the wall (±1 along y)
    float towardWall = wallClingDir;

    // Work in coordinates “along wall normal”
    float localY = p.y * towardWall; // positive = toward wall

    // How thin the blob is right against the wall (like squished contact patch)
    const float WALL_THICKNESS = 0.001f;   // tweak
    // How far the bottom droplet sticks out away from the wall
    const float TAIL_BULGE = 0.3f;       // tweak

    // Top: glued flat to the wall
    float y_on_wall = WALL_THICKNESS;

    // Bottom: slightly bulging off the wall
    float y_tail = localY + TAIL_BULGE;

    // Interpolate between “stuck” at the top and “bulge” at the bottom.
    // tail = 0 at top, 1 at bottom → nice teardrop
    float y_final = glm::mix(y_on_wall, y_tail, tail);

    // Convert back to signed y with correct side
    p.y = y_final * towardWall;
}