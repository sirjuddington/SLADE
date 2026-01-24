
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2025 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SlopeSpecials.cpp
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
#include "SlopeSpecials.h"
#include "Game/Configuration.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapSide.h"
#include "SLADEMap/MapObject/MapThing.h"
#include "SLADEMap/MapObject/MapVertex.h"
#include "SLADEMap/MapObjectList/SectorList.h"
#include "SLADEMap/MapObjectList/ThingList.h"
#include "SLADEMap/SLADEMap.h"
#include "Utility/Vector.h"

using namespace slade;
using namespace map;


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
void applyUDMFPlanes(MapSector& sector)
{
	double a{}, b{}, c{}, d{};

	// Floor -------------------------------------------------------------------
	bool has_floorplane = true;

	// A
	if (sector.hasProp("floorplane_a"))
		a = -sector.floatProperty("floorplane_a");
	else
		has_floorplane = false;

	// B
	if (has_floorplane && sector.hasProp("floorplane_b"))
		b = -sector.floatProperty("floorplane_b");
	else
		has_floorplane = false;

	// C
	if (has_floorplane && sector.hasProp("floorplane_c"))
		c = -sector.floatProperty("floorplane_c");
	else
		has_floorplane = false;

	// D
	if (sector.hasProp("floorplane_d"))
		d = sector.floatProperty("floorplane_d");
	else
		has_floorplane = false;

	if (has_floorplane)
		sector.setFloorPlane({ a, b, c, d });


	// Ceiling -----------------------------------------------------------------
	bool has_ceilingplane = true;

	// A
	if (sector.hasProp("ceilingplane_a"))
		a = -sector.floatProperty("ceilingplane_a");
	else
		has_ceilingplane = false;

	// B
	if (has_ceilingplane && sector.hasProp("ceilingplane_b"))
		b = -sector.floatProperty("ceilingplane_b");
	else
		has_ceilingplane = false;

	// C
	if (has_ceilingplane && sector.hasProp("ceilingplane_c"))
		c = -sector.floatProperty("ceilingplane_c");
	else
		has_ceilingplane = false;

	// D
	if (sector.hasProp("ceilingplane_d"))
		d = sector.floatProperty("ceilingplane_d");
	else
		has_ceilingplane = false;

	if (has_ceilingplane)
		sector.setCeilingPlane({ a, b, c, d });
}
} // namespace


// -----------------------------------------------------------------------------
//
// SlopeSpecials Class Functions
//
// -----------------------------------------------------------------------------

SlopeSpecials::SlopeSpecials(SLADEMap& map) : map_(&map) {}

void SlopeSpecials::processLineSpecial(const MapLine& line)
{
	// ZDoom
	if (game::configuration().currentPort() == "zdoom")
	{
		switch (line.special())
		{
		case 181: addPlaneAlign(line); break;
		case 118: addPlaneCopy(line); break;
		default:  break;
		}
	}

	// Eternity
	if (game::configuration().currentPort() == "eternity")
	{
		switch (line.special())
		{
		case 181: addPlaneAlign(line); break;
		case 118: addPlaneCopy(line); break;
		default:  break;
		}
	}

	// SRB2
	if (game::configuration().currentPort() == "srb2")
	{
		switch (line.special())
		{
			// Sector Slopes ---------------------------------------------------
		case 700: addPlaneAlign(line, SectorSurfaceType::Floor, 1); break;   // Front sector floor
		case 701: addPlaneAlign(line, SectorSurfaceType::Ceiling, 1); break; // Front sector ceiling
		case 702:
			// Front sector floor and ceiling
			addPlaneAlign(line, SectorSurfaceType::Floor, 1);
			addPlaneAlign(line, SectorSurfaceType::Ceiling, 1);
			break;
		case 703:
			// Front sector floor and back sector ceiling
			addPlaneAlign(line, SectorSurfaceType::Floor, 1);
			addPlaneAlign(line, SectorSurfaceType::Ceiling, 2);
			break;
		case 710: addPlaneAlign(line, SectorSurfaceType::Floor, 2); break;   // Back sector floor
		case 711: addPlaneAlign(line, SectorSurfaceType::Ceiling, 2); break; // Back sector ceiling
		case 712:
			// Back sector floor and ceiling
			addPlaneAlign(line, SectorSurfaceType::Floor, 2);
			addPlaneAlign(line, SectorSurfaceType::Ceiling, 2);
			break;
		case 713:
			// Back sector floor and front sector ceiling
			addPlaneAlign(line, SectorSurfaceType::Floor, 2);
			addPlaneAlign(line, SectorSurfaceType::Ceiling, 1);
			break;

			// Vertex Slopes ---------------------------------------------------
		case 704: addSRB2VertexSlope(line, SectorSurfaceType::Floor, true); break;   // Front sector floor
		case 705: addSRB2VertexSlope(line, SectorSurfaceType::Ceiling, true); break; // Front sector ceiling
		case 714: addSRB2VertexSlope(line, SectorSurfaceType::Floor, false); break;  // Back sector floor
		case 715:
			addSRB2VertexSlope(line, SectorSurfaceType::Ceiling, false);
			break; // Back sector ceiling

			// Slope Copy ------------------------------------------------------
		case 720: addSRB2PlaneCopy(line, SectorSurfaceType::Floor); break;
		case 721: addSRB2PlaneCopy(line, SectorSurfaceType::Ceiling); break;
		case 722:
			addSRB2PlaneCopy(line, SectorSurfaceType::Floor);
			addSRB2PlaneCopy(line, SectorSurfaceType::Ceiling);
			break;
		default: break;
		}
	}
}

