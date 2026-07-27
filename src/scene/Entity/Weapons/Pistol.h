#pragma once

#include "scene/Entity/Weapons/Weapon.h"
#include "rendering/Model.h"

#include <glm/glm.hpp>

#include <memory>

class InputManager;
class Camera;

class Pistol : public Weapon
{
public:
	Pistol();
	~Pistol() override;

	void render(
		const Renderer& renderer,
		const Shader& shader,
		const Camera& camera,
		const Light& light) const override;

	// World-space position/direction the muzzle is currently pointing,
	// derived from the camera and the same view-relative offset/rotation
	// used to render the weapon. Used to spawn projectiles/particles from
	// the correct on-screen location.
	[[nodiscard]] glm::vec3 muzzleWorldPosition(const Camera& camera) const;
	[[nodiscard]] glm::vec3 muzzleWorldDirection(const Camera& camera) const;

	// Temporary debug helper: lets you tune scale/offset live without
	// recompiling, since getting a downloaded model's scale/orientation
	// right on the first try is mostly guesswork.
	void debugAdjust(const InputManager& input, float deltaTime);

private:
	std::unique_ptr<Model> m_model;

	glm::vec3 m_viewOffset;
	glm::vec3 m_viewRotationDegrees;
	float m_viewScale;

	// Offset from m_viewOffset (in the same camera-local space) to the tip
	// of the barrel, used as the projectile spawn point.
	glm::vec3 m_muzzleLocalOffset;
};