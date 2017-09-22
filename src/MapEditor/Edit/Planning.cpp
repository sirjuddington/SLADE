
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Planning.cpp
// Description: Map Editor Planning Mode implementation
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
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#include "MapEditor/MapEditContext.h"

using namespace MapEditor;


// ----------------------------------------------------------------------------
//
// PlanNote Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// PlanNote::point
//
// Returns the object point [point]
// ----------------------------------------------------------------------------
fpoint2_t PlanNote::point(Point point)
{
	/*if (point == Point::Text)
		return { position_.x, position_.y - 20 };
	else*/
		return position_;
}


// ----------------------------------------------------------------------------
//
// Planning Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// PlanNote::Planning
//
// Planning class constructor
// ----------------------------------------------------------------------------
Planning::Planning(MapEditContext& context) :
	context_{ context }
{
}

// ----------------------------------------------------------------------------
// PlanNote::object
//
// Returns the object of [type] at [index], or nullptr if [type] and/or [index]
// is invalid
// ----------------------------------------------------------------------------
MapObject* Planning::object(ItemType type, unsigned index) const
{
	switch (type)
	{
	case ItemType::PlanVertex:	return vertex(index);
	case ItemType::PlanLine:	return line(index);
	case ItemType::PlanNote:	return note(index);
	default:					return nullptr;
	}
}

// ----------------------------------------------------------------------------
// PlanNote::createLines
//
// Creates planning lines between [points]
// ----------------------------------------------------------------------------
void Planning::createLines(vector<fpoint2_t>& points)
{
	for (auto a = 0u; a < points.size() - 1; a++)
	{
		auto v1 = createVertex(points[a].x, points[a].y);
		auto v2 = createVertex(points[a + 1].x, points[a + 1].y);
		lines_.push_back(std::make_unique<MapLine>(v1, v2, nullptr, nullptr));
	}

	updateIndices();
}

// ----------------------------------------------------------------------------
// PlanNote::createVertex
//
// Creates a planning vertex at [x,y]
// ----------------------------------------------------------------------------
MapVertex* Planning::createVertex(double x, double y)
{
	// Check if a vertex already exists at x,y
	for (auto& vertex : vertices_)
		if (vertex->xPos() == x && vertex->yPos() == y)
			return vertex.get();

	// Create vertex
	vertices_.push_back(std::make_unique<MapVertex>(x, y));
	return vertices_.back().get();
}

// ----------------------------------------------------------------------------
// PlanNote::createNote
//
// Creates a planning note at [x,y]
// ----------------------------------------------------------------------------
PlanNote* Planning::createNote(double x, double y)
{
	// Create note
	notes_.push_back(std::make_unique<PlanNote>(x, y, S_FMT("Note #%d", notes_.size())));
	notes_.back().get()->setIndex(notes_.size() - 1);
	return notes_.back().get();
}

// ----------------------------------------------------------------------------
// PlanNote::deleteVertex
//
// Deletes the given planning [vertex]
// ----------------------------------------------------------------------------
bool Planning::deleteVertex(MapVertex* vertex)
{
	// Delete lines attached to the vertex
	for (auto a = 0u; a < lines_.size(); ++a)
		if (lines_[a]->v1() == vertex || lines_[a]->v2() == vertex)
		{
			lines_[a]->disconnectFromVertices();
			lines_.erase(lines_.begin() + a);
			a--;
		}

	// Delete the vertex
	bool deleted = false;
	for (auto i = vertices_.begin(); i != vertices_.end(); ++i)
		if (i->get() == vertex)
		{
			vertices_.erase(i);
			deleted = true;
			break;
		}

	updateIndices();
	return deleted;
}

// ----------------------------------------------------------------------------
// PlanNote::deleteLine
//
// Deletes the given planning [line]
// ----------------------------------------------------------------------------
bool Planning::deleteLine(MapLine* line)
{
	// Delete the line
	bool deleted = false;
	for (auto i = lines_.begin(); i != lines_.end(); ++i)
		if (i->get() == line)
		{
			line->disconnectFromVertices();
			lines_.erase(i);
			deleted = true;
			break;
		}

	return deleted;
}

// ----------------------------------------------------------------------------
// PlanNote::nearestObject
//
// Returns the nearest planning object to [point], or nullptr if nothing is
// closer than [min] distance
// ----------------------------------------------------------------------------
MapObject* Planning::nearestObject(fpoint2_t point, double min)
{
	double min_dist = 999999999;
	double dist = 0;

	// Find nearest planning vertex
	MapVertex* nearest_vertex = nullptr;
	for (auto& vertex : vertices_)
	{
		dist = point.distance_to(vertex->pos());
		if (dist <= min && dist < min_dist)
		{
			nearest_vertex = vertex.get();
			min_dist = dist;
		}
	}

	// Find nearest planning note
	PlanNote* nearest_note = nullptr;
	for (auto& note : notes_)
	{
		dist = point.distance_to(note->pos());
		if (dist <= min && dist < min_dist)
		{
			nearest_note = note.get();
			nearest_vertex = nullptr;
			min_dist = dist;
		}
	}

	if (nearest_vertex)
		return nearest_vertex;

	// Find nearest planning line
	MapLine* nearest_line = nullptr;
	for (auto& line : lines_)
	{
		// Check with line bounding box first (since we have a minimum distance)
		fseg2_t bbox = line->seg();
		bbox.expand(min, min);
		if (!bbox.contains(point))
			continue;

		// Calculate distance to line
		dist = line->distanceTo(point);

		// Check if it's nearer than the previous nearest
		if (dist < min_dist && dist <= min)
		{
			nearest_vertex = nullptr;
			nearest_note = nullptr;
			nearest_line = line.get();
			min_dist = dist;
		}
	}

	return nearest_note ? (MapObject*)nearest_note : (MapObject*)nearest_line;
}

// ----------------------------------------------------------------------------
// PlanNote::updateIndices
//
// Updates the indices of all planning objects
// ----------------------------------------------------------------------------
void Planning::updateIndices()
{
	for (auto a = 0u; a < vertices_.size(); a++)
		vertices_[a]->setIndex(a);

	for (auto a = 0u; a < lines_.size(); a++)
		lines_[a]->setIndex(a);

	for (auto a = 0u; a < notes_.size(); a++)
		notes_[a]->setIndex(a);
}

// ----------------------------------------------------------------------------
// PlanNote::deleteDetachedVertices
//
// Deletes any planning vertices that are not connected to any lines
// ----------------------------------------------------------------------------
void Planning::deleteDetachedVertices()
{
	for (auto a = 0u; a < vertices_.size(); ++a)
		if (vertices_[a].get()->nConnectedLines() == 0)
		{
			vertices_.erase(vertices_.begin() + a);
			--a;
		}

	updateIndices();
}

// ----------------------------------------------------------------------------
// PlanNote::clearFilter
//
// Un-filters all planning objects
// ----------------------------------------------------------------------------
void Planning::clearFilter()
{
	for (auto& line : lines_)
		line->filter(false);
	for (auto& note : notes_)
		note->filter(false);
}