void SlopeSpecials::processThing(const MapThing& thing)
{
	// ZDoom
	if (game::configuration().currentPort() == "zdoom")
	{
		switch (thing.type())
		{
		case 1500: addVavoomSlopeThing(thing, SectorSurfaceType::Floor); break;
		case 1501: addVavoomSlopeThing(thing, SectorSurfaceType::Ceiling); break;
		case 1504: addVertexHeightThing(thing, SectorSurfaceType::Floor); break;
		case 1505: addVertexHeightThing(thing, SectorSurfaceType::Ceiling); break;
		case 9500: addLineSlopeThing(thing, SectorSurfaceType::Floor); break;
		case 9501: addLineSlopeThing(thing, SectorSurfaceType::Ceiling); break;
		case 9502: addSectorTiltThing(thing, SectorSurfaceType::Floor); break;
		case 9503: addSectorTiltThing(thing, SectorSurfaceType::Ceiling); break;
		case 9510: addCopySlopeThing(thing, SectorSurfaceType::Floor); break;
		case 9511: addCopySlopeThing(thing, SectorSurfaceType::Ceiling); break;
		default:   break;
		}
	}
}

void SlopeSpecials::clearSpecials()
{
	plane_align_specials_.clear();
	line_slope_things_.clear();
	sector_tilt_things_.clear();
	vavoom_things_.clear();
	sorted_slope_things_.clear();
	copy_slope_things_.clear();
	vertex_height_things_.clear();
	plane_copy_specials_.clear();
	srb2_vertex_slope_specials_.clear();
}

void SlopeSpecials::updateSectorPlanes(MapSector& sector)
{
	const auto& port   = game::configuration().currentPort();
	auto        format = map_->currentFormat();

	// 1. Init flat planes
	sector.setFloorPlane(Plane::flat(sector.floor().height));
	sector.setCeilingPlane(Plane::flat(sector.ceiling().height));

	// 2. UDMF slope properties
	if (format == MapFormat::UDMF && port == "zdoom")
		applyUDMFPlanes(sector);

	// 3. Plane_Align
	applyPlaneAlignSpecials(sector);

	// 4. Slope Things (Line, SectorTilt, Vavoom)
	applySlopeThingSpecials(sector);

	// 5. Slope Copy things
	applyCopySlopeThingSpecials(sector);

	// 6. Vertex heights
	if (port == "zdoom" || port == "edge_classic")
	{
		vector<MapVertex*> vertices;
		sector.putVertices(vertices);
		if (vertices.size() == 3)
			applyTriangleVertexSlope(sector, vertices);
		else if (vertices.size() == 4 && port == "edge_classic")
		{
			applyRectangleVertexSlope(sector, SectorSurfaceType::Floor);
			applyRectangleVertexSlope(sector, SectorSurfaceType::Ceiling);
		}
	}

	// 7. Plane_Copy
	applyPlaneCopySpecials(sector);
}

void SlopeSpecials::updateOutdatedSectorPlanes()
{
	for (const auto s : sectors_to_update_)
		updateSectorPlanes(*s);
	sectors_to_update_.clear();
}

void SlopeSpecials::lineUpdated(const MapLine& line, bool update_planes)
{
	// Remove existing specials
	removePlaneAlign(line);
	removePlaneCopy(line);

	// Re-process
	processLineSpecial(line);

	// TODO: Check if line needs to be added or removed from relevant (Line/Vavoom)SlopeThings

	// Update planes for sectors that need updating
	if (update_planes)
		updateOutdatedSectorPlanes();
}

