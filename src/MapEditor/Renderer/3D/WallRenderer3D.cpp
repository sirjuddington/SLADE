
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    WallRenderer3D.cpp
// Description: WallRenderer3D class - handles rendering of wall quads in 3D
//              mode
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
#include "WallRenderer3D.h"
#include "App.h"
#include "Geometry/Geometry.h"
#include "Geometry/Ray.h"
#include "MapEditor/Item.h"
#include "MapEditor/Renderer/MapGeometry.h"
#include "MapEditor/Renderer/Renderer.h"
#include "MapGeometryBuffer3D.h"
#include "MapRenderer3D.h"
#include "OpenGL/Camera.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/IndexBuffer.h"
#include "OpenGL/LineBuffer.h"
#include "OpenGL/Shader.h"
#include "Quad3D.h"
#include "RenderGroup.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapSide.h"
#include "SLADEMap/MapObjectList/LineList.h"
#include "SLADEMap/MapSpecials/MapSpecials.h"
#include "SLADEMap/SLADEMap.h"
#include "Utility/MathStuff.h"
#include "Utility/Vector.h"

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
// WallRenderer3D Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// WallRenderer3D class constructor
// -----------------------------------------------------------------------------
WallRenderer3D::WallRenderer3D(MapRenderer3D* renderer) : renderer_{ renderer }
{
	vb_quads_ = std::make_unique<MapGeometryBuffer3D>();
}

// -----------------------------------------------------------------------------
// WallRenderer3D class destructor
// -----------------------------------------------------------------------------
WallRenderer3D::~WallRenderer3D() = default;

// -----------------------------------------------------------------------------
// Clears all wall geometry data
// -----------------------------------------------------------------------------
void WallRenderer3D::clear()
{
	vb_quads_->buffer().clear();
	line_quads_.clear();
	quad_groups_.clear();
}

// -----------------------------------------------------------------------------
// Updates wall quad visibility from the given [camera]. Any quads further than
// [max_dist] from the camera will be hidden.
// -----------------------------------------------------------------------------
void WallRenderer3D::updateVisibility(const gl::Camera& camera, float max_dist)
{
	Vec2d cam_pos_2d = camera.position().xy();

	// Update visibility of wall quads based on camera position
	for (auto& lq : line_quads_)
	{
		if (!camera.lineInFrustum2d(lq.line->seg()))
		{
			lq.visible = false;
			continue;
		}

		lq.visible = geometry::distanceToLine(cam_pos_2d, lq.line->seg()) < max_dist;
	}

	// Force rebuild of quad groups next frame
	force_update_quad_groups_ = true;
}

