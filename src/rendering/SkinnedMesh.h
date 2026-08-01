#pragma once

#include <glm/glm.hpp>

#include <cstddef>
#include <vector>

struct SkinnedVertex
{
    glm::vec3 position{ 0.0F };
    glm::vec3 normal{ 0.0F, 1.0F, 0.0F };
    glm::vec2 texCoord{ 0.0F };
    glm::ivec4 boneIds{ 0 };
    glm::vec4 boneWeights{ 0.0F };
};

class SkinnedMesh
{
public:
    SkinnedMesh(const std::vector<SkinnedVertex>& vertices, const std::vector<unsigned int>& indices);
    ~SkinnedMesh();

    SkinnedMesh(const SkinnedMesh&) = delete;
    SkinnedMesh& operator=(const SkinnedMesh&) = delete;

    void uploadBoneMatrices(const std::vector<glm::mat4>& matrices) const;
    void draw() const;

private:
    unsigned int m_vertexArrayObject = 0;
    unsigned int m_vertexBufferObject = 0;
    unsigned int m_elementBufferObject = 0;
    unsigned int m_boneMatrixBuffer = 0;
    unsigned int m_boneMatrixTexture = 0;
    int m_indexCount = 0;
};
