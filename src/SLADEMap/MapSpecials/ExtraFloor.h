#pragma once

#include "Geometry/Plane.h"

namespace slade::map
{
struct ExtraFloor
{
	enum class Flags : u16
	{
		DisableLighting      = 1 << 0,
		LightingInsideOnly   = 1 << 1,
		InnerFogEffect       = 1 << 2,
		FlatAtCeiling        = 1 << 3,
		UseUpperTexture      = 1 << 4,
		UseLowerTexture      = 1 << 5,
		AdditiveTransparency = 1 << 6,
		Solid                = 1 << 7,
		DrawInside           = 1 << 8,

		// Normal ExtraFloors use the control sector's ceiling for the top
		// and floor for the bottom. This flag reverses that (eg. for Vavoom
		// 3d floors)
		Flipped = 1 << 9,
	};

	int              height         = 0; // Ceiling height of the control sector, used for sorting
	Plane            plane_top      = { 0., 0., 1., 0. };
	Plane            plane_bottom   = { 0., 0., 1., 0. };
	const MapSector* control_sector = nullptr;
	const MapLine*   control_line   = nullptr;
	u16              flags          = 0;
	float            alpha          = 1.0f;

	bool hasFlag(Flags flag) const { return flags & static_cast<u16>(flag); }
	void setFlag(Flags flag) { flags |= static_cast<u16>(flag); }
};
} // namespace slade::map
