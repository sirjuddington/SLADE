
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapInfo.cpp
// Description: MapInfo class, for parsing MAPINFO/ZMAPINFO
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
#include "MapInfo.h"
#include "Archive/Archive.h"
#include "UI/WxUtils.h"
#include "Utility/StringUtils.h"

using namespace Game;


// -----------------------------------------------------------------------------
//
// MapInfo Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Clears all parsed MAPINFO information abour [maps] and [editor_nums] if set
// -----------------------------------------------------------------------------
void MapInfo::clear(bool maps, bool editor_nums)
{
	if (maps)
	{
		this->maps_.clear();
		default_map_ = {};
	}

	if (editor_nums)
		this->editor_nums_.clear();
}

// -----------------------------------------------------------------------------
// Returns the map info definition for map [name]
// -----------------------------------------------------------------------------
MapInfo::Map& MapInfo::getMap(std::string_view name)
{
	for (auto& map : maps_)
		if (map.entry_name == name)
			return map;

	return default_map_;
}

// -----------------------------------------------------------------------------
// Adds [map] info, or updates the existing map info if it exists
// -----------------------------------------------------------------------------
bool MapInfo::addOrUpdateMap(Map& map)
{
	for (auto& m : maps_)
		if (m.entry_name == map.entry_name)
		{
			m = map;
			return true;
		}

	maps_.push_back(map);
	return false;
}

// -----------------------------------------------------------------------------
// Returns the DoomEdNum for the ZScript/DECORATE class [actor_class]
// -----------------------------------------------------------------------------
int MapInfo::doomEdNumForClass(std::string_view actor_class)
{
	// Find DoomEdNum def with matching class
	for (auto& i : editor_nums_)
		if (StrUtil::equalCI(i.second.actor_class, actor_class))
			return i.first;

	// Invalid
	return -1;
}

// -----------------------------------------------------------------------------
// Reads and parses all MAPINFO entries in [archive]
// -----------------------------------------------------------------------------
bool MapInfo::readMapInfo(Archive* archive)
{
	vector<ArchiveEntry*> entries;
	archive->putEntryTreeAsList(entries);

	for (auto entry : entries)
	{
		// ZMapInfo
		if (entry->type()->id() == "zmapinfo")
			parseZMapInfo(entry);

		// TODO: EMapInfo
		else if (entry->type()->id() == "emapinfo")
			Log::info("EMAPINFO not implemented");

		// MapInfo
		else if (entry->type()->id() == "mapinfo")
		{
			// Detect format
			auto format = detectMapInfoType(entry);

			if (format == Format::ZDoomNew)
				parseZMapInfo(entry);
			else
				Log::info("MAPINFO not implemented");
		}
	}

	return false;
}

