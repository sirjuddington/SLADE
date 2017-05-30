
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ColourConfiguration.cpp
 * Description: Functions for handling colour configurations
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "App.h"
#include "ColourConfiguration.h"
#include "Utility/Parser.h"
#include "Archive/ArchiveManager.h"
#include "Console/Console.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
typedef std::map<string, cc_col_t> ColourHashMap;
ColourHashMap	cc_colours;
namespace ColourConfiguration
{
	double	line_hilight_width;
	double	line_selection_width;
	double	flat_alpha;
}


/*******************************************************************
 * COLOURCONFIGURATION NAMESPACE FUNCTIONS
 *******************************************************************/

/* ColourConfiguration::getColour
 * Returns the colour [name]
 *******************************************************************/
rgba_t ColourConfiguration::getColour(string name)
{
	cc_col_t& col = cc_colours[name];
	if (col.exists)
		return col.colour;
	else
		return COL_WHITE;
}

/* ColourConfiguration::getColDef
 * Returns the colour definition [name]
 *******************************************************************/
cc_col_t ColourConfiguration::getColDef(string name)
{
	return cc_colours[name];
}

/* ColourConfiguration::setColour
 * Sets the colour definition [name]
 *******************************************************************/
void ColourConfiguration::setColour(string name, int red, int green, int blue, int alpha, int blend)
{
	cc_col_t& col = cc_colours[name];
	if (red >= 0)
		col.colour.r = red;
	if (green >= 0)
		col.colour.g = green;
	if (blue >= 0)
		col.colour.b = blue;
	if (alpha >= 0)
		col.colour.a = alpha;
	if (blend >= 0)
		col.colour.blend = blend;
	col.exists = true;
}

/* ColourConfiguration::getLineHilightWidth
 * Returns the line hilight width multiplier
 *******************************************************************/
double ColourConfiguration::getLineHilightWidth()
{
	return line_hilight_width;
}

/* ColourConfiguration::getLineSelectionWidth
 * Returns the line selection width multiplier
 *******************************************************************/
double ColourConfiguration::getLineSelectionWidth()
{
	return line_selection_width;
}

/* ColourConfiguration::getFlatAlpha
 * Returns the flat alpha multiplier
 *******************************************************************/
double ColourConfiguration::getFlatAlpha()
{
	return flat_alpha;
}

/* ColourConfiguration::setLineHilightWidth
 * Sets the line hilight width multiplier
 *******************************************************************/
void ColourConfiguration::setLineHilightWidth(double mult)
{
	line_hilight_width = mult;
}

/* ColourConfiguration::setLineSelectionWidth
 * Sets the line selection width multiplier
 *******************************************************************/
void ColourConfiguration::setLineSelectionWidth(double mult)
{
	line_selection_width = mult;
}

/* ColourConfiguration::setFlatAlpha
 * Sets the flat alpha multiplier
 *******************************************************************/
void ColourConfiguration::setFlatAlpha(double alpha)
{
	flat_alpha = alpha;
}

/* ColourConfiguration::readConfiguration
 * Reads a colour configuration from text data [mc]
 *******************************************************************/
bool ColourConfiguration::readConfiguration(MemChunk& mc)
{
	// Parse text
	Parser parser;
	parser.parseText(mc);

	// Get 'colours' block
	auto colours = parser.parseTreeRoot()->getChildPTN("colours");
	if (colours)
	{
		// Read all colour definitions
		for (unsigned a = 0; a < colours->nChildren(); a++)
		{
			auto def = colours->getChildPTN(a);

			// Read properties
			for (unsigned b = 0; b < def->nChildren(); b++)
			{
				auto prop = def->getChildPTN(b);
				cc_col_t& col = cc_colours[def->getName()];
				col.exists = true;

				// Colour name
				if (prop->getName() == "name")
					col.name = prop->stringValue();

				// Colour group (for config ui)
				else if (prop->getName() == "group")
					col.group = prop->stringValue();

				// Colour
				else if (prop->getName() == "rgb")
					col.colour.set(prop->intValue(0), prop->intValue(1), prop->intValue(2));

				// Alpha
				else if (prop->getName() == "alpha")
					col.colour.a = prop->intValue();

				// Additive
				else if (prop->getName() == "additive")
				{
					if (prop->boolValue())
						col.colour.blend = 1;
					else
						col.colour.blend = 0;
				}

				else
					LOG_MESSAGE(1, "Warning: unknown colour definition property \"%s\"", prop->getName());
			}
		}
	}

	// Get 'theme' block
	auto theme = parser.parseTreeRoot()->getChildPTN("theme");
	if (theme)
	{
		// Read all theme definitions
		for (unsigned a = 0; a < theme->nChildren(); a++)
		{
			auto prop = theme->getChildPTN(a);

			if (prop->getName() == "line_hilight_width")
				line_hilight_width = prop->floatValue();

			else if (prop->getName() == "line_selection_width")
				line_selection_width = prop->floatValue();

			else if (prop->getName() == "flat_alpha")
				flat_alpha = prop->floatValue();

			else
				LOG_MESSAGE(1, "Warning: unknown theme property \"%s\"", prop->getName());
		}
	}

	return true;
}

/* ColourConfiguration::writeConfiguration
 * Writes the current colour configuration to text data [mc]
 *******************************************************************/
