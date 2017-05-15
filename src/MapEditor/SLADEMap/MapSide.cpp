
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
MapSide::MapSide(MapSector* sector, SLADEMap* parent) : MapObject(MOBJ_SIDE, parent)
{
	// Init variables
	this->sector = sector;
	this->parent = NULL;
	this->offset_x = 0;
	this->offset_y = 0;

	// Add to parent sector
	if (sector) sector->connectSide(this);
}

/* MapSide::MapSide
 * MapSide class constructor
 *******************************************************************/
MapSide::MapSide(SLADEMap* parent) : MapObject(MOBJ_SIDE, parent)
{
	// Init variables
	this->sector = NULL;
	this->parent = NULL;
	this->offset_x = 0;
	this->offset_y = 0;
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
	if (c->getObjType() != MOBJ_SIDE)
		return;

	// Update texture counts (decrement previous)
	if (parent_map)
	{
		parent_map->updateTexUsage(tex_lower, -1);
		parent_map->updateTexUsage(tex_middle, -1);
		parent_map->updateTexUsage(tex_upper, -1);
	}

	// Copy properties
	MapSide* side = (MapSide*)c;
	this->tex_lower = side->tex_lower;
	this->tex_middle = side->tex_middle;
	this->tex_upper = side->tex_upper;
	this->offset_x = side->offset_x;
	this->offset_y = side->offset_y;

	// Update texture counts (increment new)
	if (parent_map)
	{
		parent_map->updateTexUsage(tex_lower, 1);
		parent_map->updateTexUsage(tex_middle, 1);
		parent_map->updateTexUsage(tex_upper, 1);
	}

	MapObject::copy(c);
}

/* MapSide::getLight
 * Returns the light level of the given side
 *******************************************************************/
uint8_t MapSide::getLight()
{
	int light = 0;
	bool include_sector = true;

	if (parent_map->currentFormat() == MAP_UDMF &&
		Game::configuration().featureSupported(UDMFFeature::SideLighting))
	{
		light += intProperty("light");
		if (boolProperty("lightabsolute"))
			include_sector = false;
	}

	if (include_sector && sector)
		light += sector->getLight(0);

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
	if (parent_map->currentFormat() == MAP_UDMF &&
		Game::configuration().featureSupported(UDMFFeature::SideLighting))
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
	if (this->sector)
		this->sector->disconnectSide(this);

	// Update modified time
	setModified();

	// Add side to new sector
	this->sector = sector;
	sector->connectSide(this);
}

/* MapSide::intProperty
 * Returns the value of the integer property matching [key]
 *******************************************************************/
int MapSide::intProperty(string key)
{
	if (key == "sector")
	{
		if (sector)
			return sector->getIndex();
		else
			return -1;
	}
	else if (key == "offsetx")
		return offset_x;
	else if (key == "offsety")
		return offset_y;
	else
		return MapObject::intProperty(key);
}

/* MapSide::setIntProperty
 * Sets the integer value of the property [key] to [value]
 *******************************************************************/
void MapSide::setIntProperty(string key, int value)
{
	// Update modified time
	setModified();

	if (key == "sector" && parent_map)
		setSector(parent_map->getSector(value));
	else if (key == "offsetx")
		offset_x = value;
	else if (key == "offsety")
		offset_y = value;
	else
		MapObject::setIntProperty(key, value);
}

/* MapSide::stringProperty
 * Returns the value of the string property matching [key]
 *******************************************************************/
string MapSide::stringProperty(string key)
{
	if (key == "texturetop")
		return tex_upper;
	else if (key == "texturemiddle")
		return tex_middle;
	else if (key == "texturebottom")
		return tex_lower;
	else
		return MapObject::stringProperty(key);
}

/* MapSide::setStringProperty
 * Sets the string value of the property [key] to [value]
 *******************************************************************/
void MapSide::setStringProperty(string key, string value)
{
	// Update modified time
	setModified();

	if (key == "texturetop")
	{
		if (parent_map) parent_map->updateTexUsage(tex_upper, -1);
		tex_upper = value;
		if (parent_map) parent_map->updateTexUsage(tex_upper, 1);
	}
	else if (key == "texturemiddle")
	{
		if (parent_map) parent_map->updateTexUsage(tex_middle, -1);
		tex_middle = value;
		if (parent_map) parent_map->updateTexUsage(tex_middle, 1);
	}
	else if (key == "texturebottom")
	{
		if (parent_map) parent_map->updateTexUsage(tex_lower, -1);
		tex_lower = value;
		if (parent_map) parent_map->updateTexUsage(tex_lower, 1);
	}
	else
		MapObject::setStringProperty(key, value);
}

/* MapSide::writeBackup
 * Write all side info to a mobj_backup_t struct
 *******************************************************************/
void MapSide::writeBackup(mobj_backup_t* backup)
{
	// Sector
	if (sector)
		backup->props_internal["sector"] = sector->getId();
	else
		backup->props_internal["sector"] = 0;

	// Textures
	backup->props_internal["texturetop"] = tex_upper;
	backup->props_internal["texturemiddle"] = tex_middle;
	backup->props_internal["texturebottom"] = tex_lower;

	// Offsets
	backup->props_internal["offsetx"] = offset_x;
	backup->props_internal["offsety"] = offset_y;

	//LOG_MESSAGE(1, "Side %d backup sector #%d", id, sector->getIndex());
}

/* MapSide::readBackup
 * Reads all side info from a mobj_backup_t struct
 *******************************************************************/
void MapSide::readBackup(mobj_backup_t* backup)
{
	// Sector
	MapObject* s = parent_map->getObjectById(backup->props_internal["sector"]);
	if (s)
	{
		sector->disconnectSide(this);
		sector = (MapSector*)s;
		sector->connectSide(this);
		//LOG_MESSAGE(1, "Side %d load backup sector #%d", id, s->getIndex());
	}
	else
	{
		if (sector)
			sector->disconnectSide(this);
		sector = NULL;
	}

	// Update texture counts (decrement previous)
	parent_map->updateTexUsage(tex_upper, -1);
	parent_map->updateTexUsage(tex_middle, -1);
	parent_map->updateTexUsage(tex_lower, -1);

	// Textures
	tex_upper = backup->props_internal["texturetop"].getStringValue();
	tex_middle = backup->props_internal["texturemiddle"].getStringValue();
	tex_lower = backup->props_internal["texturebottom"].getStringValue();

	// Update texture counts (increment new)
	parent_map->updateTexUsage(tex_upper, 1);
	parent_map->updateTexUsage(tex_middle, 1);
	parent_map->updateTexUsage(tex_lower, 1);

	// Offsets
	offset_x = backup->props_internal["offsetx"].getIntValue();
	offset_y = backup->props_internal["offsety"].getIntValue();
}
