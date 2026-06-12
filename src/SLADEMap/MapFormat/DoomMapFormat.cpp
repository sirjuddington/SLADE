
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    DoomMapFormat.cpp
// Description: MapFormatHandler specialization to handle Doom format maps
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "DoomMapFormat.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/MapDesc.h"
#include "Game/Configuration.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapSide.h"
#include "SLADEMap/MapObject/MapThing.h"
#include "SLADEMap/MapObject/MapVertex.h"
#include "SLADEMap/MapObjectCollection.h"
#include "SLADEMap/MapObjectList/LineList.h"
#include "SLADEMap/MapObjectList/SectorList.h"
#include "SLADEMap/MapObjectList/SideList.h"
#include "SLADEMap/MapObjectList/ThingList.h"
#include "SLADEMap/MapObjectList/VertexList.h"
#include "UI/UI.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Constants
//
// -----------------------------------------------------------------------------
namespace
{
constexpr size_t MAX_NAME_LEN = 8;
constexpr u16    INDEX_NONE   = 65535;
} // namespace


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Int, map_compress_sides, 1, CVar::Flag::Save) // 0 = never, 1 = when necessary, 2 = always


// -----------------------------------------------------------------------------
//
// DoomMapFormat Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Reads the given Doom-format [map], populating [map_data]
// -----------------------------------------------------------------------------
bool DoomMapFormat::readMap(MapDesc map, MapObjectCollection& map_data, PropertyList& map_extra_props)
{
	log::info(2, "Reading Doom format map");

	auto m_head = map.head.lock();
	if (!m_head)
		return false;

	// Find map entries
	ArchiveEntry* v       = nullptr;
	ArchiveEntry* si      = nullptr;
	ArchiveEntry* l       = nullptr;
	ArchiveEntry* se      = nullptr;
	ArchiveEntry* t       = nullptr;
	auto          entries = map.entries(*m_head->parent());
	for (auto entry : entries)
	{
		if (!v && entry->name() == "VERTEXES")
			v = entry;
		else if (!si && entry->name() == "SIDEDEFS")
			si = entry;
		else if (!l && entry->name() == "LINEDEFS")
			l = entry;
		else if (!se && entry->name() == "SECTORS")
			se = entry;
		else if (!t && entry->name() == "THINGS")
			t = entry;
	}

	// ---- Read vertices ----
	ui::setSplashProgressMessage("Reading Vertices");
	ui::setSplashProgress(0.0f);
	if (!readVERTEXES(v, map_data))
		return false;

	// ---- Read sectors ----
	ui::setSplashProgressMessage("Reading Sectors");
	ui::setSplashProgress(0.2f);
	if (!readSECTORS(se, map_data))
		return false;

	// ---- Read sides ----
	ui::setSplashProgressMessage("Reading Sides");
	ui::setSplashProgress(0.4f);
	if (!readSIDEDEFS(si, map_data))
		return false;

	// ---- Read lines ----
	ui::setSplashProgressMessage("Reading Lines");
	ui::setSplashProgress(0.6f);
	if (!readLINEDEFS(l, map_data))
		return false;

	// ---- Read things ----
	ui::setSplashProgressMessage("Reading Things");
	ui::setSplashProgress(0.8f);
	if (!readTHINGS(t, map_data))
		return false;

	ui::setSplashProgressMessage("Init Map Data");
	ui::setSplashProgress(1.0f);

	return true;
}

