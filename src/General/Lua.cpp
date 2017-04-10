
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    Lua.cpp
 * Description: Lua scripting system (nothing much here yet)
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
#include "Lua.h"
#include "External/lua/lua.hpp"
#include "General/Console/Console.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
lua_State*	lua_state = NULL;


/*******************************************************************
 * LUA NAMESPACE FUNCTIONS
 *******************************************************************/
namespace Lua
{
	// --- Functions ---

	// log_message: Prints a log message (concatenates args)
	int log_message(lua_State* ls)
	{
		int argc = lua_gettop(ls);

		string message;
		for (int a = 1; a <= argc; a++)
			message += lua_tostring(ls, a);

		if (!message.IsEmpty())
			LOG_MESSAGE(1, message);

		return 0;
	}

	/*int set_mobj_int_prop(lua_State* ls) {
		SLADEMap& map = MapEditor::editContext().getMap();

		int argc = lua_gettop(ls);
		if (argc < 4)
			return 0;

		int type = lua_tointeger(ls, 1);
		int index = lua_tointeger(ls, 2);
		MapObject* mobj = map.getObject(type, index);

		if (!mobj) {
			LOG_MESSAGE(1, "Invalid map object");
			return 0;
		}

		string prop = lua_tostring(ls, 3);
		int value = lua_tointeger(ls, 4);
		mobj->setIntProperty(prop, value);

		return 0;
	}*/
}

/* Lua::init
 * Initialises lua and registers functions
 *******************************************************************/
bool Lua::init()
{
	// Init lua state
	lua_state = luaL_newstate();
	luaL_openlibs(lua_state);

	// Register functions
	lua_register(lua_state, "log_message", log_message);
	//lua_register(lua_state, "set_mobj_int_prop", set_mobj_int_prop);

	return true;
}

/* Lua::close
 * Close the lua state
 *******************************************************************/
void Lua::close()
{
	lua_close(lua_state);
}

/* Lua::run
 * Runs a lua script [program]
 *******************************************************************/
bool Lua::run(string program)
{
	// Load string to Lua
	if (luaL_loadstring(lua_state, CHR(program)) == 0)
	{
		// Execute script
		lua_pcall(lua_state, 0, LUA_MULTRET, 0);

		return true;
	}

	return false;
}

/* Lua::runFile
 * Runs a lua script from a text file [filename]
 *******************************************************************/
bool Lua::runFile(string filename)
{
	// Load file to Lua
	if (luaL_loadfile(lua_state, CHR(filename)) == 0)
	{
		// Execute script
		lua_pcall(lua_state, 0, LUA_MULTRET, 0);

		return true;
	}

	return false;
}


/*******************************************************************
 * CONSOLE COMMANDS
 *******************************************************************/

CONSOLE_COMMAND(lua_exec, 1, true)
{
	Lua::run(args[0]);
}

CONSOLE_COMMAND(lua_execfile, 1, true)
{
	if (!wxFile::Exists(args[0]))
	{
		LOG_MESSAGE(1, "File \"%s\" does not exist", args[0]);
		return;
	}

	if (!Lua::runFile(args[0]))
		LOG_MESSAGE(1, "Error loading lua script file \"%s\"", args[0]);
}



/*

Potential Lua functions

Map Editor:
getMapObjectProperty(type, index, property)
getMapObjectProperty(id, property)
getMapObjectModifiedTime(type, index)
getMapObjectId(type, index)
setMapObjectProperty(type, index, property, value)
setMapObjectProperty(id, property, value)
getNumObjects(type)

*/
