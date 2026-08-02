#include "scene/ProjectileManager.h"
#include "scene/ParticleSystem.h"
#include "physics/RigidBodyWorld.h"
#include "util/MeshFactory.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    constexpr glm::vec3 BULLET_COLOR(1.0F, 1.0F, 1.0F);
    constexpr glm::vec3 BULLET_HALF_EXTENTS(0.02F, 0.02F, 0.08F); // thin, elongated along travel direction

    // Return the point at which a moving bullet first enters an AABB. Testing
    // the segment itself prevents tunnelling without inflating the collision
    // area beyond the visible edges of the wall.
    bool segmentIntersectsAABB(
        const glm::vec3& start,
        const glm::vec3& end,
        const AABB& box,
        float& hitTime)
    {
        const glm::vec3 delta = end - start;
        float entryTime = 0.0F;
        float exitTime = 1.0F;

        for (int axis = 0; axis < 3; ++axis)
        {
            if (std::abs(delta[axis]) <= std::numeric_limits<float>::epsilon())
            {
                if (start[axis] < box.min[axis] || start[axis] > box.max[axis])
                {
                    return false;
                }
                continue;
            }

            const float inverseDelta = 1.0F / delta[axis];
            float axisEntry = (box.min[axis] - start[axis]) * inverseDelta;
            float axisExit = (box.max[axis] - start[axis]) * inverseDelta;
            if (axisEntry > axisExit)
            {
                std::swap(axisEntry, axisExit);
            }

            entryTime = std::max(entryTime, axisEntry);
            exitTime = std::min(exitTime, axisExit);
            if (entryTime > exitTime)
            {
                return false;
            }
        }

        hitTime = entryTime;
        return true;
    }
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

void ProjectileManager::spawnAimed(
    const glm::vec3& origin,
    const glm::vec3& cameraPosition,
    const glm::vec3& cameraDirection,
    const std::vector<AABB>& colliders)
{
    const glm::vec3 normalizedCameraDirection = glm::normalize(cameraDirection);
    const glm::vec3 cameraRayEnd = cameraPosition + normalizedCameraDirection * 50.0F;
    float earliestHitTime = 1.0F;

    // Find what the center-screen camera ray is aiming at. The visible
    // projectile can then travel from the offset muzzle to that same point.
    for (const AABB& collider : colliders)
    {
        float hitTime = 0.0F;
        if (segmentIntersectsAABB(cameraPosition, cameraRayEnd, collider, hitTime)
            && hitTime < earliestHitTime)
        {
            earliestHitTime = hitTime;
        }
    }

    const glm::vec3 aimPoint = cameraPosition
        + (cameraRayEnd - cameraPosition) * earliestHitTime;

    Bullet bullet;
    bullet.position = origin;
    bullet.direction = glm::normalize(aimPoint - origin);

    m_bullets.push_back(bullet);
}

void ProjectileManager::update(
    const float deltaTime,
    const std::vector<AABB>& colliders,
    ParticleSystem& particles,
    RigidBodyWorld& rigidBodies)
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

        bool hit = false;
        float earliestHitTime = 1.0F;
        bool hitRigidBody = false;
        std::size_t rigidBodyIndex = 0;

        for (const AABB& collider : colliders)
        {
            float hitTime = 0.0F;
            if (segmentIntersectsAABB(previousPosition, nextPosition, collider, hitTime)
                && hitTime <= earliestHitTime)
            {
                hit = true;
                earliestHitTime = hitTime;
            }
        }

        float rigidHitTime = 1.0F;
        std::size_t candidateBody = 0;
        if (rigidBodies.raycast(previousPosition, nextPosition, rigidHitTime, candidateBody)
            && rigidHitTime <= earliestHitTime)
        {
            hit = true;
            hitRigidBody = true;
            earliestHitTime = rigidHitTime;
            rigidBodyIndex = candidateBody;
        }

        bullet.distanceTraveled += step;

        if (hit)
        {
            const glm::vec3 hitPosition = previousPosition
                + (nextPosition - previousPosition) * earliestHitTime;
            particles.spawnBurst(hitPosition, -bullet.direction);
            if (hitRigidBody)
                rigidBodies.applyShot(rigidBodyIndex, hitPosition, bullet.direction);
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
        model = glm::scale(model, BULLET_HALF_EXTENTS * 0.25F);

        renderer.draw(*m_bulletMesh, shader, camera, model, *m_bulletTexture, light);
    }
}
