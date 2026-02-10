
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapRenderer3D_Walls.cpp
// Description: MapRenderer3D class - wall/quad related functions
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
#include "MapEditor/Item.h"
#include "MapGeometry.h"
#include "MapGeometryBuffer3D.h"
#include "MapRenderer3D.h"
#include "OpenGL/IndexBuffer.h"
#include "OpenGL/LineBuffer.h"
#include "Quad3D.h"
#include "Renderer.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapSide.h"
#include "SLADEMap/MapObjectList/LineList.h"
#include "SLADEMap/MapSpecials/MapSpecials.h"
#include "SLADEMap/SLADEMap.h"

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
// Comparison operator for Item vs Quad3D.
// Used to find the correct quads an item represents
// -----------------------------------------------------------------------------
bool operator==(const Item& item, const Quad3D& quad)
{
	if (quad.side->index() == item.index)
	{
		if (quad.part == map::SidePart::Upper && item.type == ItemType::WallTop
			|| quad.part == map::SidePart::Middle && item.type == ItemType::WallMiddle
			|| quad.part == map::SidePart::Lower && item.type == ItemType::WallBottom)
		{
			return true;
		}
	}

	return false;
}

// -----------------------------------------------------------------------------
// Checks if any wall quads need to be updated based on [last_updated] time and
// related map geometry/specials update times
// -----------------------------------------------------------------------------
bool quadsNeedUpdate(long last_updated, const SLADEMap* map)
{
	return last_updated < map->typeLastUpdated(map::ObjectType::Line)
		   || last_updated < map->typeLastUpdated(map::ObjectType::Side)
		   || last_updated < map->typeLastUpdated(map::ObjectType::Sector)
		   || last_updated < map->mapSpecials().specialsLastUpdated()
		   || last_updated < map->sectorRenderInfoUpdated(); // ExtraFloors may affect wall quads
}

// -----------------------------------------------------------------------------
// Checks if [line]'s quads need to be updated based on [last_updated] time and
// related line update times
// -----------------------------------------------------------------------------
bool lineNeedsUpdate(long last_updated, const MapLine* line)
{
	if (last_updated < line->modifiedTime())
		return true;

	// Check sides
	if (line->s1())
	{
		if (last_updated < line->s1()->modifiedTime() || last_updated < line->s1()->sector()->modifiedTime()
			|| last_updated < line->s1()->sector()->renderInfoLastUpdated())
			return true;
	}
	if (line->s2())
	{
		if (last_updated < line->s2()->modifiedTime() || last_updated < line->s2()->sector()->modifiedTime()
			|| last_updated < line->s2()->sector()->renderInfoLastUpdated())
			return true;
	}

	return false;
}
} // namespace