// -----------------------------------------------------------------------------
// Updates wall quads and render groups as needed
// -----------------------------------------------------------------------------
bool WallRenderer3D::update(bool vis_check)
{
	using QuadFlags = Quad3D::Flags;

	auto map = renderer_->map();

	// Clear walls to be rebuilt if map geometry has been updated
	if (map->geometryUpdated() > quads_updated_)
	{
		vb_quads_->buffer().clear();
		line_quads_.clear();
		quad_groups_.clear();
		renderer_->parentRenderer()->clearAnimations();
	}

	// Generate wall quads if needed
	bool update_complete = true;
	if (line_quads_.empty() || line_quads_processed_ >= 0)
	{
		unsigned vertex_index = 0;

		if (line_quads_processed_ >= 0)
			vertex_index = quad_vb_processing_offset_; // Continue from previous partial update

		auto start_time = app::runTimer();
		for (auto* line : map->lines())
		{
			if (static_cast<int>(line->index()) <= line_quads_processed_)
				continue;

			auto [quads, vertices] = generateLineQuads(*line, vertex_index);

			line_quads_.push_back(
				{ .line                 = line,
				  .quads                = quads,
				  .vertex_buffer_offset = vertex_index,
				  .updated_time         = app::runTimer() });

			vb_quads_->addVertices(vertices);
			vertex_index += vertices.size();

			// Don't process walls for more than 200ms per frame
			if (app::runTimer() - start_time > 100)
			{
				line_quads_processed_      = line->index();
				quad_vb_processing_offset_ = vertex_index;
				update_complete            = false;
				break;
			}
		}

		if (update_complete)
			line_quads_processed_ = -1;

		vb_quads_->push(update_complete);
		quads_updated_ = app::runTimer();
		quad_groups_.clear();
	}
	else if (quadsNeedUpdate(quads_updated_, map))
	{
		map->mapSpecials().updateSpecials();

		// Check for lines that need an update
		bool             updated = false;
		vector<MGVertex> add_vertices;
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
				// More quads than before, will need to re-upload entire buffer

				// TODO: This will result in gaps in the buffer, either compact
				//       the buffer here or implement some way to track and
				//       reuse freed space later

				// Update vertex offsets for line and quads
				lq.vertex_buffer_offset = vb_quads_->buffer().size() + add_vertices.size();
				for (auto i = 0; i < new_quads.size(); ++i)
					lq.quads[i].vertex_offset = lq.vertex_buffer_offset + i * 6;

				// Add new vertex data to be added to buffer later
				vectorConcat(add_vertices, new_vertices);
			}

			// Set updated
			updated         = true;
			lq.updated_time = app::runTimer();
		}

		// Upload new vertices to the buffer, if any
		if (!add_vertices.empty())
		{
			vb_quads_->pull();                    // Pull data from GPU
			vb_quads_->addVertices(add_vertices); // Append new vertex data
			vb_quads_->push();                    // Push data back to GPU
		}

		// Clear quad groups to be rebuilt if any quads were updated
		if (updated)
		{
			quads_updated_ = app::runTimer();
			quad_groups_.clear();
			renderer_->parentRenderer()->clearAnimations();
		}
	}

	// Force rebuild of quad groups if requested
	if (force_update_quad_groups_)
	{
		quad_groups_.clear();
		force_update_quad_groups_ = false;
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
		for (auto& lq : line_quads_)
			if (!vis_check || lq.visible)
				for (auto& quad : lq.quads)
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

		// Refresh quad groups next frame if vis checking is enabled
		if (vis_check)
			force_update_quad_groups_ = true;
	}

	return update_complete;
}

// -----------------------------------------------------------------------------
// If walls have been partially updated, returns the progress so far
// -----------------------------------------------------------------------------
float WallRenderer3D::updateProgress() const
{
	if (line_quads_processed_ < 0)
		return 1.0f;

	auto map = renderer_->map();
	if (map->nLines() == 0)
		return 1.0f;

	return static_cast<float>(line_quads_processed_) / static_cast<float>(map->nLines());
}

// -----------------------------------------------------------------------------
// Renders wall quads for the given render [pass] using the specified [shader]
// -----------------------------------------------------------------------------
void WallRenderer3D::render(const gl::Shader& shader, RenderPass pass) const
{
	gl::bindVAO(vb_quads_->vao());

	for (auto& group : quad_groups_)
	{
		// Ensure group is part of the correct render pass
		if (group.render_pass != pass)
			continue;

		// Setup blending if needed
		if (pass == RenderPass::Transparent)
		{
			if (group.trans_additive)
				gl::setBlend(gl::Blend::Additive);
			else
				gl::setBlend(gl::Blend::Normal);
		}

		shader.setUniform("colour", group.colour);
		gl::Texture::bind(group.texture);
		group.index_buffer->bind();
		gl::drawElements(gl::Primitive::Triangles, group.index_buffer->size(), GL_UNSIGNED_INT);
	}

	gl::bindEBO(0);
	gl::bindVAO(0);
}

