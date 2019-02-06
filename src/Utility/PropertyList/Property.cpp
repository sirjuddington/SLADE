
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
Property::Property(Type type) : type_{ type }
{
	// Set default value depending on type
	if (type == Type::Boolean)
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
		type_          = Type::Boolean;
		value_.Boolean = true;
	}
}

// -----------------------------------------------------------------------------
// Property class constructor (boolean)
// -----------------------------------------------------------------------------
Property::Property(bool value)
{
	// Init boolean property
	type_          = Type::Boolean;
	value_.Boolean = value;
	has_value_     = true;
}

// -----------------------------------------------------------------------------
// Property class constructor (integer)
// -----------------------------------------------------------------------------
Property::Property(int value)
{
	// Init integer property
	type_          = Type::Int;
	value_.Integer = value;
	has_value_     = true;
}

// -----------------------------------------------------------------------------
// Property class constructor (floating point)
// -----------------------------------------------------------------------------
Property::Property(float value)
{
	// Init float property
	type_           = Type::Float;
	value_.Floating = value;
	has_value_      = true;
}

// -----------------------------------------------------------------------------
// Property class constructor (floating point)
// -----------------------------------------------------------------------------
Property::Property(double value)
{
	// Init float property
	type_           = Type::Float;
	value_.Floating = value;
	has_value_      = true;
}

// -----------------------------------------------------------------------------
// Property class constructor (string)
// -----------------------------------------------------------------------------
Property::Property(const wxString& value) : value_{}
{
	// Init string property
	type_       = Type::String;
	val_string_ = value;
	has_value_  = true;
}

// -----------------------------------------------------------------------------
// Property class constructor (unsigned)
// -----------------------------------------------------------------------------
Property::Property(unsigned value)
{
	// Init string property
	type_           = Type::UInt;
	value_.Unsigned = value;
	has_value_      = true;
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
		Log::warning(S_FMT("Requested Boolean value of a %s Property", typeString()));

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
		return !(!val_string_.Cmp("0") || !val_string_.CmpNoCase("no") || !val_string_.CmpNoCase("false"));
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
		Log::warning(S_FMT("Requested Integer value of a %s Property", typeString()));

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
	{
		long tmp;
		if (val_string_.ToLong(&tmp))
			return tmp;
		return 0;
	}

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
		Log::warning(S_FMT("Requested Float value of a %s Property", typeString()));

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
	{
		double tmp;
		if (val_string_.ToDouble(&tmp))
			return tmp;
		return 0.;
	}

	// Return default float value
	return 0.0f;
}

// -----------------------------------------------------------------------------
// Returns the property value as a string.
// If [warn_wrong_type] is true, a warning message is written to the log if the
// property is not of string type
// -----------------------------------------------------------------------------
wxString Property::stringValue(bool warn_wrong_type) const
{
	// If this is a flag, just return boolean 'true' (or equivalent)
	if (type_ == Type::Flag)
		return "1";

	// If the value is undefined, default to null
	if (!has_value_)
		return "";

	// Write warning to log if needed
	if (warn_wrong_type && type_ != Type::String)
		Log::warning(S_FMT("Warning: Requested String value of a %s Property", typeString()));

	// Return value (convert if needed)
	if (type_ == Type::String)
		return val_string_;
	else if (type_ == Type::Int)
		return S_FMT("%d", value_.Integer);
	else if (type_ == Type::UInt)
		return S_FMT("%u", value_.Unsigned);
	else if (type_ == Type::Boolean)
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
		Log::warning(S_FMT("Requested Integer value of a %s Property", typeString()));

	// Return value (convert if needed)
	if (type_ == Type::Int)
		return value_.Integer;
	else if (type_ == Type::Boolean)
		return (int)value_.Boolean;
	else if (type_ == Type::Float)
		return (int)value_.Floating;
	else if (type_ == Type::String)
	{
		unsigned long tmp;
		if (val_string_.ToULong(&tmp))
			return tmp;
		return 0;
	}
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
void Property::setValue(const wxString& val)
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
	if (type_ == Type::Boolean)
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
wxString Property::typeString() const
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
