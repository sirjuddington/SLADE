
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapObjectCollection.cpp
// Description: Contains and keeps track of all MapObjects (vertices, lines
//              etc.) for a map.
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
#include "MapObjectCollection.h"
#include "Game/Configuration.h"
#include "MapObject/MapLine.h"
#include "MapObject/MapSector.h"
#include "MapObject/MapSide.h"
#include "MapObject/MapThing.h"
#include "MapObject/MapVertex.h"
#include "MapObjectList/LineList.h"
#include "MapObjectList/SectorList.h"
#include "MapObjectList/SideList.h"
#include "MapObjectList/ThingList.h"
#include "MapObjectList/VertexList.h"
#include "SLADEMap.h"

using namespace slade;


namespace
{
template<class T>
void putModifiedObjects(const MapObjectList<T>& objects, long since, vector<MapObject*>& modified_objects)
{
	for (const auto& object : objects)
		if (object->modifiedTime() >= since)
			modified_objects.push_back(object);
}

template<class T> bool anyModifiedSince(const MapObjectList<T>& objects, long since)
{
	for (const auto& object : objects)
		if (object->modifiedTime() > since)
			return true;

	return false;
}
} // namespace


// -----------------------------------------------------------------------------
// MapObjectCollection class constructor
// -----------------------------------------------------------------------------
MapObjectCollection::MapObjectCollection(SLADEMap* parent_map) : parent_map_{ parent_map }
{
	vertices_ = std::make_unique<VertexList>();
	sides_    = std::make_unique<SideList>();
	lines_    = std::make_unique<LineList>();
	sectors_  = std::make_unique<SectorList>();
	things_   = std::make_unique<ThingList>();

	// Object id 0 is always null
	objects_.emplace_back(nullptr, false);
}

// -----------------------------------------------------------------------------
// Adds [object] to the map objects list
// -----------------------------------------------------------------------------
void MapObjectCollection::addMapObject(unique_ptr<MapObject> object)
{
	object->obj_id_     = objects_.size();
	object->parent_map_ = parent_map_;
	objects_.emplace_back(std::move(object), true);
}

// -----------------------------------------------------------------------------
// Removes [object] from the map
// (keeps it in the objects list, but removes the 'in map' flag)
// -----------------------------------------------------------------------------
void MapObjectCollection::removeMapObject(const MapObject* object)
{
	objects_[object->obj_id_].in_map = false;
}

// -----------------------------------------------------------------------------
// Adds all object ids of [type] currently in the map to [list]
// -----------------------------------------------------------------------------
void MapObjectCollection::putObjectIdList(MapObject::Type type, vector<unsigned>& list) const
{
	if (type == MapObject::Type::Vertex)
	{
		for (auto& vertex : *vertices_)
			list.push_back(vertex->obj_id_);
	}
	else if (type == MapObject::Type::Line)
	{
		for (auto& line : *lines_)
			list.push_back(line->obj_id_);
	}
	else if (type == MapObject::Type::Side)
	{
		for (auto& side : *sides_)
			list.push_back(side->obj_id_);
	}
	else if (type == MapObject::Type::Sector)
	{
		for (auto& sector : *sectors_)
			list.push_back(sector->obj_id_);
	}
	else if (type == MapObject::Type::Thing)
	{
		for (auto& thing : *things_)
			list.push_back(thing->obj_id_);
	}
}

