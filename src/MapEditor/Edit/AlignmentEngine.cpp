
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
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSide.h"
#include "SLADEMap/MapObject/MapVertex.h"
#include "SLADEMap/SLADEMap.h"
#include "Utility/MathStuff.h"

using namespace slade;
using namespace mapeditor;


// -----------------------------------------------------------------------------
//
// Helper Functions Predeclarations
//
// -----------------------------------------------------------------------------
static int wrap_texture_offset(int texture_offset, int texture_width);
static int getBottomTextureTopHeight(MapEditContext* context, const MapLine* line);
static int getMiddleTextureTopHeight(MapEditContext* context, const MapLine* line, int texture_height);
static int getTopTextureTopHeight(MapEditContext* context, const MapLine* line, int texture_height);


// -----------------------------------------------------------------------------
//
// Mechanism Class Declarations
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Abstract base class for alignment mechanisms
// -----------------------------------------------------------------------------
class AbstractAlignmentMechanism
{
public:
	virtual ~AbstractAlignmentMechanism()                        = default;
	virtual string getTextureToMatch(MapSide* side)              = 0;
	virtual bool   matchesTexture(MapSide* side, string texture) = 0;

	virtual int getTextureTopHeight(MapLine* line, int texture_height) = 0;

	// Offset getters and setters
	// Note that these must return and accept the total offset
	// (i.e. in case of per-part-offset, the per-part-offset plus
	// the general offset)
	virtual int  getOffsetX(MapSide* side)                     = 0;
	virtual void setOffsetX(MapSide* side, int texture_offset) = 0;
	virtual int  getOffsetY(MapSide* side)                     = 0;
	virtual void setOffsetY(MapSide* side, int texture_offset) = 0;
};


// -----------------------------------------------------------------------------
// Mechanism for alignment based on common offset and selection intent
// -----------------------------------------------------------------------------
class OverallAlignmentMechanism : public AbstractAlignmentMechanism
{
public:
	OverallAlignmentMechanism(MapEditContext* context, ItemType wall_type_);

	string getTextureToMatch(MapSide* side) override;
	bool   matchesTexture(MapSide* side, string texture) override;

	int getTextureTopHeight(MapLine* line, int texture_height) override;

	int  getOffsetX(MapSide* side) override;
	void setOffsetX(MapSide* side, int texture_offset) override;
	int  getOffsetY(MapSide* side) override;
	void setOffsetY(MapSide* side, int texture_offset) override;

protected:
	MapEditContext* context_;
	ItemType        wall_type_;
};

// -----------------------------------------------------------------------------
// Base class for generalized property-based alignment
// -----------------------------------------------------------------------------
class AbstractPropertyBasedAlignmentMechanism : public AbstractAlignmentMechanism
{
public:
	AbstractPropertyBasedAlignmentMechanism(
		string_view texture_property,
		string_view offset_x_property,
		string_view offset_y_property);

	string getTextureToMatch(MapSide* side) override;
	bool   matchesTexture(MapSide* side, string texture) override;

	int  getOffsetX(MapSide* side) override;
	void setOffsetX(MapSide* side, int texture_offset) override;
	int  getOffsetY(MapSide* side) override;
	void setOffsetY(MapSide* side, int texture_offset) override;

private:
	string texture_property_;
	string offset_x_property_;
	string offset_y_property_;
};

// -----------------------------------------------------------------------------
// Mechanism for alignment of individual top offset
// -----------------------------------------------------------------------------
class TopAlignmentMechanism : public AbstractPropertyBasedAlignmentMechanism
{
public:
	TopAlignmentMechanism(MapEditContext* context);

	int getTextureTopHeight(MapLine* line, int texture_height) override;

protected:
	MapEditContext* context_;
};

// -----------------------------------------------------------------------------
// Mechanism for alignment of individual middle offset
// -----------------------------------------------------------------------------
class MiddleAlignmentMechanism : public AbstractPropertyBasedAlignmentMechanism
{
public:
	MiddleAlignmentMechanism(MapEditContext* context);

	int getTextureTopHeight(MapLine* line, int texture_height) override;

protected:
	MapEditContext* context_;
};

// -----------------------------------------------------------------------------
// Mechanism for alignment of individual bottom offset
// -----------------------------------------------------------------------------
class BottomAlignmentMechanism : public AbstractPropertyBasedAlignmentMechanism
{
public:
	BottomAlignmentMechanism(MapEditContext* context);

