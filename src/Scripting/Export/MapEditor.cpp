
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
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
#include "Scripting/LuaBridge.h"

using namespace slade;
using namespace mapeditor;

// ReSharper disable CppParameterMayBeConstPtrOrRef


// -----------------------------------------------------------------------------
//
// Scripting Namespace Functions
//
// -----------------------------------------------------------------------------
namespace slade::scripting
{
// -----------------------------------------------------------------------------
// Attempts to correct the sectors for [line] in [self]
// -----------------------------------------------------------------------------
static bool correctLineSectors(SLADEMap& self, MapLine* line)
{
	return line && self.correctLineSectors(line);
}

// -----------------------------------------------------------------------------
// Sets [side] on [line] in [self], ignoring invalid pointers from lua
// -----------------------------------------------------------------------------
static void setLineSide(SLADEMap& self, MapLine* line, MapSide* side, bool front)
{
	if (line && side)
		self.setLineSide(line, side, front);
}

// -----------------------------------------------------------------------------
// Corrects sectors for [lines] in [self], ignoring invalid pointers from lua
// -----------------------------------------------------------------------------
static void correctSectors(SLADEMap& self, vector<MapLine*>& lines, bool existing_only)
{
	lines.erase(std::remove(lines.begin(), lines.end(), nullptr), lines.end());
	self.correctSectors(lines, existing_only);
}

// -----------------------------------------------------------------------------
// Registers the Map type with lua
// -----------------------------------------------------------------------------
static void registerSLADEMap(lua_State* lua)
{
	// Create ArchiveFormat type, no constructor
	auto lua_map = luabridge::getGlobalNamespace(lua).beginClass<SLADEMap>("Map");

	// Properties
	// -------------------------------------------------------------------------
	lua_map.addProperty("name", &SLADEMap::mapName);
	lua_map.addProperty("udmfNamespace", &SLADEMap::udmfNamespace);
	lua_map.addProperty("vertices", [](SLADEMap& self) { return self.vertices().all(); });
	lua_map.addProperty("linedefs", [](SLADEMap& self) { return self.lines().all(); });
	lua_map.addProperty("sidedefs", [](SLADEMap& self) { return self.sides().all(); });
	lua_map.addProperty("sectors", [](SLADEMap& self) { return self.sectors().all(); });
	lua_map.addProperty("things", [](SLADEMap& self) { return self.things().all(); });

	// Creation Functions
	// -------------------------------------------------------------------------
	lua_map.addFunction(
		"CreateVertex",
		[](SLADEMap& self, Vec2d pos) { return self.createVertex(pos); },
		[](SLADEMap& self, Vec2d pos, double split_dist) { return self.createVertex(pos, split_dist); });
	lua_map.addFunction(
		"CreateLine",
		[](SLADEMap& self, Vec2d p1, Vec2d p2) { return self.createLine(p1, p2); },
		[](SLADEMap& self, Vec2d p1, Vec2d p2, double split_dist)
		{ return self.createLine(p1, p2, split_dist); },
		[](SLADEMap& self, MapVertex* v1, MapVertex* v2) { return self.createLine(v1, v2); },
		[](SLADEMap& self, MapVertex* v1, MapVertex* v2, bool force) { return self.createLine(v1, v2, force); });
	lua_map.addFunction(
		"CreateThing",
		[](SLADEMap& self, Vec2d pos) { return self.createThing(pos); },
		[](SLADEMap& self, Vec2d pos, int type) { return self.createThing(pos, type); });
	lua_map.addFunction("CreateSector", &SLADEMap::createSector);
	lua_map.addFunction("CreateSide", &SLADEMap::createSide);

	// Removal Functions
	// -------------------------------------------------------------------------
	lua_map.addFunction(
		"RemoveVertex",
		[](SLADEMap& self, MapVertex* vertex) { return self.removeVertex(vertex); },
		[](SLADEMap& self, MapVertex* vertex, bool merge_lines) { return self.removeVertex(vertex, merge_lines); },
		[](SLADEMap& self, unsigned index) { return self.removeVertex(index); },
		[](SLADEMap& self, unsigned index, bool merge_lines) { return self.removeVertex(index, merge_lines); });
	lua_map.addFunction(
		"RemoveLine",
		[](SLADEMap& self, MapLine* line) { return self.removeLine(line); },
		[](SLADEMap& self, unsigned index) { return self.removeLine(index); });
	lua_map.addFunction(
		"RemoveSide",
		[](SLADEMap& self, MapSide* side) { return self.removeSide(side); },
		[](SLADEMap& self, MapSide* side, bool remove_from_line) { return self.removeSide(side, remove_from_line); },
		[](SLADEMap& self, unsigned index) { return self.removeSide(index); },
		[](SLADEMap& self, unsigned index, bool remove_from_line) { return self.removeSide(index, remove_from_line); });
	lua_map.addFunction(
		"RemoveSector",
		[](SLADEMap& self, MapSector* sector) { return self.removeSector(sector); },
		[](SLADEMap& self, unsigned index) { return self.removeSector(index); });
	lua_map.addFunction(
		"RemoveThing",
		[](SLADEMap& self, MapThing* thing) { return self.removeThing(thing); },
		[](SLADEMap& self, unsigned index) { return self.removeThing(index); });
	lua_map.addFunction("RemoveDetachedVertices", &SLADEMap::removeDetachedVertices);

	// Editing Functions
	// -------------------------------------------------------------------------
	lua_map.addFunction("MergeVertices", &SLADEMap::mergeVertices);
	lua_map.addFunction("MergeVerticesPoint", &SLADEMap::mergeVerticesPoint);
	lua_map.addFunction("SplitLine", &SLADEMap::splitLine);
	lua_map.addFunction(
		"SplitLinesAt",
		[](SLADEMap& self, MapVertex* vertex) { self.splitLinesAt(vertex); },
		[](SLADEMap& self, MapVertex* vertex, double split_dist) { self.splitLinesAt(vertex, split_dist); });
	lua_map.addFunction(
		"SetLineSector",
		[](SLADEMap& self, unsigned line_index, unsigned sector_index)
		{ return self.setLineSector(line_index, sector_index); },
		[](SLADEMap& self, unsigned line_index, unsigned sector_index, bool front)
		{ return self.setLineSector(line_index, sector_index, front); });
	lua_map.addFunction("MergeLine", &SLADEMap::mergeLine);
	lua_map.addFunction("CorrectLineSectors", &correctLineSectors);
	lua_map.addFunction("SetLineSide", &setLineSide);
	lua_map.addFunction(
		"CorrectSectors",
		[](SLADEMap& self, vector<MapLine*> lines) { correctSectors(self, lines, false); },
		[](SLADEMap& self, vector<MapLine*> lines, bool existing_only)
		{ correctSectors(self, lines, existing_only); });
}

// -----------------------------------------------------------------------------
// Selects or deselects [object] in the map editor [self]
// -----------------------------------------------------------------------------
static void selectMapObject(MapEditContext& self, MapObject* object, bool select)
{
	if (object)
		self.selection().select({ static_cast<int>(object->index()), mapeditor::itemTypeFromObject(object) }, select);
}

// -----------------------------------------------------------------------------
// Sets the map editor [mode] in the map editor [self]
// -----------------------------------------------------------------------------
static void setEditMode(MapEditContext& self, Mode mode, SectorMode sector_mode = SectorMode::Both)
{
	self.setEditMode(mode);
	if (mode == Mode::Sectors)
		self.setSectorEditMode(sector_mode);
}

// -----------------------------------------------------------------------------
// Registers the MapEditor type with lua
// -----------------------------------------------------------------------------
static void registerMapEditor(lua_State* lua)
{
	// Create MapEditor type, no constructor
	auto lua_mapeditor = luabridge::getGlobalNamespace(lua).beginClass<MapEditContext>("MapEditor");

	// Properties
	// -------------------------------------------------------------------------
	lua_mapeditor.addProperty("editMode", &MapEditContext::editMode);
	lua_mapeditor.addProperty("sectorEditMode", &MapEditContext::sectorEditMode);
	lua_mapeditor.addProperty("gridSize", &MapEditContext::gridSize);
	lua_mapeditor.addProperty("map", &MapEditContext::map);

	// Constants
	// -------------------------------------------------------------------------
	lua_mapeditor.addStaticProperty("MODE_VERTICES", [] { return static_cast<int>(Mode::Vertices); });
	lua_mapeditor.addStaticProperty("MODE_LINES", [] { return static_cast<int>(Mode::Lines); });
	lua_mapeditor.addStaticProperty("MODE_SECTORS", [] { return static_cast<int>(Mode::Sectors); });
	lua_mapeditor.addStaticProperty("MODE_THINGS", [] { return static_cast<int>(Mode::Things); });
	lua_mapeditor.addStaticProperty("MODE_VISUAL", [] { return static_cast<int>(Mode::Visual); });
	lua_mapeditor.addStaticProperty("SECTORMODE_BOTH", [] { return static_cast<int>(SectorMode::Both); });
	lua_mapeditor.addStaticProperty("SECTORMODE_FLOOR", [] { return static_cast<int>(SectorMode::Floor); });
	lua_mapeditor.addStaticProperty("SECTORMODE_CEILING", [] { return static_cast<int>(SectorMode::Ceiling); });

	// Functions
	// -------------------------------------------------------------------------
	lua_mapeditor.addFunction(
		"SelectedVertices",
		[](MapEditContext& self, bool try_hilight) { return self.selection().selectedVertices(try_hilight); },
		[](MapEditContext& self) { return self.selection().selectedVertices(false); });
	lua_mapeditor.addFunction(
		"SelectedLines",
		[](MapEditContext& self, bool try_hilight) { return self.selection().selectedLines(try_hilight); },
		[](MapEditContext& self) { return self.selection().selectedLines(false); });
	lua_mapeditor.addFunction(
		"SelectedSectors",
		[](MapEditContext& self, bool try_hilight) { return self.selection().selectedSectors(try_hilight); },
		[](MapEditContext& self) { return self.selection().selectedSectors(false); });
	lua_mapeditor.addFunction(
		"SelectedThings",
		[](MapEditContext& self, bool try_hilight) { return self.selection().selectedThings(try_hilight); },
		[](MapEditContext& self) { return self.selection().selectedThings(false); });
	lua_mapeditor.addFunction("ClearSelection", [](MapEditContext& self) { self.selection().clear(); });
	lua_mapeditor.addFunction(
		"Select",
		&selectMapObject,
		[](MapEditContext& self, MapObject* object) { selectMapObject(self, object, true); });
	lua_mapeditor.addFunction(
		"SetEditMode",
		[](MapEditContext& self, Mode mode) { setEditMode(self, mode); },
		[](MapEditContext& self, Mode mode, SectorMode sector_mode) { setEditMode(self, mode, sector_mode); });
}

// -----------------------------------------------------------------------------
// Registers various MapEditor-related types with lua
// -----------------------------------------------------------------------------
void registerMapEditorTypes(lua_State* lua)
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

} // namespace slade::scripting
