
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MobjPropertyList.cpp
// Description: A special version of the PropertyList class that uses a vector
//              rather than a map to store properties
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
#include "MobjPropertyList.h"
#include "Utility/StringUtils.h"
#include "thirdparty/fmt/fmt/color.h"


// -----------------------------------------------------------------------------
//
// MobjPropertyList Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns true if a property with the given name exists, false otherwise
// -----------------------------------------------------------------------------
bool MobjPropertyList::propertyExists(string_view key)
{
	for (const auto& prop : properties_)
		if (prop.name == key)
			return true;

	return false;
}

// -----------------------------------------------------------------------------
// Removes a property value, returns true if [key] was removed or false if key
// didn't exist
// -----------------------------------------------------------------------------
bool MobjPropertyList::removeProperty(string_view key)
{
	for (auto& prop : properties_)
	{
		if (prop.name == key)
		{
			prop = properties_.back();
			properties_.pop_back();
			return true;
		}
	}

	return false;
}

// -----------------------------------------------------------------------------
// Copies all properties to [list]
// -----------------------------------------------------------------------------
void MobjPropertyList::copyTo(MobjPropertyList& list)
{
	// Clear given list
	list.clear();

	for (auto& prop : properties_)
		list.properties_.emplace_back(prop.name, prop.value);
}

// -----------------------------------------------------------------------------
// Adds a 'flag' property [key]
// -----------------------------------------------------------------------------
void MobjPropertyList::addFlag(string_view key)
{
	Property flag;
	properties_.emplace_back(key, flag);
}

// -----------------------------------------------------------------------------
// Returns a string representation of the property list
// -----------------------------------------------------------------------------
string MobjPropertyList::toString(bool condensed)
{
	static string format_normal           = "{} = {};\n";
	static string format_normal_string    = "{} = \"{}\"\n";
	static string format_condensed        = "{}={};\n";
	static string format_condensed_string = "{}=\"{}\"\n";

	// Get formatter strings to use
	const auto& format        = condensed ? format_condensed : format_normal;
	const auto& format_string = condensed ? format_condensed_string : format_normal_string;

	// Write properties to string buffer
	fmt::memory_buffer buffer;
	for (const auto& prop : properties_)
	{
		// Skip if no value
		if (!prop.value.hasValue())
			continue;

		// Add "key = value;\n" to the buffer
		if (prop.value.type() == Property::Type::String)
			fmt::format_to(buffer, format_string, prop.name, StrUtil::escapedString(prop.value.stringValue()));
		else
			fmt::format_to(buffer, format, prop.name, prop.value.stringValue());
	}

	return fmt::to_string(buffer);
}
