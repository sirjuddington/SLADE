
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         https://slade.mancubus.net
// Filename:    Game.cpp
// Description: Game namespace, brings together all game-handling stuff like
//              game configurations, port features, etc.
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
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#include "Game.h"
#include "Configuration.h"
#include "Utility/Parser.h"
#include "Archive/ArchiveManager.h"
#include "App.h"


// ----------------------------------------------------------------------------
//
// Variables
//
// ----------------------------------------------------------------------------
namespace Game
{
	Configuration				config_current;
	std::map<string, GameDef>	game_defs;
	GameDef						game_def_unknown;
	std::map<string, PortDef>	port_defs;
	PortDef						port_def_unknown;
}
CVAR(String, game_configuration, "", CVAR_SAVE)
CVAR(String, port_configuration, "", CVAR_SAVE)


// ----------------------------------------------------------------------------
//
// GameDef Struct Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// GameDef::parse
//
// Parses the basic game definition in [mc]
// ----------------------------------------------------------------------------
bool Game::GameDef::parse(MemChunk& mc)
{
	// Parse configuration
	Parser parser;
	parser.parseText(mc, "");

	// Check for game section
	ParseTreeNode* node_game = nullptr;
	for (unsigned a = 0; a < parser.parseTreeRoot()->nChildren(); a++)
	{
		auto child = parser.parseTreeRoot()->getChildPTN(a);
		if (child->getType() == "game")
		{
			node_game = child;
			break;
		}
	}
	if (node_game)
	{
		// Game id
		name = node_game->getName();

		// Game name
		auto node_name = node_game->getChildPTN("name");
		if (node_name)
			title = node_name->getStringValue();

		// Supported map formats
		auto node_maps = node_game->getChildPTN("map_formats");
		if (node_maps)
		{
			for (unsigned a = 0; a < node_maps->nValues(); a++)
			{
				if (S_CMPNOCASE(node_maps->getStringValue(a), "doom"))
					supported_formats[MAP_DOOM] = true;
				else if (S_CMPNOCASE(node_maps->getStringValue(a), "hexen"))
					supported_formats[MAP_HEXEN] = true;
				else if (S_CMPNOCASE(node_maps->getStringValue(a), "doom64"))
					supported_formats[MAP_DOOM64] = true;
				else if (S_CMPNOCASE(node_maps->getStringValue(a), "udmf"))
					supported_formats[MAP_UDMF] = true;
			}
		}
		// Filters
		auto node_filters = node_game->getChildPTN("filters");
		if (node_filters)
		{
			for (unsigned a = 0; a < node_filters->nValues(); a++)
				filters.push_back(node_filters->getStringValue(a).Lower());
		}
	}

	return (node_game != nullptr);
}

// ----------------------------------------------------------------------------
// GameDef::supportsFilter
//
// Checks if this game supports [filter]
// ----------------------------------------------------------------------------
bool GameDef::supportsFilter(const string& filter) const
{
	for (auto& f : filters)
		if (S_CMPNOCASE(f, filter))
			return true;

	return false;
}


// ----------------------------------------------------------------------------
//
// PortDef Struct Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// PortDef::parse
//
// Parses the basic port definition in [mc]
// ----------------------------------------------------------------------------
bool Game::PortDef::parse(MemChunk& mc)
{
	// Parse configuration
	Parser parser;
	parser.parseText(mc, "");

	// Check for port section
	ParseTreeNode* node_port = nullptr;
	for (unsigned a = 0; a < parser.parseTreeRoot()->nChildren(); a++)
	{
		auto child = parser.parseTreeRoot()->getChildPTN(a);
		if (child->getType() == "port")
		{
			node_port = child;
			break;
		}
	}
	if (node_port)
	{
		// Port id
		name = node_port->getName();

		// Port name
		auto node_name = node_port->getChildPTN("name");
		if (node_name)
			title = node_name->getStringValue();

		// Supported games
		auto node_games = node_port->getChildPTN("games");
		if (node_games)
		{
			for (unsigned a = 0; a < node_games->nValues(); a++)
				supported_games.push_back(node_games->getStringValue(a));
		}

		// Supported map formats
		auto node_maps = node_port->getChildPTN("map_formats");
		if (node_maps)
		{
			for (unsigned a = 0; a < node_maps->nValues(); a++)
			{
				if (S_CMPNOCASE(node_maps->getStringValue(a), "doom"))
					supported_formats[MAP_DOOM] = true;
				else if (S_CMPNOCASE(node_maps->getStringValue(a), "hexen"))
					supported_formats[MAP_HEXEN] = true;
				else if (S_CMPNOCASE(node_maps->getStringValue(a), "doom64"))
					supported_formats[MAP_DOOM64] = true;
				else if (S_CMPNOCASE(node_maps->getStringValue(a), "udmf"))
					supported_formats[MAP_UDMF] = true;
			}
		}
	}

	return (node_port != nullptr);
}


