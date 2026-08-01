#include "rendering/SkeletalModel.h"

#include "rendering/Renderer.h"
#include "rendering/Texture.h"
#include "util/SkeletalModelLoader.h"

#include <algorithm>
#include <cctype>

namespace
{
    const glm::vec3 FALLBACK_COLOR(0.6F, 0.6F, 0.6F);

    std::string lower(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(),
            [](const unsigned char character) { return static_cast<char>(std::tolower(character)); });
        return value;
    }
}

SkeletalModel::SkeletalModel(const std::string& path)
{
    SkeletalModelData data = SkeletalModelLoader::load(path);
    m_nodes = std::move(data.nodes);
    m_bones = std::move(data.bones);
    m_clips = std::move(data.clips);
    m_inverseRoot = data.inverseRoot;

    for (SkeletalMeshData& meshData : data.meshes)
    {
        SubMesh subMesh;
        subMesh.mesh = std::make_unique<SkinnedMesh>(meshData.vertices, meshData.indices);
        if (!meshData.embeddedDiffuseTexture.empty())
        {
            subMesh.texture = std::make_shared<Texture>(
                meshData.embeddedDiffuseTexture.data(), meshData.embeddedDiffuseTexture.size());
        }
        else if (!meshData.diffuseTexturePath.empty())
        {
            subMesh.texture = std::make_shared<Texture>(meshData.diffuseTexturePath);
        }
        else
        {
            subMesh.texture = std::make_shared<Texture>(FALLBACK_COLOR);
        }
        m_subMeshes.push_back(std::move(subMesh));
    }
}

SkeletalModel::~SkeletalModel() = default;

const AnimationClip* SkeletalModel::findClip(const std::string& name) const
{
    const std::string target = lower(name);
    for (const AnimationClip& clip : m_clips)
    {
        if (lower(clip.name) == target)
        {
            return &clip;
        }
    }
    for (const AnimationClip& clip : m_clips)
    {
        if (lower(clip.name).find(target) != std::string::npos)
        {
            return &clip;
        }
    }
    return nullptr;
}

void SkeletalModel::render(
    const Renderer& renderer,
    const Shader& shader,
    const Camera& camera,
    const glm::mat4& modelMatrix,
    const Light& light,
    const std::vector<glm::mat4>& boneMatrices) const
{
    for (const SubMesh& subMesh : m_subMeshes)
    {
        renderer.drawSkinned(
            *subMesh.mesh, shader, camera, modelMatrix, *subMesh.texture, light, boneMatrices);
    }
}
