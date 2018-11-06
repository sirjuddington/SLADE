
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
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
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#define SOL_CHECK_ARGUMENTS 1
#include "Archive/ArchiveManager.h"
#include "Archive/Formats/All.h"
#include "Dialogs/ExtMessageDialog.h"
#include "External/sol/sol.hpp"
#include "Game/Configuration.h"
#include "Game/ThingType.h"
#include "General/Console/Console.h"
#include "General/Misc.h"
#include "General/UI.h"
#include "Lua.h"
#include "MainEditor/MainEditor.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/SLADEMap/SLADEMap.h"
#include "Utility/SFileDialog.h"


// ----------------------------------------------------------------------------
//
// Variables
//
// ----------------------------------------------------------------------------
namespace Lua
{
	sol::state	lua;
	wxWindow*	current_window = nullptr;
	Error		script_error;
	time_t		script_start_time;
}


// ----------------------------------------------------------------------------
// wxString support for Sol
//
// See: https://github.com/ThePhD/sol2/issues/140
// ----------------------------------------------------------------------------
namespace sol
{
	namespace stack
	{

		template<>
		struct pusher<wxString>
		{
			static int push(lua_State* L, const wxString& str)
			{
				//return stack::push(L, CHR(str));
				return stack::push(L, static_cast<const char*>(str.ToAscii()));
			}
		};

		template<>
		struct getter<wxString>
		{
			static wxString get(lua_State *L, int index, record &tracking)
			{
				tracking.use(1);
				const char* luastr = stack::get<const char *>(L, index);
				return wxString::FromUTF8(luastr);
			}
		};

	} // namespace stack

	template<>
	struct lua_type_of<wxString> : std::integral_constant<type, type::string> {};

} // namespace sol


// ----------------------------------------------------------------------------
//
// Lua Namespace Functions
//
// ----------------------------------------------------------------------------
namespace Lua
{
#include "Export/Archive.h"
#include "Export/Game.h"
#include "Export/General.h"
#include "Export/MapEditor.h"

// ----------------------------------------------------------------------------
// Lua::resetError
//
// Resets error information
// ----------------------------------------------------------------------------
void resetError()
{
	script_error.type = "No";
	script_error.message = "No error(s) occurred";
	script_error.line_no = 0;
}

// ----------------------------------------------------------------------------
// Lua::processError
//
// Processes error information from [result]
// ----------------------------------------------------------------------------
void processError(sol::protected_function_result& result)
{
	// Error Type
	script_error.type = sol::to_string(result.status());
	script_error.type = script_error.type.MakeCapitalized();

	// Error Message
	sol::error error = result;
	script_error.message = error.what();
	script_error.message = script_error.message.Right(
		script_error.message.size() - script_error.message.Find("]:") - 2
	);

	// Line No.
	auto split = wxSplit(script_error.message, ':');
	if (split.size() > 0)
	{
		long tmp;
		if (split[0].ToLong(&tmp))
			script_error.line_no = tmp;
	}

	script_error.message = script_error.message.Right(
		script_error.message.size() - script_error.message.Find(": ") - 2
	);
}

// ----------------------------------------------------------------------------
// Lua::runEditorScript
//
// Template function for Lua::run*Script functions.
// Loads [script] and runs the 'execute' function in the script, passing
// [param] to the function
// ----------------------------------------------------------------------------
template<class T>
bool runEditorScript(const string& script, T param)
{
	resetError();
	script_start_time = wxDateTime::Now().GetTicks();

	// Load script
	sol::environment sandbox(lua, sol::create, lua.globals());
	auto load_result = lua.script(CHR(script), sandbox, sol::simple_on_error);
	if (!load_result.valid())
	{
		processError(load_result);
		Log::error(S_FMT(
			"%s Error running Lua script: %d: %s",
			CHR(script_error.type),
			script_error.line_no,
			CHR(script_error.message)
		));
		return false;
	}

	// Run script execute function
	sol::protected_function func = sandbox["execute"];
	auto exec_result = func(param);
	if (!exec_result.valid())
	{
		processError(exec_result);
		Log::error(S_FMT(
			"%s Error running Lua script: %d: %s",
			CHR(script_error.type),
			script_error.line_no,
			CHR(script_error.message)
		));
		return false;
	}

	return true;
}

} // namespace Lua

// ----------------------------------------------------------------------------
// Lua::init
//
// Initialises lua and registers functions
// ----------------------------------------------------------------------------
bool Lua::init()
{
	lua.open_libraries(sol::lib::base, sol::lib::string);

	// Register namespaces
	registerAppNamespace(lua);
	registerSplashWindowNamespace(lua);
	registerGameNamespace(lua);
	registerArchivesNamespace(lua);

	// Register types
	registerMiscTypes(lua);
	registerArchiveTypes(lua);
	registerMapEditorTypes(lua);
	registerGameTypes(lua);

	return true;
}

// ----------------------------------------------------------------------------
// Lua::close
//
// Close the lua state
// ----------------------------------------------------------------------------
void Lua::close()
{
}

