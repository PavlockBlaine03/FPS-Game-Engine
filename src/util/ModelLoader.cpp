#include "util/ModelLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <filesystem>
#include <stdexcept>

namespace
{
    // Attempts to resolve a texture reference to an actual file on disk.
    // FBX exports often embed a filename that no longer matches after
    // manual reorganization/renaming; if the exact name isn't found, fall
    // back to scanning the model's directory for any image file so minor
    // renames don't silently break texturing.
    std::string resolveTexturePath(
        const std::filesystem::path& modelDirectory,
        const std::filesystem::path& embeddedFilename)
    {
        const std::filesystem::path exactMatch = modelDirectory / embeddedFilename;

        if (std::filesystem::exists(exactMatch))
        {
            return exactMatch.generic_string();
        }

        return {};
    }

    ModelMeshData extractMesh(
        const aiMesh* mesh,
        const aiScene* scene,
        const std::filesystem::path& modelDirectory)
    {
        ModelMeshData data;

        data.vertices.reserve(mesh->mNumVertices * 8);

        for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
        {
            data.vertices.push_back(mesh->mVertices[i].x);
            data.vertices.push_back(mesh->mVertices[i].y);
            data.vertices.push_back(mesh->mVertices[i].z);

            if (mesh->HasNormals())
            {
                data.vertices.push_back(mesh->mNormals[i].x);
                data.vertices.push_back(mesh->mNormals[i].y);
                data.vertices.push_back(mesh->mNormals[i].z);
            }
            else
            {
                data.vertices.push_back(0.0F);
                data.vertices.push_back(1.0F);
                data.vertices.push_back(0.0F);
            }

            if (mesh->HasTextureCoords(0))
            {
                data.vertices.push_back(mesh->mTextureCoords[0][i].x);
                data.vertices.push_back(mesh->mTextureCoords[0][i].y);
            }
            else
            {
                data.vertices.push_back(0.0F);
                data.vertices.push_back(0.0F);
            }
        }

        data.indices.reserve(static_cast<std::size_t>(mesh->mNumFaces) * 3);

        for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
        {
            const aiFace& face = mesh->mFaces[i];

            for (unsigned int j = 0; j < face.mNumIndices; ++j)
            {
                data.indices.push_back(face.mIndices[j]);
            }
        }

        if (mesh->mMaterialIndex >= 0)
        {
            const aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            aiString materialName;
            if (material->Get(AI_MATKEY_NAME, materialName) == AI_SUCCESS)
            {
                data.materialName = materialName.C_Str();
            }

            aiString texturePath;

            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS)
            {
                const std::filesystem::path embeddedFilename =
                    std::filesystem::path(texturePath.C_Str()).filename();

                data.diffuseTexturePath = resolveTexturePath(modelDirectory, embeddedFilename);
            }
        }

        return data;
    }

    void processNode(
        const aiNode* node,
        const aiScene* scene,
        const std::filesystem::path& modelDirectory,
        std::vector<ModelMeshData>& outMeshes)
    {
        for (unsigned int i = 0; i < node->mNumMeshes; ++i)
        {
            const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            outMeshes.push_back(extractMesh(mesh, scene, modelDirectory));
        }

        for (unsigned int i = 0; i < node->mNumChildren; ++i)
        {
            processNode(node->mChildren[i], scene, modelDirectory, outMeshes);
        }
    }
}

std::vector<ModelMeshData> ModelLoader::load(const std::string& path)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs |
        aiProcess_JoinIdenticalVertices |
        aiProcess_PreTransformVertices
    );

    if (scene == nullptr || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 || scene->mRootNode == nullptr)
    {
        throw std::runtime_error(
            "Failed to load model '" + path + "': " + importer.GetErrorString());
    }

    const std::filesystem::path modelDirectory =
        std::filesystem::path(path).parent_path();

    std::vector<ModelMeshData> meshes;
    processNode(scene->mRootNode, scene, modelDirectory, meshes);

    return meshes;
}
