
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
// Copyright(C) 2026 Ralf Gerlich
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    AlignmentEngine.cpp
// Description: Texture alignment engine
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
#include "AlignmentEngine.h"
#include "Game/Configuration.h"
#include "MapEditor/Item.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "OpenGL/GLTexture.h"
#include "SLADEMap/SLADEMap.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSide.h"
#include "SLADEMap/MapObject/MapVertex.h"
#include "Utility/MathStuff.h"

#include <cassert>

using namespace slade;
using namespace mapeditor;


// -----------------------------------------------------------------------------
//
// AlignmentEngine Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// AlignmentEngine class constructor
// -----------------------------------------------------------------------------
AlignmentEngine::AlignmentEngine(
		MapEditContext *context,
		AlignType align_type):
	context_(context),
	align_type_(align_type)
{
	assert(align_type == AlignType::AlignX || align_type == AlignType::AlignY || align_type == AlignType::AlignXY);
}

// -----------------------------------------------------------------------------
// Run the actual alignment
// -----------------------------------------------------------------------------
bool AlignmentEngine::run(Item start)
{
	assert(start.type == ItemType::WallBottom || start.type == ItemType::WallMiddle || start.type == ItemType::WallTop);

	auto first_side = context_->map().side(start.index);
	assert(first_side);

	// Get texture to match
	string tex;
	if (start.type == ItemType::WallBottom)
		tex = first_side->texLower();
	else if (start.type == ItemType::WallMiddle)
		tex = first_side->texMiddle();
	else if (start.type == ItemType::WallTop)
		tex = first_side->texUpper();

	// Don't try to auto-align a missing texture (every line on the map will probably match)
	if (tex == "-")
		return false;

	// Get texture size
	auto gl_tex = mapeditor::textureManager()
					  .texture(tex, game::configuration().featureSupported(game::Feature::MixTexFlats))
					  .gl_id;
	int tex_width  = -1;
	int tex_height = -1;
	if (gl_tex)
	{
		tex_width  = gl::Texture::info(gl_tex).size.x;
		tex_height = gl::Texture::info(gl_tex).size.y;
	}
	else if (align_type_ == AlignType::AlignY || align_type_ == AlignType::AlignXY)
	{
		// We need the height of the texture for vertical alignment to determine anchor points
		log::warning("Cannot determine height of texture {}, but is required for y alignment", tex);
		return false;
	}

	// Get vertical texture anchor
	// We cannot guarantee all textures of bottom, middle and top to
	// be aligned consistently. However, we assume that by selecting the
	// top, middle or bottom section of a wall as initial element the user
	// expresses the intent to align these sections of the walls.
	// We therefore store the/a height at which the top of the texture
	// on the respective wall portion would reside.
	// We use that later to determine the proper y-offset.
	auto first_line           = first_side->parentLine();
	int  first_tex_top_height = getTextureTopHeight(first_line, start.type, tex_height);

	// Adjust firstTexTopHeight with texture Y offset
	first_tex_top_height += first_side->texOffsetY();

	// Enter the first job into the queue
	AlignmentJob first_job;
	first_job.side         = first_side;
	first_job.tex_offset_x = first_side->texOffsetX();
	jobs.push(first_job);

	while (!jobs.empty())
	{
		AlignmentJob job = jobs.front();
		jobs.pop();

		// Skip if side has already been processed
		if (processed_sides.contains(job.side->index()))
			continue;

		// Skip if this wall does not have the desired texture
		if (!(job.side->texUpper() == tex || job.side->texMiddle() == tex || job.side->texLower() == tex))
			continue;

		// Add side to set of processed sides
		processed_sides.insert(job.side->index());

		// Perform X-Alignment
		switch (align_type_)
		{
		case AlignType::AlignX:
		case AlignType::AlignXY:
			// Wrap x-offset
			if (tex_width > 0)
			{
				while (job.tex_offset_x >= tex_width)
					job.tex_offset_x -= tex_width;
				while (job.tex_offset_x < 0)
					job.tex_offset_x += tex_width;
			}
			// Set X offset
			job.side->setIntProperty("offsetx", job.tex_offset_x);
			break;
		default: break;
		}

		// Perform Y-alignment
		MapLine* current_line = nullptr;
		int      current_offset_y, current_tex_top_height;
		switch (align_type_)
		{
		case AlignType::AlignY:
		case AlignType::AlignXY:
			// First we need to determine the top height for the respective texture
			current_line           = job.side->parentLine();
			current_tex_top_height = getTextureTopHeight(current_line, start.type, tex_height);
			// We set the offset such that currentTexTopHeight + offsetY == firstTexTopHeight
			current_offset_y = first_tex_top_height - current_tex_top_height;

			// Adjust the y-offset (but only, if we're not adjusting the middle part on a two-sided wall)
			if (start.type != ItemType::WallMiddle
				|| !game::configuration().lineBasicFlagSet("twosided", current_line, context_->mapDesc().format))
			{
				if (tex_height > 0)
				{
					while (current_offset_y > tex_height)
						current_offset_y -= tex_height;
					while (current_offset_y < 0)
						current_offset_y += tex_height;
				}
			}
			// Set Y offset
			job.side->setIntProperty("offsety", current_offset_y);
			break;
		default: break;
		}

		// Enqueue all sides connected to the start vertex of the current side
		enqueueConnectedLines(job.side->startVertex(), job.tex_offset_x);

		// Enqueue all sides connected to the end vertex of the current side
		const int side_len     = (int)math::round(job.side->parentLine()->length());
		const int end_offset_x = job.tex_offset_x + side_len;
		enqueueConnectedLines(job.side->endVertex(), end_offset_x);
	}

	return true;
}


