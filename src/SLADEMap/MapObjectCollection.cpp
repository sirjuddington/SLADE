
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
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
#include "SLADEMap.h"

using namespace slade;


// -----------------------------------------------------------------------------
// MapObjectCollection class constructor
// -----------------------------------------------------------------------------
MapObjectCollection::MapObjectCollection(SLADEMap* parent_map) : parent_map_{ parent_map }
{
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
void MapObjectCollection::removeMapObject(MapObject* object)
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
		for (auto& vertex : vertices_)
			list.push_back(vertex->obj_id_);
	}
	else if (type == MapObject::Type::Line)
	{
		for (auto& line : lines_)
			list.push_back(line->obj_id_);
	}
	else if (type == MapObject::Type::Side)
	{
		for (auto& side : sides_)
			list.push_back(side->obj_id_);
	}
	else if (type == MapObject::Type::Sector)
	{
		for (auto& sector : sectors_)
			list.push_back(sector->obj_id_);
	}
	else if (type == MapObject::Type::Thing)
	{
		for (auto& thing : things_)
			list.push_back(thing->obj_id_);
	}
}

// -----------------------------------------------------------------------------
// Add all object ids in [list] to the map as [type], clearing any objects of
// [type] currently in the map
// -----------------------------------------------------------------------------
void MapObjectCollection::restoreObjectIdList(MapObject::Type type, vector<unsigned>& list)
{
	if (type == MapObject::Type::Vertex)
	{
		// Clear
		for (auto& vertex : vertices_)
			objects_[vertex->obj_id_].in_map = false;
		vertices_.clear();

		// Restore
		for (auto id : list)
		{
			objects_[id].in_map = true;
			vertices_.add(dynamic_cast<MapVertex*>(objects_[id].object.get()));
			vertices_.last()->index_ = vertices_.size() - 1;
		}
	}
	else if (type == MapObject::Type::Line)
	{
		// Clear
		for (auto& line : lines_)
			objects_[line->obj_id_].in_map = false;
		lines_.clear();

		// Restore
		for (auto id : list)
		{
			objects_[id].in_map = true;
			lines_.add(dynamic_cast<MapLine*>(objects_[id].object.get()));
			lines_.back()->index_ = lines_.size() - 1;
		}
	}
	else if (type == MapObject::Type::Side)
	{
		// Clear
		for (auto& side : sides_)
			objects_[side->obj_id_].in_map = false;
		sides_.clear();

		// Restore
		for (auto id : list)
		{
			objects_[id].in_map = true;
			sides_.add(dynamic_cast<MapSide*>(objects_[id].object.get()));
			sides_.back()->index_ = sides_.size() - 1;
		}
	}
	else if (type == MapObject::Type::Sector)
	{
		// Clear
		for (auto& sector : sectors_)
			objects_[sector->obj_id_].in_map = false;
		sectors_.clear();

		// Restore
		for (auto id : list)
		{
			objects_[id].in_map = true;
			sectors_.add(dynamic_cast<MapSector*>(objects_[id].object.get()));
			sectors_.back()->index_ = sectors_.size() - 1;
		}
	}
	else if (type == MapObject::Type::Thing)
	{
		// Clear
		for (auto& thing : things_)
			objects_[thing->obj_id_].in_map = false;
		things_.clear();

		// Restore
		for (auto id : list)
		{
			objects_[id].in_map = true;
			things_.add(dynamic_cast<MapThing*>(objects_[id].object.get()));
			things_.back()->index_ = things_.size() - 1;
		}
	}
}

// -----------------------------------------------------------------------------
// Refreshes all map object indices
// -----------------------------------------------------------------------------
void MapObjectCollection::refreshIndices()
{
	// Vertex indices
	for (unsigned a = 0; a < vertices_.size(); a++)
		vertices_[a]->index_ = a;

	// Side indices
	for (unsigned a = 0; a < sides_.size(); a++)
		sides_[a]->index_ = a;

	// Line indices
	for (unsigned a = 0; a < lines_.size(); a++)
		lines_[a]->index_ = a;

	// Sector indices
	for (unsigned a = 0; a < sectors_.size(); a++)
		sectors_[a]->index_ = a;

	// Thing indices
	for (unsigned a = 0; a < things_.size(); a++)
		things_[a]->index_ = a;
}