	int getTextureTopHeight(MapLine* line, int texture_height) override;

protected:
	MapEditContext* context_;
};
} // namespace


// -----------------------------------------------------------------------------
//
// AlignmentEngine Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// AlignmentEngine class constructor
// -----------------------------------------------------------------------------
AlignmentEngine::AlignmentEngine(MapEditContext* context, AlignType align_type, AlignTarget align_target) :
	context_(context),
	align_type_(align_type),
	align_target_(align_target)
{
	wxASSERT(align_type == AlignType::AlignX || align_type == AlignType::AlignY || align_type == AlignType::AlignXY);
	wxASSERT(align_target == AlignTarget::OverallAlignment || align_target == AlignTarget::PerPartAlignment);
}

// -----------------------------------------------------------------------------
// Run the actual alignment
// -----------------------------------------------------------------------------
bool AlignmentEngine::run(Item start)
{
	wxASSERT(
		start.type == ItemType::WallBottom || start.type == ItemType::WallMiddle || start.type == ItemType::WallTop);

	auto first_side = context_->map().side(start.index);
	wxASSERT(first_side);
	if (!first_side)
		return false;

	// Select the proper alignment mechanism
	std::unique_ptr<AbstractAlignmentMechanism> mechanism;

	if (align_target_ == OverallAlignment || !game::configuration().featureSupported(game::UDMFFeature::TextureOffsets))
	{
		// If per-part-alignment is selected, but not supported, we need to
		// fall back to overall alignment
		mechanism = std::make_unique<OverallAlignmentMechanism>(context_, start.type);
	}
	else
	{
		switch (start.type)
		{
		default:
		case ItemType::WallTop:    mechanism = std::make_unique<TopAlignmentMechanism>(context_); break;
		case ItemType::WallMiddle: mechanism = std::make_unique<MiddleAlignmentMechanism>(context_); break;
		case ItemType::WallBottom: mechanism = std::make_unique<BottomAlignmentMechanism>(context_); break;
		}
	}

	// Get texture to match
	const string tex = mechanism->getTextureToMatch(first_side);

	// Don't try to auto-align a missing texture
	if (tex == "-")
		return false;

	// Get texture size
	auto gl_tex     = mapeditor::textureManager()
						  .texture(tex, game::configuration().featureSupported(game::Feature::MixTexFlats))
						  .gl_id;
	int  tex_width  = -1;
	int  tex_height = -1;
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
		case AlignType::AlignXY: mechanism->setOffsetX(job.side, texture_offset_x); break;
		default:                 break;
		}

		// Perform Y-alignment
		switch (align_type_)
		{
		case AlignType::AlignY:
		case AlignType::AlignXY:
		{
			// Determine the actual offset based on the difference between the top heights
			// of the current and the first line to consider.
			MapLine* current_line           = job.side->parentLine();
			int      current_tex_top_height = mechanism->getTextureTopHeight(current_line, tex_height);
			int      texture_offset_y       = first_tex_top_height - current_tex_top_height;
			mechanism->setOffsetY(job.side, texture_offset_y);
			break;
		}
		default: break;
		}

		// Enqueue all sides connected to the start vertex of the current side
		enqueueConnectedLines(job.side->startVertex(), texture_offset_x);

		// Enqueue all sides connected to the end vertex of the current side
		const int side_len     = math::round(job.side->parentLine()->length());
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
		if (auto s1 = line->s1())
			enqueueSide(s1, common_vertex, tex_offset_x);
		if (auto s2 = line->s2())
			enqueueSide(s2, common_vertex, tex_offset_x);
	}
}