// -----------------------------------------------------------------------------
// Renders sky wall quads only (for filling the stencil buffer)
// -----------------------------------------------------------------------------
void WallRenderer3D::renderSkyGeometry() const
{
	gl::bindVAO(vb_quads_->vao());

	for (auto& group : quad_groups_)
	{
		if (group.render_pass != RenderPass::Sky)
			continue;

		group.index_buffer->bind();
		gl::drawElements(gl::Primitive::Triangles, group.index_buffer->size(), GL_UNSIGNED_INT);
	}
}

// -----------------------------------------------------------------------------
// Adds an outline for a wall [item] to the given line [buffer].
// Can add multiple outlines if the wall part is split by extrafloors
// -----------------------------------------------------------------------------
void WallRenderer3D::addOutline(const Item& item, gl::LineBuffer& buffer, float line_width) const
{
	auto real_side = item.realSide(*renderer_->map());
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
void WallRenderer3D::addItemIndices(const Item& item, vector<GLuint>& indices) const
{
	auto real_side = item.realSide(*renderer_->map());
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

// -----------------------------------------------------------------------------
// Returns the VAO for the wall quad vertex buffer
// -----------------------------------------------------------------------------
unsigned WallRenderer3D::vao() const
{
	return vb_quads_->vao();
}

// -----------------------------------------------------------------------------
// Returns the size of the wall quad vertex buffer in bytes
// -----------------------------------------------------------------------------
unsigned WallRenderer3D::bufferSize() const
{
	return vb_quads_->buffer().size() * sizeof(MGVertex);
}

// -----------------------------------------------------------------------------
// Finds the nearest wall intersected by the given [ray], up to [max_dist].
// Returns the intersecting Item and the distance to it.
// If no intersecting wall was found, the returned distance will be negative
// -----------------------------------------------------------------------------
std::pair<Item, float> WallRenderer3D::nearestIntersectingWall(const Ray& ray, float max_dist) const
{
	auto min_dist = max_dist;
	bool found    = false;
	Item nearest;

	for (const auto& lq : line_quads_)
	{
		// Find (2d) distance to line
		const auto dist = geometry::distanceRayLine(
			ray.origin_2d, ray.origin_2d + ray.dir_2d, lq.line->start(), lq.line->end());

		// Ignore if no intersection or something was closer
		if (dist < math::EPSILON || dist >= min_dist)
			continue;

		// Check side of camera
		const auto back_side = geometry::lineSide(ray.origin_2d, lq.line->seg()) < 0;

		// Find quad intersected by the ray, if any
		const auto intersection = ray.origin_3d + ray.dir_3d * static_cast<float>(dist);
		for (const auto& quad : lq.quads)
		{
			if (quad.hasFlag(Quad3D::Flags::BackSide) != back_side)
				continue;

			const Vec2f seg_left  = back_side ? lq.line->end() : lq.line->start();
			const Vec2f seg_right = back_side ? lq.line->start() : lq.line->end();

			// Check intersection height (handles slopes)
			const double dist_along = glm::length(intersection.xy() - seg_left) / glm::length(seg_right - seg_left);
			const double top        = quad.height[0] + (quad.height[3] - quad.height[0]) * dist_along;
			const double bottom     = quad.height[1] + (quad.height[2] - quad.height[1]) * dist_along;
			if (bottom > intersection.z || intersection.z > top)
				continue;

			// Build item from the intersecting quad
			Item item;
			item.index = quad.side->index();
			switch (quad.part)
			{
			case map::SidePart::Middle: item.type = ItemType::WallMiddle; break;
			case map::SidePart::Upper:  item.type = ItemType::WallTop; break;
			case map::SidePart::Lower:  item.type = ItemType::WallBottom; break;
			default:                    break;
			}
			if (quad.hasFlag(Quad3D::Flags::ExtraFloor))
			{
				item.control_line = quad.side->parentLine()->index();
				item.real_index   = back_side ? lq.line->s2Index() : lq.line->s1Index();
			}

			nearest  = item;
			found    = true;
			min_dist = dist;
			break; // first matching quad on this line wins
		}
	}

	if (found)
		return { nearest, min_dist };

	return { nearest, -1 };
}
