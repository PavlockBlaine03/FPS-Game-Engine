#pragma once

#include "rendering/Light.h"

class Renderer;
class Shader;
class Camera;

class Entity
{
public:
	virtual ~Entity() = default;

	virtual void render(
		const Renderer& renderer,
		const Shader& shader,
		const Camera& camera,
		const Light& light) const = 0;
};