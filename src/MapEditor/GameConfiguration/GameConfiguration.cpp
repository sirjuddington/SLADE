
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    GameConfiguration.cpp
 * Description: GameConfiguration class, handles all game configuration
 *              related stuff - action specials, thing types, supported
 *              formats, etc.
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
#include "GameConfiguration.h"
#include "Utility/Tokenizer.h"
#include "Utility/Parser.h"
#include "General/Misc.h"
#include "General/Console/Console.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveManager.h"
#include "MapEditor/SLADEMap/SLADEMap.h"
#include "GenLineSpecial.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
GameConfiguration* GameConfiguration::instance = nullptr;
CVAR(String, game_configuration, "", CVAR_SAVE)
CVAR(String, port_configuration, "", CVAR_SAVE)
CVAR(Bool, debug_configuration, false, CVAR_SAVE)


/*******************************************************************
 * GAMECONFIGURATION CLASS FUNCTIONS
 *******************************************************************/

/* GameConfiguration::GameConfiguration
 * GameConfiguration class constructor
 *******************************************************************/
GameConfiguration::GameConfiguration()
{
	setDefaults();
}

/* GameConfiguration::~GameConfiguration
 * GameConfiguration class destructor
 *******************************************************************/
GameConfiguration::~GameConfiguration()
{
	// Clean up stuff
	ASpecialMap::iterator as = action_specials.begin();
	while (as != action_specials.end())
	{
		if (as->second.special) delete as->second.special;
		as++;
	}

	ThingTypeMap::iterator tt = thing_types.begin();
	while (tt != thing_types.end())
	{
		if (tt->second.type) delete tt->second.type;
		tt++;
	}

	for (unsigned a = 0; a < tt_group_defaults.size(); a++)
		delete tt_group_defaults[a];
}

/* GameConfiguration::setDefaults
 * Resets all game configuration values to defaults
 *******************************************************************/
void GameConfiguration::setDefaults()
{
	udmf_namespace = "";
	ttype_unknown.icon = "unknown";
	ttype_unknown.shrink = true;
	any_map_name = false;
	mix_tex_flats = false;
	tx_textures = false;
	defaults_line.clear();
	defaults_side.clear();
	defaults_sector.clear();
	defaults_thing.clear();
	maps.clear();
	sky_flat = "F_SKY1";
	script_language = "";
	light_levels.clear();
	for (int a = 0; a < 4; a++)
		map_formats[a] = false;
	boom = false;
	boom_sector_flag_start = 0;
	as_generalized_s.setName("Boom Generalized Switched Special");
	as_generalized_s.setTagged(AS_TT_SECTOR);
	as_generalized_m.setName("Boom Generalized Manual Special");
	as_generalized_m.setTagged(AS_TT_SECTOR_BACK);

	udmf_texture_offsets = udmf_slopes = udmf_flat_lighting = udmf_flat_panning =
	udmf_flat_rotation = udmf_flat_scaling = udmf_line_transparency = udmf_sector_color =
	udmf_sector_fog = udmf_side_lighting = udmf_side_midtex_wrapping = udmf_side_scaling =
	udmf_texture_scaling = false;
}

/* GameConfiguration::udmfNamespace
 * Returns the UDMF namespace for the game configuration
 *******************************************************************/
string GameConfiguration::udmfNamespace()
{
	return udmf_namespace.Lower();
}

/* GameConfiguration::lightLevelInterval
 * Returns the light level interval for the game configuration
 *******************************************************************/
int GameConfiguration::lightLevelInterval()
{
	if (light_levels.size() == 0)
		return 1;
	else
		return light_levels[1];
}

/* GameConfiguration::readConfigName
 * Parses the game configuration definition in [mc] and returns the
 * configuration name
 *******************************************************************/
string GameConfiguration::readConfigName(MemChunk& mc)
{
	Tokenizer tz;
	tz.openMem(&mc, "gameconfig");

	// Parse text
	string token = tz.getToken();
	while (!token.IsEmpty())
	{
		// Game section
		if (S_CMPNOCASE(token, "game"))
		{
			tz.getToken();	// Skip {

			token = tz.getToken();
			while (token != "}")
			{
				// Config name
				if (S_CMPNOCASE(token, "name"))
				{
					tz.getToken();	// Skip =
					return tz.getToken();
				}

				token = tz.getToken();
			}
		}

		token = tz.getToken();
	}

	// Name not found (invalid config?)
	return "";
}

/* GameConfiguration::readBasicGameConfig
 * Parses the game configuration definition in [mc] to a gconf_t,
 * which contains only basic information about the game
 * configuration (no thing types, specials etc)
 *******************************************************************/
GameConfiguration::gconf_t GameConfiguration::readBasicGameConfig(MemChunk& mc)
{
	// Parse configuration
	Parser parser;
	parser.parseText(mc, "");
	gconf_t conf;

	// Check for game section
	ParseTreeNode* node_game = nullptr;
	for (unsigned a = 0; a < parser.parseTreeRoot()->nChildren(); a++)
	{
		ParseTreeNode* child = (ParseTreeNode*)parser.parseTreeRoot()->getChild(a);
		if (child->getType() == "game")
		{
			node_game = child;
			break;
		}
	}
	if (node_game)
	{
		// Game id
		conf.name = node_game->getName();

		// Game name
		ParseTreeNode* node_name = (ParseTreeNode*)node_game->getChild("name");
		if (node_name)
			conf.title = node_name->getStringValue();

		// Supported map formats
		ParseTreeNode* node_maps = (ParseTreeNode*)node_game->getChild("map_formats");
		if (node_maps)
		{
			for (unsigned a = 0; a < node_maps->nValues(); a++)
			{
				if (S_CMPNOCASE(node_maps->getStringValue(a), "doom"))
					conf.supported_formats[MAP_DOOM] = true;
				else if (S_CMPNOCASE(node_maps->getStringValue(a), "hexen"))
					conf.supported_formats[MAP_HEXEN] = true;
				else if (S_CMPNOCASE(node_maps->getStringValue(a), "doom64"))
					conf.supported_formats[MAP_DOOM64] = true;
				else if (S_CMPNOCASE(node_maps->getStringValue(a), "udmf"))
					conf.supported_formats[MAP_UDMF] = true;
			}
		}
		// Filters
		ParseTreeNode* node_filters = (ParseTreeNode*)node_game->getChild("filters");
		if (node_filters)
		{
			for (unsigned a = 0; a < node_filters->nValues(); a++)
				conf.filters.push_back(node_filters->getStringValue(a).Lower());
		}

	}

	return conf;
}

/* GameConfiguration::readBasicPortConfig
 * Parses the port configuration definition in [mc] to a pconf_t,
 * which contains only basic information about the port
 * configuration (no thing types, specials etc)
 *******************************************************************/
GameConfiguration::pconf_t GameConfiguration::readBasicPortConfig(MemChunk& mc)
{
	// Parse configuration
	Parser parser;
	parser.parseText(mc, "");
	pconf_t conf;

	// Check for port section
	ParseTreeNode* node_port = nullptr;
	for (unsigned a = 0; a < parser.parseTreeRoot()->nChildren(); a++)
	{
		ParseTreeNode* child = (ParseTreeNode*)parser.parseTreeRoot()->getChild(a);
		if (child->getType() == "port")
		{
			node_port = child;
			break;
		}
	}
	if (node_port)
	{
		// Port id
		conf.name = node_port->getName();

		// Port name
		ParseTreeNode* node_name = (ParseTreeNode*)node_port->getChild("name");
		if (node_name)
			conf.title = node_name->getStringValue();

		// Supported games
		ParseTreeNode* node_games = (ParseTreeNode*)node_port->getChild("games");
		if (node_games)
		{
			for (unsigned a = 0; a < node_games->nValues(); a++)
				conf.supported_games.push_back(node_games->getStringValue(a));
		}

		// Supported map formats
		ParseTreeNode* node_maps = (ParseTreeNode*)node_port->getChild("map_formats");
		if (node_maps)
		{
			for (unsigned a = 0; a < node_maps->nValues(); a++)
			{
				if (S_CMPNOCASE(node_maps->getStringValue(a), "doom"))
					conf.supported_formats[MAP_DOOM] = true;
				else if (S_CMPNOCASE(node_maps->getStringValue(a), "hexen"))
					conf.supported_formats[MAP_HEXEN] = true;
				else if (S_CMPNOCASE(node_maps->getStringValue(a), "doom64"))
					conf.supported_formats[MAP_DOOM64] = true;
				else if (S_CMPNOCASE(node_maps->getStringValue(a), "udmf"))
					conf.supported_formats[MAP_UDMF] = true;
			}
		}
	}

	return conf;
}

/* GameConfiguration::init
 * Init all GameConfiguration related stuff - load game/port
 * configurations defined in the user dir as well as the predefined
 * ones from the program resource
 *******************************************************************/
void GameConfiguration::init()
{
	// Add game configurations from user dir
	wxArrayString allfiles;
	wxDir::GetAllFiles(appPath("games", DIR_USER), &allfiles);
	for (unsigned a = 0; a < allfiles.size(); a++)
	{
		// Read config info
		MemChunk mc;
		mc.importFile(allfiles[a]);
		gconf_t conf = readBasicGameConfig(mc);

		// Add to list if valid
		if (!conf.name.IsEmpty())
		{
			conf.filename = wxFileName(allfiles[a]).GetName();
			conf.user = true;
			game_configs.push_back(conf);
		}
	}

	// Add port configurations from user dir
	allfiles.clear();
	wxDir::GetAllFiles(appPath("ports", DIR_USER), &allfiles);
	for (unsigned a = 0; a < allfiles.size(); a++)
	{
		// Read config info
		MemChunk mc;
		mc.importFile(allfiles[a]);
		pconf_t conf = readBasicPortConfig(mc);

		// Add to list if valid
		if (!conf.name.IsEmpty())
		{
			conf.filename = wxFileName(allfiles[a]).GetName();
			conf.user = true;
			port_configs.push_back(conf);
		}
	}

	// Add game configurations from program resource
	ArchiveTreeNode* dir = theArchiveManager->programResourceArchive()->getDir("config/games");
	if (dir)
	{
		for (unsigned a = 0; a < dir->numEntries(); a++)
		{
			// Read config info
			gconf_t conf = readBasicGameConfig(dir->getEntry(a)->getMCData());

			// Ignore if invalid
			if (conf.name.IsEmpty())
				continue;

			// Add to list if it doesn't already exist
			bool exists = false;
			for (unsigned b = 0; b < game_configs.size(); b++)
			{
				if (game_configs[b].name == conf.name)
				{
					exists = true;
					break;
				}
			}
			if (!exists)
			{
				conf.filename = dir->getEntry(a)->getName(true);
				conf.user = false;
				game_configs.push_back(conf);
			}
		}
	}

	// Add port configurations from program resource
	dir = theArchiveManager->programResourceArchive()->getDir("config/ports");
	if (dir)
	{
		for (unsigned a = 0; a < dir->numEntries(); a++)
		{
			// Read config info
			pconf_t conf = readBasicPortConfig(dir->getEntry(a)->getMCData());

			// Ignore if invalid
			if (conf.name.IsEmpty())
				continue;

			// Add to list if it doesn't already exist
			bool exists = false;
			for (unsigned b = 0; b < port_configs.size(); b++)
			{
				if (port_configs[b].name == conf.name)
				{
					exists = true;
					break;
				}
			}
			if (!exists)
			{
				conf.filename = dir->getEntry(a)->getName(true);
				conf.user = false;
				port_configs.push_back(conf);
			}
		}
	}

	// Sort configuration lists by title
	std::sort(game_configs.begin(), game_configs.end());
	std::sort(port_configs.begin(), port_configs.end());
	lastDefaultConfig = game_configs.size();

	// Load last configuration if any
	if (game_configuration != "")
		openConfig(game_configuration, port_configuration);
}

/* GameConfiguration::mapName
 * Returns the map name at [index] for the game configuration
 *******************************************************************/
string GameConfiguration::mapName(unsigned index)
{
	// Check index
	if (index > maps.size())
		return "";

	return maps[index].mapname;
}

/* GameConfiguration::mapInfo
 * Returns map info for the map matching [name]
 *******************************************************************/
gc_mapinfo_t GameConfiguration::mapInfo(string name)
{
	for (unsigned a = 0; a < maps.size(); a++)
	{
		if (maps[a].mapname == name)
			return maps[a];
	}

	if (maps.size() > 0)
		return maps[0];
	else
		return gc_mapinfo_t();
}

/* GameConfiguration::gameConfig
 * Returns the basic game configuration at [index]
 *******************************************************************/
GameConfiguration::gconf_t GameConfiguration::gameConfig(unsigned index)
{
	if (index >= game_configs.size())
		return gconf_none;
	else
		return game_configs[index];
}

/* GameConfiguration::gameConfig
 * Returns the basic game configuration matching [id]
 *******************************************************************/
GameConfiguration::gconf_t GameConfiguration::gameConfig(string id)
{
	for (unsigned a = 0; a < game_configs.size(); a++)
	{
		if (game_configs[a].name == id)
			return game_configs[a];
	}

	return gconf_none;
}

