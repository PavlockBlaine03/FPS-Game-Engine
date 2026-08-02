#include "scene/Scene.h"
#include "scene/WorldGenerator.h"
#include "util/MeshFactory.h"
#include "rendering/Model.h"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace
{
    constexpr const char* FLOOR_TEXTURE_PATH = "assets/textures/flooring.png";
    constexpr const char* WALL_TEXTURE_PATH = "assets/textures/wallpaper1.png";
    constexpr const char* CEILING_TEXTURE_PATH = "assets/textures/ceiling.png";
    constexpr const char* STAIR_MODEL_PATH = "assets/models/Environment/Stairs/scala.fbx";
    constexpr int STAIR_COUNT = 16;
    constexpr float STAIR_WIDTH = 1.6F;
    constexpr float STAIR_LENGTH = 4.0F;
    constexpr float STAIR_HEIGHT = 2.95F;
    // The FBX bounds include railing posts above the upper landing. Scale
    // the visible model taller so the landing, not the post tops, meets the
    // roof while retaining the proven collision dimensions below.
    constexpr float STAIR_MODEL_HEIGHT = 3.65F;
    constexpr glm::vec3 STAIR_CENTER(0.0F, 1.375F, 9.5F);
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

    WorldGenerator(m_cubes, m_colliders).build();

    m_stairModel = std::make_unique<Model>(STAIR_MODEL_PATH);
    const AABB bounds = m_stairModel->localBounds();
    const glm::vec3 size = bounds.max - bounds.min;
    const glm::vec3 localCenter = (bounds.min + bounds.max) * 0.5F;
    const bool longAlongX = size.x > size.z;
    const glm::vec3 modelScale = longAlongX
        ? glm::vec3(
            STAIR_LENGTH / std::max(size.x, 0.0001F),
            STAIR_MODEL_HEIGHT / std::max(size.y, 0.0001F),
            STAIR_WIDTH / std::max(size.z, 0.0001F))
        : glm::vec3(
            STAIR_WIDTH / std::max(size.x, 0.0001F),
            STAIR_MODEL_HEIGHT / std::max(size.y, 0.0001F),
            STAIR_LENGTH / std::max(size.z, 0.0001F));
    m_stairTransform = glm::translate(glm::mat4(1.0F), STAIR_CENTER);
    if (longAlongX)
        m_stairTransform = glm::rotate(m_stairTransform,
            glm::radians(90.0F), glm::vec3(0.0F, 1.0F, 0.0F));
    m_stairTransform = glm::scale(m_stairTransform, modelScale);
    m_stairTransform = glm::translate(m_stairTransform, -localCenter);

    const float treadDepth = STAIR_LENGTH / static_cast<float>(STAIR_COUNT);
    const float rise = STAIR_HEIGHT / static_cast<float>(STAIR_COUNT);
    const float startZ = STAIR_CENTER.z - STAIR_LENGTH * 0.5F;
    for (int step = 0; step < STAIR_COUNT; ++step)
    {
        const float top = FLOOR_TOP_Y + rise * static_cast<float>(step + 1);
        const float height = top - FLOOR_TOP_Y;
        const glm::vec3 center(
            STAIR_CENTER.x,
            FLOOR_TOP_Y + height * 0.5F,
            startZ + treadDepth * (static_cast<float>(step) + 0.5F));
        m_colliders.push_back(AABB::fromCenterHalfExtents(
            center, glm::vec3(STAIR_WIDTH * 0.5F, height * 0.5F, treadDepth * 0.5F)));
    }
}

Scene::~Scene() = default;

void Scene::render(
    const Renderer& renderer,
    const Shader& shader,
    const Camera& camera,
    const Light& light) const
{
    for (const WorldCube& cube : m_cubes)
    {
        glm::mat4 model(1.0F);
        model = glm::translate(model, cube.position);
        model = glm::scale(model, cube.scale);

       const Texture& texture = 
           (cube.surface == WorldSurface::Floor) ? *m_floorTexture :
           (cube.surface == WorldSurface::Wall)  ? *m_wallTexture :
                                                  *m_ceilingTexture;

        renderer.draw(*m_cubeMesh, shader, camera, model, texture, light);
    }
    if (m_stairModel != nullptr)
        m_stairModel->render(renderer, shader, camera, m_stairTransform, light);
}