// ----------------------------------------------------------------------------
//
// Game Namespace Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// Game::configuration
//
// Returns the currently loaded game configuration
// ----------------------------------------------------------------------------
Configuration& Game::configuration()
{
	return config_current;
}

// ----------------------------------------------------------------------------
// Game::parseTagged
//
// Returns the tagged type of the parsed tree node [tagged]
// ----------------------------------------------------------------------------
TagType Game::parseTagged(ParseTreeNode* tagged)
{
	string str = tagged->getStringValue();
	if (S_CMPNOCASE(str, "no")) return TagType::None;
	else if (S_CMPNOCASE(str, "sector")) return TagType::Sector;
	else if (S_CMPNOCASE(str, "line")) return TagType::Line;
	else if (S_CMPNOCASE(str, "lineid")) return TagType::LineId;
	else if (S_CMPNOCASE(str, "lineid_hi5")) return TagType::LineIdHi5;
	else if (S_CMPNOCASE(str, "thing")) return TagType::Thing;
	else if (S_CMPNOCASE(str, "sector_back")) return TagType::Back;
	else if (S_CMPNOCASE(str, "sector_or_back")) return TagType::SectorOrBack;
	else if (S_CMPNOCASE(str, "sector_and_back")) return TagType::SectorAndBack;
	else if (S_CMPNOCASE(str, "line_negative")) return TagType::LineNegative;
	else if (S_CMPNOCASE(str, "ex_1thing_2sector")) return TagType::Thing1Sector2;
	else if (S_CMPNOCASE(str, "ex_1thing_3sector")) return TagType::Thing1Sector3;
	else if (S_CMPNOCASE(str, "ex_1thing_2thing")) return TagType::Thing1Thing2;
	else if (S_CMPNOCASE(str, "ex_1thing_4thing")) return TagType::Thing1Thing4;
	else if (S_CMPNOCASE(str, "ex_1thing_2thing_3thing")) return TagType::Thing1Thing2Thing3;
	else if (S_CMPNOCASE(str, "ex_1sector_2thing_3thing_5thing")) return TagType::Sector1Thing2Thing3Thing5;
	else if (S_CMPNOCASE(str, "ex_1lineid_2line")) return TagType::LineId1Line2;
	else if (S_CMPNOCASE(str, "ex_4thing")) return TagType::Thing4;
	else if (S_CMPNOCASE(str, "ex_5thing")) return TagType::Thing5;
	else if (S_CMPNOCASE(str, "ex_1line_2sector")) return TagType::Line1Sector2;
	else if (S_CMPNOCASE(str, "ex_1sector_2sector")) return TagType::Sector1Sector2;
	else if (S_CMPNOCASE(str, "ex_1sector_2sector_3sector_4_sector")) return TagType::Sector1Sector2Sector3Sector4;
	else if (S_CMPNOCASE(str, "ex_sector_2is3_line")) return TagType::Sector2Is3Line;
	else if (S_CMPNOCASE(str, "ex_1sector_2thing")) return TagType::Sector1Thing2;
	else
		return (TagType)tagged->getIntValue();
}

