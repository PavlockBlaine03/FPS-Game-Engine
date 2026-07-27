#pragma once

#include <cstddef>
#include <vector>

class Mesh
{
public:
	// vertexLayout: number of floats per vertex. Existing procedural meshes
	// use 5 (position + uv); model-loaded meshes use 8 (position + normal + uv).
	Mesh(const float* vertices, std::size_t vertexCount, int floatsPerVertex = 5);
	Mesh(
		const float* vertices,
		std::size_t vertextCount,
		const unsigned int* indices,
		std::size_t indexCount,
		int floatsPerVertex = 5);
	~Mesh();

	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;

	void draw() const;

private:
	void setupVertexAttributes() const;

	unsigned int m_vertexArrayObject = 0;
	unsigned int m_vertexBufferObject = 0;
	unsigned int m_elementBufferObject = 0;

	int m_vertexCount = 0;
	int m_indexCount = 0;
	int m_floatsPerVertex = 5;
};