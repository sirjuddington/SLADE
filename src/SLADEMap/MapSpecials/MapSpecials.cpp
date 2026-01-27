
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
#include "ExtraFloorSpecials.h"
#include "Game/Configuration.h"
#include "LineTranslucency.h"
#include "RenderSpecials.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapSide.h"
#include "SLADEMap/MapObject/MapThing.h"
#include "SLADEMap/MapObjectList/LineList.h"
#include "SLADEMap/MapObjectList/SectorList.h"
#include "SLADEMap/MapObjectList/ThingList.h"
#include "SLADEMap/SLADEMap.h"
#include "SlopeSpecials.h"
#include "Utility/Colour.h"

using namespace slade;
using namespace map;


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
	slope_specials_      = std::make_unique<SlopeSpecials>(map);
	extrafloor_specials_ = std::make_unique<ExtraFloorSpecials>(map);
	render_specials_     = std::make_unique<RenderSpecials>(map);
}

// -----------------------------------------------------------------------------
// MapSpecials destructor
// -----------------------------------------------------------------------------
MapSpecials::~MapSpecials() = default;

// -----------------------------------------------------------------------------
// Returns the ExtraFloors for the given [sector]
// -----------------------------------------------------------------------------
const vector<ExtraFloor>& MapSpecials::sectorExtraFloors(const MapSector* sector) const
{
	return extrafloor_specials_->extraFloors(sector);
}

// -----------------------------------------------------------------------------
// Returns true if the given [sector] has any ExtraFloors
// -----------------------------------------------------------------------------
bool MapSpecials::sectorHasExtraFloors(const MapSector* sector) const
{
	return extrafloor_specials_->hasExtraFloors(sector);
}

// -----------------------------------------------------------------------------
// Returns the colour for the given [sector] at [where]
// -----------------------------------------------------------------------------
ColRGBA MapSpecials::sectorColour(const MapSector& sector, SectorPart where, bool fullbright) const
{
	auto colour = ColRGBA::WHITE;
	u8   light  = fullbright ? 255 : sector.lightAt(where);

	// Check for UDMF
	if (map_->currentFormat() == MapFormat::UDMF)
	{
		// Get sector light colour (if supported and specified)
		if (game::configuration().featureSupported(game::UDMFFeature::SectorColor) && sector.hasProp("lightcolor"))
			colour = colour::fromInt(sector.intProperty("lightcolor"));
	}

	// If fullbright, just return the colour
	if (fullbright)
		return colour;

	// Otherwise, apply light level
	auto mult = static_cast<float>(light) / 255.0f;
	return colour.ampf(mult, mult, mult, 1.0f);
}

// -----------------------------------------------------------------------------
// Returns the floor height at [pos] in the given [sector], taking into account
// any ExtraFloors
// -----------------------------------------------------------------------------
float MapSpecials::sectorFloorHeightAt(const MapSector& sector, Vec3d pos) const
{
	float height = sector.floor().plane.heightAt(pos.xy());

	for (const auto& ef : extrafloor_specials_->extraFloors(&sector))
	{
		if (ef.hasFlag(ExtraFloor::Flags::Solid))
		{
			auto ef_height = ef.plane_top.heightAt(pos);
			if (ef_height > height && pos.z >= ef_height)
				height = ef_height;
		}
	}

	return height;
}

// -----------------------------------------------------------------------------
// Returns the translucency info for the given [line], if any is set via UDMF
// or line specials
// -----------------------------------------------------------------------------
optional<LineTranslucency> MapSpecials::lineTranslucency(const MapLine& line) const
{
	// First, check for UDMF alpha/renderstyle properties (this will override specials)
	// TODO: Does it override? Can alpha/renderstyle be mixed and matched with translucency line specials?
	if (map_->currentFormat() == MapFormat::UDMF
		&& game::configuration().featureSupported(game::UDMFFeature::LineTransparency)
		&& (line.hasProp("translucency") || line.hasProp("alpha")))
	{
		LineTranslucency translucency;

		// Check for alpha property
		if (line.hasProp("alpha"))
		{
			float alpha        = line.floatProperty("alpha");
			translucency.alpha = std::clamp(alpha, 0.0f, 1.0f);
		}

		// Check for "add" renderstyle property
		if (line.hasProp("renderstyle") && strutil::equalCI(line.stringProperty("renderstyle"), "additive"))
			translucency.additive = true;

		return translucency;
	}

	// Otherwise check with render specials
	return render_specials_->lineTranslucency(line);
}

// -----------------------------------------------------------------------------
// Returns the colour for the given [side] at [where]
// -----------------------------------------------------------------------------
ColRGBA MapSpecials::sideColour(const MapSide& side, SidePart where, bool fullbright) const
{
	auto colour = sectorColour(*side.sector(), SectorPart::Interior, true);

	if (fullbright)
		return colour;

	auto light = side.light();
	auto mult  = static_cast<float>(light) / 255.0f;
	return colour.ampf(mult, mult, mult, 1.0f);
}

// -----------------------------------------------------------------------------
// (Re-)Process all specials in the map
// -----------------------------------------------------------------------------
void MapSpecials::processAllSpecials() const
{
	// Clear existing specials
	slope_specials_->clearSpecials();
	extrafloor_specials_->clearSpecials();
	render_specials_->clearSpecials();

	// Process all line specials
	for (const auto line : map_->lines())
		processLineSpecial(*line);

	// Process all things
	for (const auto thing : map_->things())
		processThing(*thing);

	// Update all sector info
	for (const auto sector : map_->sectors())
	{
		slope_specials_->updateSectorPlanes(*sector);
		extrafloor_specials_->updateSectorExtraFloors(sector);
	}
}

void MapSpecials::lineUpdated(const MapLine& line) const
{
	slope_specials_->lineUpdated(line, !bulk_update_);
	render_specials_->lineUpdated(line);
}

void MapSpecials::sectorUpdated(MapSector& sector) const
{
	slope_specials_->sectorUpdated(sector, !bulk_update_);
}

void MapSpecials::thingUpdated(const MapThing& thing) const
{
	slope_specials_->thingUpdated(thing, !bulk_update_);
}

void MapSpecials::objectUpdated(MapObject& object) const
{
	switch (object.objType())
	{
	case ObjectType::Object: break;
	case ObjectType::Vertex: break;
	case ObjectType::Line:   lineUpdated(dynamic_cast<MapLine&>(object)); break;
	case ObjectType::Side:   break;
	case ObjectType::Sector: sectorUpdated(dynamic_cast<MapSector&>(object)); break;
	case ObjectType::Thing:  thingUpdated(dynamic_cast<MapThing&>(object)); break;
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

void MapSpecials::processLineSpecial(const MapLine& line) const
{
	slope_specials_->processLineSpecial(line);
	extrafloor_specials_->processLineSpecial(line);
	render_specials_->processLineSpecial(line);
}

void MapSpecials::processThing(const MapThing& thing) const
{
	slope_specials_->processThing(thing);
}
