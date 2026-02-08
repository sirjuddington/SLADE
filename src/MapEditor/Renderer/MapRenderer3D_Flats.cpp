
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapRenderer3D_Flats.cpp
// Description: MapRenderer3D class - sector flat related functions
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
#include "App.h"
#include "Flat3D.h"
#include "MapEditor/Item.h"
#include "MapGeometry.h"
#include "MapGeometryBuffer3D.h"
#include "MapRenderer3D.h"
#include "OpenGL/IndexBuffer.h"
#include "OpenGL/LineBuffer.h"
#include "Renderer.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapSide.h"
#include "SLADEMap/MapObjectList/SectorList.h"
#include "SLADEMap/MapSpecials/MapSpecials.h"
#include "SLADEMap/SLADEMap.h"
#include "SLADEMap/Types.h"

using namespace slade;
using namespace mapeditor;


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Comparison operator for Item vs Flat3D.
// Used to find the correct sector flat an item represents
// -----------------------------------------------------------------------------
bool operator==(const Item& item, const Flat3D& flat)
{
	return flat.control_sector->index() == item.index
		   && (item.type == ItemType::Floor && flat.control_surface_type == Flat3D::SurfaceType::Floor
			   || item.type == ItemType::Ceiling && flat.control_surface_type == Flat3D::SurfaceType::Ceiling);
}

// -----------------------------------------------------------------------------
// Checks if flats need to be updated based on [last_updated] time and related
// map update times
// -----------------------------------------------------------------------------
bool flatsNeedUpdate(long last_updated, const SLADEMap* map)
{
	return last_updated < map->typeLastUpdated(map::ObjectType::Sector)
		   || last_updated < map->mapSpecials().specialsLastUpdated() || last_updated < map->sectorRenderInfoUpdated();
}
} // namespace


