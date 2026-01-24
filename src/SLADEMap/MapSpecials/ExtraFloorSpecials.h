#pragma once

#include "ExtraFloor.h"

namespace slade::map
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
	void clearSpecials();

	void updateSectorExtraFloors(const MapSector* sector);

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

		enum class Flags : u16
		{
			None               = 0,
			DisableLighting    = 1 << 0,
			LightingInsideOnly = 1 << 1,
			Fog                = 1 << 2,
			FloorAtCeiling     = 1 << 3,
			UseUpperTexture    = 1 << 4,
			UseLowerTexture    = 1 << 5,
			TransAdd           = 1 << 6,
			Fade               = 1 << 9,
			ResetLighting      = 1 << 10,
		};

		const MapLine*   line           = nullptr;
		const MapSector* target         = nullptr;
		const MapSector* control_sector = nullptr;
		Type             type           = Type::Solid;
		bool             render_inside  = false;
		u16              flags          = 0;
		float            alpha          = 1.0f;

		bool hasFlag(Flags flag) const { return (flags & static_cast<u16>(flag)) != 0; }
	};

	SLADEMap*                 map_ = nullptr;
	vector<SectorExtraFloors> sector_extra_floors_;
	vector<Set3dFloorSpecial> set_3d_floor_specials_;

	SectorExtraFloors* getSectorExtraFloors(const MapSector* sector);

	void addExtraFloor(const MapSector* sector, const ExtraFloor& extra_floor);
	void clearExtraFloors(const MapSector* sector);

	void addSet3dFloorSpecial(const MapLine& line);
	void applySet3dFloorSpecial(const Set3dFloorSpecial& special, SectorExtraFloors& sector_extra_floors);
};
} // namespace slade::map
