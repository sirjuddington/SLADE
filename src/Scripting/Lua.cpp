
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
#include "Archive/ArchiveManager.h"
#include "Archive/Formats/All.h"
#include "Game/Configuration.h"
#include "Game/ThingType.h"
#include "General/Console/Console.h"
#include "General/Misc.h"
#include "General/UI.h"
#include "Lua.h"
#include "MainEditor/MainEditor.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/SLADEMap/SLADEMap.h"
#include "SolUtil.h"
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
}


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
}

// ----------------------------------------------------------------------------
// Lua::init
//
// Initialises lua and registers functions
// ----------------------------------------------------------------------------
bool Lua::init()
{
	lua.open_libraries(sol::lib::base, sol::lib::string);

	// Register namespaces
	lua.create_named_table("slade");
	registerSLADENamespace(lua);
	registerSplashWindowNamespace(lua);
	registerGameNamespace(lua);
	registerArchivesNamespace(lua);
	registerMapEditorNamespace(lua);

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
// Lua::run
//
// Runs a lua script [program]
// ----------------------------------------------------------------------------
bool Lua::run(string program)
{
	auto result = lua.script(CHR(program), sol::simple_on_error);

	if (!result.valid())
	{
		sol::error error = result;
		string error_type = sol::to_string(result.status());
		string error_message = error.what();
		Log::error(S_FMT(
			"%s Error running Lua script: %s",
			CHR(error_type.MakeCapitalized()),
			CHR(error_message)
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
	auto result = lua.script_file(CHR(filename), sol::simple_on_error);

	if (!result.valid())
	{
		sol::error error = result;
		string error_type = sol::to_string(result.status());
		string error_message = error.what();
		Log::error(S_FMT(
			"%s Error running Lua script: %s",
			CHR(error_type.MakeCapitalized()),
			CHR(error_message)
		));
		return false;
	}

	return true;
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