/* GameConfiguration::portConfig
 * Returns the basic port configuration at [index]
 *******************************************************************/
GameConfiguration::pconf_t GameConfiguration::portConfig(unsigned index)
{
	if (index >= port_configs.size())
		return pconf_none;
	else
		return port_configs[index];
}

/* GameConfiguration::portConfig
 * Returns the basic port configuration matching [id]
 *******************************************************************/
GameConfiguration::pconf_t GameConfiguration::portConfig(string id)
{
	for (unsigned a = 0; a < port_configs.size(); a++)
	{
		if (port_configs[a].name == id)
			return port_configs[a];
	}

	return pconf_none;
}

/* GameConfiguration::portSupportsGame
 * Checks if the port at index [port] supports the game [game]
 *******************************************************************/
bool GameConfiguration::portSupportsGame(unsigned port, string game)
{
	// Check index
	if (port >= port_configs.size())
		return false;

	// Check if game is supported
	for (unsigned b = 0; b < port_configs[port].supported_games.size(); b++)
	{
		if (port_configs[port].supported_games[b] == game)
			return true;
	}

	// Not supported
	return false;
}

/* GameConfiguration::gameSupportsFilter
 * Checks if [game] supports [filter]
 *******************************************************************/
bool GameConfiguration::gameSupportsFilter(string game, string filter)
{
	gconf_t config = gameConfig(game);
	if (&config == &gconf_none)
		return false;

	// Check if filter is supported
	for (unsigned b = 0; b < config.filters.size(); b++)
	{
		if (S_CMPNOCASE(config.filters[b], filter))
			return true;
	}

	// Not supported
	return false;
}

/* GameConfiguration::mapFormatSupported
 * Checks if the combination of [game] and [port] supports the map
 * format [map_format]
 *******************************************************************/
bool GameConfiguration::mapFormatSupported(int map_format, int game, int port)
{
	if (map_format < 0 || map_format > 3)
		return false;

	// Check port if one specified
	if (port >= 0 && port <= (int)port_configs.size())
		return port_configs[port].supported_formats[map_format];

	// Check game
	if (game >= 0 && game <= (int)game_configs.size())
		return game_configs[game].supported_formats[map_format];

	// Not supported
	return false;
}

/* GameConfiguration::buildConfig
 * Reads the text file at [filename], processing any #include
 * statements in the file recursively. The resulting 'expanded' text
 * is written to [out]
 *******************************************************************/
void GameConfiguration::buildConfig(string filename, string& out)
{
	// Open file
	wxTextFile file;
	if (!file.Open(filename))
		return;

	// Get file path
	wxFileName fn(filename);
	string path = fn.GetPath(true);

	// Go through line-by-line
	string line = file.GetNextLine();
	while (!file.Eof())
	{
		// Check for #include
		if (line.Lower().Trim().StartsWith("#include"))
		{
			// Get filename to include
			Tokenizer tz;
			tz.openString(line);
			tz.getToken();	// Skip #include
			string fn = tz.getToken();

			// Process the file
			buildConfig(path + fn, out);
		}
		else
			out.Append(line + "\n");

		line = file.GetNextLine();
	}
}

/* GameConfiguration::buildConfig
 * Reads the text entry [entry], processing any #include statements
 * in the entry text recursively. This will search in the resource
 * folder and archive as well as in the parent archive. The resulting
 * 'expanded' text is written to [out]
 *******************************************************************/
void GameConfiguration::buildConfig(ArchiveEntry* entry, string& out, bool use_res)
{
	// Check entry was given
	if (!entry)
		return;

	// Write entry to temp file
	string filename = appPath(entry->getName(), DIR_TEMP);
	entry->exportFile(filename);

	// Open file
	wxTextFile file;
	if (!file.Open(filename))
		return;

	// Go through line-by-line
	string line = file.GetFirstLine();
	while (!file.Eof())
	{
		// Check for #include
		if (line.Lower().Trim().StartsWith("#include"))
		{
			// Get name of entry to include
			Tokenizer tz;
			tz.openString(line);
			tz.getToken();	// Skip #include
			tz.setSpecialCharacters("");
			string inc_name = tz.getToken();
			string name = entry->getPath() + inc_name;

			// Get the entry
			bool done = false;
			ArchiveEntry* entry_inc = entry->getParent()->entryAtPath(name);
			// DECORATE paths start from the root, not from the #including entry's directory
			if (!entry_inc)
				entry_inc = entry->getParent()->entryAtPath(inc_name);
			if (entry_inc)
			{
				buildConfig(entry_inc, out);
				done = true;
			}
			else
				LOG_MESSAGE(2, "Couldn't find entry to #include: %s", name);

			// Look in resource pack
			if (use_res && !done && theArchiveManager->programResourceArchive())
			{
				name = "config/games/" + inc_name;
				entry_inc = theArchiveManager->programResourceArchive()->entryAtPath(name);
				if (entry_inc)
				{
					buildConfig(entry_inc, out);
					done = true;
				}
			}

			// Okay, we've exhausted all possibilities
			if (!done)
				wxLogMessage("Error: Attempting to #include nonexistant entry \"%s\" from entry %s", name, entry->getName());
		}
		else
			out.Append(line + "\n");

		line = file.GetNextLine();
	}

	// Delete temp file
	wxRemoveFile(filename);
}

/* GameConfiguration::readActionSpecials
 * Reads action special definitions from a parsed tree [node], using
 * [group_defaults] for default values
 *******************************************************************/
void GameConfiguration::readActionSpecials(ParseTreeNode* node, ActionSpecial* group_defaults, SpecialArgMap* shared_args)
{
	// Check if we're clearing all existing specials
	if (node->getChild("clearexisting"))
		action_specials.clear();

	// Determine current 'group'
	ParseTreeNode* group = node;
	string groupname = "";
	while (true)
	{
		if (!group || group->getName() == "action_specials")
			break;
		else
		{
			// Add current node name to group path
			groupname.Prepend(group->getName() + "/");
			group = (ParseTreeNode*)group->getParent();
		}
	}
	if (groupname.EndsWith("/"))
		groupname.RemoveLast();	// Remove last '/'

	// --- Set up group default properties ---
	bool own_shared_args = false;
	if (!shared_args)
	{
		shared_args = new SpecialArgMap();
		own_shared_args = true;
	}

	ActionSpecial* as_defaults = new ActionSpecial();
	if (group_defaults) as_defaults->copy(group_defaults);
	as_defaults->parse(node, shared_args);

	// --- Go through all child nodes ---
	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		ParseTreeNode* child = (ParseTreeNode*)node->getChild(a);

		// Check for 'group'
		if (S_CMPNOCASE(child->getType(), "group"))
			readActionSpecials(child, as_defaults, shared_args);

		// Predeclared argument, for action specials that share the same
		// complex argument
		else if (S_CMPNOCASE(child->getType(), "arg"))
			ActionSpecial::parseArg(child, shared_args, (*shared_args)[child->getName()]);

		// Action special
		else if (S_CMPNOCASE(child->getType(), "special"))
		{
			// Get special id as integer
			long special;
			child->getName().ToLong(&special);

			// Create action special object if needed
			if (!action_specials[special].special)
			{
				action_specials[special].special = new ActionSpecial();
				action_specials[special].number = special;
				action_specials[special].index = action_specials.size();
			}

			// Reset the action special (in case it's being redefined for whatever reason)
			action_specials[special].special->reset();

			// Apply group defaults
			action_specials[special].special->copy(as_defaults);
			action_specials[special].special->group = groupname;

			action_specials[special].special->parse(child, shared_args);
		}
	}

	delete as_defaults;
	if (own_shared_args)
		delete shared_args;
}

/* GameConfiguration::readThingTypes
 * Reads thing type definitions from a parsed tree [node], using
 * [group_defaults] for default values
 *******************************************************************/
void GameConfiguration::readThingTypes(ParseTreeNode* node, ThingType* group_defaults)
{
	// Check if we're clearing all existing specials
	if (node->getChild("clearexisting"))
		thing_types.clear();

	// --- Determine current 'group' ---
	ParseTreeNode* group = node;
	string groupname = "";
	while (true)
	{
		if (!group || group->getName() == "thing_types")
			break;
		else
		{
			// Add current node name to group path
			groupname.Prepend(group->getName() + "/");
			group = (ParseTreeNode*)group->getParent();
		}
	}
	if (groupname.EndsWith("/"))
		groupname.RemoveLast();	// Remove last '/'


	// --- Set up group default properties ---
	ParseTreeNode* child = nullptr;
	ThingType* tt_defaults = new ThingType();
	tt_defaults->copy(group_defaults);
	tt_defaults->parse(node);
	tt_defaults->group = groupname;
	tt_group_defaults.push_back(tt_defaults);


	// --- Go through all child nodes ---
	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		child = (ParseTreeNode*)node->getChild(a);

		// Check for 'group'
		if (S_CMPNOCASE(child->getType(), "group"))
			readThingTypes(child, tt_defaults);

		// Thing type
		else if (S_CMPNOCASE(child->getType(), "thing"))
		{
			// Get thing type as integer
			long type;
			child->getName().ToLong(&type);

			// Create thing type object if needed
			if (!thing_types[type].type)
			{
				thing_types[type].type = new ThingType();
				thing_types[type].index = thing_types.size();
			}

			// Reset the thing type (in case it's being redefined for whatever reason)
			thing_types[type].type->reset();

			// Apply group defaults
			thing_types[type].type->copy(tt_defaults);
			thing_types[type].type->group = groupname;

			// Check for simple definition
			if (child->isLeaf())
				thing_types[type].type->name = child->getStringValue();
			else
				thing_types[type].type->parse(child);	// Extended definition
		}
	}

	//delete tt_defaults;
}

/* GameConfiguration::readUDMFProperties
 * Reads UDMF property definitions from a parsed tree [node] into
 * [plist]
 *******************************************************************/
void GameConfiguration::readUDMFProperties(ParseTreeNode* block, UDMFPropMap& plist)
{
	// Read block properties
	for (unsigned a = 0; a < block->nChildren(); a++)
	{
		ParseTreeNode* group = (ParseTreeNode*)block->getChild(a);

		// Group definition
		if (S_CMPNOCASE(group->getType(), "group"))
		{
			string groupname = group->getName();

			// Go through the group
			for (unsigned b = 0; b < group->nChildren(); b++)
			{
				ParseTreeNode* def = (ParseTreeNode*)group->getChild(b);

				if (S_CMPNOCASE(def->getType(), "property"))
				{
					// Create property if needed
					if (!plist[def->getName()].property)
						plist[def->getName()].property = new UDMFProperty();

					// Parse group defaults
					plist[def->getName()].property->parse(group, groupname);

					// Parse definition
					plist[def->getName()].property->parse(def, groupname);

					// Set index
					plist[def->getName()].index = plist.size();
				}
			}
		}
	}
}

#define READ_BOOL(obj, field)	else if (S_CMPNOCASE(node->getName(), #field)) \
									obj = node->getBoolValue()

/* GameConfiguration::readGameSection
 * Reads a game or port definition from a parsed tree [node]. If
 * [port_section] is true it is a port definition
 *******************************************************************/
