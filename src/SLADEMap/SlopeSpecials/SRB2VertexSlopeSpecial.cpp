
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2025 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SRB2VertexSlopeSpecial.cpp
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
#include "SRB2VertexSlopeSpecial.h"
#include "Geometry/Geometry.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapThing.h"

using namespace slade;

SRB2VertexSlopeSpecial::SRB2VertexSlopeSpecial(
	const MapLine&    line,
	MapSector&        target,
	const MapThing*   vertices[3],
	SectorSurfaceType surface_type) :
	SlopeSpecial{ surface_type },
	line{ &line }
{
	this->target      = &target;
	this->vertices[0] = vertices[0];
	this->vertices[1] = vertices[1];
	this->vertices[2] = vertices[2];
}

void SRB2VertexSlopeSpecial::apply()
{
	auto p1 = Vec3d(vertices[0]->xPos(), vertices[0]->yPos(), vertices[0]->zPos());
	auto p2 = Vec3d(vertices[1]->xPos(), vertices[1]->yPos(), vertices[1]->zPos());
	auto p3 = Vec3d(vertices[2]->xPos(), vertices[2]->yPos(), vertices[2]->zPos());

	if (surface_type == SectorSurfaceType::Floor)
		target->setFloorPlane(geometry::planeFromTriangle(p1, p2, p3));
	else
		target->setCeilingPlane(geometry::planeFromTriangle(p1, p2, p3));
}
