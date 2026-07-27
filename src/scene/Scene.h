#pragma once

#include "physics/AABB.h"
#include "rendering/Camera.h"
#include "rendering/Light.h"
#include "rendering/Mesh.h"
#include "rendering/Renderer.h"
#include "rendering/Shader.h"
#include "rendering/Texture.h"

#include <glm/glm.hpp>
#include <vector>
#include <memory>

class Scene
{
public:
	// Doorway geometry, exposed so other systems (e.g. Door) can align
	// exactly with the opening left in buildEnvironment() without
	// duplicating magic numbers.
	static constexpr float DOORWAY_HALF_WIDTH = 0.5F;
	static constexpr float DOORWAY_HEIGHT = 2.0F;
	static constexpr float WALL_THICKNESS = 0.1F;
	static constexpr float FLOOR_TOP_Y = -0.45F; // floor collider top surface

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
	enum class SurfaceType
	{
		Floor,
		Wall,
		Ceiling
	};
	struct CubeInstance
	{
		glm::vec3 position;
		glm::vec3 scale;
		SurfaceType surface;
	};

	void buildEnvironment();

	std::unique_ptr<Texture> m_floorTexture;
	std::unique_ptr<Texture> m_wallTexture;
	std::unique_ptr<Texture> m_ceilingTexture;
	std::unique_ptr<Mesh> m_cubeMesh;
	std::vector<CubeInstance> m_cubes;
	std::vector<AABB> m_colliders;
};