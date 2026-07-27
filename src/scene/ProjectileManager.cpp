#include "scene/ProjectileManager.h"
#include "scene/ParticleSystem.h"
#include "util/MeshFactory.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace
{
    constexpr glm::vec3 BULLET_COLOR(1.0F, 1.0F, 1.0F);
    constexpr glm::vec3 BULLET_HALF_EXTENTS(0.02F, 0.02F, 0.08F); // thin, elongated along travel direction
}

ProjectileManager::ProjectileManager()
    : m_bulletTexture(std::make_unique<Texture>(BULLET_COLOR))
{
    const MeshData cubeData = MeshFactory::createCube(1.0F, 1.0F);

    m_bulletMesh = std::make_unique<Mesh>(
        cubeData.vertices.data(),
        cubeData.vertices.size(),
        cubeData.indices.data(),
        cubeData.indices.size(),
        8 // position(3) + normal(3) + uv(2)
    );
}

ProjectileManager::~ProjectileManager() = default;

void ProjectileManager::spawn(const glm::vec3& origin, const glm::vec3& direction)
{
    Bullet bullet;
    bullet.position = origin;
    bullet.direction = glm::normalize(direction);

    m_bullets.push_back(bullet);
}

void ProjectileManager::update(
    const float deltaTime,
    const std::vector<AABB>& colliders,
    ParticleSystem& particles)
{
    for (Bullet& bullet : m_bullets)
    {
        if (!bullet.alive)
        {
            continue;
        }

        const float step = bullet.speed * deltaTime;
        const glm::vec3 previousPosition = bullet.position;
        const glm::vec3 nextPosition = previousPosition + bullet.direction * step;

        // Build a small AABB around the bullet's swept segment rather than
        // just testing the endpoint -- fast bullets can otherwise tunnel
        // through thin walls within a single frame if only the final point
        // is checked.
        const glm::vec3 segmentMin = glm::min(previousPosition, nextPosition) - glm::vec3(0.05F);
        const glm::vec3 segmentMax = glm::max(previousPosition, nextPosition) + glm::vec3(0.05F);
        const AABB sweptBox{ segmentMin, segmentMax };

        bool hit = false;

        for (const AABB& collider : colliders)
        {
            if (sweptBox.intersects(collider))
            {
                hit = true;
                break;
            }
        }

        bullet.distanceTraveled += step;

        if (hit)
        {
            particles.spawnBurst(nextPosition, -bullet.direction);
            bullet.alive = false;
        }
        else if (bullet.distanceTraveled >= bullet.maxDistance)
        {
            bullet.alive = false;
        }
        else
        {
            bullet.position = nextPosition;
        }
    }

    // Remove dead projectiles so the list doesn't grow unbounded.
    m_bullets.erase(
        std::remove_if(
            m_bullets.begin(),
            m_bullets.end(),
            [](const Bullet& projectile) { return !projectile.alive; }),
        m_bullets.end());
}

void ProjectileManager::render(
    const Renderer& renderer,
    const Shader& shader,
    const Camera& camera,
    const Light& light) const
{
    for (const Bullet& projectile : m_bullets)
    {
        // Orient the bullet mesh so its elongated (Z) axis points along its
        // direction of travel.
        const glm::vec3 up = (std::abs(projectile.direction.y) > 0.99F)
            ? glm::vec3(1.0F, 0.0F, 0.0F)
            : glm::vec3(0.0F, 1.0F, 0.0F);

        const glm::mat4 lookRotation = glm::inverse(glm::lookAt(
            glm::vec3(0.0F),
            -projectile.direction,
            up));

        glm::mat4 model = glm::translate(glm::mat4(1.0F), projectile.position);
        model = model * lookRotation;
        model = glm::scale(model, BULLET_HALF_EXTENTS * 2.0F);

        renderer.draw(*m_bulletMesh, shader, camera, model, *m_bulletTexture, light);
    }
}