bool ColourConfiguration::writeConfiguration(MemChunk& mc)
{
	string cfgstring = "colours\n{\n";

	ColourHashMap::iterator i = cc_colours.begin();

	// Go through all properties
	while (i != cc_colours.end())
	{
		// Skip if it doesn't 'exist'
		cc_col_t cc = i->second;
		if (!cc.exists)
		{
			i++;
			continue;
		}

		// Colour definition name
		cfgstring += S_FMT("\t%s\n\t{\n", i->first);

		// Full name
		cfgstring += S_FMT("\t\tname = \"%s\";\n", cc.name);

		// Group
		cfgstring += S_FMT("\t\tgroup = \"%s\";\n", cc.group);

		// Colour values
		cfgstring += S_FMT("\t\trgb = %d, %d, %d;\n", cc.colour.r, cc.colour.g, cc.colour.b);

		// Alpha
		if (cc.colour.a < 255)
			cfgstring += S_FMT("\t\talpha = %d;\n", cc.colour.a);

		// Additive
		if (cc.colour.blend == 1)
			cfgstring += "\t\tadditive = true;\n";

		cfgstring += "\t}\n\n";

		// Next colour
		i++;
	}

	cfgstring += "}\n\ntheme\n{\n";

	cfgstring += S_FMT("\tline_hilight_width = %1.3f;\n", line_hilight_width);
	cfgstring += S_FMT("\tline_selection_width = %1.3f;\n", line_selection_width);
	cfgstring += S_FMT("\tflat_alpha = %1.3f;\n", flat_alpha);
	cfgstring += "}\n";

	mc.write(cfgstring.ToAscii(), cfgstring.size());

	return true;
}

/* ColourConfiguration::init
 * Initialises the colour configuration
 *******************************************************************/
bool ColourConfiguration::init()
{
	// Load default configuration
	loadDefaults();

	// Check for saved colour configuration
	if (wxFileExists(App::path("colours.cfg", App::Dir::User)))
	{
		MemChunk ccfg;
		ccfg.importFile(App::path("colours.cfg", App::Dir::User));
		readConfiguration(ccfg);
	}

	return true;
}

/* ColourConfiguration::loadDefaults
 * Sets all colours in the current configuration to default
 *******************************************************************/
void ColourConfiguration::loadDefaults()
{
	// Read default colours
	Archive* pres = theArchiveManager->programResourceArchive();
	ArchiveEntry* entry_default_cc = pres->entryAtPath("config/colours/default.txt");
	if (entry_default_cc)
		readConfiguration(entry_default_cc->getMCData());
}

/* ColourConfiguration::readConfiguration
 * Reads saved colour configuration [name]
 *******************************************************************/
bool ColourConfiguration::readConfiguration(string name)
{
	// TODO: search custom folder

	// Search resource pk3
	Archive* res = theArchiveManager->programResourceArchive();
	ArchiveTreeNode* dir = res->getDir("config/colours");
	for (unsigned a = 0; a < dir->numEntries(); a++)
	{
		if (S_CMPNOCASE(dir->getEntry(a)->getName(true), name))
			return readConfiguration(dir->getEntry(a)->getMCData());
	}

	return false;
}

/* ColourConfiguration::getConfigurationNames
 * Adds all available colour configuration names to [names]
 *******************************************************************/
void ColourConfiguration::getConfigurationNames(vector<string>& names)
{
	// TODO: search custom folder

	// Search resource pk3
	Archive* res = theArchiveManager->programResourceArchive();
	ArchiveTreeNode* dir = res->getDir("config/colours");
	for (unsigned a = 0; a < dir->numEntries(); a++)
		names.push_back(dir->getEntry(a)->getName(true));
}

/* ColourConfiguration::getColourNames
 * Adds all colour names to [list]
 *******************************************************************/
void ColourConfiguration::getColourNames(vector<string>& list)
{
	ColourHashMap::iterator i = cc_colours.begin();
	while (i != cc_colours.end())
	{
		list.push_back(i->first);
		i++;
	}
}


/*******************************************************************
 * CONSOLE COMMANDS
 *******************************************************************/
CONSOLE_COMMAND(ccfg, 1, false)
{
	// Check for 'list'
	if (S_CMPNOCASE(args[0], "list"))
	{
		// Get (sorted) list of colour names
		vector<string> list;
		ColourConfiguration::getColourNames(list);
		sort(list.begin(), list.end());

		// Dump list to console
		Log::console(S_FMT("%lu Colours:", list.size()));
		for (unsigned a = 0; a < list.size(); a++)
			Log::console(list[a]);
	}
	else
	{
		// Check for enough args to set the colour
		if (args.size() >= 4)
		{
			// Read RGB
			long red, green, blue;
			args[1].ToLong(&red);
			args[2].ToLong(&green);
			args[3].ToLong(&blue);

			// Read alpha (if specified)
			long alpha = -1;
			if (args.size() >= 5)
				args[4].ToLong(&alpha);

			// Read blend (if specified)
			long blend = -1;
			if (args.size() >= 6)
				args[5].ToLong(&blend);

			// Set colour
			ColourConfiguration::setColour(args[0], red, green, blue, alpha, blend);
		}

		// Print colour
		rgba_t col = ColourConfiguration::getColour(args[0]);
		Log::console(S_FMT("Colour \"%s\" = %d %d %d %d %d", args[0], col.r, col.g, col.b, col.a, col.blend));
	}
}

CONSOLE_COMMAND(load_ccfg, 1, false)
{
	ColourConfiguration::readConfiguration(args[0]);
}
