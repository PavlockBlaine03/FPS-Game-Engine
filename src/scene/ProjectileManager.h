#pragma once

#include "physics/AABB.h"
#include "rendering/Camera.h"
#include "rendering/Light.h"
#include "rendering/Mesh.h"
#include "rendering/Renderer.h"
#include "rendering/Shader.h"
#include "rendering/Texture.h"
#include "scene/Entity/Objects/Bullet.h"

#include <memory>
#include <vector>

class ParticleSystem;
class RigidBodyWorld;
class Person;

class ProjectileManager
{
public:
    ProjectileManager();
    ~ProjectileManager();

    ProjectileManager(const ProjectileManager&) = delete;
    ProjectileManager& operator=(const ProjectileManager&) = delete;

    void spawnAimed(
        const glm::vec3& origin,
        const glm::vec3& cameraPosition,
        const glm::vec3& cameraDirection,
        const std::vector<AABB>& colliders);

    // colliders: world geometry to test bullets against; particles: used to
    // spawn an impact burst wherever a bullet hits something.
    void update(
        float deltaTime,
        const std::vector<AABB>& colliders,
        const std::vector<std::unique_ptr<Person>>& persons,
        ParticleSystem& particles,
        RigidBodyWorld& rigidBodies);

    void render(
        const Renderer& renderer,
        const Shader& shader,
        const Camera& camera,
        const Light& light) const;

private:
    std::vector<Bullet> m_bullets;

    std::unique_ptr<Mesh> m_bulletMesh;
    std::unique_ptr<Texture> m_bulletTexture;
};
