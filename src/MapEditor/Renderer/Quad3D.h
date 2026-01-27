#pragma once

namespace slade::mapeditor
{
struct Quad3D
{
	enum class Flags : u8
	{
		None       = 0,
		MidTexture = 1 << 0,
		Sky        = 1 << 1,
		Additive   = 1 << 2,
	};

	// Origin
	const MapSide* side = nullptr;

	// Vertex buffer info (count is always 6)
	unsigned vertex_offset = 0;

	// Render info
	float     brightness   = 1.0f;
	glm::vec4 colour       = glm::vec4{ 1.0f };
	glm::vec3 normal       = glm::vec3{ 0.0f, 0.0f, 1.0f };
	unsigned  texture      = 0;
	long      updated_time = 0;
	u8        flags        = 0;

	bool hasFlag(Flags flag) const { return flags & static_cast<u8>(flag); }
	void setFlag(Flags flag) { flags |= static_cast<u8>(flag); }
};
} // namespace slade::mapeditor
