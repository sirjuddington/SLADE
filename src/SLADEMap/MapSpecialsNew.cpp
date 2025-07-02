
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapSpecials.cpp
// Description: Various functions for processing map specials and scripts,
//              mostly for visual effects (transparency, colours, slopes, etc.)
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
#include "MapSpecialsNew.h"
#include "Game/Configuration.h"
#include "General/Defs.h"
#include "Geometry/Geometry.h"
#include "Geometry/Plane.h"
#include "MapObject/MapLine.h"
#include "MapObject/MapSector.h"
#include "MapObject/MapSide.h"
#include "MapObject/MapThing.h"
#include "MapObject/MapVertex.h"
#include "MapObjectList/LineList.h"
#include "MapObjectList/SectorList.h"
#include "MapObjectList/ThingList.h"
#include "MapObjectList/VertexList.h"
#include "SLADEMap.h"
#include "SlopeSpecials/CopySlopeThing.h"
#include "SlopeSpecials/LineSlopeThing.h"
#include "SlopeSpecials/PlaneAlignSpecial.h"
#include "SlopeSpecials/PlaneCopySpecial.h"
#include "SlopeSpecials/SRB2VertexSlopeSpecial.h"
#include "SlopeSpecials/SectorTiltThing.h"
#include "SlopeSpecials/SlopeSpecials.h"
#include "SlopeSpecials/VavoomSlopeThing.h"
#include "Utility/Vector.h"

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
// MapSpecials Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// MapSpecials constructor
// -----------------------------------------------------------------------------
MapSpecialsNew::MapSpecialsNew(SLADEMap& map) : map_{ &map }
{
	slope_specials_ = std::make_unique<SlopeSpecials>(map);
}

// -----------------------------------------------------------------------------
// MapSpecials destructor
// -----------------------------------------------------------------------------
MapSpecialsNew::~MapSpecialsNew() = default;

// -----------------------------------------------------------------------------
// (Re-)Process all specials in the map
// -----------------------------------------------------------------------------
void MapSpecialsNew::processAllSpecials()
{
	// Clear existing specials
	slope_specials_->clearSpecials();

	// Process all line specials
	for (const auto line : map_->lines())
	{
		processLineSpecial(*line);
		slope_specials_->processLineSpecial(*line);
	}

	// Process all things
	for (const auto thing : map_->things())
	{
		processThing(*thing);
		slope_specials_->processThing(*thing);
	}

	// Update planes for all sectors
	for (const auto sector : map_->sectors())
		slope_specials_->updateSectorPlanes(*sector);
}

void MapSpecialsNew::processLineSpecial(const MapLine& line) {}

void MapSpecialsNew::processThing(const MapThing& thing) {}

void MapSpecialsNew::lineUpdated(const MapLine& line)
{
	// Update slope specials
	slope_specials_->lineUpdated(line, !bulk_update_);

	// Re-process
	processLineSpecial(line);
}

void MapSpecialsNew::sectorUpdated(MapSector& sector)
{
	// Update slope specials
	slope_specials_->sectorUpdated(sector, !bulk_update_);
}

void MapSpecialsNew::thingUpdated(const MapThing& thing)
{
	// Update slope specials
	slope_specials_->thingUpdated(thing, !bulk_update_);

	// Re-process
	processThing(thing);
}

void MapSpecialsNew::objectUpdated(MapObject& object)
{
	switch (object.objType())
	{
	case map::ObjectType::Object: break;
	case map::ObjectType::Vertex: break;
	case map::ObjectType::Line:   lineUpdated(dynamic_cast<MapLine&>(object)); break;
	case map::ObjectType::Side:   break;
	case map::ObjectType::Sector: sectorUpdated(dynamic_cast<MapSector&>(object)); break;
	case map::ObjectType::Thing:  thingUpdated(dynamic_cast<MapThing&>(object)); break;
	}
}

void MapSpecialsNew::objectsUpdated(const vector<MapObject*>& objects)
{
	// Update specials for all given objects
	bulk_update_ = true;
	for (const auto obj : objects)
		objectUpdated(*obj);
	bulk_update_ = false;

	// Update planes for sectors that need updating
	slope_specials_->updateOutdatedSectorPlanes();
}
