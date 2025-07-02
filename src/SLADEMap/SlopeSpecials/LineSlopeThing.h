#pragma once

#include "SlopeSpecialThing.h"

namespace slade
{
struct LineSlopeThing : SlopeSpecialThing
{
	const MapLine*   line              = nullptr;
	const MapSector* containing_sector = nullptr;

	LineSlopeThing(SectorSurfaceType surface_type);

	void apply() override;
};
} // namespace slade
