
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
#include "Utility/Parser.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
const string MapThing::PROP_X     = "x";
const string MapThing::PROP_Y     = "y";
const string MapThing::PROP_TYPE  = "type";
const string MapThing::PROP_ANGLE = "angle";
const string MapThing::PROP_FLAGS = "flags";


// -----------------------------------------------------------------------------
//
// MapThing Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapThing class constructor
// -----------------------------------------------------------------------------
MapThing::MapThing(const Vec2f& pos, short type, short angle, short flags) :
	MapObject(Type::Thing),
	type_{ type },
	position_{ pos },
	angle_{ angle }
{
	properties_[PROP_FLAGS] = flags; // TODO: convert to member variable
}

// -----------------------------------------------------------------------------
// MapThing class constructor from UDMF definition
// -----------------------------------------------------------------------------
MapThing::MapThing(const Vec2f& pos, short type, ParseTreeNode* def) :
	MapObject(Type::Thing),
	type_{ type },
	position_{ pos }
{
	// Set properties from UDMF definition
	ParseTreeNode* prop;
	for (unsigned a = 0; a < def->nChildren(); a++)
	{
		prop = def->childPTN(a);

		// Skip required properties
		if (prop->nameIsCI(PROP_X) || prop->nameIsCI(PROP_Y) || prop->nameIsCI(PROP_TYPE))
			continue;

		// Builtin properties
		if (prop->nameIsCI(PROP_ANGLE))
			angle_ = prop->intValue();
		else
			properties_[prop->name()] = prop->value();
	}
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
	if (key == PROP_TYPE)
		return type_;
	else if (key == PROP_X)
		return (int)position_.x;
	else if (key == PROP_Y)
		return (int)position_.y;
	else if (key == PROP_ANGLE)
		return angle_;
	else
		return MapObject::intProperty(key);
}

// -----------------------------------------------------------------------------
// Returns the value of the float property matching [key]
// -----------------------------------------------------------------------------
double MapThing::floatProperty(const string& key)
{
	if (key == PROP_X)
		return position_.x;
	else if (key == PROP_Y)
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

	if (key == PROP_TYPE)
		type_ = value;
	else if (key == PROP_X)
		position_.x = value;
	else if (key == PROP_Y)
		position_.y = value;
	else if (key == PROP_ANGLE)
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

	if (key == PROP_X)
		position_.x = value;
	else if (key == PROP_Y)
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
// Sets the position of the thing to [pos].
// If [modify] is false, the thing won't be marked as modified
// -----------------------------------------------------------------------------
void MapThing::move(Vec2f pos, bool modify)
{
	if (modify)
		setModified();
	position_ = pos;
}

// -----------------------------------------------------------------------------
// Sets the angle (direction) of the thing to [angle].
// If [modify] is false, the thing won't be marked as modified
// -----------------------------------------------------------------------------
void MapThing::setAngle(int angle, bool modify)
{
	if (modify)
		setModified();
	angle_ = angle;
}

// -----------------------------------------------------------------------------
// Sets the angle (direction) of the thing to be facing towards [point].
// If [modify] is false, the thing won't be marked as modified
// -----------------------------------------------------------------------------
void MapThing::setAnglePoint(Vec2f point, bool modify)
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
	if (modify)
		setModified();
	angle_ = angle;
}

// -----------------------------------------------------------------------------
// Write all thing info to a Backup struct
// -----------------------------------------------------------------------------
void MapThing::writeBackup(Backup* backup)
{
	// Type
	backup->props_internal[PROP_TYPE] = type_;

	// Position
	backup->props_internal[PROP_X] = position_.x;
	backup->props_internal[PROP_Y] = position_.y;

	// Angle
	backup->props_internal[PROP_ANGLE] = angle_;
}

// -----------------------------------------------------------------------------
// Reads all thing info from a Backup struct
// -----------------------------------------------------------------------------
void MapThing::readBackup(Backup* backup)
{
	// Type
	type_ = backup->props_internal[PROP_TYPE].intValue();

	// Position
	position_.x = backup->props_internal[PROP_X].floatValue();
	position_.y = backup->props_internal[PROP_Y].floatValue();

	// Angle
	angle_ = backup->props_internal[PROP_ANGLE].intValue();
}

// -----------------------------------------------------------------------------
// Writes the thing as a UDMF text definition to [def]
// -----------------------------------------------------------------------------
void MapThing::writeUDMF(string& def)
{
	def = S_FMT("thing//#%u\n{\n", index_);

	// Basic properties
	def += S_FMT("x=%1.3f;\ny=%1.3f;\ntype=%d;\n", position_.x, position_.y, type_);
	if (angle_ != 0)
		def += S_FMT("angle=%d;\n", angle_);

	// Other properties
	if (!properties_.isEmpty())
		def += properties_.toString(true);

	def += "}\n\n";
}