// -----------------------------------------------------------------------------
// Returns true if the next token in [tz] is '='. If not, logs an error message
// -----------------------------------------------------------------------------
bool MapInfo::checkEqualsToken(Tokenizer& tz, std::string_view parsing) const
{
	if (tz.next() != "=")
	{
		Log::error("Error Parsing {}: Expected \"=\", got \"{}\" at line {}", parsing, tz.current().text, tz.lineNo());
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------
// Converts a text colour definition [str] to a colour struct [col].
// Returns false if the given definition was invalid
// -----------------------------------------------------------------------------
bool MapInfo::strToCol(const std::string& str, ColRGBA& col) const
{
	wxColour wxcol;
	if (!wxcol.Set(str))
	{
		// Parse RR GG BB string
		auto components = StrUtil::splitV(str, ' ');
		if (components.size() >= 3)
		{
			col.r = StrUtil::asInt(components[0], 16);
			col.g = StrUtil::asInt(components[1], 16);
			col.b = StrUtil::asInt(components[2], 16);
			return true;
		}
	}
	else
	{
		col.r = wxcol.Red();
		col.g = wxcol.Green();
		col.b = wxcol.Blue();
		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Parses ZMAPINFO-format definitions in [entry]
// -----------------------------------------------------------------------------
bool MapInfo::parseZMapInfo(ArchiveEntry* entry)
{
	Tokenizer tz;
	tz.setReadLowerCase(true);
	tz.openMem(entry->data(), entry->name());

	while (!tz.atEnd())
	{
		// Include
		if (tz.check("include"))
		{
			// Get entry at include path
			auto include_entry = entry->parent()->entryAtPath(tz.next().text);

			if (!include_entry)
			{
				Log::warning(
					"Warning - Parsing ZMapInfo \"{}\": Unable to include \"{}\" at line {}",
					entry->name(),
					tz.current().text,
					tz.lineNo());
			}
			else if (!parseZMapInfo(include_entry))
				return false;
		}

		// Map
		else if (tz.check("map") || tz.check("defaultmap") || tz.check("adddefaultmap"))
		{
			if (!parseZMap(tz, tz.current().text))
				return false;
		}

		// DoomEdNums
		else if (tz.check("doomednums"))
		{
			if (!parseDoomEdNums(tz))
				return false;
		}

		// Unknown block (skip it)
		else if (tz.check("{"))
		{
			Log::warning(2, "Warning - Parsing ZMapInfo \"{}\": Skipping {{}} block", entry->name());

			tz.adv();
			tz.skipSection("{", "}");
			continue;
		}

		// Unknown
		else
		{
			Log::warning(2, R"(Warning - Parsing ZMapInfo "{}": Unknown token "{}")", entry->name(), tz.current().text);
		}

		tz.adv();
	}

	Log::info(2, "Parsed ZMapInfo entry {} successfully", entry->name());

	return true;
}

// -----------------------------------------------------------------------------
// Parses a ZMAPINFO map definition of [type] beginning at the current token in
// tokenizer [tz]
// -----------------------------------------------------------------------------
bool MapInfo::parseZMap(Tokenizer& tz, std::string_view type)
{
	// TODO: Handle adddefaultmap
	auto map = default_map_;

	// Normal map, get lump/name/etc
	tz.adv();
	if (type == "map")
	{
		// Entry name should be just after map keyword
		map.entry_name = tz.current().text;

		// Parse map name
		tz.adv();
		if (tz.check("lookup"))
		{
			map.lookup_name = true;
			map.name        = tz.next().text;
		}
		else
		{
			map.lookup_name = false;
			map.name        = tz.current().text;
		}

		tz.adv();
	}

	if (!tz.advIf("{"))
	{
		Log::error(R"(Error Parsing ZMapInfo: Expecting "{{", got "{}" at line {})", tz.current().text, tz.lineNo());
		return false;
	}

	while (!tz.checkOrEnd("}"))
	{
		// Block (skip it)
		if (tz.advIf("{"))
			tz.skipSection("{", "}");

		// LevelNum
		else if (tz.check("levelnum"))
		{
			if (!checkEqualsToken(tz, "ZMapInfo"))
				return false;

			// Parse number
			// TODO: Checks
			tz.next().toInt(map.level_num);
		}

		// Sky1
		else if (tz.check("sky1"))
		{
			if (!checkEqualsToken(tz, "ZMapInfo"))
				return false;

			map.sky1 = tz.next().text;

			// Scroll speed
			// TODO: Checks
			if (tz.advIfNext(","))
				tz.next().toFloat(map.sky1_scroll_speed);
		}

		// Sky2
		else if (tz.check("sky2"))
		{
			if (!checkEqualsToken(tz, "ZMapInfo"))
				return false;

			map.sky2 = tz.next().text;

			// Scroll speed
			// TODO: Checks
			if (tz.advIfNext(","))
				tz.next().toFloat(map.sky2_scroll_speed);
		}

		// Skybox
		else if (tz.check("skybox"))
		{
			if (!checkEqualsToken(tz, "ZMapInfo"))
				return false;

			map.sky1 = tz.next().text;
		}

		// DoubleSky
		else if (tz.check("doublesky"))
			map.sky_double = true;

		// ForceNoSkyStretch
		else if (tz.check("forcenoskystretch"))
			map.sky_force_no_stretch = true;

		// SkyStretch
		else if (tz.check("skystretch"))
			map.sky_stretch = true;

		// Fade
		else if (tz.check("fade"))
		{
			if (!checkEqualsToken(tz, "ZMapInfo"))
				return false;

			if (!strToCol(tz.next().text, map.fade))
				return false;
		}

		// OutsideFog
		else if (tz.check("outsidefog"))
		{
			if (!checkEqualsToken(tz, "ZMapInfo"))
				return false;

			if (!strToCol(tz.next().text, map.fade_outside))
				return false;
		}

		// EvenLighting
		else if (tz.check("evenlighting"))
		{
			map.lighting_wallshade_h = 0;
			map.lighting_wallshade_v = 0;
		}

		// SmoothLighting
		else if (tz.check("smoothlighting"))
			map.lighting_smooth = true;

		// VertWallShade
		else if (tz.check("vertwallshade"))
		{
			if (!checkEqualsToken(tz, "ZMapInfo"))
				return false;

			// TODO: Checks
			tz.next().toInt(map.lighting_wallshade_v);
		}

		// HorzWallShade
		else if (tz.check("horzwallshade"))
		{
			if (!checkEqualsToken(tz, "ZMapInfo"))
				return false;

			// TODO: Checks
			tz.next().toInt(map.lighting_wallshade_h);
		}

		// ForceFakeContrast
		else if (tz.check("forcefakecontrast"))
			map.force_fake_contrast = true;

		tz.adv();
	}

	if (type == "map")
	{
		Log::info(2, "Parsed ZMapInfo Map {} ({}) successfully", map.entry_name, map.name);

		// Update existing map
		bool updated = false;
		for (auto& m : maps_)
			if (m.entry_name == map.entry_name)
			{
				m       = map;
				updated = true;
				break;
			}

		// Add if it didn't exist
		if (!updated)
			maps_.push_back(map);
	}
	else if (type == "defaultmap")
		default_map_ = map;

	return true;
}

// -----------------------------------------------------------------------------
// Parses a ZMAPINFO DoomEdNums block beginning at the current position in [tz]
// -----------------------------------------------------------------------------
bool MapInfo::parseDoomEdNums(Tokenizer& tz)
{
	// Opening brace
	if (!tz.advIfNext("{", 2))
	{
		Log::error(R"(Error Parsing ZMapInfo: Expecting "{{", got "{}" at line {})", tz.peek().text, tz.lineNo());
		return false;
	}

	while (!tz.checkOrEnd("}"))
	{
		// Editor number
		if (!tz.current().isInteger())
		{
			Log::error(
				"Error Parsing ZMapInfo DoomEdNums: Expecting editor number, got \"{}\" at line {}",
				tz.current().text,
				tz.lineNo());
			return false;
		}

		// Reset editor number values
		auto number                  = tz.current().asInt();
		editor_nums_[number].special = "";
		for (int& arg : editor_nums_[number].args)
			arg = 0;

		// =
		if (!tz.advIfNext("="))
		{
			Log::error(
				R"(Error Parsing ZMapInfo DoomEdNums: Expecting "=", got "{}" at line {})",
				tz.current().text,
				tz.lineNo());
			return false;
		}

		// Actor Class
		editor_nums_[number].actor_class = tz.next().text;

		// Check for special/args definition
		if (tz.advIfNext(",", 2))
		{
			int arg = 0;

			// Check if special or arg
			if (!tz.current().isInteger())
				editor_nums_[number].special = tz.current().text;
			else
				editor_nums_[number].args[arg++] = tz.current().asInt();

			// Parse any further args
			while (tz.advIfNext(",", 2))
			{
				if (!tz.current().isInteger() && !tz.check("+"))
				{
					Log::error(
						"Error Parsing ZMapInfo DoomEdNums: Expecting arg value, got \"{}\" at line {}",
						tz.current().text,
						tz.current().line_no);
					return false;
				}

				if (arg < 5 && !tz.check("+"))
					editor_nums_[number].args[arg++] = tz.current().asInt();
			}
		}

		tz.adv();
	}

	Log::info(2, "Parsed ZMapInfo DoomEdNums successfully");

	return true;
}

// -----------------------------------------------------------------------------
// Attempts to detect the port-specific MAPINFO format of [entry]
// -----------------------------------------------------------------------------
MapInfo::Format MapInfo::detectMapInfoType(ArchiveEntry* entry) const
{
	Tokenizer tz;
	tz.openMem(entry->data(), entry->name());
	tz.setSpecialCharacters("={}[]+,|");

	std::string prev;

	while (!tz.atEnd())
	{
		// Ignore quoted strings
		if (tz.current().quoted_string)
		{
			tz.adv();
			continue;
		}

		// '[' or ']' generally means Eternity format
		if (tz.check("[") || tz.check("]"))
			return Format::Eternity;

		// Opening curly brace
		if (tz.check("{"))
		{
			// If this isn't an endgame block it's ZMAPINFO
			if (prev != "endgame")
				return Format::ZDoomNew;
		}

		prev = tz.current().text;
		tz.adv();
	}

	// Default standard MAPINFO for now
	return Format::Hexen;
}

// -----------------------------------------------------------------------------
// Dumps all parsed DoomEdNums to the log
// -----------------------------------------------------------------------------
void MapInfo::dumpDoomEdNums()
{
	for (auto& num : editor_nums_)
	{
		if (num.second.actor_class.empty())
			continue;

		Log::info(
			R"(DoomEdNum {}: Class "{}", Special "{}", Args {},{},{},{},{})",
			num.first,
			num.second.actor_class,
			num.second.special,
			num.second.args[0],
			num.second.args[1],
			num.second.args[2],
			num.second.args[3],
			num.second.args[4]);
	}
}



// TEMP TESTING STUFF
#include "General/Console/Console.h"
#include "MainEditor/MainEditor.h"

CONSOLE_COMMAND(test_parse_zmapinfo, 1, false)
{
	auto archive = MainEditor::currentArchive();
	if (archive)
	{
		auto entry = archive->entryAtPath(args[0]);
		if (!entry)
			Log::console("Invalid entry path");
		else
		{
			MapInfo test;
			if (test.parseZMapInfo(entry))
				test.dumpDoomEdNums();
		}
	}
}
