
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Doom64MapFormat.cpp
// Description: MapFormatHandler specialization to handle Doom64 format maps
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
#include "Doom64MapFormat.h"
#include "General/ResourceManager.h"
#include "General/UI.h"
#include "SLADEMap/SLADEMap.h"


// -----------------------------------------------------------------------------
// Reads the given Doom64-format [map], populating [map_data]
// -----------------------------------------------------------------------------
bool Doom64MapFormat::readMap(Archive::MapDesc map, MapObjectCollection& map_data, PropertyList& map_extra_props)
{
	Log::info(2, "Reading Doom64 format map");

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
// Writes the given [map_data] to Doom64 format, returning the list of entries
// making up the map
// -----------------------------------------------------------------------------
vector<unique_ptr<ArchiveEntry>> Doom64MapFormat::writeMap(
	const MapObjectCollection& map_data,
	const PropertyList&        map_extra_props)
{
	Global::error = "Saving Doom64 maps is not currently supported";
	Log::warning(Global::error);

	return {};
}

// -----------------------------------------------------------------------------
// Reads a Doom64-format VERTEXES [entry] into [map_data]
// -----------------------------------------------------------------------------
bool Doom64MapFormat::readVERTEXES(ArchiveEntry* entry, MapObjectCollection& map_data) const
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
		map_data.addVertex(
			std::make_unique<MapVertex>(Vec2d{ (double)vert_data[a].x / 65536, (double)vert_data[a].y / 65536 }));
	}

	Log::info(3, "Read {} vertices", map_data.vertices().size());

	return true;
}

// -----------------------------------------------------------------------------
// Reads a Doom64-format SIDEDEFS [entry] into [map_data]
// -----------------------------------------------------------------------------
bool Doom64MapFormat::readSIDEDEFS(ArchiveEntry* entry, MapObjectCollection& map_data) const
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
			ResourceManager::doom64TextureName(side_data[a].tex_upper),
			ResourceManager::doom64TextureName(side_data[a].tex_middle),
			ResourceManager::doom64TextureName(side_data[a].tex_lower),
			Vec2i{ side_data[a].x_offset, side_data[a].y_offset }));
	}

	Log::info(3, "Read {} sides", map_data.sides().size());

	return true;
}

// -----------------------------------------------------------------------------
// Reads a Doom64-format LINEDEFS [entry] into [map_data]
// -----------------------------------------------------------------------------
bool Doom64MapFormat::readLINEDEFS(ArchiveEntry* entry, MapObjectCollection& map_data) const
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
			Log::warning("Line {} invalid, not added", a);
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
			std::make_unique<MapLine>(v1, v2, map_data.sides().at(s1_index), map_data.sides().at(s2_index)));

		// Set properties
		line->setArg(0, data.sector_tag);
		if (data.type & 0x100)
			line->setIntProperty("macro", data.type & 0xFF);
		else
			line->setSpecial(data.type & 0xFF);
		line->setFlags(data.flags);
		line->setIntProperty("extraflags", data.type >> 9);
	}

	Log::info(3, "Read {} lines", map_data.lines().size());

	return true;
}

// -----------------------------------------------------------------------------
// Reads a Doom64-format SECTORS [entry] into [map_data]
// -----------------------------------------------------------------------------
bool Doom64MapFormat::readSECTORS(ArchiveEntry* entry, MapObjectCollection& map_data) const
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
		auto sector = map_data.addSector(std::make_unique<MapSector>(
			data.f_height,
			ResourceManager::doom64TextureName(data.f_tex),
			data.c_height,
			ResourceManager::doom64TextureName(data.c_tex),
			255,
			data.special,
			data.tag));

		// Set properties
		sector->setIntProperty("flags", data.flags);
		sector->setIntProperty("color_things", data.color[0]);
		sector->setIntProperty("color_floor", data.color[1]);
		sector->setIntProperty("color_ceiling", data.color[2]);
		sector->setIntProperty("color_upper", data.color[3]);
		sector->setIntProperty("color_lower", data.color[4]);
	}

	Log::info(3, "Read {} sectors", map_data.sectors().size());

	return true;
}

// -----------------------------------------------------------------------------
// Reads a Doom64-format THINGS [entry] into [map_data]
// -----------------------------------------------------------------------------
bool Doom64MapFormat::readTHINGS(ArchiveEntry* entry, MapObjectCollection& map_data) const
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

	auto              thng_data = (Thing*)entry->rawData(true);
	unsigned          nt        = entry->size() / sizeof(Thing);
	float             p         = UI::getSplashProgress();
	MapObject::ArgSet args;
	for (size_t a = 0; a < nt; a++)
	{
		UI::setSplashProgress(p + ((float)a / nt) * 0.2f);
		const auto& data = thng_data[a];

		// Create thing
		auto thing = map_data.addThing(std::make_unique<MapThing>(
			Vec3d{ (double)data.x, (double)data.y, (double)data.z },
			data.type,
			data.angle,
			data.flags,
			args,
			data.tid));
	}

	Log::info(3, "Read {} things", map_data.things().size());

	return true;
}



// Stuff below is copy+pasted from the old SLADEMap doom64 format writing functions
// No point using this stuff yet as doom64 saving is unsupported anyway
#if 0

