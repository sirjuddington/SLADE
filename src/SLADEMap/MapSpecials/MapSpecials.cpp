
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
#include "App.h"
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
#include "Utility/Vector.h"

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
	extrafloor_specials_ = std::make_unique<ExtraFloorSpecials>(map, *this);
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
	updateSpecials();
	return extrafloor_specials_->extraFloors(*sector);
}

// -----------------------------------------------------------------------------
// Returns true if the given [sector] has any ExtraFloors
// -----------------------------------------------------------------------------
bool MapSpecials::sectorHasExtraFloors(const MapSector* sector) const
{
	updateSpecials();
	return extrafloor_specials_->hasExtraFloors(*sector);
}

// -----------------------------------------------------------------------------
// Returns the colour for the given [sector] at [where]
// -----------------------------------------------------------------------------
ColRGBA MapSpecials::sectorColour(const MapSector& sector, SectorPart where) const
{
	auto colour = ColRGBA::WHITE;

	// Check for UDMF
	if (map_->currentFormat() == MapFormat::UDMF)
	{
		// Get sector light colour (if supported and specified)
		if (game::configuration().featureSupported(game::UDMFFeature::SectorColor) && sector.hasProp("lightcolor"))
			colour = colour::fromInt(sector.intProperty("lightcolor"));
	}

	return colour;
}

// -----------------------------------------------------------------------------
// Returns the floor height at [pos] in the given [sector], taking into account
// any ExtraFloors
// -----------------------------------------------------------------------------
float MapSpecials::sectorFloorHeightAt(const MapSector& sector, Vec3d pos) const
{
	updateSpecials();

	float height = sector.floor().plane.heightAt(pos.xy());

	for (const auto& ef : extrafloor_specials_->extraFloors(sector))
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

SectorLighting MapSpecials::sectorLightingAt(
	const MapSector& sector,
	SectorPart       where,
	optional<Plane>  plane,
	bool             below_plane) const
{
	if (where == SectorPart::Ceiling)
		return { .brightness = sector.lightAt(where), .colour = sectorColour(sector, where), .fog = ColRGBA::BLACK };

	auto extrafloors = extrafloor_specials_->extraFloors(sector);

	if (where == SectorPart::Floor)
	{
		// Check if ExtraFloor lighting affects the floor
		if (!extrafloors.empty() && extrafloors.back().lighting_below.has_value())
			return extrafloors.back().lighting_below.value();

		// No ExtraFloor lighting, return sector floor lighting
		return { .brightness = sector.lightAt(where), .colour = sectorColour(sector, where), .fog = ColRGBA::BLACK };
	}

	// Interior - if no ExtraFloors or no plane given just return sector interior lighting
	auto lighting_interior = SectorLighting{ .brightness = sector.lightAt(SectorPart::Interior),
											 .colour     = sectorColour(sector, SectorPart::Interior),
											 .fog        = ColRGBA::BLACK };
	if (extrafloors.empty() || !plane.has_value())
		return lighting_interior;

	// Determine height of given plane at sector midpoint
	auto plane_height = plane->heightAt(sector.getPoint(MapObject::Point::Mid));
	if (below_plane)
		plane_height -= 0.01f;
	else
		plane_height += 0.01f;
	if (plane_height > extrafloors[0].height)
		return lighting_interior; // Above all ExtraFloors, return sector interior lighting

	// Check ExtraFloors from top to bottom
	const ExtraFloor* nearest = nullptr;
	for (auto& ef : extrafloors)
	{
		if (plane_height <= ef.height)
			nearest = &ef;
		else
			break;
	}
	if (!nearest)
		return lighting_interior; // Shouldn't happen

	// Determine if we're underneath or inside the nearest ExtraFloor
	bool underneath = false;
	if (nearest->hasFlag(ExtraFloor::Flags::FlatAtCeiling))
		underneath = true;
	else
		underneath = plane_height < nearest->plane_bottom.heightAt(sector.getPoint(MapObject::Point::Mid));

	// Return appropriate lighting
	if (underneath && nearest->lighting_below.has_value())
		return nearest->lighting_below.value();
	if (!underneath && nearest->lighting_inside.has_value())
		return nearest->lighting_inside.value();

	return lighting_interior;
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
	updateSpecials();
	return render_specials_->lineTranslucency(line);
}

// -----------------------------------------------------------------------------
// Returns the colour for the given [side] at [where]
// -----------------------------------------------------------------------------
ColRGBA MapSpecials::sideColour(const MapSide& side, SidePart where, bool fullbright) const
{
	auto colour = sectorColour(*side.sector(), SectorPart::Interior);

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
		slope_specials_->updateSectorPlanes(*sector); // All slopes first because they can affect extrafloors
	for (const auto sector : map_->sectors())
		extrafloor_specials_->updateSectorExtraFloors(*sector);

	specials_updated_ = app::runTimer();
}

void MapSpecials::updateSpecials() const
{
	if (updated_objects_.empty())
		return;

	bool updated = false;
	for (auto obj : updated_objects_)
	{
		switch (obj->objType())
		{
		case ObjectType::Object: break;
		case ObjectType::Vertex: break;

		case ObjectType::Side:
		{
			auto side = dynamic_cast<MapSide*>(obj);
			extrafloor_specials_->sideUpdated(*side, false);
			break;
		}

		case ObjectType::Line:
		{
			auto line = dynamic_cast<MapLine*>(obj);
			updated |= slope_specials_->lineUpdated(*line, false);
			render_specials_->lineUpdated(*line);
			updated |= extrafloor_specials_->lineUpdated(*line);
			break;
		}

		case ObjectType::Sector:
		{
			auto sector = dynamic_cast<MapSector*>(obj);
			updated |= slope_specials_->sectorUpdated(*sector, false);
			updated |= extrafloor_specials_->sectorUpdated(*sector);
			break;
		}

		case ObjectType::Thing:
		{
			auto thing = dynamic_cast<MapThing*>(obj);
			updated |= slope_specials_->thingUpdated(*thing, false);
			break;
		}
		}
	}

	// Update planes for sectors that need updating
	slope_specials_->updateOutdatedSectorPlanes();

	if (updated)
		specials_updated_ = app::runTimer();

	updated_objects_.clear();
}

void MapSpecials::objectUpdated(MapObject& object) const
{
	vectorAddUnique(updated_objects_, &object);
}

void MapSpecials::objectsUpdated(const vector<MapObject*>& objects) const
{
	for (auto obj : objects)
		vectorAddUnique(updated_objects_, obj);
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
