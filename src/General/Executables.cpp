
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Executables.cpp
// Description: Functions to handle game executable configurations for the
//              'Run Archive/Map' dialog and external edit system
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
#include "Executables.h"
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "Utility/Parser.h"
#include "Utility/StringUtils.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace Executables
{
vector<GameExe>     game_exes;
vector<StringPair>  exe_paths;
vector<ExternalExe> external_exes;
} // namespace Executables


// -----------------------------------------------------------------------------
//
// Executables Namespace Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the game executable definition for [id]
// -----------------------------------------------------------------------------
Executables::GameExe* Executables::gameExe(string_view id)
{
	for (auto& exe : game_exes)
		if (exe.id == id)
			return &exe;

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the game executable definition at [index]
// -----------------------------------------------------------------------------
Executables::GameExe* Executables::gameExe(unsigned index)
{
	if (index < game_exes.size())
		return &(game_exes[index]);
	else
		return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the number of game executables defined
// -----------------------------------------------------------------------------
unsigned Executables::nGameExes()
{
	return game_exes.size();
}

// -----------------------------------------------------------------------------
// Sets the path of game executable [id] to [path]
// -----------------------------------------------------------------------------
void Executables::setGameExePath(string_view id, string_view path)
{
	exe_paths.emplace_back(id, path);
}

// -----------------------------------------------------------------------------
// Writes all game executable paths as a string (for slade3.cfg)
// -----------------------------------------------------------------------------
string Executables::writePaths()
{
	string ret;

	for (auto& exe : game_exes)
		ret += fmt::format("\t{} \"{}\"\n", exe.id, StrUtil::escapedString(exe.path, true));

	return ret;
}

// -----------------------------------------------------------------------------
// Writes all executable definitions as text
// -----------------------------------------------------------------------------
string Executables::writeExecutables()
{
	string ret = "executables\n{{\n";

	// Write game exes
	for (auto& exe : game_exes)
	{
		// ID
		ret += fmt::format("\tgame_exe {}\n\t{{\n", exe.id);

		// Name
		ret += fmt::format("\t\tname = \"{}\";\n", exe.name);

		// Exe name
		ret += fmt::format("\t\texe_name = \"{}\";\n\n", exe.exe_name);

		// Configs
		for (auto& config : exe.configs)
			ret += fmt::format("\t\tconfig \"{}\" = \"{}\";\n", config.first, StrUtil::escapedString(config.second));

		ret += "\t}}\n\n";
	}

	// Write external exes
	for (auto& exe : external_exes)
	{
		// Name
		ret += fmt::format("\texternal_exe \"{}\"\n\t{{\n", exe.name);

		// Entry Category
		ret += fmt::format("\t\tcategory = \"{}\";\n", exe.category);

		// Path
		auto path = exe.path;
		std::replace(path.begin(), path.end(), '\\', '/');
		ret += fmt::format("\t\tpath = \"{}\";\n", path);

		ret += "\t}}\n\n";
	}

	ret += "}}\n";

	return ret;
}

// -----------------------------------------------------------------------------
// Reads all executable definitions from the program resource and user dir
// -----------------------------------------------------------------------------
void Executables::init()
{
	// Load from pk3
	auto res_archive = App::archiveManager().programResourceArchive();
	auto entry       = res_archive->entryAtPath("config/executables.cfg");
	if (!entry)
		return;

	// Parse base executables config
	Parser p;
	p.parseText(entry->data(), "slade.pk3 - executables.cfg");
	parse(&p, false);

	// Parse user executables config
	Parser   p2;
	MemChunk mc;
	if (mc.importFile(App::path("executables.cfg", App::Dir::User)))
	{
		p2.parseText(mc, "user execuatbles.cfg");
		parse(&p2, true);
	}
}

// -----------------------------------------------------------------------------
// Parses an executables configuration from [p]
// -----------------------------------------------------------------------------
void Executables::parse(Parser* p, bool custom)
{
	auto n = p->parseTreeRoot()->childPTN("executables");
	if (!n)
		return;

	for (unsigned a = 0; a < n->nChildren(); a++)
	{
		auto exe_node = n->childPTN(a);
		auto type     = exe_node->type();

		// Game Executable (if type is blank it's a game executable in old config format)
		if (type == "game_exe" || type.empty())
			parseGameExe(exe_node, custom);

		// External Executable
		else if (type == "external_exe")
			parseExternalExe(exe_node);
	}
}

// -----------------------------------------------------------------------------
// Parses a game executable config from [node]
// -----------------------------------------------------------------------------
void Executables::parseGameExe(ParseTreeNode* node, bool custom)
{
	// Get GameExe being parsed
	auto exe = gameExe(StrUtil::lower(node->name()));
	if (!exe)
	{
		// Create if new
		GameExe nexe;
		nexe.custom = custom;
		game_exes.push_back(nexe);
		exe = &(game_exes.back());
	}

	exe->id = node->name();
	for (unsigned b = 0; b < node->nChildren(); b++)
	{
		auto prop = node->childPTN(b);

		// Config
		if (StrUtil::equalCI(prop->type(), "config"))
		{
			// Update if exists
			bool found = false;
			for (auto& config : exe->configs)
			{
				if (config.first == prop->name())
				{
					config.second = prop->stringValue();
					found         = true;
				}
			}

			// Create if new
			if (!found)
			{
				exe->configs.emplace_back(prop->name(), prop->stringValue());
				exe->configs_custom.push_back(custom);
			}
		}

		// Name
		else if (prop->nameIsCI("name"))
			exe->name = prop->stringValue();

		// Executable name
		else if (prop->nameIsCI("exe_name"))
			exe->exe_name = prop->stringValue();
	}

	// Set path if loaded
	for (auto& path : exe_paths)
		if (path.first == exe->id)
			exe->path = path.second;
}

// -----------------------------------------------------------------------------
// Adds a new game executable definition for game [name]
// -----------------------------------------------------------------------------
void Executables::addGameExe(string_view name)
{
	GameExe game;
	game.name = name;

	game.id = StrUtil::lower(name);
	std::replace(game.id.begin(), game.id.end(), ' ', '_');

	game_exes.push_back(game);
}

// -----------------------------------------------------------------------------
// Removes the game executable definition at [index]
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// Adds a run configuration for game executable at [exe_index]
// -----------------------------------------------------------------------------
void Executables::addGameExeConfig(unsigned exe_index, string_view config_name, string_view config_params, bool custom)
{
	// Check index
	if (exe_index >= game_exes.size())
		return;

	game_exes[exe_index].configs.emplace_back(config_name, config_params);
	game_exes[exe_index].configs_custom.push_back(custom);
}

// -----------------------------------------------------------------------------
// Removes run configuration at [config_index] in game exe definition at
// [exe_index]
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// Returns the number of external executables for [category], or all if
// [category] is not specified
// -----------------------------------------------------------------------------
int Executables::nExternalExes(string_view category)
{
	int num = 0;
	for (auto& exe : external_exes)
		if (category.empty() || exe.category == category)
			num++;

	return num;
}

// -----------------------------------------------------------------------------
// Returns the external executable matching [name] and [category].
// If [category] is empty, it is ignored
// -----------------------------------------------------------------------------
Executables::ExternalExe Executables::externalExe(string_view name, string_view category)
{
	for (auto& exe : external_exes)
		if (category.empty() || exe.category == category)
			if (exe.name == name)
				return exe;

	return ExternalExe();
}

// -----------------------------------------------------------------------------
// Returns a list of all external executables matching [category].
// If [category] is empty, it is ignored
// -----------------------------------------------------------------------------
vector<Executables::ExternalExe> Executables::externalExes(string_view category)
{
	vector<ExternalExe> ret;
	for (auto& exe : external_exes)
		if (category.empty() || exe.category == category)
			ret.push_back(exe);

	return ret;
}

// -----------------------------------------------------------------------------
// Parses an external executable config from [node]
// -----------------------------------------------------------------------------
void Executables::parseExternalExe(ParseTreeNode* node)
{
	ExternalExe exe;
	exe.name = node->name();

	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		auto prop = node->childPTN(a);

		// Entry category
		if (prop->nameIsCI("category"))
			exe.category = prop->stringValue();

		// Path
		else if (prop->nameIsCI("path"))
			exe.path = prop->stringValue();
	}

	external_exes.push_back(exe);
}

// -----------------------------------------------------------------------------
// Adds a new external executable, if one matching [name] and [category] doesn't
// already exist
// -----------------------------------------------------------------------------
void Executables::addExternalExe(string_view name, string_view path, string_view category)
{
	// Check it doesn't already exist
	for (auto& exe : external_exes)
		if (exe.name == name && exe.category == category)
			return;

	ExternalExe exe;
	exe.name     = name;
	exe.path     = path;
	exe.category = category;
	external_exes.push_back(exe);
}

// -----------------------------------------------------------------------------
// Sets the name of the external executable matching [name_old] and [category]
// to [name_new]
// -----------------------------------------------------------------------------
void Executables::setExternalExeName(string_view name_old, string_view name_new, string_view category)
{
	for (auto& exe : external_exes)
		if (exe.name == name_old && exe.category == category)
		{
			exe.name = name_new;
			return;
		}
}

// -----------------------------------------------------------------------------
// Sets the path of the external executable matching [name] and [category] to
// [path]
// -----------------------------------------------------------------------------
void Executables::setExternalExePath(string_view name, string_view path, string_view category)
{
	for (auto& exe : external_exes)
		if (exe.name == name && exe.category == category)
		{
			exe.path = path;
			return;
		}
}

// -----------------------------------------------------------------------------
// Removes the external executable matching [name] and [category]
// -----------------------------------------------------------------------------
void Executables::removeExternalExe(string_view name, string_view category)
{
	for (unsigned a = 0; a < external_exes.size(); a++)
		if (external_exes[a].name == name && external_exes[a].category == category)
		{
			external_exes.erase(external_exes.begin() + a);
			return;
		}
}
