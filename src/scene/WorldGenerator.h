#pragma once

#include "physics/AABB.h"

#include <glm/glm.hpp>
#include <vector>

enum class WorldSurface
{
    Floor,
    Wall,
    Ceiling
};

struct WorldCube
{
    glm::vec3 position;
    glm::vec3 scale;
    WorldSurface surface;
};

// Builds the static architectural geometry and collision representation of
// the map. Rendering and gameplay ownership remain in Scene/Application.
class WorldGenerator
{
public:
    static constexpr float DOORWAY_HALF_WIDTH = 0.5F;
    static constexpr float DOORWAY_HEIGHT = 2.0F;
    static constexpr float WALL_THICKNESS = 0.1F;
    static constexpr float FLOOR_TOP_Y = -0.45F;

    WorldGenerator(std::vector<WorldCube>& cubes, std::vector<AABB>& colliders)
        : m_cubes(cubes), m_colliders(colliders) {}

    void build();

private:
    std::vector<WorldCube>& m_cubes;
    std::vector<AABB>& m_colliders;
};
