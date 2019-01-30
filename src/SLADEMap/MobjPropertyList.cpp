
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


// -----------------------------------------------------------------------------
//
// MobjPropertyList Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns true if a property with the given name exists, false otherwise
// -----------------------------------------------------------------------------
bool MobjPropertyList::propertyExists(const string& key)
{
	for (unsigned a = 0; a < properties_.size(); ++a)
	{
		if (properties_[a].name == key)
			return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Removes a property value, returns true if [key] was removed or false if key
// didn't exist
// -----------------------------------------------------------------------------
bool MobjPropertyList::removeProperty(string key)
{
	// return properties.erase(key) > 0;
	for (unsigned a = 0; a < properties_.size(); ++a)
	{
		if (properties_[a].name == key)
		{
			properties_[a] = properties_.back();
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

	for (unsigned a = 0; a < properties_.size(); ++a)
		list.properties_.push_back(Prop(properties_[a].name, properties_[a].value));
}

// -----------------------------------------------------------------------------
// Adds a 'flag' property [key]
// -----------------------------------------------------------------------------
void MobjPropertyList::addFlag(string key)
{
	Property flag;
	properties_.push_back(Prop(key, flag));
}

// -----------------------------------------------------------------------------
// Returns a string representation of the property list
// -----------------------------------------------------------------------------
string MobjPropertyList::toString(bool condensed)
{
	// Init return string
	string ret = wxEmptyString;

	for (unsigned a = 0; a < properties_.size(); ++a)
	{
		// Skip if no value
		if (!properties_[a].value.hasValue())
			continue;

		// Add "key = value;\n" to the return string
		string key = properties_[a].name;
		string val = properties_[a].value.stringValue();

		if (properties_[a].value.type() == Property::Type::String)
		{
			val = StringUtils::escapedString(val);
			val = "\"" + val + "\"";
		}

		if (condensed)
			ret += key + "=" + val + ";\n";
		else
			ret += key + " = " + val + ";\n";
	}

	return ret;
}
