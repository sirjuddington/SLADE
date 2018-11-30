
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Property.cpp
// Description: The Property class. Basically acts as a 'dynamic' variable type,
//              for use in the PropertyList class. Can contain a boolean,
//              integer, floating point (double) or string (wxString) value.
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


// -----------------------------------------------------------------------------
//
// Property Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Property class default constructor
// -----------------------------------------------------------------------------
Property::Property(Type type)
{
	// Set property type
	this->type_      = type;
	this->has_value_ = false;

	// Set default value depending on type
	if (type == Type::Bool)
		value_.Boolean = false;
	else if (type == Type::Int)
		value_.Integer = 0;
	else if (type == Type::Float)
		value_.Floating = 0.0f;
	else if (type == Type::String)
		val_string_ = wxEmptyString;
	else if (type == Type::Flag)
		value_.Boolean = true;
	else if (type == Type::UInt)
		value_.Unsigned = 0;
	else
	{
		// Invalid type given, default to boolean
		this->type_    = Type::Bool;
		value_.Boolean = true;
	}
}

// -----------------------------------------------------------------------------
// Property class copy constructor
// -----------------------------------------------------------------------------
Property::Property(const Property& copy)
{
	this->type_       = copy.type_;
	this->value_      = copy.value_;
	this->val_string_ = copy.val_string_;
	this->has_value_  = copy.has_value_;
}

// -----------------------------------------------------------------------------
// Property class constructor (boolean)
// -----------------------------------------------------------------------------
Property::Property(bool value)
{
	// Init boolean property
	this->type_          = Type::Bool;
	this->value_.Boolean = value;
	this->has_value_     = true;
}

// -----------------------------------------------------------------------------
// Property class constructor (integer)
// -----------------------------------------------------------------------------
Property::Property(int value)
{
	// Init integer property
	this->type_          = Type::Int;
	this->value_.Integer = value;
	this->has_value_     = true;
}

// -----------------------------------------------------------------------------
// Property class constructor (floating point)
// -----------------------------------------------------------------------------
Property::Property(double value)
{
	// Init float property
	this->type_           = Type::Float;
	this->value_.Floating = value;
	this->has_value_      = true;
}

// -----------------------------------------------------------------------------
// Property class constructor (string)
// -----------------------------------------------------------------------------
Property::Property(string value)
{
	// Init string property
	this->type_       = Type::String;
	this->val_string_ = value;
	this->has_value_  = true;
}

// -----------------------------------------------------------------------------
// Property class constructor (unsigned)
// -----------------------------------------------------------------------------
Property::Property(unsigned value)
{
	// Init string property
	this->type_           = Type::UInt;
	this->value_.Unsigned = value;
	this->has_value_      = true;
}

// -----------------------------------------------------------------------------
// Property class destructor
// -----------------------------------------------------------------------------
Property::~Property() {}

// -----------------------------------------------------------------------------
// Returns the property value as a bool.
// If [warn_wrong_type] is true, a warning message is written to the log if the
// property is not of boolean type
// -----------------------------------------------------------------------------
bool Property::getBoolValue(bool warn_wrong_type) const
{
	// If this is a flag, just return boolean 'true' (or equivalent)
	if (type_ == Type::Flag)
		return true;

	// If the value is undefined, default to false
	if (!has_value_)
		return false;

	// Write warning to log if needed
	if (warn_wrong_type && type_ != Type::Bool)
		LOG_MESSAGE(1, "Warning: Requested Boolean value of a %s Property", typeString());

	// Return value (convert if needed)
	if (type_ == Type::Bool)
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
		if (!val_string_.Cmp("0") || !val_string_.CmpNoCase("no") || !val_string_.CmpNoCase("false"))
			return false;
		else
			return true;
	}

	// Return default boolean value
	return true;
}

// -----------------------------------------------------------------------------
// Returns the property value as an int.
// If [warn_wrong_type] is true, a warning message is written to the log if the
// property is not of integer type
// -----------------------------------------------------------------------------
int Property::getIntValue(bool warn_wrong_type) const
{
	// If this is a flag, just return boolean 'true' (or equivalent)
	if (type_ == Type::Flag)
		return 1;

	// If the value is undefined, default to 0
	if (!has_value_)
		return 0;

	// Write warning to log if needed
	if (warn_wrong_type && type_ != Type::Int)
		LOG_MESSAGE(1, "Warning: Requested Integer value of a %s Property", typeString());

	// Return value (convert if needed)
	if (type_ == Type::Int)
		return value_.Integer;
	else if (type_ == Type::UInt)
		return (int)value_.Unsigned;
	else if (type_ == Type::Bool)
		return (int)value_.Boolean;
	else if (type_ == Type::Float)
		return (int)value_.Floating;
	else if (type_ == Type::String)
		return atoi(CHR(val_string_));

	// Return default integer value
	return 0;
}