// -----------------------------------------------------------------------------
// Writes the given [map_data] to Doom format, returning the list of entries
// making up the map
// -----------------------------------------------------------------------------
vector<unique_ptr<ArchiveEntry>> DoomMapFormat::writeMap(
	const MapObjectCollection& map_data,
	const PropertyList&        map_extra_props)
{
	// Build doom format map data
	auto vertexes = buildVertices(map_data.vertices());
	auto sidedefs = buildSides(map_data.sides());
	auto linedefs = buildLines(map_data.lines());
	auto sectors  = buildSectors(map_data.sectors());
	auto things   = buildThings(map_data.things());

	// Check if we need to compress sides
	if (map_compress_sides == 2 || map_compress_sides == 1 && sidedefs.size() > 65535)
	{
		// Compress sides & update linedefs with compressed side indices
		auto side_index_map = compressSides(sidedefs);
		remapLineSides(linedefs, map_data.lines(), side_index_map);
	}

	// TODO: Check for exceeded limits and abort with error if so

	// Create map data entries
	vector<unique_ptr<ArchiveEntry>> map_entries;
	map_entries.push_back(std::make_unique<ArchiveEntry>("THINGS", things.size() * sizeof(Thing), things.data()));
	map_entries.push_back(
		std::make_unique<ArchiveEntry>("LINEDEFS", linedefs.size() * sizeof(LineDef), linedefs.data()));
	map_entries.push_back(
		std::make_unique<ArchiveEntry>("SIDEDEFS", sidedefs.size() * sizeof(SideDef), sidedefs.data()));
	map_entries.push_back(
		std::make_unique<ArchiveEntry>("VERTEXES", vertexes.size() * sizeof(Vertex), vertexes.data()));
	map_entries.push_back(std::make_unique<ArchiveEntry>("SECTORS", sectors.size() * sizeof(Sector), sectors.data()));

	return map_entries;
}

// -----------------------------------------------------------------------------
// Reads Doom-format VERTEXES data from [entry] into [map_data]
// -----------------------------------------------------------------------------
bool DoomMapFormat::readVERTEXES(ArchiveEntry* entry, MapObjectCollection& map_data) const
{
	if (!entry)
	{
		global::error = "Map has no VERTEXES entry!";
		log::info(global::error);
		return false;
	}

	// Check for empty entry
	if (entry->size() < sizeof(Vertex))
	{
		log::info(3, "Read 0 vertices");
		return true;
	}

	auto     vert_data = reinterpret_cast<const Vertex*>(entry->rawData());
	unsigned nv        = entry->size() / sizeof(Vertex);
	float    p         = ui::getSplashProgress();
	for (size_t a = 0; a < nv; a++)
	{
		ui::setSplashProgress(p + (static_cast<float>(a) / nv) * 0.2f);
		map_data.addVertex(
			std::make_unique<MapVertex>(
				Vec2d{ static_cast<double>(vert_data[a].x), static_cast<double>(vert_data[a].y) }));
	}

	log::info(3, "Read {} vertices", map_data.vertices().size());

	return true;
}

// -----------------------------------------------------------------------------
// Reads Doom-format SIDEDEFS data from [entry] into [map_data]
// -----------------------------------------------------------------------------
bool DoomMapFormat::readSIDEDEFS(ArchiveEntry* entry, MapObjectCollection& map_data) const
{
	if (!entry)
	{
		global::error = "Map has no SIDEDEFS entry!";
		log::info(global::error);
		return false;
	}

	// Check for empty entry
	if (entry->size() < sizeof(SideDef))
	{
		log::info(3, "Read 0 sides");
		return true;
	}

	auto     side_data = reinterpret_cast<const SideDef*>(entry->rawData());
	unsigned ns        = entry->size() / sizeof(SideDef);
	float    p         = ui::getSplashProgress();
	for (size_t a = 0; a < ns; a++)
	{
		ui::setSplashProgress(p + (static_cast<float>(a) / ns) * 0.2f);

		// Add side
		map_data.addSide(
			std::make_unique<MapSide>(
				map_data.sectors().at(side_data[a].sector),
				strutil::viewFromChars(side_data[a].tex_upper, 8),
				strutil::viewFromChars(side_data[a].tex_middle, 8),
				strutil::viewFromChars(side_data[a].tex_lower, 8),
				Vec2i{ side_data[a].x_offset, side_data[a].y_offset }));
	}

	log::info(3, "Read {} sides", map_data.sides().size());

	return true;
}

