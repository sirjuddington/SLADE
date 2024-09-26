
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Decorate.cpp
// Description: Functions for ZDoom DECORATE parsing
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
#include "Decorate.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/EntryType/EntryType.h"
#include "Configuration.h"
#include "Game.h"
#include "ThingType.h"
#include "Utility/PropertyUtils.h"
#include "Utility/StringUtils.h"
#include "Utility/Tokenizer.h"

using namespace slade;
using namespace game;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
EntryType* etype_decorate = nullptr;
}


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Parses a DECORATE 'States' block
// -----------------------------------------------------------------------------
void parseStates(Tokenizer& tz, PropertyList& props)
{
	vector<string>           states;
	string                   state_first;
	std::map<string, string> state_sprites;

	while (!tz.atEnd())
	{
		// Check for end of states block
		if (tz.check("}"))
			break;

		// Check for state label
		if (tz.checkNext(":"))
		{
			// Add to list of current states
			states.emplace_back(strutil::lower(tz.current().text));
			if (state_first.empty())
				state_first = strutil::lower(tz.current().text);

			tz.adv();
		}

		// First token after a state label, should be a base sprite
		else if (!states.empty())
		{
			// Ignore deleted state(s)
			if (tz.checkNC("stop"))
			{
				states.clear();
				tz.adv();
				continue;
			}

			// Set sprite for current states (if it is defined)
			if (!(strutil::contains(tz.current().text, '#') || strutil::contains(tz.current().text, '-')))
				for (auto& state : states)
					state_sprites[state] = tz.current().text + tz.peek().text[0];

			states.clear();
			tz.adv();
		}

		// Skip anonymous functions
		else if (tz.check("{"))
		{
			tz.adv();
			tz.skipSection("{", "}");
			continue;
		}

		tz.adv();
	}

	// Get sprite from highest priority state
	if (!state_sprites["idle"].empty())
		props["sprite"] = state_sprites["idle"] + "?";
	if (!state_sprites["see"].empty())
		props["sprite"] = state_sprites["see"] + "?";
	if (!state_sprites["inactive"].empty())
		props["sprite"] = state_sprites["inactive"] + "?";
	if (!state_sprites["spawn"].empty())
		props["sprite"] = state_sprites["spawn"] + "?";
	if (!state_sprites[state_first].empty())
		props["sprite"] = state_sprites[state_first] + "?";

	log::debug(2, "Parsed states, got sprite {}", property::asString(props["sprite"]));




	// int lastpriority = 0;
	// int priority = 0;
	// int statecounter = 0;
	// string laststate;
	// string spritestate;

	// string token = tz.next().text;
	// while (token != "}")
	//{
	//	// Idle, See, Inactive, Spawn, and finally first defined
	//	if (priority < StateSprites::Idle)
	//	{
	//		string myspritestate = token;
	//		token = tz.next().text;
	//		while (token.Cmp(":") && token.Cmp("}"))
	//		{
	//			myspritestate = token;
	//			token = tz.next().text;
	//		}
	//		if (S_CMPNOCASE(token, "}"))
	//			break;
	//		string sb = tz.next().text; // Sprite base

	//		// Handle removed states
	//		if (S_CMPNOCASE(sb, "Stop"))
	//			continue;
	//		// Handle direct gotos, like ZDoom's dead monsters
	//		if (S_CMPNOCASE(sb, "Goto"))
	//		{
	//			tz.adv();
	//			// Skip scope and state
	//			if (tz.peek() == ":")
	//			{
	//				tz.adv();	// first :
	//				tz.adv(); // second :
	//				tz.adv(); // state name
	//			}
	//			continue;
	//		}
	//		string sf = tz.next().text; // Sprite frame(s)
	//		int mypriority = 0;
	//		// If the same state is given several names,
	//		// don't read the next name as a sprite name!
	//		// If "::" is encountered, it's a scope operator.
	//		if ((!sf.Cmp(":")) && tz.peek().text.Cmp(":"))
	//		{
	//			if (S_CMPNOCASE(myspritestate, "spawn"))
	//				mypriority = StateSprites::Spawn;
	//			else if (S_CMPNOCASE(myspritestate, "inactive"))
	//				mypriority = StateSprites::Inactive;
	//			else if (S_CMPNOCASE(myspritestate, "see"))
	//				mypriority = StateSprites::See;
	//			else if (S_CMPNOCASE(myspritestate, "idle"))
	//				mypriority = StateSprites::Idle;
	//			if (mypriority > lastpriority)
	//			{
	//				laststate = myspritestate;
	//				lastpriority = mypriority;
	//			}
	//			continue;
	//		}
	//		else
	//		{
	//			spritestate = myspritestate;
	//			if (statecounter++ == 0)
	//				mypriority = StateSprites::FirstDefined;
	//			if (S_CMPNOCASE(spritestate, "spawn"))
	//				mypriority = StateSprites::Spawn;
	//			else if (S_CMPNOCASE(spritestate, "inactive"))
	//				mypriority = StateSprites::Inactive;
	//			else if (S_CMPNOCASE(spritestate, "see"))
	//				mypriority = StateSprites::See;
	//			else if (S_CMPNOCASE(spritestate, "idle"))
	//				mypriority = StateSprites::Idle;
	//			if (lastpriority > mypriority)
	//			{
	//				spritestate = laststate;
	//				mypriority = lastpriority;
	//			}
	//		}
	//		if (sb.length() == 4)
	//		{
	//			string sprite = sb + sf.Left(1) + "?";
	//			if (mypriority > priority)
	//			{
	//				priority = mypriority;
	//				props["sprite"] = sprite;
	//				//log::info(3, "Actor %s found sprite %s from state %s", name, sprite, spritestate);
	//				lastpriority = -1;
	//			}
	//		}
	//	}
	//	else
	//	{
	//		tz.skipSection("{", "}");
	//		break;
	//	}
	//	tz.adv();
	//}
}

