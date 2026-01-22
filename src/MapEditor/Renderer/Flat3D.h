#pragma once

#include "SLADEMap/Types.h"

namespace slade::mapeditor
{
struct Flat3D
{
	// Origin
	const MapSector*  sector         = nullptr; // For sector geometry (vertices/polygon)
	const MapSector*  control_sector = nullptr; // For height, texture and colour info (if nullptr use sector)
	SectorSurfaceType surface_type;             // How this flat is rendered
	SectorSurfaceType source_surface;           // What surface of the (control) sector is used for height/plane
	SectorSurfaceType source_tex;               // What surface of the (control) sector is used for texture/lighting

	// Index into flats vertex buffer
	// (vertex count is always sector polygon vertex count)
	unsigned vertex_offset = 0;

	// Render info
	glm::vec4 colour  = glm::vec4{ 1.0f };
	glm::vec3 normal  = glm::vec3{ 0.0f, 0.0f, 1.0f };
	unsigned  texture = 0;
	bool      sky     = false;

	long updated_time = 0;

	Flat3D() = default;
	Flat3D(const MapSector* sector, SectorSurfaceType surface_type) :
		sector{ sector },
		surface_type{ surface_type },
		source_surface{ surface_type },
		source_tex{ surface_type }
	{
	}
	Flat3D(
		const MapSector*  sector,
		SectorSurfaceType surface_type,
		const MapSector*  control_sector,
		SectorSurfaceType source_surface,
		SectorSurfaceType source_tex) :
		sector{ sector },
		control_sector{ control_sector },
		surface_type{ surface_type },
		source_surface{ source_surface },
		source_tex{ source_tex }
	{
	}

	const MapSector* controlSector() const { return control_sector ? control_sector : sector; }
};
} // namespace slade::mapeditor