void SlopeSpecials::sectorUpdated(MapSector& sector, bool update_planes)
{
	// Update sector planes
	vectorAddUnique(sectors_to_update_, &sector);

	// If it's the model sector for any Plane_Align or Plane_Copy specials,
	// update planes for the target sectors
	for (const auto& pa : plane_align_specials_)
	{
		if (pa.model == &sector)
			vectorAddUnique(sectors_to_update_, pa.target);
	}
	for (const auto& pc : plane_copy_specials_)
	{
		if (pc.model == &sector)
			vectorAddUnique(sectors_to_update_, pc.target);
	}

	// If it's the containing sector of any LineSlopeThings,
	// update planes for the target sectors
	for (const auto& ls : line_slope_things_)
		if (ls.containing_sector == &sector)
			vectorAddUnique(sectors_to_update_, ls.target);

	// Update planes for sectors that need updating
	if (update_planes)
		updateOutdatedSectorPlanes();
}

void SlopeSpecials::thingUpdated(const MapThing& thing, bool update_planes)
{
	// Remove existing specials
	removeSlopeThing(thing);
	removeCopySlopeThing(thing);

	// Re-process
	processThing(thing);

	// Update planes for sectors that need updating
	if (update_planes)
		updateOutdatedSectorPlanes();
}

void SlopeSpecials::addPlaneCopy(const MapLine& line)
{
	PlaneCopy pc{ SectorSurfaceType::Floor };
	pc.line = &line;

	// Front floor
	if (line.arg(0) > 0)
	{
		pc.surface_type = SectorSurfaceType::Floor;
		pc.target       = line.frontSector();
		pc.model        = map_->sectors().firstWithId(line.arg(0));
		if (!pc.model)
			log::warning("Plane copy special on line {}: no sector with tag {} (arg 1)", line.index(), line.arg(0));
		else if (!pc.target)
			log::warning("Plane copy special on line {}: line has no front sector", line.index());
		else
			plane_copy_specials_.push_back(pc);
	}

	// Front ceiling
	if (line.arg(1) > 0)
	{
		pc.surface_type = SectorSurfaceType::Ceiling;
		pc.target       = line.frontSector();
		pc.model        = map_->sectors().firstWithId(line.arg(1));
		if (!pc.model)
			log::warning("Plane copy special on line {}: no sector with tag {} (arg 2)", line.index(), line.arg(1));
		else if (!pc.target)
			log::warning("Plane copy special on line {}: line has no front sector", line.index());
		else
			plane_copy_specials_.push_back(pc);
	}

	// Back floor
	if (line.arg(2) > 0)
	{
		pc.surface_type = SectorSurfaceType::Floor;
		pc.target       = line.backSector();
		pc.model        = map_->sectors().firstWithId(line.arg(2));
		if (!pc.model)
			log::warning("Plane copy special on line {}: no sector with tag {} (arg 3)", line.index(), line.arg(2));
		else if (!pc.target)
			log::warning("Plane copy special on line {}: line has no back sector", line.index());
		else
			plane_copy_specials_.push_back(pc);
	}

	// Back ceiling
	if (line.arg(3) > 0)
	{
		pc.surface_type = SectorSurfaceType::Ceiling;
		pc.target       = line.backSector();
		pc.model        = map_->sectors().firstWithId(line.arg(3));
		if (!pc.model)
			log::warning("Plane copy special on line {}: no sector with tag {} (arg 4)", line.index(), line.arg(3));
		else if (!pc.target)
			log::warning("Plane copy special on line {}: line has no back sector", line.index());
		else
			plane_copy_specials_.push_back(pc);
	}

	// Share slope
	if (line.arg(4) != 0)
	{
		// Floor front -> back
		if (line.arg(4) & 1)
		{
			pc.surface_type = SectorSurfaceType::Floor;
			pc.target       = line.backSector();
			pc.model        = line.frontSector();
			plane_copy_specials_.push_back(pc);
		}
		// Floor back -> front
		else if (line.arg(4) & 2)
		{
			pc.surface_type = SectorSurfaceType::Floor;
			pc.target       = line.frontSector();
			pc.model        = line.backSector();
			plane_copy_specials_.push_back(pc);
		}

		// Ceiling front -> back
		if (line.arg(4) & 4)
		{
			pc.surface_type = SectorSurfaceType::Ceiling;
			pc.target       = line.backSector();
			pc.model        = line.frontSector();
			plane_copy_specials_.push_back(pc);
		}
		// Ceiling back -> front
		else if (line.arg(4) & 8)
		{
			pc.surface_type = SectorSurfaceType::Ceiling;
			pc.target       = line.frontSector();
			pc.model        = line.backSector();
			plane_copy_specials_.push_back(pc);
		}
	}
}

void SlopeSpecials::addSRB2PlaneCopy(const MapLine& line, SectorSurfaceType surface_type)
{
	auto pc   = PlaneCopy{ surface_type };
	pc.line   = &line;
	pc.target = line.frontSector();

	if (!pc.target)
	{
		log::warning("Ignoring copied slopes special on line {}, no front sector on this line", line.index());
		return;
	}

	auto tagged = map_->sectors().firstWithId(line.id());
	if (!tagged)
	{
		log::warning(
			"Ignoring copied slopes special on line {}, couldn't find sector with tag {}", line.index(), line.id());
		return;
	}

	pc.model = tagged;

	plane_copy_specials_.push_back(pc);
}

