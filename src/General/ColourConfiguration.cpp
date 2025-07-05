
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ColourConfiguration.cpp
// Description: Functions for handling colour configurations
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
#include "ColourConfiguration.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "Console.h"
#include "Utility/Colour.h"
#include "Utility/JsonUtils.h"
#include "Utility/Parser.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace slade::colourconfig
{
typedef std::map<string, Colour> ColourHashMap;

double        line_hilight_width;
double        line_selection_width;
double        flat_alpha;
ColourHashMap cc_colours;
} // namespace slade::colourconfig


// -----------------------------------------------------------------------------
//
// Colour struct <=> JSON Object Conversion Functions
//
// -----------------------------------------------------------------------------
namespace slade::colourconfig
{
void to_json(json& j, const Colour& c)
{
	auto col_string = colour::toString(
		c.colour, c.colour.a == 255 ? colour::StringFormat::RGB : colour::StringFormat::RGBA);

	j = json{ { "name", c.name }, { "group", c.group }, { "colour", col_string } };

	if (c.blend_additive)
		j["blend_additive"] = true;
}

void from_json(const json& j, Colour& c)
{
	j.at("name").get_to(c.name);
	j.at("group").get_to(c.group);

	auto col = j.at("colour");
	if (col.is_array() && col.size() == 4)
		c.colour.set(col[0].get<int>(), col[1].get<int>(), col[2].get<int>(), col[3].get<int>());
	else if (col.is_array() && col.size() == 3)
		c.colour.set(col[0].get<int>(), col[1].get<int>(), col[2].get<int>());
	else if (col.is_string())
		c.colour = colour::fromString(col.get<string>());

	if (j.contains("blend_additive"))
		j.at("blend_additive").get_to(c.blend_additive);
}
} // namespace slade::colourconfig


// -----------------------------------------------------------------------------
//
// colourconfig Namespace Functions
//
// -----------------------------------------------------------------------------
namespace slade::colourconfig
{
// -----------------------------------------------------------------------------
// Reads a pre-3.3.0 format colour configuration from text data [mc]
// -----------------------------------------------------------------------------
bool readOldConfiguration(const MemChunk& mc)
{
	// Parse text
	Parser parser;
	if (!parser.parseText(mc))
		return false;

	// Get 'colours' block
	if (auto colours = parser.parseTreeRoot()->childPTN("colours"))
	{
		// Read all colour definitions
		for (unsigned a = 0; a < colours->nChildren(); a++)
		{
			auto def = colours->childPTN(a);

			// Read properties
			for (unsigned b = 0; b < def->nChildren(); b++)
			{
				auto  prop = def->childPTN(b);
				auto& col  = cc_colours[def->name()];
				col.exists = true;

				// Colour name
				if (prop->name() == "name")
					col.name = prop->stringValue();

				// Colour group (for config ui)
				else if (prop->name() == "group")
					col.group = prop->stringValue();

				// Colour
				else if (prop->name() == "rgb")
					col.colour.set(prop->intValue(0), prop->intValue(1), prop->intValue(2));

				// Alpha
				else if (prop->name() == "alpha")
					col.colour.a = prop->intValue();

				// Additive
				else if (prop->name() == "additive")
					col.blend_additive = prop->boolValue();

				else
					log::warning(fmt::format("Unknown colour definition property \"{}\"", prop->name()));
			}
		}
	}

	// Get 'theme' block
	if (auto theme = parser.parseTreeRoot()->childPTN("theme"))
	{
		// Read all theme definitions
		for (unsigned a = 0; a < theme->nChildren(); a++)
		{
			auto prop = theme->childPTN(a);

			if (prop->name() == "line_hilight_width")
				line_hilight_width = prop->floatValue();

			else if (prop->name() == "line_selection_width")
				line_selection_width = prop->floatValue();

			else if (prop->name() == "flat_alpha")
				flat_alpha = prop->floatValue();

			else
				log::warning(fmt::format("Unknown theme property \"{}\"", prop->name()));
		}
	}

	return true;
}
} // namespace slade::colourconfig

