#pragma once

namespace OpenGL
{
struct Vertex2D
{
	glm::vec2 position;
	glm::vec4 colour;
	glm::vec2 tex_coord;

	Vertex2D(
		const glm::vec2& position,
		const glm::vec4& colour    = { 1.f, 1.f, 1.f, 1.f },
		const glm::vec2& tex_coord = { 0.f, 0.f }) :
		position{ position },
		colour{ colour },
		tex_coord{ tex_coord }
	{
	}
};
} // namespace OpenGL
