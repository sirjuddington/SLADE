
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
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
#include "Game/Configuration.h"
#include "General/UI.h"
#include "MapEditor/SLADEMap/MapLine.h"
#include "MapEditor/SLADEMap/MapObjectCollection.h"


// -----------------------------------------------------------------------------
//
// HexenMapFormat Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Reads Hexen-format LINEDEFS data from [entry] into [map_data]
// -----------------------------------------------------------------------------
bool HexenMapFormat::readLINEDEFS(ArchiveEntry* entry, MapObjectCollection& map_data) const
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

		// Get sides and duplicate if necessary
		auto s1 = map_data.sides().at(s1_index);
		if (s1 && s1->parentLine())
			s1 = map_data.duplicateSide(s1);
		auto s2 = map_data.sides().at(s2_index);
		if (s2 && s2->parentLine())
			s2 = map_data.duplicateSide(s2);

		// Create line
		auto line = map_data.addLine(std::make_unique<MapLine>(v1, v2, s1, s2, data.type));

		// Set properties
		line->setIntProperty("arg0", data.args[0]);
		line->setIntProperty("arg1", data.args[1]);
		line->setIntProperty("arg2", data.args[2]);
		line->setIntProperty("arg3", data.args[3]);
		line->setIntProperty("arg4", data.args[4]);
		line->setIntProperty("flags", data.flags);

		// Handle some special cases
		if (data.type)
		{
			switch (Game::configuration().actionSpecial(data.type).needsTag())
			{
			case Game::TagType::LineId:
			case Game::TagType::LineId1Line2: line->setIntProperty("id", data.args[0]); break;
			case Game::TagType::LineIdHi5: line->setIntProperty("id", (data.args[0] + (data.args[4] << 8))); break;
			default: break;
			}
		}
	}

	Log::info(3, S_FMT("Read %lu lines", map_data.lines().size()));

	return true;
}

// -----------------------------------------------------------------------------
// Reads Hexen-format THINGS data from [entry] into [map_data]
// -----------------------------------------------------------------------------
bool HexenMapFormat::readTHINGS(ArchiveEntry* entry, MapObjectCollection& map_data) const
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
		const auto& data = thng_data[a];

		// Create thing
		auto thing = map_data.addThing(
			std::make_unique<MapThing>(Vec2f{ (double)data.x, (double)data.y }, data.type, data.angle, data.flags));

		// Set properties
		thing->setIntProperty("height", data.z);
		thing->setIntProperty("special", data.special);
		thing->setIntProperty("id", data.tid);
		thing->setIntProperty("arg0", data.args[0]);
		thing->setIntProperty("arg1", data.args[1]);
		thing->setIntProperty("arg2", data.args[2]);
		thing->setIntProperty("arg3", data.args[3]);
		thing->setIntProperty("arg4", data.args[4]);
	}

	Log::info(3, S_FMT("Read %lu things", map_data.things().size()));

	return true;
}

// -----------------------------------------------------------------------------
// Creates and returns a Hexen-format LINEDEFS entry from [lines]
// -----------------------------------------------------------------------------
ArchiveEntry::UPtr HexenMapFormat::writeLINEDEFS(const LineList& lines) const
{
	auto entry = std::make_unique<ArchiveEntry>("LINEDEFS");

	// Init entry data
	entry->clearData();
	entry->resize(lines.size() * sizeof(LineDef), false);
	entry->seek(0, 0);

	// Write line data
	LineDef data;
	for (auto& line : lines)
	{
		data.vertex1 = line->v1Index();
		data.vertex2 = line->v2Index();

		// Properties
		data.flags   = line->intProperty("flags");
		data.type    = line->special();
		data.args[0] = line->intProperty("arg0");
		data.args[1] = line->intProperty("arg1");
		data.args[2] = line->intProperty("arg2");
		data.args[3] = line->intProperty("arg3");
		data.args[4] = line->intProperty("arg4");

		// Sides
		data.side1 = line->s1Index();
		data.side2 = line->s2Index();

		entry->write(&data, sizeof(LineDef));
	}

	return entry;
}

// -----------------------------------------------------------------------------
// Creates and returns a Hexen-format THINGS entry from [things]
// -----------------------------------------------------------------------------
ArchiveEntry::UPtr HexenMapFormat::writeTHINGS(const ThingList& things) const
{
	auto entry = std::make_unique<ArchiveEntry>("THINGS");

	// Init entry data
	entry->clearData();
	entry->resize(things.size() * sizeof(Thing), false);
	entry->seek(0, 0);

	// Write thing data
	Thing data;
	for (auto& thing : things)
	{
		// Position
		data.x = thing->xPos();
		data.y = thing->yPos();
		data.z = thing->intProperty("height");

		// Properties
		data.angle   = thing->angle();
		data.type    = thing->type();
		data.flags   = thing->intProperty("flags");
		data.special = thing->intProperty("special");
		data.tid     = thing->intProperty("id");
		data.args[0] = thing->intProperty("arg0");
		data.args[1] = thing->intProperty("arg1");
		data.args[2] = thing->intProperty("arg2");
		data.args[3] = thing->intProperty("arg3");
		data.args[4] = thing->intProperty("arg4");

		entry->write(&data, sizeof(Thing));
	}

	return entry;
}
