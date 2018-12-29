
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapSide.cpp
// Description: MapSide class, represents a line side object in a map
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
#include "MapSide.h"
#include "Game/Configuration.h"
#include "MapSector.h"
#include "SLADEMap.h"


// -----------------------------------------------------------------------------
//
// MapSide Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapSide class constructor
// -----------------------------------------------------------------------------
MapSide::MapSide(MapSector* sector, SLADEMap* parent) : MapObject(Type::Side, parent), sector_{ sector }
{
	// Add to parent sector
	if (sector)
		sector->connectSide(this);
}

// -----------------------------------------------------------------------------
// Copies another MapSide object [c]
// -----------------------------------------------------------------------------
void MapSide::copy(MapObject* c)
{
	if (c->objType() != Type::Side)
		return;

	// Update texture counts (decrement previous)
	if (parent_map_)
	{
		parent_map_->updateTexUsage(tex_lower_, -1);
		parent_map_->updateTexUsage(tex_middle_, -1);
		parent_map_->updateTexUsage(tex_upper_, -1);
	}

	// Copy properties
	auto side   = dynamic_cast<MapSide*>(c);
	tex_lower_  = side->tex_lower_;
	tex_middle_ = side->tex_middle_;
	tex_upper_  = side->tex_upper_;
	offset_x_   = side->offset_x_;
	offset_y_   = side->offset_y_;

	// Update texture counts (increment new)
	if (parent_map_)
	{
		parent_map_->updateTexUsage(tex_lower_, 1);
		parent_map_->updateTexUsage(tex_middle_, 1);
		parent_map_->updateTexUsage(tex_upper_, 1);
	}

	MapObject::copy(c);
}

// -----------------------------------------------------------------------------
// Returns the light level of the given side
// -----------------------------------------------------------------------------
uint8_t MapSide::light()
{
	int  light          = 0;
	bool include_sector = true;

	if (parent_map_->currentFormat() == MapFormat::UDMF
		&& Game::configuration().featureSupported(Game::UDMFFeature::SideLighting))
	{
		light += intProperty("light");
		if (boolProperty("lightabsolute"))
			include_sector = false;
	}

	if (include_sector && sector_)
		light += sector_->lightAt(0);

	// Clamp range
	if (light > 255)
		return 255;
	if (light < 0)
		return 0;
	return light;
}

// -----------------------------------------------------------------------------
// Change the light level of a side, if supported
// -----------------------------------------------------------------------------
void MapSide::changeLight(int amount)
{
	if (parent_map_->currentFormat() == MapFormat::UDMF
		&& Game::configuration().featureSupported(Game::UDMFFeature::SideLighting))
		setIntProperty("light", intProperty("light") + amount);
}

// -----------------------------------------------------------------------------
// Sets the side's sector to [sector]
// -----------------------------------------------------------------------------
void MapSide::setSector(MapSector* sector)
{
	if (!sector)
		return;

	// Remove side from current sector, if any
	if (sector_)
		sector_->disconnectSide(this);

	// Update modified time
	setModified();

	// Add side to new sector
	sector_ = sector;
	sector->connectSide(this);
}

// -----------------------------------------------------------------------------
// Returns the value of the integer property matching [key]
// -----------------------------------------------------------------------------
int MapSide::intProperty(const string& key)
{
	if (key == "sector")
	{
		if (sector_)
			return sector_->index();
		else
			return -1;
	}
	else if (key == "offsetx")
		return offset_x_;
	else if (key == "offsety")
		return offset_y_;
	else
		return MapObject::intProperty(key);
}

// -----------------------------------------------------------------------------
// Sets the integer value of the property [key] to [value]
// -----------------------------------------------------------------------------
void MapSide::setIntProperty(const string& key, int value)
{
	// Update modified time
	setModified();

	if (key == "sector" && parent_map_)
		setSector(parent_map_->sector(value));
	else if (key == "offsetx")
		offset_x_ = value;
	else if (key == "offsety")
		offset_y_ = value;
	else
		MapObject::setIntProperty(key, value);
}

