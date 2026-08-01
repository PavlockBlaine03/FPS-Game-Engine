#include "util/SkeletalModelLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <unordered_map>

namespace
{
    glm::mat4 toGlm(const aiMatrix4x4& value)
    {
        glm::mat4 result(1.0F);
        result[0] = glm::vec4(value.a1, value.b1, value.c1, value.d1);
        result[1] = glm::vec4(value.a2, value.b2, value.c2, value.d2);
        result[2] = glm::vec4(value.a3, value.b3, value.c3, value.d3);
        result[3] = glm::vec4(value.a4, value.b4, value.c4, value.d4);
        return result;
    }

    glm::vec3 toGlm(const aiVector3D& value)
    {
        return glm::vec3(value.x, value.y, value.z);
    }

    glm::quat toGlm(const aiQuaternion& value)
    {
        return glm::normalize(glm::quat(value.w, value.x, value.y, value.z));
    }

    void decomposeBindTransform(SkeletonNode& node, const aiMatrix4x4& source)
    {
        aiVector3D scale;
        aiVector3D translation;
        aiQuaternion rotation;
        source.Decompose(scale, rotation, translation);
        node.bindScale = toGlm(scale);
        node.bindRotation = toGlm(rotation);
        node.bindTranslation = toGlm(translation);
    }

    void readNodes(
        const aiNode* node,
        const int parent,
        std::vector<SkeletonNode>& nodes,
        std::unordered_map<std::string, int>& nodeByName)
    {
        const int nodeIndex = static_cast<int>(nodes.size());
        SkeletonNode skeletonNode;
        skeletonNode.name = node->mName.C_Str();
        skeletonNode.parent = parent;
        skeletonNode.bindLocal = toGlm(node->mTransformation);
        decomposeBindTransform(skeletonNode, node->mTransformation);
        nodes.push_back(skeletonNode);
        nodeByName[skeletonNode.name] = nodeIndex;

        for (unsigned int i = 0; i < node->mNumChildren; ++i)
        {
            readNodes(node->mChildren[i], nodeIndex, nodes, nodeByName);
        }
    }

    void readDiffuseTexture(
        const aiMesh* mesh,
        const aiScene* scene,
        const std::filesystem::path& directory,
        SkeletalMeshData& data)
    {
        if (mesh->mMaterialIndex >= scene->mNumMaterials)
        {
            return;
        }

        aiString texturePath;
        const aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        if (material->GetTexture(aiTextureType_BASE_COLOR, 0, &texturePath) != AI_SUCCESS
            && material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) != AI_SUCCESS)
        {
            return;
        }

        const std::string reference = texturePath.C_Str();
        if (!reference.empty() && reference.front() == '*')
        {
            const aiTexture* embedded = scene->GetEmbeddedTexture(reference.c_str());
            if (embedded != nullptr && embedded->mHeight == 0 && embedded->mWidth > 0)
            {
                const auto* firstByte = reinterpret_cast<const unsigned char*>(embedded->pcData);
                data.embeddedDiffuseTexture.assign(firstByte, firstByte + embedded->mWidth);
            }
            return;
        }

