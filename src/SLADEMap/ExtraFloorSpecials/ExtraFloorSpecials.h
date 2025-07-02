#pragma once

#include "Geometry/Plane.h"

namespace slade
{
struct Set3dFloorSpecial;
enum class SectorSurfaceType : u8;

class ExtraFloorSpecials
{
public:
	struct ExtraFloor
	{
		Plane             plane;
		SectorSurfaceType surface_type;
	};

	ExtraFloorSpecials(SLADEMap& map);
	~ExtraFloorSpecials() = default;

	bool                      hasExtraFloors(const MapSector* sector) const;
	const vector<ExtraFloor>& extraFloors(const MapSector* sector) const;

	void processLineSpecial(const MapLine& line);

private:
	SLADEMap* map_ = nullptr;

	struct SectorExtraFloors
	{
		const MapSector*   sector;
		vector<ExtraFloor> extra_floors;
	};
	vector<SectorExtraFloors> sector_extra_floors_;

	vector<unique_ptr<Set3dFloorSpecial>> set_3d_floor_specials_;
};
} // namespace slade
