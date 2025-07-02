#pragma once

#include "SlopeSpecial.h"

namespace slade
{
class MapLine;

struct PlaneAlignSpecial : SlopeSpecial
{
	const MapLine*   line  = nullptr;
	const MapSector* model = nullptr;

	PlaneAlignSpecial(const MapLine& line, SectorSurfaceType surface_type, int where);

	void apply() override;
};
} // namespace slade