        const std::filesystem::path candidate = directory / std::filesystem::path(reference);
        if (std::filesystem::exists(candidate))
        {
            data.diffuseTexturePath = candidate.generic_string();
        }
    }

    void addInfluence(SkinnedVertex& vertex, const int boneId, const float weight)
    {
        int weakest = 0;
        for (int slot = 0; slot < 4; ++slot)
        {
            if (vertex.boneWeights[slot] == 0.0F)
            {
                vertex.boneIds[slot] = boneId;
                vertex.boneWeights[slot] = weight;
                return;
            }
            if (vertex.boneWeights[slot] < vertex.boneWeights[weakest])
            {
                weakest = slot;
            }
        }

        if (weight > vertex.boneWeights[weakest])
        {
            vertex.boneIds[weakest] = boneId;
            vertex.boneWeights[weakest] = weight;
        }
    }

    SkeletalMeshData readMesh(
        const aiMesh* mesh,
        const aiScene* scene,
        const std::filesystem::path& directory,
        const std::unordered_map<std::string, int>& nodeByName,
        std::unordered_map<std::string, int>& boneByName,
        std::vector<SkeletonBone>& bones,
        const int skeletonRootBoneId)
    {
        SkeletalMeshData data;
        data.vertices.resize(mesh->mNumVertices);

        for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
        {
            SkinnedVertex& vertex = data.vertices[i];
            vertex.position = toGlm(mesh->mVertices[i]);
            if (mesh->HasNormals())
            {
                vertex.normal = toGlm(mesh->mNormals[i]);
            }
            if (mesh->HasTextureCoords(0))
            {
                vertex.texCoord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            }
        }

        int dominantBoneId = -1;
        float dominantBoneWeight = 0.0F;

        for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
        {
            const aiBone* sourceBone = mesh->mBones[boneIndex];
            const std::string name = sourceBone->mName.C_Str();
            int id = -1;

            const auto existing = boneByName.find(name);
            if (existing == boneByName.end())
            {
                const auto node = nodeByName.find(name);
                if (node == nodeByName.end())
                {
                    throw std::runtime_error("Skeleton bone has no matching node: " + name);
                }

                id = static_cast<int>(bones.size());
                boneByName[name] = id;
                bones.push_back(SkeletonBone{ name, node->second, toGlm(sourceBone->mOffsetMatrix) });
            }
            else
            {
                id = existing->second;
            }

            float accumulatedWeight = 0.0F;
            for (unsigned int weightIndex = 0; weightIndex < sourceBone->mNumWeights; ++weightIndex)
            {
                const aiVertexWeight& influence = sourceBone->mWeights[weightIndex];
                if (influence.mVertexId >= data.vertices.size())
                {
                    throw std::runtime_error("Bone weight references an invalid vertex");
                }
                addInfluence(data.vertices[influence.mVertexId], id, influence.mWeight);
                accumulatedWeight += influence.mWeight;
            }

            if (accumulatedWeight > dominantBoneWeight)
            {
                dominantBoneWeight = accumulatedWeight;
                dominantBoneId = id;
            }
        }

        const bool meshHasNoInfluences = dominantBoneId < 0;
        if (meshHasNoInfluences)
        {
            dominantBoneId = skeletonRootBoneId;
        }

        std::size_t unweightedVertexCount = 0;
        for (SkinnedVertex& vertex : data.vertices)
        {
            const float total = vertex.boneWeights.x + vertex.boneWeights.y
                + vertex.boneWeights.z + vertex.boneWeights.w;
            if (total <= 0.0F)
            {
                if (dominantBoneId < 0)
                {
                    throw std::runtime_error("Skeletal model has no usable root bone for an unweighted mesh");
                }

                // Some exporters leave rigid accessories or isolated vertices
                // unpainted. Keep them attached to the mesh's dominant bone
                // instead of collapsing them to the origin or rejecting the
                // entire character.
                vertex.boneIds.x = dominantBoneId;
                vertex.boneWeights.x = 1.0F;
                ++unweightedVertexCount;
                continue;
            }
            vertex.boneWeights /= total;
        }

        if (unweightedVertexCount > 0)
        {
            std::cerr << "Warning: assigned " << unweightedVertexCount
                      << " unweighted character vertices to "
                      << (meshHasNoInfluences ? "the skeleton root bone" : "their mesh's dominant bone")
                      << ".\n";
        }

        for (unsigned int faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex)
        {
            const aiFace& face = mesh->mFaces[faceIndex];
            for (unsigned int i = 0; i < face.mNumIndices; ++i)
            {
                data.indices.push_back(face.mIndices[i]);
            }
        }

        readDiffuseTexture(mesh, scene, directory, data);
        return data;
    }

    AnimationClip readAnimation(
        const aiAnimation* animation,
        const std::unordered_map<std::string, int>& nodeByName,
        const unsigned int animationIndex)
    {
        AnimationClip clip;
        clip.name = animation->mName.length > 0
            ? animation->mName.C_Str()
            : "Animation_" + std::to_string(animationIndex);

        const double ticksPerSecond = animation->mTicksPerSecond > 0.0
            ? animation->mTicksPerSecond
            : 25.0;
        clip.durationSeconds = static_cast<float>(animation->mDuration / ticksPerSecond);

        for (unsigned int channelIndex = 0; channelIndex < animation->mNumChannels; ++channelIndex)
        {
            const aiNodeAnim* source = animation->mChannels[channelIndex];
            const auto node = nodeByName.find(source->mNodeName.C_Str());
            if (node == nodeByName.end())
            {
                continue;
            }

            AnimationChannel channel;
            channel.nodeIndex = node->second;
            for (unsigned int i = 0; i < source->mNumPositionKeys; ++i)
            {
                channel.translations.push_back({
                    static_cast<float>(source->mPositionKeys[i].mTime / ticksPerSecond),
                    toGlm(source->mPositionKeys[i].mValue) });
            }
            for (unsigned int i = 0; i < source->mNumRotationKeys; ++i)
            {
                channel.rotations.push_back({
                    static_cast<float>(source->mRotationKeys[i].mTime / ticksPerSecond),
                    toGlm(source->mRotationKeys[i].mValue) });
            }
            for (unsigned int i = 0; i < source->mNumScalingKeys; ++i)
            {
                channel.scales.push_back({
                    static_cast<float>(source->mScalingKeys[i].mTime / ticksPerSecond),
                    toGlm(source->mScalingKeys[i].mValue) });
            }

            clip.channelByNode[channel.nodeIndex] = clip.channels.size();
            clip.channels.push_back(std::move(channel));
        }
        return clip;
    }

    void registerBones(
        const aiScene* scene,
        const std::unordered_map<std::string, int>& nodeByName,
        std::unordered_map<std::string, int>& boneByName,
        std::vector<SkeletonBone>& bones)
    {
        for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
        {
            const aiMesh* mesh = scene->mMeshes[meshIndex];
            for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
            {
                const aiBone* sourceBone = mesh->mBones[boneIndex];
                const std::string name = sourceBone->mName.C_Str();
                if (boneByName.contains(name))
                {
                    continue;
                }

                const auto node = nodeByName.find(name);
                if (node == nodeByName.end())
                {
                    throw std::runtime_error("Skeleton bone has no matching node: " + name);
                }

                const int id = static_cast<int>(bones.size());
                boneByName[name] = id;
                bones.push_back(SkeletonBone{ name, node->second, toGlm(sourceBone->mOffsetMatrix) });
            }
        }
    }

    int findSkeletonRootBone(const std::vector<SkeletonNode>& nodes, const std::vector<SkeletonBone>& bones)
    {
        int bestBone = -1;
        int bestDepth = std::numeric_limits<int>::max();
        for (std::size_t boneIndex = 0; boneIndex < bones.size(); ++boneIndex)
        {
            int depth = 0;
            int node = bones[boneIndex].nodeIndex;
            while (node >= 0)
            {
                ++depth;
                node = nodes[static_cast<std::size_t>(node)].parent;
            }
            if (depth < bestDepth)
            {
                bestDepth = depth;
                bestBone = static_cast<int>(boneIndex);
            }
        }
        return bestBone;
    }
}

