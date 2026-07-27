#pragma once

#include <cstddef>
#include <vector>

struct MeshData
{
	std::vector<float> vertices;
	std::vector<unsigned int> indices;
};

class MeshFactory
{
public:
	MeshFactory() = delete;

	// mirrorFrontBackU: horizontally flips the U texture coordinate on the
	// front/back faces only (u -> textureTiling - u). Used by Door so a
	// texture authored with its handle on the "wrong" side (ending up over
	// the hinge edge after the door's hinge-relative transform) can be
	// corrected without affecting other createCube() callers (e.g. Scene's
	// tiled walls/floor/ceiling).
	[[nodiscard]] static MeshData createCube(
		float size = 1.0f,
		float textureTiling = 1.0f,
		bool mirrorFrontBackU = false);
};