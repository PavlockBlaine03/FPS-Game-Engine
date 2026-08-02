#include "scene/WorldGenerator.h"

void WorldGenerator::build()
{
    constexpr int MAP_HALF_WIDTH = 12;
    constexpr int MAP_HALF_DEPTH = 12;
    constexpr float WALL_HEIGHT = 3.0F;

    // Ceiling: a grid of flat, wide cubes acting as ceiling tiles.
    for (int x = -MAP_HALF_WIDTH; x <= MAP_HALF_WIDTH; ++x)
    {
        for (int z = -MAP_HALF_DEPTH; z <= MAP_HALF_DEPTH; ++z)
        {
            // Temporary roof-access opening above the central north room.
            if (x >= -2 && x <= 2 && z >= 5 && z <= 11) continue;
            const glm::vec3 position(static_cast<float>(x), 2.5F, static_cast<float>(z));
            const glm::vec3 scale(1.0F, 0.1F, 1.0F);

            m_cubes.push_back(WorldCube{ position, scale, WorldSurface::Ceiling });
        }
    }
    // Four slabs leave a full U-shaped stairwell opening at x [-2.5, 2.5],
    // z [4.5, 11.5] instead of sealing it with one broad ceiling collider.
    m_colliders.push_back(AABB::fromCenterHalfExtents(
        glm::vec3(-7.5F, 2.5F, 0.0F), glm::vec3(5.0F, 0.05F, 12.5F)));
    m_colliders.push_back(AABB::fromCenterHalfExtents(
        glm::vec3(7.5F, 2.5F, 0.0F), glm::vec3(5.0F, 0.05F, 12.5F)));
    m_colliders.push_back(AABB::fromCenterHalfExtents(
        glm::vec3(0.0F, 2.5F, -4.0F), glm::vec3(2.5F, 0.05F, 8.5F)));
    m_colliders.push_back(AABB::fromCenterHalfExtents(
        glm::vec3(0.0F, 2.5F, 12.0F), glm::vec3(2.5F, 0.05F, 0.5F)));

    // Floor: a grid of flat, wide cubes acting as floor tiles.
    for (int x = -MAP_HALF_WIDTH; x <= MAP_HALF_WIDTH; ++x)
    {
        for (int z = -MAP_HALF_DEPTH; z <= MAP_HALF_DEPTH; ++z)
        {
            const glm::vec3 position(static_cast<float>(x), -0.5F, static_cast<float>(z));
            const glm::vec3 scale(1.0F, 0.1F, 1.0F);

            m_cubes.push_back(WorldCube{ position, scale, WorldSurface::Floor });
        }
    }
    m_colliders.push_back(AABB::fromCenterHalfExtents(
        glm::vec3(0.0F, -0.5F, 0.0F),
        glm::vec3(static_cast<float>(MAP_HALF_WIDTH) + 0.5F, 0.05F,
            static_cast<float>(MAP_HALF_DEPTH) + 0.5F)));

    // Perimeter walls: stacked vertically along the outer edges of the room.
    // Walls running along X (front/back, facing +/-Z) are thin in Z;
    // walls running along Z (left/right, facing +/-X) are thin in X.
    for (int x = -MAP_HALF_WIDTH; x <= MAP_HALF_WIDTH; ++x)
    {
        for (float y = 0.0F; y < WALL_HEIGHT; y += 1.0F)
        {
            const glm::vec3 scale(1.0F, 1.0F, WALL_THICKNESS);

            const glm::vec3 posNegZ(static_cast<float>(x), y, static_cast<float>(-MAP_HALF_DEPTH));
            m_cubes.push_back(WorldCube{ posNegZ, scale, WorldSurface::Wall });
            m_colliders.push_back(AABB::fromCenterHalfExtents(posNegZ, scale * 0.5F));

            const glm::vec3 posPosZ(static_cast<float>(x), y, static_cast<float>(MAP_HALF_DEPTH));
            m_cubes.push_back(WorldCube{ posPosZ, scale, WorldSurface::Wall });
            m_colliders.push_back(AABB::fromCenterHalfExtents(posPosZ, scale * 0.5F));
        }
    }

    for (int z = -MAP_HALF_DEPTH; z <= MAP_HALF_DEPTH; ++z)
    {
        for (float y = 0.0F; y < WALL_HEIGHT; y += 1.0F)
        {
            const glm::vec3 scale(WALL_THICKNESS, 1.0F, 1.0F);

            const glm::vec3 posNegX(static_cast<float>(-MAP_HALF_WIDTH), y, static_cast<float>(z));
            m_cubes.push_back(WorldCube{ posNegX, scale, WorldSurface::Wall });
            m_colliders.push_back(AABB::fromCenterHalfExtents(posNegX, scale * 0.5F));

            const glm::vec3 posPosX(static_cast<float>(MAP_HALF_WIDTH), y, static_cast<float>(z));
            m_cubes.push_back(WorldCube{ posPosX, scale, WorldSurface::Wall });
            m_colliders.push_back(AABB::fromCenterHalfExtents(posPosX, scale * 0.5F));
        }
    }

    // Dividing wall: splits the room into two halves along Z = 0, with a
    // doorway gap left open in the middle (no cubes placed for |x| within
    // DOORWAY_HALF_WIDTH up to DOORWAY_HEIGHT), so the player can walk
    // between both halves. Thin in Z, matching the perimeter walls it runs
    // parallel to. The gap itself is filled by a separate Door entity
    // (see Application), not by Scene.
    for (int x = -MAP_HALF_WIDTH; x <= MAP_HALF_WIDTH; ++x)
    {
        const bool isDoorwayColumn =
            (static_cast<float>(x) >= -DOORWAY_HALF_WIDTH && static_cast<float>(x) <= DOORWAY_HALF_WIDTH);

        for (float y = 0.0F; y < WALL_HEIGHT; y += 1.0F)
        {
            if (isDoorwayColumn && (y - 0.5F) < DOORWAY_HEIGHT)
            {
                continue; // leave the doorway opening clear below the header
            }

            const glm::vec3 scale(1.0F, 1.0F, WALL_THICKNESS);
            const glm::vec3 position(static_cast<float>(x), y, 0.0F);

            m_cubes.push_back(WorldCube{ position, scale, WorldSurface::Wall });
            m_colliders.push_back(AABB::fromCenterHalfExtents(position, scale * 0.5F));
        }
    }

    // Doorway header: a lintel spanning the opening, filling the gap
    // between the clear doorway height and the ceiling so the opening
    // doesn't look like a hole straight through to the ceiling tiles.
    // Matches the dividing wall's thin Z depth.
    {
        const float headerHeight = WALL_HEIGHT - DOORWAY_HEIGHT;
        const float headerCenterY = DOORWAY_HEIGHT + headerHeight * 0.5F;
        const float headerWidth = DOORWAY_HALF_WIDTH * 2.0F + 1.0F;

        const glm::vec3 position(0.0F, headerCenterY, 0.0F);
        const glm::vec3 scale(headerWidth, headerHeight, WALL_THICKNESS);

        m_cubes.push_back(WorldCube{ position, scale, WorldSurface::Wall });
        m_colliders.push_back(AABB::fromCenterHalfExtents(position, scale * 0.5F));
    }

    // Two north/south partition walls turn the expanded footprint into six
    // connected rooms. Each half has its own open passage, creating a looped
    // layout instead of a row of dead ends. These openings intentionally have
    // no doors yet; the central doorway remains the interactive entrance.
    constexpr float SIDE_WALL_X = 4.0F;
    constexpr float SIDE_DOOR_Z = 6.0F;
    constexpr float SIDE_DOOR_HALF_WIDTH = 0.6F;
    for (const float wallX : { -SIDE_WALL_X, SIDE_WALL_X })
    {
        for (int z = -MAP_HALF_DEPTH; z <= MAP_HALF_DEPTH; ++z)
        {
            const float zPosition = static_cast<float>(z);
            const bool inNorthOpening = std::abs(zPosition - SIDE_DOOR_Z) <= SIDE_DOOR_HALF_WIDTH;
            const bool inSouthOpening = std::abs(zPosition + SIDE_DOOR_Z) <= SIDE_DOOR_HALF_WIDTH;
            for (float y = 0.0F; y < WALL_HEIGHT; y += 1.0F)
            {
                if ((inNorthOpening || inSouthOpening) && (y - 0.5F) < DOORWAY_HEIGHT)
                    continue;

                const glm::vec3 position(wallX, y, zPosition);
                const glm::vec3 scale(WALL_THICKNESS, 1.0F, 1.0F);
                m_cubes.push_back(WorldCube{ position, scale, WorldSurface::Wall });
                m_colliders.push_back(AABB::fromCenterHalfExtents(position, scale * 0.5F));
            }
        }

        // Headers visually finish the four open passages.
        for (const float doorwayZ : { -SIDE_DOOR_Z, SIDE_DOOR_Z })
        {
            const float headerHeight = WALL_HEIGHT - DOORWAY_HEIGHT;
            const glm::vec3 position(
                wallX, DOORWAY_HEIGHT + headerHeight * 0.5F, doorwayZ);
            const glm::vec3 scale(WALL_THICKNESS, headerHeight, 1.0F);
            m_cubes.push_back(WorldCube{ position, scale, WorldSurface::Wall });
            m_colliders.push_back(AABB::fromCenterHalfExtents(position, scale * 0.5F));
        }
    }
}
