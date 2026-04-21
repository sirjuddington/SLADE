
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    FlatRenderer3D.cpp
// Description: FlatRenderer3D class - handles rendering of sector flats
//              (floors and ceilings) in 3D mode
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
#include "FlatRenderer3D.h"
#include "App.h"
#include "Flat3D.h"
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
#include "RenderGroup.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapSide.h"
#include "SLADEMap/MapObjectList/SectorList.h"
#include "SLADEMap/MapSpecials/MapSpecials.h"
#include "SLADEMap/SLADEMap.h"
#include "SLADEMap/Types.h"
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
// FlatRenderer3D Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// FlatRenderer3D class constructor
// -----------------------------------------------------------------------------
FlatRenderer3D::FlatRenderer3D(MapRenderer3D* renderer) : renderer_{ renderer }
{
	vb_flats_ = std::make_unique<MapGeometryBuffer3D>();
}

// -----------------------------------------------------------------------------
// FlatRenderer3D class destructor
// -----------------------------------------------------------------------------
FlatRenderer3D::~FlatRenderer3D() = default;

// -----------------------------------------------------------------------------
// Clears all flat geometry data
// -----------------------------------------------------------------------------
void FlatRenderer3D::clear()
{
	vb_flats_->buffer().clear();
	sector_flats_.clear();
	flat_groups_.clear();
}

// -----------------------------------------------------------------------------
// Updates sector flat visibility from the given [camera]. Any flats further
// than [max_dist] from the camera will be hidden.
// -----------------------------------------------------------------------------
void FlatRenderer3D::updateVisibility(const gl::Camera& camera, float max_dist)
{
	Vec2d cam_pos_2d = camera.position().xy();

	for (auto& sf : sector_flats_)
	{
		// Check sector bbox with camera frustum first
		if (!camera.bboxInFrustum2d(sf.sector->boundingBox()))
		{
			sf.visible = false;
			continue;
		}

		// Check with sector bounding box mid-point
		if (glm::distance(cam_pos_2d, sf.sector->boundingBox().mid()) < max_dist)
		{
			sf.visible = true;
			continue;
		}

		// Find closest bounding box side to camera position
		Seg2d bbox_side = sf.sector->boundingBox().bottomSide();
		auto  min_dist  = geometry::distanceToLineFast(cam_pos_2d, bbox_side);
		auto  dist      = geometry::distanceToLineFast(cam_pos_2d, sf.sector->boundingBox().topSide());
		if (dist < min_dist)
		{
			bbox_side = sf.sector->boundingBox().topSide();
			min_dist  = dist;
		}
		dist = geometry::distanceToLineFast(cam_pos_2d, sf.sector->boundingBox().leftSide());
		if (dist < min_dist)
		{
			bbox_side = sf.sector->boundingBox().leftSide();
			min_dist  = dist;
		}
		dist = geometry::distanceToLineFast(cam_pos_2d, sf.sector->boundingBox().rightSide());
		if (dist < min_dist)
			bbox_side = sf.sector->boundingBox().rightSide();

		// Check if closest side is within max distance
		if (geometry::distanceToLine(cam_pos_2d, bbox_side) < max_dist)
			sf.visible = true;
		else
			sf.visible = false;
	}

	// Force rebuild of flat groups next frame
	force_update_flat_groups_ = true;
}