// ----------------------------------------------------------------------------
// Game::init
//
// Game related initialisation (read basic definitions, etc.)
// ----------------------------------------------------------------------------
void Game::init()
{
	// Add game configurations from user dir
	wxArrayString allfiles;
	wxDir::GetAllFiles(App::path("games", App::Dir::User), &allfiles);
	for (unsigned a = 0; a < allfiles.size(); a++)
	{
		// Read config info
		MemChunk mc;
		mc.importFile(allfiles[a]);

		// Add to list if valid
		GameDef gdef;
		if (gdef.parse(mc))
		{
			gdef.filename = wxFileName(allfiles[a]).GetName();
			gdef.user = true;
			game_defs[gdef.name] = gdef;
		}
	}

	// Add port configurations from user dir
	allfiles.clear();
	wxDir::GetAllFiles(App::path("ports", App::Dir::User), &allfiles);
	for (unsigned a = 0; a < allfiles.size(); a++)
	{
		// Read config info
		MemChunk mc;
		mc.importFile(allfiles[a]);

		// Add to list if valid
		PortDef pdef;
		if (pdef.parse(mc))
		{
			pdef.filename = wxFileName(allfiles[a]).GetName();
			pdef.user = true;
			port_defs[pdef.name] = pdef;
		}
	}

	// Add game configurations from program resource
	auto dir = theArchiveManager->programResourceArchive()->getDir("config/games");
	if (dir)
	{
		for (auto& entry : dir->getEntries())
		{
			// Read config info
			GameDef conf;
			if (!conf.parse(entry.get()->getMCData()))
				continue;	// Ignore if invalid

			// Add to list if it doesn't already exist
			if (game_defs.find(conf.name) == game_defs.end())
			{
				conf.filename = entry.get()->getName(true);
				conf.user = false;
				game_defs[conf.name] = conf;
			}
		}
	}

	// Add port configurations from program resource
	dir = theArchiveManager->programResourceArchive()->getDir("config/ports");
	if (dir)
	{
		for (auto& entry : dir->getEntries())
		{
			// Read config info
			PortDef conf;
			if (!conf.parse(entry.get()->getMCData()))
				continue;	// Ignore if invalid

			// Add to list if it doesn't already exist
			if (port_defs.find(conf.name) == port_defs.end())
			{
				conf.filename = entry.get()->getName(true);
				conf.user = false;
				port_defs[conf.name] = conf;
			}
		}
	}

	// Load last configuration if any
	if (game_configuration != "")
		config_current.openConfig(game_configuration, port_configuration);
}

// ----------------------------------------------------------------------------
// Game::gameDefs
//
// Returns a vector of all basic game definitions
// ----------------------------------------------------------------------------
const std::map<string, Game::GameDef>& Game::gameDefs()
{
	return game_defs;
}

// ----------------------------------------------------------------------------
// Game::gameDef
//
// Returns the basic game configuration matching [id]
// ----------------------------------------------------------------------------
const Game::GameDef& Game::gameDef(const string& id)
{
	return game_defs.empty() ? game_def_unknown : game_defs[id];
}

// ----------------------------------------------------------------------------
// Game::portDefs
//
// Returns a vector of all basic port definitions
// ----------------------------------------------------------------------------
const std::map<string, Game::PortDef>& Game::portDefs()
{
	return port_defs;
}

// ----------------------------------------------------------------------------
// Game::portDef
//
// Returns the basic port configuration matching [id]
// ----------------------------------------------------------------------------
const Game::PortDef& Game::portDef(const string& id)
{
	return port_defs.empty() ? port_def_unknown : port_defs[id];
}

// ----------------------------------------------------------------------------
// Game::mapFormatSupported
//
// Checks if the combination of [game] and [port] supports the map [format]
// ----------------------------------------------------------------------------
bool Game::mapFormatSupported(int format, const string& game, const string& port)
{
	if (format < 0 || format >= MAP_UNKNOWN)
		return false;

	if (!port.empty())
		return port_defs[port].supported_formats[format];

	if (!game.empty())
		return game_defs[game].supported_formats[format];

	return false;
}
