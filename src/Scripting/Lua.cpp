
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
#include "Utility/FileUtils.h"
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
// Environment Struct Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Environment constructor
// -----------------------------------------------------------------------------
lua::Environment::Environment(lua_State* state)
{
	if (state)
		apply(state);
}

// -----------------------------------------------------------------------------
// Environment destructor
// -----------------------------------------------------------------------------
lua::Environment::~Environment()
{
	if (state)
	{
		// Revert back to the global environment if this was applied
		lua_pushglobaltable(state);
		lua_setglobal(state, "_ENV");
	}
}

// -----------------------------------------------------------------------------
// Creates a new environment table inheriting from global and applies it to the
// given lua [state_to_apply]
// -----------------------------------------------------------------------------
void lua::Environment::apply(lua_State* state_to_apply)
{
	if (state)
		return; // Can't apply twice

	luaL_dostring(state_to_apply, "return setmetatable({}, {__index = _G}); ");
	lua_setupvalue(state_to_apply, -2, 1);

	state = state_to_apply;
}


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
static void resetError()
{
	script_error.type    = "Unknown";
	script_error.message = "An unknown error occurred";
	script_error.line_no = -1;
}

// -----------------------------------------------------------------------------
// Logs the given [error]
// -----------------------------------------------------------------------------
static void logError(const Error& error)
{
	auto error_lines = strutil::splitV(error.message, '\n');
	if (error_lines.empty())
	{
		log::error("{} Error running Lua script: {}", error.type, error.message);
		return;
	}

	if (error.line_no >= 0)
		log::error("{} Error running Lua script: {}: {}", error.type, error.line_no, error_lines[0]);
	else
		log::error("{} Error running Lua script: {}", error.type, error_lines[0]);

	if (error_lines.size() > 1)
	{
		for (size_t a = 1; a < error_lines.size(); a++)
			log::error("{}", error_lines[a]);
	}
}

// -----------------------------------------------------------------------------
// Removes the source string from an error message
// -----------------------------------------------------------------------------
static string removeSourceStringFromError(string_view error_message, string_view source = "script")
{
	auto begin = error_message.find("[string");
	auto end   = error_message.find(']', begin);

	if (begin != string_view::npos && end != string_view::npos)
	{
		auto result = fmt::format("{}[{}]{}", error_message.substr(0, begin), source, error_message.substr(end + 1));
		return removeSourceStringFromError(result);
	}

	return string{ error_message };
}

// -----------------------------------------------------------------------------
// Processes error information from [result]
// -----------------------------------------------------------------------------
static void processError(string_view type, string_view message)
{
	// Error Type
	script_error.type = type;
	strutil::capitalizeIP(script_error.type);

	// Cut out [string ...] from error message
	script_error.message = removeSourceStringFromError(message);

	// Find Line No.
	if (auto pos = script_error.message.find("]:"); pos != string_view::npos)
	{
		auto pos_ln_end      = script_error.message.find(':', pos + 2);
		script_error.line_no = strutil::asInt(script_error.message.substr(pos + 2, pos_ln_end - pos + 2)); // Line No.
		if (script_error.message[0] == '[')
			script_error.message = script_error.message.substr(pos_ln_end + 2); // Actual error message
	}
	else
	{
		// No line number in error message
		script_error.line_no = -1;
	}
}

// -----------------------------------------------------------------------------
// Taken from the luabridge tests
//
// Traceback function, when a runtime error occurs, this will append the call
// stack to the error message
// -----------------------------------------------------------------------------
static int traceback(lua_State* L)
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

	/*lua_getglobal(L, "print");
	if (!lua_isfunction(L, -1))
	{
		lua_pop(L, 1);
		return 1;
	}

	lua_pushvalue(L, 1);
	lua_call(L, 1, 0);*/

	return 1;
}

