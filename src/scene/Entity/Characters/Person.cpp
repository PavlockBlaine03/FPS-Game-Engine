#include "scene/Entity/Characters/Person.h"

#include "rendering/SkeletalModel.h"
#include "physics/RigidBodyWorld.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cctype>
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

    std::string lower(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(),
            [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return value;
    }

    bool isRagdollBone(const std::string& source)
    {
        const std::string name = lower(source);
        if (name.find("twist") != std::string::npos
            || name.find("finger") != std::string::npos
            || name.find("thumb") != std::string::npos
            || name.find("toe") != std::string::npos
            || name.find("end") != std::string::npos)
            return false;
        constexpr const char* NAMES[] = {
            "hips", "pelvis", "spine", "chest", "neck", "head",
            "arm", "forearm", "hand", "upleg", "thigh", "calf", "shin", "leg", "foot"
        };
        for (const char* candidate : NAMES)
            if (name.find(candidate) != std::string::npos) return true;
        return false;
    }

    glm::quat matrixRotation(const glm::mat4& matrix)
    {
        glm::mat3 rotation(matrix);
        for (int column = 0; column < 3; ++column)
            rotation[column] = glm::normalize(rotation[column]);
        return glm::normalize(glm::quat_cast(rotation));
    }

    float ragdollMass(const std::string& source)
    {
        const std::string name = lower(source);
        if (name.find("hip") != std::string::npos || name.find("pelvis") != std::string::npos)
            return 10.0F;
        if (name.find("spine") != std::string::npos || name.find("chest") != std::string::npos)
            return 7.0F;
        if (name.find("head") != std::string::npos || name.find("neck") != std::string::npos)
            return 4.0F;
        if (name.find("upleg") != std::string::npos || name.find("thigh") != std::string::npos)
            return 5.5F;
        if (name.find("calf") != std::string::npos || name.find("shin") != std::string::npos)
            return 3.5F;
        if (name.find("upperarm") != std::string::npos)
            return 2.8F;
        if (name.find("forearm") != std::string::npos)
            return 1.8F;
        if (name.find("hand") != std::string::npos || name.find("foot") != std::string::npos)
            return 1.2F;
        return 2.5F;
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

void Person::update(const float deltaTime, RigidBodyWorld& rigidBodies)
{
    const float safeDeltaTime = std::clamp(deltaTime, 0.0F, 0.1F);

    if (m_ragdoll)
    {
        std::vector<glm::mat4> physicsBones;
        if (!rigidBodies.ragdollPose(m_ragdollIndex, physicsBones)) return;
        glm::mat4 characterTransform = glm::translate(glm::mat4(1.0F), m_position);
        characterTransform = glm::rotate(characterTransform, m_yawRadians, glm::vec3(0.0F, 1.0F, 0.0F));
        characterTransform = glm::scale(characterTransform, glm::vec3(m_scale));
        const glm::mat4 inverseCharacter = glm::inverse(characterTransform);
        const auto& nodes = m_model->nodes();
        std::vector<int> physicsPartByNode(nodes.size(), -1);
        for (std::size_t i = 0; i < m_ragdollNodes.size(); ++i)
            physicsPartByNode[static_cast<std::size_t>(m_ragdollNodes[i])] = static_cast<int>(i);
        for (std::size_t i = 0; i < nodes.size(); ++i)
        {
            const int part = physicsPartByNode[i];
            if (part >= 0)
                m_ragdollGlobalPose[i] = inverseCharacter * physicsBones[static_cast<std::size_t>(part)];
            else
                m_ragdollGlobalPose[i] = nodes[i].parent >= 0
                    ? m_ragdollGlobalPose[static_cast<std::size_t>(nodes[i].parent)] * m_ragdollLocalPose[i]
                    : m_ragdollLocalPose[i];
        }
        const auto& bones = m_model->bones();
        for (std::size_t i = 0; i < bones.size(); ++i)
            m_ragdollSkinMatrices[i] = m_model->inverseRoot()
                * m_ragdollGlobalPose[static_cast<std::size_t>(bones[i].nodeIndex)]
                * bones[i].inverseBind;
        return;
    }

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
    m_model->render(renderer, shader, camera, transform, light,
        m_ragdoll ? m_ragdollSkinMatrices : m_animator.skinMatrices());
}

void Person::applyShot(const glm::vec3& hitPoint, const glm::vec3& direction,
    RigidBodyWorld& rigidBodies)
{
    if (!m_ragdoll)
    {
        const auto& nodes = m_model->nodes();
        const auto& animatedGlobal = m_animator.globalPose();
        glm::mat4 characterTransform = glm::translate(glm::mat4(1.0F), m_position);
        characterTransform = glm::rotate(characterTransform, m_yawRadians, glm::vec3(0.0F, 1.0F, 0.0F));
        characterTransform = glm::scale(characterTransform, glm::vec3(m_scale));

        std::vector<int> partByNode(nodes.size(), -1);
        std::vector<RigidBodyWorld::RagdollPart> parts;
        for (const SkeletonBone& bone : m_model->bones())
        {
            if (bone.nodeIndex < 0 || !isRagdollBone(bone.name)) continue;
            const std::size_t node = static_cast<std::size_t>(bone.nodeIndex);
            if (partByNode[node] >= 0) continue;
            partByNode[node] = static_cast<int>(parts.size());
            RigidBodyWorld::RagdollPart part;
            part.nodeIndex = bone.nodeIndex;
            const glm::mat4 boneWorld = characterTransform * animatedGlobal[node];
            part.jointPosition = glm::vec3(boneWorld[3]);
            part.endPosition = part.jointPosition;
            part.boneRotation = matrixRotation(boneWorld);
            part.boneScale = glm::vec3(
                glm::length(glm::vec3(boneWorld[0])),
                glm::length(glm::vec3(boneWorld[1])),
                glm::length(glm::vec3(boneWorld[2])));
            part.mass = ragdollMass(bone.name);
            parts.push_back(part);
            m_ragdollNodes.push_back(bone.nodeIndex);
        }

        std::vector<std::vector<int>> children(parts.size());
        for (std::size_t i = 0; i < parts.size(); ++i)
        {
            int parentNode = nodes[static_cast<std::size_t>(parts[i].nodeIndex)].parent;
            while (parentNode >= 0 && partByNode[static_cast<std::size_t>(parentNode)] < 0)
                parentNode = nodes[static_cast<std::size_t>(parentNode)].parent;
            if (parentNode >= 0)
            {
                parts[i].parentPart = partByNode[static_cast<std::size_t>(parentNode)];
                children[static_cast<std::size_t>(parts[i].parentPart)].push_back(static_cast<int>(i));
            }
        }
        for (std::size_t i = 0; i < parts.size(); ++i)
        {
            if (!children[i].empty())
            {
                int chosen = children[i].front();
                const std::string parentName = lower(nodes[static_cast<std::size_t>(parts[i].nodeIndex)].name);
                for (const int child : children[i])
                {
                    const std::string childName = lower(nodes[static_cast<std::size_t>(parts[static_cast<std::size_t>(child)].nodeIndex)].name);
                    if ((parentName.find("hip") != std::string::npos && childName.find("spine") != std::string::npos)
                        || (parentName.find("spine") != std::string::npos && (childName.find("spine") != std::string::npos || childName.find("neck") != std::string::npos)))
                        chosen = child;
                }
                parts[i].endPosition = parts[static_cast<std::size_t>(chosen)].jointPosition;
            }
            else
                parts[i].endPosition = parts[i].jointPosition
                    + parts[i].boneRotation * glm::vec3(0.0F, 0.12F * m_scale, 0.0F);
            const float length = glm::length(parts[i].endPosition - parts[i].jointPosition);
            parts[i].radius = std::clamp(length * 0.22F, 0.035F * m_scale, 0.13F * m_scale);
        }

        m_ragdollLocalPose.resize(nodes.size());
        m_ragdollGlobalPose = animatedGlobal;
        for (std::size_t i = 0; i < nodes.size(); ++i)
            m_ragdollLocalPose[i] = nodes[i].parent >= 0
                ? glm::inverse(animatedGlobal[static_cast<std::size_t>(nodes[i].parent)]) * animatedGlobal[i]
                : animatedGlobal[i];
        m_ragdollSkinMatrices = m_animator.skinMatrices();
        m_ragdollIndex = rigidBodies.createRagdoll(parts);
        m_ragdoll = true;
    }
    rigidBodies.applyRagdollShot(m_ragdollIndex, hitPoint, direction);
}

AABB Person::collider() const
{
    const glm::vec3 halfExtents(0.3F * m_scale, 0.9F * m_scale, 0.3F * m_scale);
    return AABB::fromCenterHalfExtents(
        m_position + glm::vec3(0.0F, halfExtents.y, 0.0F), halfExtents);
}
