#include "scene/Entity/Characters/Person.h"

#include "rendering/SkeletalModel.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace
{
    const SkeletalModel& requireModel(const std::shared_ptr<SkeletalModel>& model)
    {
        if (model == nullptr)
        {
            throw std::invalid_argument("Person requires a skeletal model");
        }
        return *model;
    }
}

Person::Person(
    std::shared_ptr<SkeletalModel> model,
    const glm::vec3& firstPatrolPoint,
    const glm::vec3& secondPatrolPoint,
    const float scale)
    : m_model(std::move(model))
    , m_animator(requireModel(m_model))
    , m_patrolPoints{ firstPatrolPoint, secondPatrolPoint }
    , m_position(firstPatrolPoint)
    , m_scale(scale)
{
    if (!m_animator.play("Idle", true, 0.0F))
    {
        throw std::runtime_error("Person model requires an Idle animation clip");
    }
    if (m_model->findClip("Walk") == nullptr)
    {
        throw std::runtime_error("Person model requires a Walk animation clip");
    }
    m_animator.play("Walk");
}

void Person::update(const float deltaTime)
{
    const float safeDeltaTime = std::clamp(deltaTime, 0.0F, 0.1F);

    if (m_state == PatrolState::Waiting)
    {
        m_waitRemaining -= safeDeltaTime;
        if (m_waitRemaining <= 0.0F)
        {
            m_targetPoint = 1 - m_targetPoint;
            m_state = PatrolState::Walking;
            m_animator.play("Walk");
        }
    }
    else
    {
        glm::vec3 offset = m_patrolPoints[m_targetPoint] - m_position;
        offset.y = 0.0F;
        const float distance = glm::length(offset);

        if (distance <= 0.02F)
        {
            m_position = m_patrolPoints[m_targetPoint];
            m_state = PatrolState::Waiting;
            m_waitRemaining = 1.5F;
            m_animator.play("Idle");
        }
        else
        {
            const glm::vec3 direction = offset / distance;
            const float travel = std::min(distance, m_moveSpeed * safeDeltaTime);
            m_position += direction * travel;
            m_yawRadians = std::atan2(direction.x, direction.z);
        }
    }

    m_animator.update(safeDeltaTime);
}

void Person::render(
    const Renderer& renderer,
    const Shader& shader,
    const Camera& camera,
    const Light& light) const
{
    glm::mat4 transform = glm::translate(glm::mat4(1.0F), m_position);
    transform = glm::rotate(transform, m_yawRadians, glm::vec3(0.0F, 1.0F, 0.0F));
    transform = glm::scale(transform, glm::vec3(m_scale));
    m_model->render(renderer, shader, camera, transform, light, m_animator.skinMatrices());
}

AABB Person::collider() const
{
    const glm::vec3 halfExtents(0.3F * m_scale, 0.9F * m_scale, 0.3F * m_scale);
    return AABB::fromCenterHalfExtents(
        m_position + glm::vec3(0.0F, halfExtents.y, 0.0F), halfExtents);
}
