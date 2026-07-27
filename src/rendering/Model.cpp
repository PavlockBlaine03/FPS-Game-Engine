#include "rendering/Model.h"
#include "util/ModelLoader.h"

#include <iostream>

namespace
{
    // Fallback color used when a sub-mesh has no diffuse texture assigned,
    // or when the referenced texture file couldn't be found/loaded (common
    // with downloaded model packs that reference textures not included in
    // the archive).
    const glm::vec3 FALLBACK_COLOR(0.6F, 0.6F, 0.6F);
}

Model::Model(const std::string& path)
{
    const std::vector<ModelMeshData> meshDataList = ModelLoader::load(path);

    for (const ModelMeshData& meshData : meshDataList)
    {
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
                subMesh.texture = std::make_shared<Texture>(FALLBACK_COLOR);
            }
        }
        else
        {
            subMesh.texture = std::make_shared<Texture>(FALLBACK_COLOR);
        }

        m_subMeshes.push_back(std::move(subMesh));
    }
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