
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
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
#include "General/Console/Console.h"

using namespace Game;


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
		if (child->type() == "game")
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
			title = node_name->stringValue();

		// Supported map formats
		auto node_maps = node_game->getChildPTN("map_formats");
		if (node_maps)
		{
			for (unsigned a = 0; a < node_maps->nValues(); a++)
			{
				if (S_CMPNOCASE(node_maps->stringValue(a), "doom"))
					supported_formats[MAP_DOOM] = true;
				else if (S_CMPNOCASE(node_maps->stringValue(a), "hexen"))
					supported_formats[MAP_HEXEN] = true;
				else if (S_CMPNOCASE(node_maps->stringValue(a), "doom64"))
					supported_formats[MAP_DOOM64] = true;
				else if (S_CMPNOCASE(node_maps->stringValue(a), "udmf"))
					supported_formats[MAP_UDMF] = true;
			}
		}
		// Filters
		auto node_filters = node_game->getChildPTN("filters");
		if (node_filters)
		{
			for (unsigned a = 0; a < node_filters->nValues(); a++)
				filters.push_back(node_filters->stringValue(a).Lower());
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
		if (child->type() == "port")
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
			title = node_name->stringValue();

		// Supported games
		auto node_games = node_port->getChildPTN("games");
		if (node_games)
		{
			for (unsigned a = 0; a < node_games->nValues(); a++)
				supported_games.push_back(node_games->stringValue(a));
		}

		// Supported map formats
		auto node_maps = node_port->getChildPTN("map_formats");
		if (node_maps)
		{
			for (unsigned a = 0; a < node_maps->nValues(); a++)
			{
				if (S_CMPNOCASE(node_maps->stringValue(a), "doom"))
					supported_formats[MAP_DOOM] = true;
				else if (S_CMPNOCASE(node_maps->stringValue(a), "hexen"))
					supported_formats[MAP_HEXEN] = true;
				else if (S_CMPNOCASE(node_maps->stringValue(a), "doom64"))
					supported_formats[MAP_DOOM64] = true;
				else if (S_CMPNOCASE(node_maps->stringValue(a), "udmf"))
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
	static std::map<string, TagType> tag_type_map
	{
		{ "no",										TagType::None },
		{ "sector",									TagType::Sector },
		{ "line",									TagType::Line },
		{ "lineid",									TagType::LineId },
		{ "lineid_hi5",								TagType::LineIdHi5 },
		{ "thing",									TagType::Thing },
		{ "sector_back",							TagType::Back },
		{ "sector_or_back",							TagType::SectorOrBack },
		{ "sector_and_back",						TagType::SectorAndBack },
		{ "line_negative",							TagType::LineNegative },
		{ "ex_1thing_2sector",						TagType::Thing1Sector2 },
		{ "ex_1thing_3sector",						TagType::Thing1Sector3 },
		{ "ex_1thing_2thing",						TagType::Thing1Thing2 },
		{ "ex_1thing_4thing",						TagType::Thing1Thing4 },
		{ "ex_1thing_2thing_3thing",				TagType::Thing1Thing2Thing3 },
		{ "ex_1sector_2thing_3thing_5thing",		TagType::Sector1Thing2Thing3Thing5 },
		{ "ex_1lineid_2line",						TagType::LineId1Line2 },
		{ "ex_4thing",								TagType::Thing4 },
		{ "ex_5thing",								TagType::Thing5 },
		{ "ex_1line_2sector",						TagType::Line1Sector2 },
		{ "ex_1sector_2sector",						TagType::Sector1Sector2 },
		{ "ex_1sector_2sector_3sector_4_sector",	TagType::Sector1Sector2Sector3Sector4 },
		{ "ex_sector_2is3_line",					TagType::Sector2Is3Line },
		{ "ex_1sector_2thing",						TagType::Sector1Thing2 },
		{ "patrol",									TagType::Patrol },
		{ "interpolation",							TagType::Interpolation }
	};

	return tag_type_map[tagged->stringValue().MakeLower()];
}

// ----------------------------------------------------------------------------
// Game::init
//
// Game related initialisation (read basic definitions, etc.)
// ----------------------------------------------------------------------------
void Game::init()
{
	// Init static ThingTypes
	ThingType::initGlobal();

	// Init static ActionSpecials
	ActionSpecial::initGlobal();

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

	// Load custom special presets
	if (!loadCustomSpecialPresets())
		Log::warning("An error occurred loading user special_presets.cfg");
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





#if 0

// XLAT Parser to generate Special Presets from ZDoom XLAT definitions

namespace
{
	typedef std::map<string, int> enummap;
	struct xlatline_t
	{
		int type;
		int flags;
		string special;
		string args[5];
		string description;
		string group;
		string game;
		bool operator<(const xlatline_t &other)
		{
			return type < other.type;
			//return special.Cmp(other.special) < 0 ||
			//	(special.Cmp(other.special) == 0 && other.type < type);
		}
	};
	bool parseXlat(Archive* archive)
	{
		if (!archive)
			return false;

		char* entries[] = { "xlat/defines.i", "xlat/base.txt", "xlat/heretic.txt", "xlat/strife.txt", "xlat/eternity.txt" };
		char* games[] = { "", "doom2", "heretic", "strife", "doom2" };
		char* groups[] = { "", "", "Heretic", "Strife", "Eternity" };

		enummap enums;
		std::vector<xlatline_t> lines;
		long line = 0;

		ArchiveEntry* entry;
		for (uint8_t i = 0; i < 5; ++i)
		{
			entry = archive->entryAtPath(entries[i]);
			if (entry)
			{
				Tokenizer tz;
				Tokenizer tl;
				tz.setSpecialCharacters("(),[=|");
				if (tz.openMem(&(entry->getMCData()), entries[i]))
				{
					string token = tz.getToken();
					while (!token.empty())
					{
						if (S_CMPNOCASE(token, "enum"))
						{
							// enum
							int val = 0;
							string def;
							tz.checkToken("{");
							while (!S_CMP(tz.peekToken(), "}"))
							{
								def = tz.getToken();
								if (S_CMP(tz.peekToken(), "="))
								{
									tz.skipToken();
									val = tz.getInteger();
								}
								enums[def] = val;
								++val;
								if (S_CMP(tz.peekToken(), ","))
									tz.skipToken();
							}
						}

						else if (S_CMPNOCASE(token, "define"))
						{
							string def = tz.getToken();
							tz.skipToken();
							int val = tz.getInteger();
							tz.skipToken();
							enums[def] = val;
						}

						else if (token.ToLong(&line, 0) && S_CMP(tz.peekToken(), "="))
						{
							tz.setSpecialCharacters("");
							string lnstr = tz.getLine();
							tz.setSpecialCharacters("(),[=|");
							tl.openString(lnstr, 0, 0, S_FMT("Line %d", line));
							tl.setSpecialCharacters("(),[=|");
#if 1
							// Create line
							xlatline_t xline;
							xline.type = (int)line;
							xline.game = groups[i];

							// skip =
							tl.skipToken();

							// Parse flags
							tl.enableDebug(true);
							token = tl.getToken();
							int flags = 0;
							long tmp;
							while (!S_CMP(token, ","))
							{
								if (enums.find(token) != enums.end())
									flags |= enums[token];
								else if (token.ToLong(&tmp, 0))
									flags |= tmp;
								if (S_CMP(tl.peekToken(), "|"))
									tl.skipToken();
								token = tl.getToken();
							}
							tl.enableDebug(false);
							xline.flags = flags;

							// Parse special
							xline.special = tl.getToken();

							// Parse arg
							tl.skipToken(); // Skip (
							for (int a = 0; a < 5 && !token.empty(); ++a)
							{
								int val = -666;
								xline.args[a] = "";
								token = tl.getToken();
								int loop = 0;

								while (!S_CMP(token, ",") && !token.empty())
								{
									if (S_CMP(token, ")"))
										break;
									if (enums.find(token) != enums.end())
									{
										if (val == -666) val = 0;
										val |= enums[token];
									}
									else if (token.ToLong(&tmp, 0))
									{
										if (val == -666) val = 0;
										val |= tmp;
									}
									else xline.args[a].append(token);

									if (S_CMP(tl.peekToken(), "|"))
										tl.skipToken();
									token = tl.getToken();
								}

								if (val != -666 && xline.args[a].IsEmpty())
									xline.args[a] = S_FMT("%d", val);

								if (S_CMP(tl.peekToken(), ")"))
									break;
							}
							if (!S_CMP(token, ")")) tl.skipToken(); // skip )

							lines.push_back(xline);
#endif
						}

						else if (S_CMPNOCASE(token, "include"))
							tz.skipLineComment();
						else if (S_CMPNOCASE(token, "lineflag"))
							tz.skipLineComment();
						else if (S_CMPNOCASE(token, "sector"))
							tz.skipLineComment();

						// Skip generalized line definitions...
						else if (S_CMPNOCASE(token, "["))
						{
							break;
						}

						token = tz.getToken();
					}
				}
			}
			// Read configuration
			if (i > 0 && configuration().openConfig(games[i], i < 4 ? "zdoom" : "eternity", MAP_DOOM))
			{
				for (size_t z = 0; z < lines.size(); ++z)
				{
					lines[z].description = configuration().actionSpecialName(lines[z].type);
					if (lines[z].game.empty())
						lines[z].group = configuration().actionSpecial(lines[z].type).group();
					else
						lines[z].group = S_FMT("%s/%s", lines[z].game, configuration().actionSpecial(lines[z].type).group());
				}
			}
			// Convert special names to numbers
			configuration().openConfig("doom2", "zdoom", MAP_HEXEN);
			for (auto& l : lines)
			{
				for (auto& i : configuration().allActionSpecials())
					if (i.second.defined() && i.second.name() == l.special)
						l.special = S_FMT("%d", i.first);
			}
		}
#if 0
		for (enummap::iterator it = enums.begin(); it != enums.end(); ++it)
		{
			DPrintf("%s: %d", it->first, it->second);
		}
#endif
#if 1
		std::sort(lines.begin(), lines.end());

		// Remove duplicates
		for (size_t l = 1; l < lines.size(); l++)
		{
			// Check special
			if (lines[l].special != lines[l - 1].special)
				continue;

			// Check flags
			if (lines[l].flags != lines[l - 1].flags)
				continue;

			// Check args
			bool diff = false;
			for (size_t a = 0; a < 5; a++)
			{
				if (lines[l].args[a] != lines[l-1].args[a])
				{
					diff = true;
					break;
				}
			}
			if (diff)
				continue;

			// Duplicate, remove
			lines.erase(lines.begin() + l);
			l--;
		}

		string file;
		for (size_t l = 0; l < lines.size(); ++l)
		{
			string output = S_FMT("preset \"%d: %s\"\n{\n", lines[l].type, lines[l].description);
			output += S_FMT("\tgroup = \"%s\";\n", lines[l].group);
			output += S_FMT("\tspecial = %s;\n", lines[l].special);
			for (size_t i = 0; i < 5; ++i)
			{
				if (lines[l].args[i].size() && lines[l].args[i] != "tag" && lines[l].args[i] != "0")
					output += S_FMT("\targ%d = %s;\n", i+1, lines[l].args[i]);
			}
			int spac = 1 << ((lines[l].flags & 15) >> 1);
			string flags = "";
			if (spac == 1) flags = "playercross, ";
			else if (spac == 2) flags = "playeruse, ";
			else if (spac == 4) flags = "monstercross, ";
			else flags = "impact, missilecross, ";
			if (lines[l].flags & 1)  flags += "repeatspecial, ";
			if (lines[l].flags & 16) flags += "monsteractivate, ";
			flags.RemoveLast(2);
			output += S_FMT("\tflags = %s;\n", flags);
			output += "}\n\n";
			Log::debug(S_FMT("%s", output));
			file += output;
			//		DPrintf("%d = %s : %d, %s (%s, %s, %s, %s, %s)",
			//			lines[l].type, lines[l].description, lines[l].flags, lines[l].special,
			//			lines[l].args[0], lines[l].args[1], lines[l].args[2], lines[l].args[3], lines[l].args[4]);
		}
		wxFile tempfile(App::path("prefabs.txt", App::Dir::Executable), wxFile::write);
		tempfile.Write(file);
		tempfile.Close();
#endif
		return false;
	}
}

CONSOLE_COMMAND(parsexlat, 0, false)
{
	parseXlat(theArchiveManager->getArchive(0));
}

#endif
