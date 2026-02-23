
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Item.cpp
// Description: Item struct. Represents an individual map item in the map editor
//              Can be a 'part' of an object, eg. upper wall, floor, etc.
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
#include "Item.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapSide.h"
#include "SLADEMap/MapObject/MapThing.h"
#include "SLADEMap/MapObject/MapVertex.h"
#include "SLADEMap/SLADEMap.h"

using namespace slade;
using namespace mapeditor;

// -----------------------------------------------------------------------------
//
// Item Struct Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the vertex in [map] matching this item, or null if the item isn't a
// vertex
// -----------------------------------------------------------------------------
MapVertex* Item::asVertex(const SLADEMap& map) const
{
	if (type == ItemType::Vertex)
		return map.vertex(index);

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the line in [map] matching this item, or null if the item isn't a
// line
// -----------------------------------------------------------------------------
MapLine* Item::asLine(const SLADEMap& map) const
{
	if (type == ItemType::Line)
		return map.line(index);

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the side in [map] matching this item, or null if the item isn't a
// side
// -----------------------------------------------------------------------------
MapSide* Item::asSide(const SLADEMap& map) const
{
	if (type == ItemType::Side || type == ItemType::WallBottom || type == ItemType::WallMiddle
		|| type == ItemType::WallTop)
		return map.side(index);

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the sector in [map] matching this item, or null if the item isn't a
// sector
// -----------------------------------------------------------------------------
MapSector* Item::asSector(const SLADEMap& map) const
{
	if (type == ItemType::Sector || type == ItemType::Ceiling || type == ItemType::Floor)
		return map.sector(index);

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the thing in [map] matching this item, or null if the item isn't a
// thing
// -----------------------------------------------------------------------------
MapThing* Item::asThing(const SLADEMap& map) const
{
	if (type == ItemType::Thing)
		return map.thing(index);

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the object in [map] matching this item
// -----------------------------------------------------------------------------
MapObject* Item::asObject(const SLADEMap& map) const
{
	switch (type)
	{
	case ItemType::Vertex:     return map.vertex(index);
	case ItemType::Side:
	case ItemType::WallTop:
	case ItemType::WallMiddle:
	case ItemType::WallBottom:
	case ItemType::Line:       return map.line(index);
	case ItemType::Floor:
	case ItemType::Ceiling:
	case ItemType::Sector:     return map.sector(index);
	case ItemType::Thing:      return map.thing(index);
	default:                   return nullptr;
	}
}

// -----------------------------------------------------------------------------
// Returns the 'real' sector for this item, if it is a floor or ceiling
// (ie. the sector it is part of, not the control sector)
// -----------------------------------------------------------------------------
MapSector* Item::realSector(const SLADEMap& map) const
{
	if (type != ItemType::Floor && type != ItemType::Ceiling)
		return nullptr;

	return map.sector(real_index >= 0 ? real_index : index);
}

// -----------------------------------------------------------------------------
// Returns the 'real' side for this item, if it is a wall
// (ie. the side it is part of, not the control line side)
// -----------------------------------------------------------------------------
MapSide* Item::realSide(const SLADEMap& map) const
{
	if (baseItemType(type) != ItemType::Side)
		return nullptr;

	return map.side(real_index >= 0 ? real_index : index);
}

// -----------------------------------------------------------------------------
// Returns the control line for this item
// -----------------------------------------------------------------------------
MapLine* Item::controlLine(const SLADEMap& map) const
{
	return map.line(control_line);
}


// -----------------------------------------------------------------------------
//
// MapEditor Namespace Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Returns the 'base' item type for [type]
// (eg. WallMiddle is technically a Side)
// -----------------------------------------------------------------------------
ItemType mapeditor::baseItemType(const ItemType& type)
{
	switch (type)
	{
	case ItemType::Vertex:     return ItemType::Vertex;
	case ItemType::Line:       return ItemType::Line;
	case ItemType::Side:
	case ItemType::WallBottom:
	case ItemType::WallMiddle:
	case ItemType::WallTop:    return ItemType::Side;
	case ItemType::Sector:
	case ItemType::Ceiling:
	case ItemType::Floor:      return ItemType::Sector;
	case ItemType::Thing:      return ItemType::Thing;
	default:                   return ItemType::Any;
	}
}

// -----------------------------------------------------------------------------
// Returns the map editor item type of the given map [object]
// -----------------------------------------------------------------------------
ItemType mapeditor::itemTypeFromObject(const MapObject* object)
{
	switch (object->objType())
	{
	case MapObject::Type::Vertex: return ItemType::Vertex;
	case MapObject::Type::Line:   return ItemType::Line;
	case MapObject::Type::Side:   return ItemType::Side;
	case MapObject::Type::Sector: return ItemType::Sector;
	case MapObject::Type::Thing:  return ItemType::Thing;
	default:                      return ItemType::Any;
	}
}
