
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    SectorBuilder.cpp
 * Description: SectorBuilder class - handles sector creation from
 *              lines. Traces sector outlines to build an 'edge'
 *              list (edge is basically a side of a line), which
 *              is then used to create the appropriate sides etc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "SectorBuilder.h"
#include "SLADEMap.h"
#include "MathStuff.h"
#include "OpenGL.h"


/*******************************************************************
 * SECTORBUILDER CLASS FUNCTIONS
 *******************************************************************/

/* SectorBuilder::SectorBuilder
 * SectorBuilder class constructor
 *******************************************************************/
SectorBuilder::SectorBuilder()
{
	// Init variables
	vertex_right = NULL;
	map = NULL;
}

/* SectorBuilder::~SectorBuilder
 * SectorBuilder class destructor
 *******************************************************************/
SectorBuilder::~SectorBuilder()
{
}

/* SectorBuilder::getEdgeLine
 * Returns the line for the edge at [index]
 *******************************************************************/
MapLine* SectorBuilder::getEdgeLine(unsigned index)
{
	// Check index
	if (index >= sector_edges.size())
		return NULL;

	return sector_edges[index].line;
}

/* SectorBuilder::edgeIsFront
 * Returns true if the edge at [index] is on the front side of its
 * line
 *******************************************************************/
bool SectorBuilder::edgeIsFront(unsigned index)
{
	// Check index
	if (index >= sector_edges.size())
		return true;

	return sector_edges[index].front;
}

/* SectorBuilder::edgeSideCreated
 * Returns true if the MapSide for the edge at [index] has been
 * created
 *******************************************************************/
bool SectorBuilder::edgeSideCreated(unsigned index)
{
	// Check index
	if (index >= sector_edges.size())
		return false;

	return sector_edges[index].side_created;
}

/* SectorBuilder::nextEdge
 * Finds the next adjacent edge to [edge], ie the adjacent edge that
 * creates the smallest angle
 *******************************************************************/
SectorBuilder::edge_t SectorBuilder::nextEdge(SectorBuilder::edge_t edge)
{
	// Get relevant vertices
	MapVertex* vertex = edge.line->v2();		// Vertex to be tested
	MapVertex* vertex_prev = edge.line->v1();	// 'Previous' vertex
	if (!edge.front)
	{
		vertex = edge.line->v1();
		vertex_prev = edge.line->v2();
	}

	// Find next connected line with the lowest angle
	double min_angle = 2*PI;
	edge_t next;
	for (unsigned a = 0; a < vertex->nConnectedLines(); a++)
	{
		MapLine* line = vertex->connectedLine(a);

		// Ignore original line
		if (line == edge.line)
			continue;

		// Ignore if zero-length
		if (line->v1() == line->v2())
			continue;

		// Get next vertex
		MapVertex* vertex_next;
		bool front = true;
		if (line->v1() == vertex)
			vertex_next = line->v2();
		else
		{
			vertex_next = line->v1();
			front = false;
		}

		// Determine angle between lines
		double angle = MathStuff::angle2DRad(fpoint2_t(vertex_prev->xPos(), vertex_prev->yPos()),
		                                     fpoint2_t(vertex->xPos(), vertex->yPos()),
		                                     fpoint2_t(vertex_next->xPos(), vertex_next->yPos()));

		// Check if minimum angle
		if (angle < min_angle)
		{
			min_angle = angle;
			next.line = line;
			next.front = front;
		}
	}

	// Return the next edge found
	return next;
}

/* SectorBuilder::traceOutline
 * Traces the sector outline from lines beginning at [line], on
 * either the front or back side ([front])
 *******************************************************************/
