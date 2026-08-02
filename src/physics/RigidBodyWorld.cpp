#include "physics/RigidBodyWorld.h"

#include "rendering/Camera.h"
#include "rendering/Mesh.h"
#include "rendering/Renderer.h"
#include "rendering/Shader.h"
#include "rendering/Texture.h"
#include "util/MeshFactory.h"

#include <PxPhysicsAPI.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>

using namespace physx;

namespace
{
    constexpr float SHOT_IMPULSE = 8.5F;

    PxVec3 toPhysX(const glm::vec3& value) { return PxVec3(value.x, value.y, value.z); }
    glm::vec3 toGlm(const PxVec3& value) { return glm::vec3(value.x, value.y, value.z); }

    glm::mat4 toGlm(const PxTransform& transform)
    {
        const PxMat44 matrix(transform);
        glm::mat4 result(1.0F);
        for (int column = 0; column < 4; ++column)
            for (int row = 0; row < 4; ++row)
                result[column][row] = matrix[column][row];
        return result;
    }

    glm::quat rotationFromX(const glm::vec3& direction)
    {
        const glm::vec3 from(1.0F, 0.0F, 0.0F);
        const float cosine = glm::dot(from, direction);
        if (cosine < -0.9999F)
            return glm::angleAxis(glm::pi<float>(), glm::vec3(0.0F, 1.0F, 0.0F));
        const glm::vec3 axis = glm::cross(from, direction);
        return glm::normalize(glm::quat(1.0F + cosine, axis.x, axis.y, axis.z));
    }
}

struct RigidBodyWorld::Impl
{
    PxDefaultAllocator allocator;
    PxDefaultErrorCallback errors;
    PxFoundation* foundation = nullptr;
    PxPhysics* physics = nullptr;
    PxDefaultCpuDispatcher* dispatcher = nullptr;
    PxScene* scene = nullptr;
    PxMaterial* material = nullptr;

    std::vector<PxRigidDynamic*> dynamicBodies;
    std::vector<glm::vec3> visualScales;
    std::vector<bool> sphereVisuals;
    std::vector<bool> visualEnabled;
    std::vector<PxRigidStatic*> staticBodies;
    std::vector<AABB> cachedStaticColliders;
    PxRigidDynamic* playerBody = nullptr;
    std::vector<PxRigidDynamic*> npcBodies;

    struct Ragdoll
    {
        std::vector<PxRigidDynamic*> actors;
        std::vector<PxD6Joint*> joints;
        std::vector<glm::mat4> boneFromActor;
        std::vector<int> nodeIndices;
    };
    std::vector<Ragdoll> ragdolls;

