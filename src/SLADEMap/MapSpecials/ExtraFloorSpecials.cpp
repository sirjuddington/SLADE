
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2025 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ExtraFloorSpecials.cpp
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
#include "ExtraFloorSpecials.h"
#include "ExtraFloor.h"
#include "Game/Configuration.h"
#include "MapSpecials.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapSide.h"
#include "SLADEMap/MapObjectList/SectorList.h"
#include "SLADEMap/SLADEMap.h"
#include "Utility/Vector.h"

using namespace slade;
using namespace map;


ExtraFloorSpecials::ExtraFloorSpecials(SLADEMap& map, MapSpecials& map_specials) :
	map_{ &map },
	map_specials_{ &map_specials }
{
}

bool ExtraFloorSpecials::hasExtraFloors(const MapSector& sector) const
{
	return std::ranges::any_of(sector_extra_floors_, [&sector](const auto& sef) { return sef.sector == &sector; });
}

const vector<ExtraFloor>& ExtraFloorSpecials::extraFloors(const MapSector& sector)
{
	const auto it = std::ranges::find_if(
		sector_extra_floors_, [&sector](const auto& sef) { return sef.sector == &sector; });

	if (it != sector_extra_floors_.end())
		return it->extra_floors;

	static vector<ExtraFloor> empty;
	return empty;
}

void ExtraFloorSpecials::processLineSpecial(const MapLine& line)
{
	// ZDoom
	if (game::configuration().currentPort() == "zdoom")
	{
		// Sector_Set3dFloor
		if (line.special() == 160)
			addSet3dFloorSpecial(line);
	}
}

void ExtraFloorSpecials::clearSpecials()
{
	sector_extra_floors_.clear();
	set_3d_floor_specials_.clear();
	sectors_to_update_.clear();
}

void ExtraFloorSpecials::updateSectorExtraFloors(const MapSector& sector)
{
	clearExtraFloors(sector);

	SectorExtraFloors sef{ .sector = &sector };

	for (const auto& special : set_3d_floor_specials_)
		if (special.target == &sector)
			applySet3dFloorSpecial(special, sef);


	if (!sef.extra_floors.empty())
	{
		// Calculate ExtraFloor lighting
		// TODO: Fog
		optional<SectorLighting> lighting;
		for (auto& ef : sef.extra_floors)
		{
			// Reset lighting if flag set
			if (ef.hasFlag(ExtraFloor::Flags::ResetLighting))
				lighting = std::nullopt;

			// Inside
			if (!ef.hasFlag(ExtraFloor::Flags::DisableLighting))
			{
				ef.lighting_inside = { .brightness = ef.control_sector->lightAt(SectorPart::Interior),
									   .colour     = map_specials_->sectorColour(
                                           *ef.control_sector, SectorPart::Interior) };
			}
			else
				ef.lighting_inside = lighting;

			// Below
			if (!ef.hasFlag(ExtraFloor::Flags::LightingInsideOnly))
				lighting = ef.lighting_inside;
			ef.lighting_below = lighting;
		}

		// Add to list
		sector_extra_floors_.push_back(sef);
	}
}

void ExtraFloorSpecials::updateOutdatedSectorExtraFloors()
{
	for (const auto s : sectors_to_update_)
		updateSectorExtraFloors(*s);
	sectors_to_update_.clear();
}

bool ExtraFloorSpecials::lineUpdated(const MapLine& line, bool update_outdated)
{
	specials_updated_ = false;

	// Remove existing specials
	removeSet3dFloorSpecial(line);

	// Re-process
	processLineSpecial(line);

	if (specials_updated_ && update_outdated)
		updateOutdatedSectorExtraFloors();

	return specials_updated_;
}

bool ExtraFloorSpecials::sideUpdated(const MapSide& side, bool update_outdated)
{
	// Sector_Set3dFloor - if [side] is on a control line, it may be used as the
	// side texture of the ExtraFloor, so update all target sectors' render info
	for (const auto& special : set_3d_floor_specials_)
		if (special.line == side.parentLine())
			special.target->setRenderInfoUpdated();

	if (update_outdated)
		updateOutdatedSectorExtraFloors();

	return false;
}

bool ExtraFloorSpecials::sectorUpdated(const MapSector& sector, bool update_outdated)
{
	// Sector_Set3dFloor - if [sector] is a control sector, update all target sectors' ExtraFloors
	for (const auto& special : set_3d_floor_specials_)
		if (special.control_sector == &sector)
			vectorAddUnique(sectors_to_update_, special.target);

	if (update_outdated)
		updateOutdatedSectorExtraFloors();

	return false;
}

ExtraFloorSpecials::SectorExtraFloors* ExtraFloorSpecials::getSectorExtraFloors(const MapSector& sector)
{
	// Find existing extra floors for this sector
	if (auto it = std::ranges::find_if(
			sector_extra_floors_, [&sector](const auto& sef) { return sef.sector == &sector; });
		it == sector_extra_floors_.end())
	{
		// No existing extra floors, create new entry
		sector_extra_floors_.emplace_back(&sector);
		return &sector_extra_floors_.back();
	}
	else
		return &(*it);
}

void ExtraFloorSpecials::clearExtraFloors(const MapSector& sector)
{
	if (auto it = std::ranges::find_if(
			sector_extra_floors_, [&sector](const auto& sef) { return sef.sector == &sector; });
		it != sector_extra_floors_.end())
		sector_extra_floors_.erase(it);
}

