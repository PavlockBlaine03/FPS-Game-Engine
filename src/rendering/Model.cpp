#include "rendering/Model.h"
#include "util/ModelLoader.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <limits>

namespace
{
    // Fallback color used when a sub-mesh has no diffuse texture assigned,
    // or when the referenced texture file couldn't be found/loaded (common
    // with downloaded model packs that reference textures not included in
    // the archive).
    const glm::vec3 FALLBACK_COLOR(0.6F, 0.6F, 0.6F);

    glm::vec3 fallbackColorForMaterial(std::string materialName)
    {
        std::transform(materialName.begin(), materialName.end(), materialName.begin(),
            [](const unsigned char character) { return static_cast<char>(std::tolower(character)); });

        if (materialName.find("carpet") != std::string::npos)
        {
            return glm::vec3(0.24F, 0.09F, 0.07F);
        }
        if (materialName.find("legno") != std::string::npos
            || materialName.find("wood") != std::string::npos)
        {
            return glm::vec3(0.28F, 0.14F, 0.07F);
        }
        if (materialName.find("plater") != std::string::npos
            || materialName.find("plaster") != std::string::npos)
        {
            return glm::vec3(0.82F, 0.80F, 0.73F);
        }

        return FALLBACK_COLOR;
    }
}

Model::Model(const std::string& path)
{
    const std::vector<ModelMeshData> meshDataList = ModelLoader::load(path);
    glm::vec3 minimum(std::numeric_limits<float>::max());
    glm::vec3 maximum(std::numeric_limits<float>::lowest());

    for (const ModelMeshData& meshData : meshDataList)
    {
        const glm::vec3 fallbackColor = fallbackColorForMaterial(meshData.materialName);
        for (std::size_t vertex = 0; vertex + 2 < meshData.vertices.size(); vertex += 8)
        {
            const glm::vec3 position(
                meshData.vertices[vertex], meshData.vertices[vertex + 1], meshData.vertices[vertex + 2]);
            minimum = glm::min(minimum, position);
            maximum = glm::max(maximum, position);
        }
        SubMesh subMesh;

        subMesh.mesh = std::make_unique<Mesh>(
            meshData.vertices.data(),
            meshData.vertices.size(),
            meshData.indices.data(),
            meshData.indices.size(),
            8 // position(3) + normal(3) + uv(2)
        );

        if (!meshData.diffuseTexturePath.empty())
        {
            try
            {
                subMesh.texture = std::make_shared<Texture>(meshData.diffuseTexturePath);
            }
            catch (const std::exception& e)
            {
                std::cerr << "Warning: " << e.what()
                          << " -- falling back to solid color.\n";
                subMesh.texture = std::make_shared<Texture>(fallbackColor);
            }
        }
        else
        {
            subMesh.texture = std::make_shared<Texture>(fallbackColor);
        }

        m_subMeshes.push_back(std::move(subMesh));
    }
    if (!meshDataList.empty()) m_localBounds = AABB{ minimum, maximum };
}

Model::~Model() = default;

void Model::render(
    const Renderer& renderer,
    const Shader& shader,
    const Camera& camera,
    const glm::mat4& model,
    const Light& light) const
{
    for (const SubMesh& subMesh : m_subMeshes)
    {
        renderer.draw(*subMesh.mesh, shader, camera, model, *subMesh.texture, light);
    }
}
