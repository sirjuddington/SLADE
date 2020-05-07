
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Property.cpp
// Description: The Property class. Basically acts as a 'dynamic' variable type,
//              for use in the PropertyList class. Can contain a boolean,
//              integer, floating point (double) or string (string) value.
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
#include "Property.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Property Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Property class default constructor
// -----------------------------------------------------------------------------
Property::Property(Type type) : type_{ type }
{
	switch (type)
	{
	case Type::Boolean: value_.Boolean = false; break;
	case Type::Int: value_.Integer = 0; break;
	case Type::Float: value_.Floating = 0.0f; break;
	case Type::String: break;
	case Type::Flag: value_.Boolean = true; break;
	case Type::UInt: value_.Unsigned = 0; break;
	default:
		// Invalid type given, default to boolean
		type_          = Type::Boolean;
		value_.Boolean = true;
	}
}

// -----------------------------------------------------------------------------
// Property class constructor (boolean)
// -----------------------------------------------------------------------------
Property::Property(bool value) : type_{ Type::Boolean }, has_value_{ true }
{
	// Init boolean property
	value_.Boolean = value;
}

// -----------------------------------------------------------------------------
// Property class constructor (integer)
// -----------------------------------------------------------------------------
Property::Property(int value) : type_{ Type::Int }, has_value_{ true }
{
	// Init integer property
	value_.Integer = value;
}

// -----------------------------------------------------------------------------
// Property class constructor (floating point)
// -----------------------------------------------------------------------------
Property::Property(float value) : type_{ Type::Float }, has_value_{ true }
{
	// Init float property
	value_.Floating = value;
}

// -----------------------------------------------------------------------------
// Property class constructor (floating point)
// -----------------------------------------------------------------------------
Property::Property(double value) : type_{ Type::Float }, has_value_{ true }
{
	// Init float property
	value_.Floating = value;
}

// -----------------------------------------------------------------------------
// Property class constructor (string)
// -----------------------------------------------------------------------------
Property::Property(string_view value) : type_{ Type::String }, value_{}, val_string_{ value }, has_value_{ true } {}

// -----------------------------------------------------------------------------
// Property class constructor (unsigned)
// -----------------------------------------------------------------------------
Property::Property(unsigned value) : type_{ Type::UInt }, has_value_{ true }
{
	// Init string property
	value_.Unsigned = value;
}

// -----------------------------------------------------------------------------
// Returns the property value as a bool.
// If [warn_wrong_type] is true, a warning message is written to the log if the
// property is not of boolean type
// -----------------------------------------------------------------------------
bool Property::boolValue(bool warn_wrong_type) const
{
	// If this is a flag, just return boolean 'true' (or equivalent)
	if (type_ == Type::Flag)
		return true;

	// If the value is undefined, default to false
	if (!has_value_)
		return false;

	// Write warning to log if needed
	if (warn_wrong_type && type_ != Type::Boolean)
		log::warning("Requested Boolean value of a {} Property", typeString());

	// Return value (convert if needed)
	if (type_ == Type::Boolean)
		return value_.Boolean;
	else if (type_ == Type::Int)
		return !!value_.Integer;
	else if (type_ == Type::UInt)
		return !!value_.Unsigned;
	else if (type_ == Type::Float)
		return !!((int)value_.Floating);
	else if (type_ == Type::String)
	{
		// Anything except "0", "no" or "false" is considered true
		return !(val_string_ == '0' || strutil::equalCI(val_string_, "no") || strutil::equalCI(val_string_, "false"));
	}

	// Return default boolean value
	return true;
}

// -----------------------------------------------------------------------------
// Returns the property value as an int.
// If [warn_wrong_type] is true, a warning message is written to the log if the
// property is not of integer type
// -----------------------------------------------------------------------------
int Property::intValue(bool warn_wrong_type) const
{
	// If this is a flag, just return boolean 'true' (or equivalent)
	if (type_ == Type::Flag)
		return 1;

	// If the value is undefined, default to 0
	if (!has_value_)
		return 0;

	// Write warning to log if needed
	if (warn_wrong_type && type_ != Type::Int)
		log::warning("Requested Integer value of a {} Property", typeString());

	// Return value (convert if needed)
	if (type_ == Type::Int)
		return value_.Integer;
	else if (type_ == Type::UInt)
		return (int)value_.Unsigned;
	else if (type_ == Type::Boolean)
		return (int)value_.Boolean;
	else if (type_ == Type::Float)
		return (int)value_.Floating;
	else if (type_ == Type::String)
		return strutil::asInt(val_string_);

	// Return default integer value
	return 0;
}

// -----------------------------------------------------------------------------
// Returns the property value as a double.
// If [warn_wrong_type] is true, a warning message is written to the log if the
// property is not of floating point type
// -----------------------------------------------------------------------------
double Property::floatValue(bool warn_wrong_type) const
{
	// If this is a flag, just return boolean 'true' (or equivalent)
	if (type_ == Type::Flag)
		return 1;

	// If the value is undefined, default to 0
	if (!has_value_)
		return 0;

	// Write warning to log if needed
	if (warn_wrong_type && type_ != Type::Float)
		log::warning("Requested Float value of a {} Property", typeString());

	// Return value (convert if needed)
	if (type_ == Type::Float)
		return value_.Floating;
	else if (type_ == Type::Boolean)
		return (double)value_.Boolean;
	else if (type_ == Type::Int)
		return (double)value_.Integer;
	else if (type_ == Type::UInt)
		return (double)value_.Unsigned;
	else if (type_ == Type::String)
		return strutil::asDouble(val_string_);

	// Return default float value
	return 0.0f;
}

