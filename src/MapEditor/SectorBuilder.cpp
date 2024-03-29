
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SectorBuilder.cpp
// Description: SectorBuilder class - handles sector creation from lines. Traces
//              sector outlines to build an 'edge' list (edge is basically a
//              side of a line), which is then used to create the appropriate
//              sides etc.
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
#include "SectorBuilder.h"
#include "Geometry/Geometry.h"
#include "OpenGL/OpenGL.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapVertex.h"
#include "SLADEMap/SLADEMap.h"
#include "Utility/MathStuff.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
typedef std::map<MapLine*, int> MapLineSet;

// -----------------------------------------------------------------------------
// Finds the next adjacent edge to [edge], ie the adjacent edge that creates the
// smallest angle
// -----------------------------------------------------------------------------
SectorBuilder::Edge nextEdge(SectorBuilder::Edge edge, MapLineSet& visited_lines)
{
	// Get relevant vertices
	auto vertex      = edge.line->v2(); // Vertex to be tested
	auto vertex_prev = edge.line->v1(); // 'Previous' vertex
	if (!edge.front)
	{
		vertex      = edge.line->v1();
		vertex_prev = edge.line->v2();
	}

	// Find next connected line with the lowest angle
	double              min_angle = 2 * math::PI;
	SectorBuilder::Edge next;
	for (unsigned a = 0; a < vertex->nConnectedLines(); a++)
	{
		auto line = vertex->connectedLine(a);

		// Ignore original line
		if (line == edge.line)
			continue;

		// Ignore if zero-length
		if (line->v1() == line->v2())
			continue;

		// Get next vertex
		MapVertex* vertex_next;
		bool       front = true;
		if (line->v1() == vertex)
			vertex_next = line->v2();
		else
		{
			vertex_next = line->v1();
			front       = false;
		}

		// Ignore already-traversed lines
		if (visited_lines[line] & (front ? 1 : 2))
			continue;

		// Determine angle between lines
		double angle = geometry::angle2DRad(
			{ vertex_prev->xPos(), vertex_prev->yPos() },
			{ vertex->xPos(), vertex->yPos() },
			{ vertex_next->xPos(), vertex_next->yPos() });

		// Check if minimum angle
		if (angle < min_angle)
		{
			min_angle  = angle;
			next.line  = line;
			next.front = front;
		}
	}

	// Return the next edge found
	visited_lines[next.line] |= (next.front ? 1 : 2);
	return next;
}
} // namespace


// -----------------------------------------------------------------------------
//
// SectorBuilder Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the line for the edge at [index]
// -----------------------------------------------------------------------------
MapLine* SectorBuilder::edgeLine(unsigned index) const
{
	// Check index
	if (index >= sector_edges_.size())
		return nullptr;

	return sector_edges_[index].line;
}

// -----------------------------------------------------------------------------
// Returns true if the edge at [index] is on the front side of its line
// -----------------------------------------------------------------------------
bool SectorBuilder::edgeIsFront(unsigned index) const
{
	// Check index
	if (index >= sector_edges_.size())
		return true;

	return sector_edges_[index].front;
}

// -----------------------------------------------------------------------------
// Returns true if the MapSide for the edge at [index] has been created
// -----------------------------------------------------------------------------
bool SectorBuilder::edgeSideCreated(unsigned index) const
{
	// Check index
	if (index >= sector_edges_.size())
		return false;

	return sector_edges_[index].side_created;
}

