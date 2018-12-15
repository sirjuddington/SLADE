
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapObject.cpp
// Description: MapObject class, base class for all map object classes
//              (MapLine, MapSector etc)
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
#include "MapObject.h"
#include "App.h"
#include "Game/Configuration.h"
#include "SLADEMap.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
long prop_backup_time = -1;
} // namespace


// -----------------------------------------------------------------------------
//
// MapObject Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapObject class constructor
// -----------------------------------------------------------------------------
MapObject::MapObject(Type type, SLADEMap* parent)
{
	// Init variables
	this->type_          = type;
	this->parent_map_    = parent;
	this->index_         = 0;
	this->filtered_      = false;
	this->modified_time_ = App::runTimer();
	this->obj_id_        = 0;
	this->obj_backup_    = nullptr;

	if (parent)
		parent->addMapObject(this);
}

// -----------------------------------------------------------------------------
// MapObject class destructor
// -----------------------------------------------------------------------------
MapObject::~MapObject()
{
	properties_.clear();
	if (obj_backup_)
		delete obj_backup_;
}

// -----------------------------------------------------------------------------
// Returns the map index of the object
// -----------------------------------------------------------------------------
unsigned MapObject::index()
{
	return index_;
}

// -----------------------------------------------------------------------------
// Returns a string representation of the object type
// -----------------------------------------------------------------------------
string MapObject::typeName()
{
	switch (type_)
	{
	case Type::Vertex: return "Vertex";
	case Type::Side: return "Side";
	case Type::Line: return "Line";
	case Type::Sector: return "Sector";
	case Type::Thing: return "Thing";
	default: return "Unknown";
	}
}

// -----------------------------------------------------------------------------
// Sets the object as modified.
// Despite the name, THIS MUST BE CALLED **BEFORE** MODIFYING THE OBJECT --
// this is where the backup for the undo system is made!
// -----------------------------------------------------------------------------
void MapObject::setModified()
{
	// Backup current properties if required
	if (modified_time_ < prop_backup_time)
	{
		if (obj_backup_)
			delete obj_backup_;
		obj_backup_ = new Backup();
		backupTo(obj_backup_);
	}

	modified_time_ = App::runTimer();
}

// -----------------------------------------------------------------------------
// Copy properties from another MapObject [c]
// -----------------------------------------------------------------------------
void MapObject::copy(MapObject* c)
{
	// Update modified time
	setModified();

	// Can't copy an object of a different type
	if (c->type_ != type_)
		return;

	// Reset properties
	properties_.clear();

	// Copy object properties
	if (!c->properties_.isEmpty())
	{
		c->properties_.copyTo(properties_);
		this->parent_map_ = c->parent_map_;
		this->filtered_   = c->filtered_;
	}
}

// -----------------------------------------------------------------------------
// Returns true if the object has a property matching [key]
// -----------------------------------------------------------------------------
bool MapObject::hasProp(const string& key)
{
	if (properties_.propertyExists(key))
		return properties_[key].hasValue();

	return false;
}

// -----------------------------------------------------------------------------
// Returns the value of the boolean property matching [key]
// -----------------------------------------------------------------------------
bool MapObject::boolProperty(const string& key)
{
	// If the property exists already, return it
	if (properties_[key].hasValue())
		return properties_[key].boolValue();

	// Otherwise check the game configuration for a default value
	else
	{
		UDMFProperty* prop = Game::configuration().getUDMFProperty(key, type_);
		if (prop)
			return prop->defaultValue().boolValue();
		else
			return false;
	}
}

// -----------------------------------------------------------------------------
// Returns the value of the integer property matching [key]
// -----------------------------------------------------------------------------
int MapObject::intProperty(const string& key)
{
	// If the property exists already, return it
	if (properties_[key].hasValue())
		return properties_[key].intValue();

	// Otherwise check the game configuration for a default value
	else
	{
		UDMFProperty* prop = Game::configuration().getUDMFProperty(key, type_);
		if (prop)
			return prop->defaultValue().intValue();
		else
			return 0;
	}
}

// -----------------------------------------------------------------------------
// Returns the value of the float property matching [key]
// -----------------------------------------------------------------------------
double MapObject::floatProperty(const string& key)
{
	// If the property exists already, return it
	if (properties_[key].hasValue())
		return properties_[key].floatValue();

	// Otherwise check the game configuration for a default value
	else
	{
		UDMFProperty* prop = Game::configuration().getUDMFProperty(key, type_);
		if (prop)
			return prop->defaultValue().floatValue();
		else
			return 0;
	}
}

// -----------------------------------------------------------------------------
// Returns the value of the string property matching [key]
// -----------------------------------------------------------------------------
string MapObject::stringProperty(const string& key)
{
	// If the property exists already, return it
	if (properties_[key].hasValue())
		return properties_[key].stringValue();

	// Otherwise check the game configuration for a default value
	else
	{
		UDMFProperty* prop = Game::configuration().getUDMFProperty(key, type_);
		if (prop)
			return prop->defaultValue().stringValue();
		else
			return "";
	}
}

// -----------------------------------------------------------------------------
// Sets the boolean value of the property [key] to [value]
// -----------------------------------------------------------------------------
void MapObject::setBoolProperty(const string& key, bool value)
{
	// Update modified time
	setModified();

	// Set property
	properties_[key] = value;
}