// -----------------------------------------------------------------------------
// Reads Doom-format LINEDEFS data from [entry] into [map_data]
// -----------------------------------------------------------------------------
bool DoomMapFormat::readLINEDEFS(ArchiveEntry* entry, MapObjectCollection& map_data) const
{
	if (!entry)
	{
		global::error = "Map has no LINEDEFS entry!";
		log::info(global::error);
		return false;
	}

	// Check for empty entry
	if (entry->size() < sizeof(LineDef))
	{
		log::info(3, "Read 0 lines");
		return true;
	}

	auto     line_data = reinterpret_cast<const LineDef*>(entry->rawData());
	unsigned nl        = entry->size() / sizeof(LineDef);
	float    p         = ui::getSplashProgress();
	for (size_t a = 0; a < nl; a++)
	{
		ui::setSplashProgress(p + (static_cast<float>(a) / nl) * 0.2f);
		const auto& data = line_data[a];

		// Check vertices exist
		auto v1 = map_data.vertices().at(data.vertex1);
		auto v2 = map_data.vertices().at(data.vertex2);
		if (!v1 || !v2)
		{
			log::warning("Line {} invalid, not added", a);
			continue;
		}

		// Get side indices
		int  s1_index = data.side1;
		int  s2_index = data.side2;
		bool no_s2    = false;
		if (map_data.sides().size() > 32767)
		{
			// Support for > 32768 sides
			if (data.side1 != 65535)
				s1_index = static_cast<unsigned short>(data.side1);
			if (data.side2 != 65535)
				s2_index = static_cast<unsigned short>(data.side2);
			else
				no_s2 = true; // No second side if data.side2 == 65535
		}

		// Copy side(s) if they already have parent lines (compressed sidedefs)
		auto s1 = map_data.sides().at(s1_index);
		auto s2 = no_s2 ? nullptr : map_data.sides().at(s2_index);
		if (s1 && s1->parentLine())
			s1 = map_data.addSide(std::make_unique<MapSide>(s1->sector(), s1));
		if (s2 && s2->parentLine())
			s2 = map_data.addSide(std::make_unique<MapSide>(s2->sector(), s2));

		// Create line
		auto line = map_data.addLine(std::make_unique<MapLine>(v1, v2, s1, s2, data.type, data.flags));

		// Set properties
		line->setArg(0, data.sector_tag);
		line->setId(data.sector_tag);
	}

	log::info(3, "Read {} lines", map_data.lines().size());

	return true;
}

// -----------------------------------------------------------------------------
// Reads Doom-format SECTORS data from [entry] into [map_data]
// -----------------------------------------------------------------------------
bool DoomMapFormat::readSECTORS(ArchiveEntry* entry, MapObjectCollection& map_data) const
{
	if (!entry)
	{
		global::error = "Map has no SECTORS entry!";
		log::info(global::error);
		return false;
	}

	// Check for empty entry
	if (entry->size() < sizeof(Sector))
	{
		log::info(3, "Read 0 sectors");
		return true;
	}

	auto     sect_data = reinterpret_cast<const Sector*>(entry->rawData());
	unsigned ns        = entry->size() / sizeof(Sector);
	float    p         = ui::getSplashProgress();
	for (size_t a = 0; a < ns; a++)
	{
		ui::setSplashProgress(p + (static_cast<float>(a) / ns) * 0.2f);
		const auto& data = sect_data[a];

		// Add sector
		map_data.addSector(
			std::make_unique<MapSector>(
				data.f_height,
				strutil::viewFromChars(data.f_tex, 8),
				data.c_height,
				strutil::viewFromChars(data.c_tex, 8),
				data.light,
				data.special,
				data.tag));
	}

	log::info(3, "Read {} sectors", map_data.sectors().size());

	return true;
}

