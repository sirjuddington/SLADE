#pragma once

#include "RenderPass.h"

namespace slade::map
{
enum class SidePart : u8;
}

namespace slade::mapeditor
{
struct Quad3D
{
	enum class Flags : u8
	{
		ExtraFloor = 1 << 0,
		Additive   = 1 << 1,
		BackSide   = 1 << 2,
	};

	// Origin
	const MapSide* side = nullptr;
	map::SidePart  part;

	// Vertex buffer info (count is always 6)
	unsigned vertex_offset = 0;

	// Render info
	float      height[4]; // Heights of each vertex (TL, BL, BR, TR)
	float      brightness   = 1.0f;
	glm::vec4  colour       = glm::vec4{ 1.0f };
	glm::vec3  normal       = glm::vec3{ 0.0f, 0.0f, 1.0f };
	unsigned   texture      = 0;
	long       updated_time = 0;
	u8         flags        = 0;
	RenderPass render_pass  = RenderPass::Normal;

	bool hasFlag(Flags flag) const { return flags & static_cast<u8>(flag); }
	void setFlag(Flags flag) { flags |= static_cast<u8>(flag); }
	bool operator==(const Quad3D& other) const
	{
		return texture == other.texture && colour == other.colour && render_pass == other.render_pass
			   && hasFlag(Flags::Additive) == other.hasFlag(Flags::Additive);
	}
	bool operator==(int _cpp_par_) const;
};
} // namespace slade::mapeditor
