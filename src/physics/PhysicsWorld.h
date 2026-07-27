#pragma once

#include "physics/AABB.h"
#include <glm/glm.hpp>
#include <vector>

class PhysicsWorld
{
public:
	PhysicsWorld();

	void setColliders(std::vector<AABB> colliders);
	void setPlayerExtents(const glm::vec3& halfExtents);

	void update(
		glm::vec3& position,
		const glm::vec3& moveInput,
		float deltaTime);

	void jump();

	[[nodiscard]] bool isGround() const { return m_grounded; }

private:
	void resolveCollisions(glm::vec3& position);

	std::vector<AABB> m_colliders;
	glm::vec3 m_velocity{ 0.0f };
	glm::vec3 m_halfExtents{ 0.3f, 0.9f, 0.3f };

	float m_gravity = -9.81f;
	float m_moveSpeed = 4.0f;
	float m_jumpSpeed = 4.5f;

	bool m_grounded = false;
};