void GameConfiguration::readGameSection(ParseTreeNode* node_game, bool port_section)
{
	for (unsigned a = 0; a < node_game->nChildren(); a++)
	{
		ParseTreeNode* node = (ParseTreeNode*)node_game->getChild(a);

		//// Game name
		//if (S_CMPNOCASE(node->getName(), "name"))
		//	this->name = node->getStringValue();

		// Allow any map name
		if (S_CMPNOCASE(node->getName(), "map_name_any"))
			any_map_name = node->getBoolValue();

		// Map formats
		else if (S_CMPNOCASE(node->getName(), "map_formats"))
		{
			// Reset supported formats
			for (unsigned f = 0; f < 4; f++)
				map_formats[f] = false;

			// Go through values
			for (unsigned v = 0; v < node->nValues(); v++)
			{
				if (S_CMPNOCASE(node->getStringValue(v), "doom"))
				{
					map_formats[MAP_DOOM] = true;
				}
				else if (S_CMPNOCASE(node->getStringValue(v), "hexen"))
				{
					map_formats[MAP_HEXEN] = true;
				}
				else if (S_CMPNOCASE(node->getStringValue(v), "doom64"))
				{
					map_formats[MAP_DOOM64] = true;
				}
				else if (S_CMPNOCASE(node->getStringValue(v), "udmf"))
				{
					map_formats[MAP_UDMF] = true;
				}
				else
					wxLogMessage("Warning: Unknown/unsupported map format \"%s\"", node->getStringValue(v));
			}
		}

		// Boom extensions
		else if (S_CMPNOCASE(node->getName(), "boom"))
			boom = node->getBoolValue();
		else if (S_CMPNOCASE(node->getName(), "boom_sector_flag_start"))
			boom_sector_flag_start = node->getIntValue();

		// UDMF namespace
		else if (S_CMPNOCASE(node->getName(), "udmf_namespace"))
			udmf_namespace = node->getStringValue();

		// Mixed Textures and Flats
		else if (S_CMPNOCASE(node->getName(), "mix_tex_flats"))
			mix_tex_flats = node->getBoolValue();

		// TX_/'textures' namespace enabled
		else if (S_CMPNOCASE(node->getName(), "tx_textures"))
			tx_textures = node->getBoolValue();

		// Sky flat
		else if (S_CMPNOCASE(node->getName(), "sky_flat"))
			sky_flat = node->getStringValue();

		// Scripting language
		else if (S_CMPNOCASE(node->getName(), "script_language"))
			script_language = node->getStringValue().Lower();

		// Light levels interval
		else if (S_CMPNOCASE(node->getName(), "light_level_interval"))
			setLightLevelInterval(node->getIntValue());

		// Long names
		else if (S_CMPNOCASE(node->getName(), "long_names"))
			allow_long_names = node->getBoolValue();

		READ_BOOL(udmf_slopes, udmf_slopes); // UDMF slopes
		READ_BOOL(udmf_flat_lighting, udmf_flat_lighting); // UDMF flat lighting
		READ_BOOL(udmf_flat_panning, udmf_flat_panning); // UDMF flat panning
		READ_BOOL(udmf_flat_rotation, udmf_flat_rotation); // UDMF flat rotation
		READ_BOOL(udmf_flat_scaling, udmf_flat_scaling); // UDMF flat scaling
		READ_BOOL(udmf_line_transparency, udmf_line_transparency); // UDMF line transparency
		READ_BOOL(udmf_sector_color, udmf_sector_color); // UDMF sector color
		READ_BOOL(udmf_sector_fog, udmf_sector_fog); // UDMF sector fog
		READ_BOOL(udmf_side_lighting, udmf_side_lighting); // UDMF per-sidedef lighting
		READ_BOOL(udmf_side_midtex_wrapping, udmf_side_midtex_wrapping); // UDMF per-sidetex midtex wrapping
		READ_BOOL(udmf_side_scaling, udmf_side_scaling); // UDMF per-sidedef scaling
		READ_BOOL(udmf_texture_scaling, udmf_texture_scaling); // UDMF per-texture scaling
		READ_BOOL(udmf_texture_offsets, udmf_texture_offsets); // UDMF per-texture offsets

		// Defaults section
		else if (S_CMPNOCASE(node->getName(), "defaults"))
		{
			// Go through defaults blocks
			for (unsigned b = 0; b < node->nChildren(); b++)
			{
				ParseTreeNode* block = (ParseTreeNode*)node->getChild(b);

				// Linedef defaults
				if (S_CMPNOCASE(block->getName(), "linedef"))
				{
					for (unsigned c = 0; c < block->nChildren(); c++)
					{
						ParseTreeNode* def = (ParseTreeNode*)block->getChild(c);
						if (S_CMPNOCASE(def->getType(), "udmf"))
							defaults_line_udmf[def->getName()] = def->getValue();
						else	
							defaults_line[def->getName()] = def->getValue();
					}
				}

				// Sidedef defaults
				else if (S_CMPNOCASE(block->getName(), "sidedef"))
				{
					for (unsigned c = 0; c < block->nChildren(); c++)
					{
						ParseTreeNode* def = (ParseTreeNode*)block->getChild(c);
						if (S_CMPNOCASE(def->getType(), "udmf"))
							defaults_side_udmf[def->getName()] = def->getValue();
						else
							defaults_side[def->getName()] = def->getValue();
					}
				}

				// Sector defaults
				else if (S_CMPNOCASE(block->getName(), "sector"))
				{
					for (unsigned c = 0; c < block->nChildren(); c++)
					{
						ParseTreeNode* def = (ParseTreeNode*)block->getChild(c);
						if (S_CMPNOCASE(def->getType(), "udmf"))
							defaults_sector_udmf[def->getName()] = def->getValue();
						else
							defaults_sector[def->getName()] = def->getValue();
					}
				}

				// Thing defaults
				else if (S_CMPNOCASE(block->getName(), "thing"))
				{
					for (unsigned c = 0; c < block->nChildren(); c++)
					{
						ParseTreeNode* def = (ParseTreeNode*)block->getChild(c);
						if (S_CMPNOCASE(def->getType(), "udmf"))
							defaults_thing_udmf[def->getName()] = def->getValue();
						else
							defaults_thing[def->getName()] = def->getValue();
					}
				}

				else
					wxLogMessage("Unknown defaults block \"%s\"", block->getName());
			}
		}

		// Maps section (game section only)
		else if (S_CMPNOCASE(node->getName(), "maps") && !port_section)
		{
			// Go through map blocks
			for (unsigned b = 0; b < node->nChildren(); b++)
			{
				ParseTreeNode* block = (ParseTreeNode*)node->getChild(b);

				// Map definition
				if (S_CMPNOCASE(block->getType(), "map"))
				{
					gc_mapinfo_t map;
					map.mapname = block->getName();

					// Go through map properties
					for (unsigned c = 0; c < block->nChildren(); c++)
					{
						ParseTreeNode* prop = (ParseTreeNode*)block->getChild(c);

						// Sky texture
						if (S_CMPNOCASE(prop->getName(), "sky"))
						{
							// Primary sky texture
							map.sky1 = prop->getStringValue();

							// Secondary sky texture
							if (prop->nValues() > 1)
								map.sky2 = prop->getStringValue(1);
						}
					}

					maps.push_back(map);
				}
			}
		}
	}
}

/* GameConfiguration::readConfiguration
 * Reads a full game configuration from [cfg]
 *******************************************************************/
bool GameConfiguration::readConfiguration(string& cfg, string source, uint8_t format, bool ignore_game, bool clear)
{
	// Clear current configuration
	if (clear)
	{
		setDefaults();
		action_specials.clear();
		thing_types.clear();
		flags_thing.clear();
		flags_line.clear();
		sector_types.clear();
		udmf_vertex_props.clear();
		udmf_linedef_props.clear();
		udmf_sidedef_props.clear();
		udmf_sector_props.clear();
		udmf_thing_props.clear();
		for (unsigned a = 0; a < tt_group_defaults.size(); a++)
			delete tt_group_defaults[a];
		tt_group_defaults.clear();
	}

	// Parse the full configuration
	Parser parser;
	switch (format)
	{
	case MAP_DOOM:		parser.define("MAP_DOOM");		break;
	case MAP_HEXEN:		parser.define("MAP_HEXEN");		break;
	case MAP_DOOM64:	parser.define("MAP_DOOM64");	break;
	case MAP_UDMF:		parser.define("MAP_UDMF");		break;
	default:			parser.define("MAP_UNKNOWN");	break;
	}
	parser.parseText(cfg, source);

	// Process parsed data
	ParseTreeNode* base = parser.parseTreeRoot();

	// Read game/port section(s) if needed
	ParseTreeNode* node_game = nullptr;
	ParseTreeNode* node_port = nullptr;
	if (!ignore_game)
	{
		// 'Game' section (this is required for it to be a valid game configuration, shouldn't be missing)
		for (unsigned a = 0; a < base->nChildren(); a++)
		{
			ParseTreeNode* child = (ParseTreeNode*)base->getChild(a);
			if (child->getType() == "game")
			{
				node_game = child;
				break;
			}
		}
		if (!node_game)
		{
			wxLogMessage("No game section found, something is pretty wrong.");
			return false;
		}
		readGameSection(node_game, false);

		// 'Port' section
		for (unsigned a = 0; a < base->nChildren(); a++)
		{
			ParseTreeNode* child = (ParseTreeNode*)base->getChild(a);
			if (child->getType() == "port")
			{
				node_port = child;
				break;
			}
		}
		if (node_port)
			readGameSection(node_port, true);
	}

	// Go through all other config sections
	ParseTreeNode* node = nullptr;
	for (unsigned a = 0; a < base->nChildren(); a++)
	{
		node = (ParseTreeNode*)base->getChild(a);

		// Skip read game/port section
		if (node == node_game || node == node_port)
			continue;

		// A TC configuration may override the base game
		if (S_CMPNOCASE(node->getName(), "game"))
			readGameSection(node, false);

		// Action specials section
		else if (S_CMPNOCASE(node->getName(), "action_specials"))
			readActionSpecials(node);

		// Thing types section
		else if (S_CMPNOCASE(node->getName(), "thing_types"))
			readThingTypes(node);

		// Line flags section
		else if (S_CMPNOCASE(node->getName(), "line_flags"))
		{
			for (unsigned c = 0; c < node->nChildren(); c++)
			{
				ParseTreeNode* value = (ParseTreeNode*)node->getChild(c);

				// Check for 'flag' type
				if (!(S_CMPNOCASE(value->getType(), "flag")))
					continue;

				unsigned long flag_val;
				string flag_name, flag_udmf;

				if (value->nValues() == 0)
				{
					// Full definition
					flag_name = value->getName();

					for (unsigned v = 0; v < value->nChildren(); v++)
					{
						ParseTreeNode* prop = (ParseTreeNode*)value->getChild(v);

						if (S_CMPNOCASE(prop->getName(), "value"))
							flag_val = prop->getIntValue();
						else if (S_CMPNOCASE(prop->getName(), "udmf"))
						{
							for (unsigned u = 0; u < prop->nValues(); u++)
								flag_udmf += prop->getStringValue(u) + " ";
							flag_udmf.RemoveLast(1);
						}
					}
				}
				else
				{
					// Short definition
					value->getName().ToULong(&flag_val);
					flag_name = value->getStringValue();
				}

				// Check if the flag value already exists
				bool exists = false;
				for (unsigned f = 0; f < flags_line.size(); f++)
				{
					if (flags_line[f].flag == flag_val)
					{
						exists = true;
						flags_line[f].name = flag_name;
						break;
					}
				}

				// Add flag otherwise
				if (!exists)
					flags_line.push_back(flag_t(flag_val, flag_name, flag_udmf));
			}
		}

		// Line triggers section
		else if (S_CMPNOCASE(node->getName(), "line_triggers"))
		{
			for (unsigned c = 0; c < node->nChildren(); c++)
			{
				ParseTreeNode* value = (ParseTreeNode*)node->getChild(c);

				// Check for 'trigger' type
				if (!(S_CMPNOCASE(value->getType(), "trigger")))
					continue;

				long flag_val;
				string flag_name, flag_udmf;

				if (value->nValues() == 0)
				{
					// Full definition
					flag_name = value->getName();

					for (unsigned v = 0; v < value->nChildren(); v++)
					{
						ParseTreeNode* prop = (ParseTreeNode*)value->getChild(v);

						if (S_CMPNOCASE(prop->getName(), "value"))
							flag_val = prop->getIntValue();
						else if (S_CMPNOCASE(prop->getName(), "udmf"))
						{
							for (unsigned u = 0; u < prop->nValues(); u++)
								flag_udmf += prop->getStringValue(u) + " ";
							flag_udmf.RemoveLast(1);
						}
					}
				}
				else
				{
					// Short definition
					value->getName().ToLong(&flag_val);
					flag_name = value->getStringValue();
				}

				// Check if the trigger value already exists
				bool exists = false;
				for (unsigned f = 0; f < triggers_line.size(); f++)
				{
					if (triggers_line[f].flag == flag_val)
					{
						exists = true;
						triggers_line[f].name = flag_name;
						break;
					}
				}

				// Add trigger otherwise
				if (!exists)
					triggers_line.push_back(flag_t(flag_val, flag_name, flag_udmf));
			}
		}

		// Thing flags section
		else if (S_CMPNOCASE(node->getName(), "thing_flags"))
		{
			for (unsigned c = 0; c < node->nChildren(); c++)
			{
				ParseTreeNode* value = (ParseTreeNode*)node->getChild(c);

				// Check for 'flag' type
				if (!(S_CMPNOCASE(value->getType(), "flag")))
					continue;

				long flag_val;
				string flag_name, flag_udmf;

				if (value->nValues() == 0)
				{
					// Full definition
					flag_name = value->getName();

					for (unsigned v = 0; v < value->nChildren(); v++)
					{
						ParseTreeNode* prop = (ParseTreeNode*)value->getChild(v);

						if (S_CMPNOCASE(prop->getName(), "value"))
							flag_val = prop->getIntValue();
						else if (S_CMPNOCASE(prop->getName(), "udmf"))
						{
							for (unsigned u = 0; u < prop->nValues(); u++)
								flag_udmf += prop->getStringValue(u) + " ";
							flag_udmf.RemoveLast(1);
						}
					}
				}
				else
				{
					// Short definition
					value->getName().ToLong(&flag_val);
					flag_name = value->getStringValue();
				}

				// Check if the flag value already exists
				bool exists = false;
				for (unsigned f = 0; f < flags_thing.size(); f++)
				{
					if (flags_thing[f].flag == flag_val)
					{
						exists = true;
						flags_thing[f].name = flag_name;
						break;
					}
				}

				// Add flag otherwise
				if (!exists)
					flags_thing.push_back(flag_t(flag_val, flag_name, flag_udmf));
			}
		}

		// Sector types section
		else if (S_CMPNOCASE(node->getName(), "sector_types"))
		{
			for (unsigned c = 0; c < node->nChildren(); c++)
			{
				ParseTreeNode* value = (ParseTreeNode*)node->getChild(c);

				// Check for 'type'
				if (!(S_CMPNOCASE(value->getType(), "type")))
					continue;

				long type_val;
				value->getName().ToLong(&type_val);

				// Check if the sector type already exists
				bool exists = false;
				for (unsigned t = 0; t < sector_types.size(); t++)
				{
					if (sector_types[t].type == type_val)
					{
						exists = true;
						sector_types[t].name = value->getStringValue();
						break;
					}
				}

				// Add type otherwise
				if (!exists)
					sector_types.push_back(sectype_t(type_val, value->getStringValue()));
			}
		}

		// UDMF properties section
		else if (S_CMPNOCASE(node->getName(), "udmf_properties"))
		{
			// Parse vertex block properties (if any)
			ParseTreeNode* block = (ParseTreeNode*)node->getChild("vertex");
			if (block) readUDMFProperties(block, udmf_vertex_props);

			// Parse linedef block properties (if any)
			block = (ParseTreeNode*)node->getChild("linedef");
			if (block) readUDMFProperties(block, udmf_linedef_props);

			// Parse sidedef block properties (if any)
			block = (ParseTreeNode*)node->getChild("sidedef");
			if (block) readUDMFProperties(block, udmf_sidedef_props);

			// Parse sector block properties (if any)
			block = (ParseTreeNode*)node->getChild("sector");
			if (block) readUDMFProperties(block, udmf_sector_props);

			// Parse thing block properties (if any)
			block = (ParseTreeNode*)node->getChild("thing");
			if (block) readUDMFProperties(block, udmf_thing_props);
		}

		// Unknown/unexpected section
		else
			wxLogMessage("Warning: Unexpected game configuration section \"%s\", skipping", node->getName());
	}

	return true;
}