// -----------------------------------------------------------------------------
// Add all object ids in [list] to the map as [type], clearing any objects of
// [type] currently in the map
// -----------------------------------------------------------------------------
void MapObjectCollection::restoreObjectIdList(MapObject::Type type, const vector<unsigned>& list)
{
	if (type == MapObject::Type::Vertex)
	{
		auto& vertices = *vertices_;

		// Clear
		for (auto& vertex : vertices)
			objects_[vertex->obj_id_].in_map = false;
		vertices.clear();

		// Restore
		for (auto id : list)
		{
			objects_[id].in_map = true;
			vertices.add(dynamic_cast<MapVertex*>(objects_[id].object.get()));
			vertices.last()->index_ = vertices.size() - 1;
		}
	}
	else if (type == MapObject::Type::Line)
	{
		auto& lines = *lines_;

		// Clear
		for (auto& line : lines)
			objects_[line->obj_id_].in_map = false;
		lines.clear();

		// Restore
		for (auto id : list)
		{
			objects_[id].in_map = true;
			lines.add(dynamic_cast<MapLine*>(objects_[id].object.get()));
			lines.back()->index_ = lines.size() - 1;
		}
	}
	else if (type == MapObject::Type::Side)
	{
		auto& sides = *sides_;

		// Clear
		for (auto& side : sides)
			objects_[side->obj_id_].in_map = false;
		sides.clear();

		// Restore
		for (auto id : list)
		{
			objects_[id].in_map = true;
			sides.add(dynamic_cast<MapSide*>(objects_[id].object.get()));
			sides.back()->index_ = sides.size() - 1;
		}
	}
	else if (type == MapObject::Type::Sector)
	{
		auto& sectors = *sectors_;

		// Clear
		for (auto& sector : sectors)
			objects_[sector->obj_id_].in_map = false;
		sectors.clear();

		// Restore
		for (auto id : list)
		{
			objects_[id].in_map = true;
			sectors.add(dynamic_cast<MapSector*>(objects_[id].object.get()));
			sectors.back()->index_ = sectors.size() - 1;
		}
	}
	else if (type == MapObject::Type::Thing)
	{
		auto& things = *things_;

		// Clear
		for (auto& thing : things)
			objects_[thing->obj_id_].in_map = false;
		things.clear();

		// Restore
		for (auto id : list)
		{
			objects_[id].in_map = true;
			things.add(dynamic_cast<MapThing*>(objects_[id].object.get()));
			things.back()->index_ = things.size() - 1;
		}
	}
}

// -----------------------------------------------------------------------------
// Refreshes all map object indices
// -----------------------------------------------------------------------------
void MapObjectCollection::refreshIndices() const
{
	// Vertex indices
	for (unsigned a = 0; a < vertices_->size(); a++)
		(*vertices_)[a]->index_ = a;

	// Side indices
	for (unsigned a = 0; a < sides_->size(); a++)
		(*sides_)[a]->index_ = a;

	// Line indices
	for (unsigned a = 0; a < lines_->size(); a++)
		(*lines_)[a]->index_ = a;

	// Sector indices
	for (unsigned a = 0; a < sectors_->size(); a++)
		(*sectors_)[a]->index_ = a;

	// Thing indices
	for (unsigned a = 0; a < things_->size(); a++)
		(*things_)[a]->index_ = a;
}

// -----------------------------------------------------------------------------
// Clears all objects
// -----------------------------------------------------------------------------
void MapObjectCollection::clear()
{
	// Clear lists
	sides_->clear();
	lines_->clear();
	vertices_->clear();
	sectors_->clear();
	things_->clear();

	// Clear map objects
	objects_.clear();

	// Object id 0 is always null
	objects_.emplace_back(nullptr, false);
}

// -----------------------------------------------------------------------------
// Removes [vertex] from the map
// -----------------------------------------------------------------------------
bool MapObjectCollection::removeVertex(const MapVertex* vertex, bool merge_lines)
{
	// Check vertex was given
	if (!vertex)
		return false;

	return removeVertex(vertex->index_, merge_lines);
}

// -----------------------------------------------------------------------------
// Removes the vertex at [index] from the map
// -----------------------------------------------------------------------------
bool MapObjectCollection::removeVertex(unsigned index, bool merge_lines)
{
	auto& vertices = *vertices_;

	// Check index
	if (index >= vertices.size())
		return false;

	// Check if we should merge connected lines
	bool merged = false;
	auto vertex = vertices[index];
	if (merge_lines && vertex->nConnectedLines() == 2)
	{
		// Get other end vertex of second connected line
		auto l_first  = vertex->connectedLines()[0];
		auto l_second = vertex->connectedLines()[1];
		auto v_end    = l_second->v2();
		if (v_end == vertex)
			v_end = l_second->v1();

		// Remove second connected line
		removeLine(l_second);

		// Connect first connected line to other end vertex
		l_first->setModified();
		if (l_first->v1() == vertex)
			l_first->setV1(v_end);
		else
			l_first->setV2(v_end);
		vertex->disconnectLine(l_first);
		v_end->connectLine(l_first);
		l_first->resetInternals();

		// Check if we ended up with overlapping lines (ie. there was a triangle)
		for (auto& line : v_end->connectedLines())
		{
			if (l_first->overlaps(line))
			{
				// Overlap found, remove line
				removeLine(l_first);
				break;
			}
		}

		merged = true;
	}

	if (!merged)
	{
		// Remove all connected lines
		auto& clines = vertex->connectedLines();
		for (auto& line : clines)
			removeLine(line);
	}

	// Remove the vertex
	removeMapObject(vertex);
	vertices.remove(index);

	if (parent_map_)
		parent_map_->setGeometryUpdated();

	return true;
}

