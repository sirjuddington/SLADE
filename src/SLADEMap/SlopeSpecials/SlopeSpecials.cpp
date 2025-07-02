
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
#include "CopySlopeThing.h"
#include "Game/Configuration.h"
#include "Geometry/Geometry.h"
#include "LineSlopeThing.h"
#include "PlaneAlignSpecial.h"
#include "PlaneCopySpecial.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapSide.h"
#include "SLADEMap/MapObject/MapThing.h"
#include "SLADEMap/MapObject/MapVertex.h"
#include "SLADEMap/MapObjectList/LineList.h"
#include "SLADEMap/MapObjectList/SectorList.h"
#include "SLADEMap/MapObjectList/ThingList.h"
#include "SLADEMap/MapObjectList/VertexList.h"
#include "SLADEMap/SLADEMap.h"
#include "SRB2VertexSlopeSpecial.h"
#include "SectorTiltThing.h"
#include "Utility/Vector.h"
#include "VavoomSlopeThing.h"

using namespace slade;


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
	slope_things_.clear();
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
	applyPlaneAlign(sector);

	// 4. Slope Things (Line, SectorTilt, Vavoom)
	applySlopeThings(sector);

	// 5. Slope Copy things
	applyCopySlopeThings(sector);

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
	applyPlaneCopy(sector);
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
		if (pa->model == &sector)
			vectorAddUnique(sectors_to_update_, pa->target);
	}
	for (const auto& pc : plane_copy_specials_)
	{
		if (pc->model == &sector)
			vectorAddUnique(sectors_to_update_, pc->target);
	}

	// If it's the containing sector of any LineSlopeThings,
	// update planes for the target sectors
	for (const auto& st : slope_things_)
	{
		if (st->type != SlopeSpecialThing::Type::Line)
			continue;

		if (dynamic_cast<LineSlopeThing*>(st.get())->containing_sector == &sector)
			vectorAddUnique(sectors_to_update_, st->target);
	}

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
		const auto prop_name = surface_type == SectorSurfaceType::Floor ? "floorheight" : "ceilingheight";
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


void SlopeSpecials::addPlaneAlign(const MapLine& line)
{
	// Floor
	if (line.arg(0) > 0)
		plane_align_specials_.emplace_back(
			std::make_unique<PlaneAlignSpecial>(line, SectorSurfaceType::Floor, line.arg(0)));

	// Ceiling
	if (line.arg(1) > 0)
		plane_align_specials_.emplace_back(
			std::make_unique<PlaneAlignSpecial>(line, SectorSurfaceType::Ceiling, line.arg(1)));

	plane_align_specials_sorted_ = false;
}

void SlopeSpecials::addPlaneAlign(const MapLine& line, SectorSurfaceType surface_type, int where)
{
	plane_align_specials_.emplace_back(std::make_unique<PlaneAlignSpecial>(line, surface_type, where));
	plane_align_specials_sorted_ = false;
}

void SlopeSpecials::removePlaneAlign(const MapLine& line)
{
	unsigned i = 0;
	while (i < plane_align_specials_.size())
	{
		if (plane_align_specials_[i]->line == &line)
		{
			vectorAddUnique(sectors_to_update_, plane_align_specials_[i]->target);
			plane_align_specials_.erase(plane_align_specials_.begin() + i);
			continue;
		}

		i++;
	}
}

void SlopeSpecials::applyPlaneAlign(const MapSector& sector)
{
	// Sort by line index (descending) if needed
	if (!plane_align_specials_sorted_)
	{
		std::sort(
			plane_align_specials_.begin(),
			plane_align_specials_.end(),
			[](const auto& a, const auto& b) { return a->line->index() > b->line->index(); });

		plane_align_specials_sorted_ = true;
	}

	// Since the PlaneAlignSpecials are sorted by line index descending,
	// we can just apply the first one we find for each surface type
	bool pa_floor = false;
	bool pa_ceil  = false;
	for (const auto& pa : plane_align_specials_)
	{
		if (pa_floor && pa_ceil)
			break;

		if (!pa_floor && pa->isTarget(&sector, SectorSurfaceType::Floor))
		{
			pa->apply();
			pa_floor = true;
		}
		if (!pa_ceil && pa->isTarget(&sector, SectorSurfaceType::Ceiling))
		{
			pa->apply();
			pa_ceil = true;
		}
	}
}