/* GameConfiguration::openConfig
 * Opens the full game configuration [game]+[port], either from the
 * user dir or program resource
 *******************************************************************/
bool GameConfiguration::openConfig(string game, string port, uint8_t format)
{
	string full_config;

	// Get game configuration as string
	for (unsigned a = 0; a < game_configs.size(); a++)
	{
		if (game_configs[a].name == game)
		{
			if (game_configs[a].user)
			{
				// Config is in user dir
				string filename = appPath("games/", DIR_USER) + game_configs[a].filename + ".cfg";
				if (wxFileExists(filename))
					buildConfig(filename, full_config);
				else
				{
					wxLogMessage("Error: Game configuration file \"%s\" not found", filename);
					return false;
				}
			}
			else
			{
				// Config is in program resource
				string epath = S_FMT("config/games/%s.cfg", game_configs[a].filename);
				Archive* archive = theArchiveManager->programResourceArchive();
				ArchiveEntry* entry = archive->entryAtPath(epath);
				if (entry)
					buildConfig(entry, full_config);
			}
		}
	}

	// Append port configuration (if specified)
	if (!port.IsEmpty())
	{
		full_config += "\n\n";

		// Get port config
		for (unsigned a = 0; a < port_configs.size(); a++)
		{
			pconf_t& conf = port_configs[a];
			if (conf.name == port)
			{
				// Check the port supports this game
				bool supported = false;
				for (unsigned b = 0; b < conf.supported_games.size(); b++)
				{
					if (conf.supported_games[b] == game)
					{
						supported = true;
						break;
					}
				}
				if (!supported)
					continue;

				if (conf.user)
				{
					// Config is in user dir
					string filename = appPath("games/", DIR_USER) + conf.filename + ".cfg";
					if (wxFileExists(filename))
						buildConfig(filename, full_config);
					else
					{
						wxLogMessage("Error: Port configuration file \"%s\" not found", filename);
						return false;
					}
				}
				else
				{
					// Config is in program resource
					string epath = S_FMT("config/ports/%s.cfg", conf.filename);
					Archive* archive = theArchiveManager->programResourceArchive();
					ArchiveEntry* entry = archive->entryAtPath(epath);
					if (entry)
						buildConfig(entry, full_config);
				}
			}
		}
	}

	if (debug_configuration)
	{
		wxFile test("full.cfg", wxFile::write);
		test.Write(full_config);
		test.Close();
	}

	// Read fully built configuration
	bool ok = true;
	if (readConfiguration(full_config, "full.cfg", format))
	{
		current_game = game;
		current_port = port;
		game_configuration = game;
		port_configuration = port;
		wxLogMessage("Read game configuration \"%s\" + \"%s\"", current_game, current_port);
	}
	else
	{
		wxLogMessage("Error reading game configuration, not loaded");
		ok = false;
	}

	// Read any embedded configurations in resource archives
	Archive::search_options_t opt;
	opt.match_name = "sladecfg";
	vector<ArchiveEntry*> cfg_entries = theArchiveManager->findAllResourceEntries(opt);
	for (unsigned a = 0; a < cfg_entries.size(); a++)
	{
		// Log message
		Archive* parent = cfg_entries[a]->getParent();
		if (parent)
			wxLogMessage("Reading SLADECFG in %s", parent->getFilename());

		// Read embedded config
		string config = wxString::FromAscii(cfg_entries[a]->getData(), cfg_entries[a]->getSize());
		if (!readConfiguration(config, cfg_entries[a]->getName(), format, true, false))
			wxLogMessage("Error reading embedded game configuration, not loaded");
	}

	return ok;
}

/* GameConfiguration::actionSpecial
 * Returns the action special definition for [id]
 *******************************************************************/
ActionSpecial* GameConfiguration::actionSpecial(unsigned id)
{
	as_t& as = action_specials[id];
	if (as.special)
	{
		return as.special;
	}
	else if (boom && id >= 0x2f80)
	{
		if ((id & 7) >= 6)
			return &as_generalized_m;
		else
			return &as_generalized_s;
	}
	else
	{
		return &as_unknown;
	}
}

/* GameConfiguration::actionSpecialName
 * Returns the action special name for [special], if any
 *******************************************************************/
string GameConfiguration::actionSpecialName(int special)
{
	// Check special id is valid
	if (special < 0)
		return "Unknown";
	else if (special == 0)
		return "None";

	if (action_specials[special].special)
		return action_specials[special].special->getName();
	else if (special >= 0x2F80 && boom)
		return BoomGenLineSpecial::parseLineType(special);
	else
		return "Unknown";
}

/* GameConfiguration::allActionSpecials
 * Returns a list of all action specials defined in the configuration
 *******************************************************************/
vector<as_t> GameConfiguration::allActionSpecials()
{
	vector<as_t> ret;

	// Build list
	ASpecialMap::iterator i = action_specials.begin();
	while (i != action_specials.end())
	{
		if (i->second.special)
		{
			as_t as(i->second);
			as.number = i->first;
			ret.push_back(as);
		}

		i++;
	}

	return ret;
}

/* GameConfiguration::thingType
 * Returns the thing type definition for [type]
 *******************************************************************/
ThingType* GameConfiguration::thingType(unsigned type)
{
	tt_t& ttype = thing_types[type];
	if (ttype.type)
		return ttype.type;
	else
		return &ttype_unknown;
}

/* GameConfiguration::allThingTypes
 * Returns a list of all thing types defined in the configuration
 *******************************************************************/
vector<tt_t> GameConfiguration::allThingTypes()
{
	vector<tt_t> ret;

	ThingTypeMap::iterator i = thing_types.begin();
	while (i != thing_types.end())
	{
		if (i->second.type)
		{
			tt_t tt(i->second.type);
			tt.number = i->first;
			ret.push_back(tt);
		}

		i++;
	}

	return ret;
}

/* GameConfiguration::thingFlag
 * Returns the name of the thing flag at [index]
 *******************************************************************/
string GameConfiguration::thingFlag(unsigned index)
{
	// Check index
	if (index >= flags_thing.size())
		return "";

	return flags_thing[index].name;
}

/* GameConfiguration::thingFlagSet
 * Returns true if the flag at [index] is set for [thing]
 *******************************************************************/
bool GameConfiguration::thingFlagSet(unsigned index, MapThing* thing)
{
	// Check index
	if (index >= flags_thing.size())
		return false;

	// Check if flag is set
	unsigned long flags = thing->intProperty("flags");
	if (flags & flags_thing[index].flag)
		return true;
	else
		return false;
}

/* GameConfiguration::thingFlagSet
 * Returns true if the flag matching [flag] is set for [thing]
 *******************************************************************/
bool GameConfiguration::thingFlagSet(string flag, MapThing* thing, int map_format)
{
	// If UDMF, just get the bool value
	if (map_format == MAP_UDMF)
		return thing->boolProperty(flag);

	// Get current flags
	unsigned long flags = thing->intProperty("flags");

	// Iterate through flags
	for (size_t i = 0; i < flags_thing.size(); ++i)
	{
		if (flags_thing[i].udmf == flag)
			return !!(flags & flags_thing[i].flag);
	}
	LOG_MESSAGE(2, "Flag %s does not exist in this configuration", flag);
	return false;
}

/* GameConfiguration::thingBasicFlagSet
 * Returns true if the basic flag matching [flag] is set for [thing]
 *******************************************************************/
bool GameConfiguration::thingBasicFlagSet(string flag, MapThing* thing, int map_format)
{
	// If UDMF, just get the bool value
	if (map_format == MAP_UDMF)
		return thing->boolProperty(flag);

	// Get current flags
	unsigned long flags = thing->intProperty("flags");

	// Hexen-style flags in Hexen-format maps
	bool hexen = map_format == MAP_HEXEN;

	// Easy Skill
	if (flag == "skill2" || flag == "skill1")
		return !!(flags & 1);

	// Medium Skill
	else if (flag == "skill3")
		return !!(flags & 2);

	// Hard Skill
	else if (flag == "skill4" || flag == "skill5")
		return !!(flags & 4);

	// Game mode flags
	else if (flag == "single")
	{
		// Single Player
		if (hexen)
			return !!(flags & 256);
		// *Not* Multiplayer
		else
			return !(flags & 16);
	}
	else if (flag == "coop")
	{
		// Coop
		if (hexen)
			return !!(flags & 512);
		// *Not* Not In Coop
		else if (isBoom())
			return !(flags & 64);
		else
			return true;
	}
	else if (flag == "dm")
	{
		// Deathmatch
		if (hexen)
			return !!(flags & 1024);
		// *Not* Not In DM
		else if (isBoom())
			return !(flags & 32);
		else
			return true;
	}

	// Hexen class flags
	else if (hexen && flag.StartsWith("class"))
	{
		// Fighter
		if (flag == "class1")
			return !!(flags & 32);
		// Cleric
		else if (flag == "class2")
			return !!(flags & 64);
		// Mage
		else if (flag == "class3")
			return !!(flags & 128);
	}

	// Not basic
	return thingFlagSet(flag, thing, map_format);
}

/* GameConfiguration::thingFlagsString
 * Returns a string of all thing flags set in [flags]
 *******************************************************************/
string GameConfiguration::thingFlagsString(int flags)
{
	// Check against all flags
	string ret = "";
	for (unsigned a = 0; a < flags_thing.size(); a++)
	{
		if (flags & flags_thing[a].flag)
		{
			// Add flag name to string
			ret += flags_thing[a].name;
			ret += ", ";
		}
	}

	// Remove ending ', ' if needed
	if (ret.Length() > 0)
		ret.RemoveLast(2);
	else
		return "None";

	return ret;
}

/* GameConfiguration::setThingFlag
 * Sets thing flag at [index] for [thing]. If [set] is false, the
 * flag is unset
 *******************************************************************/
void GameConfiguration::setThingFlag(unsigned index, MapThing* thing, bool set)
{
	// Check index
	if (index >= flags_thing.size())
		return;

	// Determine new flags value
	unsigned long flags = thing->intProperty("flags");
	if (set)
		flags |= flags_thing[index].flag;
	else
		flags = (flags & ~flags_thing[index].flag);

	// Update thing flags
	thing->setIntProperty("flags", flags);
}

/* GameConfiguration::setThingFlag
 * Sets thing flag matching [flag] (UDMF name) for [thing]. If [set]
 * is false, the flag is unset
 *******************************************************************/
void GameConfiguration::setThingFlag(string flag, MapThing* thing, int map_format, bool set)
{
	// If UDMF, just set the bool value
	if (map_format == MAP_UDMF)
	{
		thing->setBoolProperty(flag, set);
		return;
	}

	// Iterate through flags
	unsigned long flag_val = 0;
	for (size_t i = 0; i < flags_thing.size(); ++i)
	{
		if (flags_thing[i].udmf == flag)
		{
			flag_val = flags_thing[i].flag;
			break;
		}
	}

	if (flag_val == 0)
	{
		LOG_MESSAGE(2, "Flag %s does not exist in this configuration", flag);
		return;
	}

	// Determine new flags value
	unsigned long flags = thing->intProperty("flags");
	if (set)
		flags |= flag_val;
	else
		flags = (flags & ~flag_val);

	// Update thing flags
	thing->setIntProperty("flags", flags);
}

/* GameConfiguration::setThingBasicFlag
 * Sets thing basic flag matching [flag] for [thing]. If [set] is
 * false, the flag is unset
 *******************************************************************/