// -----------------------------------------------------------------------------
// Traces the sector outline from lines beginning at [line], on either the front
// or back side ([front])
// -----------------------------------------------------------------------------
bool SectorBuilder::traceOutline(MapLine* line, bool front)
{
	// Check line was given
	if (!line)
		return false;

	// Init outline
	o_edges_.clear();
	o_bbox_.reset();
	Edge edge(line, front);
	o_edges_.push_back(edge);
	double     edge_sum = 0;
	MapLineSet visited_lines;

	// Begin tracing
	vertex_right_ = edge.line->v1();
	for (unsigned a = 0; a < 10000; a++)
	{
		// Update edge sum (for clockwise detection)
		if (edge.front)
			edge_sum += (edge.line->x1() * edge.line->y2() - edge.line->x2() * edge.line->y1());
		else
			edge_sum += (edge.line->x2() * edge.line->y1() - edge.line->x1() * edge.line->y2());

		// Update rightmost vertex
		if (edge.line->v1()->xPos() > vertex_right_->xPos())
			vertex_right_ = edge.line->v1();
		if (edge.line->v2()->xPos() > vertex_right_->xPos())
			vertex_right_ = edge.line->v2();

		// Get next edge
		Edge edge_next = nextEdge(edge, visited_lines);
		log::info(
			4,
			"Got next edge line {}, {}",
			edge_next.line ? edge_next.line->index() : -1,
			edge_next.front ? "front" : "back");

		// Check if no valid next edge was found
		if (!edge_next.line)
		{
			// Go back along the current line
			edge_next.line  = edge.line;
			edge_next.front = !edge.front;
		}

		// Discard edge vertices
		vertex_valid_[edge_next.line->v1Index()] = false;
		vertex_valid_[edge_next.line->v2Index()] = false;

		// Check if we're back to the start
		if (edge_next.line == o_edges_[0].line && edge_next.front == o_edges_[0].front)
			break;

		// Add edge to outline
		o_edges_.push_back(edge_next);
		edge.line  = edge_next.line;
		edge.front = edge_next.front;
		o_bbox_.extend(edge.line->x1(), edge.line->y1());
		o_bbox_.extend(edge.line->x2(), edge.line->y2());
	}

	// Check if outline is clockwise
	if (edge_sum < 0)
		o_clockwise_ = true;
	else
		o_clockwise_ = false;

	// Add outline edges to sector edge list
	for (auto o_edge : o_edges_)
		sector_edges_.push_back(o_edge);

	// Trace complete
	return true;
}

// -----------------------------------------------------------------------------
// Returns the index of the edge closest to [x,y]
// -----------------------------------------------------------------------------
int SectorBuilder::nearestEdge(double x, double y) const
{
	Vec2d point(x, y);

	// Init variables
	double min_dist = 99999999;
	int    nearest  = -1;

	// Go through edges
	double dist;
	for (unsigned a = 0; a < o_edges_.size(); a++)
	{
		// Get distance to edge
		dist = geometry::distanceToLineFast(point, o_edges_[a].line->seg());

		// Check if minimum
		if (dist < min_dist)
		{
			min_dist = dist;
			nearest  = a;
		}
	}

	// Return nearest edge index
	return nearest;
}

// -----------------------------------------------------------------------------
// Returns true if the point [x,y] is within the current outline
// -----------------------------------------------------------------------------
bool SectorBuilder::pointWithinOutline(double x, double y) const
{
	Vec2d point(x, y);

	// Check with bounding box
	if (!o_bbox_.pointWithin(x, y))
	{
		// If the point is not within the bbox and the outline is clockwise,
		// it can't be within the outline
		if (o_clockwise_)
			return false;

		// On the other hand, if the outline is anticlockwise, the
		// point *must* be 'within' the outline
		else
			return true;
	}

	// Find nearest edge
	int nearest = nearestEdge(x, y);
	if (nearest >= 0)
	{
		// Check what side of the edge the point is on
		double side = geometry::lineSide(point, o_edges_[nearest].line->seg());

		// Return true if it is on the correct side
		if (side >= 0 && o_edges_[nearest].front)
			return true;
		if (side < 0 && !o_edges_[nearest].front)
			return true;
	}

	// Not within the outline
	return false;
}

