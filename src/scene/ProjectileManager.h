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

class ProjectileManager
{
public:
    ProjectileManager();
    ~ProjectileManager();

    ProjectileManager(const ProjectileManager&) = delete;
    ProjectileManager& operator=(const ProjectileManager&) = delete;

    void spawn(const glm::vec3& origin, const glm::vec3& direction);

    // colliders: world geometry to test bullets against; particles: used to
    // spawn an impact burst wherever a bullet hits something.
    void update(
        float deltaTime,
        const std::vector<AABB>& colliders,
        ParticleSystem& particles);

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