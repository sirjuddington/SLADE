#pragma once

#include "SlopeSpecialThing.h"

namespace slade
{
struct CopySlopeThing : SlopeSpecialThing
{
	const MapSector* model = nullptr;

	CopySlopeThing(SectorSurfaceType surface_type);

	void apply() override;
};
} // namespace slade