// -----------------------------------------------------------------------------
// Discards any vertices outside of the current outline
// -----------------------------------------------------------------------------
void SectorBuilder::discardOutsideVertices()
{
	// Go through valid vertices list
	for (unsigned a = 0; a < vertex_valid_.size(); a++)
	{
		// Skip if already discarded
		if (!vertex_valid_[a])
			continue;

		// Discard if outside the current outline
		if (!pointWithinOutline(map_->vertex(a)->xPos(), map_->vertex(a)->yPos()))
			vertex_valid_[a] = false;
	}
}

// -----------------------------------------------------------------------------
// Finds the next closest edge outside of the current outline (that isn't part
// of the current outline)
// -----------------------------------------------------------------------------
SectorBuilder::Edge SectorBuilder::findOuterEdge() const
{
	// Check we have a rightmost vertex
	if (!vertex_right_)
		return { nullptr };

	// Init
	double   vr_x     = vertex_right_->xPos();
	double   vr_y     = vertex_right_->yPos();
	double   min_dist = 999999999;
	MapLine* nearest  = nullptr;

	// LOG_DEBUG("Finding outer edge from vertex", vertex_right, "at", vertex_right->point());

	// Fire a ray east from the vertex and find the first line it crosses
	MapLine* line = nullptr;
	for (unsigned a = 0; a < map_->nLines(); a++)
	{
		line = map_->line(a);

		// Ignore if the line is completely left of the vertex
		if (line->x1() <= vr_x && line->x2() <= vr_x)
			continue;

		// Ignore horizontal lines
		if (line->y1() == line->y2())
			continue;

		// Ignore if the line doesn't intersect the y value
		if ((line->y1() < vr_y && line->y2() < vr_y) || (line->y1() > vr_y && line->y2() > vr_y))
			continue;

		// Get x intercept
		double int_frac = (vr_y - line->y1()) / (line->y2() - line->y1());
		double int_x    = line->x1() + ((line->x2() - line->x1()) * int_frac);
		double dist     = fabs(int_x - vr_x);

		// Check if closest
		if (!nearest || dist < min_dist)
		{
			min_dist = dist;
			nearest  = line;
		}
		else if (fabs(dist - min_dist) < 0.001)
		{
			// In the case of a tie, use the distance to each line as a
			// tiebreaker -- this fixes cases where the ray hits a vertex
			// shared by two lines.  Choosing the further line would mean
			// choosing an inner edge, which is clearly wrong.
			double line_dist    = geometry::distanceToLineFast(vertex_right_->position(), line->seg());
			double nearest_dist = geometry::distanceToLineFast(vertex_right_->position(), nearest->seg());
			if (line_dist < nearest_dist)
			{
				min_dist = dist;
				nearest  = line;
			}
		}
	}

	// Check for valid line
	if (!nearest)
		return { nullptr };


	// Determine the edge side
	double side = geometry::lineSide(vertex_right_->position(), nearest->seg());
	// LOG_DEBUG("Found next outer line", nearest, "on side", side);
	if (side >= 0)
		return Edge{ nearest, true };
	else
		return Edge{ nearest, false };
}