    Impl()
    {
        foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, errors);
        if (foundation == nullptr) throw std::runtime_error("Failed to create PhysX foundation");
        physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, PxTolerancesScale(), true);
        if (physics == nullptr) throw std::runtime_error("Failed to create PhysX SDK");

        PxSceneDesc sceneDescription(physics->getTolerancesScale());
        sceneDescription.gravity = PxVec3(0.0F, -9.81F, 0.0F);
        dispatcher = PxDefaultCpuDispatcherCreate(2);
        sceneDescription.cpuDispatcher = dispatcher;
        sceneDescription.filterShader = PxDefaultSimulationFilterShader;
        scene = physics->createScene(sceneDescription);
        if (scene == nullptr) throw std::runtime_error("Failed to create PhysX scene");
        material = physics->createMaterial(0.75F, 0.75F, 0.08F);

        playerBody = physics->createRigidDynamic(PxTransform(PxIdentity));
        PxShape* shape = physics->createShape(
            PxBoxGeometry(0.3F, 0.9F, 0.3F), *material, true);
        playerBody->attachShape(*shape);
        shape->release();
        playerBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
        scene->addActor(*playerBody);
    }

    ~Impl()
    {
        for (Ragdoll& ragdoll : ragdolls)
            for (PxD6Joint* joint : ragdoll.joints) joint->release();
        if (scene != nullptr)
        {
            scene->removeActor(*playerBody);
            for (PxRigidDynamic* body : npcBodies) scene->removeActor(*body);
            for (PxRigidDynamic* body : dynamicBodies) scene->removeActor(*body);
            for (PxRigidStatic* body : staticBodies) scene->removeActor(*body);
        }
        if (playerBody != nullptr) playerBody->release();
        for (PxRigidDynamic* body : npcBodies) body->release();
        for (PxRigidDynamic* body : dynamicBodies) body->release();
        for (PxRigidStatic* body : staticBodies) body->release();
        if (material != nullptr) material->release();
        if (scene != nullptr) scene->release();
        if (dispatcher != nullptr) dispatcher->release();
        if (physics != nullptr) physics->release();
        if (foundation != nullptr) foundation->release();
    }

    void setStaticColliders(const std::vector<AABB>& colliders)
    {
        bool unchanged = colliders.size() == cachedStaticColliders.size();
        for (std::size_t i = 0; unchanged && i < colliders.size(); ++i)
            unchanged = colliders[i].min == cachedStaticColliders[i].min
                && colliders[i].max == cachedStaticColliders[i].max;
        if (unchanged) return;

        for (PxRigidStatic* body : staticBodies)
        {
            scene->removeActor(*body);
            body->release();
        }
        staticBodies.clear();
        cachedStaticColliders = colliders;
        for (const AABB& box : colliders)
        {
            const glm::vec3 halfExtents = (box.max - box.min) * 0.5F;
            const glm::vec3 center = (box.min + box.max) * 0.5F;
            PxRigidStatic* body = physics->createRigidStatic(PxTransform(toPhysX(center)));
            PxShape* shape = physics->createShape(PxBoxGeometry(toPhysX(halfExtents)), *material);
            body->attachShape(*shape);
            shape->release();
            scene->addActor(*body);
            staticBodies.push_back(body);
        }
    }

    PxRigidDynamic* createDynamic(const PxGeometry& geometry, const PxTransform& pose,
        const float mass, const glm::vec3& visualScale, const bool sphere)
    {
        PxRigidDynamic* body = physics->createRigidDynamic(pose);
        PxShape* shape = physics->createShape(geometry, *material);
        body->attachShape(*shape);
        shape->release();
        PxRigidBodyExt::setMassAndUpdateInertia(*body, mass);
        body->setLinearDamping(0.04F);
        body->setAngularDamping(0.08F);
        body->userData = reinterpret_cast<void*>(dynamicBodies.size() + 1U);
        scene->addActor(*body);
        dynamicBodies.push_back(body);
        visualScales.push_back(visualScale);
        sphereVisuals.push_back(sphere);
        visualEnabled.push_back(true);
        return body;
    }
};

RigidBodyWorld::RigidBodyWorld()
    : m_impl(std::make_unique<Impl>())
    , m_texture(std::make_unique<Texture>(glm::vec3(0.72F, 0.22F, 0.12F)))
    , m_blueTexture(std::make_unique<Texture>(glm::vec3(0.08F, 0.25F, 0.9F)))
{
    const MeshData cube = MeshFactory::createCube(1.0F, 1.0F);
    m_mesh = std::make_unique<Mesh>(cube.vertices.data(), cube.vertices.size(),
        cube.indices.data(), cube.indices.size(), 8);
    const MeshData sphere = MeshFactory::createSphere();
    m_sphereMesh = std::make_unique<Mesh>(sphere.vertices.data(), sphere.vertices.size(),
        sphere.indices.data(), sphere.indices.size(), 8);
}

RigidBodyWorld::~RigidBodyWorld() = default;