// -----------------------------------------------------------------------------
// Clears all objects
// -----------------------------------------------------------------------------
void MapObjectCollection::clear()
{
	// Clear lists
	sides_.clear();
	lines_.clear();
	vertices_.clear();
	sectors_.clear();
	things_.clear();

	// Clear map objects
	objects_.clear();

	// Object id 0 is always null
	objects_.emplace_back(nullptr, false);
}

// -----------------------------------------------------------------------------
// Removes [vertex] from the map
// -----------------------------------------------------------------------------
bool MapObjectCollection::removeVertex(MapVertex* vertex, bool merge_lines)
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
	// Check index
	if (index >= vertices_.size())
		return false;

	// Check if we should merge connected lines
	bool merged = false;
	auto vertex = vertices_[index];
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
		auto clines = vertex->connectedLines();
		for (auto& line : clines)
			removeLine(line);
	}

	// Remove the vertex
	removeMapObject(vertex);
	vertices_.remove(index);

	if (parent_map_)
		parent_map_->setGeometryUpdated();

	return true;
}

// -----------------------------------------------------------------------------
// Removes [line] from the map
// -----------------------------------------------------------------------------
bool MapObjectCollection::removeLine(MapLine* line)
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
	// Check index
	if (index >= lines_.size())
		return false;

	log::info(4, "id {}  index {}  objindex {}", lines_[index]->obj_id_, index, lines_[index]->index_);

	// Init
	auto line = lines_[index];
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
	lines_.remove(index);

	if (parent_map_)
		parent_map_->setGeometryUpdated();

	return true;
}

// -----------------------------------------------------------------------------
// Removes [side] from the map
// -----------------------------------------------------------------------------
bool MapObjectCollection::removeSide(MapSide* side, bool remove_from_line)
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
	// Check index
	if (index >= sides_.size())
		return false;

	if (remove_from_line)
	{
		// Remove from parent line
		auto l = sides_[index]->parentLine();
		l->setModified();
		if (l->s1() == sides_[index])
			l->setS1(nullptr);
		if (l->s2() == sides_[index])
			l->setS2(nullptr);

		// Set appropriate line flags
		if (parent_map_)
		{
			game::configuration().setLineBasicFlag("blocking", l, parent_map_->currentFormat(), true);
			game::configuration().setLineBasicFlag("twosided", l, parent_map_->currentFormat(), false);
		}
	}

	// Remove side from its sector, if any
	if (sides_[index]->sector())
	{
		auto  sector          = sides_[index]->sector();
		auto& connected_sides = sector->connectedSides();
		for (unsigned a = 0; a < connected_sides.size(); a++)
		{
			if (connected_sides[a] == sides_[index])
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
	removeMapObject(sides_[index]);
	sides_.remove(index);

	return true;
}

// -----------------------------------------------------------------------------
// Removes [sector] from the map
// -----------------------------------------------------------------------------
bool MapObjectCollection::removeSector(MapSector* sector)
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
	// Check index
	if (index >= sectors_.size())
		return false;

	// Remove the sector
	removeMapObject(sectors_[index]);
	sectors_.remove(index);

	return true;
}

// -----------------------------------------------------------------------------
// Removes [thing] from the map
// -----------------------------------------------------------------------------
bool MapObjectCollection::removeThing(MapThing* thing)
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
	// Check index
	if (index >= things_.size())
		return false;

	// Remove the thing
	removeMapObject(things_[index]);
	things_.remove(index);

	if (parent_map_)
		parent_map_->setThingsUpdated();

	return true;
}

// -----------------------------------------------------------------------------
// Adds [vertex] to the map
// -----------------------------------------------------------------------------
MapVertex* MapObjectCollection::addVertex(unique_ptr<MapVertex> vertex)
{
	vertex->index_ = vertices_.size();
	vertices_.add(vertex.get());
	addMapObject(std::move(vertex));
	return vertices_.back();
}

// -----------------------------------------------------------------------------
// Adds [side] to the map
// -----------------------------------------------------------------------------
MapSide* MapObjectCollection::addSide(unique_ptr<MapSide> side)
{
	side->index_ = sides_.size();
	sides_.add(side.get());
	addMapObject(std::move(side));
	return sides_.back();
}

