#include "scene/Scene.h"
#include "scene/WorldGenerator.h"
#include "util/MeshFactory.h"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

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

    WorldGenerator(m_cubes, m_colliders).build();
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
}