bool SectorBuilder::traceOutline(MapLine* line, bool front)
{
	// Check line was given
	if (!line)
		return false;

	// Init outline
	o_edges.clear();
	o_bbox.reset();
	edge_t edge(line, front);
	o_edges.push_back(edge);
	double edge_sum = 0;

	// Begin tracing
	vertex_right = edge.line->v1();
	for (unsigned a = 0; a < 10000; a++)
	{
		// Update edge sum (for clockwise detection)
		if (edge.front)
			edge_sum += (edge.line->x1()*edge.line->y2() - edge.line->x2()*edge.line->y1());
		else
			edge_sum += (edge.line->x2()*edge.line->y1() - edge.line->x1()*edge.line->y2());

		// Update rightmost vertex
		if (edge.line->v1()->xPos() > vertex_right->xPos())
			vertex_right = edge.line->v1();
		if (edge.line->v2()->xPos() > vertex_right->xPos())
			vertex_right = edge.line->v2();

		// Get next edge
		edge_t edge_next = nextEdge(edge);
		LOG_MESSAGE(4, "Got next edge line %d", edge_next.line ? edge_next.line->getIndex() : -1);

		// Check if no valid next edge was found
		if (!edge_next.line)
		{
			// Go back along the current line
			edge_next.line = edge.line;
			edge_next.front = !edge.front;
		}

		// Discard edge vertices
		vertex_valid[edge_next.line->v1Index()] = false;
		vertex_valid[edge_next.line->v2Index()] = false;

		// Check if we're back to the start
		if (edge_next.line == o_edges[0].line &&
		        edge_next.front == o_edges[0].front)
			break;

		// Add edge to outline
		o_edges.push_back(edge_next);
		edge.line = edge_next.line;
		edge.front = edge_next.front;
		o_bbox.extend(edge.line->x1(), edge.line->y1());
		o_bbox.extend(edge.line->x2(), edge.line->y2());
	}

	// Check if outline is clockwise
	if (edge_sum < 0)
		o_clockwise = true;
	else
		o_clockwise = false;

	// Add outline edges to sector edge list
	for (unsigned a = 0; a < o_edges.size(); a++)
		sector_edges.push_back(o_edges[a]);

	// Trace complete
	return true;
}

/* SectorBuilder::nearestEdge
 * Returns the index of the edge closest to [x,y]
 *******************************************************************/
int SectorBuilder::nearestEdge(double x, double y)
{
	// Init variables
	double min_dist = 99999999;
	int nearest = -1;

	// Go through edges
	double dist;
	for (unsigned a = 0; a < o_edges.size(); a++)
	{
		// Get distance to edge
		dist = MathStuff::distanceToLineFast(x, y,
		                                     o_edges[a].line->x1(), o_edges[a].line->y1(),
		                                     o_edges[a].line->x2(), o_edges[a].line->y2());

		// Check if minimum
		if (dist < min_dist)
		{
			min_dist = dist;
			nearest = a;
		}
	}

	// Return nearest edge index
	return nearest;
}

/* SectorBuilder::pointWithinOutline
 * Returns true if the point [x,y] is within the current outline
 *******************************************************************/
bool SectorBuilder::pointWithinOutline(double x, double y)
{
	// Check with bounding box
	if (!o_bbox.point_within(x, y))
	{
		// If the point is not within the bbox and the outline is clockwise,
		// it can't be within the outline
		if (o_clockwise)
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
		double side = MathStuff::lineSide(x, y,
		                                  o_edges[nearest].line->x1(), o_edges[nearest].line->y1(),
		                                  o_edges[nearest].line->x2(), o_edges[nearest].line->y2());

		// Return true if it is on the correct side
		if (side >= 0 && o_edges[nearest].front)
			return true;
		if (side < 0 && !o_edges[nearest].front)
			return true;
	}

	// Not within the outline
	return false;
}

/* SectorBuilder::discardOutsideVertices
 * Discards any vertices outside of the current outline
 *******************************************************************/
void SectorBuilder::discardOutsideVertices()
{
	// Go through valid vertices list
	for (unsigned a = 0; a < vertex_valid.size(); a++)
	{
		// Skip if already discarded
		if (!vertex_valid[a])
			continue;

		// Discard if outside the current outline
		if (!pointWithinOutline(map->getVertex(a)->xPos(), map->getVertex(a)->yPos()))
			vertex_valid[a] = false;
	}
}

/* SectorBuilder::findOuterEdge
 * Finds the next closest edge outside of the current outline (that
 * isn't part of the current outline)
 *******************************************************************/
SectorBuilder::edge_t SectorBuilder::findOuterEdge()
{
	// Check we have a rightmost vertex
	if (!vertex_right)
		return edge_t(NULL);

	// Init
	double vr_x = vertex_right->xPos();
	double vr_y = vertex_right->yPos();
	double min_dist = 999999999;
	MapLine* nearest = NULL;

	//wxLogMessage("Find outer edge from vertex %d", vertex_right->getIndex());

	// Go through map lines
	MapLine* line = NULL;
	for (unsigned a = 0; a < map->nLines(); a++)
	{
		line = map->getLine(a);

		// Ignore if the line is completely left of the vertex
		if (line->x1() <= vr_x && line->x2() <= vr_x)
			continue;

		// Ignore horizontal lines
		if (line->y1() == line->y2())
			continue;

		// Ignore if the line doesn't intersect the y value
		if ((line->y1() < vr_y && line->y2() < vr_y) ||
		        (line->y1() > vr_y && line->y2() > vr_y))
			continue;

		// Get x intercept
		double int_frac = (vr_y - line->y1()) / (line->y2() - line->y1());
		double int_x = line->x1() + ((line->x2() - line->x1()) * int_frac);

		// Check if closest
		if (int_x - vr_x < min_dist)
		{
			min_dist = int_x - vr_x;
			nearest = line;
		}
	}

	// Check for valid line
	if (!nearest)
		return edge_t(NULL);

	//wxLogMessage("Found next outer line %d", nearest->getIndex());

	// Determine the edge side
	double side = MathStuff::lineSide(vr_x, vr_y, nearest->x1(), nearest->y1(), nearest->x2(), nearest->y2());
	if (side >= 0)
		return edge_t(nearest, true);
	else
		return edge_t(nearest, false);
}

