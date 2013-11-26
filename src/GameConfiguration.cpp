
/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "GameConfiguration.h"
#include "Tokenizer.h"
#include "Parser.h"
#include "Misc.h"
#include "Console.h"
#include "Archive.h"
#include "ArchiveManager.h"
#include "SLADEMap.h"
#include "GenLineSpecial.h"
#include <wx/textfile.h>
#include <wx/filename.h>
#include <wx/dir.h>


/*******************************************************************
 * VARIABLES
 *******************************************************************/
GameConfiguration* GameConfiguration::instance = NULL;
CVAR(String, game_configuration, "", CVAR_SAVE)
CVAR(String, port_configuration, "", CVAR_SAVE)


/*******************************************************************
 * GAMECONFIGURATION CLASS FUNCTIONS
 *******************************************************************/

GameConfiguration::GameConfiguration()
{
	setDefaults();
}

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
	as_generalized_s.setName("Boom Generalized Switched Special");
	as_generalized_s.setTagged(AS_TT_SECTOR);
	as_generalized_m.setName("Boom Generalized Manual Special");
	as_generalized_m.setTagged(AS_TT_SECTOR_BACK);
}

string GameConfiguration::udmfNamespace()
{
	return udmf_namespace.Lower();
}

int GameConfiguration::lightLevelInterval()
{
	if (light_levels.size() == 0)
		return 1;
	else
		return light_levels[1];
}

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

GameConfiguration::gconf_t GameConfiguration::readBasicGameConfig(MemChunk& mc)
{
	// Parse configuration
	Parser parser;
	parser.parseText(mc, "");
	gconf_t conf;

	// Check for game section
	ParseTreeNode* node_game = NULL;
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
	}

	return conf;
}

GameConfiguration::pconf_t GameConfiguration::readBasicPortConfig(MemChunk& mc)
{
	// Parse configuration
	Parser parser;
	parser.parseText(mc, "");
	pconf_t conf;

	// Check for port section
	ParseTreeNode* node_port = NULL;
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

string GameConfiguration::mapName(unsigned index)
{
	// Check index
	if (index > maps.size())
		return "";

	return maps[index].mapname;
}

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

GameConfiguration::gconf_t GameConfiguration::gameConfig(unsigned index)
{
	if (index >= game_configs.size())
		return gconf_none;
	else
		return game_configs[index];
}

GameConfiguration::gconf_t GameConfiguration::gameConfig(string id)
{
	for (unsigned a = 0; a < game_configs.size(); a++)
	{
		if (game_configs[a].name == id)
			return game_configs[a];
	}

	return gconf_none;
}

GameConfiguration::pconf_t GameConfiguration::portConfig(unsigned index)
{
	if (index >= port_configs.size())
		return pconf_none;
	else
		return port_configs[index];
}

GameConfiguration::pconf_t GameConfiguration::portConfig(string id)
{
	for (unsigned a = 0; a < port_configs.size(); a++)
	{
		if (port_configs[a].name == id)
			return port_configs[a];
	}

	return pconf_none;
}

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
		if (line.Trim().StartsWith("#include"))
		{
			// Get filename to include
			Tokenizer tz;
			tz.openString(line);
			tz.getToken();	// Skip #include
			string file = tz.getToken();

			// Process the file
			buildConfig(path + file, out);
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
		if (line.Trim().StartsWith("#include"))
		{
			// Get name of entry to include
			Tokenizer tz;
			tz.openString(line);
			tz.getToken();	// Skip #include
			string inc_name = tz.getToken();
			string name = entry->getPath() + inc_name;

			// Get the entry
			bool done = false;
			ArchiveEntry* entry_inc = entry->getParent()->entryAtPath(name);
			if (entry_inc)
			{
				buildConfig(entry_inc, out);
				done = true;
			}
			else
				LOG_MESSAGE(2, S_FMT("Couldn't find entry to #include: %s", CHR(name)));

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
				wxLogMessage("Error: Attempting to #include nonexistant entry \"%s\"", CHR(name));
		}
		else
			out.Append(line + "\n");

		line = file.GetNextLine();
	}

	// Delete temp file
	wxRemoveFile(filename);
}

