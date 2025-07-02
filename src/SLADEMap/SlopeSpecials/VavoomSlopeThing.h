#pragma once

#include "SlopeSpecialThing.h"

namespace slade
{
class MapLine;

struct VavoomSlopeThing : SlopeSpecialThing
{
	const MapLine* line = nullptr;

	VavoomSlopeThing(SectorSurfaceType surface_type);

	void apply() override;
};
} // namespace slade
