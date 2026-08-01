#pragma once

#include "rendering/SkinnedMesh.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Camera;
struct Light;
class Renderer;
class Shader;
class Texture;

struct SkeletonNode
{
    std::string name;
    int parent = -1;
    glm::mat4 bindLocal{ 1.0F };
    glm::vec3 bindTranslation{ 0.0F };
    glm::quat bindRotation{ 1.0F, 0.0F, 0.0F, 0.0F };
    glm::vec3 bindScale{ 1.0F };
};

struct SkeletonBone
{
    std::string name;
    int nodeIndex = -1;
    glm::mat4 inverseBind{ 1.0F };
};

template<typename T>
struct AnimationKey
{
    float time = 0.0F;
    T value{};
};

struct AnimationChannel
{
    int nodeIndex = -1;
    std::vector<AnimationKey<glm::vec3>> translations;
    std::vector<AnimationKey<glm::quat>> rotations;
    std::vector<AnimationKey<glm::vec3>> scales;
};

struct AnimationClip
{
    std::string name;
    float durationSeconds = 0.0F;
    std::vector<AnimationChannel> channels;
    std::unordered_map<int, std::size_t> channelByNode;
};

struct SkeletalMeshData
{
    std::vector<SkinnedVertex> vertices;
    std::vector<unsigned int> indices;
    std::string diffuseTexturePath;
    std::vector<unsigned char> embeddedDiffuseTexture;
};

struct SkeletalModelData
{
    std::vector<SkeletonNode> nodes;
    std::vector<SkeletonBone> bones;
    std::vector<AnimationClip> clips;
    std::vector<SkeletalMeshData> meshes;
    glm::mat4 inverseRoot{ 1.0F };
};

class SkeletalModel
{
public:
    explicit SkeletalModel(const std::string& path);
    ~SkeletalModel();

    SkeletalModel(const SkeletalModel&) = delete;
    SkeletalModel& operator=(const SkeletalModel&) = delete;

    [[nodiscard]] const std::vector<SkeletonNode>& nodes() const { return m_nodes; }
    [[nodiscard]] const std::vector<SkeletonBone>& bones() const { return m_bones; }
    [[nodiscard]] const glm::mat4& inverseRoot() const { return m_inverseRoot; }
    [[nodiscard]] const AnimationClip* findClip(const std::string& name) const;

    void render(
        const Renderer& renderer,
        const Shader& shader,
        const Camera& camera,
        const glm::mat4& modelMatrix,
        const Light& light,
        const std::vector<glm::mat4>& boneMatrices) const;

private:
    struct SubMesh
    {
        std::unique_ptr<SkinnedMesh> mesh;
        std::shared_ptr<Texture> texture;
    };

    std::vector<SkeletonNode> m_nodes;
    std::vector<SkeletonBone> m_bones;
    std::vector<AnimationClip> m_clips;
    std::vector<SubMesh> m_subMeshes;
    glm::mat4 m_inverseRoot{ 1.0F };
};