void SlopeSpecials::addLineSlopeThing(const MapThing& thing, SectorSurfaceType surface_type)
{
	LineSlopeThing lst{ surface_type };
	lst.thing = &thing;

	// Check line id
	const auto line_id = thing.arg(0);
	if (!line_id)
	{
		log::warning("Ignoring line slope thing {} with no lineid argument", thing.index());
		return;
	}

	// Get containing sector
	lst.containing_sector = map_->sectors().atPos(thing.position());
	if (!lst.containing_sector)
	{
		log::warning("Line slope thing {} is not within a sector", thing.index());
		return;
	}

	// Add LineSlopeThing for each tagged line
	for (const auto line : map_->lines().allWithId(line_id))
	{
		lst.line = line;

		// Line slope things only affect the sector on the side of the line that
		// faces the thing
		const double side = geometry::lineSide(thing.position(), line->seg());
		if (side < 0)
			lst.target = line->backSector();
		else if (side > 0)
			lst.target = line->frontSector();
		if (!lst.target)
			continue;

		slope_things_.push_back(std::make_unique<LineSlopeThing>(lst));
	}

	slope_things_sorted_ = false;
}

void SlopeSpecials::addSectorTiltThing(const MapThing& thing, SectorSurfaceType surface_type)
{
	auto stt   = std::make_unique<SectorTiltThing>(surface_type);
	stt->thing = &thing;

	// Get containing (target) sector
	stt->target = map_->sectors().atPos(thing.position());
	if (!stt->target)
	{
		log::warning("Sector Tilt slope thing {} is not within a sector", thing.index());
		return;
	}

	slope_things_.push_back(std::move(stt));
}

void SlopeSpecials::removeSlopeThing(const MapThing& thing)
{
	unsigned i = 0;
	while (i < slope_things_.size())
	{
		if (slope_things_[i]->thing == &thing)
		{
			vectorAddUnique(sectors_to_update_, slope_things_[i]->target);
			slope_things_.erase(slope_things_.begin() + i);
			continue;
		}
		i++;
	}
}

void SlopeSpecials::applySlopeThings(const MapSector& sector)
{
	// Sort by thing index if needed
	if (!slope_things_sorted_)
	{
		std::sort(
			slope_things_.begin(),
			slope_things_.end(),
			[](const auto& a, const auto& b) { return a->thing->index() < b->thing->index(); });

		slope_things_sorted_ = true;
	}

	// Apply each SlopeThingSpecial in order
	for (const auto& st : slope_things_)
	{
		if (st->target == &sector)
			st->apply();
	}
}

void SlopeSpecials::addVavoomSlopeThing(const MapThing& thing, SectorSurfaceType surface_type)
{
	auto vst   = std::make_unique<VavoomSlopeThing>(surface_type);
	vst->thing = &thing;

	// Get containing (target) sector
	vst->target = map_->sectors().atPos(thing.position());
	if (!vst->target)
	{
		log::warning("Vavoom slope thing {} is not within a sector", thing.index());
		return;
	}

	// Find line in containing sector with first arg matching thing id
	const int        tid = thing.id();
	vector<MapLine*> lines;
	vst->target->putLines(lines);

	// TODO: unclear if this is the same order that ZDoom would go through the
	// lines, which matters if two lines have the same first arg
	for (unsigned a = 0; a < lines.size(); a++)
	{
		if (tid != lines[a]->arg(0))
			continue;

		// Check thing position is valid
		if (geometry::distanceToLineFast(thing.position(), lines[a]->seg()) == 0)
		{
			log::warning("Vavoom slope thing {} lies directly on its target line {}", thing.index(), a);
			return;
		}

		vst->line = lines[a];
		slope_things_.push_back(std::move(vst));

		return;
	}

	log::warning("Vavoom slope thing {} has no matching line with first arg {}", thing.index(), tid);
}

