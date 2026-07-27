#pragma once

#include "rendering/Model.h"

#include <glm/glm.hpp>

#include <memory>

class Camera;
class Pistol;
class Renderer;
class Shader;
class InputManager;
struct Light;

class Hand
{
public:
	Hand();
	~Hand();

	void render(
		const Renderer& renderer,
		const Shader& shader,
		const Camera& camera,
		const Light& light,
		const Pistol& pistol) const;

	// Temporary debug helper, mirroring Pistol::debugAdjust, so the grip
	// offset/rotation/scale can be tuned live instead of guessed blindly.
	void debugAdjust(const InputManager& input, float deltaTime);

private:
	std::unique_ptr<Model> m_model;

	// Offset/rotation/scale relative to the pistol's own view transform,
	// tuned so the hand mesh wraps around the grip.
	glm::vec3 m_gripOffset;
	glm::vec3 m_gripRotationDegrees;
	float m_gripScale;
};