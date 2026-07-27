#pragma once

#include <glm/glm.hpp>

struct Light
{
	glm::vec3 position;
	glm::vec3 color{ 1.0F, 1.0F, 1.0F };

	float ambientStrength = 0.15F;
	float specularStrength = 0.4F;
	float shininess = 32.0F;
};