void GameConfiguration::setThingBasicFlag(string flag, MapThing* thing, int map_format, bool set)
{
	// If UDMF, just set the bool value
	if (map_format == MAP_UDMF)
	{
		thing->setBoolProperty(flag, set);
		return;
	}

	// Seek flag value
	unsigned long flag_val = 0;

	// ZDoom uses Hexen-style flags
	bool hexen = (currentGame() == "hexen") || (currentPort() == "zdoom");

	// Easy Skill
	if (flag == "skill2" || flag == "skill1")
		flag_val = 1;

	// Medium Skill
	else if (flag == "skill3")
		flag_val = 2;

	// Hard Skill
	else if (flag == "skill4" || flag == "skill5")
		flag_val = 4;

	// Game mode flags
	else if (flag == "single")
	{
		// Single Player
		if (hexen)
			flag_val = 256;
		// *Not* Multiplayer
		else
		{
			flag_val = 16;
			set = !set;
		}
	}
	else if (flag == "coop")
	{
		// Coop
		if (hexen)
			flag_val = 512;
		// *Not* Not In Coop
		else if (isBoom())
		{
			flag_val = 64;
			set = !set;
		}
		// Multiplayer
		else
			flag_val = 0;
	}
	else if (flag == "dm")
	{
		// Deathmatch
		if (hexen)
			flag_val = 1024;
		// *Not* Not In DM
		else if (isBoom())
		{
			flag_val = 32;
			set = !set;
		}
		// Multiplayer
		else
			flag_val = 0;
	}

	// Hexen class flags
	else if (flag.StartsWith("class"))
	{
		if (hexen)
		{
			// Fighter
			if (flag == "class1")
				flag_val = 32;
			// Cleric
			else if (flag == "class2")
				flag_val = 64;
			// Mage
			else if (flag == "class3")
				flag_val = 128;
		}
		else
			flag_val = 0;
	}

	if (flag_val)
	{
		// Determine new flags value
		unsigned long flags = thing->intProperty("flags");
		if (set)
			flags |= flag_val;
		else
			flags = (flags & ~flag_val);

		// Update thing flags
		thing->setIntProperty("flags", flags);
		return;
	}

	// Not basic
	thingFlagSet(flag, thing, map_format);
}

// This is used to have the same priority order as DB2
// Idle, See, Inactive, Spawn, first defined
enum StateSprites
{
	SS_FIRSTDEFINED = 1,
	SS_SPAWN,
	SS_INACTIVE,
	SS_SEE,
	SS_IDLE,
};

/* GameConfiguration::parseDecorateDefs
 * Parses all DECORATE thing definitions in [archive]
 *******************************************************************/
bool GameConfiguration::parseDecorateDefs(Archive* archive)
{
	if (!archive)
		return false;

	// Get base decorate file
	Archive::search_options_t opt;
	opt.match_name = "decorate";
	//opt.match_type = EntryType::getType("text");
	opt.ignore_ext = true;
	vector<ArchiveEntry*> decorate_entries = archive->findAll(opt);
	if (decorate_entries.empty())
		return false;

	LOG_MESSAGE(2, "Parsing DECORATE entries found in archive %s", archive->getFilename());

	//ArchiveEntry* decorate_base = archive->getEntry("DECORATE", true);
	//if (!decorate_base)
	//	return false;

	// Build full definition string
	string full_defs;
	for (unsigned a = 0; a < decorate_entries.size(); a++)
		buildConfig(decorate_entries[a], full_defs, false);
	//buildConfig(decorate_base, full_defs, false);

	// Init tokenizer
	Tokenizer tz;
	tz.setSpecialCharacters(":,{}");
	tz.enableDecorate(true);
	tz.openString(full_defs);

	// --- Parse ---
	string token = tz.getToken();
	while (!token.empty())
	{
		// Check for actor definition
		if (S_CMPNOCASE(token, "actor"))
		{
			// Get actor name
			string name = tz.getToken();

			// Check for inheritance
			string next = tz.peekToken();
			if (next == ":")
			{
				tz.skipToken(); // Skip :
				tz.skipToken(); // Skip parent actor
				next = tz.peekToken();
			}

			// Check for replaces
			if (S_CMPNOCASE(next, "replaces"))
			{
				tz.skipToken(); // Skip replaces
				tz.skipToken(); // Skip replace actor
			}

			// Skip "native" keyword if present
			if (S_CMPNOCASE(tz.peekToken(), "native"))
				tz.skipToken();

			// Check for no editor number (ie can't be placed in the map)
			if (tz.peekToken() == "{")
			{
				LOG_MESSAGE(3, "Not adding actor %s, no editor number", name);

				// Skip actor definition
				tz.skipToken();
				tz.skipSection("{", "}");
			}
			else
			{
				// Read editor number
				int type;
				tz.getInteger(&type);
				string group;
				PropertyList found_props;
				bool title_given = false;
				bool sprite_given = false;
				bool group_given = false;
				bool filters_present = false;
				bool available = false;

				// Skip "native" keyword if present
				if (S_CMPNOCASE(tz.peekToken(), "native"))
					tz.skipToken();

				// Check for actor definition open
				token = tz.getToken();
				if (token == "{")
				{
					token = tz.getToken();
					while (token != "}")
					{
						// Check for subsection
						if (token == "{")
							tz.skipSection("{", "}");

						// Title
						else if (S_CMPNOCASE(token, "//$Title"))
						{
							name = tz.getLine();
							title_given = true;
						}

						// Game filter
						else if (S_CMPNOCASE(token, "game"))
						{
							filters_present = true;
							if (gameSupportsFilter(currentGame(), tz.getToken()))
								available = true;
						}

						// Tag
						else if (!title_given && S_CMPNOCASE(token, "tag"))
							name = tz.getToken();

						// Category
						else if (S_CMPNOCASE(token, "//$Group") || S_CMPNOCASE(token, "//$Category"))
						{
							group = tz.getLine();
							group_given = true;
						}

						// Sprite
						else if (S_CMPNOCASE(token, "//$EditorSprite") || S_CMPNOCASE(token, "//$Sprite"))
						{
							found_props["sprite"] = tz.getToken();
							sprite_given = true;
						}

						// Radius
						else if (S_CMPNOCASE(token, "radius"))
							found_props["radius"] = tz.getInteger();

						// Height
						else if (S_CMPNOCASE(token, "height"))
							found_props["height"] = tz.getInteger();

						// Scale
						else if (S_CMPNOCASE(token, "scale"))
							found_props["scalex"] = found_props["scaley"] = tz.getFloat();
						else if (S_CMPNOCASE(token, "xscale"))
							found_props["scalex"] = tz.getFloat();
						else if (S_CMPNOCASE(token, "yscale"))
							found_props["scaley"] = tz.getFloat();

						// Angled
						else if (S_CMPNOCASE(token, "//$Angled"))
							found_props["angled"] = true;
						else if (S_CMPNOCASE(token, "//$NotAngled"))
							found_props["angled"] = false;

						// Monster
						else if (S_CMPNOCASE(token, "monster"))
						{
							found_props["solid"] = true;		// Solid
							found_props["decoration"] = false;	// Not a decoration
						}

						// Hanging
						else if (S_CMPNOCASE(token, "+spawnceiling"))
							found_props["hanging"] = true;

						// Fullbright
						else if (S_CMPNOCASE(token, "+bright"))
							found_props["bright"] = true;

						// Is Decoration
						else if (S_CMPNOCASE(token, "//$IsDecoration"))
							found_props["decoration"] = true;

						// Icon
						else if (S_CMPNOCASE(token, "//$Icon"))
							found_props["icon"] = tz.getToken();

						// DB2 Color
						else if (S_CMPNOCASE(token, "//$Color"))
							found_props["color"] = tz.getToken();

						// SLADE 3 Colour (overrides DB2 color)
						// Good thing US spelling differs from ABC (Aussie/Brit/Canuck) spelling! :p
						else if (S_CMPNOCASE(token, "//$Colour"))
							found_props["colour"] = tz.getLine();

						// Translation
						else if (S_CMPNOCASE(token, "translation"))
						{
							string translation = "\"";
							translation += tz.getToken();
							while (tz.peekToken() == ",")
							{
								translation += tz.getToken(); // ,
								translation += tz.getToken(); // next range
							}
							translation += "\"";
							found_props["translation"] = translation;
						}

						// Solid
						else if (S_CMPNOCASE(token, "+solid"))
							found_props["solid"] = true;

						// States
						if (!sprite_given && S_CMPNOCASE(token, "states"))
						{
							tz.skipToken(); // Skip {

							int statecounter = 0;
							string spritestate;
							string laststate;
							int priority = 0;
							int lastpriority = 0;

							token = tz.getToken();
							while (token != "}")
							{
								// Idle, See, Inactive, Spawn, and finally first defined
								if (priority < SS_IDLE)
								{
									string myspritestate = token;
									token = tz.getToken();
									while (token.Cmp(":") && token.Cmp("}"))
									{
										myspritestate = token;
										token = tz.getToken();
									}
									if (S_CMPNOCASE(token, "}"))
										break;
									string sb = tz.getToken(); // Sprite base

									// Handle removed states
									if (S_CMPNOCASE(sb, "Stop"))
										continue;
									// Handle direct gotos, like ZDoom's dead monsters
									if (S_CMPNOCASE(sb, "Goto"))
									{
										tz.skipToken();
										// Skip scope and state
										if (tz.peekToken() == ":")
										{
											tz.skipToken();	// first :
											tz.skipToken(); // second :
											tz.skipToken(); // state name
										}
										continue;
									}
									string sf = tz.getToken(); // Sprite frame(s)
									int mypriority = 0;
									// If the same state is given several names, 
									// don't read the next name as a sprite name!
									// If "::" is encountered, it's a scope operator.
									if ((!sf.Cmp(":")) && tz.peekToken().Cmp(":"))
									{
										if (S_CMPNOCASE(myspritestate, "spawn"))			mypriority = SS_SPAWN;
										else if (S_CMPNOCASE(myspritestate, "inactive"))	mypriority = SS_INACTIVE;
										else if (S_CMPNOCASE(myspritestate, "see"))			mypriority = SS_SEE;
										else if (S_CMPNOCASE(myspritestate, "idle"))		mypriority = SS_IDLE;
										if (mypriority > lastpriority)
										{
											laststate = myspritestate;
											lastpriority = mypriority;
										}
										continue;
									}
									else
									{
										spritestate = myspritestate;
										if (statecounter++ == 0)						mypriority = SS_FIRSTDEFINED;
										if (S_CMPNOCASE(spritestate, "spawn"))			mypriority = SS_SPAWN;
										else if (S_CMPNOCASE(spritestate, "inactive"))	mypriority = SS_INACTIVE;
										else if (S_CMPNOCASE(spritestate, "see"))		mypriority = SS_SEE;
										else if (S_CMPNOCASE(spritestate, "idle"))		mypriority = SS_IDLE;
										if (lastpriority > mypriority)
										{
											spritestate = laststate;
											mypriority = lastpriority;
										}
									}
									if (sb.length() == 4)
									{
										string sprite = sb + sf.Left(1) + "?";
										if (mypriority > priority)
										{
											priority = mypriority;
											found_props["sprite"] = sprite;
											LOG_MESSAGE(3, "Actor %s found sprite %s from state %s", name, sprite, spritestate);
											lastpriority = -1;
										}
									}
								}
								else
								{
									tz.skipSection("{", "}");
									break;
								}
								token = tz.getToken();
							}
						}

						token = tz.getToken();
					}

					LOG_MESSAGE(3, "Parsed actor %s: %d", name, type);
				}
				else
					LOG_MESSAGE(1, "Warning: Invalid actor definition for %s", name);

				// Ignore actors filtered for other games, 
				// and actors with a negative or null type
				if (type > 0 && (available || !filters_present))
				{
					bool defined = false;

					// Create thing type object if needed
					if (!thing_types[type].type)
					{
						thing_types[type].type = new ThingType();
						thing_types[type].index = thing_types.size();
						thing_types[type].number = type;
						thing_types[type].type->decorate = true;
					}
					else
						defined = true;
					ThingType* tt = thing_types[type].type;

					// Get group defaults (if any)
					if (!group.empty())
					{
						ThingType* group_defaults = NULL;
						for (unsigned a = 0; a < tt_group_defaults.size(); a++)
						{
							if (S_CMPNOCASE(group, tt_group_defaults[a]->group))
							{
								group_defaults = tt_group_defaults[a];
								break;
							}
						}

						if (group_defaults)
						{
							tt->copy(group_defaults);
						}
					}

					// Setup thing
					if (!defined || title_given || tt->decorate)
						tt->name = name;
					if (!defined || group_given || tt->decorate)
						tt->group = group.empty() ? "Decorate" : group;
					if (!defined || sprite_given || tt->sprite.IsEmpty() || tt->decorate)
					{
						if (found_props["sprite"].hasValue())
						{
							if (S_CMPNOCASE(found_props["sprite"].getStringValue(), "tnt1a?"))
							{
								if ((!(found_props["icon"].hasValue())) && tt->icon.IsEmpty())
									tt->icon = "tnt1a0";
							}
							else
								tt->sprite = found_props["sprite"].getStringValue();
						}
					}
					if (found_props["radius"].hasValue()) tt->radius = found_props["radius"].getIntValue();
					if (found_props["height"].hasValue()) tt->height = found_props["height"].getIntValue();
					if (found_props["scalex"].hasValue()) tt->scaleX = found_props["scalex"].getFloatValue();
					if (found_props["scaley"].hasValue()) tt->scaleY = found_props["scaley"].getFloatValue();
					if (found_props["hanging"].hasValue()) tt->hanging = found_props["hanging"].getBoolValue();
					if (found_props["angled"].hasValue()) tt->angled = found_props["angled"].getBoolValue();
					if (found_props["bright"].hasValue()) tt->fullbright = found_props["bright"].getBoolValue();
					if (found_props["decoration"].hasValue()) tt->decoration = found_props["decoration"].getBoolValue();
					if (found_props["icon"].hasValue()) tt->icon = found_props["icon"].getStringValue();
					if (found_props["translation"].hasValue()) tt->translation = found_props["translation"].getStringValue();
					if (found_props["solid"].hasValue()) tt->solid = found_props["solid"].getBoolValue();
					if (found_props["colour"].hasValue())
					{
						wxColour wxc(found_props["colour"].getStringValue());
						if (wxc.IsOk())
						{
							tt->colour.r = wxc.Red(); tt->colour.g = wxc.Green(); tt->colour.b = wxc.Blue();
						}
					}
					else if (found_props["color"].hasValue())
					{
						// Translate DB2 color indices to RGB values
						int color = found_props["color"].getIntValue();
						switch (color)
						{
						case  0:	// DimGray			ARGB value of #FF696969
							tt->colour.r = 0x69; tt->colour.g = 0x69; tt->colour.b = 0x69; break;
						case  1:	// RoyalBlue		ARGB value of #FF4169E1
							tt->colour.r = 0x41; tt->colour.g = 0x69; tt->colour.b = 0xE1; break;
						case  2:	// ForestGreen		ARGB value of #FF228B22
							tt->colour.r = 0x22; tt->colour.g = 0x8B; tt->colour.b = 0x22; break;
						case  3:	// LightSeaGreen	ARGB value of #FF20B2AA
							tt->colour.r = 0x20; tt->colour.g = 0xB2; tt->colour.b = 0xAA; break;
						case  4:	// Firebrick		ARGB value of #FFB22222
							tt->colour.r = 0xB2; tt->colour.g = 0x22; tt->colour.b = 0x22; break;
						case  5:	// DarkViolet		ARGB value of #FF9400D3
							tt->colour.r = 0x94; tt->colour.g = 0x00; tt->colour.b = 0xD3; break;
						case  6:	// DarkGoldenrod	ARGB value of #FFB8860B
							tt->colour.r = 0xB8; tt->colour.g = 0x86; tt->colour.b = 0x0B; break;
						case  7:	// Silver			ARGB value of #FFC0C0C0
							tt->colour.r = 0xC0; tt->colour.g = 0xC0; tt->colour.b = 0xC0; break;
						case  8:	// Gray				ARGB value of #FF808080
							tt->colour.r = 0x80; tt->colour.g = 0x80; tt->colour.b = 0x80; break;
						case  9:	// DeepSkyBlue		ARGB value of #FF00BFFF
							tt->colour.r = 0x00; tt->colour.g = 0xBF; tt->colour.b = 0xFF; break;
						case 10:	// LimeGreen		ARGB value of #FF32CD32
							tt->colour.r = 0x32; tt->colour.g = 0xCD; tt->colour.b = 0x32; break;
						case 11:	// PaleTurquoise	ARGB value of #FFAFEEEE
							tt->colour.r = 0xAF; tt->colour.g = 0xEE; tt->colour.b = 0xEE; break;
						case 12:	// Tomato			ARGB value of #FFFF6347
							tt->colour.r = 0xFF; tt->colour.g = 0x63; tt->colour.b = 0x47; break;
						case 13:	// Violet			ARGB value of #FFEE82EE
							tt->colour.r = 0xEE; tt->colour.g = 0x82; tt->colour.b = 0xEE; break;
						case 14:	// Yellow			ARGB value of #FFFFFF00
							tt->colour.r = 0xFF; tt->colour.g = 0xFF; tt->colour.b = 0x00; break;
						case 15:	// WhiteSmoke		ARGB value of #FFF5F5F5
							tt->colour.r = 0xF5; tt->colour.g = 0xF5; tt->colour.b = 0xF5; break;
						case 16:	// LightPink		ARGB value of #FFFFB6C1
							tt->colour.r = 0xFF; tt->colour.g = 0xB6; tt->colour.b = 0xC1; break;
						case 17:	// DarkOrange		ARGB value of #FFFF8C00
							tt->colour.r = 0xFF; tt->colour.g = 0x8C; tt->colour.b = 0x00; break;
						case 18:	// DarkKhaki		ARGB value of #FFBDB76B
							tt->colour.r = 0xBD; tt->colour.g = 0xB7; tt->colour.b = 0x6B; break;
						case 19:	// Goldenrod		ARGB value of #FFDAA520
							tt->colour.r = 0xDA; tt->colour.g = 0xA5; tt->colour.b = 0x20; break;
						}
					}
				}
			}
		}
		// Old DECORATE definitions might be found
		else
		{
			string name;
			string sprite;
			string group;
			bool spritefound = false;
			char frame;
			bool framefound = false;
			int type = -1;
			PropertyList found_props;
			if (tz.peekToken() == "{")
				name = token;
			// DamageTypes aren't old DECORATE format, but we handle them here to skip over them
			else if ((S_CMPNOCASE(token, "pickup")) || (S_CMPNOCASE(token, "breakable")) 
				|| (S_CMPNOCASE(token, "projectile")) || (S_CMPNOCASE(token, "damagetype")))
			{
				group = token;
				name = tz.getToken();
			}
			tz.skipToken();	// skip '{'
			do
			{
				token = tz.getToken();
				if (S_CMPNOCASE(token, "DoomEdNum"))
				{
					tz.getInteger(&type);
				}
				else if (S_CMPNOCASE(token, "Sprite"))
				{
					sprite = tz.getToken();
					spritefound = true;
				}
				else if (S_CMPNOCASE(token, "Frames"))
				{
					token = tz.getToken();
					unsigned pos = 0;
					if (token.length() > 0)
					{
						if ((token[0] < 'a' || token[0] > 'z') && (token[0] < 'A' || token[0] > ']'))
						{
							pos = token.find(':') + 1;
							if (token.length() <= pos)
								pos = token.length() + 1;
							else if ((token.length() >= pos + 2) && token[pos + 1] == '*')
								found_props["bright"] = true;
						}
					}
					if (pos < token.length())
					{
						frame = token[pos];
						framefound = true;
					}
				}
				else if (S_CMPNOCASE(token, "Radius"))
					found_props["radius"] = tz.getInteger();
				else if (S_CMPNOCASE(token, "Height"))
					found_props["height"] = tz.getInteger();
				else if (S_CMPNOCASE(token, "Solid"))
					found_props["solid"] = true;
				else if (S_CMPNOCASE(token, "SpawnCeiling"))
					found_props["hanging"] = true;
				else if (S_CMPNOCASE(token, "Scale"))
					found_props["scale"] = tz.getFloat();
				else if (S_CMPNOCASE(token, "Translation1"))
					found_props["translation"] = S_FMT("doom%d", tz.getInteger());
			}
			while (token != "}" && !token.empty());

			// Add only if a DoomEdNum is present
			if (type > 0)
			{
				bool defined = false;
				// Create thing type object if needed
				if (!thing_types[type].type)
				{
					thing_types[type].type = new ThingType();
					thing_types[type].index = thing_types.size();
					thing_types[type].number = type;
					thing_types[type].type->decorate = true;
				}
				else
					defined = true;
				ThingType* tt = thing_types[type].type;

				// Setup thing
				if (!defined)
				{
					tt->name = name;
					tt->group = "Decorate";
					if (group.length())
						tt->group += "/" + group;
					tt->angled = false;
					if (spritefound && framefound)
					{
						sprite = sprite + frame + '?';
						if (S_CMPNOCASE(sprite, "tnt1a?"))
							tt->icon = "tnt1a0";
						else
							tt->sprite = sprite;
					}
				}
				if (found_props["radius"].hasValue()) tt->radius = found_props["radius"].getIntValue();
				if (found_props["height"].hasValue()) tt->height = found_props["height"].getIntValue();
				if (found_props["scale"].hasValue()) tt->scaleX = tt->scaleY = found_props["scale"].getFloatValue();
				if (found_props["hanging"].hasValue()) tt->hanging = found_props["hanging"].getBoolValue();
				if (found_props["bright"].hasValue()) tt->fullbright = found_props["bright"].getBoolValue();
				if (found_props["translation"].hasValue()) tt->translation = found_props["translation"].getStringValue();
				LOG_MESSAGE(3, "Parsed %s %s: %d", group.length() ? group : "decoration", name, type);
			}
			else
				LOG_MESSAGE(3, "Not adding %s %s, no editor number", group.length() ? group : "decoration", name);
		}

		token = tz.getToken();
	}

	//wxFile tempfile(appPath("decorate_full.txt", DIR_APP), wxFile::write);
	//tempfile.Write(full_defs);
	//tempfile.Close();

	return true;
}

