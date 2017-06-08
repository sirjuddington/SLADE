
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


// ----------------------------------------------------------------------------
//
// Functions
//
// ----------------------------------------------------------------------------
namespace Game
{
	// ------------------------------------------------------------------------
	// parseStates
	//
	// Parses a DECORATE 'States' block
	// ------------------------------------------------------------------------
	void parseStates(Tokenizer& tz, PropertyList& props)
	{
		int lastpriority = 0;
		int priority = 0;
		int statecounter = 0;
		string laststate;
		string spritestate;

		string token = tz.next().text;
		while (token != "}")
		{
			// Idle, See, Inactive, Spawn, and finally first defined
			if (priority < StateSprites::Idle)
			{
				string myspritestate = token;
				token = tz.next().text;
				while (token.Cmp(":") && token.Cmp("}"))
				{
					myspritestate = token;
					token = tz.next().text;
				}
				if (S_CMPNOCASE(token, "}"))
					break;
				string sb = tz.next().text; // Sprite base

				// Handle removed states
				if (S_CMPNOCASE(sb, "Stop"))
					continue;
				// Handle direct gotos, like ZDoom's dead monsters
				if (S_CMPNOCASE(sb, "Goto"))
				{
					tz.skip();
					// Skip scope and state
					if (tz.peek() == ":")
					{
						tz.skip();	// first :
						tz.skip(); // second :
						tz.skip(); // state name
					}
					continue;
				}
				string sf = tz.next().text; // Sprite frame(s)
				int mypriority = 0;
				// If the same state is given several names, 
				// don't read the next name as a sprite name!
				// If "::" is encountered, it's a scope operator.
				if ((!sf.Cmp(":")) && tz.peek().text.Cmp(":"))
				{
					if (S_CMPNOCASE(myspritestate, "spawn"))
						mypriority = StateSprites::Spawn;
					else if (S_CMPNOCASE(myspritestate, "inactive"))
						mypriority = StateSprites::Inactive;
					else if (S_CMPNOCASE(myspritestate, "see"))
						mypriority = StateSprites::See;
					else if (S_CMPNOCASE(myspritestate, "idle"))
						mypriority = StateSprites::Idle;
					if (mypriority > lastpriority)
					{
						laststate = myspritestate;
						lastpriority = mypriority;
					}
					continue;
				}
				else
				{
					spritestate = myspritestate;
					if (statecounter++ == 0)
						mypriority = StateSprites::FirstDefined;
					if (S_CMPNOCASE(spritestate, "spawn"))
						mypriority = StateSprites::Spawn;
					else if (S_CMPNOCASE(spritestate, "inactive"))
						mypriority = StateSprites::Inactive;
					else if (S_CMPNOCASE(spritestate, "see"))
						mypriority = StateSprites::See;
					else if (S_CMPNOCASE(spritestate, "idle"))
						mypriority = StateSprites::Idle;
					if (lastpriority > mypriority)
					{
						spritestate = laststate;
						mypriority = lastpriority;
					}
				}
				if (sb.length() == 4)
				{
					string sprite = sb + sf.Left(1) + "?";
					if (mypriority > priority)
					{
						priority = mypriority;
						props["sprite"] = sprite;
						//LOG_MESSAGE(3, "Actor %s found sprite %s from state %s", name, sprite, spritestate);
						lastpriority = -1;
					}
				}
			}
			else
			{
				tz.skipSection("{", "}");
				break;
			}
			tz.skip();
		}
	}