// -----------------------------------------------------------------------------
// Adds [line] to the map
// -----------------------------------------------------------------------------
MapLine* MapObjectCollection::addLine(unique_ptr<MapLine> line)
{
	line->index_ = lines_.size();
	lines_.add(line.get());
	addMapObject(std::move(line));
	return lines_.back();
}

// -----------------------------------------------------------------------------
// Adds [sector] to the map
// -----------------------------------------------------------------------------
MapSector* MapObjectCollection::addSector(unique_ptr<MapSector> sector)
{
	sector->index_ = sectors_.size();
	sectors_.add(sector.get());
	addMapObject(std::move(sector));
	return sectors_.back();
}

// -----------------------------------------------------------------------------
// Adds [thing] to the map
// -----------------------------------------------------------------------------
MapThing* MapObjectCollection::addThing(unique_ptr<MapThing> thing)
{
	thing->index_ = things_.size();
	things_.add(thing.get());
	addMapObject(std::move(thing));
	return things_.back();
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
	return sides_.back();
}

// -----------------------------------------------------------------------------
// Returns a list of objects of [type] that have a modified time later than
// [since]
// -----------------------------------------------------------------------------
vector<MapObject*> MapObjectCollection::modifiedObjects(long since, MapObject::Type type) const
{
	vector<MapObject*> modified_objects;

	if (type == MapObject::Type::Object || type == MapObject::Type::Vertex)
		vertices_.putModifiedObjects(since, modified_objects);
	if (type == MapObject::Type::Object || type == MapObject::Type::Side)
		sides_.putModifiedObjects(since, modified_objects);
	if (type == MapObject::Type::Object || type == MapObject::Type::Line)
		lines_.putModifiedObjects(since, modified_objects);
	if (type == MapObject::Type::Object || type == MapObject::Type::Sector)
		sectors_.putModifiedObjects(since, modified_objects);
	if (type == MapObject::Type::Object || type == MapObject::Type::Thing)
		things_.putModifiedObjects(since, modified_objects);

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
	case MapObject::Type::Vertex: return vertices_.modifiedSince(since);
	case MapObject::Type::Line: return lines_.modifiedSince(since);
	case MapObject::Type::Side: return sides_.modifiedSince(since);
	case MapObject::Type::Sector: return sectors_.modifiedSince(since);
	case MapObject::Type::Thing: return things_.modifiedSince(since);
	default: return false;
	}
}

// -----------------------------------------------------------------------------
// Removes any vertices not attached to any lines. Returns the number of
// vertices removed
// -----------------------------------------------------------------------------
int MapObjectCollection::removeDetachedVertices()
{
	int count = 0;
	for (int a = vertices_.size() - 1; a >= 0; a--)
	{
		if (vertices_[a]->nConnectedLines() == 0)
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
	int count = 0;
	for (int a = sides_.size() - 1; a >= 0; a--)
	{
		if (!sides_[a]->parentLine())
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
	int count = 0;
	for (int a = sectors_.size() - 1; a >= 0; a--)
	{
		if (sectors_[a]->connectedSides().empty())
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
	int count = 0;
	for (unsigned a = 0; a < lines_.size(); a++)
	{
		if (lines_[a]->v1() == lines_[a]->v2())
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
	int count = 0;
	for (unsigned a = 0; a < sides_.size(); a++)
	{
		if (!sides_[a]->sector())
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
void MapObjectCollection::rebuildConnectedLines()
{
	// Clear vertex connected lines lists
	for (auto& vertex : vertices_)
		vertex->clearConnectedLines();

	// Connect lines to their vertices
	for (auto& line : lines_)
	{
		line->v1()->connectLine(line);
		line->v2()->connectLine(line);
	}
}

// -----------------------------------------------------------------------------
// Rebuilds the connected sides lists for all map sectors
// -----------------------------------------------------------------------------
void MapObjectCollection::rebuildConnectedSides()
{
	// Clear sector connected sides lists
	for (auto& sector : sectors_)
		sector->clearConnectedSides();

	// Connect sides to their sectors
	for (auto& side : sides_)
	{
		if (side->sector())
			side->sector()->connectSide(side);
	}
}
