
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    PropertyList.cpp
 * Description: The PropertyList class. Contains a hash map with
 *              strings for keys and the 'Property' dynamic value
 *              class for values. Each property value can be a
 *              bool, int, double or string.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "PropertyList.h"


/*******************************************************************
 * PROPERTYLIST CLASS FUNCTIONS
 *******************************************************************/

/* PropertyList::PropertyList
 * PropertyList class constructor
 *******************************************************************/
PropertyList::PropertyList()
{
}

/* PropertyList::~PropertyList
 * PropertyList class destructor
 *******************************************************************/
PropertyList::~PropertyList()
{
}

/* PropertyList::propertyExists
 * Returns true if a property with the given name exists, false
 * otherwise
 *******************************************************************/
bool PropertyList::propertyExists(string key)
{
	// Try to find specified key
	if (properties.empty() || properties.find(key) == properties.end())
		return false;
	else
		return true;
}

/* PropertyList::removeProperty
 * Removes a property value, returns true if [key] was removed
 * or false if key didn't exist
 *******************************************************************/
bool PropertyList::removeProperty(string key)
{
	return properties.erase(key) > 0;
}

/* PropertyList::copyTo
 * Copies all properties to [list]
 *******************************************************************/
void PropertyList::copyTo(PropertyList& list)
{
	// Clear given list
	list.clear();

	// Get iterator to first property
	std::map<string, Property>::iterator i = properties.begin();

	// Add all properties to given list
	while (i != properties.end())
	{
		if (i->second.hasValue())
			list[i->first] = i->second;
		i++;
	}
}

/* PropertyList::addFlag
 * Adds a 'flag' property [key]
 *******************************************************************/
void PropertyList::addFlag(string key)
{
	Property flag;
	properties[key] = flag;
}

/* PropertyList::toString
 * Returns a string representation of the property list
 *******************************************************************/
string PropertyList::toString(bool condensed)
{
	// Init return string
	string ret = wxEmptyString;

	// Get iterator to first property
	std::map<string, Property>::iterator i = properties.begin();

	// Go through all properties
	while (i != properties.end())
	{
		// Skip if no value
		if (!i->second.hasValue())
		{
			i++;
			continue;
		}

		// Add "key = value;\n" to the return string
		string key = i->first;
		string val = i->second.getStringValue();

		if (i->second.getType() == PROP_STRING)
			val = "\"" + val + "\"";

		//if (!val.empty()) {
		if (condensed)
			ret += key + "=" + val + ";\n";
		else
			ret += key + " = " + val + ";\n";
		//}

		//LOG_MESSAGE(1, "key %s type %s value %s", key, i->second.typeString(), val);

		// Next property
		i++;
	}

	return ret;
}

/* PropertyList::allProperties
 * Adds all existing properties to [list]
 *******************************************************************/
void PropertyList::allProperties(vector<Property>& list)
{
	// Get iterator to first property
	std::map<string, Property>::iterator i = properties.begin();

	// Add all properties to the list
	while (i != properties.end())
	{
		list.push_back(i->second);
		i++;
	}
}

/* PropertyList::allPropertyNames
 * Adds all existing property names to [list]
 *******************************************************************/
void PropertyList::allPropertyNames(vector<string>& list)
{
	// Get iterator to first property
	std::map<string, Property>::iterator i = properties.begin();

	// Add all properties to the list
	while (i != properties.end())
	{
		list.push_back(i->first);
		i++;
	}
}
