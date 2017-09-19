
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MapSide.cpp
 * Description: MapSide class, represents a line side object in a map
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "Game/Configuration.h"
#include "MapSector.h"
#include "MapSide.h"
#include "SLADEMap.h"


/*******************************************************************
 * MAPSIDE CLASS FUNCTIONS
 *******************************************************************/

/* MapSide::MapSide
 * MapSide class constructor
 *******************************************************************/
MapSide::MapSide(MapSector* sector, SLADEMap* parent) :
	MapObject(Type::Side, parent),
	sector_{ sector }
{
	// Add to parent sector
	if (sector) sector->connectSide(this);
}

/* MapSide::MapSide
 * MapSide class constructor
 *******************************************************************/
MapSide::MapSide(SLADEMap* parent) : MapObject(Type::Side, parent)
{
	// Init variables
	this->sector_ = nullptr;
	this->parent_line_ = nullptr;
}

/* MapSide::~MapSide
 * MapSide class destructor
 *******************************************************************/
MapSide::~MapSide()
{
}

/* MapSide::copy
 * Copies another MapSide object [c]
 *******************************************************************/
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
	MapSide* side = (MapSide*)c;
	this->tex_lower_ = side->tex_lower_;
	this->tex_middle_ = side->tex_middle_;
	this->tex_upper_ = side->tex_upper_;
	this->offset_ = side->offset_;

	// Update texture counts (increment new)
	if (parent_map_)
	{
		parent_map_->updateTexUsage(tex_lower_, 1);
		parent_map_->updateTexUsage(tex_middle_, 1);
		parent_map_->updateTexUsage(tex_upper_, 1);
	}

	MapObject::copy(c);
}

/* MapSide::getLight
 * Returns the light level of the given side
 *******************************************************************/
uint8_t MapSide::light()
{
	int light = 0;
	bool include_sector = true;

	if (parent_map_->currentFormat() == MAP_UDMF &&
		Game::configuration().featureSupported(Game::UDMFFeature::SideLighting))
	{
		light += intProperty("light");
		if (boolProperty("lightabsolute"))
			include_sector = false;
	}

	if (include_sector && sector_)
		light += sector_->getLight(0);

	// Clamp range
	if (light > 255)
		return 255;
	if (light < 0)
		return 0;
	return light;
}

/* MapSide::changeLight
 * Change the light level of a side, if supported
 *******************************************************************/
void MapSide::changeLight(int amount)
{
	if (parent_map_->currentFormat() == MAP_UDMF &&
		Game::configuration().featureSupported(Game::UDMFFeature::SideLighting))
		setIntProperty("light", intProperty("light") + amount);
}

/* MapSide::setSector
 * Sets the side's sector to [sector]
 *******************************************************************/
void MapSide::setSector(MapSector* sector)
{
	if (!sector)
		return;

	// Remove side from current sector, if any
	if (this->sector_)
		this->sector_->disconnectSide(this);

	// Update modified time
	setModified();

	// Add side to new sector
	this->sector_ = sector;
	sector->connectSide(this);
}

/* MapSide::intProperty
 * Returns the value of the integer property matching [key]
 *******************************************************************/
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
		return offset_.x;
	else if (key == "offsety")
		return offset_.y;
	else
		return MapObject::intProperty(key);
}

/* MapSide::setIntProperty
 * Sets the integer value of the property [key] to [value]
 *******************************************************************/
void MapSide::setIntProperty(const string& key, int value)
{
	// Update modified time
	setModified();

	if (key == "sector" && parent_map_)
		setSector(parent_map_->getSector(value));
	else if (key == "offsetx")
		offset_.x = value;
	else if (key == "offsety")
		offset_.y = value;
	else
		MapObject::setIntProperty(key, value);
}

/* MapSide::stringProperty
 * Returns the value of the string property matching [key]
 *******************************************************************/
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

/* MapSide::setStringProperty
 * Sets the string value of the property [key] to [value]
 *******************************************************************/
void MapSide::setStringProperty(const string& key, const string& value)
{
	// Update modified time
	setModified();

	if (key == "texturetop")
	{
		if (parent_map_) parent_map_->updateTexUsage(tex_upper_, -1);
		tex_upper_ = value;
		if (parent_map_) parent_map_->updateTexUsage(tex_upper_, 1);
	}
	else if (key == "texturemiddle")
	{
		if (parent_map_) parent_map_->updateTexUsage(tex_middle_, -1);
		tex_middle_ = value;
		if (parent_map_) parent_map_->updateTexUsage(tex_middle_, 1);
	}
	else if (key == "texturebottom")
	{
		if (parent_map_) parent_map_->updateTexUsage(tex_lower_, -1);
		tex_lower_ = value;
		if (parent_map_) parent_map_->updateTexUsage(tex_lower_, 1);
	}
	else
		MapObject::setStringProperty(key, value);
}

/* MapSide::scriptCanModifyProp
 * Returns true if the property [key] can be modified via script
 *******************************************************************/
bool MapSide::scriptCanModifyProp(const string& key)
{
	if (key == "sector")
		return false;

	return true;
}

/* MapSide::writeBackup
 * Write all side info to a mobj_backup_t struct
 *******************************************************************/
void MapSide::writeBackup(Backup* backup)
{
	// Sector
	if (sector_)
		backup->props_internal["sector"] = sector_->objId();
	else
		backup->props_internal["sector"] = 0;

	// Textures
	backup->props_internal["texturetop"] = tex_upper_;
	backup->props_internal["texturemiddle"] = tex_middle_;
	backup->props_internal["texturebottom"] = tex_lower_;

	// Offsets
	backup->props_internal["offsetx"] = offset_.x;
	backup->props_internal["offsety"] = offset_.y;

	//LOG_MESSAGE(1, "Side %d backup sector #%d", id, sector->getIndex());
}

/* MapSide::readBackup
 * Reads all side info from a mobj_backup_t struct
 *******************************************************************/
void MapSide::readBackup(Backup* backup)
{
	// Sector
	MapObject* s = parent_map_->getObjectById(backup->props_internal["sector"]);
	if (s)
	{
		sector_->disconnectSide(this);
		sector_ = (MapSector*)s;
		sector_->connectSide(this);
		//LOG_MESSAGE(1, "Side %d load backup sector #%d", id, s->getIndex());
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
	tex_upper_ = backup->props_internal["texturetop"].getStringValue();
	tex_middle_ = backup->props_internal["texturemiddle"].getStringValue();
	tex_lower_ = backup->props_internal["texturebottom"].getStringValue();

	// Update texture counts (increment new)
	parent_map_->updateTexUsage(tex_upper_, 1);
	parent_map_->updateTexUsage(tex_middle_, 1);
	parent_map_->updateTexUsage(tex_lower_, 1);

	// Offsets
	offset_.x = backup->props_internal["offsetx"].getIntValue();
	offset_.y = backup->props_internal["offsety"].getIntValue();
}