void SlopeSpecials::removePlaneCopy(const MapLine& line)
{
	unsigned i = 0;
	while (i < plane_copy_specials_.size())
	{
		if (plane_copy_specials_[i].line == &line)
		{
			vectorAddUnique(sectors_to_update_, plane_copy_specials_[i].target);
			plane_copy_specials_.erase(plane_copy_specials_.begin() + i);
			continue;
		}
		i++;
	}
}

void SlopeSpecials::applyPlaneCopy(const PlaneCopy& special)
{
	// Copy the plane from the model sector
	if (special.surface_type == SectorSurfaceType::Floor)
		special.target->setFloorPlane(special.model->floor().plane);
	else
		special.target->setCeilingPlane(special.model->ceiling().plane);
}

void SlopeSpecials::applyPlaneCopySpecials(const MapSector& sector)
{
	// Sort by line index (descending) if needed
	if (!plane_copy_specials_sorted_)
	{
		std::sort(
			plane_copy_specials_.begin(),
			plane_copy_specials_.end(),
			[](const auto& a, const auto& b) { return a.line->index() > b.line->index(); });

		plane_copy_specials_sorted_ = true;
	}

	// Since the PlaneCopy specials are sorted by line index descending,
	// we can just apply the first one we find for each surface type
	bool pc_floor = false;
	bool pc_ceil  = false;
	for (const auto& pc : plane_copy_specials_)
	{
		if (!pc_floor && pc.isTarget(&sector, SectorSurfaceType::Floor))
		{
			applyPlaneCopy(pc);
			pc_floor = true;
		}
		if (!pc_ceil && pc.isTarget(&sector, SectorSurfaceType::Ceiling))
		{
			applyPlaneCopy(pc);
			pc_ceil = true;
		}
	}
}

void SlopeSpecials::addSRB2VertexSlope(const MapLine& line, SectorSurfaceType surface_type, bool front)
{
	SRB2VertexSlope svs{ surface_type };
	svs.target = front ? line.frontSector() : line.backSector();
	if (!svs.target)
	{
		log::warning(
			"Ignoring vertex slope special on line {}, the target back/front sector for this line don't exist",
			line.index());
		return;
	}

	auto sidedef = front ? line.s1() : line.s2();

	unsigned count = 0;
	for (auto& thing : map_->things())
	{
		if (thing->type() != 750)
			continue;

		if ((line.flagSet(8192)
			 && (thing->angle() == line.id() || thing->angle() == sidedef->texOffsetX()
				 || thing->angle() == sidedef->texOffsetY()))
			|| thing->angle() == line.id())
		{
			svs.vertices[count++] = thing;
			if (count >= 3)
				break;
		}
	}

	if (count < 3)
	{
		log::warning(
			"Ignoring vertex slope special on line {}, No or insufficient vertex slope things (750) were provided",
			line.index());
		return;
	}

	srb2_vertex_slope_specials_.push_back(svs);
	srb2_vertex_slope_specials_sorted_ = false;
}

void SlopeSpecials::removeSRB2VertexSlope(const MapLine& line)
{
	unsigned i = 0;
	while (i < srb2_vertex_slope_specials_.size())
	{
		if (srb2_vertex_slope_specials_[i].line == &line)
		{
			vectorAddUnique(sectors_to_update_, srb2_vertex_slope_specials_[i].target);
			srb2_vertex_slope_specials_.erase(srb2_vertex_slope_specials_.begin() + i);
			continue;
		}
		i++;
	}
}

void SlopeSpecials::applySRB2VertexSlopeSpecials(const MapSector& sector)
{
	// Sort by line index (descending) if needed
	if (!srb2_vertex_slope_specials_sorted_)
	{
		std::sort(
			srb2_vertex_slope_specials_.begin(),
			srb2_vertex_slope_specials_.end(),
			[](const auto& a, const auto& b) { return a.line->index() > b.line->index(); });

		srb2_vertex_slope_specials_sorted_ = true;
	}

	// Since the SRB2VertexSlopeSpecials are sorted by line index descending,
	// we can just apply the first one we find for each surface type
	bool vs_floor = false;
	bool vs_ceil  = false;
	for (const auto& pa : plane_align_specials_)
	{
		if (vs_floor && vs_ceil)
			break;

		if (!vs_floor && pa.isTarget(&sector, SectorSurfaceType::Floor))
		{
			applyPlaneAlign(pa);
			vs_floor = true;
		}
		if (!vs_ceil && pa.isTarget(&sector, SectorSurfaceType::Ceiling))
		{
			applyPlaneAlign(pa);
			vs_ceil = true;
		}
	}
}
