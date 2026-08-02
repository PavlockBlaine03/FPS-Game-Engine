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
    constexpr short PLAYER_GROUP = 4;
    constexpr short NPC_GROUP = 8;
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
    std::vector<glm::vec3> visualScales;
    std::vector<bool> sphereVisuals;
    std::vector<std::unique_ptr<btCollisionShape>> staticShapes;
    std::vector<std::unique_ptr<btRigidBody>> staticBodies;
    std::vector<AABB> cachedStaticColliders;
    std::unique_ptr<btBoxShape> playerShape;
    std::unique_ptr<btRigidBody> playerBody;
    bool playerInitialized = false;
    std::vector<std::unique_ptr<btBoxShape>> npcShapes;
    std::vector<std::unique_ptr<btRigidBody>> npcBodies;
    std::vector<bool> npcInitialized;

    Impl()
    {
        world.setGravity(btVector3(0.0F, -9.81F, 0.0F));
        world.getSolverInfo().m_numIterations = 12;
        world.getSolverInfo().m_splitImpulse = true;

        playerShape = std::make_unique<btBoxShape>(btVector3(0.3F, 0.9F, 0.3F));
        btRigidBody::btRigidBodyConstructionInfo playerInfo(0.0F, nullptr, playerShape.get());
        playerBody = std::make_unique<btRigidBody>(playerInfo);
        playerBody->setCollisionFlags(playerBody->getCollisionFlags()
            | btCollisionObject::CF_KINEMATIC_OBJECT);
        playerBody->setActivationState(DISABLE_DEACTIVATION);
        playerBody->setFriction(0.8F);
        world.addRigidBody(playerBody.get(), PLAYER_GROUP, DYNAMIC_GROUP);
    }

    ~Impl()
    {
        world.removeRigidBody(playerBody.get());
        for (const auto& body : npcBodies) world.removeRigidBody(body.get());
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
    , m_blueTexture(std::make_unique<Texture>(glm::vec3(0.08F, 0.25F, 0.9F)))
{
    const MeshData data = MeshFactory::createCube(1.0F, 1.0F);
    m_mesh = std::make_unique<Mesh>(data.vertices.data(), data.vertices.size(),
        data.indices.data(), data.indices.size(), 8);
    const MeshData sphereData = MeshFactory::createSphere();
    m_sphereMesh = std::make_unique<Mesh>(sphereData.vertices.data(), sphereData.vertices.size(),
        sphereData.indices.data(), sphereData.indices.size(), 8);
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
            m_impl->world.addRigidBody(body.get(), DYNAMIC_GROUP,
                DYNAMIC_GROUP | STATIC_GROUP | PLAYER_GROUP | NPC_GROUP);
            m_impl->dynamicShapes.push_back(std::move(shape));
            m_impl->dynamicMotionStates.push_back(std::move(motionState));
            m_impl->dynamicBodies.push_back(std::move(body));
            m_impl->visualScales.push_back(glm::vec3(size));
            m_impl->sphereVisuals.push_back(false);
        }
    }
}

void RigidBodyWorld::spawnBlueBall(const glm::vec3& position)
{
    constexpr float radius = 0.32F;
    auto shape = std::make_unique<btSphereShape>(radius);
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(toBullet(position));
    auto motionState = std::make_unique<btDefaultMotionState>(transform);
    btVector3 inertia(0.0F, 0.0F, 0.0F);
    constexpr float mass = 0.8F;
    shape->calculateLocalInertia(mass, inertia);
    btRigidBody::btRigidBodyConstructionInfo info(mass, motionState.get(), shape.get(), inertia);
    auto body = std::make_unique<btRigidBody>(info);
    body->setFriction(0.65F);
    body->setRestitution(0.62F);
    body->setRollingFriction(0.04F);
    body->setSpinningFriction(0.03F);
    body->setDamping(0.025F, 0.04F);
    body->setUserIndex(static_cast<int>(m_impl->dynamicBodies.size()));
    m_impl->world.addRigidBody(body.get(), DYNAMIC_GROUP,
        DYNAMIC_GROUP | STATIC_GROUP | PLAYER_GROUP | NPC_GROUP);
    m_impl->dynamicShapes.push_back(std::move(shape));
    m_impl->dynamicMotionStates.push_back(std::move(motionState));
    m_impl->dynamicBodies.push_back(std::move(body));
    m_impl->visualScales.push_back(glm::vec3(radius * 2.0F));
    m_impl->sphereVisuals.push_back(true);
}