// -----------------------------------------------------------------------------
// Removes [line] from the map
// -----------------------------------------------------------------------------
bool MapObjectCollection::removeLine(const MapLine* line)
{
	// Check line was given
	if (!line)
		return false;

	return removeLine(line->index_);
}

// -----------------------------------------------------------------------------
// Removes the line at [index] from the map
// -----------------------------------------------------------------------------
bool MapObjectCollection::removeLine(unsigned index)
{
	auto& lines = *lines_;

	// Check index
	if (index >= lines.size())
		return false;

	log::info(4, "id {}  index {}  objindex {}", lines[index]->obj_id_, index, lines[index]->index_);

	// Init
	auto line = lines[index];
	line->resetInternals();

	// Remove the line's sides
	if (line->s1())
		removeSide(line->s1(), false);
	if (line->s2())
		removeSide(line->s2(), false);

	// Disconnect from vertices
	line->v1()->disconnectLine(line);
	line->v2()->disconnectLine(line);

	// Remove the line
	removeMapObject(line);
	lines.remove(index);

	if (parent_map_)
		parent_map_->setGeometryUpdated();

	return true;
}

// -----------------------------------------------------------------------------
// Removes [side] from the map
// -----------------------------------------------------------------------------
bool MapObjectCollection::removeSide(const MapSide* side, bool remove_from_line)
{
	// Check side was given
	if (!side)
		return false;

	return removeSide(side->index_, remove_from_line);
}

// -----------------------------------------------------------------------------
// Removes the side at [index] from the map
// -----------------------------------------------------------------------------
bool MapObjectCollection::removeSide(unsigned index, bool remove_from_line)
{
	auto& sides = *sides_;

	// Check index
	if (index >= sides.size())
		return false;

	if (remove_from_line)
	{
		// Remove from parent line
		auto l = sides[index]->parentLine();
		l->setModified();
		if (l->s1() == sides[index])
			l->setS1(nullptr);
		if (l->s2() == sides[index])
			l->setS2(nullptr);

		// Set appropriate line flags
		if (parent_map_)
		{
			game::configuration().setLineBasicFlag("blocking", l, parent_map_->currentFormat(), true);
			game::configuration().setLineBasicFlag("twosided", l, parent_map_->currentFormat(), false);
		}
	}

	// Remove side from its sector, if any
	if (sides[index]->sector())
	{
		auto  sector          = sides[index]->sector();
		auto& connected_sides = sector->connectedSides();
		for (unsigned a = 0; a < connected_sides.size(); a++)
		{
			if (connected_sides[a] == sides[index])
			{
				connected_sides.erase(connected_sides.begin() + a);

				// Remove sector if all its sides are gone
				if (connected_sides.empty())
					removeSector(sector);

				break;
			}
		}
	}

	// Remove the side
	removeMapObject(sides[index]);
	sides.remove(index);

	return true;
}

// -----------------------------------------------------------------------------
// Removes [sector] from the map
// -----------------------------------------------------------------------------
bool MapObjectCollection::removeSector(const MapSector* sector)
{
	// Check sector was given
	if (!sector)
		return false;

	return removeSector(sector->index_);
}

// -----------------------------------------------------------------------------
// Removes the sector at [index] from the map
// -----------------------------------------------------------------------------
bool MapObjectCollection::removeSector(unsigned index)
{
	auto& sectors = *sectors_;

	// Check index
	if (index >= sectors.size())
		return false;

	// Remove the sector
	removeMapObject(sectors[index]);
	sectors.remove(index);

	return true;
}

// -----------------------------------------------------------------------------
// Removes [thing] from the map
// -----------------------------------------------------------------------------
bool MapObjectCollection::removeThing(const MapThing* thing)
{
	// Check thing was given
	if (!thing)
		return false;

	return removeThing(thing->index_);
}