void ExtraFloorSpecials::addExtraFloor(SectorExtraFloors& sef, const ExtraFloor& extra_floor)
{
	sef.extra_floors.push_back(extra_floor);
	sef.sector->setRenderInfoUpdated();

	// Sort extra floors by height (top-down)
	std::ranges::sort(sef.extra_floors, [](const ExtraFloor& a, const ExtraFloor& b) { return a.height > b.height; });
}

void ExtraFloorSpecials::removeExtraFloor(SectorExtraFloors& sef, const MapLine& control_line)
{
	unsigned i = 0;
	while (i < sef.extra_floors.size())
	{
		if (sef.extra_floors[i].control_line == &control_line)
		{
			sef.extra_floors.erase(sef.extra_floors.begin() + i);
			sef.sector->setRenderInfoUpdated();
			continue;
		}
		i++;
	}
}

void ExtraFloorSpecials::addSet3dFloorSpecial(const MapLine& line)
{
	// Line must have front sector
	auto control_sector = line.frontSector();
	if (!control_sector)
	{
		log::warning("Invalid Sector_Set3dFloor special on line {}: Line has no front sector", line.index());
		return;
	}

	auto tag          = line.arg(0);
	auto type         = line.arg(1);
	auto flags        = line.arg(2);
	auto alpha        = line.arg(3);
	auto tag_highbyte = line.arg(4); // TODO: Apply to tag if needed

	// Get all tagged sectors
	vector<MapSector*> target_sectors;
	map_->sectors().putAllWithId(tag, target_sectors);

	for (auto sector : target_sectors)
	{
		set_3d_floor_specials_.push_back(
			{ .line           = &line,
			  .target         = sector,
			  .control_sector = control_sector,
			  .type           = static_cast<Set3dFloorSpecial::Type>(type & 0x3),
			  .render_inside  = type & 0x4 || (type & 0x3) == 2,
			  .flags          = static_cast<u16>(flags),
			  .alpha          = alpha / 255.0f });
		vectorAddUnique(sectors_to_update_, const_cast<const MapSector*>(sector));
		specials_updated_ = true;
	}
}

void ExtraFloorSpecials::removeSet3dFloorSpecial(const MapLine& line)
{
	unsigned i = 0;
	while (i < set_3d_floor_specials_.size())
	{
		if (set_3d_floor_specials_[i].line == &line)
		{
			vectorAddUnique(sectors_to_update_, set_3d_floor_specials_[i].target);
			set_3d_floor_specials_.erase(set_3d_floor_specials_.begin() + i);
			specials_updated_ = true;
			continue;
		}
		i++;
	}
}

void ExtraFloorSpecials::applySet3dFloorSpecial(const Set3dFloorSpecial& special, SectorExtraFloors& sef)
{
	ExtraFloor ef;
	ef.control_line   = special.line;
	ef.control_sector = special.control_sector;
	ef.alpha          = special.alpha;

	auto midpoint = sef.sector->getPoint(MapObject::Point::Mid);
	if (special.type == Set3dFloorSpecial::Type::Vavoom)
	{
		ef.plane_top    = special.control_sector->floor().plane;
		ef.plane_bottom = special.control_sector->ceiling().plane;
		ef.height       = special.control_sector->floor().plane.heightAt(midpoint);
		ef.setFlag(ExtraFloor::Flags::Flipped);
	}
	else
	{
		ef.plane_top    = special.control_sector->ceiling().plane;
		ef.plane_bottom = special.control_sector->floor().plane;
		ef.height       = special.control_sector->ceiling().plane.heightAt(midpoint);
	}

	if (special.hasFlag(Set3dFloorSpecial::Flags::DisableLighting))
		ef.setFlag(ExtraFloor::Flags::DisableLighting);
	if (special.hasFlag(Set3dFloorSpecial::Flags::LightingInsideOnly))
		ef.setFlag(ExtraFloor::Flags::LightingInsideOnly);
	if (special.hasFlag(Set3dFloorSpecial::Flags::Fog))
		ef.setFlag(ExtraFloor::Flags::InnerFogEffect);
	if (special.hasFlag(Set3dFloorSpecial::Flags::FloorAtCeiling))
		ef.setFlag(ExtraFloor::Flags::FlatAtCeiling);
	if (special.hasFlag(Set3dFloorSpecial::Flags::UseUpperTexture))
		ef.setFlag(ExtraFloor::Flags::UseUpperTexture);
	if (special.hasFlag(Set3dFloorSpecial::Flags::UseLowerTexture))
		ef.setFlag(ExtraFloor::Flags::UseLowerTexture);
	if (special.hasFlag(Set3dFloorSpecial::Flags::TransAdd))
		ef.setFlag(ExtraFloor::Flags::AdditiveTransparency);
	if (special.hasFlag(Set3dFloorSpecial::Flags::ResetLighting))
		ef.setFlag(ExtraFloor::Flags::ResetLighting);
	if (special.render_inside)
		ef.setFlag(ExtraFloor::Flags::DrawInside);
	if (special.type == Set3dFloorSpecial::Type::Solid || special.type == Set3dFloorSpecial::Type::Vavoom)
		ef.setFlag(ExtraFloor::Flags::Solid);

	addExtraFloor(sef, ef);
}
