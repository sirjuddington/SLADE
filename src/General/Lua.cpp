
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Lua.cpp
// Description: Lua scripting system (nothing much here yet)
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
#include "Lua.h"
#include "External/sol/sol.hpp"
#include "General/Console/Console.h"


// ----------------------------------------------------------------------------
//
// Variables
//
// ----------------------------------------------------------------------------
namespace Lua
{
	sol::state	lua;
}


// ----------------------------------------------------------------------------
//
// Lua Namespace Functions
//
// ----------------------------------------------------------------------------
namespace Lua
{
	// --- Functions ---

	// log_message: Prints a log message (concatenates args)
	void logMessage(const char* message)
	{
		Log::message(Log::MessageType::Script, message);
	}
}

// ----------------------------------------------------------------------------
// Lua::init
//
// Initialises lua and registers functions
// ----------------------------------------------------------------------------
bool Lua::init()
{
	// Register functions
	lua.set_function("logMessage", &logMessage);

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
	return lua.script(CHR(program)).valid();
}

// ----------------------------------------------------------------------------
// Lua::runFile
//
// Runs a lua script from a text file [filename]
// ----------------------------------------------------------------------------
bool Lua::runFile(string filename)
{
	return lua.script_file(CHR(filename)).valid();
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
