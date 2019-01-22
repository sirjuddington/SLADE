
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
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
#include "General/UI.h"
#include "MapEditor/SLADEMap/MapLine.h"
#include "MapEditor/SLADEMap/MapObjectCollection.h"
#include "MapEditor/SLADEMap/MapSector.h"
#include "MapEditor/SLADEMap/MapVertex.h"


// -----------------------------------------------------------------------------
//
// DoomMapFormat Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Reads the given Doom-format [map], populating [map_data]
// -----------------------------------------------------------------------------
bool DoomMapFormat::readMap(Archive::MapDesc map, MapObjectCollection& map_data, PropertyList& map_extra_props)
{
	Log::info(2, "Reading Doom format map");

	// Find map entries
	ArchiveEntry* v     = nullptr;
	ArchiveEntry* si    = nullptr;
	ArchiveEntry* l     = nullptr;
	ArchiveEntry* se    = nullptr;
	ArchiveEntry* t     = nullptr;
	ArchiveEntry* entry = map.head;
	while (entry != map.end->nextEntry())
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

		// Next entry
		entry = entry->nextEntry();
	}

	// ---- Read vertices ----
	UI::setSplashProgressMessage("Reading Vertices");
	UI::setSplashProgress(0.0f);
	if (!readVERTEXES(v, map_data))
		return false;

	// ---- Read sectors ----
	UI::setSplashProgressMessage("Reading Sectors");
	UI::setSplashProgress(0.2f);
	if (!readSECTORS(se, map_data))
		return false;

	// ---- Read sides ----
	UI::setSplashProgressMessage("Reading Sides");
	UI::setSplashProgress(0.4f);
	if (!readSIDEDEFS(si, map_data))
		return false;

	// ---- Read lines ----
	UI::setSplashProgressMessage("Reading Lines");
	UI::setSplashProgress(0.6f);
	if (!readLINEDEFS(l, map_data))
		return false;

	// ---- Read things ----
	UI::setSplashProgressMessage("Reading Things");
	UI::setSplashProgress(0.8f);
	if (!readTHINGS(t, map_data))
		return false;

	UI::setSplashProgressMessage("Init Map Data");
	UI::setSplashProgress(1.0f);

	return true;
}

// -----------------------------------------------------------------------------
// Writes the given [map_data] to Doom format, returning the list of entries
// making up the map
// -----------------------------------------------------------------------------
vector<ArchiveEntry::UPtr> DoomMapFormat::writeMap(
	const MapObjectCollection& map_data,
	const PropertyList&        map_extra_props)
{
	vector<ArchiveEntry::UPtr> map_entries;
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
		Global::error = "Map has no VERTEXES entry!";
		Log::info(Global::error);
		return false;
	}

	// Check for empty entry
	if (entry->size() < sizeof(Vertex))
	{
		Log::info(3, "Read 0 vertices");
		return true;
	}

	auto     vert_data = (Vertex*)entry->rawData(true);
	unsigned nv        = entry->size() / sizeof(Vertex);
	float    p         = UI::getSplashProgress();
	for (size_t a = 0; a < nv; a++)
	{
		UI::setSplashProgress(p + ((float)a / nv) * 0.2f);
		map_data.addVertex(std::make_unique<MapVertex>(Vec2f{ (double)vert_data[a].x, (double)vert_data[a].y }));
	}

	Log::info(3, S_FMT("Read %lu vertices", map_data.vertices().size()));

	return true;
}

// -----------------------------------------------------------------------------
// Reads Doom-format SIDEDEFS data from [entry] into [map_data]
// -----------------------------------------------------------------------------
bool DoomMapFormat::readSIDEDEFS(ArchiveEntry* entry, MapObjectCollection& map_data) const
{
	if (!entry)
	{
		Global::error = "Map has no SIDEDEFS entry!";
		Log::info(Global::error);
		return false;
	}

	// Check for empty entry
	if (entry->size() < sizeof(SideDef))
	{
		Log::info(3, "Read 0 sides");
		return true;
	}

	auto     side_data = (SideDef*)entry->rawData(true);
	unsigned ns        = entry->size() / sizeof(SideDef);
	float    p         = UI::getSplashProgress();
	for (size_t a = 0; a < ns; a++)
	{
		UI::setSplashProgress(p + ((float)a / ns) * 0.2f);

		// Add side
		map_data.addSide(std::make_unique<MapSide>(
			map_data.sectors().at(side_data[a].sector),
			wxString::FromAscii(side_data[a].tex_upper, 8),
			wxString::FromAscii(side_data[a].tex_middle, 8),
			wxString::FromAscii(side_data[a].tex_lower, 8),
			Vec2i{ side_data[a].x_offset, side_data[a].y_offset }));
	}

	Log::info(3, S_FMT("Read %lu sides", map_data.sides().size()));

	return true;
}

