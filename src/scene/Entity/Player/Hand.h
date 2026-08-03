#pragma once

#include "rendering/SkeletalModel.h"
#include "scene/Animation/Animator.h"

#include <glm/glm.hpp>

#include <memory>

class Camera;
class Pistol;
class Renderer;
class Shader;
struct Light;

class Hand
{
public:
	Hand();
	~Hand();

	void update(float deltaTime);

	void render(
		const Renderer& renderer,
		const Shader& shader,
		const Camera& camera,
		const Light& light,
		const Pistol& pistol) const;

private:
	std::unique_ptr<SkeletalModel> m_model;
	std::unique_ptr<Animator> m_animator;

	// Offset/rotation/scale relative to the pistol's own view transform,
	// tuned so the hand mesh wraps around the grip.
	glm::vec3 m_gripOffset;
	glm::vec3 m_gripRotationDegrees;
	float m_gripScale;
};
