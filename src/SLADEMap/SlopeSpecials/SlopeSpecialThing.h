#pragma once

#include "SlopeSpecial.h"

namespace slade
{
class MapThing;

struct SlopeSpecialThing : SlopeSpecial
{
	enum class Type : u8
	{
		Line,
		SectorTilt,
		Vavoom,
		Copy,
	};

	Type            type;
	const MapThing* thing = nullptr;

	SlopeSpecialThing(Type type, SectorSurfaceType surface_type) : SlopeSpecial{ surface_type }, type{ type } {}
};
} // namespace slade
