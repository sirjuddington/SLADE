
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapRenderer3D_Picking.cpp
// Description: MapRenderer3D class - functions related to item picking
//              (finding which map element is under the mouse cursor)
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
#include "Flat3D.h"
#include "Geometry/Geometry.h"
#include "MapEditor/Item.h"
#include "MapRenderer3D.h"
#include "OpenGL/Camera.h"
#include "OpenGL/View.h"
#include "Quad3D.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapSide.h"
#include "SLADEMap/SLADEMap.h"

using namespace slade;
using namespace mapeditor;


// -----------------------------------------------------------------------------
//
// Structs
//
// -----------------------------------------------------------------------------
namespace
{
// Simple struct to hold camera position + ray direction (in 3d and 2d)
struct Ray
{
	glm::vec3 origin_3d;
	glm::vec2 origin_2d;
	glm::vec3 dir_3d;
	glm::vec2 dir_2d;
};
} // namespace


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Calculates a ray from the [camera] through the cursor position [cursor_pos]
// in world space
// -----------------------------------------------------------------------------
glm::vec3 calculateCursorRay(const gl::Camera& camera, const gl::View& view, const Vec2i& cursor_pos)
{
	// Convert cursor position to NDC (-1 to 1 range)
	auto  viewport_size = view.size();
	float ndc_x         = (2.0f * cursor_pos.x) / viewport_size.x - 1.0f;
	float ndc_y         = 1.0f - (2.0f * cursor_pos.y) / viewport_size.y; // Flip Y

	// Create NDC points for near and far plane
	glm::vec4 ray_clip_near(ndc_x, ndc_y, -1.0f, 1.0f); // Near plane
	glm::vec4 ray_clip_far(ndc_x, ndc_y, 1.0f, 1.0f);   // Far plane

	// Get inverse projection matrix
	glm::mat4 inv_projection = glm::inverse(camera.projectionMatrix());

	// Transform to view space
	glm::vec4 ray_view_near = inv_projection * ray_clip_near;
	glm::vec4 ray_view_far  = inv_projection * ray_clip_far;

	// Perspective divide
	ray_view_near /= ray_view_near.w;
	ray_view_far /= ray_view_far.w;

	// Get inverse view matrix
	glm::mat4 inv_view = glm::inverse(camera.viewMatrix());

	// Transform to world space
	glm::vec4 ray_world_near = inv_view * ray_view_near;
	glm::vec4 ray_world_far  = inv_view * ray_view_far;

	// Calculate ray direction
	return normalize(
		glm::vec3(
			ray_world_far.x - ray_world_near.x,
			ray_world_far.y - ray_world_near.y,
			ray_world_far.z - ray_world_near.z));
}

// -----------------------------------------------------------------------------
// Finds the quad in [line]'s [quads] at the given [intersection] point.
// If [back_side] is true, only checks quads on the back side of the line,
// otherwise only checks front side.
// Returns a pointer to the intersecting quad, or nullptr if no intersection.
// -----------------------------------------------------------------------------
const Quad3D* findIntersectingLineQuad(
	const MapLine*        line,
	const vector<Quad3D>& quads,
	const glm::vec3&      intersection,
	bool                  back_side)
{
	// Find quad intersect if any
	for (auto& quad : quads)
	{
		if (quad.hasFlag(Quad3D::Flags::BackSide) != back_side)
			continue;

		Vec2f seg_left  = back_side ? line->end() : line->start();
		Vec2f seg_right = back_side ? line->start() : line->end();

		// Check intersection height
		// Need to handle slopes by finding the floor and ceiling height of
		// the quad at the intersection point
		double dist_along_segment = glm::length(intersection.xy() - seg_left) / glm::length(seg_right - seg_left);
		double top                = quad.height[0] + (quad.height[3] - quad.height[0]) * dist_along_segment;
		double bottom             = quad.height[1] + (quad.height[2] - quad.height[1]) * dist_along_segment;
		if (bottom <= intersection.z && intersection.z <= top)
			return &quad; // Found intersection
	}

	// No intersection
	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the map item corresponding to the given [line] [quad]
// -----------------------------------------------------------------------------
Item itemFromQuad(const MapLine& line, const Quad3D& quad)
{
	Item item;

	// Side index/part
	item.index = quad.side->index();
	switch (quad.part)
	{
	case map::SidePart::Middle: item.type = ItemType::WallMiddle; break;
	case map::SidePart::Upper:  item.type = ItemType::WallTop; break;
	case map::SidePart::Lower:  item.type = ItemType::WallBottom; break;
	default:                    break;
	}

	// Check for extrafloor
	if (quad.hasFlag(Quad3D::Flags::ExtraFloor))
	{
		item.control_line = quad.side->parentLine()->index();
		item.real_index   = quad.hasFlag(Quad3D::Flags::BackSide) ? line.s2Index() : line.s1Index();
	}

	return item;
}

// -----------------------------------------------------------------------------
// Finds the nearest intersecting flat in [sector] from the given [ray], up to
// a maximum distance of [max_dist]. Returns a tuple of the nearest intersecting
// flat (or nullptr if no intersection) and the distance to that flat.
// -----------------------------------------------------------------------------
std::tuple<const Flat3D*, float> findNearestIntersectingSectorFlat(
	const MapSector&      sector,
	const vector<Flat3D>& flats,
	const Ray&            ray,
	float                 max_dist)
{
	// Check distance to sector planes
	float         dist         = 0.0f;
	auto          min_dist     = max_dist;
	const Flat3D* nearest_flat = nullptr;
	for (const auto& flat : flats)
	{
		auto plane = flat.control_surface_type == map::SectorSurfaceType::Ceiling
						 ? flat.controlSector()->ceiling().plane
						 : flat.controlSector()->floor().plane;

		dist = geometry::distanceRayPlane(ray.origin_3d, ray.dir_3d, plane);
		if (dist <= 0 || dist >= min_dist)
			continue;

		// Check if on the correct side of the plane
		auto flat_z = plane.heightAt(ray.origin_3d.x, ray.origin_3d.y);
		if (flat.surface_type == map::SectorSurfaceType::Ceiling && ray.origin_3d.z >= flat_z)
			continue;
		if (flat.surface_type == map::SectorSurfaceType::Floor && ray.origin_3d.z <= flat_z)
			continue;

		// Check if intersection is within sector
		if (!sector.containsPoint((ray.origin_3d + ray.dir_3d * dist).xy()))
			continue;

		// Found intersecting flat, update nearest flat and min distance
		nearest_flat = &flat;
		min_dist     = dist;
	}

	return { nearest_flat, min_dist };
}
} // namespace


