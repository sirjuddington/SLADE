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
	const MapSector* sector         = nullptr; // For sector geometry (vertices/polygon)
	const MapSector* control_sector = nullptr; // For height, texture and colour info (if nullptr use sector)
	SurfaceType      surface_type;             // How this flat is rendered
	SurfaceType      source_surface;           // What surface of the (control) sector is used for height/plane
	SurfaceType      source_tex;               // What surface of the (control) sector is used for texture/lighting

	// Index into flats vertex buffer
	// (vertex count is always sector polygon vertex count)
	unsigned vertex_offset = 0;

	// Render info
	unsigned  texture = 0;
	u8        flags   = 0;
	glm::vec4 colour{ 1.0f };
	float     brightness = 1.0f;

	long updated_time = 0;

	Flat3D() = default;
	Flat3D(const MapSector* sector, SurfaceType surface_type) :
		sector{ sector },
		surface_type{ surface_type },
		source_surface{ surface_type },
		source_tex{ surface_type }
	{
	}
	Flat3D(
		const MapSector* sector,
		SurfaceType      surface_type,
		const MapSector* control_sector,
		SurfaceType      source_surface,
		SurfaceType      source_tex) :
		sector{ sector },
		control_sector{ control_sector },
		surface_type{ surface_type },
		source_surface{ source_surface },
		source_tex{ source_tex }
	{
	}

	const MapSector* controlSector() const { return control_sector ? control_sector : sector; }
	bool             hasFlag(Flags flag) const { return flags & static_cast<u8>(flag); }
	void             setFlag(Flags flag) { flags |= static_cast<u8>(flag); }
};
} // namespace slade::mapeditor
