
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapFormat.cpp
// Description: MapFormat enum utility functions
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
#include "MapFormat.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns a MapFormat enum value for the given string [id] (case-insensitive)
// -----------------------------------------------------------------------------
MapFormat slade::mapFormatFromId(string_view id)
{
	if (strutil::equalCI(id, "doom"))
		return MapFormat::Doom;
	if (strutil::equalCI(id, "hexen"))
		return MapFormat::Hexen;
	if (strutil::equalCI(id, "doom64"))
		return MapFormat::Doom64;
	if (strutil::equalCI(id, "doom32x"))
		return MapFormat::Doom32X;
	if (strutil::equalCI(id, "udmf"))
		return MapFormat::UDMF;

	return MapFormat::Unknown;
}

// -----------------------------------------------------------------------------
// Returns the string id of the given [format] MapFormat enum value
// -----------------------------------------------------------------------------
string slade::mapFormatId(MapFormat format)
{
	switch (format)
	{
	case MapFormat::Doom:    return "doom";
	case MapFormat::Hexen:   return "hexen";
	case MapFormat::Doom64:  return "doom64";
	case MapFormat::Doom32X: return "doom32x";
	case MapFormat::UDMF:    return "udmf";
	default:                 return "unknown";
	}
}