/* SectorBuilder::SectorBuilder
 * Find the closest edge within the current outline (that isn't part
 * of the current outline)
 *******************************************************************/
SectorBuilder::edge_t SectorBuilder::findInnerEdge()
{
	// Find rightmost non-discarded vertex
	vertex_right = NULL;
	for (unsigned a = 0; a < vertex_valid.size(); a++)
	{
		// Ignore if discarded
		if (!vertex_valid[a])
			continue;

		// Set rightmost if no current rightmost vertex
		if (!vertex_right)
		{
			vertex_right = map->getVertex(a);
			continue;
		}

		// Check if the vertex is rightmost
		if (map->getVertex(a)->xPos() > vertex_right->xPos())
			vertex_right = map->getVertex(a);
	}

	// If no vertex was found, we're done
	if (!vertex_right)
		return edge_t(NULL);

	// Go through vertex's connected lines, to find
	// the line with the smallest angle parallel with
	// the right side of the bbox
	MapLine* eline = NULL;
	double min_angle = 999999;
	for (unsigned a = 0; a < vertex_right->nConnectedLines(); a++)
	{
		MapLine* line = vertex_right->connectedLine(a);

		// Ignore if zero-length
		if (line->v1() == line->v2())
			continue;

		// Get opposite vertex
		MapVertex* opposite;
		if (line->v1() == vertex_right)
			opposite = line->v2();
		else
			opposite = line->v1();

		// Determine angle
		double angle = MathStuff::angle2DRad(fpoint2_t(vertex_right->xPos() + 32, vertex_right->yPos()),
		                                     fpoint2_t(vertex_right->xPos(), vertex_right->yPos()),
		                                     fpoint2_t(opposite->xPos(), opposite->yPos()));

		// Check if minimum
		if (angle < min_angle)
		{
			min_angle = angle;
			eline = line;
		}
	}

	// If no line was found, something is wrong
	// (the vertex may have no attached lines)
	if (!eline)
	{
		// Discard vertex and try again
		vertex_valid[vertex_right->getIndex()] = false;
		return findInnerEdge();
	}

	// Determine appropriate side
	if (vertex_right == eline->v1())
		return edge_t(eline, true);
	else
		return edge_t(eline, false);
}

/* SectorBuilder::findCopySector
 * Finds an appropriate existing sector to copy properties from, for
 * the new sector being built
 *******************************************************************/
MapSector* SectorBuilder::findCopySector()
{
	// Go through new sector edges
	MapSector* sector_copy = NULL;
	for (unsigned a = 0; a < sector_edges.size(); a++)
	{
		// Check if the edge's corresponding MapSide has a front sector
		if (sector_edges[a].line->frontSector())
		{
			// Set sector to copy
			sector_copy = sector_edges[a].line->frontSector();

			// If the edge is a front edge, use this sector and ignore all else
			if (sector_edges[a].front)
				break;
		}

		// Check if the edge's corresponding MapSide has a back sector
		if (sector_edges[a].line->backSector())
		{
			// Set sector to copy
			sector_copy = sector_edges[a].line->backSector();

			// If the edge is a back edge, use this sector and ignore all else
			if (!sector_edges[a].front)
				break;
		}
	}

	return sector_copy;
}

/* SectorBuilder::findExistingSector
 * Finds any existing sector that is already part of the traced new
 * sector
 *******************************************************************/
