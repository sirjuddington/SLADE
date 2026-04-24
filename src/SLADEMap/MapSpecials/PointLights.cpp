
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    PointLights.cpp
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
#include "PointLights.h"
#include "Game/Configuration.h"
#include "Game/ThingType.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapThing.h"
#include "SLADEMap/SLADEMap.h"

using namespace slade;
using namespace map;

PointLights::PointLights(const SLADEMap& map) : map_{ &map } {}

PointLights::~PointLights() = default;

void PointLights::processThing(const MapThing& thing)
{
	// Currently only zdoom types are implemented
	if (game::configuration().currentPort() != "zdoom")
		return;

	// Check type is marked as a dynamic light
	auto& tt = game::configuration().thingType(thing.type());
	if (!tt.dynamicLight())
		return;

	auto type = thing.type();
	switch (type)
	{
		// Vavoom
	case 1502: // White
		addPointLight(thing, 255, 255, 255, thing.arg(0));
		break;
	case 1503: // Coloured
		addPointLight(thing, thing.arg(1), thing.arg(2), thing.arg(3), thing.arg(0));
		break;

		// Point Light
	case 9800: // Static
	case 9801: // Pulsing
	case 9802: // Flickering
	case 9803: // Sector-Synced
	case 9804: // Random Flickering
		addPointLight(
			thing, thing.arg(0), thing.arg(1), thing.arg(2), thing.arg(3), PointLight::Type::Normal, type == 9803);
		break;

		// Additive Point Light
	case 9810: // Static
	case 9811: // Pulsing
	case 9812: // Flickering
	case 9813: // Sector-Synced
	case 9814: // Random Flickering
		addPointLight(
			thing, thing.arg(0), thing.arg(1), thing.arg(2), thing.arg(3), PointLight::Type::Additive, type == 9813);
		break;

		// Subtractive Point Light
	case 9820: // Static
	case 9821: // Pulsing
	case 9822: // Flickering
	case 9823: // Sector-Synced
	case 9824: // Random Flickering
		addPointLight(
			thing, thing.arg(0), thing.arg(1), thing.arg(2), thing.arg(3), PointLight::Type::Subtractive, type == 9823);
		break;

		// Attenuated Point Light
	case 9830: // Static
	case 9831: // Pulsing
	case 9832: // Flickering
	case 9833: // Sector-Synced
	case 9834: // Random Flickering
		addPointLight(
			thing, thing.arg(0), thing.arg(1), thing.arg(2), thing.arg(3), PointLight::Type::Attenuated, type == 9833);
		break;

	default: break;
	}
}

void PointLights::thingUpdated(const MapThing& thing)
{
	thingDeleted(thing); // Not really deleted but we just want to re-process it
	processThing(thing);
}

void PointLights::thingDeleted(const MapThing& thing)
{
	unsigned i = 0;
	while (i < point_lights_.size())
	{
		if (point_lights_[i].thing == &thing)
		{
			point_lights_.erase(point_lights_.begin() + i);
			continue;
		}
		i++;
	}
}

void PointLights::addPointLight(
	const MapThing&  thing,
	u8               r,
	u8               g,
	u8               b,
	unsigned         radius,
	PointLight::Type type,
	bool             radius_sector_sync)
{
	// Determine z position
	auto sector   = map_->thingParentSector(thing);
	auto position = Vec3d{ thing.xPos(), thing.yPos(), thing.zPos() };
	if (sector)
		position.z += sector->floor().plane.heightAt(position.x, position.y);

	// Adjust radius by sector light level if needed
	if (radius_sector_sync && sector)
		radius = static_cast<unsigned>(radius / 8.0 * sector->lightLevel());

	point_lights_.push_back(
		PointLight{ .thing = &thing, .position = position, .r = r, .g = g, .b = b, .radius = radius, .type = type });
}