// ----------------------------------------------------------------------------
// Lua::error
//
// Returns information about the last script error that occurred
// ----------------------------------------------------------------------------
Lua::Error& Lua::error()
{
	return script_error;
}

// ----------------------------------------------------------------------------
// Lua::showErrorDialog
//
// Shows an extended message dialog with details of the last script error that
// occurred
// ----------------------------------------------------------------------------
void Lua::showErrorDialog(wxWindow* parent, const string& title, const string& message)
{
	// Get script log messages since the last script was started
	auto log = Log::since(script_start_time, Log::MessageType::Script);
	string output;
	for (auto msg : log)
		output += msg->formattedMessageLine() + "\n";

	ExtMessageDialog dlg(parent ? parent : current_window, title);
	dlg.setMessage(message);
	dlg.setExt(S_FMT(
		"%s Error\nLine %d: %s\n\nScript Output:\n%s",
		CHR(Lua::error().type),
		Lua::error().line_no,
		CHR(Lua::error().message),
		CHR(output)
	));
	dlg.CenterOnParent();
	dlg.ShowModal();
}

// ----------------------------------------------------------------------------
// Lua::run
//
// Runs a lua script [program]
// ----------------------------------------------------------------------------
bool Lua::run(string program)
{
	resetError();
	script_start_time = wxDateTime::Now().GetTicks();

	sol::environment sandbox(lua, sol::create, lua.globals());
	auto result = lua.script(CHR(program), sandbox, sol::simple_on_error);
	lua.collect_garbage();

	if (!result.valid())
	{
		processError(result);
		Log::error(S_FMT(
			"%s Error running Lua script: %d: %s",
			CHR(script_error.type),
			script_error.line_no,
			CHR(script_error.message)
		));
		return false;
	}
	
	return true;
}

// ----------------------------------------------------------------------------
// Lua::runFile
//
// Runs a lua script from a text file [filename]
// ----------------------------------------------------------------------------
bool Lua::runFile(string filename)
{
	resetError();
	script_start_time = wxDateTime::Now().GetTicks();

	sol::environment sandbox(lua, sol::create, lua.globals());
	auto result = lua.script_file(CHR(filename), sandbox, sol::simple_on_error);
	lua.collect_garbage();

	if (!result.valid())
	{
		processError(result);
		Log::error(S_FMT(
			"%s Error running Lua script: %d: %s",
			CHR(script_error.type),
			script_error.line_no,
			CHR(script_error.message)
		));
		return false;
	}

	return true;
}

// ----------------------------------------------------------------------------
// Lua::runArchiveScript
//
// Runs the 'execute(archive)' function in the given [script], passing
// [archive] as the parameter
// ----------------------------------------------------------------------------
bool Lua::runArchiveScript(const string& script, Archive* archive)
{
	return runEditorScript<Archive*>(script, archive);
}

// ----------------------------------------------------------------------------
// Lua::runEntryScript
//
// Runs the 'execute(entries)' function in the given [script], passing
// [entries] as the parameter
// ----------------------------------------------------------------------------
bool Lua::runEntryScript(const string& script, vector<ArchiveEntry*> entries)
{
	return runEditorScript<vector<ArchiveEntry*>>(script, entries);
}

// ----------------------------------------------------------------------------
// Lua::runMapScript
//
// Runs the 'execute(map)' function in the given [script], passing [map] as the
// parameter
// ----------------------------------------------------------------------------
bool Lua::runMapScript(const string& script, SLADEMap* map)
{
	return runEditorScript<SLADEMap*>(script, map);
}

// ----------------------------------------------------------------------------
// Lua::state
//
// Returns the active lua state
// ----------------------------------------------------------------------------
sol::state& Lua::state()
{
	return lua;
}

// ----------------------------------------------------------------------------
// Lua::currentWindow
//
// Returns the current window (used as the parent window for UI-related
// scripting functions such as messageBox)
// ----------------------------------------------------------------------------
wxWindow* Lua::currentWindow()
{
	return current_window;
}

// ----------------------------------------------------------------------------
// Lua::currentWindow
//
// Sets the current [window] (used as the parent window for UI-related
// scripting functions such as messageBox)
// ----------------------------------------------------------------------------
void Lua::setCurrentWindow(wxWindow* window)
{
	current_window = window;
}


// ----------------------------------------------------------------------------
//
// Console Commands
//
// ----------------------------------------------------------------------------

CONSOLE_COMMAND(script, 1, true)
{
	string script = args[0];
	for (unsigned a = 1; a < args.size(); a++)
		script += " " + args[a];

	Lua::run(script);
}

CONSOLE_COMMAND(script_file, 1, true)
{
	if (!wxFile::Exists(args[0]))
	{
		LOG_MESSAGE(1, "File \"%s\" does not exist", args[0]);
		return;
	}

	if (!Lua::runFile(args[0]))
		LOG_MESSAGE(1, "Error loading lua script file \"%s\"", args[0]);
}

CONSOLE_COMMAND(lua_mem, 0, false)
{
	auto mem = Lua::state().memory_used();
	Log::console(S_FMT("Lua state using %s memory", CHR(Misc::sizeAsString(mem))));
}
