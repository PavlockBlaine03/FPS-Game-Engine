#include "scene/Scene.h"
#include "util/MeshFactory.h"

#include <glm/gtc/matrix_transform.hpp>

namespace
{
    constexpr const char* FLOOR_TEXTURE_PATH = "assets/textures/flooring.png";
    constexpr const char* WALL_TEXTURE_PATH = "assets/textures/wallpaper1.png";
    constexpr const char* CEILING_TEXTURE_PATH = "assets/textures/ceiling.png";
}

Scene::Scene()
    : m_floorTexture(std::make_unique<Texture>(FLOOR_TEXTURE_PATH))
    , m_wallTexture(std::make_unique<Texture>(WALL_TEXTURE_PATH))
    , m_ceilingTexture(std::make_unique<Texture>(CEILING_TEXTURE_PATH))
{
    // Tile the texture a few times across each 1x1 cube face rather than
    // stretching a single copy across it, so wood planks/wallpaper repeat
    // at a reasonable scale.
    const MeshData cubeData = MeshFactory::createCube(1.0F, 2.0F);

    m_cubeMesh = std::make_unique<Mesh>(
        cubeData.vertices.data(),
        cubeData.vertices.size(),
        cubeData.indices.data(),
        cubeData.indices.size(),
        8 // position(3) + normal(3) + uv(2)
    );

    buildEnvironment();
}

Scene::~Scene() = default;

void Scene::buildEnvironment()
{
    constexpr int ROOM_SIZE = 8; // enlarged from 5 to make room for a divided layout
    constexpr float WALL_HEIGHT = 3.0F;

    // Ceiling: a grid of flat, wide cubes acting as ceiling tiles.
    for (int x = -ROOM_SIZE; x <= ROOM_SIZE; ++x)
    {
        for (int z = -ROOM_SIZE; z <= ROOM_SIZE; ++z)
        {
            const glm::vec3 position(static_cast<float>(x), 2.5F, static_cast<float>(z));
            const glm::vec3 scale(1.0F, 0.1F, 1.0F);

            m_cubes.push_back(CubeInstance{ position, scale, SurfaceType::Ceiling });
            m_colliders.push_back(AABB::fromCenterHalfExtents(position, scale * 0.5F));
        }
    }

    // Floor: a grid of flat, wide cubes acting as floor tiles.
    for (int x = -ROOM_SIZE; x <= ROOM_SIZE; ++x)
    {
        for (int z = -ROOM_SIZE; z <= ROOM_SIZE; ++z)
        {
            const glm::vec3 position(static_cast<float>(x), -0.5F, static_cast<float>(z));
            const glm::vec3 scale(1.0F, 0.1F, 1.0F);

            m_cubes.push_back(CubeInstance{ position, scale, SurfaceType::Floor });
            m_colliders.push_back(AABB::fromCenterHalfExtents(position, scale * 0.5F));
        }
    }

    // Perimeter walls: stacked vertically along the outer edges of the room.
    // Walls running along X (front/back, facing +/-Z) are thin in Z;
    // walls running along Z (left/right, facing +/-X) are thin in X.
    for (int x = -ROOM_SIZE; x <= ROOM_SIZE; ++x)
    {
        for (float y = 0.0F; y < WALL_HEIGHT; y += 1.0F)
        {
            const glm::vec3 scale(1.0F, 1.0F, WALL_THICKNESS);

            const glm::vec3 posNegZ(static_cast<float>(x), y, static_cast<float>(-ROOM_SIZE));
            m_cubes.push_back(CubeInstance{ posNegZ, scale, SurfaceType::Wall });
            m_colliders.push_back(AABB::fromCenterHalfExtents(posNegZ, scale * 0.5F));

            const glm::vec3 posPosZ(static_cast<float>(x), y, static_cast<float>(ROOM_SIZE));
            m_cubes.push_back(CubeInstance{ posPosZ, scale, SurfaceType::Wall });
            m_colliders.push_back(AABB::fromCenterHalfExtents(posPosZ, scale * 0.5F));
        }
    }

    for (int z = -ROOM_SIZE; z <= ROOM_SIZE; ++z)
    {
        for (float y = 0.0F; y < WALL_HEIGHT; y += 1.0F)
        {
            const glm::vec3 scale(WALL_THICKNESS, 1.0F, 1.0F);

            const glm::vec3 posNegX(static_cast<float>(-ROOM_SIZE), y, static_cast<float>(z));
            m_cubes.push_back(CubeInstance{ posNegX, scale, SurfaceType::Wall });
            m_colliders.push_back(AABB::fromCenterHalfExtents(posNegX, scale * 0.5F));

            const glm::vec3 posPosX(static_cast<float>(ROOM_SIZE), y, static_cast<float>(z));
            m_cubes.push_back(CubeInstance{ posPosX, scale, SurfaceType::Wall });
            m_colliders.push_back(AABB::fromCenterHalfExtents(posPosX, scale * 0.5F));
        }
    }

    // Dividing wall: splits the room into two halves along Z = 0, with a
    // doorway gap left open in the middle (no cubes placed for |x| within
    // DOORWAY_HALF_WIDTH up to DOORWAY_HEIGHT), so the player can walk
    // between both halves. Thin in Z, matching the perimeter walls it runs
    // parallel to. The gap itself is filled by a separate Door entity
    // (see Application), not by Scene.
    for (int x = -ROOM_SIZE; x <= ROOM_SIZE; ++x)
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

            m_cubes.push_back(CubeInstance{ position, scale, SurfaceType::Wall });
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

        m_cubes.push_back(CubeInstance{ position, scale, SurfaceType::Wall });
        m_colliders.push_back(AABB::fromCenterHalfExtents(position, scale * 0.5F));
    }
}

void Scene::render(
    const Renderer& renderer,
    const Shader& shader,
    const Camera& camera,
    const Light& light) const
{
    for (const CubeInstance& cube : m_cubes)
    {
        glm::mat4 model(1.0F);
        model = glm::translate(model, cube.position);
        model = glm::scale(model, cube.scale);

       const Texture& texture = 
           (cube.surface == SurfaceType::Floor) ? *m_floorTexture : 
           (cube.surface == SurfaceType::Wall)  ? *m_wallTexture :
                                                  *m_ceilingTexture;

        renderer.draw(*m_cubeMesh, shader, camera, model, texture, light);
    }
}