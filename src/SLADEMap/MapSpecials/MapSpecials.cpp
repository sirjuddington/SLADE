
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
#include "PointLights.h"
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
#include "UI/UI.h"
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
	point_lights_        = std::make_unique<PointLights>(map);

	// Flag objects as updated when they're modified, so that specials will be
	// reprocessed for them on next access
	connections_ += map.signals().object_modified.connect(
		[&](const vector<MapObject*>& objects)
		{
			for (auto obj : objects)
				vectorAddUnique(updated_objects_, obj);
		});

	// Handle deleted objects
	connections_ += map.signals().object_deleted.connect(
		[&](const vector<MapObject*>& objects)
		{
			for (auto obj : objects)
			{
				// Remove from pending updates
				std::erase(updated_objects_, obj);

				switch (obj->objType())
				{
				case MapObject::Type::Line:
					extrafloor_specials_->lineDeleted(*dynamic_cast<MapLine*>(obj));
					render_specials_->lineDeleted(*dynamic_cast<MapLine*>(obj));
					slope_specials_->lineDeleted(*dynamic_cast<MapLine*>(obj));
					break;
				case MapObject::Type::Sector:
					extrafloor_specials_->sectorDeleted(*dynamic_cast<MapSector*>(obj));
					slope_specials_->sectorDeleted(*dynamic_cast<MapSector*>(obj));
					break;
				case MapObject::Type::Thing:
					point_lights_->thingDeleted(*dynamic_cast<MapThing*>(obj));
					slope_specials_->thingDeleted(*dynamic_cast<MapThing*>(obj));
					break;
				default: break;
				}
			}
		});
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
// Returns the fade colour for the given [sector]
// -----------------------------------------------------------------------------
ColRGBA MapSpecials::sectorFadeColour(const MapSector& sector) const
{
	// Check for UDMF
	if (map_->currentFormat() == MapFormat::UDMF)
	{
		// Get sector fade colour (if supported and specified)
		if (game::configuration().featureSupported(game::UDMFFeature::SectorFog) && sector.hasProp("fadecolor"))
		{
			auto colour = colour::fromInt(sector.intProperty("fadecolor"));
			colour.a    = sector.lightAt(SectorPart::Interior); // TODO: fogdensity
			return colour;
		}
	}

	updateSpecials();
	auto colour = render_specials_->sectorFadeColour(sector);
	colour.a    = sector.lightAt(SectorPart::Interior);

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

// -----------------------------------------------------------------------------
// Returns the lighting info for the given [sector] at [where], taking into
// account any ExtraFloors
// -----------------------------------------------------------------------------
SectorLighting MapSpecials::sectorLightingAt(
	const MapSector& sector,
	SectorPart       where,
	optional<Plane>  plane,
	bool             below_plane) const
{
	if (where == SectorPart::Ceiling)
		return { .brightness = sector.lightAt(where),
				 .colour     = sectorColour(sector, where),
				 .fog        = sectorFadeColour(sector) };

	updateSpecials();
	auto extrafloors = extrafloor_specials_->extraFloors(sector);

	if (where == SectorPart::Floor)
	{
		// Check if ExtraFloor lighting affects the floor
		if (!extrafloors.empty() && extrafloors.back().lighting_below.has_value())
			return extrafloors.back().lighting_below.value();

		// No ExtraFloor lighting, return sector floor lighting
		return { .brightness = sector.lightAt(where),
				 .colour     = sectorColour(sector, where),
				 .fog        = sectorFadeColour(sector) };
	}

	// Interior - if no ExtraFloors or no plane given just return sector interior lighting
	auto lighting_interior = SectorLighting{ .brightness = sector.lightAt(SectorPart::Interior),
											 .colour     = sectorColour(sector, SectorPart::Interior),
											 .fog        = sectorFadeColour(sector) };
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
	auto underneath = nearest->hasFlag(ExtraFloor::Flags::FlatAtCeiling)
						  ? true
						  : plane_height < nearest->plane_bottom.heightAt(sector.getPoint(MapObject::Point::Mid));

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
	// First, check with render specials (will override UDMF properties if both are present)
	updateSpecials();
	if (auto t = render_specials_->lineTranslucency(line); t.has_value())
		return t;

	// Check for UDMF alpha/renderstyle properties
	if (map_->currentFormat() == MapFormat::UDMF
		&& game::configuration().featureSupported(game::UDMFFeature::LineTransparency)
		&& (line.hasProp("alpha") || line.hasProp("renderstyle")))
	{
		LineTranslucency translucency;

		// Check for alpha property
		if (line.hasProp("alpha"))
		{
			float alpha        = line.floatProperty("alpha");
			translucency.alpha = std::clamp(alpha, 0.0f, 1.0f);
		}

		// Check for "add" renderstyle property
		if (line.hasProp("renderstyle") && strutil::equalCI(line.stringProperty("renderstyle"), "add"))
			translucency.additive = true;

		return translucency;
	}

	// No translucency
	return std::nullopt;
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

const vector<PointLight>& MapSpecials::pointLights() const
{
	updateSpecials();
	return point_lights_->pointLights();
}

const PointLight* MapSpecials::pointLightForThing(const MapThing& thing) const
{
	for (const auto& pl : point_lights_->pointLights())
	{
		if (pl.thing == &thing)
			return &pl;
	}

	return nullptr;
}

// -----------------------------------------------------------------------------
// (Re-)Process all specials in the map
// -----------------------------------------------------------------------------
void MapSpecials::processAllSpecials() const
{
	specials_updating_ = true;

	// Clear existing specials
	slope_specials_->clearSpecials();
	extrafloor_specials_->clearSpecials();
	render_specials_->clearSpecials();
	point_lights_->clear();

	// Setup splash message/progress
	ui::setSplashProgressMessage("Processing map specials...");
	ui::setSplashProgress(0.0f);
	auto total = map_->nLines()
				 + map_->nThings()
				 + (map_->nSectors() * 2); // Lines + Things + Sectors (slopes and extrafloors)
	int count = 0;

	// Process all line specials
	for (const auto line : map_->lines())
	{
		processLineSpecial(*line);
		if (++count % 10 == 0)
			ui::setSplashProgress(static_cast<float>(count) / total);
	}

	// Process all things
	for (const auto thing : map_->things())
	{
		processThing(*thing);
		if (++count % 10 == 0)
			ui::setSplashProgress(static_cast<float>(count) / total);
	}

	// Update all sector info
	for (const auto sector : map_->sectors())
	{
		// All slopes first because they can affect extrafloors
		slope_specials_->updateSectorPlanes(*sector);
		if (++count % 10 == 0)
			ui::setSplashProgress(static_cast<float>(count) / total);
	}
	for (const auto sector : map_->sectors())
	{
		extrafloor_specials_->updateSectorExtraFloors(*sector);
		if (++count % 10 == 0)
			ui::setSplashProgress(static_cast<float>(count) / total);
	}

	specials_updated_  = app::runTimer();
	specials_updating_ = false;
	updated_objects_.clear();
}

void MapSpecials::updateSpecials() const
{
	if (updated_objects_.empty() || specials_updating_)
		return;

	bool updated       = false;
	specials_updating_ = true;
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
			point_lights_->thingUpdated(*thing);
			break;
		}
		}
	}

	// Update planes for sectors that need updating
	slope_specials_->updateOutdatedSectorPlanes();

	if (updated)
		specials_updated_ = app::runTimer();

	updated_objects_.clear();
	specials_updating_ = false;
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
	point_lights_->processThing(thing);
}
