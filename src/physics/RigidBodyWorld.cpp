#include "physics/RigidBodyWorld.h"

#include "rendering/Camera.h"
#include "rendering/Mesh.h"
#include "rendering/Renderer.h"
#include "rendering/Shader.h"
#include "rendering/Texture.h"
#include "util/MeshFactory.h"

#include <btBulletDynamicsCommon.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace
{
    constexpr short DYNAMIC_GROUP = 1;
    constexpr short STATIC_GROUP = 2;
    constexpr float SHOT_IMPULSE = 8.5F;

    btVector3 toBullet(const glm::vec3& value)
    {
        return btVector3(value.x, value.y, value.z);
    }

    glm::vec3 toGlm(const btVector3& value)
    {
        return glm::vec3(value.x(), value.y(), value.z());
    }
}

struct RigidBodyWorld::Impl
{
    btDefaultCollisionConfiguration collisionConfiguration;
    btCollisionDispatcher dispatcher{&collisionConfiguration};
    btDbvtBroadphase broadphase;
    btSequentialImpulseConstraintSolver solver;
    btDiscreteDynamicsWorld world{&dispatcher, &broadphase, &solver, &collisionConfiguration};

    std::vector<std::unique_ptr<btCollisionShape>> dynamicShapes;
    std::vector<std::unique_ptr<btMotionState>> dynamicMotionStates;
    std::vector<std::unique_ptr<btRigidBody>> dynamicBodies;
    std::vector<std::unique_ptr<btCollisionShape>> staticShapes;
    std::vector<std::unique_ptr<btRigidBody>> staticBodies;
    std::vector<AABB> cachedStaticColliders;

    Impl()
    {
        world.setGravity(btVector3(0.0F, -9.81F, 0.0F));
        world.getSolverInfo().m_numIterations = 12;
        world.getSolverInfo().m_splitImpulse = true;
    }

    ~Impl()
    {
        for (const auto& body : dynamicBodies) world.removeRigidBody(body.get());
        for (const auto& body : staticBodies) world.removeRigidBody(body.get());
    }

    void setStaticColliders(const std::vector<AABB>& colliders)
    {
        bool unchanged = colliders.size() == cachedStaticColliders.size();
        if (unchanged)
        {
            for (std::size_t i = 0; i < colliders.size(); ++i)
            {
                if (colliders[i].min != cachedStaticColliders[i].min
                    || colliders[i].max != cachedStaticColliders[i].max)
                { unchanged = false; break; }
            }
        }
        if (unchanged) return;

        for (const auto& body : staticBodies) world.removeRigidBody(body.get());
        staticBodies.clear();
        staticShapes.clear();
        cachedStaticColliders = colliders;

        staticBodies.reserve(colliders.size());
        staticShapes.reserve(colliders.size());
        for (const AABB& box : colliders)
        {
            const glm::vec3 halfExtents = (box.max - box.min) * 0.5F;
            const glm::vec3 center = (box.min + box.max) * 0.5F;
            auto shape = std::make_unique<btBoxShape>(toBullet(halfExtents));
            btTransform transform;
            transform.setIdentity();
            transform.setOrigin(toBullet(center));
            btRigidBody::btRigidBodyConstructionInfo info(0.0F, nullptr, shape.get());
            auto body = std::make_unique<btRigidBody>(info);
            body->setWorldTransform(transform);
            body->setFriction(0.8F);
            world.addRigidBody(body.get(), STATIC_GROUP, DYNAMIC_GROUP);
            staticShapes.push_back(std::move(shape));
            staticBodies.push_back(std::move(body));
        }
    }
};

RigidBodyWorld::RigidBodyWorld()
    : m_impl(std::make_unique<Impl>())
    , m_texture(std::make_unique<Texture>(glm::vec3(0.72F, 0.22F, 0.12F)))
{
    const MeshData data = MeshFactory::createCube(1.0F, 1.0F);
    m_mesh = std::make_unique<Mesh>(data.vertices.data(), data.vertices.size(),
        data.indices.data(), data.indices.size(), 8);
}

RigidBodyWorld::~RigidBodyWorld() = default;