// -----------------------------------------------------------------------------
// Removes the thing at [index] from the map
// -----------------------------------------------------------------------------
bool MapObjectCollection::removeThing(unsigned index)
{
	auto& things = *things_;

	// Check index
	if (index >= things.size())
		return false;

	// Remove the thing
	removeMapObject(things[index]);
	things.remove(index);

	if (parent_map_)
		parent_map_->setThingsUpdated();

	return true;
}

// -----------------------------------------------------------------------------
// Adds [vertex] to the map
// -----------------------------------------------------------------------------
MapVertex* MapObjectCollection::addVertex(unique_ptr<MapVertex> vertex)
{
	vertex->index_ = vertices_->size();
	vertices_->add(vertex.get());
	addMapObject(std::move(vertex));
	return vertices_->back();
}

// -----------------------------------------------------------------------------
// Adds [side] to the map
// -----------------------------------------------------------------------------
MapSide* MapObjectCollection::addSide(unique_ptr<MapSide> side)
{
	side->index_ = sides_->size();
	sides_->add(side.get());
	addMapObject(std::move(side));
	return sides_->back();
}

// -----------------------------------------------------------------------------
// Adds [line] to the map
// -----------------------------------------------------------------------------
MapLine* MapObjectCollection::addLine(unique_ptr<MapLine> line)
{
	line->index_ = lines_->size();
	lines_->add(line.get());
	addMapObject(std::move(line));
	return lines_->back();
}

// -----------------------------------------------------------------------------
// Adds [sector] to the map
// -----------------------------------------------------------------------------
MapSector* MapObjectCollection::addSector(unique_ptr<MapSector> sector)
{
	sector->index_ = sectors_->size();
	sectors_->add(sector.get());
	addMapObject(std::move(sector));
	return sectors_->back();
}

// -----------------------------------------------------------------------------
// Adds [thing] to the map
// -----------------------------------------------------------------------------
MapThing* MapObjectCollection::addThing(unique_ptr<MapThing> thing)
{
	thing->index_ = things_->size();
	things_->add(thing.get());
	addMapObject(std::move(thing));
	return things_->back();
}

// -----------------------------------------------------------------------------
// Creates and adds a new side duplicated from the given [side] and returns it
// -----------------------------------------------------------------------------
MapSide* MapObjectCollection::duplicateSide(MapSide* side)
{
	if (!side)
		return nullptr;

	auto ns = std::make_unique<MapSide>(side->sector());
	ns->copy(side);
	addSide(std::move(ns));
	return sides_->back();
}

// -----------------------------------------------------------------------------
// Returns a list of objects of [type] that have a modified time later than
// [since]
// -----------------------------------------------------------------------------
vector<MapObject*> MapObjectCollection::modifiedObjects(long since, MapObject::Type type) const
{
	vector<MapObject*> modified_objects;

	if (type == MapObject::Type::Object || type == MapObject::Type::Vertex)
		putModifiedObjects(*vertices_, since, modified_objects);
	if (type == MapObject::Type::Object || type == MapObject::Type::Side)
		putModifiedObjects(*sides_, since, modified_objects);
	if (type == MapObject::Type::Object || type == MapObject::Type::Line)
		putModifiedObjects(*lines_, since, modified_objects);
	if (type == MapObject::Type::Object || type == MapObject::Type::Sector)
		putModifiedObjects(*sectors_, since, modified_objects);
	if (type == MapObject::Type::Object || type == MapObject::Type::Thing)
		putModifiedObjects(*things_, since, modified_objects);

	return modified_objects;
}

// -----------------------------------------------------------------------------
// Returns a list of objects that have a modified time later than [since]
// -----------------------------------------------------------------------------
vector<MapObject*> MapObjectCollection::allModifiedObjects(long since) const
{
	vector<MapObject*> modified_objects;

	for (auto& holder : objects_)
	{
		if (holder.object && holder.object->modified_time_ >= since)
			modified_objects.push_back(holder.object.get());
	}

	return modified_objects;
}

// -----------------------------------------------------------------------------
// Returns the newest modified time on any map object
// -----------------------------------------------------------------------------
long MapObjectCollection::lastModifiedTime() const
{
	long mod_time = 0;

	for (auto& holder : objects_)
	{
		if (holder.object && holder.object->modified_time_ > mod_time)
			mod_time = holder.object->modified_time_;
	}

	return mod_time;
}

