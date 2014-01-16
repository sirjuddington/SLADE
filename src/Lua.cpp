
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


#include "Main.h"
#include "Lua.h"
#include "lua/lua.hpp"
#include "Console.h"
#include <wx/file.h>

lua_State*	lua_state = NULL;

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
			wxLogMessage(message);

		return 0;
	}

	/*int set_mobj_int_prop(lua_State* ls) {
		SLADEMap& map = theMapEditor->mapEditor().getMap();

		int argc = lua_gettop(ls);
		if (argc < 4)
			return 0;

		int type = lua_tointeger(ls, 1);
		int index = lua_tointeger(ls, 2);
		MapObject* mobj = map.getObject(type, index);

		if (!mobj) {
			wxLogMessage("Invalid map object");
			return 0;
		}

		string prop = lua_tostring(ls, 3);
		int value = lua_tointeger(ls, 4);
		mobj->setIntProperty(prop, value);

		return 0;
	}*/
}

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

void Lua::close()
{
	lua_close(lua_state);
}

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


CONSOLE_COMMAND(lua_exec, 1, true)
{
	Lua::run(args[0]);
}

CONSOLE_COMMAND(lua_execfile, 1, true)
{
	if (!wxFile::Exists(args[0]))
	{
		wxLogMessage("File \"%s\" does not exist", args[0]);
		return;
	}

	if (!Lua::runFile(args[0]))
		wxLogMessage("Error loading lua script file \"%s\"", args[0]);
}
