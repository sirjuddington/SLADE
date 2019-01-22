
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
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


// -----------------------------------------------------------------------------
//
// PropertyList Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns true if a property with the given name exists, false otherwise
// -----------------------------------------------------------------------------
bool PropertyList::propertyExists(const string& key)
{
	// Try to find specified key
	return !(properties_.empty() || properties_.find(key) == properties_.end());
}

// -----------------------------------------------------------------------------
// Removes a property value, returns true if [key] was removed or false if key
// didn't exist
// -----------------------------------------------------------------------------
bool PropertyList::removeProperty(const string& key)
{
	return properties_.erase(key) > 0;
}

// -----------------------------------------------------------------------------
// Copies all properties to [list]
// -----------------------------------------------------------------------------
void PropertyList::copyTo(PropertyList& list)
{
	// Clear given list
	list.clear();

	// Copy
	for (auto& i : properties_)
		if (i.second.hasValue())
			list[i.first] = i.second;
}

// -----------------------------------------------------------------------------
// Adds a 'flag' property [key]
// -----------------------------------------------------------------------------
void PropertyList::addFlag(const string& key)
{
	Property flag;
	properties_[key] = flag;
}

// -----------------------------------------------------------------------------
// Returns a string representation of the property list
// -----------------------------------------------------------------------------
string PropertyList::toString(bool condensed) const
{
	// Init return string
	string ret = wxEmptyString;

	// Go through all properties
	for (auto& i : properties_)
	{
		// Skip if no value
		if (!i.second.hasValue())
			continue;

		// Add "key = value;\n" to the return string
		string key = i.first;
		string val = i.second.stringValue();

		if (i.second.type() == Property::Type::String)
			val = "\"" + val + "\"";

		if (condensed)
			ret += key + "=" + val + ";\n";
		else
			ret += key + " = " + val + ";\n";
	}

	return ret;
}

// -----------------------------------------------------------------------------
// Adds all existing properties to [list]
// -----------------------------------------------------------------------------
void PropertyList::allProperties(vector<Property>& list, bool ignore_no_value)
{
	// Add all properties to the list
	for (auto& i : properties_)
		if (!(ignore_no_value && !i.second.hasValue()))
			list.push_back(i.second);
}

// -----------------------------------------------------------------------------
// Adds all existing property names to [list]
// -----------------------------------------------------------------------------
void PropertyList::allPropertyNames(vector<string>& list, bool ignore_no_value)
{
	// Add all properties to the list
	for (auto& i : properties_)
		if (!(ignore_no_value && !i.second.hasValue()))
			list.push_back(i.first);
}
