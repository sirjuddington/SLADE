
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2025 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SlopeSpecials_PlaneAlign.cpp
// Description:
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
#include "Geometry/Geometry.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapVertex.h"
#include "SLADEMap/Types.h"
#include "SlopeSpecials.h"
#include "Utility/Vector.h"

using namespace slade;


void SlopeSpecials::addPlaneAlign(const MapLine& line)
{
	// Floor
	if (line.arg(0) > 0)
		addPlaneAlign(line, SectorSurfaceType::Floor, line.arg(0));

	// Ceiling
	if (line.arg(1) > 0)
		addPlaneAlign(line, SectorSurfaceType::Ceiling, line.arg(1));

	plane_align_specials_sorted_ = false;
}

void SlopeSpecials::addPlaneAlign(const MapLine& line, SectorSurfaceType surface_type, int where)
{
	PlaneAlign pa{ surface_type };
	pa.line = &line;

	if (where == 1)
	{
		pa.target = line.frontSector();
		pa.model  = line.backSector();
	}
	else if (where == 2)
	{
		pa.target = line.backSector();
		pa.model  = line.frontSector();
	}
	else
	{
		log::warning(
			"Invalid Plane_Align special on line {}: arg{} must be 1 (front) or 2 (back)",
			line.index(),
			surface_type == SectorSurfaceType::Floor ? 0 : 1);
		return;
	}

	plane_align_specials_.push_back(pa);
	plane_align_specials_sorted_ = false;
}

void SlopeSpecials::removePlaneAlign(const MapLine& line)
{
	unsigned i = 0;
	while (i < plane_align_specials_.size())
	{
		if (plane_align_specials_[i].line == &line)
		{
			vectorAddUnique(sectors_to_update_, plane_align_specials_[i].target);
			plane_align_specials_.erase(plane_align_specials_.begin() + i);
			continue;
		}

		i++;
	}
}

void SlopeSpecials::applyPlaneAlign(const PlaneAlign& special)
{
	bool floor = special.surface_type == SectorSurfaceType::Floor;

	vector<MapVertex*> vertices;
	special.target->putVertices(vertices);

	Vec2d mid    = special.line->getPoint(MapObject::Point::Mid);
	Vec2d v1_pos = glm::normalize(special.line->start() - mid);
	Vec2d v2_pos = glm::normalize(special.line->end() - mid);

	// Extend the line to the sector boundaries
	double max_dot_1 = 0.0;
	double max_dot_2 = 0.0;
	for (auto& vertex : vertices)
	{
		Vec2d vert = vertex->position() - mid;

		double dot = glm::dot(vert, v1_pos);

		double& max_dot = dot > 0 ? max_dot_1 : max_dot_2;

		dot     = std::fabs(dot);
		max_dot = std::max(dot, max_dot);
	}

	v1_pos = v1_pos * max_dot_1 + mid;
	v2_pos = v2_pos * max_dot_2 + mid;

	// The slope is between the line with Plane_Align, and the point in the
	// sector furthest away from it, which can only be at a vertex
	double     furthest_dist   = 0.0;
	MapVertex* furthest_vertex = nullptr;
	Seg2d      seg(v1_pos, v2_pos);
	for (auto& vertex : vertices)
	{
		double dist = geometry::distanceToLine(vertex->position(), seg);

		if (!geometry::colinear(vertex->xPos(), vertex->yPos(), v1_pos.x, v1_pos.y, v2_pos.x, v2_pos.y)
			&& dist > furthest_dist)
		{
			furthest_vertex = vertex;
			furthest_dist   = dist;
		}
	}

	if (!furthest_vertex || furthest_dist < 0.01)
	{
		log::warning(
			"Ignoring Plane_Align on line {}; sector {} has no appropriate reference vertex",
			special.line->index(),
			special.target->index());
		return;
	}

	// Calculate slope plane from our three points: this line's endpoints
	// (at the model sector's height) and the found vertex (at this sector's height).
	double modelz  = floor ? special.model->floor().height : special.model->ceiling().height;
	double targetz = floor ? special.target->floor().height : special.target->ceiling().height;

	Vec3d p1(v1_pos, modelz);
	Vec3d p2(v2_pos, modelz);
	Vec3d p3(furthest_vertex->position(), targetz);

	// Apply to sector floor/ceiling plane
	if (floor)
		special.target->setFloorPlane(geometry::planeFromTriangle(p1, p2, p3));
	else
		special.target->setCeilingPlane(geometry::planeFromTriangle(p1, p2, p3));
}

void SlopeSpecials::applyPlaneAlignSpecials(const MapSector& sector)
{
	// Sort by line index (descending) if needed
	if (!plane_align_specials_sorted_)
	{
		std::sort(
			plane_align_specials_.begin(),
			plane_align_specials_.end(),
			[](const auto& a, const auto& b) { return a.line->index() > b.line->index(); });

		plane_align_specials_sorted_ = true;
	}

	// Since the PlaneAlign specials are sorted by line index descending,
	// we can just apply the first one we find for each surface type
	bool pa_floor = false;
	bool pa_ceil  = false;
	for (const auto& pa : plane_align_specials_)
	{
		if (pa_floor && pa_ceil)
			break;

		if (!pa_floor && pa.isTarget(&sector, SectorSurfaceType::Floor))
		{
			applyPlaneAlign(pa);
			pa_floor = true;
		}
		if (!pa_ceil && pa.isTarget(&sector, SectorSurfaceType::Ceiling))
		{
			applyPlaneAlign(pa);
			pa_ceil = true;
		}
	}
}