// -----------------------------------------------------------------------------
// Updates sector flats/geometry and render groups if needed
// -----------------------------------------------------------------------------
bool FlatRenderer3D::update(bool vis_check)
{
	using FlatFlags = Flat3D::Flags;

	auto map = renderer_->map();

	// Clear flats to be rebuilt if map geometry has been updated
	if (map->geometryUpdated() > flats_updated_)
	{
		vb_flats_->buffer().clear();
		sector_flats_.clear();
		flat_groups_.clear();
		renderer_->parentRenderer()->clearAnimations();
	}

	// Generate flats if needed
	bool update_complete = true;
	if (sector_flats_.empty() || sector_flats_processed_ >= 0)
	{
		unsigned vertex_index = 0;

		if (sector_flats_processed_ >= 0)
			vertex_index = sector_vb_processing_offset_; // Continue from previous partial update

		auto start_time = app::runTimer();
		for (auto* sector : map->sectors())
		{
			if (static_cast<int>(sector->index()) <= sector_flats_processed_)
				continue;

			auto [flats, vertices] = generateSectorFlats(*sector, vertex_index);

			sector_flats_.push_back(
				{ .sector               = sector,
				  .flats                = flats,
				  .vertex_buffer_offset = vertex_index,
				  .updated_time         = app::runTimer() });

			vb_flats_->addVertices(vertices);
			vertex_index += vertices.size();

			// Don't process flats for more than 200ms per frame
			if (app::runTimer() - start_time > 200)
			{
				sector_flats_processed_      = sector->index();
				sector_vb_processing_offset_ = vertex_index;
				update_complete              = false;
				break;
			}
		}

		if (update_complete)
			sector_flats_processed_ = -1;

		vb_flats_->push(update_complete);
		flats_updated_ = app::runTimer();
		flat_groups_.clear();
	}
	else if (flatsNeedUpdate(flats_updated_, map))
	{
		map->mapSpecials().updateSpecials();

		// Check for sectors that need an update
		bool             updated = false;
		vector<MGVertex> add_vertices;
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
				// More flats than before, will need to re-upload entire buffer

				// TODO: This will result in gaps in the buffer, either compact
				//       the buffer here or implement some way to track and
				//       reuse freed space later

				// Update vertex offsets for sector and flats
				sf.vertex_buffer_offset = vb_flats_->buffer().size() + add_vertices.size();
				const auto vertex_count = sf.sector->polygonVertices().size();
				for (auto i = 0; i < new_flats.size(); ++i)
					sf.flats[i].vertex_offset = sf.vertex_buffer_offset + i * vertex_count;

				// Add new vertex data to be added to buffer later
				vectorConcat(add_vertices, new_vertices);
			}

			// Set updated
			updated         = true;
			sf.updated_time = app::runTimer();
		}

		// Upload new vertices to the buffer, if any
		if (!add_vertices.empty())
		{
			vb_flats_->pull();                    // Pull data from GPU
			vb_flats_->addVertices(add_vertices); // Append new vertex data
			vb_flats_->push();                    // Push data back to GPU
		}

		// Clear flat groups to be rebuilt if any flats were updated
		if (updated)
		{
			flats_updated_ = app::runTimer();
			flat_groups_.clear();
			renderer_->parentRenderer()->clearAnimations();
		}
	}

	// Force rebuild of flat groups if requested
	if (force_update_flat_groups_)
	{
		flat_groups_.clear();
		force_update_flat_groups_ = false;
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
			if (!vis_check || sf.visible)
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
				  .fade_colour    = fp1.flat->fade_colour.rgb(),
				  .index_buffer   = std::make_unique<gl::IndexBuffer>(),
				  .render_pass    = fp1.flat->render_pass,
				  .trans_additive = fp1.flat->hasFlag(FlatFlags::Additive) });
			flat_groups_.back().index_buffer->upload(indices);

			fp1.processed = true;
		}

		// Refresh flat groups next frame if vis checking is enabled
		if (vis_check)
			force_update_flat_groups_ = true;
	}

	return update_complete;
}

// -----------------------------------------------------------------------------
// If flats have been partially updated, returns the progress so far
// -----------------------------------------------------------------------------
float FlatRenderer3D::updateProgress() const
{
	if (sector_flats_processed_ < 0)
		return 1.0f;

	auto map = renderer_->map();
	if (map->nSectors() == 0)
		return 1.0f;

	return static_cast<float>(sector_flats_processed_) / static_cast<float>(map->nSectors());
}

// -----------------------------------------------------------------------------
// Renders sector flats for the given render [pass] using the specified [shader]
// -----------------------------------------------------------------------------
void FlatRenderer3D::render(const gl::Shader& shader, RenderPass pass) const
{
	gl::bindVAO(vb_flats_->vao());

	for (auto& group : flat_groups_)
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
		shader.setUniform("fog_colour", group.fade_colour);
		gl::Texture::bind(group.texture);
		group.index_buffer->bind();
		gl::drawElements(gl::Primitive::Triangles, group.index_buffer->size(), GL_UNSIGNED_INT);
	}

	gl::bindEBO(0);
	gl::bindVAO(0);
}

