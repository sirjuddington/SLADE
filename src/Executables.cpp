
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    Executables.cpp
 * Description: Functions to handle game executable configurations
 *              for the 'Run Archive/Map' dialog
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
#include "Executables.h"
#include "ArchiveManager.h"
#include "Parser.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
namespace Executables
{
	vector<game_exe_t>	game_exes;
	vector<key_value_t>	exe_paths;
}


/*******************************************************************
 * EXECUTABLES NAMESPACE FUNCTIONS
 *******************************************************************/

/* Executables::getGameExe
 * Returns the game executable definition for [id]
 *******************************************************************/
Executables::game_exe_t* Executables::getGameExe(string id)
{
	for (unsigned a = 0; a < game_exes.size(); a++)
	{
		if (game_exes[a].id == id)
			return &(game_exes[a]);
	}

	return NULL;
}

/* Executables::getGameExe
 * Returns the game executable definition at [index]
 *******************************************************************/
Executables::game_exe_t* Executables::getGameExe(unsigned index)
{
	if (index < game_exes.size())
		return &(game_exes[index]);
	else
		return NULL;
}

/* Executables::nGameExes
 * Returns the number of game executables defined
 *******************************************************************/
unsigned Executables::nGameExes()
{
	return game_exes.size();
}

/* Executables::setExePath
 * Sets the path of executable [id] to [path]
 *******************************************************************/
void Executables::setExePath(string id, string path)
{
	exe_paths.push_back(key_value_t(id, path));
}

/* Executables::writePaths
 * Writes all game executable paths as a string (for slade3.cfg)
 *******************************************************************/
string Executables::writePaths()
{
	string ret;
	for (unsigned a = 0; a < game_exes.size(); a++)
	{
		string path = game_exes[a].path;
		path.Replace("\\", "/");
		ret += S_FMT("\t%s \"%s\"\n", game_exes[a].id, path);
	}
	return ret;
}

/* Executables::writeExecutables
 * Writes all game executable definitions as text
 *******************************************************************/
string Executables::writeExecutables()
{
	string ret = "executables\n{\n";

	// Go through game exes
	for (unsigned a = 0; a < game_exes.size(); a++)
	{
		// ID
		ret += S_FMT("\t%s\n\t{\n", game_exes[a].id);

		// Name
		ret += S_FMT("\t\tname = \"%s\";\n", game_exes[a].name);

		// Exe name
		ret += S_FMT("\t\texe_name = \"%s\";\n\n", game_exes[a].exe_name);

		// Configs
		for (unsigned b = 0; b < game_exes[a].configs.size(); b++)
			ret += S_FMT("\t\tconfig \"%s\" = \"%s\";\n", game_exes[a].configs[b].key, game_exes[a].configs[b].value);

		ret += "\t}\n\n";
	}

	ret += "}";

	return ret;
}

/* Executables::init
 * Reads all game executables definitions from the program resource
 * and user dir
 *******************************************************************/
void Executables::init()
{
	// Load from pk3
	Archive* res_archive = theArchiveManager->programResourceArchive();
	ArchiveEntry* entry = res_archive->entryAtPath("config/executables.cfg");
	if (!entry)
		return;

	// Parse base executables config
	Parser p;
	p.parseText(entry->getMCData(), "slade.pk3 - executables.cfg");
	parse(&p, false);

	// Parse user executables config
	Parser p2;
	MemChunk mc;
	if (mc.importFile(appPath("executables.cfg", DIR_USER)))
	{
		p2.parseText(mc, "user execuatbles.cfg");
		parse(&p2, true);
	}
}

/* Executables::parse
 * Parses a game executables configuration from [p]
 *******************************************************************/
void Executables::parse(Parser* p, bool custom)
{
	ParseTreeNode* n = (ParseTreeNode*)p->parseTreeRoot()->getChild("executables");
	if (!n) return;

	for (unsigned a = 0; a < n->nChildren(); a++)
	{
		// Get game_exe_t being parsed
		ParseTreeNode* exe_node = (ParseTreeNode*)n->getChild(a);
		game_exe_t* exe = getGameExe(exe_node->getName().Lower());
		if (!exe)
		{
			// Create if new
			game_exe_t nexe;
			nexe.custom = custom;
			game_exes.push_back(nexe);
			exe = &(game_exes.back());
		}

		exe->id = exe_node->getName();
		for (unsigned b = 0; b < exe_node->nChildren(); b++)
		{
			ParseTreeNode* prop = (ParseTreeNode*)exe_node->getChild(b);
			string prop_name = prop->getName().Lower();

			// Config
			if (prop->getType().Lower() == "config")
			{
				// Update if exists
				bool found = false;
				for (unsigned c = 0; c < exe->configs.size(); c++)
				{
					if (exe->configs[c].key == prop->getName())
					{
						exe->configs[c].value = prop->getStringValue();
						found = true;
					}
				}

				// Create if new
				if (!found)
				{
					exe->configs.push_back(key_value_t(prop->getName(), prop->getStringValue()));
					exe->configs_custom.push_back(custom);
				}
			}

			// Name
			else if (prop_name == "name")
				exe->name = prop->getStringValue();

			// Executable name
			else if (prop_name == "exe_name")
				exe->exe_name = prop->getStringValue();
		}

		// Set path if loaded
		for (unsigned pa = 0; pa < exe_paths.size(); pa++)
		{
			if (exe_paths[pa].key == exe->id)
				exe->path = exe_paths[pa].value;
		}
	}
}

/* Executables::addGameExe
 * Adds a new game executable definition for game [name]
 *******************************************************************/
void Executables::addGameExe(string name)
{
	game_exe_t game;
	game.name = name;
	
	name.Replace(" ", "_");
	name.MakeLower();
	game.id = name;

	game_exes.push_back(game);
}

/* Executables::removeGameExe
 * Removes the game executable definition at [index]
 *******************************************************************/
bool Executables::removeGameExe(unsigned index)
{
	if (index < game_exes.size())
	{
		if (game_exes[index].custom)
		{
			VECTOR_REMOVE_AT(game_exes, index);
			return true;
		}
	}

	return false;
}

/* Executables::addGameExeConfig
 * Adds a run configuration for game executable at [exe_index]
 *******************************************************************/
void Executables::addGameExeConfig(unsigned exe_index, string config_name, string config_params, bool custom)
{
	// Check index
	if (exe_index >= game_exes.size())
		return;

	game_exes[exe_index].configs.push_back(key_value_t(config_name, config_params));
	game_exes[exe_index].configs_custom.push_back(custom);
}

/* Executables::removeGameExeConfig
 * Removes run configuration at [config_index] in game exe definition
 * at [exe_index]
 *******************************************************************/
bool Executables::removeGameExeConfig(unsigned exe_index, unsigned config_index)
{
	// Check indices
	if (exe_index >= game_exes.size())
		return false;
	if (config_index >= game_exes[exe_index].configs.size())
		return false;

	// Check config is custom
	if (game_exes[exe_index].configs_custom[config_index])
	{
		VECTOR_REMOVE_AT(game_exes[exe_index].configs, config_index);
		VECTOR_REMOVE_AT(game_exes[exe_index].configs_custom, config_index);

		return true;
	}
	else
		return false;
}
