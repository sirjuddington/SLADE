#pragma once

#include "MapFormatHandler.h"

class Doom64MapFormat : public MapFormatHandler
{
public:
	struct Vertex
	{
		// These are actually fixed_t
		int32_t x;
		int32_t y;
	};

	struct SideDef
	{
		short    x_offset;
		short    y_offset;
		uint16_t tex_upper;
		uint16_t tex_lower;
		uint16_t tex_middle;
		short    sector;
	};

	struct LineDef
	{
		uint16_t vertex1;
		uint16_t vertex2;
		uint32_t flags;
		uint16_t type;
		uint16_t sector_tag;
		uint16_t side1;
		uint16_t side2;
	};

	struct Sector
	{
		short    f_height;
		short    c_height;
		uint16_t f_tex;
		uint16_t c_tex;
		uint16_t color[5];
		short    special;
		short    tag;
		uint16_t flags;
	};

	struct Thing
	{
		short x;
		short y;
		short z;
		short angle;
		short type;
		short flags;
		short tid;
	};

	bool readMap(Archive::MapDesc map, MapObjectCollection& map_data, PropertyList& map_extra_props) override;
	vector<ArchiveEntry::UPtr> writeMap(const MapObjectCollection& map_data, const PropertyList& map_extra_props)
		override;

	wxString udmfNamespace() override { return ""; }

private:
	virtual bool readVERTEXES(ArchiveEntry* entry, MapObjectCollection& map_data) const;
	virtual bool readSIDEDEFS(ArchiveEntry* entry, MapObjectCollection& map_data) const;
	virtual bool readLINEDEFS(ArchiveEntry* entry, MapObjectCollection& map_data) const;
	virtual bool readSECTORS(ArchiveEntry* entry, MapObjectCollection& map_data) const;
	virtual bool readTHINGS(ArchiveEntry* entry, MapObjectCollection& map_data) const;
};