void GameConfiguration::readActionSpecials(ParseTreeNode* node, ActionSpecial* group_defaults)
{
	// Check if we're clearing all existing specials
	if (node->getChild("clearexisting"))
		action_specials.clear();

	// Determine current 'group'
	ParseTreeNode* group = node;
	string groupname = "";
	while (true)
	{
		if (group->getName() == "action_specials" || !group)
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
	ActionSpecial* as_defaults = new ActionSpecial();
	if (group_defaults) as_defaults->copy(group_defaults);
	as_defaults->parse(node);

	// --- Go through all child nodes ---
	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		ParseTreeNode* child = (ParseTreeNode*)node->getChild(a);

		// Check for 'group'
		if (S_CMPNOCASE(child->getType(), "group"))
			readActionSpecials(child, as_defaults);

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
				action_specials[special].index = action_specials.size();
			}

			// Reset the action special (in case it's being redefined for whatever reason)
			action_specials[special].special->reset();

			// Apply group defaults
			action_specials[special].special->copy(as_defaults);
			action_specials[special].special->group = groupname;

			// Check for simple definition
			if (child->isLeaf())
				action_specials[special].special->name = child->getStringValue();
			else
				action_specials[special].special->parse(child);	// Extended definition
		}
	}

	delete as_defaults;
}

void GameConfiguration::readThingTypes(ParseTreeNode* node, ThingType* group_defaults)
{
	// --- Determine current 'group' ---
	ParseTreeNode* group = node;
	string groupname = "";
	while (true)
	{
		if (group->getName() == "thing_types" || !group)
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
	ParseTreeNode* child = NULL;
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
					wxLogMessage("Warning: Unknown/unsupported map format \"%s\"", CHR(node->getStringValue(v)));
			}
		}

		// Boom extensions
		else if (S_CMPNOCASE(node->getName(), "boom"))
			boom = node->getBoolValue();

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
						defaults_line[def->getName()] = def->getValue();
					}
				}

				// Sidedef defaults
				else if (S_CMPNOCASE(block->getName(), "sidedef"))
				{
					for (unsigned c = 0; c < block->nChildren(); c++)
					{
						ParseTreeNode* def = (ParseTreeNode*)block->getChild(c);
						defaults_side[def->getName()] = def->getValue();
					}
				}

				// Sector defaults
				else if (S_CMPNOCASE(block->getName(), "sector"))
				{
					for (unsigned c = 0; c < block->nChildren(); c++)
					{
						ParseTreeNode* def = (ParseTreeNode*)block->getChild(c);
						defaults_sector[def->getName()] = def->getValue();
					}
				}

				// Thing defaults
				else if (S_CMPNOCASE(block->getName(), "thing"))
				{
					for (unsigned c = 0; c < block->nChildren(); c++)
					{
						ParseTreeNode* def = (ParseTreeNode*)block->getChild(c);
						defaults_thing[def->getName()] = def->getValue();
					}
				}

				else
					wxLogMessage("Unknown defaults block \"%s\"", CHR(block->getName()));
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

