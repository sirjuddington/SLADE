#pragma once

#include "ExtraFloor.h"

namespace slade::map
{
class MapSpecials;
enum class SectorSurfaceType : u8;

class ExtraFloorSpecials
{
public:
	ExtraFloorSpecials(SLADEMap& map, MapSpecials& map_specials);
	~ExtraFloorSpecials() = default;

	bool                      hasExtraFloors(const MapSector& sector) const;
	const vector<ExtraFloor>& extraFloors(const MapSector& sector);

	void processLineSpecial(const MapLine& line);
	void clearSpecials();

	void updateSectorExtraFloors(const MapSector& sector);
	void updateOutdatedSectorExtraFloors();

	bool lineUpdated(const MapLine& line, bool update_outdated = true);
	bool sideUpdated(const MapSide& side, bool update_outdated = true);
	bool sectorUpdated(const MapSector& sector, bool update_outdated = true);

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

	SLADEMap*                        map_          = nullptr;
	MapSpecials*                     map_specials_ = nullptr;
	vector<SectorExtraFloors>        sector_extra_floors_;
	vector<Set3dFloorSpecial>        set_3d_floor_specials_;
	mutable vector<const MapSector*> sectors_to_update_;
	mutable bool                     specials_updated_ = false;

	SectorExtraFloors* getSectorExtraFloors(const MapSector& sector);

	void clearExtraFloors(const MapSector& sector);
	void addExtraFloor(SectorExtraFloors& sef, const ExtraFloor& extra_floor);
	void removeExtraFloor(SectorExtraFloors& sef, const MapLine& control_line);

	void addSet3dFloorSpecial(const MapLine& line);
	void removeSet3dFloorSpecial(const MapLine& line);
	void applySet3dFloorSpecial(const Set3dFloorSpecial& special, SectorExtraFloors& sef);
};
} // namespace slade::map
