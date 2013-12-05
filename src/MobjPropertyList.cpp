/*******************************************************************
* SLADE - It's a Doom Editor
* Copyright (C) 2008-2012 Simon Judd
*
* Email:       sirjuddington@gmail.com
* Web:         http://slade.mancubus.net
* Filename:    MobjPropertyList.cpp
* Description: A special version of the PropertyList class that
*              uses a vector rather than a map to store properties
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
#include "MobjPropertyList.h"


/*******************************************************************
* MOBJPROPERTYLIST CLASS FUNCTIONS
*******************************************************************/

/* MobjPropertyList::MobjPropertyList
* MobjPropertyList class constructor
*******************************************************************/
MobjPropertyList::MobjPropertyList()
{
}

/* MobjPropertyList::~MobjPropertyList
* MobjPropertyList class destructor
*******************************************************************/
MobjPropertyList::~MobjPropertyList()
{
}

/* MobjPropertyList::propertyExists
* Returns true if a property with the given name exists, false
* otherwise
*******************************************************************/
bool MobjPropertyList::propertyExists(string key)
{
	for (unsigned a = 0; a < properties.size(); ++a)
	{
		if (properties[a].name == key)
			return true;
	}

	return false;
}

/* MobjPropertyList::removeProperty
* Removes a property value, returns true if [key] was removed
* or false if key didn't exist
*******************************************************************/
bool MobjPropertyList::removeProperty(string key)
{
	//return properties.erase(key) > 0;
	for (unsigned a = 0; a < properties.size(); ++a)
	{
		if (properties[a].name == key)
		{
			properties[a] = properties.back();
			properties.pop_back();
			return true;
		}
	}

	return false;
}

/* MobjPropertyList::copyTo
* Copies all properties to [list]
*******************************************************************/
void MobjPropertyList::copyTo(MobjPropertyList& list)
{
	// Clear given list
	list.clear();

	for (unsigned a = 0; a < properties.size(); ++a)
		list.properties.push_back(prop_t(properties[a].name, properties[a].value));
}

/* MobjPropertyList::addFlag
* Adds a 'flag' property [key]
*******************************************************************/
void MobjPropertyList::addFlag(string key)
{
	Property flag;
	properties.push_back(prop_t(key, flag));
}

/* MobjPropertyList::toString
* Returns a string representation of the property list
*******************************************************************/
string MobjPropertyList::toString(bool condensed)
{
	// Init return string
	string ret = wxEmptyString;

	for (unsigned a = 0; a < properties.size(); ++a)
	{
		// Skip if no value
		if (!properties[a].value.hasValue())
			continue;

		// Add "key = value;\n" to the return string
		string key = properties[a].name;
		string val = properties[a].value.getStringValue();

		if (properties[a].value.getType() == PROP_STRING)
			val = "\"" + val + "\"";

		if (condensed)
			ret += key + "=" + val + ";\n";
		else
			ret += key + " = " + val + ";\n";
	}

	return ret;
}

/* MobjPropertyList::allProperties
* Adds all existing properties to [list]
*******************************************************************/
void MobjPropertyList::allProperties(vector<Property>& list)
{
	// Add all properties to the list
	for (unsigned a = 0; a < properties.size(); ++a)
		list.push_back(properties[a].value);
}

/* MobjPropertyList::allPropertyNames
* Adds all existing property names to [list]
*******************************************************************/
void MobjPropertyList::allPropertyNames(vector<string>& list)
{
	// Add all properties to the list
	for (unsigned a = 0; a < properties.size(); ++a)
		list.push_back(properties[a].name);
}
