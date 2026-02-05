#pragma once

#include "RenderPass.h"
#include "SLADEMap/Types.h"

namespace slade::mapeditor
{
struct Flat3D
{
	using SurfaceType = map::SectorSurfaceType;

	enum class Flags : u8
	{
		ExtraFloor = 1 << 0,
		Additive   = 1 << 1,
	};

	// Origin
	const MapSector* sector = nullptr;
	SurfaceType      surface_type;

	// Control sector (for ExtraFloors)
	const MapSector* control_sector = nullptr; // If nullptr use sector
	SurfaceType      control_surface_type;     // The surface of the control sector we get texture/plane/etc from

	// Index into flats vertex buffer
	// (vertex count is always sector polygon vertex count)
	unsigned vertex_offset = 0;

	// Render info
	unsigned   texture = 0;
	u8         flags   = 0;
	glm::vec4  colour{ 1.0f };
	RenderPass render_pass = RenderPass::Normal;

	bool             hasFlag(Flags flag) const { return flags & static_cast<u8>(flag); }
	void             setFlag(Flags flag) { flags |= static_cast<u8>(flag); }
	const MapSector* controlSector() const { return control_sector ? control_sector : sector; }

	bool operator==(const Flat3D& other) const
	{
		return texture == other.texture && colour == other.colour && render_pass == other.render_pass
			   && hasFlag(Flags::Additive) == other.hasFlag(Flags::Additive);
	}
};
} // namespace slade::mapeditor
