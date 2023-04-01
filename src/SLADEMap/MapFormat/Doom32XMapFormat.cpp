
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd, Victor Luchits
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Doom32XMapFormat.cpp
// Description: MapFormatHandler specialization to handle Doom32X format maps
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
#include "Doom32XMapFormat.h"
#include "General/UI.h"
#include "SLADEMap/MapObject/MapVertex.h"
#include "SLADEMap/MapObjectCollection.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Doom32XMapFormat Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Reads Doom32X-format VERTEXES data from [entry] into [map_data]
// -----------------------------------------------------------------------------
bool Doom32XMapFormat::readVERTEXES(ArchiveEntry* entry, MapObjectCollection& map_data) const
{
	if (!entry)
	{
		global::error = "Map has no VERTEXES entry!";
		log::info(global::error);
		return false;
	}

	// Check for empty entry
	if (entry->size() < sizeof(Vertex32BE))
	{
		log::info(3, "Read 0 vertices");
		return true;
	}

	auto     vert_data = reinterpret_cast<const Vertex32BE*>(entry->rawData());
	unsigned nv        = entry->size() / sizeof(Vertex32BE);
	float    p         = ui::getSplashProgress();
	for (size_t a = 0; a < nv; a++)
	{
		ui::setSplashProgress(p + (static_cast<float>(a) / nv) * 0.2f);
		map_data.addVertex(
			std::make_unique<MapVertex>(Vec2d{ static_cast<double>(wxUINT32_SWAP_ON_LE(vert_data[a].x)) / 65536.0,
											   static_cast<double>(wxUINT32_SWAP_ON_LE(vert_data[a].y)) / 65536.0 }));
	}

	log::info(3, "Read {} vertices", map_data.vertices().size());

	return true;
}
