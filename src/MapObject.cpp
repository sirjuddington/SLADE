
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MapObject.cpp
 * Description: MapObject class, base class for all map object classes
 *              (MapLine, MapSector etc)
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
#include "MapObject.h"
#include "SLADEMap.h"
#include "GameConfiguration.h"
#include "MainApp.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
long prop_backup_time = -1;


/*******************************************************************
 * MAPOBJECT CLASS FUNCTIONS
 *******************************************************************/

/* MapObject::MapObject
 * MapObject class constructor
 *******************************************************************/
MapObject::MapObject(int type, SLADEMap* parent)
{
	// Init variables
	this->type = type;
	this->parent_map = parent;
	this->index = 0;
	this->filtered = false;
	this->modified_time = theApp->runTimer();
	this->id = 0;
	this->obj_backup = NULL;

	if (parent)
		parent->addMapObject(this);
}

/* MapObject::~MapObject
 * MapObject class destructor
 *******************************************************************/
MapObject::~MapObject()
{
	properties.clear();
	if (obj_backup)
		delete obj_backup;
}

/* MapObject::getIndex
 * Returns the map index of the object
 *******************************************************************/
unsigned MapObject::getIndex()
{
	return index;
}

/* MapObject::getTypeName
 * Returns a string representation of the object type
 *******************************************************************/
string MapObject::getTypeName()
{
	switch (type)
	{
	case MOBJ_VERTEX:
		return "Vertex";
	case MOBJ_SIDE:
		return "Side";
	case MOBJ_LINE:
		return "Line";
	case MOBJ_SECTOR:
		return "Sector";
	case MOBJ_THING:
		return "Thing";
	default:
		return "Unknown";
	}
}

/* MapObject::setModified
 * Sets the object as modified
 *******************************************************************/
void MapObject::setModified()
{
	// Backup current properties if required
	if (modified_time < prop_backup_time)
	{
		if (obj_backup) delete obj_backup;
		obj_backup = new mobj_backup_t();
		backup(obj_backup);
	}

	modified_time = theApp->runTimer();
}

/* MapObject::copy
 * Copy properties from another MapObject [c]
 *******************************************************************/
void MapObject::copy(MapObject* c)
{
	// Can't copy an object of a different type
	if (c->type != type)
		return;

	// Reset properties
	properties.clear();

	// Copy object properties
	if (!c->properties.isEmpty())
	{
		c->properties.copyTo(properties);
		this->parent_map = c->parent_map;
		this->filtered = c->filtered;
	}

	// Update modified time
	setModified();
}

/* MapObject::boolProperty
 * Returns the value of the boolean property matching [key]
 *******************************************************************/
bool MapObject::boolProperty(string key)
{
	// If the property exists already, return it
	if (properties[key].hasValue())
		return properties[key].getBoolValue();

	// Otherwise check the game configuration for a default value
	else
	{
		UDMFProperty* prop = theGameConfiguration->getUDMFProperty(key, type);
		if (prop)
			return prop->getDefaultValue().getBoolValue();
		else
			return false;
	}
}

/* MapObject::intProperty
 * Returns the value of the integer property matching [key]
 *******************************************************************/
int MapObject::intProperty(string key)
{
	// If the property exists already, return it
	if (properties[key].hasValue())
		return properties[key].getIntValue();

	// Otherwise check the game configuration for a default value
	else
	{
		UDMFProperty* prop = theGameConfiguration->getUDMFProperty(key, type);
		if (prop)
			return prop->getDefaultValue().getIntValue();
		else
			return 0;
	}
}

/* MapObject::floatProperty
 * Returns the value of the float property matching [key]
 *******************************************************************/
double MapObject::floatProperty(string key)
{
	// If the property exists already, return it
	if (properties[key].hasValue())
		return properties[key].getFloatValue();

	// Otherwise check the game configuration for a default value
	else
	{
		UDMFProperty* prop = theGameConfiguration->getUDMFProperty(key, type);
		if (prop)
			return prop->getDefaultValue().getFloatValue();
		else
			return 0;
	}
}

/* MapObject::stringProperty
 * Returns the value of the string property matching [key]
 *******************************************************************/
string MapObject::stringProperty(string key)
{
	// If the property exists already, return it
	if (properties[key].hasValue())
		return properties[key].getStringValue();

	// Otherwise check the game configuration for a default value
	else
	{
		UDMFProperty* prop = theGameConfiguration->getUDMFProperty(key, type);
		if (prop)
			return prop->getDefaultValue().getStringValue();
		else
			return "";
	}
}

/* MapObject::setBoolProperty
 * Sets the boolean value of the property [key] to [value]
 *******************************************************************/
void MapObject::setBoolProperty(string key, bool value)
{
	// Update modified time
	setModified();

	// Set property
	properties[key] = value;
}

/* MapObject::setIntProperty
 * Sets the integer value of the property [key] to [value]
 *******************************************************************/
void MapObject::setIntProperty(string key, int value)
{
	// Update modified time
	setModified();

	// Set property
	properties[key] = value;
}

/* MapObject::setFloatProperty
 * Sets the float value of the property [key] to [value]
 *******************************************************************/
void MapObject::setFloatProperty(string key, double value)
{
	// Update modified time
	setModified();

	// Set property
	properties[key] = value;
}

/* MapObject::setStringProperty
 * Sets the string value of the property [key] to [value]
 *******************************************************************/
void MapObject::setStringProperty(string key, string value)
{
	// Update modified time
	setModified();

	// Set property
	properties[key] = value;
}

/* MapObject::backup
 * Writes all object properties to [backup]
 *******************************************************************/
void MapObject::backup(mobj_backup_t* backup)
{
	// Save basic info
	backup->id = id;
	backup->type = type;

	// Save general properties to list
	properties.copyTo(backup->properties);

	// Object-specific properties
	writeBackup(backup);
}

/* MapObject::loadFromBackup
 * Restores all object properties from [backup]
 *******************************************************************/
void MapObject::loadFromBackup(mobj_backup_t* backup)
{
	// Check type match
	if (backup->type != type)
	{
		wxLogMessage("loadFromBackup: Mobj type mismatch, %d != %d", type, backup->type);
		return;
	}
	// Check id match
	if (backup->id != id)
	{
		wxLogMessage("loadFromBackup: Mobj id mismatch, %d != %d", id, backup->id);
		return;
	}

	// Load general properties from list
	properties.clear();
	backup->properties.copyTo(properties);

	// Object-specific properties
	readBackup(backup);

	// Update modified time
	setModified();
}

/* MapObject::getBackup
 * Returns the backup struct for this object
 *******************************************************************/
mobj_backup_t* MapObject::getBackup(bool remove)
{
	mobj_backup_t* bak = obj_backup;
	if (remove) obj_backup = NULL;
	return bak;
}


/*******************************************************************
 * MAPOBJECT CLASS STATIC FUNCTIONS
 *******************************************************************/

/* MapObject::propBackupTime
 * Returns the property backup time (used for the undo system - if
 * an object's properties are modified, they will be backed up first
 * if they haven't since this time)
 *******************************************************************/
long MapObject::propBackupTime()
{
	return prop_backup_time;
}

/* MapObject::beginPropBackup
 * Begins property backup, any time a MapObject property is changed
 * it's properties will be backed up before changing (only once)
 *******************************************************************/
void MapObject::beginPropBackup(long current_time)
{
	prop_backup_time = current_time;
}

/* MapObject::endPropBackup
 * End property backup
 *******************************************************************/
void MapObject::endPropBackup()
{
	prop_backup_time = -1;
}
