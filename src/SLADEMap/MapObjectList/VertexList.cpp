
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    VertexList.cpp
// Description: A (non-owning) list of map vertices. Includes std::vector-like
//              API for accessing items and some misc functions to get info
//              about the contained items.
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
#include "VertexList.h"
#include "Geometry/Geometry.h"
#include "Geometry/Rect.h"
#include "SLADEMap/MapObject/MapVertex.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// VertexList Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the vertex closest to the point, or null if none found.
// Igonres any vertices further away than [min]
// -----------------------------------------------------------------------------
MapVertex* VertexList::nearest(const Vec2d& point, double min) const
{
	// Go through vertices
	double     dist;
	double     min_dist = 999999999;
	MapVertex* nearest  = nullptr;
	for (const auto& vertex : objects_)
	{
		// Get 'quick' distance (no need to get real distance)
		dist = geometry::taxicabDistance(point, vertex->position());

		// Check if it's nearer than the previous nearest
		if (dist < min_dist)
		{
			nearest  = vertex;
			min_dist = dist;
		}
	}

	// Now determine the real distance to the closest vertex,
	// to check for minimum hilight distance
	if (nearest)
	{
		double rdist = glm::distance(nearest->position(), point);
		if (rdist > min)
			return nullptr;
	}

	return nearest;
}

// -----------------------------------------------------------------------------
// Returns the vertex at [x,y], or null if none there
// -----------------------------------------------------------------------------
MapVertex* VertexList::vertexAt(double x, double y) const
{
	// Go through all vertices
	for (auto& vertex : objects_)
	{
		if (vertex->position_.x == x && vertex->position_.y == y)
			return vertex;
	}

	// No vertex at [x,y]
	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the first vertex that [line] crosses over
// -----------------------------------------------------------------------------
MapVertex* VertexList::firstCrossed(const Seg2d& line) const
{
	// Go through vertices
	MapVertex* cv       = nullptr;
	double     min_dist = 999999;
	for (const auto& vertex : objects_)
	{
		auto point = vertex->position();

		// Skip if outside line bbox
		if (!line.contains(point))
			continue;

		// Skip if it's at an end of the line
		if (point == line.start() || point == line.end())
			continue;

		// Check if on line
		if (geometry::distanceToLineFast(point, line) == 0)
		{
			// Check distance between line start and vertex
			double dist = glm::distance(line.start(), point);
			if (dist < min_dist)
			{
				cv       = vertex;
				min_dist = dist;
			}
		}
	}

	// Return closest overlapping vertex to line start
	return cv;
}
