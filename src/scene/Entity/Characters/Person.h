#pragma once

#include "physics/AABB.h"
#include "scene/Animation/Animator.h"
#include "scene/Entity/Entity.h"

#include <glm/glm.hpp>

#include <memory>
#include <vector>

class SkeletalModel;
class RigidBodyWorld;

class Person : public Entity
{
public:
    Person(
        std::shared_ptr<SkeletalModel> model,
        const glm::vec3& firstPatrolPoint,
        const glm::vec3& secondPatrolPoint,
        float scale = 1.0F);

    void update(float deltaTime, RigidBodyWorld& rigidBodies);

    void render(
        const Renderer& renderer,
        const Shader& shader,
        const Camera& camera,
        const Light& light) const override;

    [[nodiscard]] AABB collider() const;
    [[nodiscard]] const glm::vec3& position() const { return m_position; }
    [[nodiscard]] bool isRagdoll() const { return m_ragdoll; }
    void applyShot(const glm::vec3& hitPoint, const glm::vec3& direction,
        RigidBodyWorld& rigidBodies);

private:
    enum class PatrolState { Walking, Waiting };

    std::shared_ptr<SkeletalModel> m_model;
    Animator m_animator;
    glm::vec3 m_patrolPoints[2];
    glm::vec3 m_position;
    float m_yawRadians = 0.0F;
    float m_scale = 1.0F;
    float m_moveSpeed = 1.25F;
    float m_waitRemaining = 0.0F;
    int m_targetPoint = 1;
    PatrolState m_state = PatrolState::Walking;
    bool m_ragdoll = false;
    std::size_t m_ragdollIndex = static_cast<std::size_t>(-1);
    std::vector<int> m_ragdollNodes;
    std::vector<glm::mat4> m_ragdollLocalPose;
    std::vector<glm::mat4> m_ragdollGlobalPose;
    std::vector<glm::mat4> m_ragdollSkinMatrices;
};