// -----------------------------------------------------------------------------
//
// MapRenderer3D Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Updates sector flats/geometry and render groups if needed
// -----------------------------------------------------------------------------
void MapRenderer3D::updateFlats()
{
	using FlatFlags = Flat3D::Flags;

	// Clear flats to be rebuilt if map geometry has been updated
	if (map_->geometryUpdated() > flats_updated_)
	{
		vb_flats_->buffer().clear();
		sector_flats_.clear();
		flat_groups_.clear();
		renderer_->clearAnimations();
	}

	// Generate flats if needed
	if (sector_flats_.empty())
	{
		unsigned vertex_index = 0;
		for (auto* sector : map_->sectors())
		{
			auto [flats, vertices] = generateSectorFlats(*sector, vertex_index);

			sector_flats_.push_back(
				{ .sector               = sector,
				  .flats                = flats,
				  .vertex_buffer_offset = vertex_index,
				  .updated_time         = app::runTimer() });

			vb_flats_->addVertices(vertices);
			vertex_index += vertices.size();
		}

		vb_flats_->push();
		flats_updated_ = app::runTimer();
		flat_groups_.clear();
	}
	else if (flatsNeedUpdate(flats_updated_, map_))
	{
		map_->mapSpecials().updateSpecials();

		// Check for sectors that need an update
		bool updated = false;
		for (auto& sf : sector_flats_)
		{
			if (sf.updated_time >= sf.sector->modifiedTime() && sf.updated_time >= sf.sector->renderInfoLastUpdated())
				continue;

			// Build new flats/vertices
			auto flats_count               = sf.flats.size();
			auto [new_flats, new_vertices] = generateSectorFlats(*sf.sector, sf.vertex_buffer_offset);
			sf.flats                       = new_flats;

			// Update vertex buffer
			if (new_flats.size() <= flats_count)
			{
				// Same or fewer flats, just update existing vertex data
				vb_flats_->buffer().update(sf.vertex_buffer_offset, new_vertices);
			}
			else
			{
				// More flats than before, need to re-upload entire buffer

				// TODO: This will result in gaps in the buffer, either compact
				//       the buffer here or implement some way to track and
				//       reuse freed space later

				sf.vertex_buffer_offset = vb_flats_->buffer().size();
				vb_flats_->pull();                    // Pull data from GPU
				vb_flats_->addVertices(new_vertices); // Add new vertex data
				vb_flats_->push();                    // Push data back to GPU
			}

			// Set updated
			updated         = true;
			sf.updated_time = app::runTimer();
		}

		// Clear flat groups to be rebuilt if any flats were updated
		if (updated)
		{
			flats_updated_ = app::runTimer();
			flat_groups_.clear();
			renderer_->clearAnimations();
		}
	}

	// Generate flat groups if needed
	if (flat_groups_.empty())
	{
		// Build list of flats to process
		struct FlatToProcess
		{
			Flat3D* flat;
			bool    processed;
		};
		vector<FlatToProcess> flats_to_process;
		for (auto& sf : sector_flats_)
			for (auto& flat : sf.flats)
				flats_to_process.push_back({ .flat = &flat, .processed = false });

		for (unsigned i1 = 0; i1 < flats_to_process.size(); ++i1)
		{
			auto& fp1 = flats_to_process[i1];

			if (fp1.processed)
				continue;

			// Build list of vertex indices for flats with matching texture/flags/etc
			vector<GLuint> indices;
			for (unsigned i2 = i1; i2 < flats_to_process.size(); ++i2)
			{
				auto& fp2 = flats_to_process[i2];

				// Ignore if already processed or not matching
				if (fp2.processed || *fp1.flat != *fp2.flat)
					continue;

				// Add indices
				auto vi = fp2.flat->vertex_offset;
				while (vi < fp2.flat->vertex_offset + fp2.flat->sector->polygonVertices().size())
					indices.push_back(vi++);

				fp2.processed = true;
			}

			// Add flat group for texture
			flat_groups_.push_back(
				{ .texture        = fp1.flat->texture,
				  .colour         = fp1.flat->colour,
				  .index_buffer   = std::make_unique<gl::IndexBuffer>(),
				  .render_pass    = fp1.flat->render_pass,
				  .trans_additive = fp1.flat->hasFlag(FlatFlags::Additive) });
			flat_groups_.back().index_buffer->upload(indices);

			fp1.processed = true;
		}
	}
}

// -----------------------------------------------------------------------------
// Adds an outline for a sector floor/ceiling [item] to the given line [buffer].
// -----------------------------------------------------------------------------
void MapRenderer3D::addFlatOutline(const Item& item, gl::LineBuffer& buffer, float line_width) const
{
	auto sector = item.asSector(*map_);
	if (!sector)
		return;

	Plane plane;
	switch (item.type)
	{
	case ItemType::Ceiling: plane = sector->ceiling().plane; break;
	case ItemType::Floor:   plane = sector->floor().plane; break;
	default:                return;
	}

	auto colour = glm::vec4{ 1.0f };

	for (auto side : item.realSector(*map_)->connectedSides())
		buffer.add3d(side->parentLine()->start(), side->parentLine()->end(), plane, colour, line_width);
}

// -----------------------------------------------------------------------------
// Adds vertex indices for the flat representing [item] to the given list of
// [indices].
// -----------------------------------------------------------------------------
void MapRenderer3D::addItemFlatIndices(const Item& item, vector<GLuint>& indices) const
{
	auto real_index = item.real_index >= 0 ? item.real_index : item.index;
	if (real_index < 0 || real_index >= sector_flats_.size())
		return;

	// Find the sector flat for the item
	for (const auto& flat : sector_flats_[real_index].flats)
	{
		if (flat == item)
		{
			// Add vertex indices for this flat
			auto vi = flat.vertex_offset;
			while (vi < flat.vertex_offset + flat.sector->polygonVertices().size())
				indices.push_back(vi++);
			break;
		}
	}
}
