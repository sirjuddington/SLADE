
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Archive.cpp
// Description: Functions to export Game-related types and namespaces to lua
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
#include "Export.h"
#include "Game/Configuration.h"
#include "Game/ThingType.h"
#include "Scripting/LuaBridge.h"


// -----------------------------------------------------------------------------
//
// Scripting Namespace Functions
//
// -----------------------------------------------------------------------------
namespace slade::scripting
{
// -----------------------------------------------------------------------------
// Registers the ThingType type with lua
// -----------------------------------------------------------------------------
static void registerThingType(lua_State* lua)
{
	// Create ThingType type, no constructor
	auto lua_ttype = luabridge::getGlobalNamespace(lua).beginClass<game::ThingType>("ThingType");

	// Properties
	// -------------------------------------------------------------------------
	lua_ttype.addProperty("name", &game::ThingType::name);
	lua_ttype.addProperty("group", &game::ThingType::group);
	lua_ttype.addProperty("radius", &game::ThingType::radius);
	lua_ttype.addProperty("height", &game::ThingType::height);
	lua_ttype.addProperty("scaleY", &game::ThingType::scaleY);
	lua_ttype.addProperty("scaleX", &game::ThingType::scaleX);
	lua_ttype.addProperty("angled", &game::ThingType::angled);
	lua_ttype.addProperty("hanging", &game::ThingType::hanging);
	lua_ttype.addProperty("fullbright", &game::ThingType::fullbright);
	lua_ttype.addProperty("decoration", &game::ThingType::decoration);
	lua_ttype.addProperty("solid", &game::ThingType::solid);
	lua_ttype.addProperty("sprite", &game::ThingType::sprite);
	lua_ttype.addProperty("icon", &game::ThingType::icon);
	lua_ttype.addProperty("translation", &game::ThingType::translation);
	lua_ttype.addProperty("palette", &game::ThingType::palette);

	// TODO: lua_ttype["tagged"] = sol::property(&Game::ThingType::needsTag);
}

// -----------------------------------------------------------------------------
// Registers the Game namespace with lua
// -----------------------------------------------------------------------------
void registerGameNamespace(lua_State* lua)
{
	auto game = luabridge::getGlobalNamespace(lua).beginNamespace("Game");
	game.addFunction("ThingType", [](int type) { return game::configuration().thingType(type); });
}

// -----------------------------------------------------------------------------
// Registers various Game-related types with lua
// -----------------------------------------------------------------------------
void registerGameTypes(lua_State* lua)
{
	registerThingType(lua);
}

} // namespace slade::scripting