// -----------------------------------------------------------------------------
//
// MapRenderer3D Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Finds the map item at the cursor position [cursor_pos] in the given [camera]
// and [view]
// -----------------------------------------------------------------------------
Item MapRenderer3D::findHighlightedItem(const gl::Camera& camera, const gl::View& view, const Vec2i& cursor_pos)
{
	// Calculate ray from camera through cursor position
	auto ray_dir = calculateCursorRay(camera, view, cursor_pos);

	// Init variables
	float min_dist = 9999999.f;
	Item  current;
	Ray   ray{
		  .origin_3d = camera.position(), .origin_2d = camera.position().xy(), .dir_3d = ray_dir, .dir_2d = ray_dir.xy()
	};
	// Seg2d           strafe(camera.position().xy(), (camera.position() + camera.str).get2d());

	// Check for required map structures
	if (!map_ || line_quads_.size() != map_->nLines() || sector_flats_.size() != map_->nSectors()
		/*|| things_.size() != map_->nThings()*/)
		return current;

	// Check lines
	float height, dist;
	for (const auto& lq : line_quads_)
	{
		// Find (2d) distance to line
		dist = geometry::distanceRayLine(ray.origin_2d, ray.origin_2d + ray.dir_2d, lq.line->start(), lq.line->end());

		// Ignore if no intersection or something was closer
		if (dist < 0 || dist >= min_dist)
			continue;

		// Check side of camera
		auto back_side = geometry::lineSide(ray.origin_2d, lq.line->seg()) < 0;

		// Find quad intersected by the ray, if any
		auto intersection = ray.origin_3d + ray.dir_3d * dist;
		if (auto quad = findIntersectingLineQuad(lq.line, lq.quads, intersection, back_side))
		{
			// Determine selected item from quad flags
			current = itemFromQuad(*lq.line, *quad);

			// Update min distance
			min_dist = dist;
		}
	}

	// Check sectors
	for (unsigned a = 0; a < map_->nSectors(); a++)
	{
		// Find intersecting flat
		if (auto [flat, dist] = findNearestIntersectingSectorFlat(
				*map_->sector(a), sector_flats_[a].flats, ray, min_dist);
			flat)
		{
			// Found intersecting flat, update min distance
			min_dist = dist;

			// Set current item info
			current.index      = flat->control_sector->index();
			current.real_index = flat->sector->index();
			if (flat->control_surface_type == map::SectorSurfaceType::Ceiling)
				current.type = ItemType::Ceiling;
			else
				current.type = ItemType::Floor;
		}
	}

	// TODO Update item distance
	// if (min_dist >= 9999999 || min_dist < 0)
	//	item_dist_ = -1;
	// else
	//	item_dist_ = math::round(min_dist);

#if 0
	// Check things (if visible)
	if (render_3d_things == 0)
		return current;
	double halfwidth, theight;
	for (unsigned a = 0; a < map_->nThings(); a++)
	{
		// Ignore if no sprite
		if (!things_[a].sprite)
			continue;

		// Ignore if not visible
		auto thing = map_->thing(a);
		if (math::lineSide(thing->position(), strafe) > 0)
			continue;

		// Ignore if not shown
		if (!things_[a].type->decoration() && render_3d_things == 2)
			continue;

		// Find distance to thing sprite
		auto& tex_info = gl::Texture::info(things_[a].sprite);
		halfwidth      = tex_info.size.x * 0.5;
		if (things_[a].flags & ICON)
			halfwidth = render_thing_icon_size * 0.5;
		dist = math::distanceRayLine(
			cam_position_.get2d(),
			(cam_position_ + cam_dir3d_).get2d(),
			thing->position() - cam_strafe_.get2d() * halfwidth,
			thing->position() + cam_strafe_.get2d() * halfwidth);

		// Ignore if no intersection or something was closer
		if (dist < 0 || dist >= min_dist)
			continue;

		// Check intersection height
		theight = tex_info.size.y;
		height  = cam_position_.z + cam_dir3d_.z * dist;
		if (things_[a].flags & ICON)
			theight = render_thing_icon_size;
		if (height >= things_[a].z && height <= things_[a].z + theight)
		{
			current.index = a;
			current.type  = mapeditor::ItemType::Thing;
			min_dist      = dist;
		}
	}
#endif

	// TODO Update item distance
	// if (min_dist >= 9999999 || min_dist < 0)
	//	item_dist_ = -1;
	// else
	//	item_dist_ = math::round(min_dist);

	return current;
}
