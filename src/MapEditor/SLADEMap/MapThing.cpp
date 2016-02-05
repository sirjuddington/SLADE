
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MapThing.cpp
 * Description: MapThing class, represents a thing object in a map
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
#include "MapThing.h"
#include "MainApp.h"


/*******************************************************************
 * MAPTHING CLASS FUNCTIONS
 *******************************************************************/

/* MapThing::MapThing
 * MapThing class constructor
 *******************************************************************/
MapThing::MapThing(SLADEMap* parent) : MapObject(MOBJ_THING, parent)
{
	// Init variables
	this->x = 0;
	this->y = 0;
	this->type = 1;
	this->angle = 0;
}

/* MapThing::MapThing
 * MapThing class constructor
 *******************************************************************/
MapThing::MapThing(double x, double y, short type, SLADEMap* parent) : MapObject(MOBJ_THING, parent)
{
	// Init variables
	this->x = x;
	this->y = y;
	this->type = type;
	this->angle = 0;
}

/* MapThing::~MapThing
 * MapThing class destructor
 *******************************************************************/
MapThing::~MapThing()
{
}

/* MapThing::getPoint
 * Returns the object point [point]. Currently for things this is
 * always the thing position
 *******************************************************************/
fpoint2_t MapThing::getPoint(uint8_t point)
{
	return fpoint2_t(x, y);
}

/* MapThing::point
 * Returns the position of the thing, more explicitly than the
 * generic method getPoint
 *******************************************************************/
fpoint2_t MapThing::point()
{
	return fpoint2_t(x, y);
}

/* MapThing::intProperty
 * Returns the value of the integer property matching [key]
 *******************************************************************/
int MapThing::intProperty(string key)
{
	if (key == "type")
		return type;
	else if (key == "x")
		return (int)x;
	else if (key == "y")
		return (int)y;
	else if (key == "angle")
		return angle;
	else
		return MapObject::intProperty(key);
}

/* MapThing::floatProperty
 * Returns the value of the float property matching [key]
 *******************************************************************/
double MapThing::floatProperty(string key)
{
	if (key == "x")
		return x;
	else if (key == "y")
		return y;
	else
		return MapObject::floatProperty(key);
}

/* MapThing::setIntProperty
 * Sets the integer value of the property [key] to [value]
 *******************************************************************/
void MapThing::setIntProperty(string key, int value)
{
	// Update modified time
	setModified();

	if (key == "type")
		type = value;
	else if (key == "x")
		x = value;
	else if (key == "y")
		y = value;
	else if (key == "angle")
		angle = value;
	else
		return MapObject::setIntProperty(key, value);
}

/* MapThing::setFloatProperty
 * Sets the float value of the property [key] to [value]
 *******************************************************************/
void MapThing::setFloatProperty(string key, double value)
{
	// Update modified time
	setModified();

	if (key == "x")
		x = value;
	else if (key == "y")
		y = value;
	else
		return MapObject::setFloatProperty(key, value);
}

/* MapThing::copy
 * Copies another map object [c]
 *******************************************************************/
void MapThing::copy(MapObject* c)
{
	// Don't copy a non-thing
	if (c->getObjType() != MOBJ_THING)
		return;

	// Basic variables
	MapThing* thing = (MapThing*)c;
	this->x = thing->x;
	this->y = thing->y;
	this->type = thing->type;
	this->angle = thing->angle;

	// Other properties
	MapObject::copy(c);
}

/* MapThing::setAnglePoint
 * Sets the angle of the thing to be facing towards [point]
 *******************************************************************/
void MapThing::setAnglePoint(fpoint2_t point)
{
	// Calculate direction vector
	fpoint2_t vec(point.x - x, point.y - y);
	double mag = sqrt((vec.x * vec.x) + (vec.y * vec.y));
	double x = vec.x / mag;
	double y = vec.y / mag;

	// Determine angle
	int angle = 0;
	if (x > 0.89)				// east
		angle = 0;
	else if (x < -0.89)			// west
		angle = 180;
	else if (y > 0.89)			// north
		angle = 90;
	else if (y < -0.89)			// south
		angle = 270;
	else if (x > 0 && y > 0)	// northeast
		angle = 45;
	else if (x < 0 && y > 0)	// northwest
		angle = 135;
	else if (x < 0 && y < 0)	// southwest
		angle = 225;
	else if (x > 0 && y < 0)	// southeast
		angle = 315;

	// Set thing angle
	setIntProperty("angle", angle);
}

/* MapThing::writeBackup
 * Write all thing info to a mobj_backup_t struct
 *******************************************************************/
void MapThing::writeBackup(mobj_backup_t* backup)
{
	// Type
	backup->props_internal["type"] = type;

	// Position
	backup->props_internal["x"] = x;
	backup->props_internal["y"] = y;

	// Angle
	backup->props_internal["angle"] = angle;
}

/* MapThing::readBackup
 * Reads all thing info from a mobj_backup_t struct
 *******************************************************************/
void MapThing::readBackup(mobj_backup_t* backup)
{
	// Type
	type = backup->props_internal["type"].getIntValue();

	// Position
	x = backup->props_internal["x"].getFloatValue();
	y = backup->props_internal["y"].getFloatValue();

	// Angle
	angle = backup->props_internal["angle"].getIntValue();
}