// -----------------------------------------------------------------------------
// Returns the colour [name]
// -----------------------------------------------------------------------------
ColRGBA colourconfig::colour(const string& name)
{
	auto& col = cc_colours[name];
	if (col.exists)
		return col.colour;
	else
		return ColRGBA::WHITE;
}

// -----------------------------------------------------------------------------
// Returns the colour definition [name]
// -----------------------------------------------------------------------------
const colourconfig::Colour& colourconfig::colDef(const string& name)
{
	return cc_colours[name];
}

// -----------------------------------------------------------------------------
// Sets the colour definition [name]
// -----------------------------------------------------------------------------
void colourconfig::setColour(const string& name, int red, int green, int blue, int alpha, bool blend_additive)
{
	auto& col = cc_colours[name];
	if (red >= 0)
		col.colour.r = red;
	if (green >= 0)
		col.colour.g = green;
	if (blue >= 0)
		col.colour.b = blue;
	if (alpha >= 0)
		col.colour.a = alpha;

	col.blend_additive = blend_additive;
	col.exists         = true;
}

// -----------------------------------------------------------------------------
// Returns the line hilight width multiplier
// -----------------------------------------------------------------------------
double colourconfig::lineHilightWidth()
{
	return line_hilight_width;
}

// -----------------------------------------------------------------------------
// Returns the line selection width multiplier
// -----------------------------------------------------------------------------
double colourconfig::lineSelectionWidth()
{
	return line_selection_width;
}

// -----------------------------------------------------------------------------
// Returns the flat alpha multiplier
// -----------------------------------------------------------------------------
double colourconfig::flatAlpha()
{
	return flat_alpha;
}

// -----------------------------------------------------------------------------
// Sets the line hilight width multiplier
// -----------------------------------------------------------------------------
void colourconfig::setLineHilightWidth(double mult)
{
	line_hilight_width = mult;
}

// -----------------------------------------------------------------------------
// Sets the line selection width multiplier
// -----------------------------------------------------------------------------
void colourconfig::setLineSelectionWidth(double mult)
{
	line_selection_width = mult;
}

// -----------------------------------------------------------------------------
// Sets the flat alpha multiplier
// -----------------------------------------------------------------------------
void colourconfig::setFlatAlpha(double alpha)
{
	flat_alpha = alpha;
}

// -----------------------------------------------------------------------------
// Reads a colour configuration from JSON [j]
// -----------------------------------------------------------------------------
void colourconfig::readConfiguration(json& j)
{
	// Colours
	if (j.contains("colours"))
	{
		for (auto& [id, colour] : j["colours"].items())
		{
			auto& col = cc_colours[id];
			from_json(colour, col);
			col.exists = true;
		}
	}

	// Theme
	if (j.contains("theme"))
	{
		if (j["theme"].contains("line_hilight_width"))
			j["theme"]["line_hilight_width"].get_to(line_hilight_width);
		if (j["theme"].contains("line_selection_width"))
			j["theme"]["line_selection_width"].get_to(line_selection_width);
		if (j["theme"].contains("flat_alpha"))
			j["theme"]["flat_alpha"].get_to(flat_alpha);
	}
}

// -----------------------------------------------------------------------------
// Writes the current colour configuration to [json_file]
// -----------------------------------------------------------------------------
void colourconfig::writeConfiguration(string_view json_file)
{
	json j;

	// Colours
	for (const auto& i : cc_colours)
	{
		const auto& cc = i.second;
		if (!cc.exists)
			continue;

		j["colours"][i.first] = cc;
	}

	// Theme
	j["theme"]["line_hilight_width"]   = line_hilight_width;
	j["theme"]["line_selection_width"] = line_selection_width;
	j["theme"]["flat_alpha"]           = flat_alpha;

	jsonutil::writeFile(j, json_file);
}

