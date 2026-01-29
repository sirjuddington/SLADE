#pragma once

#include "SLADEMap/Types.h"

namespace slade::mapeditor
{
struct Flat3D
{
	using SurfaceType = map::SectorSurfaceType;

	enum class Flags : u8
	{
		None       = 0,
		Sky        = 1 << 0,
		ExtraFloor = 1 << 1,
		Additive   = 1 << 2,
	};

	// Origin
	const MapSector* sector = nullptr;

	// Index into flats vertex buffer
	// (vertex count is always sector polygon vertex count)
	unsigned vertex_offset = 0;

	// Render info
	unsigned  texture = 0;
	u8        flags   = 0;
	glm::vec4 colour{ 1.0f };

	bool hasFlag(Flags flag) const { return flags & static_cast<u8>(flag); }
	void setFlag(Flags flag) { flags |= static_cast<u8>(flag); }

	bool operator==(const Flat3D& other) const
	{
		return texture == other.texture && colour == other.colour && flags == other.flags;
	}
};
} // namespace slade::mapeditor
