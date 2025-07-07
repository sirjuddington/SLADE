
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveManager.h"
#include "Utility/JsonUtils.h"
#include "Utility/Parser.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace slade::executables
{
vector<GameExe>     game_exes;
vector<StringPair>  exe_paths;
vector<ExternalExe> external_exes;
} // namespace slade::executables


// -----------------------------------------------------------------------------
//
// Executables Namespace Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the game executable definition for [id]
// -----------------------------------------------------------------------------
executables::GameExe* executables::gameExe(string_view id)
{
	for (auto& exe : game_exes)
		if (exe.id == id)
			return &exe;

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the game executable definition at [index]
// -----------------------------------------------------------------------------
executables::GameExe* executables::gameExe(unsigned index)
{
	if (index < game_exes.size())
		return &(game_exes[index]);
	else
		return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the number of game executables defined
// -----------------------------------------------------------------------------
unsigned executables::nGameExes()
{
	return game_exes.size();
}

// -----------------------------------------------------------------------------
// Sets the path of game executable [id] to [path]
// -----------------------------------------------------------------------------
void executables::setGameExePath(string_view id, string_view path)
{
	exe_paths.emplace_back(id, path);
}

// -----------------------------------------------------------------------------
// Writes all game executable paths to json object [j]
// -----------------------------------------------------------------------------
void executables::writePaths(nlohmann::json& j)
{
	for (auto& exe : game_exes)
		j["executable_paths"][exe.id] = strutil::escapedString(exe.path, true);
}

// -----------------------------------------------------------------------------
// Writes all executable definitions as JSON to a file at [path]
// -----------------------------------------------------------------------------
bool executables::writeExecutables(string_view path)
{
	ordered_json j;

	// Game executables
	for (auto& exe : game_exes)
	{
		auto j_gameexe = ordered_json{ { "name", exe.name }, { "file_name", exe.exe_name } };

		// Run configs
		auto configs = ordered_json::array();
		for (auto& [name, command] : exe.run_configs)
			configs.push_back(ordered_json{ { "name", name }, { "command", command } });
		j_gameexe["configs"] = configs;

		// Map run configs
		auto map_configs = ordered_json::array();
		for (auto& [name, command] : exe.map_configs)
			map_configs.push_back(ordered_json{ { "name", name }, { "command", command } });
		j_gameexe["map_configs"] = map_configs;

		j["game_executables"][exe.id] = j_gameexe;
	}

	// External executables
	auto j_externalexes = ordered_json::array();
	for (auto& exe : external_exes)
	{
		j_externalexes.push_back(ordered_json{ { "name", exe.name }, { "category", exe.category } });
		auto path = exe.path;
		std::replace(path.begin(), path.end(), '\\', '/');
		j_externalexes.back()["path"] = path;
	}
	j["external_executables"] = j_externalexes;

	// Write to file
	return jsonutil::writeFile(j, path);
}

// -----------------------------------------------------------------------------
// Reads all executable definitions from json object [j]
// If [custom] is true, the executables are considered user-defined/custom
// (and will be saved to the user executables config file)
// -----------------------------------------------------------------------------
void executables::readExecutables(nlohmann::json& j, bool custom)
{
	// Read game executables
	if (j.contains("game_executables"))
	{
		for (auto& [id, j_game_exe] : j["game_executables"].items())
		{
			// Get GameExe being parsed
			auto exe = gameExe(strutil::lower(id));
			if (!exe)
			{
				// Create if new
				GameExe nexe;
				nexe.id     = strutil::lower(id);
				nexe.custom = custom;
				game_exes.push_back(nexe);
				exe = &(game_exes.back());
			}

			// Basic info
			exe->name = j_game_exe["name"];
			if (j_game_exe.contains("file_name"))
				exe->exe_name = j_game_exe["file_name"];

			// Run configs
			if (j_game_exe.contains("configs") && j_game_exe["configs"].is_array())
			{
				for (auto& j_config : j_game_exe["configs"])
				{
					// Update if exists
					bool found = false;
					for (auto& config : exe->run_configs)
					{
						if (config.first == j_config["name"])
						{
							config.second = j_config["command"];
							found         = true;
						}
					}

					// Create if new
					if (!found)
					{
						exe->run_configs.emplace_back(j_config["name"], j_config["command"]);
						exe->run_configs_custom.push_back(custom);
					}
				}
			}

			// Map Run configs
			if (j_game_exe.contains("map_configs") && j_game_exe["map_configs"].is_array())
			{
				for (auto& j_config : j_game_exe["map_configs"])
				{
					// Update if exists
					bool found = false;
					for (auto& config : exe->map_configs)
					{
						if (config.first == j_config["name"])
						{
							config.second = j_config["command"];
							found         = true;
						}
					}

					// Create if new
					if (!found)
					{
						exe->map_configs.emplace_back(j_config["name"], j_config["command"]);
						exe->map_configs_custom.push_back(custom);
					}
				}
			}

			// Set path if loaded
			for (auto& path : exe_paths)
				if (path.first == exe->id)
					exe->path = path.second;
		}
	}

	// Read external executables
	if (j.contains("external_executables"))
	{
		for (auto& j_ext_exe : j["external_executables"])
		{
			ExternalExe exe;
			exe.name     = j_ext_exe["name"];
			exe.category = j_ext_exe["category"];
			exe.path     = j_ext_exe["path"];
			external_exes.push_back(exe);
		}
	}
}

// -----------------------------------------------------------------------------
// Reads all executable definitions from the program resource and user dir
// -----------------------------------------------------------------------------
void executables::init()
{
	// Load from pk3
	auto res_archive = app::archiveManager().programResourceArchive();
	auto entry       = res_archive->entryAtPath("config/executables.json");
	if (!entry)
		return;

	// Parse base executables config
	if (auto j = jsonutil::parse(entry->data()); !j.is_discarded())
		readExecutables(j, false);

	// Parse user executables config
	if (auto j = jsonutil::parseFile(app::path("executables.json", app::Dir::User)); !j.is_discarded())
		readExecutables(j, true);
	else
	{
		// No json config found, try pre-3.3.0 executables.cfg
		if (MemChunk mc; mc.importFile(app::path("executables.cfg", app::Dir::User)))
		{
			Parser p;
			p.parseText(mc, "user execuatbles.cfg");
			parse(&p, true);
		}
	}
}

// -----------------------------------------------------------------------------
// Parses an executables configuration from [p]
// -----------------------------------------------------------------------------
void executables::parse(const Parser* p, bool custom)
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
void executables::parseGameExe(const ParseTreeNode* node, bool custom)
{
	// Get GameExe being parsed
	auto exe = gameExe(strutil::lower(node->name()));
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

		// Run Config
		if (strutil::equalCI(prop->type(), "config"))
		{
			// Update if exists
			bool found = false;
			for (auto& config : exe->run_configs)
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
				exe->run_configs.emplace_back(prop->name(), prop->stringValue());
				exe->run_configs_custom.push_back(custom);
			}
		}

		// Map Run Config
		if (strutil::equalCI(prop->type(), "map_config"))
		{
			// Update if exists
			bool found = false;
			for (auto& config : exe->map_configs)
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
				exe->map_configs.emplace_back(prop->name(), prop->stringValue());
				exe->map_configs_custom.push_back(custom);
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
void executables::addGameExe(string_view name)
{
	GameExe game;
	game.name = name;

	game.id = strutil::lower(name);
	std::replace(game.id.begin(), game.id.end(), ' ', '_');

	game_exes.push_back(game);
}

// -----------------------------------------------------------------------------
// Removes the game executable definition at [index]
// -----------------------------------------------------------------------------
bool executables::removeGameExe(unsigned index)
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
void executables::addGameExeRunConfig(
	unsigned    exe_index,
	string_view config_name,
	string_view config_params,
	bool        custom)
{
	// Check index
	if (exe_index >= game_exes.size())
		return;

	game_exes[exe_index].run_configs.emplace_back(config_name, config_params);
	game_exes[exe_index].run_configs_custom.push_back(custom);
}

// -----------------------------------------------------------------------------
// Removes run configuration at [config_index] in game exe definition at
// [exe_index]
// -----------------------------------------------------------------------------
bool executables::removeGameExeRunConfig(unsigned exe_index, unsigned config_index)
{
	// Check indices
	if (exe_index >= game_exes.size())
		return false;
	if (config_index >= game_exes[exe_index].run_configs.size())
		return false;

	// Check config is custom
	if (game_exes[exe_index].run_configs_custom[config_index])
	{
		VECTOR_REMOVE_AT(game_exes[exe_index].run_configs, config_index);
		VECTOR_REMOVE_AT(game_exes[exe_index].run_configs_custom, config_index);

		return true;
	}
	else
		return false;
}

// -----------------------------------------------------------------------------
// Adds a map run configuration for game executable at [exe_index]
// -----------------------------------------------------------------------------
void executables::addGameExeMapConfig(
	unsigned    exe_index,
	string_view config_name,
	string_view config_params,
	bool        custom)
{
	// Check index
	if (exe_index >= game_exes.size())
		return;

	game_exes[exe_index].map_configs.emplace_back(config_name, config_params);
	game_exes[exe_index].map_configs_custom.push_back(custom);
}

// -----------------------------------------------------------------------------
// Removes map run configuration at [config_index] in game exe definition at
// [exe_index]
// -----------------------------------------------------------------------------
bool executables::removeGameExeMapConfig(unsigned exe_index, unsigned config_index)
{
	// Check indices
	if (exe_index >= game_exes.size())
		return false;
	if (config_index >= game_exes[exe_index].map_configs.size())
		return false;

	// Check config is custom
	if (game_exes[exe_index].map_configs_custom[config_index])
	{
		VECTOR_REMOVE_AT(game_exes[exe_index].map_configs, config_index);
		VECTOR_REMOVE_AT(game_exes[exe_index].map_configs_custom, config_index);

		return true;
	}
	else
		return false;
}

// -----------------------------------------------------------------------------
// Returns the number of external executables for [category], or all if
// [category] is not specified
// -----------------------------------------------------------------------------
int executables::nExternalExes(string_view category)
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
executables::ExternalExe executables::externalExe(string_view name, string_view category)
{
	for (auto& exe : external_exes)
		if (category.empty() || exe.category == category)
			if (exe.name == name)
				return exe;

	return {};
}

// -----------------------------------------------------------------------------
// Returns a list of all external executables matching [category].
// If [category] is empty, it is ignored
// -----------------------------------------------------------------------------
vector<executables::ExternalExe> executables::externalExes(string_view category)
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
void executables::parseExternalExe(const ParseTreeNode* node)
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
void executables::addExternalExe(string_view name, string_view path, string_view category)
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
void executables::setExternalExeName(string_view name_old, string_view name_new, string_view category)
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
void executables::setExternalExePath(string_view name, string_view path, string_view category)
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
void executables::removeExternalExe(string_view name, string_view category)
{
	for (unsigned a = 0; a < external_exes.size(); a++)
		if (external_exes[a].name == name && external_exes[a].category == category)
		{
			external_exes.erase(external_exes.begin() + a);
			return;
		}
}
