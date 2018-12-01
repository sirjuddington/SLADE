
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapThing.cpp
// Description: MapThing class, represents a thing object in a map
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
#include "MapThing.h"
#include "App.h"


// -----------------------------------------------------------------------------
//
// MapThing Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapThing class constructor
// -----------------------------------------------------------------------------
MapThing::MapThing(SLADEMap* parent) : MapObject(Type::Thing, parent)
{
	// Init variables
	this->x_     = 0;
	this->y_     = 0;
	this->type_  = 1;
	this->angle_ = 0;
}

// -----------------------------------------------------------------------------
// MapThing class constructor
// -----------------------------------------------------------------------------
MapThing::MapThing(double x, double y, short type, SLADEMap* parent) : MapObject(Type::Thing, parent)
{
	// Init variables
	this->x_     = x;
	this->y_     = y;
	this->type_  = type;
	this->angle_ = 0;
}

// -----------------------------------------------------------------------------
// MapThing class destructor
// -----------------------------------------------------------------------------
MapThing::~MapThing() {}

// -----------------------------------------------------------------------------
// Returns the object point [point].
// Currently for things this is always the thing position
// -----------------------------------------------------------------------------
Vec2f MapThing::getPoint(Point point)
{
	return Vec2f(x_, y_);
}

// -----------------------------------------------------------------------------
// Returns the position of the thing, more explicitly than the generic method
// getPoint
// -----------------------------------------------------------------------------
Vec2f MapThing::point()
{
	return Vec2f(x_, y_);
}

// -----------------------------------------------------------------------------
// Returns the value of the integer property matching [key]
// -----------------------------------------------------------------------------
int MapThing::intProperty(const string& key)
{
	if (key == "type")
		return type_;
	else if (key == "x")
		return (int)x_;
	else if (key == "y")
		return (int)y_;
	else if (key == "angle")
		return angle_;
	else
		return MapObject::intProperty(key);
}

// -----------------------------------------------------------------------------
// Returns the value of the float property matching [key]
// -----------------------------------------------------------------------------
double MapThing::floatProperty(const string& key)
{
	if (key == "x")
		return x_;
	else if (key == "y")
		return y_;
	else
		return MapObject::floatProperty(key);
}

// -----------------------------------------------------------------------------
// Sets the integer value of the property [key] to [value]
// -----------------------------------------------------------------------------
void MapThing::setIntProperty(const string& key, int value)
{
	// Update modified time
	setModified();

	if (key == "type")
		type_ = value;
	else if (key == "x")
		x_ = value;
	else if (key == "y")
		y_ = value;
	else if (key == "angle")
		angle_ = value;
	else
		return MapObject::setIntProperty(key, value);
}

// -----------------------------------------------------------------------------
// Sets the float value of the property [key] to [value]
// -----------------------------------------------------------------------------
void MapThing::setFloatProperty(const string& key, double value)
{
	// Update modified time
	setModified();

	if (key == "x")
		x_ = value;
	else if (key == "y")
		y_ = value;
	else
		return MapObject::setFloatProperty(key, value);
}

// -----------------------------------------------------------------------------
// Copies another map object [c]
// -----------------------------------------------------------------------------
void MapThing::copy(MapObject* c)
{
	// Don't copy a non-thing
	if (c->objType() != Type::Thing)
		return;

	// Basic variables
	MapThing* thing = (MapThing*)c;
	this->x_        = thing->x_;
	this->y_        = thing->y_;
	this->type_     = thing->type_;
	this->angle_    = thing->angle_;

	// Other properties
	MapObject::copy(c);
}

// -----------------------------------------------------------------------------
// Sets the angle of the thing to be facing towards [point]
// -----------------------------------------------------------------------------
void MapThing::setAnglePoint(Vec2f point)
{
	// Calculate direction vector
	Vec2f vec(point.x - x_, point.y - y_);
	double    mag = sqrt((vec.x * vec.x) + (vec.y * vec.y));
	double    x   = vec.x / mag;
	double    y   = vec.y / mag;

	// Determine angle
	int angle = 0;
	if (x > 0.89) // east
		angle = 0;
	else if (x < -0.89) // west
		angle = 180;
	else if (y > 0.89) // north
		angle = 90;
	else if (y < -0.89) // south
		angle = 270;
	else if (x > 0 && y > 0) // northeast
		angle = 45;
	else if (x < 0 && y > 0) // northwest
		angle = 135;
	else if (x < 0 && y < 0) // southwest
		angle = 225;
	else if (x > 0 && y < 0) // southeast
		angle = 315;

	// Set thing angle
	setIntProperty("angle", angle);
}

// -----------------------------------------------------------------------------
// Write all thing info to a Backup struct
// -----------------------------------------------------------------------------
void MapThing::writeBackup(Backup* backup)
{
	// Type
	backup->props_internal["type"] = type_;

	// Position
	backup->props_internal["x"] = x_;
	backup->props_internal["y"] = y_;

	// Angle
	backup->props_internal["angle"] = angle_;
}

// -----------------------------------------------------------------------------
// Reads all thing info from a Backup struct
// -----------------------------------------------------------------------------
void MapThing::readBackup(Backup* backup)
{
	// Type
	type_ = backup->props_internal["type"].intValue();

	// Position
	x_ = backup->props_internal["x"].floatValue();
	y_ = backup->props_internal["y"].floatValue();

	// Angle
	angle_ = backup->props_internal["angle"].intValue();
}
