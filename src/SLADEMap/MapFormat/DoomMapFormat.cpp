
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
	vector<unique_ptr<ArchiveEntry>> map_entries;
	map_entries.push_back(writeTHINGS(map_data.things()));
	map_entries.push_back(writeLINEDEFS(map_data.lines()));
	map_entries.push_back(writeSIDEDEFS(map_data.sides()));
	map_entries.push_back(writeVERTEXES(map_data.vertices()));
	map_entries.push_back(writeSECTORS(map_data.sectors()));
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
		map_data.addVertex(std::make_unique<MapVertex>(
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
		map_data.addSide(std::make_unique<MapSide>(
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
		map_data.addSector(std::make_unique<MapSector>(
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
		MapThing* thing = map_data.addThing(std::make_unique<MapThing>(
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
// Creates and returns a Doom-format VERTEXES entry from [vertices]
// -----------------------------------------------------------------------------
unique_ptr<ArchiveEntry> DoomMapFormat::writeVERTEXES(const VertexList& vertices) const
{
	auto entry = std::make_unique<ArchiveEntry>("VERTEXES");

	// Init entry data
	entry->clearData();
	entry->resize(vertices.size() * 4, false);
	entry->seek(0, 0);

	// Write vertex data
	short x, y;
	for (auto& vertex : vertices)
	{
		x = vertex->xPos();
		y = vertex->yPos();
		entry->write(&x, 2);
		entry->write(&y, 2);
	}

	return entry;
}

// -----------------------------------------------------------------------------
// Creates and returns a Doom-format SIDEDEFS entry from [sides]
// -----------------------------------------------------------------------------
unique_ptr<ArchiveEntry> DoomMapFormat::writeSIDEDEFS(const SideList& sides) const
{
	auto entry = std::make_unique<ArchiveEntry>("SIDEDEFS");

	// Init entry data
	entry->clearData();
	entry->resize(sides.size() * 30, false);
	entry->seek(0, 0);

	// Write side data
	SideDef data;
	for (auto& side : sides)
	{
		memset(&data, 0, 30);

		// Offsets
		data.x_offset = side->texOffsetX();
		data.y_offset = side->texOffsetY();

		// Sector
		data.sector = -1;
		if (side->sector())
			data.sector = side->sector()->index();

		// Textures
		memcpy(data.tex_middle, side->texMiddle().data(), side->texMiddle().size());
		memcpy(data.tex_upper, side->texUpper().data(), side->texUpper().size());
		memcpy(data.tex_lower, side->texLower().data(), side->texLower().size());

		entry->write(&data, 30);
	}

	return entry;
}

// -----------------------------------------------------------------------------
// Creates and returns a Doom-format LINEDEFS entry from [lines]
// -----------------------------------------------------------------------------
unique_ptr<ArchiveEntry> DoomMapFormat::writeLINEDEFS(const LineList& lines) const
{
	auto entry = std::make_unique<ArchiveEntry>("LINEDEFS");

	// Init entry data
	entry->clearData();
	entry->resize(lines.size() * 14, false);
	entry->seek(0, 0);

	// Write line data
	LineDef data;
	for (auto& line : lines)
	{
		data.vertex1 = line->v1Index();
		data.vertex2 = line->v2Index();

		// Properties
		data.flags      = line->flags();
		data.type       = line->special();
		data.sector_tag = line->arg(0);

		// Sides
		data.side1 = line->s1Index();
		data.side2 = line->s2Index();

		entry->write(&data, 14);
	}

	return entry;
}

// -----------------------------------------------------------------------------
// Creates and returns a Doom-format SECTORS entry from [sectors]
// -----------------------------------------------------------------------------
unique_ptr<ArchiveEntry> DoomMapFormat::writeSECTORS(const SectorList& sectors) const
{
	auto entry = std::make_unique<ArchiveEntry>("SECTORS");

	// Init entry data
	entry->clearData();
	entry->resize(sectors.size() * 26, false);
	entry->seek(0, 0);

	// Write sector data
	Sector data;
	for (auto& sector : sectors)
	{
		memset(&data, 0, 26);

		// Height
		data.f_height = sector->floor().height;
		data.c_height = sector->ceiling().height;

		// Textures
		memcpy(data.f_tex, sector->floor().texture.data(), sector->floor().texture.size());
		memcpy(data.c_tex, sector->ceiling().texture.data(), sector->ceiling().texture.size());

		// Properties
		data.light   = sector->lightLevel();
		data.special = sector->special();
		data.tag     = sector->tag();

		entry->write(&data, 26);
	}

	return entry;
}

// -----------------------------------------------------------------------------
// Creates and returns a Doom-format THINGS entry from [things]
// -----------------------------------------------------------------------------
unique_ptr<ArchiveEntry> DoomMapFormat::writeTHINGS(const ThingList& things) const
{
	auto entry = std::make_unique<ArchiveEntry>("THINGS");

	// Init entry data
	entry->clearData();
	entry->resize(things.size() * 10, false);
	entry->seek(0, 0);

	// Write thing data
	Thing data;
	for (auto& thing : things)
	{
		// Position
		data.x = thing->xPos();
		data.y = thing->yPos();

		// Properties
		data.angle = thing->angle();
		data.type  = thing->type();
		data.flags = thing->flags();

		if (game::configuration().currentGame() == "srb2") // Sonic robo blast 2
		{
			// Srb2 stores thing's z position at the upper 12 bits from the thing's flags
			data.flags = (data.flags & 0xf) | (static_cast<unsigned>(thing->zPos()) << 4);
		}

		entry->write(&data, 10);
	}

	return entry;
}