// -----------------------------------------------------------------------------
// Returns true if any objects of [type] have a modified time newer than [since]
// -----------------------------------------------------------------------------
bool MapObjectCollection::modifiedSince(long since, MapObject::Type type) const
{
	switch (type)
	{
	case MapObject::Type::Object: return lastModifiedTime() > since;
	case MapObject::Type::Vertex: return anyModifiedSince(*vertices_, since);
	case MapObject::Type::Line:   return anyModifiedSince(*lines_, since);
	case MapObject::Type::Side:   return anyModifiedSince(*sides_, since);
	case MapObject::Type::Sector: return anyModifiedSince(*sectors_, since);
	case MapObject::Type::Thing:  return anyModifiedSince(*things_, since);
	default:                      return false;
	}
}

// -----------------------------------------------------------------------------
// Removes any vertices not attached to any lines. Returns the number of
// vertices removed
// -----------------------------------------------------------------------------
int MapObjectCollection::removeDetachedVertices()
{
	auto& vertices = *vertices_;
	int   count    = 0;
	for (int a = vertices.size() - 1; a >= 0; a--)
	{
		if (vertices[a]->nConnectedLines() == 0)
		{
			removeVertex(a);
			count++;
		}
	}

	refreshIndices();

	return count;
}

// -----------------------------------------------------------------------------
// Removes any sides that have no parent line.
// Returns the number of sides removed
// -----------------------------------------------------------------------------
int MapObjectCollection::removeDetachedSides()
{
	auto& sides = *sides_;
	int   count = 0;
	for (int a = sides.size() - 1; a >= 0; a--)
	{
		if (!sides[a]->parentLine())
		{
			removeSide(a, false);
			count++;
		}
	}

	refreshIndices();

	return count;
}

// -----------------------------------------------------------------------------
// Removes any sectors that are not referenced by any sides.
// Returns the number of sectors removed
// -----------------------------------------------------------------------------
int MapObjectCollection::removeDetachedSectors()
{
	auto& sectors = *sectors_;
	int   count   = 0;
	for (int a = sectors.size() - 1; a >= 0; a--)
	{
		if (sectors[a]->connectedSides().empty())
		{
			removeSector(a);
			count++;
		}
	}

	refreshIndices();

	return count;
}

// -----------------------------------------------------------------------------
// Removes any lines that have identical first and second vertices.
// Returns the number of lines removed
// -----------------------------------------------------------------------------
int MapObjectCollection::removeZeroLengthLines()
{
	auto& lines = *lines_;
	int   count = 0;
	for (unsigned a = 0; a < lines.size(); a++)
	{
		if (lines[a]->v1() == lines[a]->v2())
		{
			removeLine(a);
			a--;
			count++;
		}
	}

	return count;
}

// -----------------------------------------------------------------------------
// Removes any sides that reference non-existant sectors
// -----------------------------------------------------------------------------
int MapObjectCollection::removeInvalidSides()
{
	auto& sides = *sides_;
	int   count = 0;
	for (unsigned a = 0; a < sides.size(); a++)
	{
		if (!sides[a]->sector())
		{
			removeSide(a);
			a--;
			count++;
		}
	}

	return count;
}

// -----------------------------------------------------------------------------
// Rebuilds the connected lines lists for all map vertices
// -----------------------------------------------------------------------------
void MapObjectCollection::rebuildConnectedLines() const
{
	// Clear vertex connected lines lists
	for (auto& vertex : *vertices_)
		vertex->clearConnectedLines();

	// Connect lines to their vertices
	for (auto& line : *lines_)
	{
		line->v1()->connectLine(line);
		line->v2()->connectLine(line);
	}
}

// -----------------------------------------------------------------------------
// Rebuilds the connected sides lists for all map sectors
// -----------------------------------------------------------------------------
void MapObjectCollection::rebuildConnectedSides() const
{
	// Clear sector connected sides lists
	for (auto& sector : *sectors_)
		sector->clearConnectedSides();

	// Connect sides to their sectors
	for (auto& side : *sides_)
	{
		if (side->sector())
			side->sector()->connectSide(side);
	}
}

MapObjectCollection::MapObjectHolder::MapObjectHolder(unique_ptr<MapObject> object, bool in_map) :
	object{ std::move(object) },
	in_map{ in_map }
{
}
