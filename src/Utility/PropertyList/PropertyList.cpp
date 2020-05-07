
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    PropertyList.cpp
// Description: The PropertyList class. Contains a hash map with strings for
//              keys and the 'Property' dynamic value class for values.
//              Each property value can be a bool, int, double or string.
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
#include "PropertyList.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// PropertyList Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns true if a property with the given key exists, false otherwise
// -----------------------------------------------------------------------------
bool PropertyList::propertyExists(string_view key) const
{
	// Try to find specified key
	for (const auto& p : properties_)
		if (p.key == key)
			return true;

	return false;
}

// -----------------------------------------------------------------------------
// Removes a property value, returns true if [key] was removed or false if key
// didn't exist
// -----------------------------------------------------------------------------
bool PropertyList::removeProperty(string_view key)
{
	for (auto i = properties_.begin(); i != properties_.end(); ++i)
	{
		if (i->key == key)
		{
			properties_.erase(i);
			return true;
		}
	}

	return false;
}

// -----------------------------------------------------------------------------
// Copies all properties to [list]
// -----------------------------------------------------------------------------
void PropertyList::copyTo(PropertyList& list) const
{
	// Clear given list
	list.clear();

	// Copy
	for (const auto& i : properties_)
		if (i.prop.hasValue())
			list.properties_.emplace_back(i);
}

// -----------------------------------------------------------------------------
// Returns a string representation of the property list
// -----------------------------------------------------------------------------
string PropertyList::toString(bool condensed) const
{
	// Init return string
	string ret;

	// Go through all properties
	for (const auto& i : properties_)
	{
		// Skip if no value
		if (!i.prop.hasValue())
			continue;

		// Add "key = value;\n" to the return string
		auto key = i.key;
		auto val = i.prop.stringValue();

		if (i.prop.type() == Property::Type::String)
		{
			val.insert(val.begin(), '\"');
			val.push_back('\"');
		}

		if (condensed)
			ret += fmt::format("{}={};\n", key, val);
		else
			ret += fmt::format("{} = {};\n", key, val);
	}

	return ret;
}

// -----------------------------------------------------------------------------
// Adds all existing properties to [list]
// -----------------------------------------------------------------------------
void PropertyList::allProperties(vector<Property>& list, bool ignore_no_value) const
{
	// Add all properties to the list
	for (const auto& i : properties_)
		if (!(ignore_no_value && !i.prop.hasValue()))
			list.push_back(i.prop);
}

// -----------------------------------------------------------------------------
// Adds all existing property names to [list]
// -----------------------------------------------------------------------------
void PropertyList::allPropertyNames(vector<string>& list, bool ignore_no_value) const
{
	// Add all properties to the list
	for (const auto& i : properties_)
		if (!(ignore_no_value && !i.prop.hasValue()))
			list.push_back(i.key);
}
