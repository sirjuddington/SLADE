
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
MapThing::MapThing(double x, double y, short type, SLADEMap* parent) :
	MapObject(Type::Thing, parent),
	type_{ type },
	position_{ x, y }
{
}

// -----------------------------------------------------------------------------
// Returns the object point [point].
// Currently for things this is always the thing position
// -----------------------------------------------------------------------------
Vec2f MapThing::getPoint(Point point)
{
	return position_;
}

// -----------------------------------------------------------------------------
// Returns the value of the integer property matching [key]
// -----------------------------------------------------------------------------
int MapThing::intProperty(const string& key)
{
	if (key == "type")
		return type_;
	else if (key == "x")
		return (int)position_.x;
	else if (key == "y")
		return (int)position_.y;
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
		return position_.x;
	else if (key == "y")
		return position_.y;
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
		position_.x = value;
	else if (key == "y")
		position_.y = value;
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
		position_.x = value;
	else if (key == "y")
		position_.y = value;
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
	auto thing        = dynamic_cast<MapThing*>(c);
	this->position_.x = thing->position_.x;
	this->position_.y = thing->position_.y;
	this->type_       = thing->type_;
	this->angle_      = thing->angle_;

	// Other properties
	MapObject::copy(c);
}

// -----------------------------------------------------------------------------
// Sets the angle of the thing to be facing towards [point]
// -----------------------------------------------------------------------------
void MapThing::setAnglePoint(Vec2f point)
{
	// Calculate direction vector
	Vec2f  vec(point.x - position_.x, point.y - position_.y);
	double mag = sqrt((vec.x * vec.x) + (vec.y * vec.y));
	double x   = vec.x / mag;
	double y   = vec.y / mag;

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
	backup->props_internal["x"] = position_.x;
	backup->props_internal["y"] = position_.y;

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
	position_.x = backup->props_internal["x"].floatValue();
	position_.y = backup->props_internal["y"].floatValue();

	// Angle
	angle_ = backup->props_internal["angle"].intValue();
}
