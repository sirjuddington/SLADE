
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
#include "Game/Configuration.h"
#include "Game/ThingType.h"
#include "thirdparty/sol/sol.hpp"


// -----------------------------------------------------------------------------
//
// Lua Namespace Functions
//
// -----------------------------------------------------------------------------
namespace slade::lua
{
// -----------------------------------------------------------------------------
// Registers the ThingType type with lua
// -----------------------------------------------------------------------------
void registerThingType(sol::state& lua)
{
	// Create ThingType type, no constructor
	auto lua_ttype = lua.new_usertype<game::ThingType>("ThingType", "new", sol::no_constructor);

	// Properties
	// -------------------------------------------------------------------------
	lua_ttype.set("name", sol::property(&game::ThingType::name));
	lua_ttype.set("group", sol::property(&game::ThingType::group));
	lua_ttype.set("radius", sol::property(&game::ThingType::radius));
	lua_ttype.set("height", sol::property(&game::ThingType::height));
	lua_ttype.set("scaleY", sol::property(&game::ThingType::scaleY));
	lua_ttype.set("scaleX", sol::property(&game::ThingType::scaleX));
	lua_ttype.set("angled", sol::property(&game::ThingType::angled));
	lua_ttype.set("hanging", sol::property(&game::ThingType::hanging));
	lua_ttype.set("fullbright", sol::property(&game::ThingType::fullbright));
	lua_ttype.set("decoration", sol::property(&game::ThingType::decoration));
	lua_ttype.set("solid", sol::property(&game::ThingType::solid));
	lua_ttype.set("sprite", sol::property(&game::ThingType::sprite));
	lua_ttype.set("icon", sol::property(&game::ThingType::icon));
	lua_ttype.set("translation", sol::property(&game::ThingType::translation));
	lua_ttype.set("palette", sol::property(&game::ThingType::palette));

	// TODO: lua_ttype["tagged"] = sol::property(&Game::ThingType::needsTag);
}

// -----------------------------------------------------------------------------
// Registers the Game namespace with lua
// -----------------------------------------------------------------------------
void registerGameNamespace(sol::state& lua)
{
	auto game = lua.create_table("Game");
	game.set_function("ThingType", [](int type) { return game::configuration().thingType(type); });
}

// -----------------------------------------------------------------------------
// Registers various Game-related types with lua
// -----------------------------------------------------------------------------
void registerGameTypes(sol::state& lua)
{
	registerThingType(lua);
}

} // namespace slade::lua