// -----------------------------------------------------------------------------
// Parses a DECORATE 'actor' definition
// -----------------------------------------------------------------------------
void parseDecorateActor(Tokenizer& tz, std::map<int, ThingType>& types, vector<ThingType>& parsed)
{
	// Get actor name
	auto   name       = tz.next().text;
	auto   actor_name = name;
	string parent;

	// Check for inheritance
	// string next = tz.peekToken();
	if (tz.advIfNext(":"))
		parent = tz.next().text;

	// Check for replaces
	if (tz.checkNextNC("replaces"))
		tz.adv(2); // Skip 'replaces' and actor

	// Skip "native" keyword if present
	if (tz.checkNextNC("native"))
		tz.adv();

	// Check for no editor number (ie can't be placed in the map)
	int ednum;
	if (!tz.peek().isInteger())
		ednum = -1;
	else
		tz.next().toInt(ednum);

	PropertyList                      found_props;
	bool                              available       = false;
	bool                              filters_present = false;
	string                            group;
	vector<std::pair<string, string>> editor_properties;

	// Skip "native" keyword if present
	tz.advIfNextNC("native");

	// Check for actor definition open
	if (tz.advIfNext("{", 2))
	{
		while (!tz.check("}") && !tz.atEnd())
		{
			auto token = tz.peekToken();
			if (strutil::startsWith(token, "//$"))
			{
				// Doom Builder magic editor comment
				editor_properties.emplace_back(Tokenizer::parseEditorComment(token));
				tz.advToNextLine();
				continue;
			}

			// Check for subsection
			if (tz.advIf("{"))
			{
				tz.skipSection("{", "}");
				continue;
			}

			// Game filter
			else if (tz.checkNC("game"))
			{
				filters_present = true;
				if (gameDef(configuration().currentGame()).supportsFilter(tz.next().text))
					available = true;
			}

			// Tag
			else if (tz.checkNC("tag"))
				name = tz.next().text;

			// Radius
			else if (tz.checkNC("radius"))
				found_props["radius"] = tz.next().asInt();

			// Height
			else if (tz.checkNC("height"))
				found_props["height"] = tz.next().asInt();

			// Scale
			else if (tz.checkNC("scale"))
			{
				auto val              = tz.next().asFloat();
				found_props["scalex"] = val;
				found_props["scaley"] = val;
			}
			else if (tz.checkNC("xscale"))
				found_props["scalex"] = tz.next().asFloat();
			else if (tz.checkNC("yscale"))
				found_props["scaley"] = tz.next().asFloat();

			// Monster
			else if (tz.checkNC("monster"))
			{
				found_props["solid"]      = true;  // Solid
				found_props["decoration"] = false; // Not a decoration
			}

			// Hanging
			else if (tz.checkNC("+spawnceiling"))
				found_props["hanging"] = true;

			// Fullbright
			else if (tz.checkNC("+bright"))
				found_props["bright"] = true;

			// Translation
			else if (tz.checkNC("translation"))
			{
				string translation = "\"";
				translation += tz.next().text;
				while (tz.checkNext(","))
				{
					translation += tz.next().text; // ,
					translation += tz.next().text; // next range
				}
				translation += "\"";
				found_props["translation"] = translation;
			}

			// Solid
			else if (tz.checkNC("+solid"))
				found_props["solid"] = true;

			// States
			if (tz.checkNC("states"))
			{
				tz.adv(2); // Skip past {
				parseStates(tz, found_props);
			}

			tz.adv();
		}

		for (auto& i : editor_properties)
		{
			// Title
			if (i.first == "title")
				name = i.second;

			// Category
			else if (i.first == "group" || i.first == "category")
				group = i.second;

			// Sprite
			else if (i.first == "sprite" || i.first == "editorsprite")
				found_props["sprite"] = i.second;

			// Angled
			else if (i.first == "angled")
				found_props["angled"] = true;
			else if (i.first == "notangled")
				found_props["angled"] = false;

			// Is Decoration
			else if (i.first == "isdecoration")
				found_props["decoration"] = true;

			// Icon
			else if (i.first == "icon")
				found_props["icon"] = i.second;

			// DB2 Color
			else if (i.first == "color")
				found_props["color"] = i.second;

			// SLADE 3 Colour (overrides DB2 color)
			// Good thing US spelling differs from ABC (Aussie/Brit/Canuck) spelling! :p
			else if (i.first == "colour")
				found_props["colour"] = i.second;

			// Obsolete thing
			else if (i.first == "obsolete")
				found_props["obsolete"] = true;
		}
		editor_properties.clear();

		log::info(3, "Parsed actor {}: {}", name, ednum);
	}
	else
		log::warning("Warning: Invalid actor definition for {}", name);

	// Ignore actors filtered for other games,
	// and actors with a negative or null type
	if (available || !filters_present)
	{
		auto group_path = group.empty() ? "Decorate" : "Decorate/" + group;

		// Find existing definition or create it
		ThingType* def = nullptr;
		if (ednum <= 0)
		{
			for (auto& ptype : parsed)
				if (strutil::equalCI(ptype.className(), actor_name))
				{
					def = &ptype;
					break;
				}

			if (!def)
			{
				parsed.emplace_back(name, group_path, actor_name);
				def = &parsed.back();
			}
		}
		else
			def = &types[ednum];

		// Add/update definition
		def->define(ednum, name, group_path);

		// Set group defaults (if any)
		if (!group.empty())
		{
			auto& group_defaults = configuration().thingTypeGroupDefaults(group);
			if (!group_defaults.group().empty())
				def->copy(group_defaults);
		}

		// Inherit from parent
		if (!parent.empty())
			for (auto& ptype : parsed)
				if (strutil::equalCI(ptype.className(), parent))
				{
					def->copy(ptype);
					break;
				}

		// Set parsed properties
		def->loadProps(found_props);
	}
}

