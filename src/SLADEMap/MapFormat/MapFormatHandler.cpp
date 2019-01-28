
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapFormatHandler.cpp
// Description: The base class for map format handlers. A map format handler
//              must implement read/write functions for the particular map
//              format it handles.
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
#include "Doom64MapFormat.h"
#include "DoomMapFormat.h"
#include "HexenMapFormat.h"
#include "UniversalDoomMapFormat.h"


// -----------------------------------------------------------------------------
// NoMapFormat Class
//
// A 'dummy' map format class for unknown formats that always fails load & save
// -----------------------------------------------------------------------------
class NoMapFormat : public MapFormatHandler
{
public:
	bool readMap(Archive::MapDesc map, MapObjectCollection& map_data, PropertyList& map_extra_props) override
	{
		return false;
	}

	vector<ArchiveEntry::UPtr> writeMap(const MapObjectCollection& map_data, const PropertyList& map_extra_props)
		override
	{
		return {};
	}

	string udmfNamespace() override { return ""; }
};


// -----------------------------------------------------------------------------
//
// MapFormatHandler Class Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns an appropriate map format handler for the given map [format]
// -----------------------------------------------------------------------------
MapFormatHandler::UPtr MapFormatHandler::get(MapFormat format)
{
	switch (format)
	{
	case MapFormat::Doom: return std::make_unique<DoomMapFormat>();
	case MapFormat::Hexen: return std::make_unique<HexenMapFormat>();
	case MapFormat::UDMF: return std ::make_unique<UniversalDoomMapFormat>();
	case MapFormat::Doom64: return std::make_unique<Doom64MapFormat>();
	default: return std::make_unique<NoMapFormat>();
	}
}
