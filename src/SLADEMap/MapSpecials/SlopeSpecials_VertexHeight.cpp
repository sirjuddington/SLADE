
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2025 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SlopeSpecials_VertexHeight.cpp
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
#include "General/MapFormat.h"
#include "Geometry/Geometry.h"
#include "Geometry/Plane.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapThing.h"
#include "SLADEMap/MapObject/MapVertex.h"
#include "SLADEMap/MapObjectList/VertexList.h"
#include "SLADEMap/SLADEMap.h"
#include "SLADEMap/Types.h"
#include "SlopeSpecials.h"

using namespace slade;
using namespace map;


std::optional<double> SlopeSpecials::vertexHeight(const MapVertex& vertex, SectorSurfaceType surface_type) const
{
	// Check for vertex height thing
	for (auto& vht : vertex_height_things_)
	{
		if (vht.vertex == &vertex && vht.surface_type == surface_type)
			return vht.thing->zPos();
	}

	// Check for UDMF property
	if (map_->currentFormat() == MapFormat::UDMF)
	{
		const auto prop_name = surface_type == SectorSurfaceType::Floor ? "zfloor" : "zceiling";
		if (vertex.hasProp(prop_name))
			return vertex.floatProperty(prop_name);
	}

	return std::nullopt;
}

void SlopeSpecials::applyTriangleVertexSlope(MapSector& sector, const vector<MapVertex*>& vertices) const
{
	// Floor
	auto h1 = vertexHeight(*vertices[0], SectorSurfaceType::Floor);
	auto h2 = vertexHeight(*vertices[1], SectorSurfaceType::Floor);
	auto h3 = vertexHeight(*vertices[2], SectorSurfaceType::Floor);
	if (h1 || h2 || h3)
	{
		Vec3d p1(vertices[0]->position(), h1 ? *h1 : sector.floor().height);
		Vec3d p2(vertices[1]->position(), h2 ? *h2 : sector.floor().height);
		Vec3d p3(vertices[2]->position(), h3 ? *h3 : sector.floor().height);
		sector.setFloorPlane(geometry::planeFromTriangle(p1, p2, p3));
	}

	// Ceiling
	h1 = vertexHeight(*vertices[0], SectorSurfaceType::Ceiling);
	h2 = vertexHeight(*vertices[1], SectorSurfaceType::Ceiling);
	h3 = vertexHeight(*vertices[2], SectorSurfaceType::Ceiling);
	if (h1 || h2 || h3)
	{
		Vec3d p1(vertices[0]->position(), h1 ? *h1 : sector.ceiling().height);
		Vec3d p2(vertices[1]->position(), h2 ? *h2 : sector.ceiling().height);
		Vec3d p3(vertices[2]->position(), h3 ? *h3 : sector.ceiling().height);
		sector.setCeilingPlane(geometry::planeFromTriangle(p1, p2, p3));
	}
}

void SlopeSpecials::applyRectangleVertexSlope(MapSector& sector, SectorSurfaceType surface_type) const
{
	vector<MapVertex*> vertices;
	sector.putVertices(vertices);
	std::vector<int> height_verts;
	bool             floor        = surface_type == SectorSurfaceType::Floor;
	string           prop         = floor ? "zfloor" : "zceiling";
	auto             v1_hasheight = vertices[0]->hasProp(prop);
	auto             v2_hasheight = vertices[1]->hasProp(prop);
	auto             v3_hasheight = vertices[2]->hasProp(prop);
	auto             v4_hasheight = vertices[3]->hasProp(prop);
	if (v1_hasheight)
		height_verts.push_back(0);
	if (v2_hasheight)
		height_verts.push_back(1);
	if (v3_hasheight)
		height_verts.push_back(2);
	if (v4_hasheight)
		height_verts.push_back(3);
	if (height_verts.size() == 2) // Must only have two out of the four verts assigned a zfloor/ceiling value
	{
		MapVertex* v1 = vertices[height_verts[0]];
		MapVertex* v2 = vertices[height_verts[1]];
		// Must be both verts of the same line
		bool same_line = false;
		for (const auto& line : v1->connectedLines())
		{
			if ((line->v1() == v1 && line->v2() == v2) || (line->v1() == v2 && line->v2() == v1))
			{
				same_line = true;
				break;
			}
		}
		if (same_line)
		{
			// The zfloor/zceiling values must be equal
			auto sector_z = floor ? sector.floor().height : sector.ceiling().height;
			if (fabs(
					vertexHeight(*v1, surface_type).value_or(sector_z)
					- vertexHeight(*v2, surface_type).value_or(sector_z))
				< 0.001f)
			{
				// Psuedo-Plane_Align routine
				double     furthest_dist   = 0.0;
				MapVertex* furthest_vertex = nullptr;
				Seg2d      seg(v1->position(), v2->position());
				for (auto& vertex : vertices)
				{
					double dist = geometry::distanceToLine(vertex->position(), seg);

					if (!geometry::colinear(
							vertex->xPos(), vertex->yPos(), v1->xPos(), v1->yPos(), v2->xPos(), v2->yPos())
						&& dist > furthest_dist)
					{
						furthest_vertex = vertex;
						furthest_dist   = dist;
					}
				}

				if (!furthest_vertex || furthest_dist < 0.01)
					return;

				// Calculate slope plane from our three points: this line's endpoints
				// (at the model sector's height) and the found vertex (at this sector's height).
				double modelz  = vertexHeight(*v1, surface_type).value_or(sector_z);
				double targetz = sector_z;

				Vec3d p1(v1->position(), modelz);
				Vec3d p2(v2->position(), modelz);
				Vec3d p3(furthest_vertex->position(), targetz);

				if (floor)
					sector.setFloorPlane(geometry::planeFromTriangle(p1, p2, p3));
				else
					sector.setCeilingPlane(geometry::planeFromTriangle(p1, p2, p3));
			}
		}
	}
}

void SlopeSpecials::addVertexHeightThing(const MapThing& thing, SectorSurfaceType surface_type)
{
	VertexHeightThing vht;
	vht.surface_type = surface_type;
	vht.thing        = &thing;

	// Get containing (target) sector
	vht.vertex = map_->vertices().vertexAt(thing.xPos(), thing.yPos());
	if (!vht.vertex)
	{
		log::warning("Vertex height thing {} is not on a vertex", thing.index());
		return;
	}

	vertex_height_things_.push_back(vht);
}

void SlopeSpecials::removeVertexHeightThing(const MapThing& thing)
{
	unsigned i = 0;
	while (i < vertex_height_things_.size())
	{
		if (vertex_height_things_[i].thing == &thing)
		{
			vertex_height_things_.erase(vertex_height_things_.begin() + i);
			continue;
		}
		i++;
	}
}
