#pragma once
#include <glm/vec3.hpp>

namespace slade
{
struct Ray
{
	glm::vec3 origin_3d;
	glm::vec2 origin_2d;
	glm::vec3 dir_3d;
	glm::vec2 dir_2d;

	Ray(const glm::vec3& origin, const glm::vec3& dir) :
		origin_3d{ origin },
		origin_2d{ origin.xy() },
		dir_3d{ dir },
		dir_2d{ dir.xy() }
	{
	}
};
} // namespace slade