void RigidBodyWorld::moveNpcColliders(const std::vector<AABB>& colliders, float deltaTime)
{
    while (m_impl->npcBodies.size() > colliders.size())
    {
        m_impl->world.removeRigidBody(m_impl->npcBodies.back().get());
        m_impl->npcBodies.pop_back();
        m_impl->npcShapes.pop_back();
        m_impl->npcInitialized.pop_back();
    }

    while (m_impl->npcBodies.size() < colliders.size())
    {
        auto shape = std::make_unique<btBoxShape>(btVector3(0.3F, 0.9F, 0.3F));
        btRigidBody::btRigidBodyConstructionInfo info(0.0F, nullptr, shape.get());
        auto body = std::make_unique<btRigidBody>(info);
        body->setCollisionFlags(body->getCollisionFlags()
            | btCollisionObject::CF_KINEMATIC_OBJECT);
        body->setActivationState(DISABLE_DEACTIVATION);
        body->setFriction(0.8F);
        m_impl->world.addRigidBody(body.get(), NPC_GROUP, DYNAMIC_GROUP);
        m_impl->npcShapes.push_back(std::move(shape));
        m_impl->npcBodies.push_back(std::move(body));
        m_impl->npcInitialized.push_back(false);
    }

    for (std::size_t i = 0; i < colliders.size(); ++i)
    {
        const glm::vec3 halfExtents = (colliders[i].max - colliders[i].min) * 0.5F;
        const glm::vec3 center = (colliders[i].min + colliders[i].max) * 0.5F;
        const btScalar margin = m_impl->npcShapes[i]->getMargin();
        m_impl->npcShapes[i]->setImplicitShapeDimensions(toBullet(halfExtents)
            - btVector3(margin, margin, margin));

        btTransform transform = m_impl->npcBodies[i]->getWorldTransform();
        const btVector3 nextPosition = toBullet(center);
        if (m_impl->npcInitialized[i] && deltaTime > 0.00001F)
            m_impl->npcBodies[i]->setLinearVelocity(
                (nextPosition - transform.getOrigin()) / deltaTime);
        else
            m_impl->npcBodies[i]->setLinearVelocity(btVector3(0.0F, 0.0F, 0.0F));
        transform.setIdentity();
        transform.setOrigin(nextPosition);
        m_impl->npcBodies[i]->setWorldTransform(transform);
        m_impl->world.updateSingleAabb(m_impl->npcBodies[i].get());
        m_impl->npcInitialized[i] = true;
    }
}

void RigidBodyWorld::movePlayerCollider(const glm::vec3& position,
    const glm::vec3& halfExtents, float deltaTime)
{
    m_impl->playerShape->setImplicitShapeDimensions(toBullet(halfExtents)
        - btVector3(m_impl->playerShape->getMargin(), m_impl->playerShape->getMargin(),
            m_impl->playerShape->getMargin()));

    btTransform transform = m_impl->playerBody->getWorldTransform();
    const btVector3 nextPosition = toBullet(position);
    if (m_impl->playerInitialized && deltaTime > 0.00001F)
        m_impl->playerBody->setLinearVelocity(
            (nextPosition - transform.getOrigin()) / deltaTime);
    else
        m_impl->playerBody->setLinearVelocity(btVector3(0.0F, 0.0F, 0.0F));
    transform.setIdentity();
    transform.setOrigin(nextPosition);
    m_impl->playerBody->setWorldTransform(transform);
    m_impl->world.updateSingleAabb(m_impl->playerBody.get());
    m_impl->playerInitialized = true;
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
        model = glm::scale(model, m_impl->visualScales[i]);
        const Mesh& mesh = m_impl->sphereVisuals[i] ? *m_sphereMesh : *m_mesh;
        const Texture& texture = m_impl->sphereVisuals[i] ? *m_blueTexture : *m_texture;
        renderer.draw(mesh, shader, camera, model, texture, light);
    }
}
