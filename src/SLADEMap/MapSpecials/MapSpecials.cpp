
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
#include "MapSpecials.h"
#include "Game/Configuration.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapThing.h"
#include "SLADEMap/MapObjectList/LineList.h"
#include "SLADEMap/MapObjectList/SectorList.h"
#include "SLADEMap/MapObjectList/ThingList.h"
#include "SLADEMap/SLADEMap.h"
#include "SlopeSpecials.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// MapSpecials Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// MapSpecials constructor
// -----------------------------------------------------------------------------
MapSpecials::MapSpecials(SLADEMap& map) : map_{ &map }
{
	slope_specials_ = std::make_unique<SlopeSpecials>(map);
}

// -----------------------------------------------------------------------------
// MapSpecials destructor
// -----------------------------------------------------------------------------
MapSpecials::~MapSpecials() = default;

// -----------------------------------------------------------------------------
// (Re-)Process all specials in the map
// -----------------------------------------------------------------------------
void MapSpecials::processAllSpecials()
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

void MapSpecials::processLineSpecial(const MapLine& line)
{
	slope_specials_->processLineSpecial(line);
}

void MapSpecials::processThing(const MapThing& thing)
{
	slope_specials_->processThing(thing);
}

void MapSpecials::lineUpdated(const MapLine& line)
{
	// Update slope specials
	slope_specials_->lineUpdated(line, !bulk_update_);

	// Re-process
	processLineSpecial(line);
}

void MapSpecials::sectorUpdated(MapSector& sector)
{
	// Update slope specials
	slope_specials_->sectorUpdated(sector, !bulk_update_);
}

void MapSpecials::thingUpdated(const MapThing& thing)
{
	// Update slope specials
	slope_specials_->thingUpdated(thing, !bulk_update_);

	// Re-process
	processThing(thing);
}

void MapSpecials::objectUpdated(MapObject& object)
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

void MapSpecials::objectsUpdated(const vector<MapObject*>& objects)
{
	// Update specials for all given objects
	bulk_update_ = true;
	for (const auto obj : objects)
		objectUpdated(*obj);
	bulk_update_ = false;

	// Update planes for sectors that need updating
	slope_specials_->updateOutdatedSectorPlanes();
}
