
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
// Helper Functions Predeclarations
//
// -----------------------------------------------------------------------------
static int wrap_texture_offset(int texture_offset, int texture_width);


// -----------------------------------------------------------------------------
//
// Mechanism Class Declarations
//
// -----------------------------------------------------------------------------
class AbstractAlignmentMechanism {
public:
	virtual string getTextureToMatch(MapSide *side) = 0;
	virtual bool matchesTexture(MapSide *side, string texture) = 0;

	virtual int getTextureTopHeight(MapLine *line, int texture_height) = 0;

	virtual void setOffsetX(MapSide *side, int texture_offset) = 0;
	virtual void setOffsetY(MapSide *side, int texture_offset) = 0;
};

class OverallAlignmentMechanism: public AbstractAlignmentMechanism {
public:
	OverallAlignmentMechanism(MapEditContext *context, ItemType wall_type_);

	virtual string getTextureToMatch(MapSide *side);
	virtual bool matchesTexture(MapSide *side, string texture);

	virtual int getTextureTopHeight(MapLine *line, int texture_height);

	int getOffsetX(MapSide *side);
	void setOffsetX(MapSide *side, int texture_offset);

	int getOffsetY(MapSide *side);
	void setOffsetY(MapSide *side, int texture_offset);
protected:
	MapEditContext	*context_;
	ItemType		wall_type_;
};


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
	if (!first_side)
		return false;

	std::unique_ptr<AbstractAlignmentMechanism> mechanism(new OverallAlignmentMechanism(context_, start.type));

	// Get texture to match
	const string tex = mechanism->getTextureToMatch(first_side);

	// Don't try to auto-align a missing texture
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
	int  first_tex_top_height = mechanism->getTextureTopHeight(first_line, tex_height);

	// Adjust firstTexTopHeight with texture Y offset
	first_tex_top_height += mechanism->getOffsetY(first_side);

	// Enter the first job into the queue
	AlignmentJob first_job;
	first_job.side         = first_side;
	first_job.tex_offset_x = mechanism->getOffsetX(first_side);
	jobs.push(first_job);

	processed_sides.clear();

	while (!jobs.empty())
	{
		AlignmentJob job = jobs.front();
		jobs.pop();

		// Skip if side has already been processed
		if (processed_sides.contains(job.side->index()))
			continue;

		// Skip if this wall does not have the desired texture
		if (!mechanism->matchesTexture(job.side, tex))
			continue;

		// Add side to set of processed sides
		processed_sides.insert(job.side->index());

		// Determine adjusted X-Offset
		const int texture_offset_x = wrap_texture_offset(job.tex_offset_x, tex_width);

		// Perform X-alignment
		switch (align_type_)
		{
		case AlignType::AlignX:
		case AlignType::AlignXY:
			mechanism->setOffsetX(job.side, texture_offset_x);
			break;
		default: break;
		}

		// Perform Y-alignment
		switch (align_type_)
		{
		case AlignType::AlignY:
		case AlignType::AlignXY:
			{
				// Determine the actual offset based on the difference between the top heights
				// of the current and the first line to consider.
				MapLine*	current_line = job.side->parentLine();
				int			current_tex_top_height = mechanism->getTextureTopHeight(current_line, tex_height);
				int      	texture_offset_y = first_tex_top_height - current_tex_top_height;
				mechanism->setOffsetY(job.side, texture_offset_y);
				break;
			}
		default: break;
		}

		// Enqueue all sides connected to the start vertex of the current side
		enqueueConnectedLines(job.side->startVertex(), texture_offset_x);

		// Enqueue all sides connected to the end vertex of the current side
		const int side_len     = (int)math::round(job.side->parentLine()->length());
		const int end_offset_x = texture_offset_x + side_len;
		enqueueConnectedLines(job.side->endVertex(), end_offset_x);
	}

	return true;
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

