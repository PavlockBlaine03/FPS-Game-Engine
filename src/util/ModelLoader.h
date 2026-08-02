#pragma once

#include <string>
#include <vector>

struct ModelMeshData
{
	std::vector<float> vertices;
	std::vector<unsigned int> indices;
	std::string materialName;
	std::string diffuseTexturePath;
};

class ModelLoader
{
public:
	ModelLoader() = delete;

	// Loads all meshes from a model file (.fbx, .obj, .gltf, etc. -- anything
	// Assimp supports). Each returned ModelMeshData corresponds to one
	// sub-mesh/material grouping in the source file.
	[[nodiscard]] static std::vector<ModelMeshData> load(const std::string& path);
};
