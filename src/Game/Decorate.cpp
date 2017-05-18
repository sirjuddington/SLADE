
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

		string token = tz.getToken();
		while (token != "}")
		{
			// Idle, See, Inactive, Spawn, and finally first defined
			if (priority < StateSprites::Idle)
			{
				string myspritestate = token;
				token = tz.getToken();
				while (token.Cmp(":") && token.Cmp("}"))
				{
					myspritestate = token;
					token = tz.getToken();
				}
				if (S_CMPNOCASE(token, "}"))
					break;
				string sb = tz.getToken(); // Sprite base

				// Handle removed states
				if (S_CMPNOCASE(sb, "Stop"))
					continue;
				// Handle direct gotos, like ZDoom's dead monsters
				if (S_CMPNOCASE(sb, "Goto"))
				{
					tz.skipToken();
					// Skip scope and state
					if (tz.peekToken() == ":")
					{
						tz.skipToken();	// first :
						tz.skipToken(); // second :
						tz.skipToken(); // state name
					}
					continue;
				}
				string sf = tz.getToken(); // Sprite frame(s)
				int mypriority = 0;
				// If the same state is given several names, 
				// don't read the next name as a sprite name!
				// If "::" is encountered, it's a scope operator.
				if ((!sf.Cmp(":")) && tz.peekToken().Cmp(":"))
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
			token = tz.getToken();
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
		string name = tz.getToken();

		// Check for inheritance
		string next = tz.peekToken();
		if (next == ":")
		{
			tz.skipToken(); // Skip :
			tz.skipToken(); // Skip parent actor
			next = tz.peekToken();
		}

		// Check for replaces
		if (S_CMPNOCASE(next, "replaces"))
		{
			tz.skipToken(); // Skip replaces
			tz.skipToken(); // Skip replace actor
		}

		// Skip "native" keyword if present
		if (S_CMPNOCASE(tz.peekToken(), "native"))
			tz.skipToken();

		// Check for no editor number (ie can't be placed in the map)
		if (tz.peekToken() == "{")
		{
			LOG_MESSAGE(3, "Not adding actor %s, no editor number", name);

			// Skip actor definition
			tz.skipToken();
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
			tz.getInteger(&type);

			// Skip "native" keyword if present
			if (S_CMPNOCASE(tz.peekToken(), "native"))
				tz.skipToken();

			// Check for actor definition open
			string token = tz.getToken();
			if (token == "{")
			{
				token = tz.getToken();
				while (token != "}")
				{
					// Check for subsection
					if (token == "{")
						tz.skipSection("{", "}");

					// Title
					else if (S_CMPNOCASE(token, "//$Title"))
					{
						name = tz.getLine();
						title_given = true;
					}

					// Game filter
					else if (S_CMPNOCASE(token, "game"))
					{
						filters_present = true;
						if (gameDef(configuration().currentGame()).supportsFilter(tz.getToken()))
							available = true;
					}

					// Tag
					else if (!title_given && S_CMPNOCASE(token, "tag"))
						name = tz.getToken();

					// Category
					else if (S_CMPNOCASE(token, "//$Group") || S_CMPNOCASE(token, "//$Category"))
						group = tz.getLine();

					// Sprite
					else if (S_CMPNOCASE(token, "//$EditorSprite") || S_CMPNOCASE(token, "//$Sprite"))
					{
						found_props["sprite"] = tz.getToken();
						sprite_given = true;
					}

					// Radius
					else if (S_CMPNOCASE(token, "radius"))
						found_props["radius"] = tz.getInteger();

					// Height
					else if (S_CMPNOCASE(token, "height"))
						found_props["height"] = tz.getInteger();

					// Scale
					else if (S_CMPNOCASE(token, "scale"))
						found_props["scalex"] = found_props["scaley"] = tz.getFloat();
					else if (S_CMPNOCASE(token, "xscale"))
						found_props["scalex"] = tz.getFloat();
					else if (S_CMPNOCASE(token, "yscale"))
						found_props["scaley"] = tz.getFloat();

					// Angled
					else if (S_CMPNOCASE(token, "//$Angled"))
						found_props["angled"] = true;
					else if (S_CMPNOCASE(token, "//$NotAngled"))
						found_props["angled"] = false;

					// Monster
					else if (S_CMPNOCASE(token, "monster"))
					{
						found_props["solid"] = true;		// Solid
						found_props["decoration"] = false;	// Not a decoration
					}

					// Hanging
					else if (S_CMPNOCASE(token, "+spawnceiling"))
						found_props["hanging"] = true;

					// Fullbright
					else if (S_CMPNOCASE(token, "+bright"))
						found_props["bright"] = true;

					// Is Decoration
					else if (S_CMPNOCASE(token, "//$IsDecoration"))
						found_props["decoration"] = true;

					// Icon
					else if (S_CMPNOCASE(token, "//$Icon"))
						found_props["icon"] = tz.getToken();

					// DB2 Color
					else if (S_CMPNOCASE(token, "//$Color"))
						found_props["color"] = tz.getToken();

					// SLADE 3 Colour (overrides DB2 color)
					// Good thing US spelling differs from ABC (Aussie/Brit/Canuck) spelling! :p
					else if (S_CMPNOCASE(token, "//$Colour"))
						found_props["colour"] = tz.getLine();

					// Obsolete thing
					else if (S_CMPNOCASE(token, "//$Obsolete"))
						found_props["obsolete"] = true;

					// Translation
					else if (S_CMPNOCASE(token, "translation"))
					{
						string translation = "\"";
						translation += tz.getToken();
						while (tz.peekToken() == ",")
						{
							translation += tz.getToken(); // ,
							translation += tz.getToken(); // next range
						}
						translation += "\"";
						found_props["translation"] = translation;
					}

					// Solid
					else if (S_CMPNOCASE(token, "+solid"))
						found_props["solid"] = true;

					// States
					if (!sprite_given && S_CMPNOCASE(token, "states"))
					{
						tz.skipToken(); // Skip {
						parseStates(tz, found_props);
					}

					token = tz.getToken();
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
		string name, sprite, group, token;
		bool spritefound = false;
		char frame;
		bool framefound = false;
		int type = -1;
		PropertyList found_props;
		if (tz.peekToken() == "{")
			name = token;
		// DamageTypes aren't old DECORATE format, but we handle them here to skip over them
		else if ((S_CMPNOCASE(token, "pickup")) || (S_CMPNOCASE(token, "breakable"))
			|| (S_CMPNOCASE(token, "projectile")) || (S_CMPNOCASE(token, "damagetype")))
		{
			group = token;
			name = tz.getToken();
		}
		tz.skipToken();	// skip '{'
		do
		{
			token = tz.getToken();
			if (S_CMPNOCASE(token, "DoomEdNum"))
			{
				tz.getInteger(&type);
			}
			else if (S_CMPNOCASE(token, "Sprite"))
			{
				sprite = tz.getToken();
				spritefound = true;
			}
			else if (S_CMPNOCASE(token, "Frames"))
			{
				token = tz.getToken();
				unsigned pos = 0;
				if (token.length() > 0)
				{
					if ((token[0] < 'a' || token[0] > 'z') && (token[0] < 'A' || token[0] > ']'))
					{
						pos = token.find(':') + 1;
						if (token.length() <= pos)
							pos = token.length() + 1;
						else if ((token.length() >= pos + 2) && token[pos + 1] == '*')
							found_props["bright"] = true;
					}
				}
				if (pos < token.length())
				{
					frame = token[pos];
					framefound = true;
				}
			}
			else if (S_CMPNOCASE(token, "Radius"))
				found_props["radius"] = tz.getInteger();
			else if (S_CMPNOCASE(token, "Height"))
				found_props["height"] = tz.getInteger();
			else if (S_CMPNOCASE(token, "Solid"))
				found_props["solid"] = true;
			else if (S_CMPNOCASE(token, "SpawnCeiling"))
				found_props["hanging"] = true;
			else if (S_CMPNOCASE(token, "Scale"))
				found_props["scale"] = tz.getFloat();
			else if (S_CMPNOCASE(token, "Translation1"))
				found_props["translation"] = S_FMT("doom%d", tz.getInteger());
		} while (token != "}" && !token.empty());

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
	string token = tz.getToken();
	while (!token.empty())
	{
		// Check for actor definition
		if (S_CMPNOCASE(token, "actor"))
			parseDecorateActor(tz, types);
		else
			parseDecorateOld(tz, types);	// Old DECORATE definitions might be found

		token = tz.getToken();
	}

	return true;
}
