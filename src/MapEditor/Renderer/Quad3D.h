#pragma once

namespace slade::mapeditor
{
struct Quad3D
{
	// Origin
	const MapSide* side = nullptr;

	// Vertex buffer info (count is always 6)
	unsigned vertex_offset = 0;

	// Render info
	glm::vec4 colour       = glm::vec4{ 1.0f };
	glm::vec3 normal       = glm::vec3{ 0.0f, 0.0f, 1.0f };
	unsigned  texture      = 0;
	long      updated_time = 0;
};
} // namespace slade::mapeditor
