#pragma once

#include "scene/Entity/Weapons/Weapon.h"
#include "rendering/Model.h"

#include <glm/glm.hpp>

#include <memory>

class Camera;

class Pistol : public Weapon
{
public:
	Pistol();
	~Pistol() override;

	// Kicks off a recoil impulse; call this alongside firing.
	void triggerRecoil();

	// Starts a reload animation; call on reload input.
	//void triggerReload();

	void update(float deltaTime);

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

	[[nodiscard]] glm::mat4 viewTransform(const Camera& camera) const;

private:
	std::unique_ptr<Model> m_model;

	glm::vec3 m_viewOffset;
	glm::vec3 m_viewRotationDegrees;
	float m_viewScale;

	// Offset from m_viewOffset (in the same camera-local space) to the tip
	// of the barrel, used as the projectile spawn point.
	glm::vec3 m_muzzleLocalOffset;

	// Recoil: a simple decaying spring on rotation/position, reset to a
	// peak offset on triggerRecoil() and settled back to zero over time.
	glm::vec3 m_recoilOffset{ 0.0F };
	glm::vec3 m_recoilVelocity{ 0.0F };

	// Reload: normalized progress [0, 1] driving a canned offset/rotation
	// curve (e.g. dipping the gun down and back up).
	//float m_reloadProgress = -1.0F; // -1 = not reloading
	//bool m_isReloading = false;
};
