
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
	const auto& ttype = game::configuration().thingType(thing.type());

	// Not a point light
	if (ttype.pointLight().empty())
		return;

	// Determine z position
	auto sector   = map_->thingParentSector(thing);
	auto position = Vec3d{ thing.xPos(), thing.yPos(), thing.zPos() };
	if (sector)
		position.z += sector->floor().plane.heightAt(position.x, position.y);

	// ZDoom point light
	if (ttype.pointLight() == "zdoom")
	{
		point_lights_.push_back(
			PointLight{ .thing    = &thing,
						.position = position,
						.r        = static_cast<u8>(thing.arg(0)),
						.g        = static_cast<u8>(thing.arg(1)),
						.b        = static_cast<u8>(thing.arg(2)),
						.radius   = static_cast<u8>(thing.arg(3)) });
	}

	// Vavoom point light
	else if (ttype.pointLight() == "vavoom")
	{
		point_lights_.push_back(
			PointLight{ .thing    = &thing,
						.position = position,
						.r        = static_cast<u8>(thing.arg(1)),
						.g        = static_cast<u8>(thing.arg(2)),
						.b        = static_cast<u8>(thing.arg(3)),
						.radius   = static_cast<u8>(thing.arg(0)) });
	}

	// Vavoom white light
	else if (ttype.pointLight() == "vavoom_white")
	{
		point_lights_.push_back(
			PointLight{ .thing    = &thing,
						.position = position,
						.r        = 255,
						.g        = 255,
						.b        = 255,
						.radius   = static_cast<u8>(thing.arg(0)) });
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
