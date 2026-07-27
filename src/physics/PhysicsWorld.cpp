#include "physics/PhysicsWorld.h"

#include <algorithm>

PhysicsWorld::PhysicsWorld() = default;

void PhysicsWorld::setColliders(std::vector<AABB> colliders)
{
    m_colliders = std::move(colliders);
}

void PhysicsWorld::setPlayerExtents(const glm::vec3& halfExtents)
{
    m_halfExtents = halfExtents;
}

void PhysicsWorld::jump()
{
    if (m_grounded)
    {
        m_velocity.y = m_jumpSpeed;
        m_grounded = false;
    }
}

void PhysicsWorld::update(
    glm::vec3& position,
    const glm::vec3& moveInput,
    const float deltaTime)
{
    // Horizontal movement is direct (not accelerated), matching the
    // existing WASD feel; only vertical motion is governed by gravity.
    m_velocity.x = moveInput.x * m_moveSpeed;
    m_velocity.z = moveInput.z * m_moveSpeed;

    m_velocity.y += m_gravity * deltaTime;

    position += m_velocity * deltaTime;

    resolveCollisions(position);
}

void PhysicsWorld::resolveCollisions(glm::vec3& position)
{
    m_grounded = false;

    // Resolve each overlapping collider by pushing the player out along
    // whichever axis has the *smallest* penetration depth (minimum
    // translation vector). This correctly distinguishes "mostly overlapping
    // vertically" (floor/ceiling) from "mostly overlapping horizontally"
    // (wall), which is essential at room corners where two wall colliders'
    // footprints meet -- resolving axes independently (as a naive X-then-Y-
    // then-Z pass does) can otherwise snap the player onto a wall's top
    // surface instead of stopping them at its side.
    for (const AABB& collider : m_colliders)
    {
        const AABB playerBox = AABB::fromCenterHalfExtents(position, m_halfExtents);

        if (!playerBox.intersects(collider))
        {
            continue;
        }

        const float overlapX = std::min(playerBox.max.x, collider.max.x)
            - std::max(playerBox.min.x, collider.min.x);
        const float overlapY = std::min(playerBox.max.y, collider.max.y)
            - std::max(playerBox.min.y, collider.min.y);
        const float overlapZ = std::min(playerBox.max.z, collider.max.z)
            - std::max(playerBox.min.z, collider.min.z);

        const glm::vec3 colliderCenter = (collider.min + collider.max) * 0.5F;
        const glm::vec3 centerDelta = position - colliderCenter;

        if (overlapX <= overlapY && overlapX <= overlapZ)
        {
            position.x += (centerDelta.x > 0.0F) ? overlapX : -overlapX;
            m_velocity.x = 0.0F;
        }
        else if (overlapY <= overlapX && overlapY <= overlapZ)
        {
            if (centerDelta.y > 0.0F)
            {
                // Player is above the collider's center: pushed up onto it.
                position.y += overlapY;

                if (m_velocity.y < 0.0F)
                {
                    m_velocity.y = 0.0F;
                }

                m_grounded = true;
            }
            else
            {
                // Player is below the collider's center: hit their head.
                position.y -= overlapY;

                if (m_velocity.y > 0.0F)
                {
                    m_velocity.y = 0.0F;
                }
            }
        }
        else
        {
            position.z += (centerDelta.z > 0.0F) ? overlapZ : -overlapZ;
            m_velocity.z = 0.0F;
        }
    }
}