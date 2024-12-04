
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Lua.cpp
// Description: Lua scripting system
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
#include "Lua.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Export/Export.h"
#include "General/Console.h"
#include "General/Misc.h"
#include "LuaBridge.h"
#include "SLADEMap/SLADEMap.h"
#include "UI/Dialogs/ExtMessageDialog.h"
#include "UI/WxUtils.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace slade::lua
{
lua_State* lua            = nullptr;
wxWindow*  current_window = nullptr;
Error      script_error;
time_t     script_start_time;
} // namespace slade::lua


// -----------------------------------------------------------------------------
//
// Lua Namespace Functions
//
// -----------------------------------------------------------------------------
namespace slade::lua
{
// -----------------------------------------------------------------------------
// Resets error information
// -----------------------------------------------------------------------------
void resetError()
{
	script_error.type    = "No";
	script_error.message = "No error(s) occurred";
	script_error.line_no = 0;
}

// -----------------------------------------------------------------------------
// Logs the given [error]
// -----------------------------------------------------------------------------
void logError(const Error& error)
{
	if (error.line_no >= 0)
		log::error("{} Error running Lua script: {}: {}", error.type, error.line_no, error.message);
	else
		log::error("{} Error running Lua script: {}", error.type, error.message);
}

// -----------------------------------------------------------------------------
// Processes error information from [result]
// -----------------------------------------------------------------------------
// void processError(const sol::protected_function_result& result)
//{
//	// Error Type
//	script_error.type = sol::to_string(result.status());
//	strutil::capitalizeIP(script_error.type);
//
//	// Error Message
//	sol::error  error     = result;
//	string_view error_msg = error.what();
//
//	// Find Line No.
//	if (auto pos = error_msg.find("]:"); pos != string_view::npos)
//	{
//		auto pos_ln_end      = error_msg.find(':', pos + 2);
//		script_error.line_no = strutil::asInt(error_msg.substr(pos + 2, pos_ln_end - pos + 2)); // Line No.
//		script_error.message = error_msg.substr(pos_ln_end + 2);                                // Actual error message
//	}
//	else
//	{
//		// No line number in error message
//		script_error.line_no = -1;
//		script_error.message = error_msg;
//	}
//}

// -----------------------------------------------------------------------------
// Handles a lua error from a sol2 protected_function_result [pfr]
// -----------------------------------------------------------------------------
// sol::protected_function_result handleError(lua_State* L, sol::protected_function_result pfr)
//{
//	processError(pfr);
//	logError(script_error);
//
//	return pfr;
//}

// Traceback function, when a runtime error occurs, this will append the call stack to the error message
int traceback(lua_State* L)
{
	// look up Lua's 'debug.traceback' function
	lua_getglobal(L, "debug");
	if (!lua_istable(L, -1))
	{
		lua_pop(L, 1);
		return 1;
	}

	lua_getfield(L, -1, "traceback");
	if (!lua_isfunction(L, -1))
	{
		lua_pop(L, 2);
		return 1;
	}

	lua_pushvalue(L, 1);
	lua_pushinteger(L, 2);
	lua_call(L, 2, 1);

	lua_getglobal(L, "print");
	if (!lua_isfunction(L, -1))
	{
		lua_pop(L, 1);
		return 1;
	}

	lua_pushvalue(L, 1);
	lua_call(L, 1, 0);

	return 1;
}

bool runLua(const std::string& script, lua_State* state = nullptr)
{
	if (!state)
		state = lua;

	lua_settop(state, 0);

	luabridge::lua_pushcfunction_x(state, &traceback, "traceback");

	if (luaL_loadstring(state, script.c_str()) != LUABRIDGE_LUA_OK)
	{
		auto error_string = lua_tostring(state, -1);

		throw std::runtime_error(error_string ? error_string : "Unknown lua compilation error");
	}

	if (lua_pcall(state, 0, 0, -2) != LUABRIDGE_LUA_OK)
	{
		auto error_string = lua_tostring(state, -1);

		throw std::runtime_error(error_string ? error_string : "Unknown lua runtime error");
	}

	return true;
}

// -----------------------------------------------------------------------------
// Template function for Lua::run*Script functions.
// Loads [script] and runs the 'Execute' function in the script, passing
// [param] to the function
// -----------------------------------------------------------------------------
template<class T> bool runEditorScript(const string& script, T param)
{
	// resetError();
	// script_start_time = wxDateTime::Now().GetTicks();

	//// Load script
	// sol::environment sandbox(lua, sol::create, lua.globals());
	// auto             load_result = lua.script(script, sandbox, handleError);
	// if (!load_result.valid())
	//{
	//	processError(load_result);
	//	logError(script_error);
	//	return false;
	// }

	//// Run script execute function
	// sol::protected_function        func(sandbox["Execute"]);
	// sol::protected_function_result exec_result = func(param);
	// if (!exec_result.valid())
	//{
	//	sol::error error = exec_result;
	//	processError(exec_result);
	//	logError(script_error);
	//	return false;
	// }

	return true;
}

} // namespace slade::lua