void RigidBodyWorld::spawnCubeStack(const glm::vec3& basePosition, const int requestedRows)
{
    constexpr float size = 0.45F;
    constexpr float gap = 0.025F;
    const int rows = std::clamp(requestedRows, 1, 8);
    for (int row = 0; row < rows; ++row)
    {
        const int count = rows - row;
        for (int column = 0; column < count; ++column)
        {
            glm::vec3 position(
                (static_cast<float>(column) - static_cast<float>(count - 1) * 0.5F) * (size + gap),
                basePosition.y + size * 0.5F + static_cast<float>(row) * (size + gap),
                basePosition.z);
            position.x += basePosition.x;
            m_impl->createDynamic(PxBoxGeometry(size * 0.5F, size * 0.5F, size * 0.5F),
                PxTransform(toPhysX(position)), 1.0F, glm::vec3(size), false);
        }
    }
}

void RigidBodyWorld::spawnBlueBall(const glm::vec3& position)
{
    constexpr float radius = 0.32F;
    PxRigidDynamic* body = m_impl->createDynamic(PxSphereGeometry(radius),
        PxTransform(toPhysX(position)), 0.8F, glm::vec3(radius * 2.0F), true);
    body->setLinearDamping(0.025F);
    body->setAngularDamping(0.04F);
}

void RigidBodyWorld::moveNpcColliders(const std::vector<AABB>& colliders, const float deltaTime)
{
    (void)deltaTime;
    while (m_impl->npcBodies.size() > colliders.size())
    {
        PxRigidDynamic* body = m_impl->npcBodies.back();
        m_impl->scene->removeActor(*body);
        body->release();
        m_impl->npcBodies.pop_back();
    }
    while (m_impl->npcBodies.size() < colliders.size())
    {
        PxRigidDynamic* body = m_impl->physics->createRigidDynamic(PxTransform(PxIdentity));
        body->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
        m_impl->scene->addActor(*body);
        m_impl->npcBodies.push_back(body);
    }
    for (std::size_t i = 0; i < colliders.size(); ++i)
    {
        PxRigidDynamic& body = *m_impl->npcBodies[i];
        PxShape* oldShapes[1]{};
        body.getShapes(oldShapes, 1);
        const bool enabled = colliders[i].min.x <= colliders[i].max.x
            && colliders[i].min.y <= colliders[i].max.y
            && colliders[i].min.z <= colliders[i].max.z;
        if (!enabled)
        {
            if (oldShapes[0] != nullptr)
                oldShapes[0]->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
            body.setGlobalPose(PxTransform(PxVec3(
                10000.0F + static_cast<float>(i) * 2.0F, 10000.0F, 10000.0F)));
            body.setLinearVelocity(PxVec3(0.0F));
            body.setAngularVelocity(PxVec3(0.0F));
            continue;
        }
        if (oldShapes[0] != nullptr)
            oldShapes[0]->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
        const glm::vec3 half = (colliders[i].max - colliders[i].min) * 0.5F;
        const glm::vec3 center = (colliders[i].min + colliders[i].max) * 0.5F;
        if (oldShapes[0] != nullptr) oldShapes[0]->setGeometry(PxBoxGeometry(toPhysX(half)));
        else
        {
            PxShape* shape = m_impl->physics->createShape(
                PxBoxGeometry(toPhysX(half)), *m_impl->material, true);
            body.attachShape(*shape);
            shape->release();
        }
        body.setKinematicTarget(PxTransform(toPhysX(center)));
    }
}

void RigidBodyWorld::movePlayerCollider(const glm::vec3& position,
    const glm::vec3& halfExtents, const float deltaTime)
{
    (void)deltaTime;
    PxShape* shapes[1]{};
    m_impl->playerBody->getShapes(shapes, 1);
    if (shapes[0] != nullptr) shapes[0]->setGeometry(PxBoxGeometry(toPhysX(halfExtents)));
    m_impl->playerBody->setKinematicTarget(PxTransform(toPhysX(position)));
}

void RigidBodyWorld::update(const float deltaTime, const std::vector<AABB>& staticColliders)
{
    m_impl->setStaticColliders(staticColliders);
    m_impl->scene->simulate(std::clamp(deltaTime, 0.0F, 1.0F / 15.0F));
    m_impl->scene->fetchResults(true);
}

