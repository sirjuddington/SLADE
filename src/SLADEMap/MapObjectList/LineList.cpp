
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    LineList.cpp
// Description: A (non-owning) list of map lines. Includes std::vector-like API
//              for accessing items and some misc functions to get info about
//              the contained items.
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
#include "LineList.h"
#include "Game/ActionSpecial.h"
#include "Game/Configuration.h"
#include "Game/Game.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/SLADEMap.h"
#include "Utility/MathStuff.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// LineList Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the line closest to the point, or null if none is found.
// Ignores lines further away than [mindist]
// -----------------------------------------------------------------------------
MapLine* LineList::nearest(const Vec2d& point, double min) const
{
	// Go through lines
	double   dist;
	double   min_dist = min;
	MapLine* nearest  = nullptr;
	for (const auto& line : objects_)
	{
		// Check with line bounding box first (since we have a minimum distance)
		auto bbox = line->seg();
		bbox.expand(min, min);
		if (!bbox.contains(point))
			continue;

		// Calculate distance to line
		dist = line->distanceTo(point);

		// Check if it's nearer than the previous nearest
		if (dist < min_dist && dist < min)
		{
			nearest  = line;
			min_dist = dist;
		}
	}

	return nearest;
}

// -----------------------------------------------------------------------------
// Returns the first line in the list with vertices [v1] and [v2].
// If [reverse] is false, only looks for lines with first vertex [v1] and second
// vertex [v2] (not the other way around)
// -----------------------------------------------------------------------------
MapLine* LineList::withVertices(const MapVertex* v1, const MapVertex* v2, bool reverse) const
{
	for (const auto& line : objects_)
		if (line->v1() == v1 && line->v2() == v2 || reverse && line->v2() == v1 && line->v1() == v2)
			return line;

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns a list of points where the 'cutting' line [cutter] crosses any
// existing lines in the list.
// The point list is sorted along the direction of [cutter]
// -----------------------------------------------------------------------------
vector<Vec2d> LineList::cutPoints(const Seg2d& cutter) const
{
	// Init
	vector<Vec2d> intersect_points;
	Vec2d         intersection;

	// Go through map lines
	for (const auto& line : objects_)
	{
		// Check for intersection
		intersection = cutter.start();
		if (math::linesIntersect(cutter, line->seg(), intersection))
		{
			// Add intersection point to vector
			intersect_points.push_back(intersection);
			LOG_DEBUG("Intersection point", intersection, "valid with", line);
		}
		else if (intersection != cutter.start())
		{
			LOG_DEBUG("Intersection point", intersection, "invalid");
		}
	}

	// Return if no intersections
	if (intersect_points.empty())
		return intersect_points;

	// Check cutting line direction
	double xdif = cutter.br.x - cutter.tl.x;
	double ydif = cutter.br.y - cutter.tl.y;
	if ((xdif * xdif) > (ydif * ydif))
	{
		// Sort points along x axis
		if (xdif >= 0)
			std::sort(
				intersect_points.begin(),
				intersect_points.end(),
				[](const Vec2d& left, const Vec2d& right) { return left.x < right.x; });
		else
			std::sort(
				intersect_points.begin(),
				intersect_points.end(),
				[](const Vec2d& left, const Vec2d& right) { return left.x > right.x; });
	}
	else
	{
		// Sort points along y axis
		if (ydif >= 0)
			std::sort(
				intersect_points.begin(),
				intersect_points.end(),
				[](const Vec2d& left, const Vec2d& right) { return left.y < right.y; });
		else
			std::sort(
				intersect_points.begin(),
				intersect_points.end(),
				[](const Vec2d& left, const Vec2d& right) { return left.y > right.y; });
	}

	return intersect_points;
}

// -----------------------------------------------------------------------------
// Returns the first line found with [id], or null if none found
// -----------------------------------------------------------------------------
MapLine* LineList::firstWithId(int id) const
{
	for (auto& line : objects_)
		if (line->id() == id)
			return line;

	return nullptr;
}

// -----------------------------------------------------------------------------
// Adds all lines with [id] to [list]
// -----------------------------------------------------------------------------
void LineList::putAllWithId(int id, vector<MapLine*>& list) const
{
	for (auto& line : objects_)
		if (line->id() == id)
			list.push_back(line);
}

// -----------------------------------------------------------------------------
// Returns a list of all lines with [id]
// -----------------------------------------------------------------------------
vector<MapLine*> LineList::allWithId(int id) const
{
	vector<MapLine*> list;
	putAllWithId(id, list);
	return list;
}

// -----------------------------------------------------------------------------
// Adds all lines with special affecting matching [id] to [list]
// -----------------------------------------------------------------------------
#define IDEQ(x) (((x) != 0) && ((x) == id))
void LineList::putAllTaggingWithId(int id, int type, vector<MapLine*>& list) const
{
	using game::TagType;

	// Find lines with special affecting matching id
	int tag, arg2, arg3, arg4, arg5;
	for (auto& line : objects_)
	{
		int special = line->special();
		if (special)
		{
			tag       = line->arg(0);
			bool fits = false;
			switch (game::configuration().actionSpecial(special).needsTag())
			{
			case TagType::Sector:
			case TagType::SectorOrBack:
			case TagType::SectorAndBack: fits = (IDEQ(tag) && type == SLADEMap::SECTORS); break;
			case TagType::LineNegative:  tag = abs(tag);
			case TagType::Line:          fits = (IDEQ(tag) && type == SLADEMap::LINEDEFS); break;
			case TagType::Thing:         fits = (IDEQ(tag) && type == SLADEMap::THINGS); break;
			case TagType::Thing1Sector2:
				arg2 = line->arg(1);
				fits = (type == SLADEMap::THINGS ? IDEQ(tag) : (IDEQ(arg2) && type == SLADEMap::SECTORS));
				break;
			case TagType::Thing1Sector3:
				arg3 = line->arg(2);
				fits = (type == SLADEMap::THINGS ? IDEQ(tag) : (IDEQ(arg3) && type == SLADEMap::SECTORS));
				break;
			case TagType::Thing1Thing2:
				arg2 = line->arg(1);
				fits = (type == SLADEMap::THINGS && (IDEQ(tag) || IDEQ(arg2)));
				break;
			case TagType::Thing1Thing4:
				arg4 = line->arg(3);
				fits = (type == SLADEMap::THINGS && (IDEQ(tag) || IDEQ(arg4)));
				break;
			case TagType::Thing1Thing2Thing3:
				arg2 = line->arg(1);
				arg3 = line->arg(2);
				fits = (type == SLADEMap::THINGS && (IDEQ(tag) || IDEQ(arg2) || IDEQ(arg3)));
				break;
			case TagType::Sector1Thing2Thing3Thing5:
				arg2 = line->arg(1);
				arg3 = line->arg(2);
				arg5 = line->arg(4);
				fits =
					(type == SLADEMap::SECTORS ?
						 (IDEQ(tag)) :
						 (type == SLADEMap::THINGS && (IDEQ(arg2) || IDEQ(arg3) || IDEQ(arg5))));
				break;
			case TagType::LineId1Line2:
				arg2 = line->arg(1);
				fits = (type == SLADEMap::LINEDEFS && IDEQ(arg2));
				break;
			case TagType::Thing4:
				arg4 = line->arg(3);
				fits = (type == SLADEMap::THINGS && IDEQ(arg4));
				break;
			case TagType::Thing5:
				arg5 = line->arg(4);
				fits = (type == SLADEMap::THINGS && IDEQ(arg5));
				break;
			case TagType::Line1Sector2:
				arg2 = line->arg(1);
				fits = (type == SLADEMap::LINEDEFS ? (IDEQ(tag)) : (IDEQ(arg2) && type == SLADEMap::SECTORS));
				break;
			case TagType::Sector1Sector2:
				arg2 = line->arg(1);
				fits = (type == SLADEMap::SECTORS && (IDEQ(tag) || IDEQ(arg2)));
				break;
			case TagType::Sector1Sector2Sector3Sector4:
				arg2 = line->arg(1);
				arg3 = line->arg(2);
				arg4 = line->arg(3);
				fits = (type == SLADEMap::SECTORS && (IDEQ(tag) || IDEQ(arg2) || IDEQ(arg3) || IDEQ(arg4)));
				break;
			case TagType::Sector2Is3Line:
				arg2 = line->arg(1);
				fits = (IDEQ(tag) && (arg2 == 3 ? type == SLADEMap::LINEDEFS : type == SLADEMap::SECTORS));
				break;
			case TagType::Sector1Thing2:
				arg2 = line->arg(1);
				fits = (type == SLADEMap::SECTORS ? (IDEQ(tag)) : (IDEQ(arg2) && type == SLADEMap::THINGS));
				break;
			default: break;
			}
			if (fits)
				list.push_back(line);
		}
	}
}
#undef IDEQ

// -----------------------------------------------------------------------------
// Returns the lowest unused id.
// Takes a map [format] parameter as line ids work differently in different map
// formats
// -----------------------------------------------------------------------------
int LineList::firstFreeId(MapFormat format) const
{
	int id = 1;

	// UDMF (id property)
	if (format == MapFormat::UDMF)
	{
		for (unsigned a = 0; a < count_; a++)
		{
			if (objects_[a]->id() == id)
			{
				id++;
				a = 0;
			}
		}
	}

	// Hexen (special 121 arg0)
	else if (format == MapFormat::Hexen)
	{
		for (unsigned a = 0; a < count_; a++)
		{
			if (objects_[a]->special() == 121 && objects_[a]->arg(0) == id)
			{
				id++;
				a = 0;
			}
		}
	}

	// Boom (sector tag (arg0))
	else if (format == MapFormat::Doom && game::configuration().featureSupported(game::Feature::Boom))
	{
		for (unsigned a = 0; a < count_; a++)
		{
			if (objects_[a]->arg(0) == id)
			{
				id++;
				a = 0;
			}
		}
	}

	return id;
}
