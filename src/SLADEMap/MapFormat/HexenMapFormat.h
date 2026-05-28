#pragma once

#include "DoomMapFormat.h"

namespace slade
{
class HexenMapFormat : public DoomMapFormat
{
public:
	struct LineDef
	{
		u16 vertex1;
		u16 vertex2;
		u16 flags;
		u8  type;
		u8  args[5];
		u16 side1;
		u16 side2;
	};

	struct Thing
	{
		i16 tid;
		i16 x;
		i16 y;
		i16 z;
		i16 angle;
		i16 type;
		u16 flags;
		u8  special;
		u8  args[5];
	};

	vector<unique_ptr<ArchiveEntry>> writeMap(const MapObjectCollection& map_data, const PropertyList& map_extra_props)
		override;

protected:
	bool readLINEDEFS(ArchiveEntry* entry, MapObjectCollection& map_data) const override;
	bool readTHINGS(ArchiveEntry* entry, MapObjectCollection& map_data) const override;

	vector<LineDef> buildHexenLines(const LineList& lines) const;
	vector<Thing>   buildHexenThings(const ThingList& things) const;
};
} // namespace slade
