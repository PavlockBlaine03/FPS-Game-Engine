#pragma once

#include "rendering/Camera.h"
#include "rendering/Mesh.h"
#include "rendering/Shader.h"
#include "rendering/Texture.h"
#include "rendering/Light.h"

#include <vector>

class SkinnedMesh;

class Renderer
{
public:
	Renderer(int width, int height);

	void beginFrame() const;

	void draw(
		const Mesh& mesh,
		const Shader& shader,
		const Camera& camera,
		const glm::mat4& model,
		const Texture& texture,
		const Light& light) const;

    void drawSkinned(
        const SkinnedMesh& mesh,
        const Shader& shader,
        const Camera& camera,
        const glm::mat4& model,
        const Texture& texture,
        const Light& light,
        const std::vector<glm::mat4>& boneMatrices) const;

	void resize(int width, int height);

private:
	int m_width;
	int m_height;
};