std::vector<AABB> RigidBodyWorld::colliders() const
{
    std::vector<AABB> result;
    result.reserve(m_impl->dynamicBodies.size());
    for (const PxRigidDynamic* body : m_impl->dynamicBodies)
    {
        const PxBounds3 bounds = body->getWorldBounds();
        result.push_back(AABB{ toGlm(bounds.minimum), toGlm(bounds.maximum) });
    }
    return result;
}

bool RigidBodyWorld::raycast(const glm::vec3& start, const glm::vec3& end,
    float& hitTime, std::size_t& bodyIndex) const
{
    const glm::vec3 delta = end - start;
    const float distance = glm::length(delta);
    if (distance <= 0.00001F) return false;
    PxRaycastBuffer hit;
    PxQueryFilterData filter;
    filter.flags = PxQueryFlag::eDYNAMIC;
    if (!m_impl->scene->raycast(toPhysX(start), toPhysX(delta / distance), distance,
        hit, PxHitFlag::eDEFAULT, filter)
        || hit.block.actor == nullptr || hit.block.actor->userData == nullptr)
        return false;
    hitTime = hit.block.distance / distance;
    bodyIndex = reinterpret_cast<std::uintptr_t>(hit.block.actor->userData) - 1U;
    return bodyIndex < m_impl->dynamicBodies.size();
}

void RigidBodyWorld::applyShot(const std::size_t bodyIndex, const glm::vec3& hitPoint,
    const glm::vec3& direction)
{
    if (bodyIndex >= m_impl->dynamicBodies.size()) return;
    PxRigidDynamic& body = *m_impl->dynamicBodies[bodyIndex];
    body.wakeUp();
    PxRigidBodyExt::addForceAtPos(body, toPhysX(glm::normalize(direction) * SHOT_IMPULSE),
        toPhysX(hitPoint), PxForceMode::eIMPULSE);
}

void RigidBodyWorld::render(const Renderer& renderer, const Shader& shader,
    const Camera& camera, const Light& light) const
{
    for (std::size_t i = 0; i < m_impl->dynamicBodies.size(); ++i)
    {
        if (!m_impl->visualEnabled[i]) continue;
        glm::mat4 model = toGlm(m_impl->dynamicBodies[i]->getGlobalPose());
        model = glm::scale(model, m_impl->visualScales[i]);
        const Mesh& mesh = m_impl->sphereVisuals[i] ? *m_sphereMesh : *m_mesh;
        const Texture& texture = m_impl->sphereVisuals[i] ? *m_blueTexture : *m_texture;
        renderer.draw(mesh, shader, camera, model, texture, light);
    }
}

