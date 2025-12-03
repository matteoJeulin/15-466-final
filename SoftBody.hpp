#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <numeric>
#include <algorithm>
#include "Scene.hpp"
#include "DynamicMeshBuffer.hpp"
#include "MetaballProgram.hpp"


// --- Soft Body Constants ---
// Define these constants centrally. You can adjust these values for desired cheese elasticity.
constexpr float SPRING_K = 100.0f; // Spring constant (stiffness)
constexpr float DAMPING = 5.0f;    // Damping factor (slows oscillation)
constexpr float MASS = 1.0f;       // Mass of a single vertex/mass point
constexpr float METABALL_SPAWN_INTERVAL = 0.1f;
constexpr float METABALL_MAX_DISTANCE = 5.0f;
constexpr float INTERNAL_GRAVITY_MULTIPLIER = 0.5f; 
constexpr float RADIAL_MELT_REDUCTION = 0.2f;
// --- Soft Body Data Structures ---

/**
 * @brief Represents a single mass point in the soft-body simulation (a vertex of the cheese mesh).
 */
struct SoftBodyMassPoint {
    glm::vec3 initial_position; // Position of vertex in its unmelted, unrotated, resting state.
    glm::vec3 position;         // Current position relative to the character's center.
    glm::vec3 velocity = glm::vec3(0.0f);
    glm::vec3 acceleration = glm::vec3(0.0f);
    // You might add neighbor indices here for a more complex volumetric spring system.
};



/**
 * @brief Represents a single metaball for the dripping cheese effect.
 */
struct Metaball {
    glm::vec3 position; // World position of the metaball center.
    float radius;
};

/**
 * @brief Manages the state and logic for the soft-body cheese physics.
 */
class SoftBody {
public:
    std::vector<SoftBodyMassPoint> mass_points;
    std::vector<Metaball> metaballs;
    float metaball_timer = 0.0f;


 

    SoftBody() = default;

    /**
     * @brief Initializes the mass points from the Character's initial vertex data.
     * @param initial_vertices The starting positions of the mesh vertices.
     */
    void init(std::vector<DynamicMeshBuffer::Vertex>& initial_vertices);

    /**
     * @brief Performs one step of the soft-body simulation and metaball logic.
     * @param elapsed Time since last update.
     * @param center_pos The world position of the character's center (e.g., collision->position).
     * @param melt_factor A normalized value (0.0 to 1.0) indicating how melted the cheese is.
     * @param gravity The character's gravity constant.
     * @param on_platform Whether the player is currently standing on a platform.
     */
       // New signature includes the lists of relevant Scene::Transforms
    void update(float elapsed, const glm::vec3& center_pos, float melt_factor, float gravity, bool on_platform, 
                 std::vector<Scene::Transform*>& collision_platforms,
                 std::vector<Scene::Transform*>& grates);

    /**
     * @brief Applies physics forces and updates mass point positions.
     */
    void apply_physics(float elapsed, float melt_factor, float gravity);

    /**
     * @brief Spawns, updates, and culls metaballs.
     */
    void update_metaballs(float elapsed, const glm::vec3& center_pos, float melt_factor, float gravity, bool on_platform, 
                               std::vector<Scene::Transform*>& collision_platforms,
                               std::vector<Scene::Transform*>& grates);

    const std::vector<glm::vec3> get_mass_point_positions() const;
};