SkeletalModelData SkeletalModelLoader::load(const std::string& path)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices);

    if (scene == nullptr || scene->mRootNode == nullptr
        || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0)
    {
        throw std::runtime_error("Failed to load skeletal model '" + path + "': " + importer.GetErrorString());
    }

    SkeletalModelData data;
    std::unordered_map<std::string, int> nodeByName;
    readNodes(scene->mRootNode, -1, data.nodes, nodeByName);
    data.inverseRoot = glm::inverse(toGlm(scene->mRootNode->mTransformation));

    std::unordered_map<std::string, int> boneByName;
    registerBones(scene, nodeByName, boneByName, data.bones);
    if (data.bones.empty())
    {
        throw std::runtime_error(
            "Model contains no skin/deform bones. Export the humanoid mesh with its armature, "
            "vertex weights, and Idle/Walk actions enabled");
    }
    const int skeletonRootBoneId = findSkeletonRootBone(data.nodes, data.bones);
    const std::filesystem::path directory = std::filesystem::path(path).parent_path();
    for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
    {
        data.meshes.push_back(readMesh(
            scene->mMeshes[meshIndex], scene, directory, nodeByName, boneByName,
            data.bones, skeletonRootBoneId));
    }

    for (unsigned int animationIndex = 0; animationIndex < scene->mNumAnimations; ++animationIndex)
    {
        AnimationClip clip = readAnimation(scene->mAnimations[animationIndex], nodeByName, animationIndex);
        if (clip.durationSeconds > 0.0F)
        {
            data.clips.push_back(std::move(clip));
        }
    }

    if (data.clips.empty())
    {
        throw std::runtime_error("Model has no usable animation clips: " + path);
    }
    return data;
}
