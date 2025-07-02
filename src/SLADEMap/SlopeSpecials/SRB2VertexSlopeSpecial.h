#pragma once

#include "SLADEMap/MapObject/MapLine.h"
#include "SlopeSpecial.h"

namespace slade
{
class MapThing;

struct SRB2VertexSlopeSpecial : SlopeSpecial
{
	const MapLine*  line;
	const MapThing* vertices[3];

	SRB2VertexSlopeSpecial(
		const MapLine&    line,
		MapSector&        target,
		const MapThing*   vertices[3],
		SectorSurfaceType surface_type);

	void apply() override;
};
} // namespace slade
