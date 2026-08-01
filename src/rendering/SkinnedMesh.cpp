#include "rendering/SkinnedMesh.h"

#include <glad/glad.h>

#include <cstddef>

SkinnedMesh::SkinnedMesh(
    const std::vector<SkinnedVertex>& vertices,
    const std::vector<unsigned int>& indices)
    : m_indexCount(static_cast<int>(indices.size()))
{
    glGenVertexArrays(1, &m_vertexArrayObject);
    glGenBuffers(1, &m_vertexBufferObject);
    glGenBuffers(1, &m_elementBufferObject);
    glGenBuffers(1, &m_boneMatrixBuffer);
    glGenTextures(1, &m_boneMatrixTexture);

    glBindVertexArray(m_vertexArrayObject);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferObject);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(SkinnedVertex)),
        vertices.data(),
        GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBufferObject);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
        indices.data(),
        GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
        reinterpret_cast<void*>(offsetof(SkinnedVertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
        reinterpret_cast<void*>(offsetof(SkinnedVertex, normal)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
        reinterpret_cast<void*>(offsetof(SkinnedVertex, texCoord)));
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 4, GL_INT, sizeof(SkinnedVertex),
        reinterpret_cast<void*>(offsetof(SkinnedVertex, boneIds)));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
        reinterpret_cast<void*>(offsetof(SkinnedVertex, boneWeights)));

    glBindVertexArray(0);
}

SkinnedMesh::~SkinnedMesh()
{
    glDeleteTextures(1, &m_boneMatrixTexture);
    glDeleteBuffers(1, &m_boneMatrixBuffer);
    glDeleteBuffers(1, &m_elementBufferObject);
    glDeleteBuffers(1, &m_vertexBufferObject);
    glDeleteVertexArrays(1, &m_vertexArrayObject);
}

void SkinnedMesh::uploadBoneMatrices(const std::vector<glm::mat4>& matrices) const
{
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, m_boneMatrixTexture);
    glBindBuffer(GL_TEXTURE_BUFFER, m_boneMatrixBuffer);
    glBufferData(
        GL_TEXTURE_BUFFER,
        static_cast<GLsizeiptr>(matrices.size() * sizeof(glm::mat4)),
        matrices.data(),
        GL_STREAM_DRAW);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_boneMatrixBuffer);
}

void SkinnedMesh::draw() const
{
    glBindVertexArray(m_vertexArrayObject);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
}