void SlopeSpecials::addCopySlopeThing(const MapThing& thing, SectorSurfaceType surface_type)
{
	auto cst   = std::make_unique<CopySlopeThing>(surface_type);
	cst->thing = &thing;

	// Get containing (target) sector
	cst->target = map_->sectors().atPos(thing.position());
	if (!cst->target)
	{
		log::warning("Copy slope thing {} is not within a sector", thing.index());
		return;
	}

	// First argument is the tag of a sector whose slope should be copied
	const int tag = thing.arg(0);
	if (!tag)
	{
		log::warning("Ignoring slope copy thing {} in sector {} with no argument", thing.index(), cst->target->index());
		return;
	}

	// Model sector to copy is the first with the tag
	cst->model = map_->sectors().firstWithId(tag);
	if (!cst->model)
	{
		log::warning(
			"Ignoring slope copy thing {} in sector {}; no sectors have target tag {}",
			thing.index(),
			cst->target->index(),
			tag);
		return;
	}

	copy_slope_things_.push_back(std::move(cst));
	copy_slope_things_sorted_ = false;
}

void SlopeSpecials::removeCopySlopeThing(const MapThing& thing)
{
	unsigned i = 0;
	while (i < copy_slope_things_.size())
	{
		if (copy_slope_things_[i]->thing == &thing)
		{
			vectorAddUnique(sectors_to_update_, copy_slope_things_[i]->target);
			copy_slope_things_.erase(copy_slope_things_.begin() + i);
			continue;
		}
		i++;
	}
}

void SlopeSpecials::applyCopySlopeThings(const MapSector& sector)
{
	// Sort by thing index if needed
	if (!copy_slope_things_sorted_)
	{
		std::sort(
			copy_slope_things_.begin(),
			copy_slope_things_.end(),
			[](const auto& a, const auto& b) { return a->thing->index() < b->thing->index(); });

		copy_slope_things_sorted_ = true;
	}

	// Apply each CopySlopeThing in order
	for (const auto& cst : copy_slope_things_)
	{
		if (cst->target == &sector)
			cst->apply();
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

void SlopeSpecials::addPlaneCopy(const MapLine& line)
{
	PlaneCopySpecial pc{ SectorSurfaceType::Floor };
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
			plane_copy_specials_.push_back(std::make_unique<PlaneCopySpecial>(pc));
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
			plane_copy_specials_.push_back(std::make_unique<PlaneCopySpecial>(pc));
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
			plane_copy_specials_.push_back(std::make_unique<PlaneCopySpecial>(pc));
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
			plane_copy_specials_.push_back(std::make_unique<PlaneCopySpecial>(pc));
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
			plane_copy_specials_.push_back(std::make_unique<PlaneCopySpecial>(pc));
		}
		// Floor back -> front
		else if (line.arg(4) & 2)
		{
			pc.surface_type = SectorSurfaceType::Floor;
			pc.target       = line.frontSector();
			pc.model        = line.backSector();
			plane_copy_specials_.push_back(std::make_unique<PlaneCopySpecial>(pc));
		}

		// Ceiling front -> back
		if (line.arg(4) & 4)
		{
			pc.surface_type = SectorSurfaceType::Ceiling;
			pc.target       = line.backSector();
			pc.model        = line.frontSector();
			plane_copy_specials_.push_back(std::make_unique<PlaneCopySpecial>(pc));
		}
		// Ceiling back -> front
		else if (line.arg(4) & 8)
		{
			pc.surface_type = SectorSurfaceType::Ceiling;
			pc.target       = line.frontSector();
			pc.model        = line.backSector();
			plane_copy_specials_.push_back(std::make_unique<PlaneCopySpecial>(pc));
		}
	}
}

