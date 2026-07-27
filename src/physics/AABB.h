#pragma once

#include <glm/glm.hpp>

struct AABB
{
	glm::vec3 min;
	glm::vec3 max;

	[[nodiscard]] static AABB fromCenterHalfExtents(
		const glm::vec3& center,
		const glm::vec3& halfExtents)
	{
		return AABB{ center - halfExtents, center + halfExtents };
	}

	[[nodiscard]] bool intersects(const AABB& other) const
	{
		return (min.x <= other.max.x && max.x >= other.min.x) &&
			   (min.y <= other.max.y && max.y >= other.min.y) &&
			   (min.z <= other.max.z && max.z >= other.min.z);
	}

	[[nodiscard]] AABB translated(const glm::vec3& offset) const
	{
		return AABB{ min + offset, max + offset };
	}
};