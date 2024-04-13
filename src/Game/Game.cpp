
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "Game.h"
#include "ActionSpecial.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveFormat.h"
#include "Archive/ArchiveManager.h"
#include "Configuration.h"
#include "SpecialPreset.h"
#include "TextEditor/TextLanguage.h"
#include "ThingType.h"
#include "Utility/Parser.h"
#include "Utility/StringUtils.h"
#include "ZScript.h"
#include <thread>

using namespace slade;
using namespace game;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace slade::game
{
std::map<string, GameDef> game_defs;
GameDef                   game_def_unknown;
std::map<string, PortDef> port_defs;
PortDef                   port_def_unknown;
zscript::Definitions      zscript_base;
zscript::Definitions      zscript_custom;
unique_ptr<std::thread>   zscript_parse_thread;
} // namespace slade::game
CVAR(String, game_configuration, "", CVar::Flag::Save)
CVAR(String, port_configuration, "", CVar::Flag::Save)
CVAR(String, zdoom_pk3_path, "", CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// GameDef Struct Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Parses the basic game definition in [mc]
// -----------------------------------------------------------------------------
bool GameDef::parse(const MemChunk& mc)
{
	// Parse configuration
	Parser parser;
	parser.parseText(mc, "");

	// Check for game section
	ParseTreeNode* node_game = nullptr;
	for (unsigned a = 0; a < parser.parseTreeRoot()->nChildren(); a++)
	{
		auto child = parser.parseTreeRoot()->childPTN(a);
		if (child->type() == "game")
		{
			node_game = child;
			break;
		}
	}
	if (node_game)
	{
		// Game id
		name = node_game->name();

		// Game name
		auto node_name = node_game->childPTN("name");
		if (node_name)
			title = node_name->stringValue();

		// Supported map formats
		auto node_maps = node_game->childPTN("map_formats");
		if (node_maps)
		{
			for (const auto& str_val : node_maps->stringValues())
			{
				if (strutil::equalCI(str_val, "doom"))
					supported_formats[MapFormat::Doom] = true;
				else if (strutil::equalCI(str_val, "hexen"))
					supported_formats[MapFormat::Hexen] = true;
				else if (strutil::equalCI(str_val, "doom64"))
					supported_formats[MapFormat::Doom64] = true;
				else if (strutil::equalCI(str_val, "doom32x"))
					supported_formats[MapFormat::Doom32X] = true;
				else if (strutil::equalCI(str_val, "udmf"))
					supported_formats[MapFormat::UDMF] = true;
			}
		}
		// Filters
		auto node_filters = node_game->childPTN("filters");
		if (node_filters)
		{
			for (unsigned a = 0; a < node_filters->nValues(); a++)
				filters.push_back(strutil::lower(node_filters->stringValue(a)));
		}
	}

	return (node_game != nullptr);
}

// -----------------------------------------------------------------------------
// Checks if this game supports [filter]
// -----------------------------------------------------------------------------
bool GameDef::supportsFilter(string_view filter) const
{
	for (auto& f : filters)
		if (strutil::equalCI(f, filter))
			return true;

	return false;
}


// -----------------------------------------------------------------------------
//
// PortDef Struct Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Parses the basic port definition in [mc]
// -----------------------------------------------------------------------------
bool PortDef::parse(const MemChunk& mc)
{
	// Parse configuration
	Parser parser;
	parser.parseText(mc, "");

	// Check for port section
	ParseTreeNode* node_port = nullptr;
	for (unsigned a = 0; a < parser.parseTreeRoot()->nChildren(); a++)
	{
		auto child = parser.parseTreeRoot()->childPTN(a);
		if (child->type() == "port")
		{
			node_port = child;
			break;
		}
	}
	if (node_port)
	{
		// Port id
		name = node_port->name();

		// Port name
		auto node_name = node_port->childPTN("name");
		if (node_name)
			title = node_name->stringValue();

		// Supported games
		auto node_games = node_port->childPTN("games");
		if (node_games)
		{
			for (unsigned a = 0; a < node_games->nValues(); a++)
				supported_games.emplace_back(node_games->stringValue(a));
		}

		// Supported map formats
		auto node_maps = node_port->childPTN("map_formats");
		if (node_maps)
		{
			for (const auto& str_val : node_maps->stringValues())
			{
				if (strutil::equalCI(str_val, "doom"))
					supported_formats[MapFormat::Doom] = true;
				else if (strutil::equalCI(str_val, "hexen"))
					supported_formats[MapFormat::Hexen] = true;
				else if (strutil::equalCI(str_val, "doom64"))
					supported_formats[MapFormat::Doom64] = true;
				else if (strutil::equalCI(str_val, "doom32x"))
					supported_formats[MapFormat::Doom32X] = true;
				else if (strutil::equalCI(str_val, "udmf"))
					supported_formats[MapFormat::UDMF] = true;
			}
		}
	}

	return (node_port != nullptr);
}


// -----------------------------------------------------------------------------
//
// Game Namespace Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Clears and re-parses custom definitions in all open archives
// (DECORATE, *MAPINFO, ZScript etc.)
// -----------------------------------------------------------------------------
void game::updateCustomDefinitions()
{
	auto& config_current = configuration();

	// Clear out all existing custom definitions
	config_current.clearDecorateDefs();
	config_current.clearMapInfo();
	zscript_custom.clear();

	// Parse custom definitions in base resource
	auto base_resource = app::archiveManager().baseResourceArchive();
	if (base_resource)
	{
		zscript_custom.parseZScript(base_resource);
		config_current.parseDecorateDefs(base_resource);
		config_current.parseMapInfo(*base_resource);
	}

	// Parse custom definitions in all resource archives
	auto resource_archives = app::archiveManager().allArchives(true);

	// ZScript first
	for (const auto& archive : resource_archives)
		zscript_custom.parseZScript(archive.get());

	// Other definitions
	for (const auto& archive : resource_archives)
	{
		config_current.parseDecorateDefs(archive.get());
		config_current.parseMapInfo(*archive);
	}

	// Process custom definitions
	config_current.importZScriptDefs(zscript_custom);
	config_current.linkDoomEdNums();

	auto lang = TextLanguage::fromId("zscript");
	if (lang)
	{
		lang->clearCustomDefs();
		lang->loadZScript(zscript_custom, true);
	}
}

// -----------------------------------------------------------------------------
// Returns the tagged type of the parsed tree node [tagged]
// -----------------------------------------------------------------------------
TagType game::parseTagged(const ParseTreeNode* tagged)
{
	static std::map<string, TagType> tag_type_map{
		{ "no", TagType::None },
		{ "sector", TagType::Sector },
		{ "line", TagType::Line },
		{ "lineid", TagType::LineId },
		{ "lineid_hi5", TagType::LineIdHi5 },
		{ "thing", TagType::Thing },
		{ "sector_back", TagType::Back },
		{ "sector_or_back", TagType::SectorOrBack },
		{ "sector_and_back", TagType::SectorAndBack },
		{ "line_negative", TagType::LineNegative },
		{ "ex_1thing_2sector", TagType::Thing1Sector2 },
		{ "ex_1thing_3sector", TagType::Thing1Sector3 },
		{ "ex_1thing_2thing", TagType::Thing1Thing2 },
		{ "ex_1thing_4thing", TagType::Thing1Thing4 },
		{ "ex_1thing_2thing_3thing", TagType::Thing1Thing2Thing3 },
		{ "ex_1sector_2thing_3thing_5thing", TagType::Sector1Thing2Thing3Thing5 },
		{ "ex_1lineid_2line", TagType::LineId1Line2 },
		{ "ex_4thing", TagType::Thing4 },
		{ "ex_5thing", TagType::Thing5 },
		{ "ex_1line_2sector", TagType::Line1Sector2 },
		{ "ex_1sector_2sector", TagType::Sector1Sector2 },
		{ "ex_1sector_2sector_3sector_4_sector", TagType::Sector1Sector2Sector3Sector4 },
		{ "ex_sector_2is3_line", TagType::Sector2Is3Line },
		{ "ex_1sector_2thing", TagType::Sector1Thing2 },
		{ "patrol", TagType::Patrol },
		{ "interpolation", TagType::Interpolation }
	};

	return tag_type_map[strutil::lower(tagged->stringValue())];
}

// -----------------------------------------------------------------------------
// Game related initialisation (read basic definitions, etc.)
// -----------------------------------------------------------------------------
void game::init()
{
	// Init static ThingTypes
	ThingType::initGlobal();

	// Init static ActionSpecials
	ActionSpecial::initGlobal();

	// Add game configurations from user dir
	wxArrayString allfiles;
	wxDir::GetAllFiles(app::path("games", app::Dir::User), &allfiles);
	for (const auto& filename : allfiles)
	{
		// Read config info
		MemChunk mc;
		mc.importFile(filename.ToStdString());

		// Add to list if valid
		GameDef gdef;
		if (gdef.parse(mc))
		{
			gdef.filename        = wxFileName(filename).GetName();
			gdef.user            = true;
			game_defs[gdef.name] = gdef;
		}
	}

	// Add port configurations from user dir
	allfiles.clear();
	wxDir::GetAllFiles(app::path("ports", app::Dir::User), &allfiles);
	for (const auto& filename : allfiles)
	{
		// Read config info
		MemChunk mc;
		mc.importFile(filename.ToStdString());

		// Add to list if valid
		PortDef pdef;
		if (pdef.parse(mc))
		{
			pdef.filename        = wxFileName(filename).GetName();
			pdef.user            = true;
			port_defs[pdef.name] = pdef;
		}
	}

	// Add game configurations from program resource
	auto dir = app::archiveManager().programResourceArchive()->dirAtPath("config/games");
	if (dir)
	{
		for (auto& entry : dir->entries())
		{
			// Read config info
			GameDef conf;
			if (!conf.parse(entry->data()))
				continue; // Ignore if invalid

			// Add to list if it doesn't already exist
			if (game_defs.find(conf.name) == game_defs.end())
			{
				conf.filename        = entry->nameNoExt();
				conf.user            = false;
				game_defs[conf.name] = conf;
			}
		}
	}

	// Add port configurations from program resource
	dir = app::archiveManager().programResourceArchive()->dirAtPath("config/ports");
	if (dir)
	{
		for (auto& entry : dir->entries())
		{
			// Read config info
			PortDef conf;
			if (!conf.parse(entry->data()))
				continue; // Ignore if invalid

			// Add to list if it doesn't already exist
			if (port_defs.find(conf.name) == port_defs.end())
			{
				conf.filename        = entry->nameNoExt();
				conf.user            = false;
				port_defs[conf.name] = conf;
			}
		}
	}

	// Load last configuration if any
	if (!game_configuration.value.empty())
		configuration().openConfig(game_configuration, port_configuration);

	// Load custom special presets
	if (!loadCustomSpecialPresets())
		log::warning("An error occurred loading user special_presets.cfg");

	// Load zdoom.pk3 stuff
	if (wxFileExists(zdoom_pk3_path))
	{
		zscript_parse_thread = std::make_unique<std::thread>(
			[=]()
			{
				Archive zdoom_pk3(ArchiveFormat::Zip);
				if (!zdoom_pk3.open(zdoom_pk3_path))
					return;

				// ZScript
				auto zscript_entry = zdoom_pk3.entryAtPath("zscript.txt");

				if (!zscript_entry)
				{
					// Bail out if no entry is found.
					log::warning(1, "Could not find \'zscript.txt\' in " + zdoom_pk3_path);
				}
				else
				{
					zscript_base.parseZScript(zscript_entry);

					auto lang = TextLanguage::fromId("zscript");
					if (lang)
						lang->loadZScript(zscript_base);

					// MapInfo
					configuration().parseMapInfo(zdoom_pk3);
				}
			});
		zscript_parse_thread->detach();
	}

	// Update custom definitions when an archive is opened or closed
	app::archiveManager().signals().archive_added.connect([](unsigned) { updateCustomDefinitions(); });
	app::archiveManager().signals().archive_closed.connect([](unsigned) { updateCustomDefinitions(); });
}

// -----------------------------------------------------------------------------
// Returns a vector of all basic game definitions
// -----------------------------------------------------------------------------
const std::map<string, GameDef>& game::gameDefs()
{
	return game_defs;
}

// -----------------------------------------------------------------------------
// Returns the basic game configuration matching [id]
// -----------------------------------------------------------------------------
const GameDef& game::gameDef(const string& id)
{
	return game_defs.empty() ? game_def_unknown : game_defs[id];
}

// -----------------------------------------------------------------------------
// Returns a vector of all basic port definitions
// -----------------------------------------------------------------------------
const std::map<string, PortDef>& game::portDefs()
{
	return port_defs;
}

// -----------------------------------------------------------------------------
// Returns the basic port configuration matching [id]
// -----------------------------------------------------------------------------
const PortDef& game::portDef(const string& id)
{
	return port_defs.empty() ? port_def_unknown : port_defs[id];
}

// -----------------------------------------------------------------------------
// Checks if the combination of [game] and [port] supports the map [format]
// -----------------------------------------------------------------------------
bool game::mapFormatSupported(MapFormat format, const string& game, const string& port)
{
	if (format == MapFormat::Unknown)
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
							tl.openString(lnstr, 0, 0, wxString::Format("Line %d", line));
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
									xline.args[a] = wxString::Format("%d", val);

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
			if (i > 0 && configuration().openConfig(games[i], i < 4 ? "zdoom" : "eternity", MapFormat::Doom))
			{
				for (size_t z = 0; z < lines.size(); ++z)
				{
					lines[z].description = configuration().actionSpecialName(lines[z].type);
					if (lines[z].game.empty())
						lines[z].group = configuration().actionSpecial(lines[z].type).group();
					else
						lines[z].group = wxString::Format("%s/%s", lines[z].game, configuration().actionSpecial(lines[z].type).group());
				}
			}
			// Convert special names to numbers
			configuration().openConfig("doom2", "zdoom", MapFormat::Hexen);
			for (auto& l : lines)
			{
				for (auto& i : configuration().allActionSpecials())
					if (i.second.defined() && i.second.name() == l.special)
						l.special = wxString::Format("%d", i.first);
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
			string output = wxString::Format("preset \"%d: %s\"\n{\n", lines[l].type, lines[l].description);
			output += wxString::Format("\tgroup = \"%s\";\n", lines[l].group);
			output += wxString::Format("\tspecial = %s;\n", lines[l].special);
			for (size_t i = 0; i < 5; ++i)
			{
				if (lines[l].args[i].size() && lines[l].args[i] != "tag" && lines[l].args[i] != "0")
					output += wxString::Format("\targ%d = %s;\n", i+1, lines[l].args[i]);
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
			output += wxString::Format("\tflags = %s;\n", flags);
			output += "}\n\n";
			log::debug(wxString::Format("%s", output));
			file += output;
			//		DPrintf("%d = %s : %d, %s (%s, %s, %s, %s, %s)",
			//			lines[l].type, lines[l].description, lines[l].flags, lines[l].special,
			//			lines[l].args[0], lines[l].args[1], lines[l].args[2], lines[l].args[3], lines[l].args[4]);
		}
		wxFile tempfile(app::path("prefabs.txt", app::Dir::Executable), wxFile::write);
		tempfile.Write(file);
		tempfile.Close();
#endif
		return false;
	}
}

#include "General/Console.h"

CONSOLE_COMMAND(parsexlat, 0, false)
{
	parseXlat(app::archiveManager().getArchive(0));
}

#endif