// -----------------------------------------------------------------------------
// Initialises lua and registers functions
// -----------------------------------------------------------------------------
bool lua::init()
{
	// Create global lua state
	lua = luaL_newstate();

	// Load all lua libraries except io and os
	luaL_requiref(lua, "_G", luaopen_base, 1);
	luaL_requiref(lua, LUA_LOADLIBNAME, luaopen_package, 1);
	luaL_requiref(lua, LUA_COLIBNAME, luaopen_coroutine, 1);
	luaL_requiref(lua, LUA_TABLIBNAME, luaopen_table, 1);
	// luaL_requiref(lua, LUA_IOLIBNAME, luaopen_io, 1);
	// luaL_requiref(lua, LUA_OSLIBNAME, luaopen_os, 1);
	luaL_requiref(lua, LUA_STRLIBNAME, luaopen_string, 1);
	luaL_requiref(lua, LUA_MATHLIBNAME, luaopen_math, 1);
	luaL_requiref(lua, LUA_UTF8LIBNAME, luaopen_utf8, 1);
	luaL_requiref(lua, LUA_DBLIBNAME, luaopen_debug, 1);

	// Register namespaces
	registerAppNamespace(lua);
	registerUINamespace(lua);
	registerGameNamespace(lua);
	registerArchivesNamespace(lua);
	registerGraphicsNamespace(lua);

	// Register types
	registerMiscTypes(lua);
	registerArchiveTypes(lua);
	registerMapEditorTypes(lua);
	registerGameTypes(lua);
	registerGraphicsTypes(lua);

	// Override default lua print to redirect it to the script log
	auto new_print =
		"function  print (...)\
        local line = ''\
        for i,v in ipairs({...}) do\
           line = line .. tostring(v) .. ' '\
        end\
        App.LogMessage(line)\
     end";
	runLua(new_print, lua);

	return true;
}

// -----------------------------------------------------------------------------
// Close the lua state
// -----------------------------------------------------------------------------
void lua::close() {}

// -----------------------------------------------------------------------------
// Returns information about the last script error that occurred
// -----------------------------------------------------------------------------
lua::Error& lua::error()
{
	return script_error;
}

// -----------------------------------------------------------------------------
// Shows an extended message dialog with details of the last script error that
// occurred
// -----------------------------------------------------------------------------
void lua::showErrorDialog(wxWindow* parent, string_view title, string_view message)
{
	// Get script log messages since the last script was started
	auto   log = log::since(script_start_time, log::MessageType::Script);
	string output;
	for (auto msg : log)
		output += msg->formattedMessageLine() + "\n";

	ExtMessageDialog dlg(parent ? parent : current_window, wxutil::strFromView(title));
	dlg.setMessage(wxutil::strFromView(message));
	const auto& [type, error_msg, line_no] = lua::error();
	if (line_no >= 0)
		dlg.setExt(wxString::Format("%s Error\nLine %d: %s\n\nScript Output:\n%s", type, line_no, error_msg, output));
	else
		dlg.setExt(wxString::Format("%s Error\n%s\n\nScript Output:\n%s", type, error_msg, output));

	dlg.CenterOnParent();
	dlg.ShowModal();
}

