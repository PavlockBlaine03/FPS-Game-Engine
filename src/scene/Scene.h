#pragma once

#include "physics/AABB.h"
#include "rendering/Camera.h"
#include "rendering/Light.h"
#include "rendering/Mesh.h"
#include "rendering/Renderer.h"
#include "rendering/Shader.h"
#include "rendering/Texture.h"
#include "scene/WorldGenerator.h"

#include <glm/glm.hpp>
#include <vector>
#include <memory>

class Model;

class Scene
{
public:
	// Doorway geometry, exposed so other systems (e.g. Door) can align
	// exactly with the opening left in buildEnvironment() without
	// duplicating magic numbers.
	static constexpr float DOORWAY_HALF_WIDTH = WorldGenerator::DOORWAY_HALF_WIDTH;
	static constexpr float DOORWAY_HEIGHT = WorldGenerator::DOORWAY_HEIGHT;
	static constexpr float WALL_THICKNESS = WorldGenerator::WALL_THICKNESS;
	static constexpr float FLOOR_TOP_Y = WorldGenerator::FLOOR_TOP_Y;

	Scene();
	~Scene();

	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	void render(
		const Renderer& renderer,
		const Shader& shader,
		const Camera& camera,
		const Light& light) const;

	[[nodiscard]] const std::vector<AABB>& colliders() const { return m_colliders; }

private:
	std::unique_ptr<Texture> m_floorTexture;
	std::unique_ptr<Texture> m_wallTexture;
	std::unique_ptr<Texture> m_ceilingTexture;
	std::unique_ptr<Mesh> m_cubeMesh;
	std::vector<WorldCube> m_cubes;
	std::vector<AABB> m_colliders;
	std::unique_ptr<Model> m_stairModel;
	glm::mat4 m_stairTransform{ 1.0F };
};