// -----------------------------------------------------------------------------
// Initialises the colour configuration
// -----------------------------------------------------------------------------
bool colourconfig::init()
{
	// Load default configuration
	loadDefaults();

	// Check for saved colour configuration
	if (auto j = jsonutil::parseFile(app::path("colours.json", app::Dir::User)); !j.is_discarded())
		readConfiguration(j);

	// Check for pre-3.3.0 configuration
	else if (MemChunk mc; mc.importFile(app::path("colours.cfg", app::Dir::User)))
		readOldConfiguration(mc);

	return true;
}

// -----------------------------------------------------------------------------
// Sets all colours in the current configuration to default
// -----------------------------------------------------------------------------
void colourconfig::loadDefaults()
{
	// Read default colours
	auto pres             = app::programResource();
	auto entry_default_cc = pres->entryAtPath("config/colours/default.json");
	if (entry_default_cc)
	{
		if (auto j = jsonutil::parse(entry_default_cc->data()); !j.is_discarded())
			readConfiguration(j);
	}
}

// -----------------------------------------------------------------------------
// Reads saved colour configuration [name]
// -----------------------------------------------------------------------------
bool colourconfig::readConfiguration(string_view name)
{
	// Search resource pk3
	auto res = app::programResource();
	auto dir = res->dirAtPath("config/colours");
	for (unsigned a = 0; a < dir->numEntries(); a++)
	{
		if (strutil::equalCI(dir->entryAt(a)->nameNoExt(), name))
		{
			if (auto j = jsonutil::parse(dir->entryAt(a)->data()); !j.is_discarded())
				readConfiguration(j);
			return true;
		}
	}

	return false;
}

// -----------------------------------------------------------------------------
// Adds all available colour configuration names to [names]
// -----------------------------------------------------------------------------
void colourconfig::putConfigurationNames(vector<string>& names)
{
	// Search resource pk3
	auto res = app::programResource();
	auto dir = res->dirAtPath("config/colours");
	for (unsigned a = 0; a < dir->numEntries(); a++)
		names.emplace_back(dir->entryAt(a)->nameNoExt());
}

// -----------------------------------------------------------------------------
// Adds all colour names to [list]
// -----------------------------------------------------------------------------
void colourconfig::putColourNames(vector<string>& list)
{
	for (auto& i : cc_colours)
		list.push_back(i.first);
}


// -----------------------------------------------------------------------------
//
// Console Commands
//
// -----------------------------------------------------------------------------

CONSOLE_COMMAND(ccfg, 1, false)
{
	// Check for 'list'
	if (strutil::equalCI(args[0], "list"))
	{
		// Get (sorted) list of colour names
		vector<string> list;
		colourconfig::putColourNames(list);
		sort(list.begin(), list.end());

		// Dump list to console
		log::console(fmt::format("{} Colours:", list.size()));
		for (const auto& a : list)
			log::console(a);
	}
	else
	{
		// Check for enough args to set the colour
		if (args.size() >= 4)
		{
			// Read RGB
			int red   = strutil::asInt(args[1]);
			int green = strutil::asInt(args[2]);
			int blue  = strutil::asInt(args[3]);

			// Read alpha (if specified)
			int alpha = -1;
			if (args.size() >= 5)
				alpha = strutil::asInt(args[4]);

			// Read blend (if specified)
			int blend = -1;
			if (args.size() >= 6)
				blend = strutil::asInt(args[5]);

			// Set colour
			colourconfig::setColour(args[0], red, green, blue, alpha, blend);
		}

		// Print colour
		auto def = colourconfig::colDef(args[0]);
		log::console(
			fmt::format(
				"Colour \"{}\" = {} {} {} {} {}",
				args[0],
				def.colour.r,
				def.colour.g,
				def.colour.b,
				def.colour.a,
				def.blend_additive));
	}
}

CONSOLE_COMMAND(load_ccfg, 1, false)
{
	colourconfig::readConfiguration(args[0]);
}
