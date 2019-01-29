
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
const string MapThing::PROP_X       = "x";
const string MapThing::PROP_Y       = "y";
const string MapThing::PROP_Z       = "height";
const string MapThing::PROP_TYPE    = "type";
const string MapThing::PROP_ANGLE   = "angle";
const string MapThing::PROP_FLAGS   = "flags";
const string MapThing::PROP_ARG0    = "arg0";
const string MapThing::PROP_ARG1    = "arg1";
const string MapThing::PROP_ARG2    = "arg2";
const string MapThing::PROP_ARG3    = "arg3";
const string MapThing::PROP_ARG4    = "arg4";
const string MapThing::PROP_ID      = "id";
const string MapThing::PROP_SPECIAL = "special";


// -----------------------------------------------------------------------------
//
// MapThing Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapThing class constructor
// -----------------------------------------------------------------------------
MapThing::MapThing(const Vec3d& pos, short type, short angle, short flags, const ArgSet& args, int id, int special) :
	MapObject(Type::Thing),
	type_{ type },
	position_{ pos.x, pos.y },
	z_{ pos.z },
	angle_{ angle },
	flags_{ flags },
	args_{ args },
	id_{ id },
	special_{ special }
{
}

// -----------------------------------------------------------------------------
// MapThing class constructor from UDMF definition
// -----------------------------------------------------------------------------
MapThing::MapThing(const Vec3d& pos, short type, ParseTreeNode* def) :
	MapObject(Type::Thing),
	type_{ type },
	position_{ pos.x, pos.y },
	z_{ pos.z }
{
	// Set properties from UDMF definition
	ParseTreeNode* prop;
	for (unsigned a = 0; a < def->nChildren(); a++)
	{
		prop           = def->childPTN(a);
		auto& propName = prop->nameRef();

		// Skip required properties
		if (propName == PROP_X || propName == PROP_Y || propName == PROP_Z || propName == PROP_TYPE)
			continue;

		// Builtin properties
		if (propName == PROP_ANGLE)
			angle_ = prop->intValue();
		else if (propName == PROP_FLAGS)
			flags_ = prop->intValue();
		else if (propName == PROP_ARG0)
			args_[0] = prop->intValue();
		else if (propName == PROP_ARG1)
			args_[1] = prop->intValue();
		else if (propName == PROP_ARG2)
			args_[2] = prop->intValue();
		else if (propName == PROP_ARG3)
			args_[3] = prop->intValue();
		else if (propName == PROP_ARG4)
			args_[4] = prop->intValue();
		else if (propName == PROP_ID)
			id_ = prop->intValue();
		else if (propName == PROP_SPECIAL)
			special_ = prop->intValue();
		else
			properties_[prop->nameRef()] = prop->value();
	}
}

// -----------------------------------------------------------------------------
// Returns the object point [point].
// Currently for things this is always the thing position
// -----------------------------------------------------------------------------
Vec2d MapThing::getPoint(Point point)
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
	if (key == PROP_X)
		return (int)position_.x;
	if (key == PROP_Y)
		return (int)position_.y;
	if (key == PROP_Z)
		return (int)z_;
	if (key == PROP_ANGLE)
		return angle_;
	if (key == PROP_FLAGS)
		return flags_;
	if (key == PROP_ARG0)
		return args_[0];
	if (key == PROP_ARG1)
		return args_[1];
	if (key == PROP_ARG2)
		return args_[2];
	if (key == PROP_ARG3)
		return args_[3];
	if (key == PROP_ARG4)
		return args_[4];
	if (key == PROP_ID)
		return id_;
	if (key == PROP_SPECIAL)
		return special_;

	return MapObject::intProperty(key);
}

// -----------------------------------------------------------------------------
// Returns the value of the float property matching [key]
// -----------------------------------------------------------------------------
double MapThing::floatProperty(const string& key)
{
	if (key == PROP_X)
		return position_.x;
	if (key == PROP_Y)
		return position_.y;
	if (key == PROP_Z)
		return z_;

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
	else if (key == PROP_Z)
		z_ = value;
	else if (key == PROP_ANGLE)
		angle_ = value;
	else if (key == PROP_FLAGS)
		flags_ = value;
	else if (key == PROP_ARG0)
		args_[0] = value;
	else if (key == PROP_ARG1)
		args_[1] = value;
	else if (key == PROP_ARG2)
		args_[2] = value;
	else if (key == PROP_ARG3)
		args_[3] = value;
	else if (key == PROP_ARG4)
		args_[4] = value;
	else if (key == PROP_ID)
		id_ = value;
	else if (key == PROP_SPECIAL)
		special_ = value;
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
	else if (key == PROP_Z)
		z_ = value;
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
	auto thing  = dynamic_cast<MapThing*>(c);
	position_.x = thing->position_.x;
	position_.y = thing->position_.y;
	type_       = thing->type_;
	angle_      = thing->angle_;
	flags_      = thing->flags_;
	id_         = thing->id_;
	special_    = thing->special_;
	for (unsigned i = 0; i < 5; ++i)
		args_[i] = thing->args_[i];

	// Other properties
	MapObject::copy(c);
}

