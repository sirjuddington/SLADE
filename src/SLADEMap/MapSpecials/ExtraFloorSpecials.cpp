
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
#include "SLADEMap/MapObject/MapLine.h"
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
	// Sector_Set3dFloor
	// TODO: game/port check
	if (line.special() == 160)
		addSet3dFloorSpecial(line);
}

void ExtraFloorSpecials::addExtraFloor(const MapSector* sector, const ExtraFloor& extra_floor)
{
	if (auto it = std::ranges::find_if(
			sector_extra_floors_, [sector](const auto& sef) { return sef.sector == sector; });
		it == sector_extra_floors_.end())
	{
		SectorExtraFloors sef;
		sef.sector       = sector;
		sef.extra_floors = { extra_floor };
		sector_extra_floors_.push_back(sef);
	}
	else
	{
		it->extra_floors.push_back(extra_floor);

		// Sort extra floors by height (top-down)
		std::ranges::sort(
			it->extra_floors, [](const ExtraFloor& a, const ExtraFloor& b) { return a.height > b.height; });
	}
}

void ExtraFloorSpecials::clearExtraFloors(const MapSector* sector)
{
	if (auto it = std::ranges::find_if(
			sector_extra_floors_, [sector](const auto& sef) { return sef.sector == sector; });
		it != sector_extra_floors_.end())
		sector_extra_floors_.erase(it);
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

	// Get all tagged sectors
	vector<MapSector*> target_sectors;
	map_->sectors().putAllWithId(line.arg(0), target_sectors);

	for (auto sector : target_sectors)
	{
		Set3dFloorSpecial s3fs;
		s3fs.target         = sector;
		s3fs.control_sector = control_sector;
		s3fs.line           = &line;
	}
}
