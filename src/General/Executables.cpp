
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
#include "Archive/ArchiveManager.h"
#include "Utility/Parser.h"
#include "Utility/StringUtils.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
namespace Executables
{
	vector<game_exe_t>		game_exes;
	vector<key_value_t>		exe_paths;
	vector<external_exe_t>	external_exes;
}


/*******************************************************************
 * EXECUTABLES NAMESPACE FUNCTIONS
 *******************************************************************/

/* Executables::getGameExe
 * Returns the game executable definition for [id]
 *******************************************************************/
Executables::game_exe_t* Executables::getGameExe(string id)
{
	for (auto& exe : game_exes)
		if (exe.id == id)
			return &exe;

	return nullptr;
}

/* Executables::getGameExe
 * Returns the game executable definition at [index]
 *******************************************************************/
Executables::game_exe_t* Executables::getGameExe(unsigned index)
{
	if (index < game_exes.size())
		return &(game_exes[index]);
	else
		return nullptr;
}

/* Executables::nGameExes
 * Returns the number of game executables defined
 *******************************************************************/
unsigned Executables::nGameExes()
{
	return game_exes.size();
}

/* Executables::setGameExePath
 * Sets the path of game executable [id] to [path]
 *******************************************************************/
void Executables::setGameExePath(string id, string path)
{
	exe_paths.push_back({ id, path });
}

/* Executables::writePaths
 * Writes all game executable paths as a string (for slade3.cfg)
 *******************************************************************/
string Executables::writePaths()
{
	string ret;

	for (auto& exe : game_exes)
		ret += S_FMT("\t%s \"%s\"\n", exe.id, StringUtils::escapedString(exe.path, true));

	return ret;
}

/* Executables::writeExecutables
 * Writes all executable definitions as text
 *******************************************************************/
string Executables::writeExecutables()
{
	string ret = "executables\n{\n";

	// Write game exes
	for (auto& exe : game_exes)
	{
		// ID
		ret += S_FMT("\tgame_exe %s\n\t{\n", exe.id);

		// Name
		ret += S_FMT("\t\tname = \"%s\";\n", exe.name);

		// Exe name
		ret += S_FMT("\t\texe_name = \"%s\";\n\n", exe.exe_name);

		// Configs
		for (auto& config : exe.configs)
			ret += S_FMT(
				"\t\tconfig \"%s\" = \"%s\";\n",
				config.key,
				StringUtils::escapedString(config.value)
			);

		ret += "\t}\n\n";
	}

	// Write external exes
	for (auto& exe : external_exes)
	{
		// Name
		ret += S_FMT("\texternal_exe \"%s\"\n\t{\n", exe.name);

		// Entry Category
		ret += S_FMT("\t\tcategory = \"%s\";\n", exe.category);

		// Path
		string path = exe.path;
		path.Replace("\\", "/");
		ret += S_FMT("\t\tpath = \"%s\";\n", path);

		ret += "\t}\n\n";
	}

	ret += "}\n";

	return ret;
}

/* Executables::init
 * Reads all executable definitions from the program resource
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
 * Parses an executables configuration from [p]
 *******************************************************************/
void Executables::parse(Parser* p, bool custom)
{
	ParseTreeNode* n = (ParseTreeNode*)p->parseTreeRoot()->getChild("executables");
	if (!n) return;

	for (unsigned a = 0; a < n->nChildren(); a++)
	{
		ParseTreeNode* exe_node = (ParseTreeNode*)n->getChild(a);
		string type = exe_node->getType();

		// Game Executable (if type is blank it's a game executable in old config format)
		if (type == "game_exe" || type.IsEmpty())
			parseGameExe(exe_node, custom);

		// External Executable
		else if (type == "external_exe")
			parseExternalExe(exe_node);
	}
}

/* Executables::parseGameExe
 * Parses a game executable config from [node]
 *******************************************************************/
