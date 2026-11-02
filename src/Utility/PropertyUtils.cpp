
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    PropertyUtil.cpp
// Description: Property system utils - a Property is just a dynamic type
//              (std::variant) that can contain a boolean, int, unsigned int,
//              float or string value.
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
#include "StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Property namespace functions
//
// -----------------------------------------------------------------------------
namespace slade::property
{
// -----------------------------------------------------------------------------
// Returns [prop] as a boolean value
// -----------------------------------------------------------------------------
bool asBool(const Property& prop)
{
	switch (prop.index())
	{
	case 0:  return std::get<bool>(prop);
	case 1:  return std::get<int>(prop) != 0;
	case 2:  return std::get<unsigned int>(prop) != 0;
	case 3:  return std::get<double>(prop) != 0.;
	case 4:  return strutil::asBoolean(std::get<string>(prop));
	default: return false;
	}
}

// -----------------------------------------------------------------------------
// Returns [prop] as an integer value
// -----------------------------------------------------------------------------
int asInt(const Property& prop)
{
	switch (prop.index())
	{
	case 0:  return std::get<bool>(prop) ? 1 : 0;
	case 1:  return std::get<int>(prop);
	case 2:  return static_cast<int>(std::get<unsigned int>(prop));
	case 3:  return static_cast<int>(std::get<double>(prop));
	case 4:  return strutil::asInt(std::get<string>(prop));
	default: return 0;
	}
}

// -----------------------------------------------------------------------------
// Returns [prop] as an unsigned integer value
// -----------------------------------------------------------------------------
unsigned asUInt(const Property& prop)
{
	switch (prop.index())
	{
	case 0:  return std::get<bool>(prop) ? 1 : 0;
	case 1:  return static_cast<unsigned int>(std::get<int>(prop));
	case 2:  return std::get<unsigned int>(prop);
	case 3:  return static_cast<unsigned int>(std::get<double>(prop));
	case 4:  return strutil::asUInt(std::get<string>(prop));
	default: return 0;
	}
}

// -----------------------------------------------------------------------------
// Returns [prop] as a float value
// -----------------------------------------------------------------------------
double asFloat(const Property& prop)
{
	switch (prop.index())
	{
	case 0:  return std::get<bool>(prop) ? 1. : 0.;
	case 1:  return std::get<int>(prop);
	case 2:  return std::get<unsigned int>(prop);
	case 3:  return std::get<double>(prop);
	case 4:  return strutil::asDouble(std::get<string>(prop));
	default: return 0.;
	}
}

// -----------------------------------------------------------------------------
// Returns [prop] as a string.
// If [prop] is a float value, use [float_precision] decimal points
// -----------------------------------------------------------------------------
string asString(const Property& prop, int float_precision)
{
	switch (prop.index())
	{
	case 0: return std::get<bool>(prop) ? "true" : "false";
	case 1: return fmt::format("{}", std::get<int>(prop));
	case 2: return fmt::format("{}", std::get<unsigned int>(prop));
	case 3:
	{
		if (float_precision <= 0)
			return fmt::format("{}", std::get<double>(prop));
		else
			return fmt::format("{:.{}f}", std::get<double>(prop), float_precision);
	}
	case 4:  return std::get<string>(prop);
	default: return {};
	}
}
} // namespace slade::property