bool GameConfiguration::readConfiguration(string& cfg, string source, bool ignore_game, bool clear)
{
	// Clear current configuration
	if (clear)
	{
		setDefaults();
		action_specials.clear();
		thing_types.clear();
		flags_thing.clear();
		flags_line.clear();
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
	parser.parseText(cfg, source);

	// Process parsed data
	ParseTreeNode* base = parser.parseTreeRoot();

	// Read game/port section(s) if needed
	ParseTreeNode* node_game = NULL;
	ParseTreeNode* node_port = NULL;
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
	ParseTreeNode* node = NULL;
	for (unsigned a = 0; a < base->nChildren(); a++)
	{
		node = (ParseTreeNode*)base->getChild(a);

		// Skip game/port section
		if (node == node_game || node == node_port)
			continue;

		// Action specials section
		if (S_CMPNOCASE(node->getName(), "action_specials"))
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
			wxLogMessage("Warning: Unexpected game configuration section \"%s\", skipping", CHR(node->getName()));
	}

	return true;
}

/*
bool GameConfiguration::readConfiguration(string& cfg, string source) {
	// Clear current configuration
	setDefaults();
	name = "Invalid Configuration";
	action_specials.clear();
	thing_types.clear();
	flags_thing.clear();
	flags_line.clear();
	udmf_vertex_props.clear();
	udmf_linedef_props.clear();
	udmf_sidedef_props.clear();
	udmf_sector_props.clear();
	udmf_thing_props.clear();

	// Parse the full configuration
	Parser parser;
	parser.parseText(cfg, source);

	// Process parsed data
	ParseTreeNode* base = parser.parseTreeRoot();

	// 'Game' section (this is required for it to be a valid game configuration)
	ParseTreeNode* node_game = (ParseTreeNode*)base->getChild("game");
	if (!node_game) {
		wxLogMessage("Error: Invalid game configuration, 'game' section required but not found");
		return false;
	}
	for (unsigned a = 0; a < node_game->nChildren(); a++) {
		ParseTreeNode* node = (ParseTreeNode*)node_game->getChild(a);

		// Game name
		if (S_CMPNOCASE(node->getName(), "name"))
			this->name = node->getStringValue();

		// Allow any map name
		else if (S_CMPNOCASE(node->getName(), "map_name_any"))
			any_map_name = node->getBoolValue();

		// Map format
		else if (S_CMPNOCASE(node->getName(), "map_format")) {
			if (S_CMPNOCASE(node->getStringValue(), "doom"))
				map_formats[MAP_DOOM] = true;
			else if (S_CMPNOCASE(node->getStringValue(), "hexen"))
				map_formats[MAP_HEXEN] = true;
			else if (S_CMPNOCASE(node->getStringValue(), "doom64"))
				map_formats[MAP_DOOM64] = true;
			else if (S_CMPNOCASE(node->getStringValue(), "udmf"))
				map_formats[MAP_UDMF] = true;
			else {
				wxLogMessage("Warning: Unknown/unsupported map format \"%s\", defaulting to doom format", CHR(node->getStringValue()));
				//map_format = MAP_DOOM;
			}
		}

		// Boom extensions
		else if (S_CMPNOCASE(node->getName(), "boom"))
			boom = node->getBoolValue();

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
	}

	// Go through all other config sections
	ParseTreeNode* node = NULL;
	for (unsigned a = 0; a < base->nChildren(); a++) {
		node = (ParseTreeNode*)base->getChild(a);

		// Skip game section
		if (node == node_game)
			continue;

		// Action specials section
		if (S_CMPNOCASE(node->getName(), "action_specials"))
			readActionSpecials(node);

		// Thing types section
		else if (S_CMPNOCASE(node->getName(), "thing_types"))
			readThingTypes(node);

		// Line flags section
		else if (S_CMPNOCASE(node->getName(), "line_flags")) {
			for (unsigned c = 0; c < node->nChildren(); c++) {
				ParseTreeNode* value = (ParseTreeNode*)node->getChild(c);

				// Check for 'flag' type
				if (!(S_CMPNOCASE(value->getType(), "flag")))
					continue;

				long flag_val;
				value->getName().ToLong(&flag_val);

				// Check if the flag value already exists
				bool exists = false;
				for (unsigned f = 0; f < flags_line.size(); f++) {
					if (flags_line[f].flag == flag_val) {
						exists = true;
						flags_line[f].name = value->getStringValue();
						break;
					}
				}

				// Add flag otherwise
				if (!exists)
					flags_line.push_back(flag_t(flag_val, value->getStringValue()));
			}
		}

		// Line triggers section
		else if (S_CMPNOCASE(node->getName(), "line_triggers")) {
			for (unsigned c = 0; c < node->nChildren(); c++) {
				ParseTreeNode* value = (ParseTreeNode*)node->getChild(c);

				// Check for 'trigger' type
				if (!(S_CMPNOCASE(value->getType(), "trigger")))
					continue;

				long flag_val;
				value->getName().ToLong(&flag_val);

				// Check if the flag value already exists
				bool exists = false;
				for (unsigned f = 0; f < triggers_line.size(); f++) {
					if (triggers_line[f].flag == flag_val) {
						exists = true;
						triggers_line[f].name = value->getStringValue();
						break;
					}
				}

				// Add flag otherwise
				if (!exists)
					triggers_line.push_back(flag_t(flag_val, value->getStringValue()));
			}
		}

		// Thing flags section
		else if (S_CMPNOCASE(node->getName(), "thing_flags")) {
			for (unsigned c = 0; c < node->nChildren(); c++) {
				ParseTreeNode* value = (ParseTreeNode*)node->getChild(c);

				// Check for 'flag' type
				if (!(S_CMPNOCASE(value->getType(), "flag")))
					continue;

				long flag_val;
				value->getName().ToLong(&flag_val);

				// Check if the flag value already exists
				bool exists = false;
				for (unsigned f = 0; f < flags_thing.size(); f++) {
					if (flags_thing[f].flag == flag_val) {
						exists = true;
						flags_thing[f].name = value->getStringValue();
						break;
					}
				}

				// Add flag otherwise
				if (!exists)
					flags_thing.push_back(flag_t(flag_val, value->getStringValue()));
			}
		}

		// Sector types section
		else if (S_CMPNOCASE(node->getName(), "sector_types")) {
			for (unsigned c = 0; c < node->nChildren(); c++) {
				ParseTreeNode* value = (ParseTreeNode*)node->getChild(c);

				// Check for 'type'
				if (!(S_CMPNOCASE(value->getType(), "type")))
					continue;

				long type_val;
				value->getName().ToLong(&type_val);

				// Check if the sector type already exists
				bool exists = false;
				for (unsigned t = 0; t < sector_types.size(); t++) {
					if (sector_types[t].type == type_val) {
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
		else if (S_CMPNOCASE(node->getName(), "udmf_properties")) {
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

		// Defaults section
		else if (S_CMPNOCASE(node->getName(), "defaults")) {
			// Go through defaults blocks
			for (unsigned b = 0; b < node->nChildren(); b++) {
				ParseTreeNode* block = (ParseTreeNode*)node->getChild(b);

				// Linedef defaults
				if (S_CMPNOCASE(block->getName(), "linedef")) {
					for (unsigned c = 0; c < block->nChildren(); c++) {
						ParseTreeNode* def = (ParseTreeNode*)block->getChild(c);
						defaults_line[def->getName()] = def->getValue();
					}
				}

				// Sidedef defaults
				else if (S_CMPNOCASE(block->getName(), "sidedef")) {
					for (unsigned c = 0; c < block->nChildren(); c++) {
						ParseTreeNode* def = (ParseTreeNode*)block->getChild(c);
						defaults_side[def->getName()] = def->getValue();
					}
				}

				// Sector defaults
				else if (S_CMPNOCASE(block->getName(), "sector")) {
					for (unsigned c = 0; c < block->nChildren(); c++) {
						ParseTreeNode* def = (ParseTreeNode*)block->getChild(c);
						defaults_sector[def->getName()] = def->getValue();
					}
				}

				// Thing defaults
				else if (S_CMPNOCASE(block->getName(), "thing")) {
					for (unsigned c = 0; c < block->nChildren(); c++) {
						ParseTreeNode* def = (ParseTreeNode*)block->getChild(c);
						defaults_thing[def->getName()] = def->getValue();
					}
				}

				else
					wxLogMessage("Unknown defaults block \"%s\"", CHR(block->getName()));
			}
		}

		// Maps section
		else if (S_CMPNOCASE(node->getName(), "maps")) {
			// Go through map blocks
			for (unsigned b = 0; b < node->nChildren(); b++) {
				ParseTreeNode* block = (ParseTreeNode*)node->getChild(b);

				// Map definition
				if (S_CMPNOCASE(block->getType(), "map")) {
					gc_mapinfo_t map;
					map.mapname = block->getName();

					// Go through map properties
					for (unsigned c = 0; c < block->nChildren(); c++) {
						ParseTreeNode* prop = (ParseTreeNode*)block->getChild(c);

						// Sky texture
						if (S_CMPNOCASE(prop->getName(), "sky")) {
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

		// Unknown/unexpected section
		else
			wxLogMessage("Warning: Unexpected game configuration section \"%s\", skipping", CHR(node->getName()));
	}

	wxLogMessage("Read game configuration \"%s\"", CHR(this->name));
	return true;
}
*/

/*
bool GameConfiguration::openEmbeddedConfig(ArchiveEntry* entry) {
// Check entry was given
if (!entry)
	return false;

// Build configuration string from entry (process #includes, etc)
string cfg;
buildConfig(entry, cfg);

if (readConfiguration(cfg, entry->getName())) {
	string name = readConfigName(entry->getMCData());

	// Add to list if valid
	if (!name.IsEmpty()) {
		gconf_t gc;
		gc.filename = entry->getParent()->getFilename();
		gc.title = name;
		game_configs.push_back(gc);
	}

	return true;
}
return false;
}

bool GameConfiguration::removeEmbeddedConfig(string name) {
if (game_configs.size() > lastDefaultConfig) {
	vector<gconf_t>::iterator it, stop = game_configs.begin() + lastDefaultConfig;
	for (it = game_configs.end() - 1; it >= stop; --it) {
		if (!name.Cmp(it->filename)) {
			game_configs.erase(it);
			return true;
		}
	}
}
return false;
}
*/

bool GameConfiguration::openConfig(string game, string port)
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
					wxLogMessage("Error: Game configuration file \"%s\" not found", CHR(filename));
					return false;
				}
			}
			else
			{
				// Config is in program resource
				string epath = S_FMT("config/games/%s.cfg", CHR(game_configs[a].filename));
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
						wxLogMessage("Error: Port configuration file \"%s\" not found", CHR(filename));
						return false;
					}
				}
				else
				{
					// Config is in program resource
					string epath = S_FMT("config/ports/%s.cfg", CHR(conf.filename));
					Archive* archive = theArchiveManager->programResourceArchive();
					ArchiveEntry* entry = archive->entryAtPath(epath);
					if (entry)
						buildConfig(entry, full_config);
				}
			}
		}
	}

	/*wxFile test("full.cfg", wxFile::write);
	test.Write(full_config);
	test.Close();*/

	// Read fully built configuration
	bool ok = true;
	if (readConfiguration(full_config))
	{
		current_game = game;
		current_port = port;
		game_configuration = game;
		port_configuration = port;
		wxLogMessage("Read game configuration \"%s\" + \"%s\"", CHR(current_game), CHR(current_port));
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
		if (!readConfiguration(config, cfg_entries[a]->getName(), true, false))
			wxLogMessage("Error reading embedded game configuration, not loaded");
	}

	return ok;
}

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

