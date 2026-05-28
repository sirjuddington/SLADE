#pragma once

#include "MapFormatHandler.h"

namespace slade
{
class ThingList;
class SectorList;
class LineList;
class SideList;
class VertexList;

class DoomMapFormat : public MapFormatHandler
{
public:
	struct Vertex
	{
		i16 x;
		i16 y;
	};

	struct SideDef
	{
		i16  x_offset;
		i16  y_offset;
		char tex_upper[8];
		char tex_lower[8];
		char tex_middle[8];
		u16  sector;
	};

	struct LineDef
	{
		u16 vertex1;
		u16 vertex2;
		u16 flags;
		u16 type;
		u16 sector_tag;
		u16 side1;
		u16 side2;
	};

	struct Sector
	{
		i16  f_height;
		i16  c_height;
		char f_tex[8];
		char c_tex[8];
		i16  light;
		i16  special;
		i16  tag;
	};

	struct Thing
	{
		i16 x;
		i16 y;
		i16 angle;
		u16 type;
		u16 flags;
	};

	bool readMap(MapDesc map, MapObjectCollection& map_data, PropertyList& map_extra_props) override;
	vector<unique_ptr<ArchiveEntry>> writeMap(const MapObjectCollection& map_data, const PropertyList& map_extra_props)
		override;

protected:
	virtual bool readVERTEXES(ArchiveEntry* entry, MapObjectCollection& map_data) const;
	virtual bool readSIDEDEFS(ArchiveEntry* entry, MapObjectCollection& map_data) const;
	virtual bool readLINEDEFS(ArchiveEntry* entry, MapObjectCollection& map_data) const;
	virtual bool readSECTORS(ArchiveEntry* entry, MapObjectCollection& map_data) const;
	virtual bool readTHINGS(ArchiveEntry* entry, MapObjectCollection& map_data) const;

	vector<Vertex>  buildVertices(const VertexList& vertices) const;
	vector<SideDef> buildSides(const SideList& sides) const;
	vector<LineDef> buildLines(const LineList& lines) const;
	vector<Sector>  buildSectors(const SectorList& sectors) const;
	vector<Thing>   buildThings(const ThingList& things) const;

	std::unordered_map<unsigned, u16> compressSides(vector<SideDef>& sides) const;

	// Remaps side1/side2 on each linedef using the compressed-side index map,
	// reading original indices from the MapLine objects to avoid u16 truncation
	template<typename TLineDef>
	static void remapLineSides(
		vector<TLineDef>&                        linedefs,
		const LineList&                          lines,
		const std::unordered_map<unsigned, u16>& side_index_map)
	{
		for (size_t i = 0; i < linedefs.size(); ++i)
		{
			const int s1      = lines[i]->s1Index();
			const int s2      = lines[i]->s2Index();
			linedefs[i].side1 = s1 >= 0 ? side_index_map.at(static_cast<unsigned>(s1)) : static_cast<u16>(65535);
			linedefs[i].side2 = s2 >= 0 ? side_index_map.at(static_cast<unsigned>(s2)) : static_cast<u16>(65535);
		}
	}
};
} // namespace slade