std::size_t RigidBodyWorld::createRagdoll(const std::vector<RagdollPart>& parts)
{
    Impl::Ragdoll ragdoll;
    ragdoll.actors.reserve(parts.size());
    ragdoll.boneFromActor.reserve(parts.size());
    ragdoll.nodeIndices.reserve(parts.size());

    for (const RagdollPart& part : parts)
    {
        glm::vec3 direction = part.endPosition - part.jointPosition;
        float length = glm::length(direction);
        if (length < part.radius * 2.0F)
        {
            direction = part.boneRotation * glm::vec3(1.0F, 0.0F, 0.0F);
            length = part.radius * 2.0F;
        }
        else direction /= length;

        const glm::quat actorRotation = rotationFromX(direction);
        const glm::vec3 actorPosition = (part.jointPosition + part.endPosition) * 0.5F;
        const PxTransform actorPose(toPhysX(actorPosition),
            PxQuat(actorRotation.x, actorRotation.y, actorRotation.z, actorRotation.w));
        const float halfHeight = std::max(0.01F, length * 0.5F - part.radius);
        PxRigidDynamic* actor = m_impl->createDynamic(
            PxCapsuleGeometry(part.radius, halfHeight), actorPose,
            std::max(0.2F, part.mass), glm::vec3(1.0F), false);
        m_impl->visualEnabled.back() = false;
        actor->setLinearDamping(0.08F);
        actor->setAngularDamping(0.18F);
        actor->setMaxLinearVelocity(14.0F);
        actor->setMaxAngularVelocity(20.0F);
        actor->setSolverIterationCounts(24, 8);

        const glm::mat4 actorWorld = toGlm(actorPose);
        glm::mat4 boneWorld = glm::translate(glm::mat4(1.0F), part.jointPosition)
            * glm::mat4_cast(part.boneRotation)
            * glm::scale(glm::mat4(1.0F), part.boneScale);
        ragdoll.actors.push_back(actor);
        ragdoll.boneFromActor.push_back(glm::inverse(actorWorld) * boneWorld);
        ragdoll.nodeIndices.push_back(part.nodeIndex);
    }

    for (std::size_t i = 0; i < parts.size(); ++i)
    {
        const int parent = parts[i].parentPart;
        if (parent < 0 || static_cast<std::size_t>(parent) >= ragdoll.actors.size()) continue;
        const PxTransform jointWorld(toPhysX(parts[i].jointPosition));
        const PxTransform parentFrame = ragdoll.actors[static_cast<std::size_t>(parent)]
            ->getGlobalPose().getInverse() * jointWorld;
        const PxTransform childFrame = ragdoll.actors[i]->getGlobalPose().getInverse() * jointWorld;
        PxD6Joint* joint = PxD6JointCreate(*m_impl->physics,
            ragdoll.actors[static_cast<std::size_t>(parent)], parentFrame,
            ragdoll.actors[i], childFrame);
        if (joint == nullptr) continue;
        joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eLIMITED);
        joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eLIMITED);
        joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eLIMITED);
        joint->setTwistLimit(PxJointAngularLimitPair(-0.55F, 0.55F));
        joint->setSwingLimit(PxJointLimitCone(0.72F, 0.72F));
        joint->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, false);
        ragdoll.joints.push_back(joint);
    }

    const std::size_t index = m_impl->ragdolls.size();
    m_impl->ragdolls.push_back(std::move(ragdoll));
    return index;
}

bool RigidBodyWorld::ragdollPose(const std::size_t ragdollIndex,
    std::vector<glm::mat4>& boneWorldTransforms) const
{
    if (ragdollIndex >= m_impl->ragdolls.size()) return false;
    const Impl::Ragdoll& ragdoll = m_impl->ragdolls[ragdollIndex];
    boneWorldTransforms.resize(ragdoll.actors.size());
    for (std::size_t i = 0; i < ragdoll.actors.size(); ++i)
        boneWorldTransforms[i] = toGlm(ragdoll.actors[i]->getGlobalPose())
            * ragdoll.boneFromActor[i];
    return true;
}

void RigidBodyWorld::applyRagdollShot(const std::size_t ragdollIndex,
    const glm::vec3& hitPoint, const glm::vec3& direction)
{
    if (ragdollIndex >= m_impl->ragdolls.size()) return;
    Impl::Ragdoll& ragdoll = m_impl->ragdolls[ragdollIndex];
    PxRigidDynamic* closest = nullptr;
    float closestDistance = std::numeric_limits<float>::max();
    for (PxRigidDynamic* actor : ragdoll.actors)
    {
        const glm::vec3 offset = toGlm(actor->getGlobalPose().p) - hitPoint;
        const float distance = glm::dot(offset, offset);
        if (distance < closestDistance)
        {
            closestDistance = distance;
            closest = actor;
        }
    }
    if (closest == nullptr) return;
    closest->wakeUp();
    PxRigidBodyExt::addForceAtPos(*closest,
        toPhysX(glm::normalize(direction) * (SHOT_IMPULSE * 0.72F)),
        toPhysX(hitPoint), PxForceMode::eIMPULSE);
}