// -----------------------------------------------------------------------------
// Reads Doom-format THINGS data from [entry] into [map_data]
// -----------------------------------------------------------------------------
bool DoomMapFormat::readTHINGS(ArchiveEntry* entry, MapObjectCollection& map_data) const
{
	if (!entry)
	{
		global::error = "Map has no THINGS entry!";
		log::info(global::error);
		return false;
	}

	// Check for empty entry
	if (entry->size() < sizeof(Thing))
	{
		log::info(3, "Read 0 things");
		return true;
	}

	auto     thng_data = reinterpret_cast<const Thing*>(entry->rawData());
	unsigned nt        = entry->size() / sizeof(Thing);
	float    p         = ui::getSplashProgress();
	for (size_t a = 0; a < nt; a++)
	{
		ui::setSplashProgress(p + (static_cast<float>(a) / nt) * 0.2f);
		MapThing* thing = map_data.addThing(
			std::make_unique<MapThing>(
				Vec3d{ static_cast<double>(thng_data[a].x), static_cast<double>(thng_data[a].y), 0. },
				thng_data[a].type,
				thng_data[a].angle,
				thng_data[a].flags));

		if (game::configuration().currentGame() == "srb2") // Sonic robo blast 2
		{
			// Srb2 stores thing's z position at the upper 12-bit from the thing's flags
			thing->setZ(static_cast<unsigned>(thng_data[a].flags >> 4));
		}
	}

	log::info(3, "Read {} things", map_data.things().size());

	return true;
}

// -----------------------------------------------------------------------------
// Builds a vector of Doom-format Vertices from the given [vertices]
// -----------------------------------------------------------------------------
vector<DoomMapFormat::Vertex> DoomMapFormat::buildVertices(const VertexList& vertices) const
{
	vector<Vertex> data;
	data.reserve(vertices.size());

	for (auto& vertex : vertices)
		data.push_back({ .x = static_cast<short>(vertex->xPos()), .y = static_cast<short>(vertex->yPos()) });

	return data;
}

// -----------------------------------------------------------------------------
// Builds a vector of Doom-format SideDefs from the given [sides]
// -----------------------------------------------------------------------------
vector<DoomMapFormat::SideDef> DoomMapFormat::buildSides(const SideList& sides) const
{
	vector<SideDef> data;
	data.reserve(sides.size());

	for (auto& side : sides)
	{
		SideDef sdef{ .x_offset   = side->texOffsetX(),
					  .y_offset   = side->texOffsetY(),
					  .tex_upper  = {},
					  .tex_lower  = {},
					  .tex_middle = {},
					  .sector     = side->sector() ? static_cast<u16>(side->sector()->index()) : INDEX_NONE };
		memcpy(sdef.tex_upper, side->texUpper().data(), std::min(side->texUpper().size(), MAX_NAME_LEN));
		memcpy(sdef.tex_lower, side->texLower().data(), std::min(side->texLower().size(), MAX_NAME_LEN));
		memcpy(sdef.tex_middle, side->texMiddle().data(), std::min(side->texMiddle().size(), MAX_NAME_LEN));
		data.push_back(sdef);
	}

	return data;
}

// -----------------------------------------------------------------------------
// Builds a vector of Doom-format LineDefs from the given [lines]
// -----------------------------------------------------------------------------
vector<DoomMapFormat::LineDef> DoomMapFormat::buildLines(const LineList& lines) const
{
	vector<LineDef> data;
	data.reserve(lines.size());

	for (auto& line : lines)
	{
		data.push_back(
			{ .vertex1    = static_cast<u16>(line->v1Index()),
			  .vertex2    = static_cast<u16>(line->v2Index()),
			  .flags      = static_cast<u16>(line->flags()),
			  .type       = static_cast<u16>(line->special()),
			  .sector_tag = static_cast<u16>(line->arg(0)),
			  .side1      = static_cast<u16>(line->s1Index()),
			  .side2      = static_cast<u16>(line->s2Index()) });
	}

	return data;
}