MapSector* SectorBuilder::findExistingSector(vector<MapSide*>& sides_ignore)
{
	// Go through new sector edges
	MapSector* sector = NULL;
	MapSector* sector_priority = NULL;
	for (unsigned a = 0; a < sector_edges.size(); a++)
	{
		// Check if the edge's corresponding MapSide has a front sector
		if (sector_edges[a].front && sector_edges[a].line->frontSector())
		{
			//return sector_edges[a].line->frontSector();
			if (VECTOR_EXISTS(sides_ignore, sector_edges[a].line->s1()))
				sector = sector_edges[a].line->frontSector();
			else
				sector_priority = sector_edges[a].line->frontSector();
		}

		// Check if the edge's corresponding MapSide has a back sector
		if (!sector_edges[a].front && sector_edges[a].line->backSector())
		{
			//return sector_edges[a].line->backSector();
			if (VECTOR_EXISTS(sides_ignore, sector_edges[a].line->s2()))
				sector = sector_edges[a].line->backSector();
			else
				sector_priority = sector_edges[a].line->backSector();
		}
	}

	if (sector_priority)
		return sector_priority;
	else
		return sector;
}

/* SectorBuilder::isValidSector
 * Checks if the traced sector is valid (ie. all edges are currently
 * the same (existing) sector)
 *******************************************************************/
bool SectorBuilder::isValidSector()
{
	if (sector_edges.empty())
		return false;

	// Get first edge's sector
	MapSector* sector = sector_edges[0].front ? sector_edges[0].line->frontSector() : sector_edges[0].line->backSector();
	if (!sector)
		return false;	// Sector is invalid if any edge has no current sector

	// Go through subsequent edges
	for (unsigned a = 1; a < sector_edges.size(); a++)
	{
		// Get edge sector
		MapSector* ssector;
		if (sector_edges[a].front)
			ssector = sector_edges[a].line->frontSector();
		else
			ssector = sector_edges[a].line->backSector();

		// Check if different
		if (sector != ssector)
			return false;
	}

	// Check the entire sector was traced
	if (sector && (sector->connectedSides().size() != sector_edges.size()))
		return false;

	return sector != NULL;
}

/* SectorBuilder::traceSector
 * Traces all edges to build a closed sector starting from [line]
 *******************************************************************/
bool SectorBuilder::traceSector(SLADEMap* map, MapLine* line, bool front)
{
	// Check info was given
	if (!line || !map)
		return false;

	// Init
	this->map = map;
	sector_edges.clear();
	error = "Unknown error";

	// Create valid vertices list
	vertex_valid.clear();
	for (unsigned a = 0; a < map->nVertices(); a++)
		vertex_valid.push_back(true);

	// Find outmost outline
	for (unsigned a = 0; a < 10000; a++)
	{
		// Trace outline
		if (!traceOutline(line, front))
			break;

		// Discard any vertices outside the traced outline
		discardOutsideVertices();

		// If it is clockwise, we've found the outmost outline
		if (o_clockwise)
			break;

		// Otherwise, find the next edge outside the outline
		edge_t next = findOuterEdge();

		// If none was found, we're outside the map
		if (!next.line)
		{
			//sector_edges.clear();
			error = "Outside map area";
			return false;
		}

		// Repeat with this edge
		line = next.line;
		front = next.front;

		// Check for possible infinite loop
		if (a == 9999)
		{
			error = "Invalid map geometry";
			return false;
		}
	}

	// Trace all inner outlines, by tracing from the rightmost vertex
	// until all vertices have been discarded
	for (unsigned a = 0; a < 10000; a++)
	{
		// Get inner edge
		edge_t edge = findInnerEdge();

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
			error = "Invalid map geometry";
			return false;
		}
	}

	return true;
}

/* SectorBuilder::createSector
 * Sets all traced edges to [sector], or creates a new sector using
 * properties from [sector_copy] if none given
 *******************************************************************/
void SectorBuilder::createSector(MapSector* sector, MapSector* sector_copy)
{
	// Create the sector if needed
	if (!sector)
	{
		sector = map->createSector();
		if (!sector_copy)
			sector_copy = findCopySector();		// Find potential sector to copy if none specified
		if (sector_copy)
			sector->copy(sector_copy);
	}

	// Set sides to new sector
	for (unsigned a = 0; a < sector_edges.size(); a++)
		sector_edges[a].side_created = map->setLineSector(sector_edges[a].line->getIndex(), sector->getIndex(), sector_edges[a].front);
}

/* SectorBuilder::drawResult
 * Draws lines showing the currently traced edges
 *******************************************************************/
void SectorBuilder::drawResult()
{
	glDisable(GL_TEXTURE_2D);
	OpenGL::setColour(255, 255, 255, 255, 0);

	// Go through sector edges
	for (unsigned a = 0; a < sector_edges.size(); a++)
	{
		// Setup colour
		if (sector_edges[a].front)
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
		glVertex2d(sector_edges[a].line->x1(), sector_edges[a].line->y1());
		glVertex2d(sector_edges[a].line->x2(), sector_edges[a].line->y2());
		glEnd();
	}
}