/* GameConfiguration::clearDecorateDefs
 * Removes any thing definitions parsed from DECORATE entries
 *******************************************************************/
void GameConfiguration::clearDecorateDefs()
{
	/*for (auto def : thing_types)
		if (def.second.type && def.second.type->decorate)
		{
			delete def.second.type;
			def.second.type = nullptr;
		}*/
}

/* GameConfiguration::lineFlag
 * Returns the name of the line flag at [index]
 *******************************************************************/
string GameConfiguration::lineFlag(unsigned index)
{
	// Check index
	if (index >= flags_line.size())
		return "";

	return flags_line[index].name;
}

/* GameConfiguration::lineFlagSet
 * Returns true if the flag at [index] is set for [line]
 *******************************************************************/
bool GameConfiguration::lineFlagSet(unsigned index, MapLine* line)
{
	// Check index
	if (index >= flags_line.size())
		return false;

	// Check if flag is set
	unsigned long flags = line->intProperty("flags");
	if (flags & flags_line[index].flag)
		return true;
	else
		return false;
}

/* GameConfiguration::lineFlagSet
 * Returns true if the flag matching [flag] (UDMF name) is set for
 * [line]
 *******************************************************************/
bool GameConfiguration::lineFlagSet(string flag, MapLine* line, int map_format)
{
	// If UDMF, just get the bool value
	if (map_format == MAP_UDMF)
		return line->boolProperty(flag);

	// Get current flags
	unsigned long flags = line->intProperty("flags");

	// Iterate through flags
	for (size_t i = 0; i < flags_line.size(); ++i)
	{
		if (flags_line[i].udmf == flag)
			return !!(flags & flags_line[i].flag);
	}
	LOG_MESSAGE(2, "Flag %s does not exist in this configuration", flag);
	return false;
}

/* GameConfiguration::lineBasicFlagSet
 * Returns true if the basic flag matching [flag] (UDMF name) is set
 * for [line]. 'Basic' flags are flags that are available in some
 * way or another in all game configurations
 *******************************************************************/
bool GameConfiguration::lineBasicFlagSet(string flag, MapLine* line, int map_format)
{
	// If UDMF, just get the bool value
	if (map_format == MAP_UDMF)
		return line->boolProperty(flag);

	// Get current flags
	unsigned long flags = line->intProperty("flags");

	// Impassable
	if (flag == "blocking")
		return !!(flags & 1);

	// Two Sided
	else if (flag == "twosided")
		return !!(flags & 4);

	// Upper unpegged
	else if (flag == "dontpegtop")
		return !!(flags & 8);

	// Lower unpegged
	else if (flag == "dontpegbottom")
		return !!(flags & 16);

	// Not basic
	return lineFlagSet(flag, line, map_format);
}

/* GameConfiguration::lineFlagsString
 * Returns a string containing all flags set on [line]
 *******************************************************************/
string GameConfiguration::lineFlagsString(MapLine* line)
{
	if (!line)
		return "None";

	// Get raw flags
	unsigned long flags = line->intProperty("flags");
	// TODO: UDMF flags

	// Check against all flags
	string ret = "";
	for (unsigned long a = 0; a < flags_line.size(); a++)
	{
		if (flags & flags_line[a].flag)
		{
			// Add flag name to string
			ret += flags_line[a].name;
			ret += ", ";
		}
	}

	// Remove ending ', ' if needed
	if (ret.Length() > 0)
		ret.RemoveLast(2);
	else
		ret = "None";

	return ret;
}

/* GameConfiguration::setLineFlag
 * Sets line flag at [index] for [line]. If [set] is false, the flag
 * is unset
 *******************************************************************/
void GameConfiguration::setLineFlag(unsigned index, MapLine* line, bool set)
{
	// Check index
	if (index >= flags_line.size())
		return;

	// Determine new flags value
	unsigned long flags = line->intProperty("flags");
	if (set)
		flags |= flags_line[index].flag;
	else
		flags = (flags & ~flags_line[index].flag);

	// Update line flags
	line->setIntProperty("flags", flags);
}

