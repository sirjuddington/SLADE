
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SectorList.cpp
// Description: A (non-owning) list of map sectors. Includes std::vector-like
//              API for accessing items and some misc functions to get info
//              about the contained items.
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
#include "SectorList.h"
#include "General/UI.h"


// -----------------------------------------------------------------------------
//
// SectorList Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Clears the list (and texture usage)
// -----------------------------------------------------------------------------
void SectorList::clear()
{
	usage_tex_.clear();
	MapObjectList::clear();
}

// -----------------------------------------------------------------------------
// Adds [sector] to the list and updates texture usage
// -----------------------------------------------------------------------------
void SectorList::add(MapSector* sector)
{
	// Update texture counts
	usage_tex_[sector->floor().texture.Upper()] += 1;
	usage_tex_[sector->ceiling().texture.Upper()] += 1;

	MapObjectList::add(sector);
}

// -----------------------------------------------------------------------------
// Removes [sector] from the list and updates texture usage
// -----------------------------------------------------------------------------
void SectorList::remove(unsigned index)
{
	if (index >= objects_.size())
		return;

	// Update texture counts
	usage_tex_[objects_[index]->floor().texture.Upper()] -= 1;
	usage_tex_[objects_[index]->ceiling().texture.Upper()] -= 1;

	MapObjectList::remove(index);
}

// -----------------------------------------------------------------------------
// Returns the sector at the given [point], or null if not within a sector
// -----------------------------------------------------------------------------
MapSector* SectorList::atPos(Vec2d point) const
{
	// Go through sectors
	for (const auto& sector : objects_)
	{
		// Check if point is within sector
		if (sector->containsPoint(point))
			return sector;
	}

	// Not within a sector
	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns a bounding box containing all sectors in the list
// -----------------------------------------------------------------------------
BBox SectorList::allSectorBounds() const
{
	BBox bbox;

	if (count_ == 0)
		return bbox;

	// Go through sectors
	// This is quicker than generating it from vertices,
	// but relies on sector bboxes being up-to-date (which they should be)
	bbox = objects_[0]->boundingBox();
	for (unsigned i = 1; i < count_; ++i)
	{
		auto sbb = objects_[i]->boundingBox();
		if (sbb.min.x < bbox.min.x)
			bbox.min.x = sbb.min.x;
		if (sbb.min.y < bbox.min.y)
			bbox.min.y = sbb.min.y;
		if (sbb.max.x > bbox.max.x)
			bbox.max.x = sbb.max.x;
		if (sbb.max.y > bbox.max.y)
			bbox.max.y = sbb.max.y;
	}

	return bbox;
}

// -----------------------------------------------------------------------------
// Forces building of polygons for all sectors in the list
// -----------------------------------------------------------------------------
void SectorList::initPolygons()
{
	UI::setSplashProgressMessage("Building sector polygons");
	UI::setSplashProgress(0.0f);
	for (unsigned i = 0; i < count_; ++i)
	{
		UI::setSplashProgress((float)i / (float)count_);
		objects_[i]->polygon();
	}
	UI::setSplashProgress(1.0f);
}

// -----------------------------------------------------------------------------
// Forces update of bounding boxes for all sectors in the list
// -----------------------------------------------------------------------------
void SectorList::initBBoxes()
{
	for (auto& sector : objects_)
		sector->updateBBox();
}

// -----------------------------------------------------------------------------
// Adds all sectors with tag [id] to [list]
// -----------------------------------------------------------------------------
void SectorList::putAllWithId(int id, vector<MapSector*>& list) const
{
	for (auto& sector : objects_)
		if (sector->tag() == id)
			list.push_back(sector);
}

// -----------------------------------------------------------------------------
// Returns a list of all sectors with tag [id]
// -----------------------------------------------------------------------------
vector<MapSector*> SectorList::allWithId(int id) const
{
	vector<MapSector*> list;
	putAllWithId(id, list);
	return list;
}

// -----------------------------------------------------------------------------
// Returns the first sector found with tag [id], or null if none found
// -----------------------------------------------------------------------------
MapSector* SectorList::firstWithId(int id) const
{
	for (auto& sector : objects_)
		if (sector->tag() == id)
			return sector;

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the lowest unused sector tag
// -----------------------------------------------------------------------------
int SectorList::firstFreeId() const
{
	int id = 1;
	for (unsigned i = 0; i < count_; ++i)
	{
		if (objects_[i]->tag() == id)
		{
			id++;
			i = 0;
		}
	}

	return id;
}
