
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Archive.cpp
// Description: Functions to export Map Editor-related types and namespaces to
//              lua using sol3
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
#include "MapEditor/MapEditor.h"
#include "Export.h"
#include "MapEditor/Item.h"
#include "MapEditor/ItemSelection.h"
#include "MapEditor/MapEditContext.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapSide.h"
#include "SLADEMap/MapObject/MapThing.h"
#include "SLADEMap/MapObject/MapVertex.h"
#include "SLADEMap/MapObjectList/LineList.h"
#include "SLADEMap/MapObjectList/SectorList.h"
#include "SLADEMap/MapObjectList/SideList.h"
#include "SLADEMap/MapObjectList/ThingList.h"
#include "SLADEMap/MapObjectList/VertexList.h"
#include "SLADEMap/SLADEMap.h"
#include "thirdparty/sol/sol.hpp"

using namespace slade;
using namespace mapeditor;

// ReSharper disable CppParameterMayBeConstPtrOrRef


// -----------------------------------------------------------------------------
//
// Lua Namespace Functions
//
// -----------------------------------------------------------------------------
namespace slade::lua
{
// -----------------------------------------------------------------------------
// Registers the Map type with lua
// -----------------------------------------------------------------------------
void registerSLADEMap(sol::state& lua)
{
	// Create ArchiveFormat type, no constructor
	auto lua_map = lua.new_usertype<SLADEMap>("Map", "new", sol::no_constructor);

	// Properties
	// -------------------------------------------------------------------------
	lua_map.set("name", sol::property(&SLADEMap::mapName));
	lua_map.set("udmfNamespace", sol::property(&SLADEMap::udmfNamespace));
	lua_map.set("vertices", sol::property([](SLADEMap& self) { return self.vertices().all(); }));
	lua_map.set("linedefs", sol::property([](SLADEMap& self) { return self.lines().all(); }));
	lua_map.set("sidedefs", sol::property([](SLADEMap& self) { return self.sides().all(); }));
	lua_map.set("sectors", sol::property([](SLADEMap& self) { return self.sectors().all(); }));
	lua_map.set("things", sol::property([](SLADEMap& self) { return self.things().all(); }));
}

// -----------------------------------------------------------------------------
// Selects or deselects [object] in the map editor [self]
// -----------------------------------------------------------------------------
void selectMapObject(MapEditContext& self, MapObject* object, bool select)
{
	if (object)
		self.selection().select({ static_cast<int>(object->index()), mapeditor::itemTypeFromObject(object) }, select);
}

// -----------------------------------------------------------------------------
// Sets the map editor [mode] in the map editor [self]
// -----------------------------------------------------------------------------
void setEditMode(MapEditContext& self, Mode mode, SectorMode sector_mode = SectorMode::Both)
{
	self.setEditMode(mode);
	if (mode == Mode::Sectors)
		self.setSectorEditMode(sector_mode);
}

// -----------------------------------------------------------------------------
// Registers the MapEditor type with lua
// -----------------------------------------------------------------------------
void registerMapEditor(sol::state& lua)
{
	// Create MapEditor type, no constructor
	auto lua_mapeditor = lua.new_usertype<MapEditContext>("MapEditor", "new", sol::no_constructor);

	// Properties
	// -------------------------------------------------------------------------
	lua_mapeditor.set("editMode", sol::property(&MapEditContext::editMode));
	lua_mapeditor.set("sectorEditMode", sol::property(&MapEditContext::sectorEditMode));
	lua_mapeditor.set("gridSize", sol::property(&MapEditContext::gridSize));
	lua_mapeditor.set("map", sol::property(&MapEditContext::map));

	// Constants
	// -------------------------------------------------------------------------
	lua_mapeditor.set("MODE_VERTICES", sol::property([]() { return Mode::Vertices; }));
	lua_mapeditor.set("MODE_LINES", sol::property([]() { return Mode::Lines; }));
	lua_mapeditor.set("MODE_SECTORS", sol::property([]() { return Mode::Sectors; }));
	lua_mapeditor.set("MODE_THINGS", sol::property([]() { return Mode::Things; }));
	lua_mapeditor.set("MODE_VISUAL", sol::property([]() { return Mode::Visual; }));
	lua_mapeditor.set("SECTORMODE_BOTH", sol::property([]() { return SectorMode::Both; }));
	lua_mapeditor.set("SECTORMODE_FLOOR", sol::property([]() { return SectorMode::Floor; }));
	lua_mapeditor.set("SECTORMODE_CEILING", sol::property([]() { return SectorMode::Ceiling; }));

	// Functions
	// -------------------------------------------------------------------------
	lua_mapeditor.set_function(
		"SelectedVertices",
		sol::overload(
			[](MapEditContext& self, bool try_hilight) { return self.selection().selectedVertices(try_hilight); },
			[](MapEditContext& self) { return self.selection().selectedVertices(false); }));
	lua_mapeditor.set_function(
		"SelectedLines",
		sol::overload(
			[](MapEditContext& self, bool try_hilight) { return self.selection().selectedLines(try_hilight); },
			[](MapEditContext& self) { return self.selection().selectedLines(false); }));
	lua_mapeditor.set_function(
		"SelectedSectors",
		sol::overload(
			[](MapEditContext& self, bool try_hilight) { return self.selection().selectedSectors(try_hilight); },
			[](MapEditContext& self) { return self.selection().selectedSectors(false); }));
	lua_mapeditor.set_function(
		"SelectedThings",
		sol::overload(
			[](MapEditContext& self, bool try_hilight) { return self.selection().selectedThings(try_hilight); },
			[](MapEditContext& self) { return self.selection().selectedThings(false); }));
	lua_mapeditor.set_function("ClearSelection", [](MapEditContext& self) { self.selection().clear(); });
	lua_mapeditor.set_function(
		"Select",
		sol::overload(
			&selectMapObject, [](MapEditContext& self, MapObject* object) { selectMapObject(self, object, true); }));
	lua_mapeditor.set_function(
		"SetEditMode",
		sol::overload(
			[](MapEditContext& self, Mode mode) { setEditMode(self, mode); },
			[](MapEditContext& self, Mode mode, SectorMode sector_mode) { setEditMode(self, mode, sector_mode); }));
}

// -----------------------------------------------------------------------------
// Registers various MapEditor-related types with lua
// -----------------------------------------------------------------------------
void registerMapEditorTypes(sol::state& lua)
{
	registerMapEditor(lua);
	registerSLADEMap(lua);
	registerMapObject(lua);
	registerMapVertex(lua);
	registerMapLine(lua);
	registerMapSide(lua);
	registerMapSector(lua);
	registerMapThing(lua);
}

} // namespace slade::lua
