
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Archive.cpp
// Description: Functions to export general/misc. types and namespaces to lua
//              using sol3
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
#include "Archive/Archive.h"
#include "MainEditor/MainEditor.h"
#include "MapEditor/MapEditContext.h"
#include "thirdparty/sol/sol.hpp"


// -----------------------------------------------------------------------------
//
// Lua Namespace Functions
//
// -----------------------------------------------------------------------------
namespace Lua
{
// -----------------------------------------------------------------------------
// Writes a log [message]
// -----------------------------------------------------------------------------
void logMessage(const char* message)
{
	Log::message(Log::MessageType::Script, message);
}

// -----------------------------------------------------------------------------
// Switches to the tab for [archive], opening it if necessary
// -----------------------------------------------------------------------------
bool showArchive(Archive* archive)
{
	if (!archive)
		return false;

	MainEditor::openArchiveTab(archive);
	return true;
}

// -----------------------------------------------------------------------------
// Registers some misc. types with lua
// -----------------------------------------------------------------------------
void registerMiscTypes(sol::state& lua)
{
	// Point type
	auto lua_vec2f = lua.new_usertype<Vec2f>("Point", sol::constructors<Vec2f(), Vec2f(double, double)>());
	lua_vec2f["x"] = &Vec2f::x;
	lua_vec2f["y"] = &Vec2f::y;

	// Colour type
	auto lua_colour = lua.new_usertype<ColRGBA>(
		"Colour",
		sol::
			constructors<ColRGBA(), ColRGBA(uint8_t, uint8_t, uint8_t), ColRGBA(uint8_t, uint8_t, uint8_t, uint8_t)>());
	lua_colour["r"] = &ColRGBA::r;
	lua_colour["g"] = &ColRGBA::g;
	lua_colour["b"] = &ColRGBA::b;
	lua_colour["a"] = &ColRGBA::a;

	// Plane type
	auto lua_plane = lua.new_usertype<Plane>(
		"Plane", sol::constructors<Plane(), Plane(double, double, double, double)>());
	lua_plane["a"]        = &Plane::a;
	lua_plane["b"]        = &Plane::b;
	lua_plane["c"]        = &Plane::c;
	lua_plane["d"]        = &Plane::d;
	lua_plane["HeightAt"] = sol::resolve<double(Vec2d) const>(&Plane::heightAt);
}

// -----------------------------------------------------------------------------
// Registers the App namespace with lua
// -----------------------------------------------------------------------------
void registerAppNamespace(sol::state& lua)
{
	auto app = lua.create_named_table("App");

	// Functions
	// -------------------------------------------------------------------------
	app["LogMessage"] = &logMessage;
	app["CurrentArchive"]        = &MainEditor::currentArchive;
	app["CurrentEntry"]          = &MainEditor::currentEntry;
	app["CurrentEntrySelection"] = &MainEditor::currentEntrySelection;
	app["ShowArchive"]           = &showArchive;
	app["ShowEntry"]             = &MainEditor::openEntry;
	app["MapEditor"]             = &MapEditor::editContext;
}

} // namespace Lua