// -----------------------------------------------------------------------------
// Sets the position of the thing to [pos].
// If [modify] is false, the thing won't be marked as modified
// -----------------------------------------------------------------------------
void MapThing::move(Vec2d pos, bool modify)
{
	if (modify)
		setModified();
	position_ = pos;
}

// -----------------------------------------------------------------------------
// Sets the Z-position (height) of the thing to [pos]
// -----------------------------------------------------------------------------
void MapThing::setZ(double z)
{
	setModified();
	z_ = z;
}

// -----------------------------------------------------------------------------
// Sets the thing [type]
// -----------------------------------------------------------------------------
void MapThing::setType(int type)
{
	setModified();
	type_ = type;
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
void MapThing::setAnglePoint(Vec2d point, bool modify)
{
	// Calculate direction vector
	Vec2d  vec(point.x - position_.x, point.y - position_.y);
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
// Sets the thing id to [id]
// -----------------------------------------------------------------------------
void MapThing::setId(int id)
{
	setModified();
	id_ = id;
}

// -----------------------------------------------------------------------------
// Sets all thing flags to [flags]
// -----------------------------------------------------------------------------
void MapThing::setFlags(int flags)
{
	setModified();
	flags_ = flags;
}

// -----------------------------------------------------------------------------
// Sets a thing [flag]
// -----------------------------------------------------------------------------
void MapThing::setFlag(int flag)
{
	setModified();
	flags_ |= flag;
}

// -----------------------------------------------------------------------------
// Clears a thing [flag]
// -----------------------------------------------------------------------------
void MapThing::clearFlag(int flag)
{
	setModified();
	flags_ = flags_ & ~flag;
}

// -----------------------------------------------------------------------------
// Sets the thing arg at [index] to [value]
// -----------------------------------------------------------------------------
void MapThing::setArg(unsigned index, int value)
{
	if (index < 5)
	{
		setModified();
		args_[index] = value;
	}
}

// -----------------------------------------------------------------------------
// Sets the thing action special to [special]
// -----------------------------------------------------------------------------
void MapThing::setSpecial(int special)
{
	setModified();
	special_ = special;
}

// -----------------------------------------------------------------------------
// Write all thing info to a Backup struct
// -----------------------------------------------------------------------------
void MapThing::writeBackup(Backup* backup)
{
	backup->props_internal[PROP_TYPE]    = type_;
	backup->props_internal[PROP_X]       = position_.x;
	backup->props_internal[PROP_Y]       = position_.y;
	backup->props_internal[PROP_Z]       = z_;
	backup->props_internal[PROP_ANGLE]   = angle_;
	backup->props_internal[PROP_FLAGS]   = flags_;
	backup->props_internal[PROP_ARG0]    = args_[0];
	backup->props_internal[PROP_ARG1]    = args_[1];
	backup->props_internal[PROP_ARG2]    = args_[2];
	backup->props_internal[PROP_ARG3]    = args_[3];
	backup->props_internal[PROP_ARG4]    = args_[4];
	backup->props_internal[PROP_ID]      = id_;
	backup->props_internal[PROP_SPECIAL] = special_;
}

// -----------------------------------------------------------------------------
// Reads all thing info from a Backup struct
// -----------------------------------------------------------------------------
void MapThing::readBackup(Backup* backup)
{
	type_       = backup->props_internal[PROP_TYPE].intValue();
	position_.x = backup->props_internal[PROP_X].floatValue();
	position_.y = backup->props_internal[PROP_Y].floatValue();
	z_          = backup->props_internal[PROP_Z].floatValue();
	angle_      = backup->props_internal[PROP_ANGLE].intValue();
	flags_      = backup->props_internal[PROP_FLAGS].intValue();
	args_[0]    = backup->props_internal[PROP_ARG0].intValue();
	args_[1]    = backup->props_internal[PROP_ARG1].intValue();
	args_[2]    = backup->props_internal[PROP_ARG2].intValue();
	args_[3]    = backup->props_internal[PROP_ARG3].intValue();
	args_[4]    = backup->props_internal[PROP_ARG4].intValue();
	id_         = backup->props_internal[PROP_ID].intValue();
	special_    = backup->props_internal[PROP_SPECIAL].intValue();
}

// -----------------------------------------------------------------------------
// Writes the thing as a UDMF text definition to [def]
// -----------------------------------------------------------------------------
void MapThing::writeUDMF(string& def)
{
	def = S_FMT("thing//#%u\n{\n", index_);

	// Basic properties
	def += S_FMT("x=%1.3f;\ny=%1.3f;\ntype=%d;\n", position_.x, position_.y, type_);
	if (z_ != 0)
		def += S_FMT("height=%1.3f;\n", z_);
	if (angle_ != 0)
		def += S_FMT("angle=%d;\n", angle_);
	if (flags_ != 0)
		def += S_FMT("flags=%d;\n", flags_);
	if (id_ != 0)
		def += S_FMT("id=%d;\n", id_);
	for (unsigned i = 0; i < 5; ++i)
		if (args_[i] != 0)
			def += S_FMT("arg%d=%d;\n", i, args_[i]);
	if (special_ != 0)
		def += S_FMT("special=%d;\n", special_);

	// Other properties
	if (!properties_.isEmpty())
		def += properties_.toString(true);

	def += "}\n\n";
}
