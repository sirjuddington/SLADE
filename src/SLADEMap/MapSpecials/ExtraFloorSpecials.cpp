
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
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObjectList/SectorList.h"
#include "SLADEMap/SLADEMap.h"

using namespace slade;



ExtraFloorSpecials::ExtraFloorSpecials(SLADEMap& map) : map_(&map) {}

bool ExtraFloorSpecials::hasExtraFloors(const MapSector* sector) const
{
	return std::ranges::any_of(sector_extra_floors_, [sector](const auto& sef) { return sef.sector == sector; });
}

const vector<ExtraFloor>& ExtraFloorSpecials::extraFloors(const MapSector* sector) const
{
	const auto it = std::ranges::find_if(
		sector_extra_floors_, [sector](const auto& sef) { return sef.sector == sector; });

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
}

void ExtraFloorSpecials::updateSectorExtraFloors(const MapSector* sector)
{
	clearExtraFloors(sector);

	SectorExtraFloors sef{ .sector = sector };

	for (const auto& special : set_3d_floor_specials_)
		if (special.target == sector)
			applySet3dFloorSpecial(special, sef);

	if (!sef.extra_floors.empty())
		sector_extra_floors_.push_back(sef);
}

ExtraFloorSpecials::SectorExtraFloors* ExtraFloorSpecials::getSectorExtraFloors(const MapSector* sector)
{
	// Find existing extra floors for this sector
	if (auto it = std::ranges::find_if(
			sector_extra_floors_, [sector](const auto& sef) { return sef.sector == sector; });
		it == sector_extra_floors_.end())
	{
		// No existing extra floors, create new entry
		sector_extra_floors_.emplace_back(sector);
		return &sector_extra_floors_.back();
	}
	else
		return &(*it);
}

void ExtraFloorSpecials::addExtraFloor(const MapSector* sector, const ExtraFloor& extra_floor)
{
	auto sef = getSectorExtraFloors(sector);
	sef->extra_floors.push_back(extra_floor);

	// Sort extra floors by height (top-down)
	std::ranges::sort(sef->extra_floors, [](const ExtraFloor& a, const ExtraFloor& b) { return a.height > b.height; });
}

void ExtraFloorSpecials::clearExtraFloors(const MapSector* sector)
{
	if (auto it = std::ranges::find_if(
			sector_extra_floors_, [sector](const auto& sef) { return sef.sector == sector; });
		it != sector_extra_floors_.end())
		it->extra_floors.clear();
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
	}
}

void ExtraFloorSpecials::applySet3dFloorSpecial(
	const Set3dFloorSpecial& special,
	SectorExtraFloors&       sector_extra_floors)
{
	ExtraFloor ef;
	ef.control_line   = special.line;
	ef.control_sector = special.control_sector;
	// ef.render_inside  = special.render_inside;
	// ef.flags          = special.flags;
	ef.alpha = special.alpha;

	if (special.type == Set3dFloorSpecial::Type::Vavoom)
	{
		ef.plane_top    = special.control_sector->floor().plane;
		ef.plane_bottom = special.control_sector->ceiling().plane;
		ef.height       = special.control_sector->floor().height;
		ef.setFlag(ExtraFloor::Flags::Flipped);
	}
	else
	{
		ef.plane_top    = special.control_sector->ceiling().plane;
		ef.plane_bottom = special.control_sector->floor().plane;
		ef.height       = special.control_sector->ceiling().height;
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
	if (special.render_inside)
		ef.setFlag(ExtraFloor::Flags::DrawInside);
	// if (special.type == Set3dFloorSpecial::Type::Solid || special.type == Set3dFloorSpecial::Type::Vavoom)
	//	ef.setFlag(ExtraFloor::Flags::Solid);

	sector_extra_floors.extra_floors.push_back(ef);
}
