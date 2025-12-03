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
    apply_physics(elapsed, center_pos, melt_factor, gravity);
    update_metaballs(elapsed, center_pos, gravity,melt_factor, on_platform, collision_platforms, grates);
}

void SoftBody::apply_physics(float elapsed, const glm::vec3& center_pos, float melt_factor, float gravity) {
    // melt_factor ranges from 0.0 (not melted) to 1.0 (fully melted)
    // The cheese_spread value from the original code (used for radial expansion)
    constexpr float CHEESE_SPREAD = 1.0f; 

    for (auto &mp : mass_points) {
        mp.acceleration = glm::vec3(0.0f); // Reset acceleration

        // 1. Force from Gravity (Applied only along the Z-axis)
        // Note: Gravity here is for the internal deforma tion, not for the Character's movement.
        // It helps the cheese droop internally.
        mp.acceleration.z -= (gravity / MASS) * 0.0f; // Apply reduced gravity force

        // 2. Force from Center Spring (Radial Spring Simulation)
        
        // mp.position is relative to the character center. center_pos is the world center.
        // The spring calculation is done in the character's local/rotated frame (relative to center_pos).
        
        glm::vec3 to_center_dir = -mp.position; // Vector from mass point *to* the center (0,0,0)
        float current_dist = glm::length(to_center_dir);
        
        // Calculate the target resting distance for the spring.
        // As melt_factor increases, the target distance increases, causing the spring to push outwards.
        // (1.0 + melt_factor * CHEESE_SPREAD) expands the cheese radius.
        float target_dist = glm::length(mp.initial_position) * (1.0f + (melt_factor) * CHEESE_SPREAD);

        // Hooke's Law: F_spring = -k * (x - L) * direction
        float displacement = current_dist - target_dist;
        
        // Normalize only if current_dist is non-zero
        glm::vec3 direction = current_dist > 0.001f ? glm::normalize(to_center_dir) : glm::vec3(0.0f);
        
        // The force should push *out* if the current distance is *less* than the target (displacement < 0).
        // The force should pull *in* if the current distance is *greater* than the target (displacement > 0).
        glm::vec3 spring_force = SPRING_K * displacement * direction;
        mp.acceleration += spring_force / MASS;

        // 3. Force from Damping (to stabilize the system)
        glm::vec3 damping_force = -DAMPING * mp.velocity;
        mp.acceleration += damping_force / MASS;

        // 4. Integration (Euler or Verlet for stability, here using simple Euler)
        mp.velocity += mp.acceleration * elapsed;
        mp.position += mp.velocity * elapsed;
    }
}

void SoftBody::update_metaballs(float elapsed, const glm::vec3& center_pos, float melt_factor, float gravity, bool on_platform, 
                              std::vector<Scene::Transform*>& collision_platforms,
                               std::vector<Scene::Transform*>& grates) {
    
    // --- Spawning Logic ---
    metaball_timer += elapsed;

    // Spawn when melted and on ground (or based on speed/melt level)
    if (on_platform && mass_points.size() > 0) {
        // Condition: melt_factor is implicitly used by checking if timer > 0.
        // You may want to add an explicit melt_factor check here (e.g., melt_factor > 0.5f)
        if (metaball_timer >= METABALL_SPAWN_INTERVAL) {
            Metaball new_ball;
            
            // // Spawn near the bottom/side, relative to the cheese center
            // float radius_y = mass_points[0].position.y; // Estimate max radius
            float radius_z = mass_points[0].position.z; // Estimate max height

            new_ball.position = center_pos + glm::vec3(
                (float)rand() / RAND_MAX * 0.5f - 0.25f, // Random X offset
                (float)rand() / RAND_MAX * 0.5f - 0.25f, // Random Y offset
                -radius_z // Spawn low, near the base
            );
            new_ball.radius = 0.2f; 
            metaballs.push_back(new_ball);
            metaball_timer = 0.0f;
        }
    }

    // --- Physics and Culling Logic ---
    metaballs.erase(
        std::remove_if(metaballs.begin(), metaballs.end(),
            [&center_pos, gravity, elapsed](Metaball& ball) {
                // Apply simple gravity
                ball.position.z -= gravity * elapsed; 

                // Check for removal condition (too far from player's center)
                if (glm::distance(ball.position, center_pos) > METABALL_MAX_DISTANCE) {
                    // std::cout << "Culling metaball: too far.\n"; // DEBUG
                    return true; // Mark for removal
                }
                return false; // Keep the metaball
            }),
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

// void draw_cheese_metaballs(SoftBody const &soft,
//                            Scene::Drawable::Pipeline const &pipeline,
//                            glm::mat4 const &object_from_world) {

// 	glUseProgram(metaball_program->program);

// 	// Set standard pipeline uniforms (MVP, time, EYE)
// 	metaball_program->set_uniforms(pipeline);

// 	// 1. Upload metaballs:
// 	std::array<glm::vec4, MetaballProgram::MaxMetaballs> packed;
// 	int count = std::min<int>(soft.metaballs.size(), MetaballProgram::MaxMetaballs);

// 	for (int i = 0; i < count; ++i) {
// 		const auto &mb = soft.metaballs[i];

// 		// If mb.position is already in object space, just use it:
// 		glm::vec3 posOS = mb.position;

// 		// otherwise, if mb.position is world-space:
// 		// glm::vec4 pOS4 = object_from_world * glm::vec4(mb.position, 1.0f);
// 		// glm::vec3 posOS = glm::vec3(pOS4);

// 		packed[i] = glm::vec4(posOS, mb.radius);
// 	}

// 	glUniform1i(metaball_program->METABALL_COUNT_int, count);
// 	if (count > 0) {
// 		glUniform4fv(metaball_program->METABALLS_vec4, count, glm::value_ptr(packed[0]));
// 	}

// 	// 2. Make sure pipeline.EYE is eye in *object space*:
// 	//    (e.g., pipeline.EYE = object_from_world * vec4(eye_world,1)).xyz
// 	//    You’ll set pipeline.EYE before calling set_uniforms.

// 	// 3. Bind VAO for your bounding cube (object space [-R,R]^3) and draw it:
// 	// glBindVertexArray(cube_vao);
// 	// glDrawElements(...);

// 	glUseProgram(0);
// }