// -----------------------------------------------------------------------------
// Add alignment job for the given side.
// Calculates the correct texture x-offset for the respective sides.
// tex_offset_x gives the texture x-offset to be assumed for the common vertex.
// -----------------------------------------------------------------------------
void AlignmentEngine::enqueueSide(MapSide* side, const MapVertex* common_vertex, int tex_offset_x)
{
	AlignmentJob job;
	job.side         = side;
	job.tex_offset_x = tex_offset_x;
	if (side->endVertex() == common_vertex)
	{
		// The side starts opposite from the given vertex,
		// so we need to adjust the texture offset by the length
		// of the side;
		job.tex_offset_x -= math::round(side->parentLine()->length());
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
// Determine the top height of the bottom wall
// -----------------------------------------------------------------------------
static int getBottomTextureTopHeight(MapEditContext* context, const MapLine* line)
{
	// If the "lower unpegged" flag is set: Top of texture is at the highest ceiling
	// Otherwise: Top of texture is at the highest floor
	if (game::configuration().lineBasicFlagSet("dontpegbottom", line, context->mapDesc().format))
		return line->highestCeiling();
	else
		return line->highestFloor();
}

// -----------------------------------------------------------------------------
// Determine the top height of the middle wall
// -----------------------------------------------------------------------------
static int getMiddleTextureTopHeight(MapEditContext* context, const MapLine* line, int texture_height)
{
	// If the "lower unpegged" flag is set: Top of texture is at the highest floor plus texture height
	// Otherwise: Top of texture is at the lowest ceiling
	if (game::configuration().lineBasicFlagSet("dontpegbottom", line, context->mapDesc().format))
		return line->highestFloor() + texture_height;
	else
		return line->lowestCeiling();
}

// -----------------------------------------------------------------------------
// Determine the top height of the top wall
// -----------------------------------------------------------------------------
static int getTopTextureTopHeight(MapEditContext* context, const MapLine* line, int texture_height)
{
	// If the "upper unpegged" flag is set: Top of texture is at the highest ceiling
	// Otherwise: Top of texture is at lowest ceiling plus texture height
	if (game::configuration().lineBasicFlagSet("dontpegtop", line, context->mapDesc().format))
		return line->highestCeiling();
	else
		return line->lowestCeiling() + texture_height;
}


// -----------------------------------------------------------------------------
//
// OverallAlignmentMechanism Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// OverallAlignmentMechanism class constructor
// -----------------------------------------------------------------------------
OverallAlignmentMechanism::OverallAlignmentMechanism(MapEditContext* context, ItemType wall_type) :
	context_(context),
	wall_type_(wall_type)
{
	wxASSERT(
		wall_type_ == ItemType::WallBottom || wall_type_ == ItemType::WallMiddle || wall_type == ItemType::WallTop);
}


// -----------------------------------------------------------------------------
// Determine the texture to match from the given side
// -----------------------------------------------------------------------------
string OverallAlignmentMechanism::getTextureToMatch(MapSide* side)
{
	switch (wall_type_)
	{
	case ItemType::WallBottom: return side->texLower();
	case ItemType::WallMiddle: return side->texMiddle();
	case ItemType::WallTop:    return side->texUpper();
	default:                   return "-";
	}
}

// -----------------------------------------------------------------------------
// Determine if the relevant texture of the given side matches the given texture
// -----------------------------------------------------------------------------
bool OverallAlignmentMechanism::matchesTexture(MapSide* side, string texture)
{
	return (side->texUpper() == texture || side->texMiddle() == texture || side->texLower() == texture);
}

// -----------------------------------------------------------------------------
// Get the top height of the relevant wall
// -----------------------------------------------------------------------------
int OverallAlignmentMechanism::getTextureTopHeight(MapLine* line, int texture_height)
{
	switch (wall_type_)
	{
	case ItemType::WallBottom: return getBottomTextureTopHeight(context_, line);
	case ItemType::WallMiddle: return getMiddleTextureTopHeight(context_, line, texture_height);
	default:
	case ItemType::WallTop:    return getTopTextureTopHeight(context_, line, texture_height);
	}
}

// -----------------------------------------------------------------------------
// Get the X-Offset
// -----------------------------------------------------------------------------
int OverallAlignmentMechanism::getOffsetX(MapSide* side)
{
	return side->texOffsetX();
}

// -----------------------------------------------------------------------------
// Set the X-Offset
// -----------------------------------------------------------------------------
void OverallAlignmentMechanism::setOffsetX(MapSide* side, int texture_offset)
{
	side->setTexOffsetX(texture_offset);
}

// -----------------------------------------------------------------------------
// Get the Y-Offset
// -----------------------------------------------------------------------------
int OverallAlignmentMechanism::getOffsetY(MapSide* side)
{
	return side->texOffsetY();
}

// -----------------------------------------------------------------------------
// Set the Y-Offset
// -----------------------------------------------------------------------------
void OverallAlignmentMechanism::setOffsetY(MapSide* side, int texture_offset)
{
	side->setTexOffsetY(texture_offset);
}


// -----------------------------------------------------------------------------
//
// AbstractPropertyBasedAlignmentMechanism Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// AbstractPropertyBasedAlignmentMechanism class constructor
// -----------------------------------------------------------------------------
AbstractPropertyBasedAlignmentMechanism::AbstractPropertyBasedAlignmentMechanism(
	string_view texture_property,
	string_view offset_x_property,
	string_view offset_y_property) :
	texture_property_(texture_property),
	offset_x_property_(offset_x_property),
	offset_y_property_(offset_y_property)
{
}

// -----------------------------------------------------------------------------
// Get the X-Offset
// -----------------------------------------------------------------------------
int AbstractPropertyBasedAlignmentMechanism::getOffsetX(MapSide* side)
{
	// We need to correct for the basic texture X-offset
	return side->intProperty(offset_x_property_) + side->texOffsetX();
}

// -----------------------------------------------------------------------------
// Set the X-Offset
// -----------------------------------------------------------------------------
void AbstractPropertyBasedAlignmentMechanism::setOffsetX(MapSide* side, int texture_offset)
{
	// We need to correct for the basic texture X-offset
	side->setIntProperty(offset_x_property_, texture_offset - side->texOffsetX());
}

// -----------------------------------------------------------------------------
// Get the Y-Offset
// -----------------------------------------------------------------------------
int AbstractPropertyBasedAlignmentMechanism::getOffsetY(MapSide* side)
{
	// We need to correct for the basic texture Y-offset
	return side->intProperty(offset_y_property_) + side->texOffsetY();
}

// -----------------------------------------------------------------------------
// Set the Y-Offset
// -----------------------------------------------------------------------------
void AbstractPropertyBasedAlignmentMechanism::setOffsetY(MapSide* side, int texture_offset)
{
	// We need to correct for the basic texture Y-offset
	side->setIntProperty(offset_y_property_, texture_offset - side->texOffsetY());
}

// -----------------------------------------------------------------------------
// Determine the texture to match from the given side
// -----------------------------------------------------------------------------
string AbstractPropertyBasedAlignmentMechanism::getTextureToMatch(MapSide* side)
{
	return side->stringProperty(texture_property_);
}

// -----------------------------------------------------------------------------
// Determine if the relevant texture of the given side matches the given texture
// -----------------------------------------------------------------------------
bool AbstractPropertyBasedAlignmentMechanism::matchesTexture(MapSide* side, string texture)
{
	return getTextureToMatch(side) == texture;
}


// -----------------------------------------------------------------------------
//
// TopAlignmentMechanism Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// TopAlignmentMechanism class constructor
// -----------------------------------------------------------------------------
TopAlignmentMechanism::TopAlignmentMechanism(MapEditContext* context) :
	AbstractPropertyBasedAlignmentMechanism(MapSide::PROP_TEXUPPER, "offsetx_top", "offsety_top"),
	context_(context)
{
}

// -----------------------------------------------------------------------------
// Get the top height of the top wall
// -----------------------------------------------------------------------------
int TopAlignmentMechanism::getTextureTopHeight(MapLine* line, int texture_height)
{
	return getTopTextureTopHeight(context_, line, texture_height);
}


// -----------------------------------------------------------------------------
//
// MiddleAlignmentMechanism Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MiddleAlignmentMechanism class constructor
// -----------------------------------------------------------------------------
MiddleAlignmentMechanism::MiddleAlignmentMechanism(MapEditContext* context) :
	AbstractPropertyBasedAlignmentMechanism(MapSide::PROP_TEXMIDDLE, "offsetx_mid", "offsety_mid"),
	context_(context)
{
}

// -----------------------------------------------------------------------------
// Get the top height of the middle wall
// -----------------------------------------------------------------------------
int MiddleAlignmentMechanism::getTextureTopHeight(MapLine* line, int texture_height)
{
	return getMiddleTextureTopHeight(context_, line, texture_height);
}


// -----------------------------------------------------------------------------
//
// BottomAlignmentMechanism Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// BottomAlignmentMechanism class constructor
// -----------------------------------------------------------------------------
BottomAlignmentMechanism::BottomAlignmentMechanism(MapEditContext* context) :
	AbstractPropertyBasedAlignmentMechanism(MapSide::PROP_TEXLOWER, "offsetx_bottom", "offsety_bottom"),
	context_(context)
{
}

// -----------------------------------------------------------------------------
// Get the top height of the bottom wall
// -----------------------------------------------------------------------------
int BottomAlignmentMechanism::getTextureTopHeight(MapLine* line, int texture_height)
{
	return getBottomTextureTopHeight(context_, line);
}
