
#include "Main.h"
#include "Lua.h"
#include "lua/lua.hpp"
#include "Console.h"
#include <wx/file.h>

lua_State*	lua_state = NULL;

namespace Lua {
	// --- Functions ---
	
	// log_message: Prints a log message (concatenates args)
	int log_message(lua_State* ls) {
		int argc = lua_gettop(ls);

		string message;
		for (int a = 1; a <= argc; a++)
			message += lua_tostring(ls, a);

		if (!message.IsEmpty())
			wxLogMessage(message);

		return 0;
	}
}

bool Lua::init() {
	// Init lua state
	lua_state = luaL_newstate();
	luaL_openlibs(lua_state);

	// Register functions
	lua_register(lua_state, "log_message", log_message);

	return true;
}

void Lua::close() {
	lua_close(lua_state);
}

bool Lua::run(string program) {
	if (luaL_loadstring(lua_state, CHR(program)) == 0) {
		lua_pcall(lua_state, 0, LUA_MULTRET, 0);
		return true;
	}

	return false;
}


CONSOLE_COMMAND(lua_exec, 1) {
	Lua::run(args[0]);
}

CONSOLE_COMMAND(lua_execfile, 1) {
	wxFile file(args[0], wxFile::read);
	if (file.IsOpened()) {
		// Read file
		char* buf = (char*)malloc(file.Length());
		file.Read(buf, file.Length());

		// Run
		Lua::run(wxString::FromAscii(buf, file.Length()));

		// Cleanup
		free(buf);
	}
	else
		wxLogMessage("Unable to open file \"%s\"", CHR(args[0]));
}