// -----------------------------------------------------------------------------
//
// MapRenderer3D Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Updates wall quads and render groups as needed
// -----------------------------------------------------------------------------
void MapRenderer3D::updateWalls()
{
	using QuadFlags = Quad3D::Flags;

	// Clear walls to be rebuilt if map geometry has been updated
	if (map_->geometryUpdated() > quads_updated_)
	{
		vb_quads_->buffer().clear();
		line_quads_.clear();
		quad_groups_.clear();
		renderer_->clearAnimations();
	}

	// Generate wall quads if needed
	if (line_quads_.empty())
	{
		unsigned vertex_index = 0;
		for (auto* line : map_->lines())
		{
			auto [quads, vertices] = generateLineQuads(*line, vertex_index);

			line_quads_.push_back(
				{ .line                 = line,
				  .quads                = quads,
				  .vertex_buffer_offset = vertex_index,
				  .updated_time         = app::runTimer() });

			vb_quads_->addVertices(vertices);
			vertex_index += vertices.size();
		}
		vb_quads_->push();
		quads_updated_ = app::runTimer();
	}
	else if (quadsNeedUpdate(quads_updated_, map_))
	{
		map_->mapSpecials().updateSpecials();

		// Check for lines that need an update
		bool updated = false;
		for (auto& lq : line_quads_)
		{
			if (!lineNeedsUpdate(lq.updated_time, lq.line))
				continue;

			// Build new quads/vertices
			auto quads_count               = lq.quads.size();
			auto [new_quads, new_vertices] = generateLineQuads(*lq.line, lq.vertex_buffer_offset);
			lq.quads                       = new_quads;

			// Update vertex buffer
			if (new_quads.size() <= quads_count)
			{
				// Same or fewer quads, just update existing vertex data
				vb_quads_->buffer().update(lq.vertex_buffer_offset, new_vertices);
			}
			else
			{
				// More quads than before, need to re-upload entire buffer

				// TODO: This will result in gaps in the buffer, either compact
				//       the buffer here or implement some way to track and
				//       reuse freed space later

				// Update vertex offsets for line and quads
				lq.vertex_buffer_offset = vb_quads_->buffer().size();
				for (auto i = 0; i < new_quads.size(); ++i)
					lq.quads[i].vertex_offset = lq.vertex_buffer_offset + i * 6;

				vb_quads_->pull();                    // Pull data from GPU
				vb_quads_->addVertices(new_vertices); // Add new vertex data
				vb_quads_->push();                    // Push data back to GPU
			}

			// Set updated
			updated         = true;
			lq.updated_time = app::runTimer();
		}

		// Clear quad groups to be rebuilt if any quads were updated
		if (updated)
		{
			quads_updated_ = app::runTimer();
			quad_groups_.clear();
			renderer_->clearAnimations();
		}
	}

	// Generate quad groups if needed
	if (quad_groups_.empty())
	{
		// Build list of quads to process
		struct QuadToProcess
		{
			Quad3D* quad;
			bool    processed;
		};
		vector<QuadToProcess> quads_to_process;
		for (auto& sf : line_quads_)
			for (auto& quad : sf.quads)
				quads_to_process.push_back({ .quad = &quad, .processed = false });

		for (unsigned i1 = 0; i1 < quads_to_process.size(); ++i1)
		{
			if (quads_to_process[i1].processed)
				continue;

			auto quad = quads_to_process[i1].quad;

			// Build list of vertex indices for quads with this texture+colour+flags
			vector<GLuint> indices;
			for (unsigned i2 = i1; i2 < quads_to_process.size(); ++i2)
			{
				auto& match = quads_to_process[i2];

				// Ignore if already processed or not matching
				if (match.processed || *quad != *match.quad)
					continue;

				// Add indices
				auto vi = match.quad->vertex_offset;
				while (vi < match.quad->vertex_offset + 6)
					indices.push_back(vi++);

				match.processed = true;
			}

			// Add quad group for texture
			quad_groups_.push_back(
				{ .texture        = quad->texture,
				  .colour         = quad->colour,
				  .index_buffer   = std::make_unique<gl::IndexBuffer>(),
				  .render_pass    = quad->render_pass,
				  .trans_additive = quad->hasFlag(QuadFlags::Additive) });
			quad_groups_.back().index_buffer->upload(indices);

			quads_to_process[i1].processed = true;
		}
	}
}

// -----------------------------------------------------------------------------
// Adds an outline for a wall [item] to the given line [buffer].
// Can add multiple outlines if the wall part is split by extrafloors
// -----------------------------------------------------------------------------
void MapRenderer3D::addQuadOutline(const Item& item, gl::LineBuffer& buffer, float line_width) const
{
	auto real_side = item.realSide(*map_);
	if (!real_side)
		return;

	auto line   = real_side->parentLine();
	auto front  = line->s1() == real_side;
	auto v1     = front ? line->start() : line->end();
	auto v2     = front ? line->end() : line->start();
	auto colour = glm::vec4{ 1.0f };

	// Find quad(s)
	for (const auto& q : line_quads_[line->index()].quads)
	{
		if (q == item)
		{
			buffer.add3d({ v1.x, v1.y, q.height[0] }, { v2.x, v2.y, q.height[3] }, colour, line_width); // Top
			buffer.add3d({ v1.x, v1.y, q.height[0] }, { v1.x, v1.y, q.height[1] }, colour, line_width); // Left
			buffer.add3d({ v1.x, v1.y, q.height[1] }, { v2.x, v2.y, q.height[2] }, colour, line_width); // Bottom
			buffer.add3d({ v2.x, v2.y, q.height[2] }, { v2.x, v2.y, q.height[3] }, colour, line_width); // Right
		}
	}
}

// -----------------------------------------------------------------------------
// Adds vertex indices for the quad(s) representing [item] to the given list of
// [indices].
// Can add multiple quads if the wall part is split by extrafloors
// -----------------------------------------------------------------------------
void MapRenderer3D::addItemQuadIndices(const Item& item, vector<GLuint>& indices) const
{
	auto real_side = item.realSide(*map_);
	if (!real_side)
		return;

	// Find the line quads for the item
	auto line = real_side->parentLine();
	for (const auto& quad : line_quads_[line->index()].quads)
	{
		if (quad == item)
		{
			// Add quad vertex indices
			auto vi = quad.vertex_offset;
			while (vi < quad.vertex_offset + 6)
				indices.push_back(vi++);
		}
	}
}
