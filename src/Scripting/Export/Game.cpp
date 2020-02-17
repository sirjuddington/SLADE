
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
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
namespace Lua
{
// -----------------------------------------------------------------------------
// Registers the ThingType type with lua
// -----------------------------------------------------------------------------
void registerThingType(sol::state& lua)
{
	// Create ThingType type, no constructor
	auto lua_ttype = lua.new_usertype<Game::ThingType>("ThingType", "new", sol::no_constructor);

	// Properties
	// -------------------------------------------------------------------------
	lua_ttype["name"]        = sol::property(&Game::ThingType::name);
	lua_ttype["group"]       = sol::property(&Game::ThingType::group);
	lua_ttype["radius"]      = sol::property(&Game::ThingType::radius);
	lua_ttype["height"]      = sol::property(&Game::ThingType::height);
	lua_ttype["scaleY"]      = sol::property(&Game::ThingType::scaleY);
	lua_ttype["scaleX"]      = sol::property(&Game::ThingType::scaleX);
	lua_ttype["angled"]      = sol::property(&Game::ThingType::angled);
	lua_ttype["hanging"]     = sol::property(&Game::ThingType::hanging);
	lua_ttype["fullbright"]  = sol::property(&Game::ThingType::fullbright);
	lua_ttype["decoration"]  = sol::property(&Game::ThingType::decoration);
	lua_ttype["solid"]       = sol::property(&Game::ThingType::solid);
	lua_ttype["sprite"]      = sol::property(&Game::ThingType::sprite);
	lua_ttype["icon"]        = sol::property(&Game::ThingType::icon);
	lua_ttype["translation"] = sol::property(&Game::ThingType::translation);
	lua_ttype["palette"]     = sol::property(&Game::ThingType::palette);

	// TODO: lua_ttype["tagged"] = sol::property(&Game::ThingType::needsTag);
}

// -----------------------------------------------------------------------------
// Registers the Game namespace with lua
// -----------------------------------------------------------------------------
void registerGameNamespace(sol::state& lua)
{
	auto game         = lua.create_table("Game");
	game["ThingType"] = [](int type) { return Game::configuration().thingType(type); };
}

// -----------------------------------------------------------------------------
// Registers various Game-related types with lua
// -----------------------------------------------------------------------------
void registerGameTypes(sol::state& lua)
{
	registerThingType(lua);
}

} // namespace Lua