// -----------------------------------------------------------------------------
// Builds a vector of Doom-format Sectors from the given [sectors]
// -----------------------------------------------------------------------------
vector<DoomMapFormat::Sector> DoomMapFormat::buildSectors(const SectorList& sectors) const
{
	vector<Sector> data;
	data.reserve(sectors.size());

	for (auto& sector : sectors)
	{
		Sector s{ .f_height = static_cast<short>(sector->floor().height),
				  .c_height = static_cast<short>(sector->ceiling().height),
				  .f_tex    = {},
				  .c_tex    = {},
				  .light    = sector->lightLevel(),
				  .special  = sector->special(),
				  .tag      = sector->id() };
		memcpy(s.f_tex, sector->floor().texture.data(), std::min(sector->floor().texture.size(), MAX_NAME_LEN));
		memcpy(s.c_tex, sector->ceiling().texture.data(), std::min(sector->ceiling().texture.size(), MAX_NAME_LEN));
		data.push_back(s);
	}

	return data;
}

// -----------------------------------------------------------------------------
// Builds a vector of Doom-format Things from the given [things]
// -----------------------------------------------------------------------------
vector<DoomMapFormat::Thing> DoomMapFormat::buildThings(const ThingList& things) const
{
	vector<Thing> data;
	data.reserve(things.size());

	for (auto& thing : things)
	{
		Thing t{ .x     = static_cast<short>(thing->xPos()),
				 .y     = static_cast<short>(thing->yPos()),
				 .angle = thing->angle(),
				 .type  = static_cast<u16>(thing->type()),
				 .flags = static_cast<u16>(thing->flags()) };

		// Sonic robo blast 2
		if (game::configuration().currentGame() == "srb2")
		{
			// Srb2 stores thing's z position at the upper 12 bits from the thing's flags
			t.flags = (t.flags & 0xf) | (static_cast<unsigned>(thing->zPos()) << 4);
		}

		data.push_back(t);
	}

	return data;
}

// -----------------------------------------------------------------------------
// Compresses the given [sides] by removing duplicates, returning a map of old
// side indices to new compressed indices
// -----------------------------------------------------------------------------
std::unordered_map<unsigned, u16> DoomMapFormat::compressSides(vector<SideDef>& sides) const
{
	// Hash and equality functors for SideDef struct
	struct SideHash
	{
		size_t operator()(const SideDef& s) const
		{
			// FNV-1a over the struct bytes
			// (see: http://www.isthe.com/chongo/tech/comp/fnv/)
			const auto* p = reinterpret_cast<const u8*>(&s);
			size_t      h = 14695981039346656037ull;
			for (size_t i = 0; i < sizeof(SideDef); ++i)
				h = (h ^ p[i]) * 1099511628211ull;
			return h;
		}
	};
	struct SideEqual
	{
		bool operator()(const SideDef& a, const SideDef& b) const { return memcmp(&a, &b, sizeof(SideDef)) == 0; }
	};

	std::unordered_map<unsigned, u16>                     index_map;
	std::unordered_map<SideDef, u16, SideHash, SideEqual> lookup;
	vector<SideDef>                                       compressed;

	index_map.reserve(sides.size());
	lookup.reserve(sides.size());
	compressed.reserve(sides.size());

	// Build lookup of unique sides and map of old side indices to new
	// compressed indices
	for (unsigned i = 0; i < sides.size(); ++i)
	{
		// Insert side into lookup, get compressed index
		auto [it, inserted] = lookup.emplace(sides[i], static_cast<u16>(compressed.size()));

		// If side was unique, add to compressed list
		if (inserted)
			compressed.push_back(sides[i]);

		index_map[i] = it->second;
	}

	sides = std::move(compressed);

	return index_map;
}