/* GameConfiguration::setLineFlag
 * Sets line flag matching [flag] (UDMF name) for [line]. If [set]
 * is false, the flag is unset
 *******************************************************************/
void GameConfiguration::setLineFlag(string flag, MapLine* line, int map_format, bool set)
{
	// If UDMF, just set the bool value
	if (map_format == MAP_UDMF)
	{
		line->setBoolProperty(flag, set);
		return;
	}

	// Iterate through flags
	unsigned long flag_val = 0;
	for (size_t i = 0; i < flags_line.size(); ++i)
	{
		if (flags_line[i].udmf == flag)
		{
			flag_val = flags_line[i].flag;
			break;
		}
	}

	if (flag_val == 0)
	{
		LOG_MESSAGE(2, "Flag %s does not exist in this configuration", flag);
		return;
	}

	// Determine new flags value
	unsigned long flags = line->intProperty("flags");
	if (set)
		flags |= flag_val;
	else
		flags = (flags & ~flag_val);

	// Update line flags
	line->setIntProperty("flags", flags);
}

/* GameConfiguration::setLineBasicFlag
 * Sets line basic flag [flag] (UDMF name) for [line]. If [set]
 * is false, the flag is unset
 *******************************************************************/
void GameConfiguration::setLineBasicFlag(string flag, MapLine* line, int map_format, bool set)
{
	// If UDMF, just set the bool value
	if (map_format == MAP_UDMF)
	{
		line->setBoolProperty(flag, set);
		return;
	}

	// Get current flags
	unsigned long flags = line->intProperty("flags");
	unsigned long fval = 0;

	// Impassable
	if (flag == "blocking")
		fval = 1;

	// Two Sided
	else if (flag == "twosided")
		fval = 4;

	// Upper unpegged
	else if (flag == "dontpegtop")
		fval = 8;

	// Lower unpegged
	else if (flag == "dontpegbottom")
		fval = 16;

	// Set/unset flag
	if (fval)
	{
		if (set)
			line->setIntProperty("flags", flags|fval);
		else
			line->setIntProperty("flags", flags & ~fval);
	}
	// Not basic
	else setLineFlag(flag, line, map_format, set);
}

/* GameConfiguration::spacTriggerString
 * Returns the hexen SPAC trigger for [line] as a string
 *******************************************************************/
string GameConfiguration::spacTriggerString(MapLine* line, int map_format)
{
	if (!line)
		return "None";

	// Hexen format
	if (map_format == MAP_HEXEN)
	{
		// Get raw flags
		int flags = line->intProperty("flags");

		// Get SPAC trigger value from flags
		int trigger = ((flags & 0x1c00) >> 10);

		// Find matching trigger name
		for (unsigned a = 0; a < triggers_line.size(); a++)
		{
			if (triggers_line[a].flag == trigger)
				return triggers_line[a].name;
		}
	}

	// UDMF format
	else if (map_format == MAP_UDMF)
	{
		// Go through all line UDMF properties
		string trigger = "";
		vector<udmfp_t> props = allUDMFProperties(MOBJ_LINE);
		sort(props.begin(), props.end());
		for (unsigned a = 0; a < props.size(); a++)
		{
			// Check for trigger property
			if (props[a].property->isTrigger())
			{
				// Check if the line has this property
				if (line->boolProperty(props[a].property->getProperty()))
				{
					// Add to trigger line
					if (!trigger.IsEmpty())
						trigger += ", ";
					trigger += props[a].property->getName();
				}
			}
		}

		// Check if there was any trigger
		if (trigger.IsEmpty())
			return "None";
		else
			return trigger;
	}

	// Unknown trigger
	return "Unknown";
}

/* GameConfiguration::spacTriggerIndexHexen
 * Returns the hexen SPAC trigger index for [line]
 *******************************************************************/
int GameConfiguration::spacTriggerIndexHexen(MapLine* line)
{
	// Get raw flags
	int flags = line->intProperty("flags");

	// Get SPAC trigger value from flags
	int trigger = ((flags & 0x1c00) >> 10);

	// Find matching trigger name
	for (unsigned a = 0; a < triggers_line.size(); a++)
	{
		if (triggers_line[a].flag == trigger)
			return a;
	}

	return 0;
}

/* GameConfiguration::allSpacTriggers
 * Returns a list of all defined SPAC triggers
 *******************************************************************/
wxArrayString GameConfiguration::allSpacTriggers()
{
	wxArrayString ret;

	for (unsigned a = 0; a < triggers_line.size(); a++)
		ret.Add(triggers_line[a].name);

	return ret;
}

/* GameConfiguration::setLineSpacTrigger
 * Sets the SPAC trigger for [line] to the trigger at [index]
 *******************************************************************/
void GameConfiguration::setLineSpacTrigger(unsigned index, MapLine* line)
{
	// Check index
	if (index >= triggers_line.size())
		return;

	// Get trigger value
	int trigger = triggers_line[index].flag;

	// Get raw line flags
	int flags = line->intProperty("flags");

	// Apply trigger to flags
	trigger = trigger << 10;
	flags &= ~0x1c00;
	flags |= trigger;

	// Update line flags
	line->setIntProperty("flags", flags);
}

/* GameConfiguration::getUDMFProperty
 * Returns the UDMF property definition matching [name] for
 * MapObject type [type]
 *******************************************************************/
UDMFProperty* GameConfiguration::getUDMFProperty(string name, int type)
{
	if (type == MOBJ_VERTEX)
		return udmf_vertex_props[name].property;
	else if (type == MOBJ_LINE)
		return udmf_linedef_props[name].property;
	else if (type == MOBJ_SIDE)
		return udmf_sidedef_props[name].property;
	else if (type == MOBJ_SECTOR)
		return udmf_sector_props[name].property;
	else if (type == MOBJ_THING)
		return udmf_thing_props[name].property;
	else
		return nullptr;
}

/* GameConfiguration::allUDMFProperties
 * Returns all defined UDMF properties for MapObject type [type]
 *******************************************************************/
vector<udmfp_t> GameConfiguration::allUDMFProperties(int type)
{
	vector<udmfp_t> ret;

	// Build list depending on type
	UDMFPropMap* map = nullptr;
	if (type == MOBJ_VERTEX)
		map = &udmf_vertex_props;
	else if (type == MOBJ_LINE)
		map = &udmf_linedef_props;
	else if (type == MOBJ_SIDE)
		map = &udmf_sidedef_props;
	else if (type == MOBJ_SECTOR)
		map = &udmf_sector_props;
	else if (type == MOBJ_THING)
		map = &udmf_thing_props;
	else
		return ret;

	UDMFPropMap::iterator i = map->begin();
	while (i != map->end())
	{
		if (i->second.property)
		{
			udmfp_t up(i->second.property);
			up.index = i->second.index;
			ret.push_back(up);
		}

		i++;
	}

	return ret;
}

/* GameConfiguration::cleanObjectUDMFProperties
 * Removes any UDMF properties in [object] that have default values
 * (so they are not written to the UDMF map unnecessarily)
 *******************************************************************/
void GameConfiguration::cleanObjectUDMFProps(MapObject* object)
{
	// Get UDMF properties list for type
	UDMFPropMap* map = nullptr;
	int type = object->getObjType();
	if (type == MOBJ_VERTEX)
		map = &udmf_vertex_props;
	else if (type == MOBJ_LINE)
		map = &udmf_linedef_props;
	else if (type == MOBJ_SIDE)
		map = &udmf_sidedef_props;
	else if (type == MOBJ_SECTOR)
		map = &udmf_sector_props;
	else if (type == MOBJ_THING)
		map = &udmf_thing_props;
	else
		return;

	// Go through properties
	UDMFPropMap::iterator i = map->begin();
	while (i != map->end())
	{
		if (!i->second.property)
		{
			i++;
			continue;
		}

		// Check if the object even has this property
		if (!object->hasProp(i->first))
		{
			i++;
			continue;
		}

		// Remove the property from the object if it is the default value
		//Property& def = i->second.property->getDefaultValue();
		if (i->second.property->getDefaultValue().getType() == PROP_BOOL)
		{
			if (i->second.property->getDefaultValue().getBoolValue() == object->boolProperty(i->first))
				object->props().removeProperty(i->first);
		}
		else if (i->second.property->getDefaultValue().getType() == PROP_INT)
		{
			if (i->second.property->getDefaultValue().getIntValue() == object->intProperty(i->first))
				object->props().removeProperty(i->first);
		}
		else if (i->second.property->getDefaultValue().getType() == PROP_FLOAT)
		{
			if (i->second.property->getDefaultValue().getFloatValue() == object->floatProperty(i->first))
				object->props().removeProperty(i->first);
		}
		else if (i->second.property->getDefaultValue().getType() == PROP_STRING)
		{
			if (i->second.property->getDefaultValue().getStringValue() == object->stringProperty(i->first))
				object->props().removeProperty(i->first);
		}

		i++;
	}
}

/* GameConfiguration::sectorTypeName
 * Returns the name for sector type value [type], taking generalised
 * types into account
 *******************************************************************/
string GameConfiguration::sectorTypeName(int type)
{
	// Check for zero type
	if (type == 0)
		return "Normal";

	// Deal with generalised flags
	vector<string> gen_flags;
	if (supportsSectorFlags() && type >= boom_sector_flag_start)
	{
		// Damage flags
		int damage = sectorBoomDamage(type);
		if (damage == 1)
			gen_flags.push_back("5% Damage");
		else if (damage == 2)
			gen_flags.push_back("10% Damage");
		else if (damage == 3)
			gen_flags.push_back("20% Damage");

		// Secret
		if (sectorBoomSecret(type))
			gen_flags.push_back("Secret");

		// Friction
		if (sectorBoomFriction(type))
			gen_flags.push_back("Friction Enabled");

		// Pushers/Pullers
		if (sectorBoomPushPull(type))
			gen_flags.push_back("Pushers/Pullers Enabled");

		// Remove flag bits from type value
		type = type & (boom_sector_flag_start - 1);
	}

	// Check if the type only has generalised flags
	if (type == 0 && gen_flags.size() > 0)
	{
		// Just return flags in this case
		string name = gen_flags[0];
		for (unsigned a = 1; a < gen_flags.size(); a++)
			name += S_FMT(" + %s", gen_flags[a]);

		return name;
	}

	// Go through sector types
	string name = "Unknown";
	for (unsigned a = 0; a < sector_types.size(); a++)
	{
		if (sector_types[a].type == type)
		{
			name = sector_types[a].name;
			break;
		}
	}

	// Add generalised flags to type name
	for (unsigned a = 0; a < gen_flags.size(); a++)
		name += S_FMT(" + %s", gen_flags[a]);

	return name;
}

/* GameConfiguration::sectorTypeByName
 * Returns the sector type value matching [name]
 *******************************************************************/
int GameConfiguration::sectorTypeByName(string name)
{
	for (unsigned a = 0; a < sector_types.size(); a++)
	{
		if (sector_types[a].name == name)
			return sector_types[a].type;
	}

	return 0;
}

/* GameConfiguration::baseSectorType
 * Returns the 'base' sector type for value [type] (strips generalised
 * flags/type)
 *******************************************************************/
int GameConfiguration::baseSectorType(int type)
{
	// No type
	if (type == 0)
		return 0;

	// Strip boom flags depending on map format
	if (supportsSectorFlags())
		return type & (boom_sector_flag_start - 1);

	// No flags
	return type;
}

/* GameConfiguration::sectorBoomDamage
 * Returns the generalised 'damage' flag for [type]: 0=none, 1=5%,
 * 2=10% 3=20%
 *******************************************************************/
int GameConfiguration::sectorBoomDamage(int type)
{
	if (!supportsSectorFlags())
		return 0;

	// No type
	if (type == 0)
		return 0;

	int low_bit = boom_sector_flag_start << 0;
	int high_bit = boom_sector_flag_start << 1;

	if ((type & (low_bit | high_bit)) == (low_bit | high_bit))
		return 3;
	else if (type & low_bit)
		return 1;
	else if (type & high_bit)
		return 2;

	// No damage
	return 0;
}

/* GameConfiguration::sectorBoomSecret
 * Returns true if the generalised 'secret' flag is set for [type]
 *******************************************************************/
bool GameConfiguration::sectorBoomSecret(int type)
{
	if (!supportsSectorFlags())
		return false;

	// No type
	if (type == 0)
		return false;

	if (type & (boom_sector_flag_start << 2))
		return true;

	// Not secret
	return false;
}

/* GameConfiguration::sectorBoomFriction
* Returns true if the generalised 'friction' flag is set for [type]
*******************************************************************/
bool GameConfiguration::sectorBoomFriction(int type)
{
	if (!supportsSectorFlags())
		return false;

	// No type
	if (type == 0)
		return false;

	if (type & (boom_sector_flag_start << 3))
		return true;

	// Friction disabled
	return false;
}

/* GameConfiguration::sectorBoomPushPull
 * Returns true if the generalised 'pusher/puller' flag is set for
 * [type]
 *******************************************************************/