// -----------------------------------------------------------------------------
// Returns the value of the string property matching [key]
// -----------------------------------------------------------------------------
string MapSide::stringProperty(const string& key)
{
	if (key == "texturetop")
		return tex_upper_;
	else if (key == "texturemiddle")
		return tex_middle_;
	else if (key == "texturebottom")
		return tex_lower_;
	else
		return MapObject::stringProperty(key);
}

// -----------------------------------------------------------------------------
// Sets the string value of the property [key] to [value]
// -----------------------------------------------------------------------------
void MapSide::setStringProperty(const string& key, const string& value)
{
	// Update modified time
	setModified();

	if (key == "texturetop")
	{
		if (parent_map_)
			parent_map_->updateTexUsage(tex_upper_, -1);
		tex_upper_ = value;
		if (parent_map_)
			parent_map_->updateTexUsage(tex_upper_, 1);
	}
	else if (key == "texturemiddle")
	{
		if (parent_map_)
			parent_map_->updateTexUsage(tex_middle_, -1);
		tex_middle_ = value;
		if (parent_map_)
			parent_map_->updateTexUsage(tex_middle_, 1);
	}
	else if (key == "texturebottom")
	{
		if (parent_map_)
			parent_map_->updateTexUsage(tex_lower_, -1);
		tex_lower_ = value;
		if (parent_map_)
			parent_map_->updateTexUsage(tex_lower_, 1);
	}
	else
		MapObject::setStringProperty(key, value);
}

// -----------------------------------------------------------------------------
// Returns true if the property [key] can be modified via script
// -----------------------------------------------------------------------------
bool MapSide::scriptCanModifyProp(const string& key)
{
	return key != "sector";
}

// -----------------------------------------------------------------------------
// Write all side info to a Backup struct
// -----------------------------------------------------------------------------
void MapSide::writeBackup(Backup* backup)
{
	// Sector
	if (sector_)
		backup->props_internal["sector"] = sector_->objId();
	else
		backup->props_internal["sector"] = 0;

	// Textures
	backup->props_internal["texturetop"]    = tex_upper_;
	backup->props_internal["texturemiddle"] = tex_middle_;
	backup->props_internal["texturebottom"] = tex_lower_;

	// Offsets
	backup->props_internal["offsetx"] = offset_x_;
	backup->props_internal["offsety"] = offset_y_;
}

// -----------------------------------------------------------------------------
// Reads all side info from a Backup struct
// -----------------------------------------------------------------------------
void MapSide::readBackup(Backup* backup)
{
	// Sector
	auto s = parent_map_->getObjectById(backup->props_internal["sector"]);
	if (s)
	{
		sector_->disconnectSide(this);
		sector_ = dynamic_cast<MapSector*>(s);
		sector_->connectSide(this);
	}
	else
	{
		if (sector_)
			sector_->disconnectSide(this);
		sector_ = nullptr;
	}

	// Update texture counts (decrement previous)
	parent_map_->updateTexUsage(tex_upper_, -1);
	parent_map_->updateTexUsage(tex_middle_, -1);
	parent_map_->updateTexUsage(tex_lower_, -1);

	// Textures
	tex_upper_  = backup->props_internal["texturetop"].stringValue();
	tex_middle_ = backup->props_internal["texturemiddle"].stringValue();
	tex_lower_  = backup->props_internal["texturebottom"].stringValue();

	// Update texture counts (increment new)
	parent_map_->updateTexUsage(tex_upper_, 1);
	parent_map_->updateTexUsage(tex_middle_, 1);
	parent_map_->updateTexUsage(tex_lower_, 1);

	// Offsets
	offset_x_ = backup->props_internal["offsetx"].intValue();
	offset_y_ = backup->props_internal["offsety"].intValue();
}