// -----------------------------------------------------------------------------
// Returns the property value as a string.
// If [warn_wrong_type] is true, a warning message is written to the log if the
// property is not of string type
// -----------------------------------------------------------------------------
string Property::stringValue(bool warn_wrong_type) const
{
	// If this is a flag, just return boolean 'true' (or equivalent)
	if (type_ == Type::Flag)
		return "1";

	// If the value is undefined, default to null
	if (!has_value_)
		return "";

	// Write warning to log if needed
	if (warn_wrong_type && type_ != Type::String)
		log::warning("Warning: Requested String value of a {} Property", typeString());

	// Return value (convert if needed)
	if (type_ == Type::String)
		return val_string_;
	else if (type_ == Type::Int)
		return fmt::format("{}", value_.Integer);
	else if (type_ == Type::UInt)
		return fmt::format("{}", value_.Unsigned);
	else if (type_ == Type::Boolean)
	{
		if (value_.Boolean)
			return "true";
		else
			return "false";
	}
	else if (type_ == Type::Float)
		return fmt::format("{}", value_.Floating);

	// Return default string value
	return {};
}

// -----------------------------------------------------------------------------
// Returns the property value as an unsigned int.
// If [warn_wrong_type] is true, a warning message is written to the log if the
// property is not of integer type
// -----------------------------------------------------------------------------
unsigned Property::unsignedValue(bool warn_wrong_type) const
{
	// If this is a flag, just return boolean 'true' (or equivalent)
	if (type_ == Type::Flag)
		return 1;

	// If the value is undefined, default to 0
	if (!has_value_)
		return 0;

	// Write warning to log if needed
	if (warn_wrong_type && type_ != Type::Int)
		log::warning("Requested Integer value of a {} Property", typeString());

	// Return value (convert if needed)
	if (type_ == Type::Int)
		return value_.Integer;
	else if (type_ == Type::Boolean)
		return (int)value_.Boolean;
	else if (type_ == Type::Float)
		return (int)value_.Floating;
	else if (type_ == Type::String)
		return strutil::asUInt(val_string_);
	else if (type_ == Type::UInt)
		return value_.Unsigned;

	// Return default integer value
	return 0;
}

// -----------------------------------------------------------------------------
// Sets the property to [val], and changes its type to boolean if necessary
// -----------------------------------------------------------------------------
void Property::setValue(bool val)
{
	// Change type if necessary
	if (type_ != Type::Boolean)
		changeType(Type::Boolean);

	// Set value
	value_.Boolean = val;
	has_value_     = true;
}

// -----------------------------------------------------------------------------
// Sets the property to [val], and changes its type to integer if necessary
// -----------------------------------------------------------------------------
void Property::setValue(int val)
{
	// Change type if necessary
	if (type_ != Type::Int)
		changeType(Type::Int);

	// Set value
	value_.Integer = val;
	has_value_     = true;
}

// -----------------------------------------------------------------------------
// Sets the property to [val], and changes its type to floating point if
// necessary
// -----------------------------------------------------------------------------
void Property::setValue(double val)
{
	// Change type if necessary
	if (type_ != Type::Float)
		changeType(Type::Float);

	// Set value
	value_.Floating = val;
	has_value_      = true;
}

// -----------------------------------------------------------------------------
// Sets the property to [val], and changes its type to string if necessary
// -----------------------------------------------------------------------------
void Property::setValue(string_view val)
{
	// Change type if necessary
	if (type_ != Type::String)
		changeType(Type::String);

	// Set value
	val_string_ = val;
	has_value_  = true;
}

// -----------------------------------------------------------------------------
// Sets the property to [val], and changes its type to unsigned int if necessary
// -----------------------------------------------------------------------------
void Property::setValue(unsigned val)
{
	// Change type if necessary
	if (type_ != Type::UInt)
		changeType(Type::UInt);

	// Set value
	value_.Unsigned = val;
	has_value_      = true;
}

// -----------------------------------------------------------------------------
// Changes the property's value type and gives it a default value
// -----------------------------------------------------------------------------
void Property::changeType(Type newtype)
{
	// Do nothing if changing to same type
	if (type_ == newtype)
		return;

	// Clear string data if changing from string
	if (type_ == Type::String)
		val_string_.clear();

	// Update type
	type_ = newtype;

	// Update value
	if (type_ == Type::Boolean)
		value_.Boolean = true;
	else if (type_ == Type::Int)
		value_.Integer = 0;
	else if (type_ == Type::Float)
		value_.Floating = 0.0f;
	// else if (type_ == Type::String)
	//	val_string_ = "";
	else if (type_ == Type::Flag)
		value_.Boolean = true;
	else if (type_ == Type::UInt)
		value_.Unsigned = 0;
}

// -----------------------------------------------------------------------------
// Returns a string representing the property's value type
// -----------------------------------------------------------------------------
string Property::typeString() const
{
	switch (type_)
	{
	case Type::Boolean: return "Boolean";
	case Type::Int: return "Integer";
	case Type::Float: return "Float";
	case Type::String: return "String";
	case Type::Flag: return "Flag";
	case Type::UInt: return "Unsigned";
	default: return "Unknown";
	}
}
