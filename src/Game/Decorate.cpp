
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
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
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#include "Archive/Archive.h"
#include "Configuration.h"
#include "Decorate.h"
#include "Game.h"
#include "ThingType.h"
#include "Utility/StringUtils.h"
#include "Utility/Tokenizer.h"

using namespace Game;

// ----------------------------------------------------------------------------
//
// Local Functions
//
// ----------------------------------------------------------------------------
namespace
{

struct DecorateState
{
	struct Frame
	{
		string	sprite_base;
		string	sprite_frame;
		int		duration;
	};

	string			name;
	vector<Frame>	frames;
};

// ----------------------------------------------------------------------------
// parseStates
//
// Parses a DECORATE 'States' block
// ----------------------------------------------------------------------------
void parseStates(Tokenizer& tz, PropertyList& props)
{
	vector<string> states;
	string state_first;
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
			states.push_back(tz.current().text.Lower());
			if (state_first.empty())
				state_first = tz.current().text.Lower();

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
			if (!(tz.current().text.Contains("#") || tz.current().text.Contains("-")))
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

	Log::debug(2, S_FMT("Parsed states, got sprite %s", CHR(props["sprite"].getStringValue())));




	//int lastpriority = 0;
	//int priority = 0;
	//int statecounter = 0;
	//string laststate;
	//string spritestate;

	//string token = tz.next().text;
	//while (token != "}")
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
	//				//LOG_MESSAGE(3, "Actor %s found sprite %s from state %s", name, sprite, spritestate);
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

// ----------------------------------------------------------------------------
// parseDecorateActor
//
// Parses a DECORATE 'actor' definition
// ----------------------------------------------------------------------------
void parseDecorateActor(
	Tokenizer& tz,
	std::map<int, ThingType>& types,
	std::map<string, ThingType>& parsed)
{
	// Get actor name
	string name = tz.next().text;
	string actor_name = name;

	// Check for inheritance
	//string next = tz.peekToken();
	if (tz.checkNext(":"))
		tz.adv(2); // Skip ':' and parent
		
	// Check for replaces
	if (tz.checkNextNC("replaces"))
		tz.adv(2); // Skip 'replaces' and actor

	// Skip "native" keyword if present
	if (tz.checkNextNC("native"))
		tz.adv();

	// Check for no editor number (ie can't be placed in the map)
	int type;
	if (!tz.peek().isInteger())
		type = -1;
	else
		tz.next().toInt(type);
		
	PropertyList found_props;
	bool available = false;
	bool filters_present = false;
	bool sprite_given = false;
	bool title_given = false;
	string group;

	// Skip "native" keyword if present
	tz.advIfNextNC("native");

	// Check for actor definition open
	if (tz.advIfNext("{", 2))
	{
		while (!tz.check("}") && !tz.atEnd())
		{
			// Check for subsection
			if (tz.advIf("{"))
			{
				tz.skipSection("{", "}");
				continue;
			}

			// Title
			else if (tz.checkNC("//$Title"))
			{
				name = tz.getLine();
				title_given = true;
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
			else if (!title_given && tz.checkNC("tag"))
				name = tz.next().text;

			// Category
			else if (tz.checkNC("//$Group") || tz.checkNC("//$Category"))
			{
				group = tz.getLine();
				continue;
			}

			// Sprite
			else if (tz.checkNC("//$EditorSprite") || tz.checkNC("//$Sprite"))
			{
				found_props["sprite"] = tz.next().text;
				sprite_given = true;
			}

			// Radius
			else if (tz.checkNC("radius"))
				found_props["radius"] = tz.next().asInt();

			// Height
			else if (tz.checkNC("height"))
				found_props["height"] = tz.next().asInt();

			// Scale
			else if (tz.checkNC("scale"))
				found_props["scalex"] = found_props["scaley"] = tz.next().asFloat();
			else if (tz.checkNC("xscale"))
				found_props["scalex"] = tz.next().asFloat();
			else if (tz.checkNC("yscale"))
				found_props["scaley"] = tz.next().asFloat();

			// Angled
			else if (tz.checkNC("//$Angled"))
				found_props["angled"] = true;
			else if (tz.checkNC("//$NotAngled"))
				found_props["angled"] = false;

			// Monster
			else if (tz.checkNC("monster"))
			{
				found_props["solid"] = true;		// Solid
				found_props["decoration"] = false;	// Not a decoration
			}

			// Hanging
			else if (tz.checkNC("+spawnceiling"))
				found_props["hanging"] = true;

			// Fullbright
			else if (tz.checkNC("+bright"))
				found_props["bright"] = true;

			// Is Decoration
			else if (tz.checkNC("//$IsDecoration"))
				found_props["decoration"] = true;

			// Icon
			else if (tz.checkNC("//$Icon"))
				found_props["icon"] = tz.next().text;

			// DB2 Color
			else if (tz.checkNC("//$Color"))
				found_props["color"] = tz.next().text;

			// SLADE 3 Colour (overrides DB2 color)
			// Good thing US spelling differs from ABC (Aussie/Brit/Canuck) spelling! :p
			else if (tz.checkNC("//$Colour"))
			{
				found_props["colour"] = tz.getLine();
				continue;
			}

			// Obsolete thing
			else if (tz.checkNC("//$Obsolete"))
				found_props["obsolete"] = true;

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

			// Unrecognised DB comment prop
			else if (tz.current().text.StartsWith("//$"))
			{
				tz.advToNextLine();
				continue;
			}

			// States
			if (!sprite_given && tz.checkNC("states"))
			{
				tz.adv(2); // Skip past {
				parseStates(tz, found_props);
			}

			tz.adv();
		}

		LOG_MESSAGE(3, "Parsed actor %s: %d", name, type);
	}
	else
		LOG_MESSAGE(1, "Warning: Invalid actor definition for %s", name);

	// Ignore actors filtered for other games, 
	// and actors with a negative or null type
	if (available || !filters_present)
	{
		// Add/update definition
		auto& def = (type > 0) ? types[type] : parsed[actor_name];
		def.define(type, name, group.empty() ? "Decorate" : "Decorate/" + group);

		// Set group defaults (if any)
		if (!group.empty())
		{
			auto& group_defaults = configuration().thingTypeGroupDefaults(group);
			if (!group_defaults.group().empty())
				def.copy(group_defaults);
		}

		// Set parsed properties
		def.loadProps(found_props);
	}
}

// ----------------------------------------------------------------------------
// parseDecorateOld
//
// Parses an old-style (non-actor) DECORATE definition
// ----------------------------------------------------------------------------
void parseDecorateOld(Tokenizer& tz, std::map<int, ThingType>& types)
{
	string name, sprite, group;
	bool spritefound = false;
	char frame;
	bool framefound = false;
	int type = -1;
	PropertyList found_props;
	if (tz.checkNext("{"))
		name = tz.current().text;
	// DamageTypes aren't old DECORATE format, but we handle them here to skip over them
	else if (
		tz.checkNC("pickup") ||
		tz.checkNC("breakable") ||
		tz.checkNC("projectile") ||
		tz.checkNC("damagetype"))
	{
		group = tz.current().text;
		name = tz.next().text;
	}
	tz.adv();	// skip '{'
	do
	{
		tz.adv();

		//if (S_CMPNOCASE(token, "DoomEdNum"))
		if (tz.checkNC("doomednum"))
		{
			//tz.getInteger(&type);
			type = tz.next().asInt();
		}
		//else if (S_CMPNOCASE(token, "Sprite"))
		else if (tz.checkNC("sprite"))
		{
			sprite = tz.next().text;
			spritefound = true;
		}
		//else if (S_CMPNOCASE(token, "Frames"))
		else if (tz.checkNC("frames"))
		{
			string frames = tz.next().text;
			unsigned pos = 0;
			if (frames.length() > 0)
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
				frame = frames[pos];
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
			found_props["translation"] = S_FMT("doom%d", tz.next().asInt());
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

		LOG_MESSAGE(3, "Parsed %s %s: %d", group.length() ? group : "decoration", name, type);
	}
	else
		LOG_MESSAGE(3, "Not adding %s %s, no editor number", group.length() ? group : "decoration", name);
}

// ----------------------------------------------------------------------------
// getIncludeEntry
//
// Returns the entry at [path] relative to [base], or failing that, the entry
// at absolute [path] in the archive
// ----------------------------------------------------------------------------
ArchiveEntry* getIncludeEntry(ArchiveEntry* base, const string& path)
{
	// Try relative to the base entry
	auto include = base->getParent()->entryAtPath(base->getPath() + path);

	// Try absolute path
	if (!include)
		include = base->getParent()->entryAtPath(path);

	return include;
}

// ----------------------------------------------------------------------------
// parseDecorateEntry
//
// Parses all DECORATE thing definitions in [entry] and adds them to [types]
// ----------------------------------------------------------------------------
void parseDecorateEntry(ArchiveEntry* entry, std::map<int, ThingType>& types, std::map<string, ThingType>& parsed)
{
	// Init tokenizer
	Tokenizer tz;
	tz.setSpecialCharacters(":,{}");
	tz.enableDecorate(true);
	tz.openMem(entry->getMCData(), entry->getName());

	// --- Parse ---
	while (!tz.atEnd())
	{
		Log::debug(2, S_FMT("token %s", CHR(tz.current().text)));

		// Check for #include
		if (tz.checkNC("#include"))
		{
			auto inc_entry = getIncludeEntry(entry, tz.next().text);

			// Check #include path could be resolved
			if (!inc_entry)
			{
				Log::warning(
					S_FMT(
						"Warning parsing DECORATE entry %s: "
						"Unable to find #included entry \"%s\" at line %d, skipping",
						CHR(entry->getName()),
						CHR(tz.current().text),
						tz.current().line_no
				));
			}
			else
				parseDecorateEntry(inc_entry, types, parsed);

			tz.adv();
		}

		// Check for actor definition
		else if (tz.checkNC("actor"))
			parseDecorateActor(tz, types, parsed);
		else
			parseDecorateOld(tz, types);	// Old DECORATE definitions might be found

		tz.advIf("}");
	}
}

} // namespace


// ----------------------------------------------------------------------------
//
// Game Namespace Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// Game::readDecorateDefs
//
// Parses all DECORATE thing definitions in [archive] and adds them to [types]
// ----------------------------------------------------------------------------
bool Game::readDecorateDefs(Archive* archive, std::map<int, ThingType>& types, std::map<string, ThingType>& parsed)
{
	if (!archive)
		return false;

	// Get base decorate file
	Archive::SearchOptions opt;
	opt.match_name = "decorate";
	opt.ignore_ext = true;
	vector<ArchiveEntry*> decorate_entries = archive->findAll(opt);
	if (decorate_entries.empty())
		return false;

	Log::info(2, S_FMT("Parsing DECORATE entries found in archive %s", archive->filename()));

	// Parse DECORATE entries
	for (auto entry : decorate_entries)
		parseDecorateEntry(entry, types, parsed);

	return true;
}



#include "General/Console/Console.h"
#include "MainEditor/MainEditor.h"
CONSOLE_COMMAND(test_decorate, 0, false)
{
	auto archive = MainEditor::currentArchive();
	if (!archive)
		return;

	std::map<int, Game::ThingType> types;
	std::map<string, Game::ThingType> parsed;

	if (args.size() == 0)
		Game::readDecorateDefs(archive, types, parsed);
	else
	{
		auto entry = archive->entryAtPath(args[0]);
		if (entry)
			parseDecorateEntry(entry, types, parsed);
		else
			Log::console("Entry not found");
	}

	for (auto& i : types)
		Log::console(S_FMT("%d: %s", i.first, CHR(i.second.stringDesc())));
	if (!parsed.empty())
	{
		Log::console("Parsed types with no DoomEdNum:");
		for (auto& i : parsed)
			Log::console(S_FMT("%s: %s", CHR(i.first), CHR(i.second.stringDesc())));
	}
}