// -----------------------------------------------------------------------------
// Reads Doom-format LINEDEFS data from [entry] into [map_data]
// -----------------------------------------------------------------------------
bool DoomMapFormat::readLINEDEFS(ArchiveEntry* entry, MapObjectCollection& map_data) const
{
	if (!entry)
	{
		Global::error = "Map has no LINEDEFS entry!";
		Log::info(Global::error);
		return false;
	}

	// Check for empty entry
	if (entry->size() < sizeof(LineDef))
	{
		Log::info(3, "Read 0 lines");
		return true;
	}

	auto     line_data = (LineDef*)entry->rawData(true);
	unsigned nl        = entry->size() / sizeof(LineDef);
	float    p         = UI::getSplashProgress();
	for (size_t a = 0; a < nl; a++)
	{
		UI::setSplashProgress(p + ((float)a / nl) * 0.2f);
		const auto& data = line_data[a];

		// Check vertices exist
		auto v1 = map_data.vertices().at(data.vertex1);
		auto v2 = map_data.vertices().at(data.vertex2);
		if (!v1 || !v2)
		{
			Log::warning(S_FMT("Line %lu invalid, not added", a));
			continue;
		}

		// Get side indices
		int s1_index = data.side1;
		int s2_index = data.side2;
		if (map_data.sides().size() > 32767)
		{
			// Support for > 32768 sides
			if (data.side1 != 65535)
				s1_index = static_cast<unsigned short>(data.side1);
			if (data.side2 != 65535)
				s2_index = static_cast<unsigned short>(data.side2);
		}

		// Create line
		auto line = map_data.addLine(
			std::make_unique<MapLine>(v1, v2, map_data.sides().at(s1_index), map_data.sides().at(s2_index), data.type));

		// Set properties
		line->setIntProperty("arg0", data.sector_tag);
		line->setIntProperty("id", data.sector_tag);
		line->setIntProperty("flags", data.flags);
	}

	Log::info(3, S_FMT("Read %lu lines", map_data.lines().size()));

	return true;
}

// -----------------------------------------------------------------------------
// Reads Doom-format SECTORS data from [entry] into [map_data]
// -----------------------------------------------------------------------------
bool DoomMapFormat::readSECTORS(ArchiveEntry* entry, MapObjectCollection& map_data) const
{
	if (!entry)
	{
		Global::error = "Map has no SECTORS entry!";
		Log::info(Global::error);
		return false;
	}

	// Check for empty entry
	if (entry->size() < sizeof(Sector))
	{
		Log::info(3, "Read 0 sectors");
		return true;
	}

	auto     sect_data = (Sector*)entry->rawData(true);
	unsigned ns        = entry->size() / sizeof(Sector);
	float    p         = UI::getSplashProgress();
	for (size_t a = 0; a < ns; a++)
	{
		UI::setSplashProgress(p + ((float)a / ns) * 0.2f);
		const auto& data = sect_data[a];

		// Add sector
		map_data.addSector(std::make_unique<MapSector>(
			data.f_height,
			wxString::FromAscii(data.f_tex, 8),
			data.c_height,
			wxString::FromAscii(data.c_tex, 8),
			data.light,
			data.special,
			data.tag));
	}

	Log::info(3, S_FMT("Read %lu sectors", map_data.sectors().size()));

	return true;
}

// -----------------------------------------------------------------------------
// Reads Doom-format THINGS data from [entry] into [map_data]
// -----------------------------------------------------------------------------
bool DoomMapFormat::readTHINGS(ArchiveEntry* entry, MapObjectCollection& map_data) const
{
	if (!entry)
	{
		Global::error = "Map has no THINGS entry!";
		Log::info(Global::error);
		return false;
	}

	// Check for empty entry
	if (entry->size() < sizeof(Thing))
	{
		Log::info(3, "Read 0 things");
		return true;
	}

	auto     thng_data = (Thing*)entry->rawData(true);
	unsigned nt        = entry->size() / sizeof(Thing);
	float    p         = UI::getSplashProgress();
	for (size_t a = 0; a < nt; a++)
	{
		UI::setSplashProgress(p + ((float)a / nt) * 0.2f);
		map_data.addThing(std::make_unique<MapThing>(
			Vec2f{ (double)thng_data[a].x, (double)thng_data[a].y },
			thng_data[a].type,
			thng_data[a].angle,
			thng_data[a].flags));
	}

	Log::info(3, S_FMT("Read %lu things", map_data.things().size()));

	return true;
}

// -----------------------------------------------------------------------------
// Creates and returns a Doom-format VERTEXES entry from [vertices]
// -----------------------------------------------------------------------------
ArchiveEntry::UPtr DoomMapFormat::writeVERTEXES(const VertexList& vertices) const
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
ArchiveEntry::UPtr DoomMapFormat::writeSIDEDEFS(const SideList& sides) const
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
		memcpy(data.tex_middle, CHR(side->texMiddle()), side->texMiddle().Length());
		memcpy(data.tex_upper, CHR(side->texUpper()), side->texUpper().Length());
		memcpy(data.tex_lower, CHR(side->texLower()), side->texLower().Length());

		entry->write(&data, 30);
	}

	return entry;
}

// -----------------------------------------------------------------------------
// Creates and returns a Doom-format LINEDEFS entry from [lines]
// -----------------------------------------------------------------------------
ArchiveEntry::UPtr DoomMapFormat::writeLINEDEFS(const LineList& lines) const
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
		data.flags      = line->intProperty("flags");
		data.type       = line->special();
		data.sector_tag = line->intProperty("arg0");

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
ArchiveEntry::UPtr DoomMapFormat::writeSECTORS(const SectorList& sectors) const
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
		memcpy(data.f_tex, CHR(sector->floor().texture), sector->floor().texture.Length());
		memcpy(data.c_tex, CHR(sector->ceiling().texture), sector->ceiling().texture.Length());

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
ArchiveEntry::UPtr DoomMapFormat::writeTHINGS(const ThingList& things) const
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
		data.flags = thing->intProperty("flags");

		entry->write(&data, 10);
	}

	return entry;
}