// -----------------------------------------------------------------------------
// Find the closest edge within the current outline (that isn't part of the
// current outline)
// -----------------------------------------------------------------------------
SectorBuilder::Edge SectorBuilder::findInnerEdge()
{
	// Find rightmost non-discarded vertex
	vertex_right_ = nullptr;
	for (unsigned a = 0; a < vertex_valid_.size(); a++)
	{
		// Ignore if discarded
		if (!vertex_valid_[a])
			continue;

		// Set rightmost if no current rightmost vertex
		if (!vertex_right_)
		{
			vertex_right_ = map_->vertex(a);
			continue;
		}

		// Check if the vertex is rightmost
		if (map_->vertex(a)->xPos() > vertex_right_->xPos())
			vertex_right_ = map_->vertex(a);
	}

	// If no vertex was found, we're done
	if (!vertex_right_)
		return { nullptr };

	// Go through vertex's connected lines, to find
	// the line with the smallest angle parallel with
	// the right side of the bbox
	MapLine* eline     = nullptr;
	double   min_angle = 999999;
	for (unsigned a = 0; a < vertex_right_->nConnectedLines(); a++)
	{
		auto line = vertex_right_->connectedLine(a);

		// Ignore if zero-length
		if (line->v1() == line->v2())
			continue;

		// Get opposite vertex
		MapVertex* opposite;
		if (line->v1() == vertex_right_)
			opposite = line->v2();
		else
			opposite = line->v1();

		// Determine angle
		double angle = geometry::angle2DRad(
			Vec2d(vertex_right_->xPos() + 32, vertex_right_->yPos()),
			Vec2d(vertex_right_->xPos(), vertex_right_->yPos()),
			Vec2d(opposite->xPos(), opposite->yPos()));

		// Check if minimum
		if (angle < min_angle)
		{
			min_angle = angle;
			eline     = line;
		}
	}

	// If no line was found, something is wrong
	// (the vertex may have no attached lines)
	if (!eline)
	{
		// Discard vertex and try again
		vertex_valid_[vertex_right_->index()] = false;
		return findInnerEdge();
	}

	// Determine appropriate side
	if (vertex_right_ == eline->v1())
		return { eline, true };
	else
		return { eline, false };
}

// -----------------------------------------------------------------------------
// Finds an appropriate existing sector to copy properties from, for the new
// sector being built
// -----------------------------------------------------------------------------
MapSector* SectorBuilder::findCopySector() const
{
	// Go through new sector edges
	MapSector* sector_copy = nullptr;
	for (auto& edge : sector_edges_)
	{
		// Check if the edge's corresponding MapSide has a front sector
		if (edge.line->frontSector())
		{
			// Set sector to copy
			sector_copy = edge.line->frontSector();

			// If the edge is a front edge, use this sector and ignore all else
			if (edge.front)
				break;
		}

		// Check if the edge's corresponding MapSide has a back sector
		if (edge.line->backSector())
		{
			// Set sector to copy
			sector_copy = edge.line->backSector();

			// If the edge is a back edge, use this sector and ignore all else
			if (!edge.front)
				break;
		}
	}

	return sector_copy;
}

// -----------------------------------------------------------------------------
// Finds any existing sector that is already part of the traced new sector
// -----------------------------------------------------------------------------
MapSector* SectorBuilder::findExistingSector(vector<MapSide*>& sides_ignore) const
{
	// Go through new sector edges
	MapSector* sector          = nullptr;
	MapSector* sector_priority = nullptr;
	for (auto& edge : sector_edges_)
	{
		// Check if the edge's corresponding MapSide has a front sector
		if (edge.front && edge.line->frontSector())
		{
			// return sector_edges[a].line->frontSector();
			if (VECTOR_EXISTS(sides_ignore, edge.line->s1()))
				sector = edge.line->frontSector();
			else
				sector_priority = edge.line->frontSector();
		}

		// Check if the edge's corresponding MapSide has a back sector
		if (!edge.front && edge.line->backSector())
		{
			// return sector_edges[a].line->backSector();
			if (VECTOR_EXISTS(sides_ignore, edge.line->s2()))
				sector = edge.line->backSector();
			else
				sector_priority = edge.line->backSector();
		}
	}

	if (sector_priority)
		return sector_priority;
	else
		return sector;
}

// -----------------------------------------------------------------------------
// Checks if the traced sector is valid (ie. all edges are currently the same
// (existing) sector)
// -----------------------------------------------------------------------------
bool SectorBuilder::isValidSector() const
{
	if (sector_edges_.empty())
		return false;

	// Get first edge's sector
	auto sector = sector_edges_[0].front ? sector_edges_[0].line->frontSector() : sector_edges_[0].line->backSector();
	if (!sector)
		return false; // Sector is invalid if any edge has no current sector

	// Go through subsequent edges
	for (unsigned a = 1; a < sector_edges_.size(); a++)
	{
		// Get edge sector
		MapSector* ssector;
		if (sector_edges_[a].front)
			ssector = sector_edges_[a].line->frontSector();
		else
			ssector = sector_edges_[a].line->backSector();

		// Check if different
		if (sector != ssector)
			return false;
	}

	// Check the entire sector was traced
	if (sector && (sector->connectedSides().size() != sector_edges_.size()))
		return false;

	return sector != nullptr;
}

