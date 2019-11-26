
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
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
#define SOL_CHECK_ARGUMENTS 1
#include "Dialogs/ExtMessageDialog.h"
#include "Export/Export.h"
#include "General/Console/Console.h"
#include "General/Misc.h"
#include "Lua.h"
#include "SLADEMap/SLADEMap.h"
#include "UI/WxUtils.h"
#include "Utility/StringUtils.h"
#include "thirdparty/sol/sol.hpp"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace Lua
{
sol::state lua;
wxWindow*  current_window = nullptr;
Error      script_error;
time_t     script_start_time;
} // namespace Lua


// -----------------------------------------------------------------------------
//
// Lua Namespace Functions
//
// -----------------------------------------------------------------------------
namespace Lua
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
// Processes error information from [result]
// -----------------------------------------------------------------------------
void processError(const sol::protected_function_result& result)
{
	// Error Type
	script_error.type = sol::to_string(result.status());
	StrUtil::capitalizeIP(script_error.type);

	// Error Message
	sol::error  error     = result;
	string_view error_msg = error.what();

	// Line No.
	auto pos             = error_msg.find("]:");
	auto pos_ln_end      = error_msg.find(':', pos + 2);
	script_error.line_no = StrUtil::asInt(error_msg.substr(pos + 2, pos_ln_end - pos + 2));

	// Actual error message
	script_error.message = error_msg.substr(pos_ln_end + 2);
}

sol::protected_function_result handleError(lua_State* L, sol::protected_function_result pfr)
{
	processError(pfr);
	Log::error("{} Error running Lua script: {}: {}", script_error.type, script_error.line_no, script_error.message);

	return pfr;
}

// -----------------------------------------------------------------------------
// Template function for Lua::run*Script functions.
// Loads [script] and runs the 'Execute' function in the script, passing
// [param] to the function
// -----------------------------------------------------------------------------
template<class T> bool runEditorScript(const string& script, T param)
{
	resetError();
	script_start_time = wxDateTime::Now().GetTicks();

	// Load script
	sol::environment sandbox(lua, sol::create, lua.globals());
	auto             load_result = lua.script(script, sandbox, handleError);
	if (!load_result.valid())
	{
		processError(load_result);
		Log::error(
			"{} Error running Lua script: {}: {}", script_error.type, script_error.line_no, script_error.message);
		return false;
	}

	// Run script execute function
	// sol::protected_function func        = sandbox["execute"];
	// auto                    exec_result = func(param);
	sol::protected_function        func(sandbox["Execute"]);
	sol::protected_function_result exec_result = func(param);
	if (!exec_result.valid())
	{
		sol::error error = exec_result;
		processError(exec_result);
		Log::error(
			"{} Error running Lua script: {}: {}", script_error.type, script_error.line_no, script_error.message);
		return false;
	}

	return true;
}

} // namespace Lua

// -----------------------------------------------------------------------------
// Initialises lua and registers functions
// -----------------------------------------------------------------------------
bool Lua::init()
{
	lua.open_libraries(sol::lib::base, sol::lib::string, sol::lib::math, sol::lib::table);

	// Register namespaces
	registerAppNamespace(lua);
	registerUINamespace(lua);
	registerGameNamespace(lua);
	registerArchivesNamespace(lua);

	// Register types
	registerMiscTypes(lua);
	registerArchiveTypes(lua);
	registerMapEditorTypes(lua);
	registerGameTypes(lua);
	registerGraphicsTypes(lua);

	return true;
}

// -----------------------------------------------------------------------------
// Close the lua state
// -----------------------------------------------------------------------------
void Lua::close() {}

// -----------------------------------------------------------------------------
// Returns information about the last script error that occurred
// -----------------------------------------------------------------------------
Lua::Error& Lua::error()
{
	return script_error;
}