void SlopeSpecials::addSRB2PlaneCopy(const MapLine& line, SectorSurfaceType surface_type)
{
	auto pc    = std::make_unique<PlaneCopySpecial>(surface_type);
	pc->line   = &line;
	pc->target = line.frontSector();

	if (!pc->target)
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

	pc->model = tagged;

	plane_copy_specials_.push_back(std::move(pc));
}

void SlopeSpecials::removePlaneCopy(const MapLine& line)
{
	unsigned i = 0;
	while (i < plane_copy_specials_.size())
	{
		if (plane_copy_specials_[i]->line == &line)
		{
			vectorAddUnique(sectors_to_update_, plane_copy_specials_[i]->target);
			plane_copy_specials_.erase(plane_copy_specials_.begin() + i);
			continue;
		}
		i++;
	}
}

void SlopeSpecials::applyPlaneCopy(const MapSector& sector)
{
	// Sort by line index (descending) if needed
	if (!plane_copy_specials_sorted_)
	{
		std::sort(
			plane_copy_specials_.begin(),
			plane_copy_specials_.end(),
			[](const auto& a, const auto& b) { return a->line->index() > b->line->index(); });

		plane_copy_specials_sorted_ = true;
	}

	// Since the PlaneCopySpecials are sorted by line index descending,
	// we can just apply the first one we find for each surface type
	bool pc_floor = false;
	bool pc_ceil  = false;
	for (const auto& pc : plane_copy_specials_)
	{
		if (!pc_floor && pc->isTarget(&sector, SectorSurfaceType::Floor))
		{
			pc->apply();
			pc_floor = true;
		}
		if (!pc_ceil && pc->isTarget(&sector, SectorSurfaceType::Ceiling))
		{
			pc->apply();
			pc_ceil = true;
		}
	}
}

void SlopeSpecials::addSRB2VertexSlope(const MapLine& line, SectorSurfaceType surface_type, bool front)
{
	auto target = front ? line.frontSector() : line.backSector();
	if (!target)
	{
		log::warning(
			"Ignoring vertex slope special on line {}, the target back/front sector for this line don't exist",
			line.index());
		return;
	}

	auto sidedef = front ? line.s1() : line.s2();

	const MapThing* vertices[3];
	unsigned        count = 0;
	for (auto& thing : map_->things())
	{
		if (thing->type() != 750)
			continue;

		if ((line.flagSet(8192)
			 && (thing->angle() == line.id() || thing->angle() == sidedef->texOffsetX()
				 || thing->angle() == sidedef->texOffsetY()))
			|| thing->angle() == line.id())
		{
			vertices[count++] = thing;
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

	srb2_vertex_slope_specials_.push_back(
		std::make_unique<SRB2VertexSlopeSpecial>(line, *target, vertices, surface_type));

	srb2_vertex_slope_specials_sorted_ = false;
}

void SlopeSpecials::removeSRB2VertexSlope(const MapLine& line)
{
	unsigned i = 0;
	while (i < srb2_vertex_slope_specials_.size())
	{
		if (srb2_vertex_slope_specials_[i]->line == &line)
		{
			vectorAddUnique(sectors_to_update_, srb2_vertex_slope_specials_[i]->target);
			srb2_vertex_slope_specials_.erase(srb2_vertex_slope_specials_.begin() + i);
			continue;
		}
		i++;
	}
}

void SlopeSpecials::applySRB2VertexSlopes(const MapSector& sector)
{
	// Sort by line index (descending) if needed
	if (!srb2_vertex_slope_specials_sorted_)
	{
		std::sort(
			srb2_vertex_slope_specials_.begin(),
			srb2_vertex_slope_specials_.end(),
			[](const auto& a, const auto& b) { return a->line->index() > b->line->index(); });

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

		if (!vs_floor && pa->isTarget(&sector, SectorSurfaceType::Floor))
		{
			pa->apply();
			vs_floor = true;
		}
		if (!vs_ceil && pa->isTarget(&sector, SectorSurfaceType::Ceiling))
		{
			pa->apply();
			vs_ceil = true;
		}
	}
}