// -----------------------------------------------------------------------------
// Sets the integer value of the property [key] to [value]
// -----------------------------------------------------------------------------
void MapObject::setIntProperty(const string& key, int value)
{
	// Update modified time
	setModified();

	// Set property
	properties_[key] = value;
}

// -----------------------------------------------------------------------------
// Sets the float value of the property [key] to [value]
// -----------------------------------------------------------------------------
void MapObject::setFloatProperty(const string& key, double value)
{
	// Update modified time
	setModified();

	// Set property
	properties_[key] = value;
}

// -----------------------------------------------------------------------------
// Sets the string value of the property [key] to [value]
// -----------------------------------------------------------------------------
void MapObject::setStringProperty(const string& key, const string& value)
{
	// Update modified time
	setModified();

	// Set property
	properties_[key] = value;
}

// -----------------------------------------------------------------------------
// Writes all object properties to [backup]
// -----------------------------------------------------------------------------
void MapObject::backupTo(Backup* backup)
{
	// Save basic info
	backup->id   = obj_id_;
	backup->type = type_;

	// Save general properties to list
	properties_.copyTo(backup->properties);

	// Object-specific properties
	writeBackup(backup);
}

// -----------------------------------------------------------------------------
// Restores all object properties from [backup]
// -----------------------------------------------------------------------------
void MapObject::loadFromBackup(Backup* backup)
{
	// Check type match
	if (backup->type != type_)
	{
		LOG_MESSAGE(1, "loadFromBackup: Mobj type mismatch, %d != %d", type_, backup->type);
		return;
	}
	// Check id match
	if (backup->id != obj_id_)
	{
		LOG_MESSAGE(1, "loadFromBackup: Mobj id mismatch, %d != %d", obj_id_, backup->id);
		return;
	}

	// Update modified time
	setModified();

	// Load general properties from list
	properties_.clear();
	backup->properties.copyTo(properties_);

	// Object-specific properties
	readBackup(backup);
}

// -----------------------------------------------------------------------------
// Returns the backup struct for this object
// -----------------------------------------------------------------------------
MapObject::Backup* MapObject::backup(bool remove)
{
	Backup* bak = obj_backup_;
	if (remove)
		obj_backup_ = nullptr;
	return bak;
}


// -----------------------------------------------------------------------------
//
// MapObject Class Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the property backup time (used for the undo system - if an object's
// properties are modified, they will be backed up first if they haven't since
// this time)
// -----------------------------------------------------------------------------
long MapObject::propBackupTime()
{
	return prop_backup_time;
}

// -----------------------------------------------------------------------------
// Begins property backup, any time a MapObject property is changed its
// properties will be backed up before changing (only once)
// -----------------------------------------------------------------------------
void MapObject::beginPropBackup(long current_time)
{
	prop_backup_time = current_time;
}

// -----------------------------------------------------------------------------
// End property backup
// -----------------------------------------------------------------------------
void MapObject::endPropBackup()
{
	prop_backup_time = -1;
}


// -----------------------------------------------------------------------------
// Checks the boolean property [prop] on all objects in [objects].
// If all values are the same, [value] is set and returns true, otherwise
// just returns false
// -----------------------------------------------------------------------------
bool MapObject::multiBoolProperty(vector<MapObject*>& objects, string prop, bool& value)
{
	// Check objects given
	if (objects.empty())
		return false;

	// Check values
	bool first = objects[0]->boolProperty(prop);
	for (unsigned a = 1; a < objects.size(); a++)
	{
		// Differing values
		if (objects[a]->boolProperty(prop) != first)
			return false;
	}

	// All had same value
	value = first;
	return true;
}

// -----------------------------------------------------------------------------
// Checks the integer property [prop] on all objects in [objects].
// If all values are the same, [value] is set and returns true, otherwise
// just returns false
// -----------------------------------------------------------------------------
bool MapObject::multiIntProperty(vector<MapObject*>& objects, string prop, int& value)
{
	// Check objects given
	if (objects.empty())
		return false;

	// Check values
	int first = objects[0]->intProperty(prop);
	for (unsigned a = 1; a < objects.size(); a++)
	{
		// Differing values
		if (objects[a]->intProperty(prop) != first)
			return false;
	}

	// All had same value
	value = first;
	return true;
}

// -----------------------------------------------------------------------------
// Checks the float property [prop] on all objects in [objects].
// If all values are the same, [value] is set and returns true, otherwise
// just returns false
// -----------------------------------------------------------------------------
bool MapObject::multiFloatProperty(vector<MapObject*>& objects, string prop, double& value)
{
	// Check objects given
	if (objects.empty())
		return false;

	// Check values
	double first = objects[0]->floatProperty(prop);
	for (unsigned a = 1; a < objects.size(); a++)
	{
		// Differing values
		if (objects[a]->floatProperty(prop) != first)
			return false;
	}

	// All had same value
	value = first;
	return true;
}

// -----------------------------------------------------------------------------
// Checks the string property [prop] on all objects in [objects].
// If all values are the same, [value] is set and returns true, otherwise
// just returns false
// -----------------------------------------------------------------------------
bool MapObject::multiStringProperty(vector<MapObject*>& objects, string prop, string& value)
{
	// Check objects given
	if (objects.empty())
		return false;

	// Check values
	string first = objects[0]->stringProperty(prop);
	for (unsigned a = 1; a < objects.size(); a++)
	{
		// Differing values
		if (objects[a]->stringProperty(prop) != first)
			return false;
	}

	// All had same value
	value = first;
	return true;
}
