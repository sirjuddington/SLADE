#pragma once

#include "ExtraFloor.h"

namespace slade
{
enum class SectorSurfaceType : u8;

class ExtraFloorSpecials
{
public:
	ExtraFloorSpecials(SLADEMap& map);
	~ExtraFloorSpecials() = default;

	bool                      hasExtraFloors(const MapSector* sector) const;
	const vector<ExtraFloor>& extraFloors(const MapSector* sector) const;

	void processLineSpecial(const MapLine& line);

private:
	struct SectorExtraFloors
	{
		const MapSector*   sector;
		vector<ExtraFloor> extra_floors;
	};

	struct Set3dFloorSpecial
	{
		enum class Type : u8
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

	SLADEMap*                             map_ = nullptr;
	vector<SectorExtraFloors>             sector_extra_floors_;
	vector<unique_ptr<Set3dFloorSpecial>> set_3d_floor_specials_;

	void addExtraFloor(const MapSector* sector, const ExtraFloor& extra_floor);
	void clearExtraFloors(const MapSector* sector);

	void addSet3dFloorSpecial(const MapLine& line);
};
} // namespace slade
