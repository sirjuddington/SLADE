#pragma once

#include "SlopeSpecial.h"

namespace slade
{
class MapLine;

struct PlaneCopySpecial : SlopeSpecial
{
	const MapLine*   line  = nullptr;
	const MapSector* model = nullptr;

	PlaneCopySpecial(SectorSurfaceType surface_type);

	void apply() override;
};
} // namespace slade
