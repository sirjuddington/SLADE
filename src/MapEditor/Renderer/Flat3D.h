#pragma once

namespace slade::mapeditor
{
struct Flat3D
{
	// Origin
	const MapSector* sector = nullptr;
	// const MapSector* control_sector = nullptr; // Maybe for 3d floors?
	bool ceiling = false;

	// Vertex buffer info
	unsigned vertex_offset = 0;
	unsigned vertex_count  = 0;

	// Render info
	glm::vec4 colour  = glm::vec4{ 1.0f };
	glm::vec3 normal  = glm::vec3{ 0.0f, 0.0f, 1.0f };
	unsigned  texture = 0;

	long updated_time = 0;
};
} // namespace slade::mapeditor