// -----------------------------------------------------------------------------
// Writes doom64 format vertex definitions to [entry]
// -----------------------------------------------------------------------------
bool SLADEMap::writeDoom64Vertexes(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// Init entry data
	entry->clearData();
	entry->resize(vertices_.size() * 8, false);
	entry->seek(0, 0);

	// Write vertex data
	int32_t x, y; // Those are actually fixed_t, so shift by FRACBIT (16)
	for (auto& vertex : vertices_)
	{
		x = vertex->xPos() * 65536;
		y = vertex->yPos() * 65536;
		entry->write(&x, 4);
		entry->write(&y, 4);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Writes doom64 format sidedef definitions to [entry]
// -----------------------------------------------------------------------------
bool SLADEMap::writeDoom64Sidedefs(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// Init entry data
	entry->clearData();
	entry->resize(sides_.size() * sizeof(MapSide::Doom64Data), false);
	entry->seek(0, 0);

	// Write side data
	MapSide::Doom64Data d64_side;
	for (auto& side : sides_)
	{
		memset(&d64_side, 0, sizeof(MapSide::Doom64Data));

		// Offsets
		d64_side.x_offset = side->offset_x_;
		d64_side.y_offset = side->offset_y_;

		// Sector
		d64_side.sector = -1;
		if (side->sector_)
			d64_side.sector = side->sector_->index();

		// Textures
		d64_side.tex_middle = App::resources().getTextureHash(side->tex_middle_);
		d64_side.tex_upper  = App::resources().getTextureHash(side->tex_upper_);
		d64_side.tex_lower  = App::resources().getTextureHash(side->tex_lower_);

		entry->write(&d64_side, sizeof(MapSide::Doom64Data));
	}

	return true;
}

// -----------------------------------------------------------------------------
// Writes doom64 format linedef definitions to [entry]
// -----------------------------------------------------------------------------
bool SLADEMap::writeDoom64Linedefs(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// Init entry data
	entry->clearData();
	entry->resize(lines_.size() * sizeof(MapLine::Doom64Data), false);
	entry->seek(0, 0);

	// Write line data
	MapLine::Doom64Data d64_line;
	for (auto& line : lines_)
	{
		// Vertices
		d64_line.vertex1 = line->v1Index();
		d64_line.vertex2 = line->v2Index();

		// Properties
		d64_line.flags      = line->intProperty("flags");
		d64_line.type       = line->intProperty("special");
		d64_line.sector_tag = line->intProperty("arg0");

		// Sides
		d64_line.side1 = -1;
		d64_line.side2 = -1;
		if (line->side1_)
			d64_line.side1 = line->side1_->index();
		if (line->side2_)
			d64_line.side2 = line->side2_->index();

		entry->write(&d64_line, sizeof(MapLine::Doom64Data));
	}

	return true;
}

// -----------------------------------------------------------------------------
// Writes doom64 format sector definitions to [entry]
// -----------------------------------------------------------------------------
bool SLADEMap::writeDoom64Sectors(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// Init entry data
	entry->clearData();
	entry->resize(sectors_.size() * sizeof(MapSector::Doom64Data), false);
	entry->seek(0, 0);

	// Write sector data
	MapSector::Doom64Data d64_sector;
	for (auto& sector : sectors_)
	{
		memset(&d64_sector, 0, sizeof(MapSector::Doom64Data));

		// Height
		d64_sector.f_height = sector->floor_.height;
		d64_sector.c_height = sector->ceiling_.height;

		// Textures
		d64_sector.f_tex = App::resources().getTextureHash(sector->stringProperty("texturefloor"));
		d64_sector.c_tex = App::resources().getTextureHash(sector->stringProperty("textureceiling"));

		// Colors
		d64_sector.color[0] = sector->intProperty("color_things");
		d64_sector.color[1] = sector->intProperty("color_floor");
		d64_sector.color[2] = sector->intProperty("color_ceiling");
		d64_sector.color[3] = sector->intProperty("color_upper");
		d64_sector.color[4] = sector->intProperty("color_lower");

		// Properties
		d64_sector.special = sector->special_;
		d64_sector.flags   = sector->intProperty("flags");
		d64_sector.tag     = sector->id_;

		entry->write(&d64_sector, sizeof(MapSector::Doom64Data));
	}

	return true;
}

// -----------------------------------------------------------------------------
// Writes doom64 format thing definitions to [entry]
// -----------------------------------------------------------------------------
bool SLADEMap::writeDoom64Things(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// Init entry data
	entry->clearData();
	entry->resize(things_.size() * sizeof(MapThing::Doom64Data), false);
	entry->seek(0, 0);

	// Write thing data
	MapThing::Doom64Data d64_thing;
	for (auto& thing : things_)
	{
		// Position
		d64_thing.x = thing->xPos();
		d64_thing.y = thing->yPos();
		d64_thing.z = thing->intProperty("height");

		// Properties
		d64_thing.angle = thing->angle();
		d64_thing.type  = thing->type_;
		d64_thing.flags = thing->intProperty("flags");
		d64_thing.tid   = thing->intProperty("id");

		entry->write(&d64_thing, sizeof(MapThing::Doom64Data));
	}

	return true;
}

#endif