void Executables::parseGameExe(ParseTreeNode* node, bool custom)
{
	// Get game_exe_t being parsed
	game_exe_t* exe = getGameExe(node->getName().Lower());
	if (!exe)
	{
		// Create if new
		game_exe_t nexe;
		nexe.custom = custom;
		game_exes.push_back(nexe);
		exe = &(game_exes.back());
	}

	exe->id = node->getName();
	for (unsigned b = 0; b < node->nChildren(); b++)
	{
		ParseTreeNode* prop = (ParseTreeNode*)node->getChild(b);
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
	for (auto& path : exe_paths)
		if (path.key == exe->id)
			exe->path = path.value;
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

/* Executables::nExternalExes
 * Returns the number of external executables for [category], or
 * all if [category] is not specified
 *******************************************************************/
int Executables::nExternalExes(string category)
{
	int num = 0;
	for (auto& exe : external_exes)
		if (category.IsEmpty() || exe.category == category)
			num++;

	return num;
}

/* Executables::getExternalExe
 * Returns the external executable matching [name] and [category].
 * If [category] is empty, it is ignored
 *******************************************************************/
Executables::external_exe_t Executables::getExternalExe(string name, string category)
{
	for (auto& exe : external_exes)
		if (category.IsEmpty() || exe.category == category)
			if (exe.name == name)
				return exe;

	return external_exe_t();
}

/* Executables::getExternalExe
 * Returns a list of all external executables matching [category].
 * If [category] is empty, it is ignored
 *******************************************************************/
vector<Executables::external_exe_t> Executables::getExternalExes(string category)
{
	vector<external_exe_t> ret;
	for (auto& exe : external_exes)
		if (category.IsEmpty() || exe.category == category)
			ret.push_back(exe);

	return ret;
}

/* Executables::parseExternalExe
 * Parses an external executable config from [node]
 *******************************************************************/
void Executables::parseExternalExe(ParseTreeNode* node)
{
	external_exe_t exe;
	exe.name = node->getName();

	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		ParseTreeNode* prop = (ParseTreeNode*)node->getChild(a);
		string prop_name = prop->getName().Lower();

		// Entry category
		if (prop_name == "category")
			exe.category = prop->getStringValue();

		// Path
		else if (prop_name == "path")
			exe.path = prop->getStringValue();
	}

	external_exes.push_back(exe);
}

/* Executables::addExternalExe
 * Adds a new external executable, if one matching [name] and
 * [category] doesn't already exist
 *******************************************************************/
void Executables::addExternalExe(string name, string path, string category)
{
	// Check it doesn't already exist
	for (auto& exe : external_exes)
		if (exe.name == name && exe.category == category)
			return;

	external_exe_t exe;
	exe.name = name;
	exe.path = path;
	exe.category = category;
	external_exes.push_back(exe);
}

/* Executables::setExternalExeName
 * Sets the name of the external executable matching [name_old] and
 * [category] to [name_new]
 *******************************************************************/
void Executables::setExternalExeName(string name_old, string name_new, string category)
{
	for (auto& exe : external_exes)
		if (exe.name == name_old && exe.category == category)
		{
			exe.name = name_new;
			return;
		}
}

/* Executables::setExternalExePath
 * Sets the path of the external executable matching [name] and
 * [category] to [path]
 *******************************************************************/
void Executables::setExternalExePath(string name, string path, string category)
{
	for (auto& exe : external_exes)
		if (exe.name == name && exe.category == category)
		{
			exe.path = path;
			return;
		}
}

/* Executables::removeExternalExe
 * Removes the external executable matching [name] and [category]
 *******************************************************************/
void Executables::removeExternalExe(string name, string category)
{
	for (unsigned a = 0; a < external_exes.size(); a++)
		if (external_exes[a].name == name && external_exes[a].category == category)
		{
			external_exes.erase(external_exes.begin() + a);
			return;
		}
}
