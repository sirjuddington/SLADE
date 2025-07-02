#pragma once

namespace slade
{
struct Set3dFloorSpecial
{
	enum class Type
	{
		Vavoom    = 0,
		Solid     = 1,
		Swimmable = 2,
		NonSolid  = 3,
	};

	const MapLine* line;
	MapSector*     target;
	MapSector*     control_sector;
	Type           type;
	bool           render_inside;
};
} // namespace slade