// -----------------------------------------------------------------------------
// Traces all edges to build a closed sector starting from [line]
// -----------------------------------------------------------------------------
bool SectorBuilder::traceSector(SLADEMap* map, MapLine* line, bool front)
{
	// Check info was given
	if (!line || !map)
		return false;

	// Init
	map_ = map;
	sector_edges_.clear();
	error_ = "Unknown error";

	// Create valid vertices list
	vertex_valid_.clear();
	for (unsigned a = 0; a < map->nVertices(); a++)
		vertex_valid_.push_back(true);

	// Find outmost outline
	for (unsigned a = 0; a < 10000; a++)
	{
		// Trace outline
		if (!traceOutline(line, front))
			break;

		// Discard any vertices outside the traced outline
		discardOutsideVertices();

		// If it is clockwise, we've found the outmost outline
		if (o_clockwise_)
			break;

		// Otherwise, find the next edge outside the outline
		auto next = findOuterEdge();

		// If none was found, we're outside the map
		if (!next.line)
		{
			// sector_edges.clear();
			error_ = "Outside map area";
			return false;
		}

		// Repeat with this edge
		line  = next.line;
		front = next.front;

		// Check for possible infinite loop
		if (a == 9999)
		{
			error_ = "Invalid map geometry";
			return false;
		}
	}

	// Trace all inner outlines, by tracing from the rightmost vertex
	// until all vertices have been discarded
	for (unsigned a = 0; a < 10000; a++)
	{
		// Get inner edge
		auto edge = findInnerEdge();

		// Check if we're done
		if (!edge.line)
			break;

		// Trace outline from edge
		if (!traceOutline(edge.line, edge.front))
			break;

		// Discard any vertices outside the traced outline
		discardOutsideVertices();

		// Check for possible infinite loop
		if (a == 9999)
		{
			error_ = "Invalid map geometry";
			return false;
		}
	}

	return true;
}

// -----------------------------------------------------------------------------
// Sets all traced edges to [sector], or creates a new sector using properties
// from [sector_copy] if none given
// -----------------------------------------------------------------------------
void SectorBuilder::createSector(MapSector* sector, MapSector* sector_copy)
{
	// Create the sector if needed
	if (!sector)
	{
		sector = map_->createSector();
		if (!sector_copy)
			sector_copy = findCopySector(); // Find potential sector to copy if none specified
		if (sector_copy)
			sector->copy(sector_copy);
	}

	// Set sides to new sector
	for (auto& edge : sector_edges_)
		edge.side_created = map_->setLineSector(edge.line->index(), sector->index(), edge.front);
}

// -----------------------------------------------------------------------------
// Draws lines showing the currently traced edges
// -----------------------------------------------------------------------------
void SectorBuilder::drawResult() const
{
	glDisable(GL_TEXTURE_2D);
	gl::setColour(255, 255, 255, 255, gl::Blend::Normal);

	// Go through sector edges
	for (auto& edge : sector_edges_)
	{
		// Setup colour
		if (edge.front)
		{
			glLineWidth(2.0f);
			glColor3f(0.0f, 1.0f, 0.0f);
		}
		else
		{
			glLineWidth(3.0f);
			glColor3f(0.0f, 0.0f, 1.0f);
		}

		// Draw line
		glBegin(GL_LINES);
		glVertex2d(edge.line->x1(), edge.line->y1());
		glVertex2d(edge.line->x2(), edge.line->y2());
		glEnd();
	}
}