void RigidBodyWorld::spawnCubeStack()
{
    constexpr float size = 0.45F;
    constexpr float gap = 0.025F;
    constexpr int rows = 5;
    for (int row = 0; row < rows; ++row)
    {
        const int count = rows - row;
        for (int column = 0; column < count; ++column)
        {
            const glm::vec3 position(
                (static_cast<float>(column) - static_cast<float>(count - 1) * 0.5F) * (size + gap),
                -0.45F + size * 0.5F + static_cast<float>(row) * (size + gap), 4.0F);
            auto shape = std::make_unique<btBoxShape>(btVector3(size * 0.5F, size * 0.5F, size * 0.5F));
            btTransform transform;
            transform.setIdentity();
            transform.setOrigin(toBullet(position));
            auto motionState = std::make_unique<btDefaultMotionState>(transform);
            btVector3 inertia(0.0F, 0.0F, 0.0F);
            constexpr float mass = 1.0F;
            shape->calculateLocalInertia(mass, inertia);
            btRigidBody::btRigidBodyConstructionInfo info(mass, motionState.get(), shape.get(), inertia);
            auto body = std::make_unique<btRigidBody>(info);
            body->setFriction(0.75F);
            body->setRestitution(0.42F);
            body->setRollingFriction(0.08F);
            body->setSpinningFriction(0.08F);
            body->setDamping(0.04F, 0.08F);
            body->setUserIndex(static_cast<int>(m_impl->dynamicBodies.size()));
            m_impl->world.addRigidBody(body.get(), DYNAMIC_GROUP, DYNAMIC_GROUP | STATIC_GROUP);
            m_impl->dynamicShapes.push_back(std::move(shape));
            m_impl->dynamicMotionStates.push_back(std::move(motionState));
            m_impl->dynamicBodies.push_back(std::move(body));
        }
    }
}

void RigidBodyWorld::update(float deltaTime, const std::vector<AABB>& staticColliders)
{
    m_impl->setStaticColliders(staticColliders);
    m_impl->world.stepSimulation(std::min(deltaTime, 1.0F / 15.0F), 4, 1.0F / 120.0F);
}

std::vector<AABB> RigidBodyWorld::colliders() const
{
    std::vector<AABB> result;
    result.reserve(m_impl->dynamicBodies.size());
    for (const auto& body : m_impl->dynamicBodies)
    {
        btVector3 minimum; btVector3 maximum;
        body->getAabb(minimum, maximum);
        result.push_back(AABB{toGlm(minimum), toGlm(maximum)});
    }
    return result;
}

bool RigidBodyWorld::raycast(const glm::vec3& start, const glm::vec3& end,
    float& hitTime, std::size_t& bodyIndex) const
{
    btCollisionWorld::ClosestRayResultCallback callback(toBullet(start), toBullet(end));
    callback.m_collisionFilterGroup = DYNAMIC_GROUP;
    callback.m_collisionFilterMask = DYNAMIC_GROUP;
    m_impl->world.rayTest(toBullet(start), toBullet(end), callback);
    if (!callback.hasHit()) return false;
    const int index = callback.m_collisionObject->getUserIndex();
    if (index < 0) return false;
    hitTime = callback.m_closestHitFraction;
    bodyIndex = static_cast<std::size_t>(index);
    return true;
}

void RigidBodyWorld::applyShot(std::size_t bodyIndex, const glm::vec3& hitPoint,
    const glm::vec3& direction)
{
    if (bodyIndex >= m_impl->dynamicBodies.size()) return;
    btRigidBody& body = *m_impl->dynamicBodies[bodyIndex];
    body.activate(true);
    const btVector3 relativePoint = toBullet(hitPoint) - body.getCenterOfMassPosition();
    body.applyImpulse(toBullet(glm::normalize(direction) * SHOT_IMPULSE), relativePoint);
}

void RigidBodyWorld::render(const Renderer& renderer, const Shader& shader,
    const Camera& camera, const Light& light) const
{
    for (std::size_t i = 0; i < m_impl->dynamicBodies.size(); ++i)
    {
        btTransform transform;
        m_impl->dynamicBodies[i]->getMotionState()->getWorldTransform(transform);
        btScalar rawMatrix[16];
        transform.getOpenGLMatrix(rawMatrix);
        glm::mat4 model(1.0F);
        for (int column = 0; column < 4; ++column)
            for (int row = 0; row < 4; ++row)
                model[column][row] = rawMatrix[column * 4 + row];
        const btVector3 halfExtents = static_cast<btBoxShape*>(
            m_impl->dynamicShapes[i].get())->getHalfExtentsWithMargin();
        model = glm::scale(model, toGlm(halfExtents) * 2.0F);
        renderer.draw(*m_mesh, shader, camera, model, *m_texture, light);
    }
}