// -----------------------------------------------------------------------------
// Determine the height of the top end of the texture on the respective wall part
// of the given line using the given texture height.
// -----------------------------------------------------------------------------
int AlignmentEngine::getTextureTopHeight(const MapLine* first_line, ItemType wall_type, int tex_height) const
{
	assert(wall_type == ItemType::WallBottom || wall_type == ItemType::WallMiddle || wall_type == ItemType::WallTop);
	if (wall_type == ItemType::WallBottom)
	{
		const bool unpegged = game::configuration().lineBasicFlagSet(
			"dontpegbottom", first_line, context_->mapDesc().format);
		// If the "lower unpegged" flag is set: Top of texture is at the highest ceiling
		// Otherwise: Top of texture is at the highest floor
		if (unpegged)
			return first_line->highestCeiling();
		else
			return first_line->highestFloor();
	}
	else if (wall_type == ItemType::WallMiddle)
	{
		const bool unpegged = game::configuration().lineBasicFlagSet(
			"dontpegbottom", first_line, context_->mapDesc().format);
		// If the "lower unpegged" flag is set: Top of texture is at the highest floor plus texture height
		// Otherwise: Top of texture is at the lowest ceiling
		if (unpegged)
			return first_line->highestFloor() + tex_height;
		else
			return first_line->lowestCeiling();
	}
	else
	{
		// wallType == ItemType::WallTop
		// If the "upper unpegged" flag is set: Top of texture is at the highest ceiling
		// Otherwise: Top of texture is at lowest ceiling plus texture height
		const bool unpegged = game::configuration().lineBasicFlagSet(
			"dontpegtop", first_line, context_->mapDesc().format);
		if (unpegged)
			return first_line->highestCeiling();
		else
			return first_line->lowestCeiling() + tex_height;
	}
}

// -----------------------------------------------------------------------------
// Add alignment jobs for all sides connected to the given common vertex.
// Calculates the correct texture x-offset for the respective sides.
// tex_offset_x gives the texture x-offset to be assumed for the common vertex.
// -----------------------------------------------------------------------------
void AlignmentEngine::enqueueConnectedLines(const MapVertex* common_vertex, int tex_offset_x)
{
	for (MapLine* line : common_vertex->connectedLines())
	{
		auto s1 = line->s1();
		if (s1)
			enqueueSide(s1, common_vertex, tex_offset_x);
		auto s2 = line->s2();
		if (s2)
			enqueueSide(s2, common_vertex, tex_offset_x);
	}
}

// -----------------------------------------------------------------------------
// Add alignment job for the given side.
// Calculates the correct texture x-offset for the respective sides.
// tex_offset_x gives the texture x-offset to be assumed for the common vertex.
// -----------------------------------------------------------------------------
void AlignmentEngine::enqueueSide(
	MapSide*                  side,
	const MapVertex*          common_vertex,
	int                       tex_offset_x)
{
	AlignmentJob job;
	job.side         = side;
	job.tex_offset_x = tex_offset_x;
	if (side->endVertex() == common_vertex)
	{
		// The side starts opposite from the given vertex,
		// so we need to adjust the texture offset by the length
		// of the side;
		job.tex_offset_x -= (int)math::round(side->parentLine()->length());
	}

	jobs.push(job);
}