// -----------------------------------------------------------------------------
// Runs the given [script] in the given lua [state] with the given (optional)
// [environment].
// If [state] is nullptr, the global lua state is used
// -----------------------------------------------------------------------------
static bool runLua(const std::string& script, lua_State* state = nullptr, Environment* environment = nullptr)
{
	if (!state)
		state = lua;

	lua_settop(state, 0);

	luabridge::lua_pushcfunction_x(state, &traceback, "traceback");

	if (luaL_loadstring(state, script.c_str()) != LUABRIDGE_LUA_OK)
	{
		auto error_string = lua_tostring(state, -1);

		throw LuaException("Syntax", error_string ? error_string : "Unknown lua compilation error");
	}

	if (environment)
		environment->apply(state);

	if (lua_pcall(state, 0, LUA_MULTRET, -2) != LUABRIDGE_LUA_OK)
	{
		auto error_string = lua_tostring(state, -1);

		throw LuaException("Runtime", error_string ? error_string : "Unknown lua runtime error");
	}

	return true;
}

// -----------------------------------------------------------------------------
// Template function for Lua::run*Script functions.
// Loads [script] and runs the function returned from the script, passing
// [param] to the function
// -----------------------------------------------------------------------------
template<class T> static bool runEditorScript(const string& script, T param)
{
	resetError();
	script_start_time = wxDateTime::Now().GetTicks();

	bool        success = true;
	Environment env;
	try
	{
		runLua(script, lua, &env);

		auto exec_func = luabridge::LuaRef::fromStack(lua);
		if (exec_func.isFunction())
		{
			exec_func(param);
		}
		else
		{
			script_error.type    = "Runtime";
			script_error.message = "Editor script must return a function";
			script_error.line_no = -1;
			logError(script_error);
			success = false;
		}
	}
	catch (const LuaException& ex)
	{
		processError(ex.errorType(), ex.what());
		logError(script_error);
		success = false;
	}

	return success;
}
} // namespace slade::lua

// -----------------------------------------------------------------------------
// Initialises lua and registers functions
// -----------------------------------------------------------------------------
bool lua::init()
{
	if (lua)
	{
		lua_close(lua);
		lua = nullptr;
	}

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
	resetError();
	script_start_time = wxDateTime::Now().GetTicks();

	bool        success = true;
	Environment env;
	try
	{
		runLua(program, lua, &env);
	}
	catch (const LuaException& ex)
	{
		processError(ex.errorType(), ex.what());
		logError(script_error);
		success = false;
	}
	catch (const std::exception& ex)
	{
		processError("Runtime", ex.what());
		logError(script_error);
		success = false;
	}

	lua_gc(lua, LUA_GCCOLLECT, 0);

	return success;
}

// -----------------------------------------------------------------------------
// Runs a lua script from a text file [filename]
// -----------------------------------------------------------------------------
bool lua::runFile(const string& filename)
{
	string script;

	if (fileutil::readFileToString(filename, script))
		return run(script);

	return false;
}

// -----------------------------------------------------------------------------
// Runs the function returned from the given [script], passing [archive]
// as the parameter
// -----------------------------------------------------------------------------
bool lua::runArchiveScript(const string& script, Archive* archive)
{
	return runEditorScript<Archive*>(script, archive);
}

// -----------------------------------------------------------------------------
// Runs the function returned from the given [script], passing [entries]
// as the parameter
// -----------------------------------------------------------------------------
bool lua::runEntryScript(const string& script, vector<ArchiveEntry*>& entries)
{
	return runEditorScript<vector<ArchiveEntry*>&>(script, entries);
}

// -----------------------------------------------------------------------------
// Runs the function returned from the given [script], passing [map] as the
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
	auto mem = lua_gc(lua::state(), LUA_GCCOUNT, 0);
	if (mem < 1024)
		log::console(fmt::format("Lua state using {} bytes memory", mem));
	else
		log::console(fmt::format("Lua state using {} memory", misc::sizeAsString(mem)));
}