// -----------------------------------------------------------------------------
//
// Helper Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Clamp the texture offset to the range [0;texture_width-1]
// -----------------------------------------------------------------------------
static int wrap_texture_offset(int texture_offset, int texture_width)
{
	if (texture_width != 0)
	{
		texture_offset %= texture_width;
		if (texture_offset < 0)
		{
			texture_offset += texture_width;
		}
	}
	return texture_offset;
}

// -----------------------------------------------------------------------------
//
// CentralMechanism Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// AbstractMechanism class constructor
// -----------------------------------------------------------------------------
OverallAlignmentMechanism::OverallAlignmentMechanism(MapEditContext *context, ItemType wall_type):
	context_(context),
	wall_type_(wall_type)
{
	assert(wall_type_ == ItemType::WallBottom
		|| wall_type_ == ItemType::WallMiddle
		|| wall_type == ItemType::WallTop);
}


// -----------------------------------------------------------------------------
// Determine the texture to match from the given side
// -----------------------------------------------------------------------------
string OverallAlignmentMechanism::getTextureToMatch(MapSide *side)
{
	switch (wall_type_)
	{
		case ItemType::WallBottom: return side->texLower();
		case ItemType::WallMiddle: return side->texMiddle();
		case ItemType::WallTop: return side->texUpper();
		default: return "-";
	}
}

// -----------------------------------------------------------------------------
// Determine if the relevant texture of the given side matches the given texture
// -----------------------------------------------------------------------------
bool OverallAlignmentMechanism::matchesTexture(MapSide *side, string texture)
{
	return (side->texUpper() == texture
		 || side->texMiddle() == texture
		 || side->texLower() == texture);
}

int OverallAlignmentMechanism::getTextureTopHeight(MapLine* line, int tex_height)
{
	switch (wall_type_)
	{
		case ItemType::WallBottom:
		{
			const bool unpegged = game::configuration().lineBasicFlagSet(
				"dontpegbottom", line, context_->mapDesc().format);
			// If the "lower unpegged" flag is set: Top of texture is at the highest ceiling
			// Otherwise: Top of texture is at the highest floor
			if (unpegged)
				return line->highestCeiling();
			else
				return line->highestFloor();
		}
		case ItemType::WallMiddle:
		{
			const bool unpegged = game::configuration().lineBasicFlagSet(
				"dontpegbottom", line, context_->mapDesc().format);
			// If the "lower unpegged" flag is set: Top of texture is at the highest floor plus texture height
			// Otherwise: Top of texture is at the lowest ceiling
			if (unpegged)
				return line->highestFloor() + tex_height;
			else
				return line->lowestCeiling();
		}
		default:
		case ItemType::WallTop:
		{
			// wallType == ItemType::WallTop
			// If the "upper unpegged" flag is set: Top of texture is at the highest ceiling
			// Otherwise: Top of texture is at lowest ceiling plus texture height
			const bool unpegged = game::configuration().lineBasicFlagSet(
				"dontpegtop", line, context_->mapDesc().format);
			if (unpegged)
				return line->highestCeiling();
			else
				return line->lowestCeiling() + tex_height;
		}
	}
}

// -----------------------------------------------------------------------------
// Get the X-Offset
// -----------------------------------------------------------------------------
int OverallAlignmentMechanism::getOffsetX(MapSide *side)
{
	return side->texOffsetX();
}

// -----------------------------------------------------------------------------
// Set the X-Offset
// -----------------------------------------------------------------------------
void OverallAlignmentMechanism::setOffsetX(MapSide *side, int texture_offset)
{
	side->setIntProperty("offsetx", texture_offset);
}

// -----------------------------------------------------------------------------
// Get the Y-Offset
// -----------------------------------------------------------------------------
int OverallAlignmentMechanism::getOffsetY(MapSide *side)
{
	return side->texOffsetY();
}

// -----------------------------------------------------------------------------
// Set the Y-Offset
// -----------------------------------------------------------------------------
void OverallAlignmentMechanism::setOffsetY(MapSide *side, int texture_offset)
{
	side->setIntProperty("offsety", texture_offset);
}