// -----------------------------------------------------------------------------
// Runs a lua script [program]
// -----------------------------------------------------------------------------
bool lua::run(const string& program)
{
	/*resetError();
	script_start_time = wxDateTime::Now().GetTicks();

	sol::environment sandbox(lua, sol::create, lua.globals());
	auto             result = lua.script(program, sandbox, handleError);
	lua.collect_garbage();

	if (!result.valid())
	{
		processError(result);
		logError(script_error);
		return false;
	}*/

	auto thread = lua_newthread(lua);

	try
	{
		runLua(program, thread);
	}
	catch (const std::exception& ex)
	{
		log::error(ex.what());
	}

	return true;
}

// -----------------------------------------------------------------------------
// Runs a lua script from a text file [filename]
// -----------------------------------------------------------------------------
bool lua::runFile(const string& filename)
{
	/*resetError();
	script_start_time = wxDateTime::Now().GetTicks();

	sol::environment sandbox(lua, sol::create, lua.globals());
	auto             result = lua.script_file(filename, sandbox, handleError);
	lua.collect_garbage();

	if (!result.valid())
	{
		processError(result);
		logError(script_error);
		return false;
	}*/

	return true;
}

// -----------------------------------------------------------------------------
// Runs the 'Execute(archive)' function in the given [script], passing [archive]
// as the parameter
// -----------------------------------------------------------------------------
bool lua::runArchiveScript(const string& script, Archive* archive)
{
	return runEditorScript<Archive*>(script, archive);
}

// -----------------------------------------------------------------------------
// Runs the 'Execute(entries)' function in the given [script], passing [entries]
// as the parameter
// -----------------------------------------------------------------------------
bool lua::runEntryScript(const string& script, vector<ArchiveEntry*>& entries)
{
	return runEditorScript<vector<ArchiveEntry*>&>(script, entries);
}

// -----------------------------------------------------------------------------
// Runs the 'Execute(map)' function in the given [script], passing [map] as the
// parameter
// -----------------------------------------------------------------------------
bool lua::runMapScript(const string& script, SLADEMap* map)
{
	return runEditorScript<SLADEMap*>(script, map);
}

// -----------------------------------------------------------------------------
// Returns the active lua state
// -----------------------------------------------------------------------------
lua_State* lua::state()
{
	return lua;
}

// -----------------------------------------------------------------------------
// Returns the current window (used as the parent window for UI-related
// scripting functions such as messageBox)
// -----------------------------------------------------------------------------
wxWindow* lua::currentWindow()
{
	return current_window;
}

// -----------------------------------------------------------------------------
// Sets the current [window] (used as the parent window for UI-related scripting
// functions such as messageBox)
// -----------------------------------------------------------------------------
void lua::setCurrentWindow(wxWindow* window)
{
	current_window = window;
}


// -----------------------------------------------------------------------------
//
// Console Commands
//
// -----------------------------------------------------------------------------

CONSOLE_COMMAND(script, 1, true)
{
	auto script_text = args[0];
	for (unsigned a = 1; a < args.size(); a++)
		script_text += " " + args[a];

	lua::run(script_text);
}

CONSOLE_COMMAND(script_file, 1, true)
{
	if (!wxFile::Exists(args[0]))
	{
		log::error("File \"{}\" does not exist", args[0]);
		return;
	}

	if (!lua::runFile(args[0]))
		log::error("Error loading lua script file \"{}\"", args[0]);
}

CONSOLE_COMMAND(lua_mem, 0, false)
{
	// auto mem = lua::state().memory_used();
	// log::console(fmt::format("Lua state using {} memory", misc::sizeAsString(mem)));
}
