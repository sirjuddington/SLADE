
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
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
#include "Game/Configuration.h"
#include "MapEditor/MapEditContext.h"
#include "SLADEMap/MapObject/MapObject.h"
#include "Scripting/Lua.h"
#include "thirdparty/sol/sol.hpp"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Lua Namespace Functions
//
// -----------------------------------------------------------------------------
namespace slade::lua
{
// -----------------------------------------------------------------------------
// Sets a boolean property [key] on the MapObject [self] to [value].
// Also checks if the property [key] is allowed to be modified by scripts
// -----------------------------------------------------------------------------
void objectSetBoolProperty(MapObject& self, string_view key, bool value)
{
	if (self.scriptCanModifyProp(key))
		self.setBoolProperty(key, value);
	else
		log::warning("{} boolean property \"{}\" can not be modified via script", self.typeName(), key);
}

// -----------------------------------------------------------------------------
// Sets an int property [key] on the MapObject [self] to [value].
// Also checks if the property [key] is allowed to be modified by scripts
// -----------------------------------------------------------------------------
void objectSetIntProperty(MapObject& self, string_view key, int value)
{
	if (self.scriptCanModifyProp(key))
		self.setIntProperty(key, value);
	else
		log::warning("{} integer property \"{}\" can not be modified via script", self.typeName(), key);
}

// -----------------------------------------------------------------------------
// Sets a float property [key] on the MapObject [self] to [value].
// Also checks if the property [key] is allowed to be modified by scripts
// -----------------------------------------------------------------------------
void objectSetFloatProperty(MapObject& self, string_view key, double value)
{
	if (self.scriptCanModifyProp(key))
		self.setFloatProperty(key, value);
	else
		log::warning("{} float property \"{}\" can not be modified via script", self.typeName(), key);
}

// -----------------------------------------------------------------------------
// Sets a string property [key] on the MapObject [self] to [value].
// Also checks if the property [key] is allowed to be modified by scripts
// -----------------------------------------------------------------------------
void objectSetStringProperty(MapObject& self, string_view key, string_view value)
{
	if (self.scriptCanModifyProp(key))
		self.setStringProperty(key, value);
	else
		log::warning("{} string property \"{}\" can not be modified via script", self.typeName(), key);
}

// -----------------------------------------------------------------------------
// Registers the Map type with lua
// -----------------------------------------------------------------------------
void registerSLADEMap(sol::state& lua)
{
	// Create ArchiveFormat type, no constructor
	auto lua_map = lua.new_usertype<SLADEMap>("Map", "new", sol::no_constructor);

	// Properties
	// -------------------------------------------------------------------------
	lua_map["name"]          = sol::property(&SLADEMap::mapName);
	lua_map["udmfNamespace"] = sol::property(&SLADEMap::udmfNamespace);
	lua_map["vertices"]      = sol::property([](SLADEMap& self) { return self.vertices().all(); });
	lua_map["linedefs"]      = sol::property([](SLADEMap& self) { return self.lines().all(); });
	lua_map["sidedefs"]      = sol::property([](SLADEMap& self) { return self.sides().all(); });
	lua_map["sectors"]       = sol::property([](SLADEMap& self) { return self.sectors().all(); });
	lua_map["things"]        = sol::property([](SLADEMap& self) { return self.things().all(); });
}

// -----------------------------------------------------------------------------
// Selects or deselects [object] in the map editor [self]
// -----------------------------------------------------------------------------
void selectMapObject(MapEditContext& self, MapObject* object, bool select)
{
	if (object)
		self.selection().select({ (int)object->index(), mapeditor::itemTypeFromObject(object) }, select);
}

// -----------------------------------------------------------------------------
// Sets the map editor [mode] in the map editor [self]
// -----------------------------------------------------------------------------
void setEditMode(
	MapEditContext&       self,
	mapeditor::Mode       mode,
	mapeditor::SectorMode sector_mode = mapeditor::SectorMode::Both)
{
	self.setEditMode(mode);
	if (mode == mapeditor::Mode::Sectors)
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
	lua_mapeditor["editMode"]       = sol::property(&MapEditContext::editMode);
	lua_mapeditor["sectorEditMode"] = sol::property(&MapEditContext::sectorEditMode);
	lua_mapeditor["gridSize"]       = sol::property(&MapEditContext::gridSize);
	lua_mapeditor["map"]            = sol::property(&MapEditContext::map);

	// Constants
	// -------------------------------------------------------------------------
	lua_mapeditor["MODE_VERTICES"]      = sol::property([]() { return mapeditor::Mode::Vertices; });
	lua_mapeditor["MODE_LINES"]         = sol::property([]() { return mapeditor::Mode::Lines; });
	lua_mapeditor["MODE_SECTORS"]       = sol::property([]() { return mapeditor::Mode::Sectors; });
	lua_mapeditor["MODE_THINGS"]        = sol::property([]() { return mapeditor::Mode::Things; });
	lua_mapeditor["MODE_VISUAL"]        = sol::property([]() { return mapeditor::Mode::Visual; });
	lua_mapeditor["SECTORMODE_BOTH"]    = sol::property([]() { return mapeditor::SectorMode::Both; });
	lua_mapeditor["SECTORMODE_FLOOR"]   = sol::property([]() { return mapeditor::SectorMode::Floor; });
	lua_mapeditor["SECTORMODE_CEILING"] = sol::property([]() { return mapeditor::SectorMode::Ceiling; });

	// Functions
	// -------------------------------------------------------------------------
	lua_mapeditor["SelectedVertices"] = sol::overload(
		[](MapEditContext& self, bool try_hilight) { return self.selection().selectedVertices(try_hilight); },
		[](MapEditContext& self) { return self.selection().selectedVertices(false); });
	lua_mapeditor["SelectedLines"] = sol::overload(
		[](MapEditContext& self, bool try_hilight) { return self.selection().selectedLines(try_hilight); },
		[](MapEditContext& self) { return self.selection().selectedLines(false); });
	lua_mapeditor["SelectedSectors"] = sol::overload(
		[](MapEditContext& self, bool try_hilight) { return self.selection().selectedSectors(try_hilight); },
		[](MapEditContext& self) { return self.selection().selectedSectors(false); });
	lua_mapeditor["SelectedThings"] = sol::overload(
		[](MapEditContext& self, bool try_hilight) { return self.selection().selectedThings(try_hilight); },
		[](MapEditContext& self) { return self.selection().selectedThings(false); });
	lua_mapeditor["ClearSelection"] = [](MapEditContext& self) { self.selection().clear(); };
	lua_mapeditor["Select"]         = sol::overload(
        &selectMapObject, [](MapEditContext& self, MapObject* object) { selectMapObject(self, object, true); });
	lua_mapeditor["SetEditMode"] = sol::overload(
		[](MapEditContext& self, mapeditor::Mode mode) { setEditMode(self, mode); },
		[](MapEditContext& self, mapeditor::Mode mode, mapeditor::SectorMode sector_mode)
		{ setEditMode(self, mode, sector_mode); });
}

// -----------------------------------------------------------------------------
// Registers the MapVertex type with lua
// -----------------------------------------------------------------------------
void registerMapVertex(sol::state& lua)
{
	// Create MapVertex type, MapObject base class, no constructor
	auto lua_vertex = lua.new_usertype<MapVertex>(
		"MapVertex", sol::base_classes, sol::bases<MapObject>(), "new", sol::no_constructor);

	// Properties
	// -------------------------------------------------------------------------
	lua_vertex["x"]              = &MapVertex::xPos;
	lua_vertex["y"]              = &MapVertex::yPos;
	lua_vertex["connectedLines"] = &MapVertex::connectedLines;
}

// -----------------------------------------------------------------------------
// Returns a table of needed textures for map line [self]
// -----------------------------------------------------------------------------
sol::table lineVisibleTextures(MapLine& self)
{
	auto needs_tex = self.needsTexture();
	return lua::state().create_table_with(
		"frontUpper",
		(needs_tex & MapLine::Part::FrontUpper) != 0,
		"frontMiddle",
		(needs_tex & MapLine::Part::FrontMiddle) != 0,
		"frontLower",
		(needs_tex & MapLine::Part::FrontLower) != 0,
		"backUpper",
		(needs_tex & MapLine::Part::BackUpper) != 0,
		"backMiddle",
		(needs_tex & MapLine::Part::BackMiddle) != 0,
		"backLower",
		(needs_tex & MapLine::Part::BackLower) != 0);
}

// -----------------------------------------------------------------------------
// Returns true if the [flag] on map line [self] is set
// -----------------------------------------------------------------------------
bool lineFlag(MapLine& self, const string& flag)
{
	if (game::configuration().lineBasicFlagSet(flag, &self, self.parentMap()->currentFormat()))
		return true;

	return game::configuration().lineFlagSet(flag, &self, self.parentMap()->currentFormat());
}

// -----------------------------------------------------------------------------
// Registers the MapLine type with lua
// -----------------------------------------------------------------------------
void registerMapLine(sol::state& lua)
{
	// Create MapLine type, MapObject base class, no constructor
	auto lua_line = lua.new_usertype<MapLine>(
		"MapLine", sol::base_classes, sol::bases<MapObject>(), "new", sol::no_constructor);

	// Properties
	// -------------------------------------------------------------------------
	lua_line["x1"]      = sol::property(&MapLine::x1);
	lua_line["y1"]      = sol::property(&MapLine::y1);
	lua_line["x2"]      = sol::property(&MapLine::x2);
	lua_line["y2"]      = sol::property(&MapLine::y2);
	lua_line["vertex1"] = sol::property(&MapLine::v1);
	lua_line["vertex2"] = sol::property(&MapLine::v2);
	lua_line["side1"]   = sol::property(&MapLine::s1);
	lua_line["side2"]   = sol::property(&MapLine::s2);
	lua_line["special"] = sol::property(&MapLine::special);
	lua_line["length"]  = sol::property(&MapLine::length);

	// Functions
	// -------------------------------------------------------------------------
	lua_line["Flag"]            = &lineFlag;
	lua_line["Flip"]            = sol::overload(&MapLine::flip, [](MapLine& self) { self.flip(true); });
	lua_line["VisibleTextures"] = &lineVisibleTextures;
}

// -----------------------------------------------------------------------------
// Registers the MapSide type with lua
// -----------------------------------------------------------------------------
void registerMapSide(sol::state& lua)
{
	// Create MapSide type, MapObject base class, no constructor
	auto lua_side = lua.new_usertype<MapSide>(
		"MapSide", sol::base_classes, sol::bases<MapObject>(), "new", sol::no_constructor);

	// Properties
	// -------------------------------------------------------------------------
	lua_side["sector"]        = sol::property(&MapSide::sector);
	lua_side["line"]          = sol::property(&MapSide::parentLine);
	lua_side["textureBottom"] = sol::property(&MapSide::texLower);
	lua_side["textureMiddle"] = sol::property(&MapSide::texMiddle);
	lua_side["textureTop"]    = sol::property(&MapSide::texUpper);
	lua_side["offsetX"]       = sol::property(&MapSide::texOffsetX);
	lua_side["offsetY"]       = sol::property(&MapSide::texOffsetY);
}

// -----------------------------------------------------------------------------
// Registers the MapSector type with lua
// -----------------------------------------------------------------------------
void registerMapSector(sol::state& lua)
{
	// Create MapSector type, MapObject base class, no constructor
	auto lua_sector = lua.new_usertype<MapSector>(
		"MapSector", sol::base_classes, sol::bases<MapObject>(), "new", sol::no_constructor);

	// Properties
	// -------------------------------------------------------------------------
	lua_sector["textureFloor"]   = sol::property([](MapSector& self) { return self.floor().texture; });
	lua_sector["textureCeiling"] = sol::property([](MapSector& self) { return self.ceiling().texture; });
	lua_sector["heightFloor"]    = sol::property([](MapSector& self) { return self.floor().height; });
	lua_sector["heightCeiling"]  = sol::property([](MapSector& self) { return self.ceiling().height; });
	lua_sector["lightLevel"]     = sol::property(&MapSector::lightLevel);
	lua_sector["special"]        = sol::property(&MapSector::special);
	lua_sector["id"]             = sol::property(&MapSector::tag);
	lua_sector["connectedSides"] = sol::property(&MapSector::connectedSides);
	lua_sector["colour"]         = sol::property(&MapSector::colourAt);
	lua_sector["fogColour"]      = sol::property(&MapSector::fogColour);
	lua_sector["planeFloor"]     = sol::property([](MapSector& self) { return self.floor().plane; });
	lua_sector["planeCeiling"]   = sol::property([](MapSector& self) { return self.ceiling().plane; });
	// TODO: bbox (need to export BBox struct first)

	// Functions
	// -------------------------------------------------------------------------
	lua_sector["ContainsPoint"] = &MapSector::containsPoint;
}

// -----------------------------------------------------------------------------
// Returns true if [flag] is set on map thing [self]
// -----------------------------------------------------------------------------
bool thingFlag(MapThing& self, const string& flag)
{
	if (game::configuration().thingBasicFlagSet(flag, &self, self.parentMap()->currentFormat()))
		return true;

	return game::configuration().thingFlagSet(flag, &self, self.parentMap()->currentFormat());
}

// -----------------------------------------------------------------------------
// Registers the MapThing type with lua
// -----------------------------------------------------------------------------
void registerMapThing(sol::state& lua)
{
	auto lua_thing = lua.new_usertype<MapThing>(
		"MapThing", sol::base_classes, sol::bases<MapObject>(), "new", sol::no_constructor);

	// Properties
	// -------------------------------------------------------------------------
	lua_thing["x"]     = sol::property(&MapThing::xPos);
	lua_thing["y"]     = sol::property(&MapThing::yPos);
	lua_thing["type"]  = sol::property(&MapThing::type);
	lua_thing["angle"] = sol::property(&MapThing::angle);

	// Functions
	// -------------------------------------------------------------------------
	lua_thing["Flag"]          = &thingFlag;
	lua_thing["SetAnglePoint"] = &MapThing::setAnglePoint;
}

// -----------------------------------------------------------------------------
// Registers the MapObject type with lua
// -----------------------------------------------------------------------------
void registerMapObject(sol::state& lua)
{
	auto lua_mapobject = lua.new_usertype<MapObject>("MapObject", "new", sol::no_constructor);

	// Properties
	// -------------------------------------------------------------------------
	lua_mapobject["index"]    = sol::property(&MapObject::index);
	lua_mapobject["type"]     = sol::property(&MapObject::objType);
	lua_mapobject["typeName"] = sol::property(&MapObject::typeName);
	// lua_mapobject["properties"] = sol::property(&MapObject::props); // TODO: Need to export MobjPropertyList first


	// Constants
	// -------------------------------------------------------------------------
	lua_mapobject["TYPE_OBJECT"] = sol::property([]() { return MapObject::Type::Object; });
	lua_mapobject["TYPE_VERTEX"] = sol::property([]() { return MapObject::Type::Vertex; });
	lua_mapobject["TYPE_LINE"]   = sol::property([]() { return MapObject::Type::Line; });
	lua_mapobject["TYPE_SIDE"]   = sol::property([]() { return MapObject::Type::Side; });
	lua_mapobject["TYPE_SECTOR"] = sol::property([]() { return MapObject::Type::Sector; });
	lua_mapobject["TYPE_THING"]  = sol::property([]() { return MapObject::Type::Thing; });


	// Functions
	// -------------------------------------------------------------------------
	lua_mapobject["HasProperty"]       = &MapObject::hasProp;
	lua_mapobject["BoolProperty"]      = &MapObject::boolProperty;
	lua_mapobject["IntProperty"]       = &MapObject::intProperty;
	lua_mapobject["FloatProperty"]     = &MapObject::floatProperty;
	lua_mapobject["StringProperty"]    = &MapObject::stringProperty;
	lua_mapobject["SetBoolProperty"]   = &objectSetBoolProperty;
	lua_mapobject["SetIntProperty"]    = &objectSetIntProperty;
	lua_mapobject["SetFloatProperty"]  = &objectSetFloatProperty;
	lua_mapobject["SetStringProperty"] = &objectSetStringProperty;
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