// -----------------------------------------------------------------------------
// Renders sky flats only (for filling the stencil buffer)
// -----------------------------------------------------------------------------
void FlatRenderer3D::renderSkyGeometry() const
{
	gl::bindVAO(vb_flats_->vao());

	for (auto& group : flat_groups_)
	{
		if (group.render_pass != RenderPass::Sky)
			continue;

		group.index_buffer->bind();
		gl::drawElements(gl::Primitive::Triangles, group.index_buffer->size(), GL_UNSIGNED_INT);
	}
}

// -----------------------------------------------------------------------------
// Adds an outline for a sector floor/ceiling [item] to the given line [buffer]
// -----------------------------------------------------------------------------
void FlatRenderer3D::addOutline(const Item& item, gl::LineBuffer& buffer, float line_width) const
{
	auto map    = renderer_->map();
	auto sector = item.asSector(*map);
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

	for (auto side : item.realSector(*map)->connectedSides())
		buffer.add3d(side->parentLine()->start(), side->parentLine()->end(), plane, colour, line_width);
}

// -----------------------------------------------------------------------------
// Adds vertex indices for the flat representing [item] to the given list of
// [indices]
// -----------------------------------------------------------------------------
void FlatRenderer3D::addItemIndices(const Item& item, vector<GLuint>& indices) const
{
	auto real_index = item.real_index >= 0 ? item.real_index : item.index;
	if (real_index < 0 || real_index >= static_cast<int>(sector_flats_.size()))
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

// -----------------------------------------------------------------------------
// Returns the VAO for the flat vertex buffer
// -----------------------------------------------------------------------------
unsigned FlatRenderer3D::vao() const
{
	return vb_flats_->vao();
}

// -----------------------------------------------------------------------------
// Returns the size of the flat vertex buffer in bytes
// -----------------------------------------------------------------------------
unsigned FlatRenderer3D::bufferSize() const
{
	return vb_flats_->buffer().size() * sizeof(MGVertex);
}

// -----------------------------------------------------------------------------
// Finds the nearest flat intersected by the given [ray], up to [max_dist].
// Returns the intersecting Item, or nullopt if no intersection.
// -----------------------------------------------------------------------------
std::pair<Item, float> FlatRenderer3D::nearestIntersectingFlat(const Ray& ray, float max_dist) const
{
	auto min_dist = max_dist;
	bool found    = false;
	Item nearest;

	for (const auto& sf : sector_flats_)
	{
		for (const auto& flat : sf.flats)
		{
			const auto plane = flat.control_surface_type == map::SectorSurfaceType::Ceiling
								   ? flat.controlSector()->ceiling().plane
								   : flat.controlSector()->floor().plane;

			const auto dist = geometry::distanceRayPlane(ray.origin_3d, ray.dir_3d, plane);
			if (dist < math::EPSILON || dist >= min_dist)
				continue;

			// Check if on the correct side of the plane
			const auto flat_z = plane.heightAt(ray.origin_3d.x, ray.origin_3d.y);
			if (flat.surface_type == map::SectorSurfaceType::Ceiling && ray.origin_3d.z >= flat_z)
				continue;
			if (flat.surface_type == map::SectorSurfaceType::Floor && ray.origin_3d.z <= flat_z)
				continue;

			// Check if intersection is within sector
			if (!sf.sector->containsPoint((ray.origin_3d + ray.dir_3d * static_cast<float>(dist)).xy()))
				continue;

			Item item;
			item.index      = flat.control_sector->index();
			item.real_index = flat.sector->index();
			item.type       = flat.control_surface_type == map::SectorSurfaceType::Ceiling ? ItemType::Ceiling
																						   : ItemType::Floor;

			nearest  = item;
			found    = true;
			min_dist = dist;
		}
	}

	if (found)
		return { nearest, min_dist };

	return { nearest, -1 };
}
