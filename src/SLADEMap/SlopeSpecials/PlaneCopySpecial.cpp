
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2025 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    PlaneCopySpecial.cpp
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
#include "PlaneCopySpecial.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/Types.h"

using namespace slade;

PlaneCopySpecial::PlaneCopySpecial(SectorSurfaceType surface_type) : SlopeSpecial{ surface_type } {}

void PlaneCopySpecial::apply()
{
	// Copy the plane from the model sector
	if (surface_type == SectorSurfaceType::Floor)
		target->setFloorPlane(model->floor().plane);
	else
		target->setCeilingPlane(model->ceiling().plane);
}