// -----------------------------------------------------------------------------
// Parses an old-style (non-actor) DECORATE definition
// -----------------------------------------------------------------------------
void parseDecorateOld(Tokenizer& tz, std::map<int, ThingType>& types)
{
	string       name, sprite, group;
	bool         spritefound = false;
	char         frame       = 'A';
	bool         framefound  = false;
	int          type        = -1;
	PropertyList found_props;
	if (tz.checkNext("{"))
		name = tz.current().text;
	// DamageTypes aren't old DECORATE format, but we handle them here to skip over them
	else if (tz.checkNC("pickup") || tz.checkNC("breakable") || tz.checkNC("projectile") || tz.checkNC("damagetype"))
	{
		group = tz.current().text;
		name  = tz.next().text;
	}
	tz.adv(); // skip '{'
	do
	{
		tz.adv();

		// if (S_CMPNOCASE(token, "DoomEdNum"))
		if (tz.checkNC("doomednum"))
		{
			// tz.getInteger(&type);
			type = tz.next().asInt();
		}
		// else if (S_CMPNOCASE(token, "Sprite"))
		else if (tz.checkNC("sprite"))
		{
			sprite      = tz.next().text;
			spritefound = true;
		}
		// else if (S_CMPNOCASE(token, "Frames"))
		else if (tz.checkNC("frames"))
		{
			auto     frames = tz.next().text;
			unsigned pos    = 0;
			if (!frames.empty())
			{
				if ((frames[0] < 'a' || frames[0] > 'z') && (frames[0] < 'A' || frames[0] > ']'))
				{
					pos = frames.find(':') + 1;
					if (frames.length() <= pos)
						pos = frames.length() + 1;
					else if ((frames.length() >= pos + 2) && frames[pos + 1] == '*')
						found_props["bright"] = true;
				}
			}
			if (pos < frames.length())
			{
				frame      = frames[pos];
				framefound = true;
			}
		}
		else if (tz.checkNC("radius"))
			found_props["radius"] = tz.next().asInt();
		else if (tz.checkNC("height"))
			found_props["height"] = tz.next().asInt();
		else if (tz.checkNC("solid"))
			found_props["solid"] = true;
		else if (tz.checkNC("spawnceiling"))
			found_props["hanging"] = true;
		else if (tz.checkNC("scale"))
			found_props["scale"] = tz.next().asFloat();
		else if (tz.checkNC("translation1"))
			found_props["translation"] = fmt::format("doom{}", tz.next().asInt());
	} while (!tz.check("}") && !tz.atEnd());

	// Add only if a DoomEdNum is present
	if (type > 0)
	{
		// Determine sprite
		if (spritefound && framefound)
			found_props["sprite"] = sprite + frame + '?';

		// Add type
		types[type].define(type, name, group.empty() ? "Decorate" : "Decorate/" + group);

		// Set parsed properties
		types[type].loadProps(found_props);

		log::info(3, "Parsed {} {}: {}", group.empty() ? "decoration" : group, name, type);
	}
	else
		log::info(3, "Not adding {} {}, no editor number", group.empty() ? "decoration" : group, name);
}

