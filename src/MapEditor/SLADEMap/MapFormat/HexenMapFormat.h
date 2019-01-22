#pragma once

#include "DoomMapFormat.h"

class HexenMapFormat : public DoomMapFormat
{
public:
	struct LineDef
	{
		uint16_t vertex1;
		uint16_t vertex2;
		uint16_t flags;
		uint8_t  type;
		uint8_t  args[5];
		uint16_t side1;
		uint16_t side2;
	};

	struct Thing
	{
		short   tid;
		short   x;
		short   y;
		short   z;
		short   angle;
		short   type;
		short   flags;
		uint8_t special;
		uint8_t args[5];
	};

protected:
	bool readLINEDEFS(ArchiveEntry* entry, MapObjectCollection& map_data) const override;
	bool readTHINGS(ArchiveEntry* entry, MapObjectCollection& map_data) const override;

	ArchiveEntry::UPtr writeLINEDEFS(const LineList& lines) const override;
	ArchiveEntry::UPtr writeTHINGS(const ThingList& things) const override;
};