	// ------------------------------------------------------------------------
	// parseDecorateActor
	//
	// Parses a DECORATE 'actor' definition
	// ------------------------------------------------------------------------
	void parseDecorateActor(Tokenizer& tz, std::map<int, ThingType>& types)
	{
		// Get actor name
		string name = tz.next().text;

		// Check for inheritance
		//string next = tz.peekToken();
		if (tz.checkNext(":"))
			tz.skip(2); // Skip ':' and parent
		
		// Check for replaces
		if (tz.checkNextNC("replaces"))
			tz.skip(2); // Skip 'replaces' and actor

		// Skip "native" keyword if present
		if (tz.checkNextNC("native"))
			tz.skip();

		// Check for no editor number (ie can't be placed in the map)
		if (tz.checkNext("{"))
		{
			LOG_MESSAGE(3, "Not adding actor %s, no editor number", name);

			// Skip actor definition
			tz.skip(2);
			tz.skipSection("{", "}");
		}
		else
		{
			PropertyList found_props;
			bool available = false;
			bool filters_present = false;
			bool sprite_given = false;
			bool title_given = false;
			int type;
			string group;

			// Read editor number
			tz.next().asInt(type);

			// Skip "native" keyword if present
			tz.skipIfNextNC("native");

			// Check for actor definition open
			if (tz.skipIfNext("{", 2))
			{
				while (!tz.check("}") && !tz.atEnd())
				{
					// Check for subsection
					if (tz.skipIf("{"))
						tz.skipSection("{", "}");

					// Title
					else if (tz.checkNC("//$Title"))
					{
						name = tz.getLine();
						title_given = true;
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
						group = tz.getLine();

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
						found_props["colour"] = tz.getLine();

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
						tz.skipToNextLine();
						continue;
					}

					// States
					if (!sprite_given && tz.checkNC("states"))
					{
						tz.skip(); // Skip {
						parseStates(tz, found_props);
						continue;
					}

					tz.skip();
				}

				LOG_MESSAGE(3, "Parsed actor %s: %d", name, type);
			}
			else
				LOG_MESSAGE(1, "Warning: Invalid actor definition for %s", name);

			// Ignore actors filtered for other games, 
			// and actors with a negative or null type
			if (type > 0 && (available || !filters_present))
			{
				// Add new ThingType
				types[type].define(type, name, group.empty() ? "Decorate" : "Decorate/" + group);

				// Set group defaults (if any)
				if (!group.empty())
				{
					auto& group_defaults = configuration().thingTypeGroupDefaults(group);
					if (!group_defaults.group().empty())
						types[type].copy(group_defaults);
				}

				// Set parsed properties
				types[type].loadProps(found_props);
			}
		}
	}

	// ------------------------------------------------------------------------
	// parseDecorateOld
	//
	// Parses an old-style (non-actor) DECORATE definition
	// ------------------------------------------------------------------------
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
		tz.skip();	// skip '{'
		do
		{
			tz.skip();

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
}

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
bool Game::readDecorateDefs(Archive* archive, std::map<int, ThingType>& types)
{
	if (!archive)
		return false;

	// Get base decorate file
	Archive::search_options_t opt;
	opt.match_name = "decorate";
	opt.ignore_ext = true;
	vector<ArchiveEntry*> decorate_entries = archive->findAll(opt);
	if (decorate_entries.empty())
		return false;

	Log::info(2, S_FMT("Parsing DECORATE entries found in archive %s", archive->getFilename()));

	// Build full definition string
	string full_defs;
	for (unsigned a = 0; a < decorate_entries.size(); a++)
		StringUtils::processIncludes(decorate_entries[a], full_defs, false);

	// Init tokenizer
	Tokenizer tz;
	tz.setSpecialCharacters(":,{}");
	tz.enableDecorate(true);
	tz.openString(full_defs);

	// --- Parse ---
	while (!tz.atEnd())
	{
		// Check for actor definition
		if (tz.checkNC("actor"))
			parseDecorateActor(tz, types);
		else
			parseDecorateOld(tz, types);	// Old DECORATE definitions might be found

		tz.skipIf("}");
	}

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
	Game::readDecorateDefs(archive, types);

	for (auto& i : types)
		Log::console(S_FMT("%d: %s", i.first, CHR(i.second.stringDesc())));
}
