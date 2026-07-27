#pragma once

#include <glm/glm.hpp>

struct Bullet
{
	glm::vec3 position;
	glm::vec3 direction;
	float speed = 80.0F;
	float distanceTraveled = 0.0F;
	float maxDistance = 50.0F;
	bool alive = true;
};