// -----------------------------------------------------------------------------
// Parses all DECORATE thing definitions in [entry] and adds them to [types]
// -----------------------------------------------------------------------------
void parseDecorateEntry(ArchiveEntry* entry, std::map<int, ThingType>& types, vector<ThingType>& parsed)
{
	// Init tokenizer
	Tokenizer tz;
	tz.setSpecialCharacters(":,{}");
	tz.openMem(entry->data(), entry->name());

	// --- Parse ---
	while (!tz.atEnd())
	{
		// Check for #include
		if (tz.checkNC("#include"))
		{
			auto inc_entry = entry->relativeEntry(tz.next().text);

			// Check #include path could be resolved
			if (!inc_entry)
			{
				log::warning(
					"Warning parsing DECORATE entry {}: "
					"Unable to find #included entry \"{}\" at line {}, skipping",
					entry->name(),
					tz.current().text,
					tz.current().line_no);
			}
			else
				parseDecorateEntry(inc_entry, types, parsed);

			tz.adv();
		}

		// Check for actor definition
		else if (tz.checkNC("actor"))
			parseDecorateActor(tz, types, parsed);
		else
			parseDecorateOld(tz, types); // Old DECORATE definitions might be found

		tz.advIf("}");
	}

	// Set entry type
	if (etype_decorate && entry->type() != etype_decorate)
		entry->setType(etype_decorate);
}

} // namespace


// -----------------------------------------------------------------------------
//
// Game Namespace Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Parses all DECORATE thing definitions in [archive] and adds them to [types]
// -----------------------------------------------------------------------------
bool game::readDecorateDefs(Archive* archive, std::map<int, ThingType>& types, vector<ThingType>& parsed)
{
	if (!archive)
		return false;

	// Get base decorate file
	ArchiveSearchOptions opt;
	opt.match_name        = "decorate";
	opt.ignore_ext        = true;
	auto decorate_entries = archive->findAll(opt);
	if (decorate_entries.empty())
		return false;

	log::info(2, "Parsing DECORATE entries found in archive {}", archive->filename());

	// Get DECORATE entry type (all parsed DECORATE entries will be set to this)
	etype_decorate = EntryType::fromId("decorate");
	if (etype_decorate == EntryType::unknownType())
		etype_decorate = nullptr;

	// Parse DECORATE entries
	for (auto entry : decorate_entries)
		parseDecorateEntry(entry, types, parsed);

	return true;
}


// -----------------------------------------------------------------------------
//
// Console Commands
//
// -----------------------------------------------------------------------------


#include "General/Console.h"
#include "MainEditor/MainEditor.h"
CONSOLE_COMMAND(test_decorate, 0, false)
{
	auto archive = maineditor::currentArchive();
	if (!archive)
		return;

	std::map<int, game::ThingType> types;
	vector<ThingType>              parsed;

	if (args.empty())
		game::readDecorateDefs(archive, types, parsed);
	else
	{
		auto entry = archive->entryAtPath(args[0]);
		if (entry)
			parseDecorateEntry(entry, types, parsed);
		else
			log::console("Entry not found");
	}

	for (auto& i : types)
		log::console(fmt::format("{}: {}", i.first, i.second.stringDesc()));
	if (!parsed.empty())
	{
		log::console("Parsed types with no DoomEdNum:");
		for (auto& i : parsed)
			log::console(fmt::format("{}: {}", i.className(), i.stringDesc()));
	}
}