vector<as_t> GameConfiguration::allActionSpecials()
{
	vector<as_t> ret;

	// Build list
	ASpecialMap::iterator i = action_specials.begin();
	while (i != action_specials.end())
	{
		if (i->second.special)
		{
			as_t as(i->second.special);
			as.number = i->first;
			ret.push_back(as);
		}

		i++;
	}

	return ret;
}

ThingType* GameConfiguration::thingType(unsigned type)
{
	tt_t& ttype = thing_types[type];
	if (ttype.type)
		return ttype.type;
	else
		return &ttype_unknown;
}

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

string GameConfiguration::thingFlag(unsigned index)
{
	// Check index
	if (index >= flags_thing.size())
		return "";

	return flags_thing[index].name;
}

bool GameConfiguration::thingFlagSet(unsigned index, MapThing* thing)
{
	// Check index
	if (index >= flags_thing.size())
		return false;

	// Check if flag is set
	int flags = thing->intProperty("flags");
	if (flags & flags_thing[index].flag)
		return true;
	else
		return false;
}

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

	return ret;
}

void GameConfiguration::setThingFlag(unsigned index, MapThing* thing, bool set)
{
	// Check index
	if (index >= flags_thing.size())
		return;

	// Determine new flags value
	int flags = thing->intProperty("flags");
	if (set)
		flags |= flags_thing[index].flag;
	else
		flags = (flags & ~flags_thing[index].flag);

	// Update thing flags
	thing->setIntProperty("flags", flags);
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

			// Check for no editor number (ie can't be placed in the map)
			if (tz.peekToken() == "{")
			{
				LOG_MESSAGE(2, S_FMT("Not adding actor %s, no editor number", CHR(name)));

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

				// Check for actor definition open
				token = tz.getToken();
				if (token == "{")
				{
					token = tz.getToken();
					bool title_given = false;
					bool sprite_given = false;
					while (token != "}")
					{
						// Check for subsection
						if (token == "{")
							tz.skipSection("{", "}");

						// Title
						else if (S_CMPNOCASE(token, "//$Title"))
						{
							name = tz.getToken();
							title_given = true;
						}

						// Tag
						else if (!title_given && S_CMPNOCASE(token, "tag"))
							name = tz.getToken();

						// Category
						else if (S_CMPNOCASE(token, "//$Category"))
							group = tz.getToken();

						// Sprite
						else if (S_CMPNOCASE(token, "//$EditorSprite"))
						{
							found_props["sprite"] = tz.getToken();
							sprite_given = true;
						}
						else if (S_CMPNOCASE(token, "//$Sprite"))
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

						// Angled
						else if (S_CMPNOCASE(token, "//$Angled"))
							found_props["angled"] = true;
						else if (S_CMPNOCASE(token, "//$NotAngled"))
							found_props["angled"] = false;

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

						// Translation
						else if (S_CMPNOCASE(token, "translation"))
						{
							found_props["translation"] = tz.getToken();
							// TODO: multiple translation strings
						}

						// States
						if (!sprite_given && S_CMPNOCASE(token, "states"))
						{
							tz.skipToken(); // Skip {

							int statecounter = 0;
							string spritestate;
							int priority = 0;

							token = tz.getToken();
							while (token != "}")
							{
								// Idle, See, Inactive, Spawn, and finally first defined
								if (priority < SS_IDLE)
								{
									spritestate = token;
									token = tz.getToken();
									while (token.Cmp(":") && token.Cmp("}"))
									{
										spritestate = token;
										token = tz.getToken();
									}
									if (S_CMPNOCASE(token, "}"))
										break;
									string sb = tz.getToken(); // Sprite base
									string sf = tz.getToken(); // Sprite frame(s)
									string sprite = sb + sf.Left(1) + "?";
									int mypriority = 0;
									if (statecounter++ == 0)						mypriority = SS_FIRSTDEFINED;
									if (S_CMPNOCASE(spritestate, "spawn"))			mypriority = SS_SPAWN;
									else if (S_CMPNOCASE(spritestate, "inactive"))	mypriority = SS_INACTIVE;
									else if (S_CMPNOCASE(spritestate, "see"))		mypriority = SS_SEE;
									else if (S_CMPNOCASE(spritestate, "idle"))		mypriority = SS_IDLE;
									if (mypriority > priority)
									{
										priority = mypriority;
										found_props["sprite"] = sprite;
										LOG_MESSAGE(2, S_FMT("Actor %s found sprite %s from state %s", CHR(name), CHR(sprite), CHR(spritestate)));
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

					LOG_MESSAGE(2, S_FMT("Parsed actor %s: %d", CHR(name), type));
				}
				else
					LOG_MESSAGE(1, S_FMT("Warning: Invalid actor definition for %s", CHR(name)));

				// Create thing type object if needed
				if (!thing_types[type].type)
				{
					thing_types[type].type = new ThingType();
					thing_types[type].index = thing_types.size();
					thing_types[type].number = type;
					thing_types[type].type->decorate = true;
				}
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
						tt->copy(group_defaults);
				}

				// Setup thing
				tt->name = name;
				tt->group = group.empty() ? "Decorate" : group;
				if (found_props["sprite"].hasValue()) tt->sprite = found_props["sprite"].getStringValue();
				if (found_props["radius"].hasValue()) tt->radius = found_props["radius"].getIntValue();
				if (found_props["height"].hasValue()) tt->height = found_props["height"].getIntValue();
				if (found_props["hanging"].hasValue()) tt->hanging = found_props["hanging"].getBoolValue();
				if (found_props["angled"].hasValue()) tt->angled = found_props["angled"].getBoolValue();
				if (found_props["bright"].hasValue()) tt->fullbright = found_props["bright"].getBoolValue();
				if (found_props["decoration"].hasValue()) tt->decoration = found_props["decoration"].getBoolValue();
				if (found_props["icon"].hasValue()) tt->icon = found_props["icon"].getStringValue();
				if (found_props["translation"].hasValue()) tt->translation = found_props["translation"].getStringValue();
			}
		}

		token = tz.getToken();
	}

	//wxFile tempfile(appPath("decorate_full.txt", DIR_APP), wxFile::write);
	//tempfile.Write(full_defs);
	//tempfile.Close();

	return true;
}

string GameConfiguration::lineFlag(unsigned index)
{
	// Check index
	if (index >= flags_line.size())
		return "";

	return flags_line[index].name;
}

bool GameConfiguration::lineFlagSet(unsigned index, MapLine* line)
{
	// Check index
	if (index >= flags_line.size())
		return false;

	// Check if flag is set
	int flags = line->intProperty("flags");
	if (flags & flags_line[index].flag)
		return true;
	else
		return false;
}

bool GameConfiguration::lineBasicFlagSet(string flag, MapLine* line, int map_format)
{
	// If UDMF, just get the bool value
	if (map_format == MAP_UDMF)
		return line->boolProperty(flag);

	// Get current flags
	int flags = line->intProperty("flags");

	// Impassible
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

	// Unknown
	return false;
}

string GameConfiguration::lineFlagsString(MapLine* line)
{
	if (!line)
		return "";

	// Get raw flags
	int flags = line->intProperty("flags");
	// TODO: UDMF flags

	// Check against all flags
	string ret = "";
	for (unsigned a = 0; a < flags_line.size(); a++)
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

	return ret;
}

void GameConfiguration::setLineFlag(unsigned index, MapLine* line, bool set)
{
	// Check index
	if (index >= flags_line.size())
		return;

	// Determine new flags value
	int flags = line->intProperty("flags");
	if (set)
		flags |= flags_line[index].flag;
	else
		flags = (flags & ~flags_line[index].flag);

	// Update line flags
	line->setIntProperty("flags", flags);
}

void GameConfiguration::setLineBasicFlag(string flag, MapLine* line, int map_format, bool set)
{
	// If UDMF, just set the bool value
	if (map_format == MAP_UDMF)
	{
		line->setBoolProperty(flag, set);
		return;
	}

	// Get current flags
	int flags = line->intProperty("flags");
	int fval = 0;

	// Impassible
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
	if (set)
		line->setIntProperty("flags", flags|fval);
	else
		line->setIntProperty("flags", flags & ~fval);
}

string GameConfiguration::spacTriggerString(MapLine* line, int map_format)
{
	if (!line)
		return "";

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

wxArrayString GameConfiguration::allSpacTriggers()
{
	wxArrayString ret;

	for (unsigned a = 0; a < triggers_line.size(); a++)
		ret.Add(triggers_line[a].name);

	return ret;
}

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
		return NULL;
}

vector<udmfp_t> GameConfiguration::allUDMFProperties(int type)
{
	vector<udmfp_t> ret;

	// Build list depending on type
	UDMFPropMap* map = NULL;
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

void GameConfiguration::cleanObjectUDMFProps(MapObject* object)
{
	// Get UDMF properties list for type
	UDMFPropMap* map = NULL;
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

string GameConfiguration::sectorTypeName(int type, int map_format)
{
	// Check for zero type
	if (type == 0)
		return "Normal";

	// Deal with generalised flags
	vector<string> gen_flags;
	if (boom)
	{
		// Check what the map format is (the flag bits differ between doom/hexen format)
		if (map_format == MAP_DOOM && type >= 32)
		{
			// Damage flags
			if ((type & 96) == 96)
				gen_flags.push_back("20% Damage");
			else if (type & 32)
				gen_flags.push_back("5% Damage");
			else if (type & 64)
				gen_flags.push_back("10% Damage");

			// Secret
			if (type & 128)
				gen_flags.push_back("Secret");

			// Friction
			if (type & 256)
				gen_flags.push_back("Friction Enabled");

			// Pushers/Pullers
			if (type & 512)
				gen_flags.push_back("Pushers/Pullers Enabled");

			// Remove flag bits from type value
			type = type & 31;
		}
		else if (type >= 256)
		{
			// Damage flags
			if ((type & 768) == 768)
				gen_flags.push_back("20% Damage");
			else if (type & 256)
				gen_flags.push_back("5% Damage");
			else if (type & 512)
				gen_flags.push_back("10% Damage");

			// Secret
			if (type & 1024)
				gen_flags.push_back("Secret");

			// Friction
			if (type & 2048)
				gen_flags.push_back("Friction Enabled");

			// Pushers/Pullers
			if (type & 4096)
				gen_flags.push_back("Pushers/Pullers Enabled");

			// Remove flag bits from type value
			type = type & 255;
		}
	}

	// Check if the type only has generalised flags
	if (type == 0 && gen_flags.size() > 0)
	{
		// Just return flags in this case
		string name = gen_flags[0];
		for (unsigned a = 1; a < gen_flags.size(); a++)
			name += S_FMT(" + %s", CHR(gen_flags[a]));

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
		name += S_FMT(" + %s", CHR(gen_flags[a]));

	return name;
}

//vector<string> GameConfiguration::allSectorTypeNames() {
//	vector<string> ret;
//	for (unsigned a = 0; a < sector_types.size(); a++)
//		ret.push_back(sector_types[a].name);
//	return ret;
//}

int GameConfiguration::sectorTypeByName(string name)
{
	for (unsigned a = 0; a < sector_types.size(); a++)
	{
		if (sector_types[a].name == name)
			return sector_types[a].type;
	}

	return 0;
}

int GameConfiguration::baseSectorType(int type, int map_format)
{
	// No type
	if (type == 0)
		return 0;

	// Strip boom flags depending on map format
	if (map_format == MAP_DOOM && type >= 32)
		return type & 31;
	else if (type >= 256)
		return type & 255;

	// No flags
	return type;
}

int GameConfiguration::sectorBoomDamage(int type, int map_format)
{
	// No type
	if (type == 0)
		return 0;

	// Doom format
	if (map_format == MAP_DOOM && type >= 32)
	{
		if ((type & 96) == 96)
			return 3;
		else if (type & 32)
			return 1;
		else if (type & 64)
			return 2;
	}

	// Hexen format
	else if (type >= 256)
	{
		if ((type & 768) == 768)
			return 3;
		else if (type & 256)
			return 1;
		else if (type & 512)
			return 2;
	}

	// No damage
	return 0;
}

bool GameConfiguration::sectorBoomSecret(int type, int map_format)
{
	// No type
	if (type == 0)
		return false;

	// Doom format
	if (map_format == MAP_DOOM && type >= 32 && type & 128)
		return true;

	// Hexen format
	else if (type >= 256 && type & 1024)
		return true;

	// Not secret
	return false;
}

bool GameConfiguration::sectorBoomFriction(int type, int map_format)
{
	// No type
	if (type == 0)
		return false;

	// Doom format
	if (map_format == MAP_DOOM && type >= 32 && type & 256)
		return true;

	// Hexen format
	else if (type >= 256 && type & 2048)
		return true;

	// Friction disabled
	return false;
}

bool GameConfiguration::sectorBoomPushPull(int type, int map_format)
{
	// No type
	if (type == 0)
		return false;

	// Doom format
	if (map_format == MAP_DOOM && type >= 32 && type & 512)
		return true;

	// Hexen format
	else if (type >= 256 && type & 4096)
		return true;

	// Pusher/Puller disabled
	return false;
}

int GameConfiguration::boomSectorType(int base, int damage, bool secret, bool friction, bool pushpull, int map_format)
{
	int fulltype = base;

	// Doom format
	if (map_format == MAP_DOOM)
	{
		// Damage
		if (damage == 1)
			fulltype += 32;
		else if (damage == 2)
			fulltype += 64;
		else if (damage == 3)
			fulltype += 96;

		// Secret
		if (secret)
			fulltype += 128;

		// Friction
		if (friction)
			fulltype += 256;

		// Pusher/Puller
		if (pushpull)
			fulltype += 512;
	}

	// Hexen format
	else
	{
		// Damage
		if (damage == 1)
			fulltype += 256;
		else if (damage == 2)
			fulltype += 512;
		else if (damage == 3)
			fulltype += 768;

		// Secret
		if (secret)
			fulltype += 1024;

		// Friction
		if (friction)
			fulltype += 2048;

		// Pusher/Puller
		if (pushpull)
			fulltype += 4096;
	}

	return fulltype;
}

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

void GameConfiguration::applyDefaults(MapObject* object)
{
	// Get all defaults for the object type
	vector<string> prop_names;
	vector<Property> prop_vals;

	// Line defaults
	if (object->getObjType() == MOBJ_LINE)
	{
		defaults_line.allProperties(prop_vals);
		defaults_line.allPropertyNames(prop_names);
	}

	// Side defaults
	else if (object->getObjType() == MOBJ_SIDE)
	{
		defaults_side.allProperties(prop_vals);
		defaults_side.allPropertyNames(prop_names);
	}

	// Sector defaults
	else if (object->getObjType() == MOBJ_SECTOR)
	{
		defaults_sector.allProperties(prop_vals);
		defaults_sector.allPropertyNames(prop_names);
	}

	// Thing defaults
	else if (object->getObjType() == MOBJ_THING)
	{
		defaults_thing.allProperties(prop_vals);
		defaults_thing.allPropertyNames(prop_names);
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
	}
}

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




void GameConfiguration::dumpActionSpecials()
{
	ASpecialMap::iterator i = action_specials.begin();

	while (i != action_specials.end())
	{
		wxLogMessage("Action special %d = %s", i->first, CHR(i->second.special->stringDesc()));
		i++;
	}
}

void GameConfiguration::dumpThingTypes()
{
	ThingTypeMap::iterator i = thing_types.begin();

	while (i != thing_types.end())
	{
		wxLogMessage("Thing type %d = %s", i->first, CHR(i->second.type->stringDesc()));
		i++;
	}
}

void GameConfiguration::dumpValidMapNames()
{
	wxLogMessage("Valid Map Names:");
	for (unsigned a = 0; a < maps.size(); a++)
		wxLogMessage(maps[a].mapname);
}

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
