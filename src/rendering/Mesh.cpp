#include "rendering/Mesh.h"
#include <glad/glad.h>

Mesh::Mesh(const float* vertices, const std::size_t vertexCount, const int floatsPerVertex)
	: m_vertexCount(static_cast<int>(vertexCount / floatsPerVertex))
	, m_floatsPerVertex(floatsPerVertex)
{
	glGenVertexArrays(1, &m_vertexArrayObject);
	glGenBuffers(1, &m_vertexBufferObject);

	glBindVertexArray(m_vertexArrayObject);

	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferObject);
	glBufferData(
		GL_ARRAY_BUFFER,
		static_cast<long long>(vertexCount * sizeof(float)),
		vertices,
		GL_STATIC_DRAW
	);
	setupVertexAttributes();

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}
Mesh::Mesh(
    const float* vertices,
    const std::size_t vertexCount,
    const unsigned int* indices,
    const std::size_t indexCount,
    const int floatsPerVertex)
    : m_indexCount(static_cast<int>(indexCount))
    , m_floatsPerVertex(floatsPerVertex)
{
    glGenVertexArrays(1, &m_vertexArrayObject);
    glGenBuffers(1, &m_vertexBufferObject);
    glGenBuffers(1, &m_elementBufferObject);

    glBindVertexArray(m_vertexArrayObject);

    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferObject);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<long long>(vertexCount * sizeof(float)),
        vertices,
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBufferObject);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<long long>(indexCount * sizeof(unsigned int)),
        indices,
        GL_STATIC_DRAW
    );

    setupVertexAttributes();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

Mesh::~Mesh()
{
    glDeleteVertexArrays(1, &m_vertexArrayObject);
    glDeleteBuffers(1, &m_vertexBufferObject);

    if (m_elementBufferObject != 0)
    {
        glDeleteBuffers(1, &m_elementBufferObject);
    }
}

void Mesh::setupVertexAttributes() const
{
    const auto stride = static_cast<GLsizei>(m_floatsPerVertex * sizeof(float));

    // Position: location 0, 3 floats
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
    glEnableVertexAttribArray(0);

    if (m_floatsPerVertex == 5)
    {
        // Legacy procedural layout: position + uv (no normal).
        glVertexAttribPointer(
            1,
            2,
            GL_FLOAT,
            GL_FALSE,
            stride,
            reinterpret_cast<void*>(3 * sizeof(float))
        );
        glEnableVertexAttribArray(1);
    }
    else if (m_floatsPerVertex == 8)
    {
        // Model layout: position + normal + uv.
        glVertexAttribPointer(
            1,
            3,
            GL_FLOAT,
            GL_FALSE,
            stride,
            reinterpret_cast<void*>(3 * sizeof(float))
        );
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(
            2,
            2,
            GL_FLOAT,
            GL_FALSE,
            stride,
            reinterpret_cast<void*>(6 * sizeof(float))
        );
        glEnableVertexAttribArray(2);
    }
}

void Mesh::draw() const
{
    glBindVertexArray(m_vertexArrayObject);

    if (m_indexCount > 0)
    {
        glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
    }
    else
    {
        glDrawArrays(GL_TRIANGLES, 0, m_vertexCount);
    }
}