bool GameConfiguration::sectorBoomPushPull(int type)
{
	if (!supportsSectorFlags())
		return false;

	// No type
	if (type == 0)
		return false;

	if (type & (boom_sector_flag_start << 4))
		return true;

	// Pusher/Puller disabled
	return false;
}

/* GameConfiguration::boomSectorType
 * Returns the generalised boom sector type built from parameters
 *******************************************************************/
int GameConfiguration::boomSectorType(int base, int damage, bool secret, bool friction, bool pushpull)
{
	int fulltype = base;

	// Damage
	fulltype += damage * boom_sector_flag_start;

	// Secret
	if (secret)
		fulltype += boom_sector_flag_start << 2;

	// Friction
	if (friction)
		fulltype += boom_sector_flag_start << 3;

	// Pusher/Puller
	if (pushpull)
		fulltype += boom_sector_flag_start << 4;

	return fulltype;
}

/* GameConfiguration::getDefaultString
 * Returns the default string value for [property] of MapObject
 * type [type]
 *******************************************************************/
string GameConfiguration::getDefaultString(int type, string property)
{
	switch (type)
	{
	case MOBJ_LINE:
		return defaults_line[property].getStringValue(); break;
	case MOBJ_SIDE:
		return defaults_side[property].getStringValue(); break;
	case MOBJ_SECTOR:
		return defaults_sector[property].getStringValue(); break;
	case MOBJ_THING:
		return defaults_thing[property].getStringValue(); break;
	default:
		return "";
	}
}

/* GameConfiguration::getDefaultInt
 * Returns the default int value for [property] of MapObject
 * type [type]
 *******************************************************************/
int GameConfiguration::getDefaultInt(int type, string property)
{
	switch (type)
	{
	case MOBJ_LINE:
		return defaults_line[property].getIntValue(); break;
	case MOBJ_SIDE:
		return defaults_side[property].getIntValue(); break;
	case MOBJ_SECTOR:
		return defaults_sector[property].getIntValue(); break;
	case MOBJ_THING:
		return defaults_thing[property].getIntValue(); break;
	default:
		return 0;
	}
}

/* GameConfiguration::getDefaultFloat
 * Returns the default float value for [property] of MapObject
 * type [type]
 *******************************************************************/
double GameConfiguration::getDefaultFloat(int type, string property)
{
	switch (type)
	{
	case MOBJ_LINE:
		return defaults_line[property].getFloatValue(); break;
	case MOBJ_SIDE:
		return defaults_side[property].getFloatValue(); break;
	case MOBJ_SECTOR:
		return defaults_sector[property].getFloatValue(); break;
	case MOBJ_THING:
		return defaults_thing[property].getFloatValue(); break;
	default:
		return 0;
	}
}

/* GameConfiguration::getDefaultBool
 * Returns the default boolean value for [property] of MapObject
 * type [type]
 *******************************************************************/
bool GameConfiguration::getDefaultBool(int type, string property)
{
	switch (type)
	{
	case MOBJ_LINE:
		return defaults_line[property].getBoolValue(); break;
	case MOBJ_SIDE:
		return defaults_side[property].getBoolValue(); break;
	case MOBJ_SECTOR:
		return defaults_sector[property].getBoolValue(); break;
	case MOBJ_THING:
		return defaults_thing[property].getBoolValue(); break;
	default:
		return false;
	}
}

/* GameConfiguration::applyDefaults
 * Apply defined default values to [object]
 *******************************************************************/
void GameConfiguration::applyDefaults(MapObject* object, bool udmf)
{
	// Get all defaults for the object type
	vector<string> prop_names;
	vector<Property> prop_vals;

	// Line defaults
	if (object->getObjType() == MOBJ_LINE)
	{
		defaults_line.allProperties(prop_vals);
		defaults_line.allPropertyNames(prop_names);
		if (udmf)
		{
			defaults_line_udmf.allProperties(prop_vals);
			defaults_line_udmf.allPropertyNames(prop_names);
		}
	}

	// Side defaults
	else if (object->getObjType() == MOBJ_SIDE)
	{
		defaults_side.allProperties(prop_vals);
		defaults_side.allPropertyNames(prop_names);
		if (udmf)
		{
			defaults_side_udmf.allProperties(prop_vals);
			defaults_side_udmf.allPropertyNames(prop_names);
		}
	}

	// Sector defaults
	else if (object->getObjType() == MOBJ_SECTOR)
	{
		defaults_sector.allProperties(prop_vals);
		defaults_sector.allPropertyNames(prop_names);
		if (udmf)
		{
			defaults_sector_udmf.allProperties(prop_vals);
			defaults_sector_udmf.allPropertyNames(prop_names);
		}
	}

	// Thing defaults
	else if (object->getObjType() == MOBJ_THING)
	{
		defaults_thing.allProperties(prop_vals);
		defaults_thing.allPropertyNames(prop_names);
		if (udmf)
		{
			defaults_thing_udmf.allProperties(prop_vals);
			defaults_thing_udmf.allPropertyNames(prop_names);
		}
	}

	// Apply defaults to object
	for (unsigned a = 0; a < prop_names.size(); a++)
	{
		if (prop_vals[a].getType() == PROP_BOOL)
			object->setBoolProperty(prop_names[a], prop_vals[a].getBoolValue());
		else if (prop_vals[a].getType() == PROP_INT)
			object->setIntProperty(prop_names[a], prop_vals[a].getIntValue());
		else if (prop_vals[a].getType() == PROP_FLOAT)
			object->setFloatProperty(prop_names[a], prop_vals[a].getFloatValue());
		else if (prop_vals[a].getType() == PROP_STRING)
			object->setStringProperty(prop_names[a], prop_vals[a].getStringValue());
		LOG_MESSAGE(3, "Applied default property %s = %s", prop_names[a], prop_vals[a].getStringValue());
	}
}

/* GameConfiguration::setLightLevelInterval
 * Builds the array of valid light levels from [interval]
 *******************************************************************/
void GameConfiguration::setLightLevelInterval(int interval)
{
	// Clear current
	light_levels.clear();

	// Fill light levels array
	int light = 0;
	while (light < 255)
	{
		light_levels.push_back(light);
		light += interval;
	}
	light_levels.push_back(255);
}

/* GameConfiguration::upLightLevel
 * Returns [light_level] incremented to the next 'valid' light level
 * (defined by the game light interval)
 *******************************************************************/
int GameConfiguration::upLightLevel(int light_level)
{
	// No defined levels
	if (light_levels.size() == 0)
		return light_level;

	for (int a = 0; a < (int)light_levels.size() - 1; a++)
	{
		if (light_level >= light_levels[a] && light_level < light_levels[a+1])
			return light_levels[a+1];
	}

	return light_levels.back();
}

/* GameConfiguration::downLightLevel
 * Returns [light_level] decremented to the next 'valid' light level
 * (defined by the game light interval)
 *******************************************************************/
int GameConfiguration::downLightLevel(int light_level)
{
	// No defined levels
	if (light_levels.size() == 0)
		return light_level;

	for (int a = 0; a < (int)light_levels.size() - 1; a++)
	{
		if (light_level > light_levels[a] && light_level <= light_levels[a+1])
			return light_levels[a];
	}

	return 0;
}

/* GameConfiguration::parseTagged
 * Returns the tagged type of the parsed tree node [tagged]
 *******************************************************************/
int GameConfiguration::parseTagged(ParseTreeNode* tagged)
{
	string str = tagged->getStringValue();
	if (S_CMPNOCASE(str, "no")) return AS_TT_NO;
	else if (S_CMPNOCASE(str, "sector")) return AS_TT_SECTOR;
	else if (S_CMPNOCASE(str, "line")) return AS_TT_LINE;
	else if (S_CMPNOCASE(str, "lineid")) return AS_TT_LINEID;
	else if (S_CMPNOCASE(str, "lineid_hi5")) return AS_TT_LINEID_HI5;
	else if (S_CMPNOCASE(str, "thing")) return AS_TT_THING;
	else if (S_CMPNOCASE(str, "sector_back")) return AS_TT_SECTOR_BACK;
	else if (S_CMPNOCASE(str, "sector_or_back")) return AS_TT_SECTOR_OR_BACK;
	else if (S_CMPNOCASE(str, "sector_and_back")) return AS_TT_SECTOR_AND_BACK;
	else if (S_CMPNOCASE(str, "line_negative")) return AS_TT_LINE_NEGATIVE;
	else if (S_CMPNOCASE(str, "ex_1thing_2sector")) return AS_TT_1THING_2SECTOR;
	else if (S_CMPNOCASE(str, "ex_1thing_3sector")) return AS_TT_1THING_3SECTOR;
	else if (S_CMPNOCASE(str, "ex_1thing_2thing")) return AS_TT_1THING_2THING;
	else if (S_CMPNOCASE(str, "ex_1thing_4thing")) return AS_TT_1THING_4THING;
	else if (S_CMPNOCASE(str, "ex_1thing_2thing_3thing")) return AS_TT_1THING_2THING_3THING;
	else if (S_CMPNOCASE(str, "ex_1sector_2thing_3thing_5thing")) return AS_TT_1SECTOR_2THING_3THING_5THING;
	else if (S_CMPNOCASE(str, "ex_1lineid_2line")) return AS_TT_1LINEID_2LINE;
	else if (S_CMPNOCASE(str, "ex_4thing")) return AS_TT_4THING;
	else if (S_CMPNOCASE(str, "ex_5thing")) return AS_TT_5THING;
	else if (S_CMPNOCASE(str, "ex_1line_2sector")) return AS_TT_1LINE_2SECTOR;
	else if (S_CMPNOCASE(str, "ex_1sector_2sector")) return AS_TT_1SECTOR_2SECTOR;
	else if (S_CMPNOCASE(str, "ex_1sector_2sector_3sector_4_sector")) return AS_TT_1SECTOR_2SECTOR_3SECTOR_4SECTOR;
	else if (S_CMPNOCASE(str, "ex_sector_2is3_line")) return AS_TT_SECTOR_2IS3_LINE;
	else if (S_CMPNOCASE(str, "ex_1sector_2thing")) return AS_TT_1SECTOR_2THING;
	else
		return tagged->getIntValue();
}

/* GameConfiguration::dumpActionSpecials
 * Dumps all defined action specials to the log
 *******************************************************************/
void GameConfiguration::dumpActionSpecials()
{
	ASpecialMap::iterator i = action_specials.begin();

	while (i != action_specials.end())
	{
		wxLogMessage("Action special %d = %s", i->first, i->second.special->stringDesc());
		i++;
	}
}

/* GameConfiguration::dumpThingTypes
 * Dumps all defined thing types to the log
 *******************************************************************/
void GameConfiguration::dumpThingTypes()
{
	ThingTypeMap::iterator i = thing_types.begin();

	while (i != thing_types.end())
	{
		wxLogMessage("Thing type %d = %s", i->first, i->second.type->stringDesc());
		i++;
	}
}

/* GameConfiguration::dumpValidMapNames
 * Dumps all defined map names to the log
 *******************************************************************/
void GameConfiguration::dumpValidMapNames()
{
	wxLogMessage("Valid Map Names:");
	for (unsigned a = 0; a < maps.size(); a++)
		wxLogMessage(maps[a].mapname);
}

/* GameConfiguration::dumpUDMFProperties
 * Dumps all defined UDMF properties to the log
 *******************************************************************/
void GameConfiguration::dumpUDMFProperties()
{
	// Vertex
	wxLogMessage("\nVertex properties:");
	UDMFPropMap::iterator i = udmf_vertex_props.begin();
	while (i != udmf_vertex_props.end())
	{
		wxLogMessage(i->second.property->getStringRep());
		i++;
	}

	// Line
	wxLogMessage("\nLine properties:");
	i = udmf_linedef_props.begin();
	while (i != udmf_linedef_props.end())
	{
		wxLogMessage(i->second.property->getStringRep());
		i++;
	}

	// Side
	wxLogMessage("\nSide properties:");
	i = udmf_sidedef_props.begin();
	while (i != udmf_sidedef_props.end())
	{
		wxLogMessage(i->second.property->getStringRep());
		i++;
	}

	// Sector
	wxLogMessage("\nSector properties:");
	i = udmf_sector_props.begin();
	while (i != udmf_sector_props.end())
	{
		wxLogMessage(i->second.property->getStringRep());
		i++;
	}

	// Thing
	wxLogMessage("\nThing properties:");
	i = udmf_thing_props.begin();
	while (i != udmf_thing_props.end())
	{
		wxLogMessage(i->second.property->getStringRep());
		i++;
	}
}


/*******************************************************************
* CONSOLE COMMANDS
*******************************************************************/

CONSOLE_COMMAND(testgc, 0, false)
{
	string game = "doomu";

	if (args.size() > 0)
		game = args[0];

	theGameConfiguration->openConfig(game);
}

CONSOLE_COMMAND(dumpactionspecials, 0, false)
{
	theGameConfiguration->dumpActionSpecials();
}

CONSOLE_COMMAND(dumpudmfprops, 0, false)
{
	theGameConfiguration->dumpUDMFProperties();
}

CONSOLE_COMMAND(dumpthingtypes, 0, false)
{
	theGameConfiguration->dumpThingTypes();
}
