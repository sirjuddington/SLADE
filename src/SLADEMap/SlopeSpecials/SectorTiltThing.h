#pragma once

#include "SlopeSpecialThing.h"

namespace slade
{
struct SectorTiltThing : SlopeSpecialThing
{
	SectorTiltThing(SectorSurfaceType surface_type);

	void apply() override;
};
} // namespace slade