// -----------------------------------------------------------------------------
// Shows an extended message dialog with details of the last script error that
// occurred
// -----------------------------------------------------------------------------
void Lua::showErrorDialog(wxWindow* parent, string_view title, string_view message)
{
	// Get script log messages since the last script was started
	auto   log = Log::since(script_start_time, Log::MessageType::Script);
	string output;
	for (auto msg : log)
		output += msg->formattedMessageLine() + "\n";

	ExtMessageDialog dlg(parent ? parent : current_window, WxUtils::strFromView(title));
	dlg.setMessage(WxUtils::strFromView(message));
	dlg.setExt(wxString::Format(
		"%s Error\nLine %d: %s\n\nScript Output:\n%s",
		Lua::error().type,
		Lua::error().line_no,
		Lua::error().message,
		output));
	dlg.CenterOnParent();
	dlg.ShowModal();
}

// -----------------------------------------------------------------------------
// Runs a lua script [program]
// -----------------------------------------------------------------------------
bool Lua::run(const string& program)
{
	resetError();
	script_start_time = wxDateTime::Now().GetTicks();

	sol::environment sandbox(lua, sol::create, lua.globals());
	auto             result = lua.script(program, sandbox, handleError);
	lua.collect_garbage();

	if (!result.valid())
	{
		processError(result);
		Log::error(
			"{} Error running Lua script: {}: {}", script_error.type, script_error.line_no, script_error.message);
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------
// Runs a lua script from a text file [filename]
// -----------------------------------------------------------------------------
bool Lua::runFile(const string& filename)
{
	resetError();
	script_start_time = wxDateTime::Now().GetTicks();

	sol::environment sandbox(lua, sol::create, lua.globals());
	auto             result = lua.script_file(filename, sandbox, handleError);
	lua.collect_garbage();

	if (!result.valid())
	{
		processError(result);
		Log::error(
			"{} Error running Lua script: {}: {}", script_error.type, script_error.line_no, script_error.message);
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------
// Runs the 'Execute(archive)' function in the given [script], passing [archive]
// as the parameter
// -----------------------------------------------------------------------------
bool Lua::runArchiveScript(const string& script, Archive* archive)
{
	return runEditorScript<Archive*>(script, archive);
}

// -----------------------------------------------------------------------------
// Runs the 'Execute(entries)' function in the given [script], passing [entries]
// as the parameter
// -----------------------------------------------------------------------------
bool Lua::runEntryScript(const string& script, vector<ArchiveEntry*>& entries)
{
	return runEditorScript<vector<ArchiveEntry*>&>(script, entries);
}

// -----------------------------------------------------------------------------
// Runs the 'Execute(map)' function in the given [script], passing [map] as the
// parameter
// -----------------------------------------------------------------------------
bool Lua::runMapScript(const string& script, SLADEMap* map)
{
	return runEditorScript<SLADEMap*>(script, map);
}

// -----------------------------------------------------------------------------
// Returns the active lua state
// -----------------------------------------------------------------------------
sol::state& Lua::state()
{
	return lua;
}

// -----------------------------------------------------------------------------
// Returns the current window (used as the parent window for UI-related
// scripting functions such as messageBox)
// -----------------------------------------------------------------------------
wxWindow* Lua::currentWindow()
{
	return current_window;
}

// -----------------------------------------------------------------------------
// Sets the current [window] (used as the parent window for UI-related scripting
// functions such as messageBox)
// -----------------------------------------------------------------------------
void Lua::setCurrentWindow(wxWindow* window)
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
	auto script = args[0];
	for (unsigned a = 1; a < args.size(); a++)
		script += " " + args[a];

	Lua::run(script);
}

CONSOLE_COMMAND(script_file, 1, true)
{
	if (!wxFile::Exists(args[0]))
	{
		Log::error("File \"{}\" does not exist", args[0]);
		return;
	}

	if (!Lua::runFile(args[0]))
		Log::error("Error loading lua script file \"{}\"", args[0]);
}

CONSOLE_COMMAND(lua_mem, 0, false)
{
	auto mem = Lua::state().memory_used();
	Log::console(fmt::format("Lua state using {} memory", Misc::sizeAsString(mem)));
}