// -----------------------------------------------------------------------------
// Returns the property value as a double.
// If [warn_wrong_type] is true, a warning message is written to the log if the
// property is not of floating point type
// -----------------------------------------------------------------------------
double Property::getFloatValue(bool warn_wrong_type) const
{
	// If this is a flag, just return boolean 'true' (or equivalent)
	if (type_ == Type::Flag)
		return 1;

	// If the value is undefined, default to 0
	if (!has_value_)
		return 0;

	// Write warning to log if needed
	if (warn_wrong_type && type_ != Type::Float)
		LOG_MESSAGE(1, "Warning: Requested Float value of a %s Property", typeString());

	// Return value (convert if needed)
	if (type_ == Type::Float)
		return value_.Floating;
	else if (type_ == Type::Bool)
		return (double)value_.Boolean;
	else if (type_ == Type::Int)
		return (double)value_.Integer;
	else if (type_ == Type::UInt)
		return (double)value_.Unsigned;
	else if (type_ == Type::String)
		return (double)atof(CHR(val_string_));

	// Return default float value
	return 0.0f;
}

// -----------------------------------------------------------------------------
// Returns the property value as a string.
// If [warn_wrong_type] is true, a warning message is written to the log if the
// property is not of string type
// -----------------------------------------------------------------------------
string Property::getStringValue(bool warn_wrong_type) const
{
	// If this is a flag, just return boolean 'true' (or equivalent)
	if (type_ == Type::Flag)
		return "1";

	// If the value is undefined, default to null
	if (!has_value_)
		return "";

	// Write warning to log if needed
	if (warn_wrong_type && type_ != Type::String)
		LOG_MESSAGE(1, "Warning: Requested String value of a %s Property", typeString());

	// Return value (convert if needed)
	if (type_ == Type::String)
		return val_string_;
	else if (type_ == Type::Int)
		return S_FMT("%d", value_.Integer);
	else if (type_ == Type::UInt)
		return S_FMT("%u", value_.Unsigned);
	else if (type_ == Type::Bool)
	{
		if (value_.Boolean)
			return "true";
		else
			return "false";
	}
	else if (type_ == Type::Float)
		return S_FMT("%f", value_.Floating);

	// Return default string value
	return wxEmptyString;
}

// -----------------------------------------------------------------------------
// Returns the property value as an unsigned int.
// If [warn_wrong_type] is true, a warning message is written to the log if the
// property is not of integer type
// -----------------------------------------------------------------------------
unsigned Property::getUnsignedValue(bool warn_wrong_type) const
{
	// If this is a flag, just return boolean 'true' (or equivalent)
	if (type_ == Type::Flag)
		return 1;

	// If the value is undefined, default to 0
	if (!has_value_)
		return 0;

	// Write warning to log if needed
	if (warn_wrong_type && type_ != Type::Int)
		LOG_MESSAGE(1, "Warning: Requested Integer value of a %s Property", typeString());

	// Return value (convert if needed)
	if (type_ == Type::Int)
		return value_.Integer;
	else if (type_ == Type::Bool)
		return (int)value_.Boolean;
	else if (type_ == Type::Float)
		return (int)value_.Floating;
	else if (type_ == Type::String)
		return atoi(CHR(val_string_));
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
	if (type_ != Type::Bool)
		changeType(Type::Bool);

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
void Property::setValue(string val)
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
		val_string_.Clear();

	// Update type
	type_ = newtype;

	// Update value
	if (type_ == Type::Bool)
		value_.Boolean = true;
	else if (type_ == Type::Int)
		value_.Integer = 0;
	else if (type_ == Type::Float)
		value_.Floating = 0.0f;
	else if (type_ == Type::String)
		val_string_ = wxEmptyString;
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
	case Type::Bool: return "Boolean";
	case Type::Int: return "Integer";
	case Type::Float: return "Float";
	case Type::String: return "String";
	case Type::Flag: return "Flag";
	case Type::UInt: return "Unsigned";
	default: return "Unknown";
	}
}
