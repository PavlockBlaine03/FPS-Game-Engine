#pragma once

#include "physics/AABB.h"
#include "rendering/Light.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <vector>

class Camera;
class Mesh;
class Renderer;
class Shader;
class Texture;

class RigidBodyWorld
{
public:
    struct RagdollPart
    {
        int nodeIndex = -1;
        int parentPart = -1;
        glm::vec3 jointPosition{ 0.0F };
        glm::vec3 endPosition{ 0.0F };
        glm::quat boneRotation{ 1.0F, 0.0F, 0.0F, 0.0F };
        glm::vec3 boneScale{ 1.0F };
        float radius = 0.08F;
        float mass = 1.0F;
    };

    RigidBodyWorld();
    ~RigidBodyWorld();

    RigidBodyWorld(const RigidBodyWorld&) = delete;
    RigidBodyWorld& operator=(const RigidBodyWorld&) = delete;

    void spawnCubeStack(const glm::vec3& basePosition = glm::vec3(0.0F, -0.45F, 4.0F),
        int rows = 5);
    void spawnBlueBall(const glm::vec3& position = glm::vec3(3.0F, 0.8F, 4.0F));
    void movePlayerCollider(const glm::vec3& position,
        const glm::vec3& halfExtents, float deltaTime);
    void moveNpcColliders(const std::vector<AABB>& colliders, float deltaTime);
    void update(float deltaTime, const std::vector<AABB>& staticColliders);
    void render(const Renderer& renderer, const Shader& shader,
        const Camera& camera, const Light& light) const;

    [[nodiscard]] std::vector<AABB> colliders() const;
    [[nodiscard]] bool raycast(const glm::vec3& start, const glm::vec3& end,
        float& hitTime, std::size_t& bodyIndex) const;
    void applyShot(std::size_t bodyIndex, const glm::vec3& hitPoint,
        const glm::vec3& direction);
    [[nodiscard]] std::size_t createRagdoll(const std::vector<RagdollPart>& parts);
    [[nodiscard]] bool ragdollPose(std::size_t ragdollIndex,
        std::vector<glm::mat4>& boneWorldTransforms) const;
    void applyRagdollShot(std::size_t ragdollIndex, const glm::vec3& hitPoint,
        const glm::vec3& direction);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    std::unique_ptr<Mesh> m_mesh;
    std::unique_ptr<Texture> m_texture;
    std::unique_ptr<Mesh> m_sphereMesh;
    std::unique_ptr<Texture> m_blueTexture;
};
