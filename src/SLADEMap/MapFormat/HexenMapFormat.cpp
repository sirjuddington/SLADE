
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    HexenMapFormat.cpp
// Description: MapFormatHandler specialization to handle Hexen format maps.
//              Pretty much just DoomMapFormat with different line & thing
//              handling
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
#include "HexenMapFormat.h"
#include "Archive/ArchiveEntry.h"
#include "Game/ActionSpecial.h"
#include "Game/Configuration.h"
#include "Game/Game.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSide.h"
#include "SLADEMap/MapObject/MapThing.h"
#include "SLADEMap/MapObjectCollection.h"
#include "SLADEMap/MapObjectList/LineList.h"
#include "SLADEMap/MapObjectList/SideList.h"
#include "SLADEMap/MapObjectList/ThingList.h"
#include "SLADEMap/MapObjectList/VertexList.h"
#include "UI/UI.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// HexenMapFormat Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Writes the given [map_data] to Hexen format, returning the list of entries
// making up the map
// -----------------------------------------------------------------------------
vector<unique_ptr<ArchiveEntry>> HexenMapFormat::writeMap(
	const MapObjectCollection& map_data,
	const PropertyList&        map_extra_props)
{
	// Build hexen format map data
	auto vertexes = buildVertices(map_data.vertices());
	auto sidedefs = buildSides(map_data.sides());
	auto linedefs = buildHexenLines(map_data.lines());
	auto sectors  = buildSectors(map_data.sectors());
	auto things   = buildHexenThings(map_data.things());

	// Check if we need to compress sides
	auto map_compress_sides = CVar::getInt("map_compress_sides");
	if (map_compress_sides == 2 || map_compress_sides == 1 && sidedefs.size() > 65535)
	{
		// Compress sides & update linedefs with compressed side indices
		auto side_index_map = compressSides(sidedefs);
		remapLineSides(linedefs, map_data.lines(), side_index_map);
	}

	// TODO: Check for exceeded limits and abort with error if so

	// Create map data entries
	vector<unique_ptr<ArchiveEntry>> map_entries;
	map_entries.push_back(
		std::make_unique<ArchiveEntry>("VERTEXES", vertexes.size() * sizeof(Vertex), vertexes.data()));
	map_entries.push_back(
		std::make_unique<ArchiveEntry>("LINEDEFS", linedefs.size() * sizeof(LineDef), linedefs.data()));
	map_entries.push_back(
		std::make_unique<ArchiveEntry>("SIDEDEFS", sidedefs.size() * sizeof(SideDef), sidedefs.data()));
	map_entries.push_back(std::make_unique<ArchiveEntry>("SECTORS", sectors.size() * sizeof(Sector), sectors.data()));
	map_entries.push_back(std::make_unique<ArchiveEntry>("THINGS", things.size() * sizeof(Thing), things.data()));

	return map_entries;
}

// -----------------------------------------------------------------------------
// Reads Hexen-format LINEDEFS data from [entry] into [map_data]
// -----------------------------------------------------------------------------
bool HexenMapFormat::readLINEDEFS(ArchiveEntry* entry, MapObjectCollection& map_data) const
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

		// Get sides and duplicate if necessary
		auto s1 = map_data.sides().at(s1_index);
		if (s1 && s1->parentLine())
			s1 = map_data.duplicateSide(s1);
		auto s2 = map_data.sides().at(s2_index);
		if (s2 && s2->parentLine())
			s2 = map_data.duplicateSide(s2);

		// Create line
		auto line = map_data.addLine(std::make_unique<MapLine>(v1, v2, s1, s2, data.type, data.flags));

		// Set properties
		for (unsigned i = 0; i < 5; ++i)
			line->setArg(i, data.args[i]);

		// Handle some special cases
		if (data.type)
		{
			switch (game::configuration().actionSpecial(data.type).needsTag())
			{
			case game::TagType::LineId:
			case game::TagType::LineId1Line2: line->setId(data.args[0]); break;
			case game::TagType::LineIdHi5:    line->setId((data.args[0] + (data.args[4] << 8))); break;
			default:                          break;
			}
		}
	}

	log::info(3, "Read {} lines", map_data.lines().size());

	return true;
}

// -----------------------------------------------------------------------------
// Reads Hexen-format THINGS data from [entry] into [map_data]
// -----------------------------------------------------------------------------
bool HexenMapFormat::readTHINGS(ArchiveEntry* entry, MapObjectCollection& map_data) const
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

	auto        thng_data = reinterpret_cast<const Thing*>(entry->rawData());
	unsigned    nt        = entry->size() / sizeof(Thing);
	float       p         = ui::getSplashProgress();
	map::ArgSet args;
	for (size_t a = 0; a < nt; a++)
	{
		ui::setSplashProgress(p + (static_cast<float>(a) / nt) * 0.2f);
		const auto& data = thng_data[a];

		// Set args
		for (unsigned i = 0; i < 5; ++i)
			args[i] = data.args[i];

		// Create thing
		map_data.addThing(
			std::make_unique<MapThing>(
				Vec3d{ static_cast<double>(data.x), static_cast<double>(data.y), static_cast<double>(data.z) },
				data.type,
				data.angle,
				data.flags,
				args,
				data.tid,
				data.special));
	}

	log::info(3, "Read {} things", map_data.things().size());

	return true;
}

// -----------------------------------------------------------------------------
// Builds a vector of Hexen-format LineDefs from the given [lines]
// -----------------------------------------------------------------------------
vector<HexenMapFormat::LineDef> HexenMapFormat::buildHexenLines(const LineList& lines) const
{
	vector<LineDef> data;
	data.reserve(lines.size());

	for (auto& line : lines)
	{
		data.push_back(
			{ .vertex1 = static_cast<u16>(line->v1Index()),
			  .vertex2 = static_cast<u16>(line->v2Index()),
			  .flags   = static_cast<u16>(line->flags()),
			  .type    = static_cast<u8>(line->special()),
			  .args    = { static_cast<u8>(line->arg(0)),
						   static_cast<u8>(line->arg(1)),
						   static_cast<u8>(line->arg(2)),
						   static_cast<u8>(line->arg(3)),
						   static_cast<u8>(line->arg(4)) },
			  .side1   = static_cast<u16>(line->s1Index()),
			  .side2   = static_cast<u16>(line->s2Index()) });
	}

	return data;
}

// -----------------------------------------------------------------------------
// Builds a vector of Hexen-format Things from the given [things]
// -----------------------------------------------------------------------------
vector<HexenMapFormat::Thing> HexenMapFormat::buildHexenThings(const ThingList& things) const
{
	vector<Thing> data;
	data.reserve(things.size());

	for (auto& thing : things)
	{
		data.push_back(
			{ .tid     = static_cast<i16>(thing->id()),
			  .x       = static_cast<i16>(thing->xPos()),
			  .y       = static_cast<i16>(thing->yPos()),
			  .z       = static_cast<i16>(thing->zPos()),
			  .angle   = thing->angle(),
			  .type    = thing->type(),
			  .flags   = static_cast<u16>(thing->flags()),
			  .special = static_cast<u8>(thing->special()),
			  .args    = { static_cast<u8>(thing->arg(0)),
						   static_cast<u8>(thing->arg(1)),
						   static_cast<u8>(thing->arg(2)),
						   static_cast<u8>(thing->arg(3)),
						   static_cast<u8>(thing->arg(4)) } });
	}

	return data;
}
