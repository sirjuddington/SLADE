
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    PropertyList.cpp
// Description: PropertyList class - a simple list of named Property values with
//              various utility functions
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
#include "PropertyUtils.h"
#include "StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// PropertyList Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// [] operator to get an existing property [key].
// If no property with the key exists, one is added and returned
// -----------------------------------------------------------------------------
Property& PropertyList::operator[](string_view key)
{
	for (auto& prop : properties_)
		if (strutil::equalCI(key, prop.name))
			return prop.value;

	properties_.emplace_back(key, Property{});
	return properties_.back().value;
}

// -----------------------------------------------------------------------------
// Returns true if the list contains a property named [key]
// -----------------------------------------------------------------------------
bool PropertyList::contains(string_view key) const
{
	for (const auto& prop : properties_)
		if (strutil::equalCI(key, prop.name))
			return true;

	return false;
}

// -----------------------------------------------------------------------------
// Returns the property matching [key] if it exists, or an empty std::optional
// if not
// -----------------------------------------------------------------------------
std::optional<Property> PropertyList::getIf(string_view key) const
{
	for (const auto& prop : properties_)
		if (strutil::equalCI(key, prop.name))
			return prop.value;

	return {};
}

// -----------------------------------------------------------------------------
// Adds all properties to [list]
// -----------------------------------------------------------------------------
void PropertyList::allProperties(vector<Property>& list) const
{
	for (const auto& prop : properties_)
		list.push_back(prop.value);
}

// -----------------------------------------------------------------------------
// Adds all property names to [list]
// -----------------------------------------------------------------------------
void PropertyList::allPropertyNames(vector<string>& list) const
{
	for (const auto& prop : properties_)
		list.push_back(prop.name);
}

// -----------------------------------------------------------------------------
// Clears all properties in the list
// -----------------------------------------------------------------------------
void PropertyList::clear()
{
	properties_.clear();
}

// -----------------------------------------------------------------------------
// Removes the property matching [key].
// Returns true if the property existed and was removed, false otherwise
// -----------------------------------------------------------------------------
bool PropertyList::remove(string_view key)
{
	const auto count = properties_.size();
	for (unsigned i = 0; i < count; ++i)
		if (strutil::equalCI(key, properties_[i].name))
		{
			properties_.erase(properties_.begin() + i);
			return true;
		}

	return false;
}

// -----------------------------------------------------------------------------
// Returns a string representation of the property list
// -----------------------------------------------------------------------------
string PropertyList::toString(bool condensed, int float_precision) const
{
	// Init return string
	string ret;

	// Go through all properties
	for (const auto& prop : properties_)
	{
		// Add "key = value;\n" to the return string
		auto val = property::asString(prop.value, float_precision);

		if (property::valueType(prop.value) == property::ValueType::String)
		{
			val = strutil::escapedString(val, false, true);
			val.insert(val.begin(), '\"');
			val.push_back('\"');
		}

		if (condensed)
			ret += fmt::format("{}={};\n", prop.name, val);
		else
			ret += fmt::format("{} = {};\n", prop.name, val);
	}

	return ret;
}
