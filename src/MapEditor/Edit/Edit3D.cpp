
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Edit3D.cpp
// Description: Map Editor 3D mode editing functionality
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
#include "Edit3D.h"
#include "Game/Configuration.h"
#include "General/UndoRedo.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapTextureManager.h"
#include "MapEditor/UndoSteps.h"
#include "Utility/MathStuff.h"

#include <queue>
#include <set>

using namespace slade;
using mapeditor::ItemType;


// -----------------------------------------------------------------------------
//
// Edit3D Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Edit3D class constructor
// -----------------------------------------------------------------------------
Edit3D::Edit3D(MapEditContext& context) : context_{ context }, link_light_{ false }, link_offset_{ false }
{
	undo_manager_ = std::make_unique<UndoManager>(&context.map());
}

// -----------------------------------------------------------------------------
// Selects all adjacent walls or flats to [item]
// -----------------------------------------------------------------------------
void Edit3D::selectAdjacent(mapeditor::Item item) const
{
	// Check item
	if (item.index < 0)
		return;

	// Select every adjacent item
	context_.selection().select(getAdjacent(item));
	context_.selectionUpdated();
}

// -----------------------------------------------------------------------------
// Changes the light level of selected sectors by [amount]
// -----------------------------------------------------------------------------
void Edit3D::changeSectorLight(int amount) const
{
	// Get items to process
	vector<mapeditor::Item> items;
	auto&                   selection_3d = context_.selection();
	auto                    hilight_3d   = context_.hilightItem();
	if (selection_3d.empty() && hilight_3d.index >= 0 && hilight_3d.type != ItemType::Thing)
		items.push_back(hilight_3d);
	else
	{
		for (auto& item : selection_3d)
		{
			if (item.type != ItemType::Thing)
				items.push_back(item);
		}
	}
	if (items.empty())
		return;

	// Begin undo level
	context_.beginUndoRecordLocked("Change Sector Light", true, false, false);

	// Go through items
	std::set<MapObject*> processed;
	for (auto& item : items)
	{
		// Wall
		if (item.type == ItemType::WallBottom || item.type == ItemType::WallMiddle || item.type == ItemType::WallTop)
		{
			// Get side
			auto side = item.asSide(context_.map());
			if (!side)
				continue;
			auto sector = side->sector();
			if (!sector)
				continue;

			if (link_light_)
			{
				// Ignore if sector already processed
				if (processed.count(sector))
					continue;
				processed.insert(sector);
			}
			else
			{
				// Ignore if side already processed
				if (processed.count(side))
					continue;
				processed.insert(side);
			}

			// Check for decrease when light = 255
			int current_light = side->light();
			if (current_light == 255 && amount < -1)
				amount++;

			// Change wall or sector light level
			if (link_light_)
				sector->changeLight(amount);
			else
				side->changeLight(amount);
		}

		// Flat
		if (item.type == ItemType::Floor || item.type == ItemType::Ceiling)
		{
			// Get sector
			auto s     = context_.map().sector(item.index);
			int  where = 0;
			if (item.type == ItemType::Floor && !link_light_)
				where = 1;
			else if (item.type == ItemType::Ceiling && !link_light_)
				where = 2;

			// Check for decrease when light = 255
			if (s->lightAt(where) == 255 && amount < -1)
				amount++;

			// Ignore if sector already processed
			if (link_light_)
			{
				if (processed.count(s))
					continue;
				processed.insert(s);
			}

			// Change light level
			s->changeLight(amount, where);
		}
	}

	// End undo level
	context_.endUndoRecord();

	// Editor message
	if (!items.empty())
	{
		if (amount > 0)
			context_.addEditorMessage(fmt::format("Light increased by {}", amount));
		else
			context_.addEditorMessage(fmt::format("Light decreased by {}", -amount));
	}
}

// -----------------------------------------------------------------------------
// Changes the offset of selected walls by [amount].
// X axis if [x] is true, otherwise Y axis
// -----------------------------------------------------------------------------
void Edit3D::changeOffset(int amount, bool x) const
{
	// Get items to process
	vector<mapeditor::Item> items;
	auto&                   selection_3d = context_.selection();
	auto                    hilight_3d   = context_.hilightItem();
	if (selection_3d.empty())
	{
		if (hilight_3d.index >= 0 && hilight_3d.type != ItemType::Thing)
			items.push_back(hilight_3d);
	}
	else
	{
		for (auto& item : selection_3d)
		{
			if (item.type != ItemType::Thing)
				items.push_back(item);
		}
	}
	if (items.empty())
		return;

	// Begin undo level
	context_.beginUndoRecordLocked("Change Offset", true, false, false);

	// Go through items
	vector<int> done;
	bool        changed = false;
	for (auto& item : items)
	{
		int index = item.real_index >= 0 ? item.real_index : item.index;

		// Wall
		if (item.type >= ItemType::WallTop && item.type <= ItemType::WallBottom)
		{
			auto side = context_.map().side(index);

			// If offsets are linked, just change the whole side offset
			if (link_offset_)
			{
				// Check we haven't processed this side already
				if (VECTOR_EXISTS(done, index))
					continue;

				// Change the appropriate offset
				if (x)
					side->setIntProperty("offsetx", side->texOffsetX() + amount);
				else
					side->setIntProperty("offsety", side->texOffsetY() + amount);

				// Add to done list
				done.push_back(index);
			}

			// Unlinked offsets
			else
			{
				// Build property string (offset[x/y]_[top/mid/bottom])
				string ofs = "offsetx";
				if (!x)
					ofs = "offsety";
				if (item.type == ItemType::WallBottom)
					ofs += "_bottom";
				else if (item.type == ItemType::WallTop)
					ofs += "_top";
				else
					ofs += "_mid";

				// Change the offset
				double offset = side->floatProperty(ofs);
				side->setFloatProperty(ofs, offset + amount);
			}

			changed = true;
		}

		// Flat (UDMF only)
		else if (item.type == ItemType::Ceiling || item.type == ItemType::Floor)
		{
			auto sector = context_.map().sector(item.index);

			if (game::configuration().featureSupported(game::UDMFFeature::FlatPanning))
			{
				if (item.type == ItemType::Floor)
				{
					if (x)
					{
						double offset = sector->floatProperty("xpanningfloor");
						sector->setFloatProperty("xpanningfloor", offset + amount);
					}
					else
					{
						double offset = sector->floatProperty("ypanningfloor");
						sector->setFloatProperty("ypanningfloor", offset + amount);
					}

					changed = true;
				}
				else if (item.type == ItemType::Ceiling)
				{
					if (x)
					{
						double offset = sector->floatProperty("xpanningceiling");
						sector->setFloatProperty("xpanningceiling", offset + amount);
					}
					else
					{
						double offset = sector->floatProperty("ypanningceiling");
						sector->setFloatProperty("ypanningceiling", offset + amount);
					}

					changed = true;
				}
			}
		}
	}

	// End undo level
	context_.endUndoRecord(changed);

	// Editor message
	if (!items.empty() && changed)
	{
		char axis = 'X';
		if (!x)
			axis = 'Y';

		if (amount > 0)
			context_.addEditorMessage(fmt::format("{} offset increased by {}", axis, amount));
		else
			context_.addEditorMessage(fmt::format("{} offset decreased by {}", axis, -amount));
	}
}

// -----------------------------------------------------------------------------
// Changes the height of the selected 3d mode flats by [amount]
// (walls selected will change ceiling height)
// -----------------------------------------------------------------------------
void Edit3D::changeSectorHeight(int amount) const
{
	// Get items to process
	vector<mapeditor::Item> items;
	auto&                   selection_3d = context_.selection();
	auto                    hilight_3d   = context_.hilightItem();
	if (selection_3d.empty() && hilight_3d.type != ItemType::Thing && hilight_3d.index >= 0)
		items.push_back(hilight_3d);
	else
	{
		for (auto& item : selection_3d)
		{
			if (item.type != ItemType::Thing)
				items.push_back(item);
		}
	}
	if (items.empty())
		return;

	// Begin undo level
	context_.beginUndoRecordLocked("Change Sector Height", true, false, false);

	// Go through items
	vector<int> ceilings;
	for (auto& item : items)
	{
		// Wall (ceiling only for now)
		if (item.type == ItemType::WallBottom || item.type == ItemType::WallMiddle || item.type == ItemType::WallTop)
		{
			// Get sector
			auto sector = context_.map().side(item.index)->sector();

			// Check this sector's ceiling hasn't already been changed
			int index = sector->index();
			if (VECTOR_EXISTS(ceilings, index))
				continue;

			// Change height
			sector->setCeilingHeight(sector->ceiling().height + amount);

			// Set to changed
			ceilings.push_back(index);
		}

		// Floor or ceiling
		else if (item.type == ItemType::Floor || item.type == ItemType::Ceiling)
		{
			bool floor = (item.type == ItemType::Floor);

			// Get sector
			MapSector* sector = context_.map().sector(item.index);

			if (floor)
			{
				// Change height
				sector->setFloorHeight(sector->floor().height + amount);
			}
			else
			{
				// Check this sector's ceiling hasn't already been changed
				int index = sector->index();
				if (VECTOR_EXISTS(ceilings, index))
					continue;

				// Change height
				sector->setCeilingHeight(sector->ceiling().height + amount);

				// Set to changed
				ceilings.push_back(sector->index());
			}
		}
	}

	// End undo level
	context_.endUndoRecord();

	// Editor message
	if (!items.empty())
	{
		if (amount > 0)
			context_.addEditorMessage(fmt::format("Height increased by {}", amount));
		else
			context_.addEditorMessage(fmt::format("Height decreased by {}", -amount));
	}
}

// -----------------------------------------------------------------------------
// Aligns texture offsets beginning from the wall selection [start]
// -----------------------------------------------------------------------------
void Edit3D::autoAlign(mapeditor::Item start, AlignType alignType) const
{
	// Check align type
	if (alignType!=AlignType::AlignX && alignType!=AlignType::AlignY && alignType!=AlignType::AlignXY)
	{
		log::error("Edit3d::autoAlign: alignType is invalid");
		return;
	}
	
	// Check start is a wall
	if (start.type != ItemType::WallBottom && start.type != ItemType::WallMiddle && start.type != ItemType::WallTop)
		return;

	// Get starting side
	auto firstSide = context_.map().side(start.index);
	if (!firstSide)
		return;

	// Get texture to match
	string tex;
	if (start.type == ItemType::WallBottom)
		tex = firstSide->texLower();
	else if (start.type == ItemType::WallMiddle)
		tex = firstSide->texMiddle();
	else if (start.type == ItemType::WallTop)
		tex = firstSide->texUpper();

	// Don't try to auto-align a missing texture (every line on the map will probably match)
	if (tex == "-")
		return;

	// Get texture size
	auto gl_tex = mapeditor::textureManager()
					  .texture(tex, game::configuration().featureSupported(game::Feature::MixTexFlats))
					  .gl_id;
	int tex_width = -1;
	int tex_height = -1;
	if (gl_tex)
	{
		tex_width = gl::Texture::info(gl_tex).size.x;
		tex_height = gl::Texture::info(gl_tex).size.y;
	}
	else if (alignType == AlignType::AlignY || alignType == AlignType::AlignXY)
	{
		log::warning("Cannot determine height of texture {}, but is required for y alignment", tex);
		// We need the height of the texture for vertical alignment to determine anchor points
		return;
	}

	// Get vertical texture anchor
	// We cannot guarantee all textures of bottom, middle and top to
	// be aligned consistently. However, we assume that by selecting the
	// top, middle or bottom section of a wall as initial element the user
	// expresses the intent to align these sections of the walls.
	// We therefore store the/a height at which the top of the texture
	// on the respective wall portion would reside.
	// We use that later to determine the proper y-offset.
	auto firstLine = firstSide->parentLine();
	int firstTexTopHeight = getTextureTopHeight(firstLine, start.type, tex_height);
	
	// Adjust firstTexTopHeight with texture Y offset
	firstTexTopHeight += firstSide->texOffsetY();

	// Begin undo level
	string axis_names;
	switch (alignType)
	{
	case AlignType::AlignX:  axis_names = "X axis"; break;
	case AlignType::AlignY:  axis_names = "Y axis"; break;
	case AlignType::AlignXY: axis_names = "X and Y axis"; break;
	}
	context_.beginUndoRecord("Auto Align on "+axis_names, true, false, false);

	// Queue of jobs to process
	std::queue<AlignmentJob> jobs;

	// Set of sides already processed
	std::set<unsigned int> processedSides;

	// Enter the first job into the queue
	AlignmentJob firstJob;
	firstJob.side = firstSide;
	firstJob.offsetX = firstSide->texOffsetX();
	jobs.push(firstJob);

	while (!jobs.empty())
	{
		AlignmentJob job = jobs.front();
		jobs.pop();

		// Skip if side has already been processed
		if (processedSides.find(job.side->index()) != processedSides.end())
			continue;

		// Skip if this wall does not have the desired texture
		if (!(job.side->texUpper() == tex || job.side->texMiddle() == tex || job.side->texLower() == tex))
			continue;

		// Add side to set of processed sides
		processedSides.insert(job.side->index());


		switch (alignType)
		{
		case AlignType::AlignX:
		case AlignType::AlignXY:
			// Wrap x-offset
			if (tex_width > 0)
			{
				while (job.offsetX >= tex_width)
					job.offsetX -= tex_width;
				while (job.offsetX < 0)
					job.offsetX += tex_width;
			}
			// Set X offset
			job.side->setIntProperty("offsetx", job.offsetX);
			break;
		}

		switch (alignType)
		{
		case AlignType::AlignY:
		case AlignType::AlignXY:
			// First we need to determine the top height for the respective texture
			int currentTexTopHeight = -1;
			auto currentLine = job.side->parentLine();
			currentTexTopHeight = getTextureTopHeight(currentLine, start.type, tex_height);
 			// We set the offset such that currentTexTopHeight + offsetY == firstTexTopHeight
			int currentOffsetY = firstTexTopHeight - currentTexTopHeight;

			// Adjust the y-offset (but only, if we're not adjusting the middle part on a two-sided wall)
			if (start.type != ItemType::WallMiddle || !game::configuration().lineBasicFlagSet("twosided", currentLine, context_.mapDesc().format))
			{
				if (tex_height > 0) {
					while (currentOffsetY > tex_height)
						currentOffsetY -= tex_height;
					while (currentOffsetY < 0)
						currentOffsetY += tex_height;
				}
			}
			// Set Y offset
			job.side->setIntProperty("offsety", currentOffsetY);
			break;
		}

		// Align the next side on the sector, which is offset by the length of this line
		auto nextSide = job.side->nextInSector();
		if (nextSide) // Be conservative for the time being
		{
			int sideLen = (int)math::round(job.side->parentLine()->length());
			AlignmentJob nextSideJob;
			nextSideJob.side = nextSide;
			nextSideJob.offsetX = job.offsetX + sideLen;
			jobs.push(nextSideJob);

			if (game::configuration().lineBasicFlagSet("twosided", nextSide->parentLine(), context_.mapDesc().format))
			{
				// The line is two-sided, so we consider the sector on the other side
				auto nextOpposite = nextSide->oppositeSide();
				if (nextOpposite)
				{
					// The line is two-sided, so we need to process the successor of the opposite as well.
					// That side starts of at our end vertex, so the texture is offset by sideLen.
					AlignmentJob nextOpposideSideJob;
					nextOpposideSideJob.side = nextOpposite->nextInSector();
					nextOpposideSideJob.offsetX = job.offsetX + sideLen;
					jobs.push(nextOpposideSideJob);
				}
			}
		}

		// Align the previous side on the sector, which is offset by the length of the previous line
		auto prevSide = job.side->prevInSector();
		if (prevSide) // Be conservative for the time being
		{
			auto prevLine = prevSide->parentLine();
			int prevLen = (int)math::round(prevLine->length());
			AlignmentJob prevSideJob;
			prevSideJob.side = prevSide;
			prevSideJob.offsetX = job.offsetX - prevLen;
			jobs.push(prevSideJob);

			if (game::configuration().lineBasicFlagSet("twosided", prevLine, context_.mapDesc().format))
			{
				// The line is two-sided, so we consider the sector on the other side
				auto prevOpposite = prevSide->oppositeSide();
				if (prevOpposite)
				{
					// The line is two-sided, so we need to process the predecessor of the opposite as well.
					// That side starts at our start vertex, so the texture is not offset.
					AlignmentJob prevOppositeJob;
					prevOppositeJob.side = prevOpposite->prevInSector();
					prevOppositeJob.offsetX = job.offsetX;
					jobs.push(prevOppositeJob);
				}
			}
		}
	}

	// End undo level
	context_.endUndoRecord();

	// Editor message
	context_.addEditorMessage("Auto-aligned on "+axis_names);
}

// -----------------------------------------------------------------------------
// Resets offsets and scaling for the currently selected wall(s)
// -----------------------------------------------------------------------------
void Edit3D::resetOffsets() const
{
	using game::UDMFFeature;

	// Get items to process
	vector<mapeditor::Item> walls;
	vector<mapeditor::Item> flats;
	vector<mapeditor::Item> things;
	auto&                   selection_3d = context_.selection();
	auto                    hilight_3d   = context_.hilightItem();
	if (selection_3d.empty())
	{
		if (hilight_3d.type == ItemType::WallTop || hilight_3d.type == ItemType::WallBottom
			|| hilight_3d.type == ItemType::WallMiddle)
			walls.push_back(hilight_3d);
		else if (hilight_3d.type == ItemType::Floor || hilight_3d.type == ItemType::Ceiling)
			flats.push_back(hilight_3d);
		else if (hilight_3d.type == ItemType::Thing)
			things.push_back(hilight_3d);
	}
	else
	{
		for (auto& item : selection_3d)
		{
			if (item.type == ItemType::WallTop || item.type == ItemType::WallBottom
				|| item.type == ItemType::WallMiddle)
				walls.push_back(item);
			else if (item.type == ItemType::Floor || item.type == ItemType::Ceiling)
				flats.push_back(item);
			else if (item.type == ItemType::Thing)
				things.push_back(item);
		}
	}
	if (walls.empty() && flats.empty() && things.empty())
		return;

	// Begin undo level
	context_.beginUndoRecord("Reset Offsets", true, false, false);

	// Go through walls
	for (auto& wall : walls)
	{
		auto side = wall.asSide(context_.map());
		if (!side)
			continue;

		// Reset offsets
		if (link_offset_)
		{
			// If offsets are linked, reset base offsets
			side->setIntProperty("offsetx", 0);
			side->setIntProperty("offsety", 0);
		}
		else
		{
			// Otherwise, reset offsets for the current wall part
			if (wall.type == ItemType::WallTop)
			{
				side->setFloatProperty("offsetx_top", 0);
				side->setFloatProperty("offsety_top", 0);
			}
			else if (wall.type == ItemType::WallMiddle)
			{
				side->setFloatProperty("offsetx_mid", 0);
				side->setFloatProperty("offsety_mid", 0);
			}
			else
			{
				side->setFloatProperty("offsetx_bottom", 0);
				side->setFloatProperty("offsety_bottom", 0);
			}
		}

		// Reset scaling
		if (context_.mapDesc().format == MapFormat::UDMF
			&& game::configuration().featureSupported(UDMFFeature::TextureScaling))
		{
			if (wall.type == ItemType::WallTop)
			{
				side->setFloatProperty("scalex_top", 1);
				side->setFloatProperty("scaley_top", 1);
			}
			else if (wall.type == ItemType::WallMiddle)
			{
				side->setFloatProperty("scalex_mid", 1);
				side->setFloatProperty("scaley_mid", 1);
			}
			else
			{
				side->setFloatProperty("scalex_bottom", 1);
				side->setFloatProperty("scaley_bottom", 1);
			}
		}
	}

	// Go through flats
	if (context_.mapDesc().format == MapFormat::UDMF)
	{
		for (auto& flat : flats)
		{
			auto sector = flat.asSector(context_.map());
			if (!sector)
				continue;

			string plane;
			if (flat.type == ItemType::Floor)
				plane = "floor";
			else
				plane = "ceiling";

			// Reset offsets, scale, and rotation
			if (game::configuration().featureSupported(UDMFFeature::FlatPanning))
			{
				sector->setFloatProperty("xpanning" + plane, 0);
				sector->setFloatProperty("ypanning" + plane, 0);
			}
			if (game::configuration().featureSupported(UDMFFeature::FlatScaling))
			{
				sector->setFloatProperty("xscale" + plane, 1);
				sector->setFloatProperty("yscale" + plane, 1);
			}
			if (game::configuration().featureSupported(UDMFFeature::FlatRotation))
				sector->setFloatProperty("rotation" + plane, 0);
		}
	}

	// Go through things
	if (context_.mapDesc().format != MapFormat::Doom)
	{
		for (auto& item : things)
		{
			auto thing = item.asThing(context_.map());
			if (!thing)
				continue;

			// Reset height
			if (context_.mapDesc().format != MapFormat::UDMF)
				thing->setZ(0);
			else
			{
				thing->setZ(0);
				// Reset scale
				if (game::configuration().featureSupported(UDMFFeature::ThingScaling))
				{
					thing->setFloatProperty("scalex", 1);
					thing->setFloatProperty("scaley", 1);
					thing->setFloatProperty("scale", 1);
				}
				// Reset non-angle rotations
				if (game::configuration().featureSupported(UDMFFeature::ThingRotation))
				{
					thing->setIntProperty("pitch", 0);
					thing->setIntProperty("yaw", 0);
				}
			}
		}
	}

	// End undo level
	context_.endUndoRecord();

	// Editor message
	if (context_.mapDesc().format == MapFormat::UDMF
		&& (game::configuration().featureSupported(UDMFFeature::FlatScaling)
			|| game::configuration().featureSupported(UDMFFeature::SideScaling)
			|| game::configuration().featureSupported(UDMFFeature::TextureScaling)))
		context_.addEditorMessage("Offsets and scaling reset");
	else
		context_.addEditorMessage("Offsets reset");
}

// -----------------------------------------------------------------------------
// Toggles the lower/upper unpegged flag for selected walls depending on [lower]
// -----------------------------------------------------------------------------
void Edit3D::toggleUnpegged(bool lower) const
{
	auto& selection_3d = context_.selection();
	auto  hilight_3d   = context_.hilightItem();
	if (selection_3d.empty() && hilight_3d.index < 0)
		return;

	// Get items to process
	vector<mapeditor::Item> items;
	if (selection_3d.empty())
	{
		if (hilight_3d.type == ItemType::WallTop || hilight_3d.type == ItemType::WallBottom
			|| hilight_3d.type == ItemType::WallMiddle)
			items.push_back(hilight_3d);
	}
	else
	{
		for (auto& item : selection_3d)
		{
			if (item.type == ItemType::WallTop || item.type == ItemType::WallBottom
				|| item.type == ItemType::WallMiddle)
				items.push_back(item);
		}
	}
	if (items.empty())
		return;

	// Begin undo level
	string undo_type = lower ? "Toggle Lower Unpegged" : "Toggle Upper Unpegged";
	undo_manager_->beginRecord(undo_type);

	// Go through items
	vector<MapLine*> processed_lines;
	for (auto& item : items)
	{
		auto side = item.asSide(context_.map());
		if (!side || !side->parentLine())
			continue;

		// Get line
		auto line = side->parentLine();

		// Skip if line already processed
		if (VECTOR_EXISTS(processed_lines, line))
			continue;
		else
			processed_lines.push_back(line);

		// Toggle flag
		context_.recordPropertyChangeUndoStep(line);
		if (lower)
		{
			bool unpegged = game::configuration().lineBasicFlagSet("dontpegbottom", line, context_.mapDesc().format);
			game::configuration().setLineBasicFlag("dontpegbottom", line, context_.map().currentFormat(), !unpegged);
		}
		else
		{
			bool unpegged = game::configuration().lineBasicFlagSet("dontpegtop", line, context_.mapDesc().format);
			game::configuration().setLineBasicFlag("dontpegtop", line, context_.map().currentFormat(), !unpegged);
		}
	}

	// End undo level
	undo_manager_->endRecord(true);

	// Editor message
	if (lower)
		context_.addEditorMessage("Lower Unpegged flag toggled");
	else
		context_.addEditorMessage("Upper Unpegged flag toggled");
}

// -----------------------------------------------------------------------------
// Copies the currently hilighted 3d wall/flat/thing
// -----------------------------------------------------------------------------
void Edit3D::copy(CopyType type)
{
	auto  hl  = context_.selection().hilight();
	auto& map = context_.map();

	auto side   = hl.asSide(map);
	auto sector = hl.asSector(map);

	// Check hilight
	if (hl.index < 0)
		return;

	// Upper wall
	else if (side && hl.type == ItemType::WallTop)
	{
		// Texture
		if (type == CopyType::TexType)
			copy_texture_ = side->texUpper();
	}

	// Middle wall
	else if (side && hl.type == ItemType::WallMiddle)
	{
		// Texture
		if (type == CopyType::TexType)
			copy_texture_ = side->texMiddle();
	}

	// Lower wall
	else if (side && hl.type == ItemType::WallBottom)
	{
		// Texture
		if (type == CopyType::TexType)
			copy_texture_ = side->texLower();
	}

	// Floor
	else if (sector && hl.type == ItemType::Floor)
	{
		// Texture
		if (type == CopyType::TexType)
			copy_texture_ = sector->floor().texture;
	}

	// Ceiling
	else if (sector && hl.type == ItemType::Ceiling)
	{
		// Texture
		if (type == CopyType::TexType)
			copy_texture_ = sector->ceiling().texture;
	}

	// Thing
	else if (hl.type == ItemType::Thing)
	{
		if (auto thing = hl.asThing(map))
			copy_thing_.copy(thing);
	}

	// Flash
	context_.renderer().animateSelectionChange(hl);

	// Editor message
	if (type == CopyType::TexType)
	{
		if (hl.type == ItemType::Thing)
			context_.addEditorMessage("Copied Thing Type");
		else
			context_.addEditorMessage("Copied Texture");
	}
}

// -----------------------------------------------------------------------------
// Pastes previously copied wall/flat/thing info to selection
// -----------------------------------------------------------------------------
void Edit3D::paste(CopyType type) const
{
	// Begin undo step
	string ptype = "Paste Properties";
	if (type == CopyType::TexType)
		ptype = "Paste Texture/Type";
	undo_manager_->beginRecord(ptype);

	// Go through items
	auto& selection = context_.selection();
	for (auto& item : selection.selectionOrHilight())
	{
		// Wall
		if (auto side = item.asSide(context_.map()))
		{
			undo_manager_->recordUndoStep(std::make_unique<mapeditor::PropertyChangeUS>(side));

			// Upper wall
			if (item.type == ItemType::WallTop)
			{
				// Texture
				if (type == CopyType::TexType)
					side->setTexUpper(copy_texture_);
			}

			// Middle wall
			else if (item.type == ItemType::WallMiddle)
			{
				// Texture
				if (type == CopyType::TexType)
					side->setTexMiddle(copy_texture_);
			}

			// Lower wall
			else if (item.type == ItemType::WallBottom)
			{
				// Texture
				if (type == CopyType::TexType)
					side->setTexLower(copy_texture_);
			}
		}

		// Flat
		else if (auto sector = item.asSector(context_.map()))
		{
			undo_manager_->recordUndoStep(std::make_unique<mapeditor::PropertyChangeUS>(sector));

			// Floor
			if (item.type == ItemType::Floor)
			{
				// Texture
				if (type == CopyType::TexType)
					sector->setFloorTexture(copy_texture_);
			}

			// Ceiling
			if (item.type == ItemType::Ceiling)
			{
				// Texture
				if (type == CopyType::TexType)
					sector->setCeilingTexture(copy_texture_);
			}
		}

		// Thing
		else if (item.type == ItemType::Thing)
		{
			if (auto thing = item.asThing(context_.map()))
			{
				undo_manager_->recordUndoStep(std::make_unique<mapeditor::PropertyChangeUS>(thing));

				// Type
				if (type == CopyType::TexType)
					thing->setType(copy_thing_.type());
			}
		}
	}

	// Editor message
	if (type == CopyType::TexType)
	{
		if (selection.hilight().type == ItemType::Thing)
			context_.addEditorMessage("Pasted Thing Type");
		else
			context_.addEditorMessage("Pasted Texture");
	}

	undo_manager_->endRecord(true);
}

// -----------------------------------------------------------------------------
// Pastes previously copied wall/flat/thing info to selection and ad
// -----------------------------------------------------------------------------
void Edit3D::floodFill(CopyType type) const
{
	// Get items to paste to
	auto& selection = context_.selection();
	auto  items     = getAdjacent(selection.hilight());

	// Restrict floodfill to selection, if any
	if (!selection.empty())
	{
		for (unsigned i = 0; i < items.size(); ++i)
		{
			bool found = false;
			for (auto& sel_item : selection)
			{
				if (sel_item.type == items[i].type && sel_item.index == items[i].index)
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				items.erase(items.begin() + i);
				--i;
			}
		}
	}

	// Begin undo step
	string ptype = "Floodfill textures";
	undo_manager_->beginRecord(ptype);

	// Go through items
	for (auto& item : items)
	{
		// Wall
		if (auto side = item.asSide(context_.map()))
		{
			undo_manager_->recordUndoStep(std::make_unique<mapeditor::PropertyChangeUS>(side));

			// Upper wall
			if (item.type == ItemType::WallTop)
			{
				// Texture
				if (type == CopyType::TexType)
					side->setTexUpper(copy_texture_);
			}

			// Middle wall
			else if (item.type == ItemType::WallMiddle)
			{
				// Texture
				if (type == CopyType::TexType)
					side->setTexMiddle(copy_texture_);
			}

			// Lower wall
			else if (item.type == ItemType::WallBottom)
			{
				// Texture
				if (type == CopyType::TexType)
					side->setTexLower(copy_texture_);
			}
		}

		// Flat
		else if (auto sector = item.asSector(context_.map()))
		{
			undo_manager_->recordUndoStep(std::make_unique<mapeditor::PropertyChangeUS>(sector));

			// Floor
			if (item.type == ItemType::Floor)
			{
				// Texture
				if (type == CopyType::TexType)
					sector->setFloorTexture(copy_texture_);
			}

			// Ceiling
			if (item.type == ItemType::Ceiling)
			{
				// Texture
				if (type == CopyType::TexType)
					sector->setCeilingTexture(copy_texture_);
			}
		}
	}

	// Editor message
	if (type == CopyType::TexType)
	{
		context_.addEditorMessage("Floodfilled Texture");
	}

	undo_manager_->endRecord(true);
}

// -----------------------------------------------------------------------------
// Changes the Z height of selected 3d mode things by [amount]
// -----------------------------------------------------------------------------
void Edit3D::changeThingZ(int amount) const
{
	// Ignore for doom format
	if (context_.map().currentFormat() == MapFormat::Doom)
		return;

	// Go through 3d selection
	auto& selection_3d = context_.selection();
	bool  changed      = false;
	for (auto& item : selection_3d)
	{
		// Check if thing
		if (auto thing = item.asThing(context_.map()))
		{
			// Change z height
			context_.recordPropertyChangeUndoStep(thing);
			double z = thing->zPos();
			z += amount;
			thing->setZ(z);
			changed = true;
		}
	}

	if (changed)
		context_.map().recomputeSpecials();
}

// -----------------------------------------------------------------------------
// Deletes any selected 3d mode things
// -----------------------------------------------------------------------------
void Edit3D::deleteThing() const
{
	// Begin undo level
	context_.beginUndoRecord("Delete Thing", false, false, true);

	// Go through 3d selection
	auto& selection_3d = context_.selection();
	for (auto& item : selection_3d)
	{
		// Check if thing
		if (item.type == ItemType::Thing)
			context_.map().removeThing(item.index);
	}

	context_.endUndoRecord();
}

// -----------------------------------------------------------------------------
// Changes scaling for the currently selected walls/flats.
// X scale if [x] is true, y scale otherwise
// -----------------------------------------------------------------------------
void Edit3D::changeScale(double amount, bool x) const
{
	using game::UDMFFeature;

	// Get items to process
	vector<mapeditor::Item> items;
	auto&                   selection_3d = context_.selection();
	auto                    hilight_3d   = context_.hilightItem();
	if (selection_3d.empty())
	{
		if (hilight_3d.index >= 0 && hilight_3d.type != ItemType::Thing)
			items.push_back(hilight_3d);
	}
	else
	{
		for (auto& item : selection_3d)
		{
			if (item.type != ItemType::Thing)
				items.push_back(item);
		}
	}
	if (items.empty())
		return;

	// Begin undo level
	context_.beginUndoRecordLocked("Change Scale", true, false, false);

	// Go through selection
	for (auto& item : items)
	{
		// Wall
		if (game::configuration().featureSupported(UDMFFeature::SideScaling)
			|| game::configuration().featureSupported(UDMFFeature::TextureScaling))
		{
			if (auto side = item.asSide(context_.map()))
			{
				// Build property string (offset[x/y]_[top/mid/bottom])
				string ofs = "scalex";
				if (!x)
					ofs = "scaley";
				if (game::configuration().featureSupported(UDMFFeature::TextureScaling))
				{
					if (item.type == ItemType::WallBottom)
						ofs += "_bottom";
					else if (item.type == ItemType::WallTop)
						ofs += "_top";
					else
						ofs += "_mid";
				}

				// Change the offset
				double scale = side->floatProperty(ofs);
				if (scale + amount > 0)
					side->setFloatProperty(ofs, scale + amount);
			}
		}

		// Flat (UDMF only)
		else if (game::configuration().featureSupported(UDMFFeature::FlatScaling))
		{
			if (auto sector = item.asSector(context_.map()))
			{
				// Build property string
				string prop = x ? "xscale" : "yscale";
				prop += (item.type == ItemType::Floor) ? "floor" : "ceiling";

				// Set
				double scale = sector->floatProperty(prop);
				if (scale + amount > 0)
					sector->setFloatProperty(prop, scale + amount);
			}
		}
	}

	// End undo record
	context_.endUndoRecord(true);

	// Editor message
}

// -----------------------------------------------------------------------------
// Changes the height of objects, depending on type:
// Things: Z height
// Flat: height
// Wall: vertical offset
// -----------------------------------------------------------------------------
void Edit3D::changeHeight(int amount) const
{
	// Get items to process
	vector<mapeditor::Item> items;
	auto&                   selection_3d = context_.selection();
	auto                    hilight_3d   = context_.hilightItem();
	auto&                   map          = context_.map();
	if (selection_3d.empty() && hilight_3d.index >= 0)
	{
		if (hilight_3d.type != ItemType::Thing || map.currentFormat() != MapFormat::Doom)
			items.push_back(hilight_3d);
	}
	else
		for (auto& item : selection_3d)
		{
			if (item.type != ItemType::Thing || map.currentFormat() != MapFormat::Doom)
				items.push_back(item);
		}
	if (items.empty())
		return;

	// Begin undo level
	context_.beginUndoRecordLocked("Change Height", true, false, false);

	// Go through items
	for (auto& item : items)
	{
		auto type = item.type;

		// Thing
		if (auto thing = item.asThing(map))
		{
			double z = thing->zPos();
			z += amount;
			thing->setZ(z);
		}

		// Wall
		if (auto side = item.asSide(map))
		{
			string ofs = "offsety";

			// If offsets are linked, just change the whole side offset
			if (link_offset_)
			{
				int offset = side->intProperty(ofs);
				side->setIntProperty(ofs, offset + amount);
				continue;
			}
			// Unlinked offsets, build string (offsety_[top/mid/bottom])
			else if (item.type == ItemType::WallBottom)
				ofs += "_bottom";
			else if (item.type == ItemType::WallTop)
				ofs += "_top";
			else
				ofs += "_mid";

			// Change the offset
			float offset = side->floatProperty(ofs);
			side->setFloatProperty(ofs, offset + amount);
		}

		// Flat
		else if (auto sector = item.asSector(map))
		{
			if (type == ItemType::Floor)
				sector->setFloorHeight(sector->floor().height + amount);
			else if (type == ItemType::Ceiling)
				sector->setCeilingHeight(sector->ceiling().height + amount);
		}
	}

	// End undo level
	context_.endUndoRecord();

	// Editor message
	if (!items.empty())
	{
		if (amount > 0)
			context_.addEditorMessage(fmt::format("Height increased by {}", amount));
		else
			context_.addEditorMessage(fmt::format("Height decreased by {}", -amount));
	}
}

// -----------------------------------------------------------------------------
// Opens the texture browser for the currently selected 3d mode walls and/or
// floors
// -----------------------------------------------------------------------------
void Edit3D::changeTexture() const
{
	// Check for selection or hilight
	auto selection = context_.selection().selectionOrHilight();
	if (selection.empty())
		return;

	// Get initial texture
	string tex;
	auto   type  = mapeditor::TextureType::Texture;
	auto&  first = selection[0];
	auto&  map   = context_.map();
	if (auto sector = first.asSector(map))
	{
		type = mapeditor::TextureType::Flat;

		if (first.type == ItemType::Floor)
			tex = sector->floor().texture;
		else if (first.type == ItemType::Ceiling)
			tex = sector->ceiling().texture;
	}
	else if (auto side = first.asSide(map))
	{
		if (first.type == ItemType::WallBottom)
			tex = side->texLower();
		else if (first.type == ItemType::WallMiddle)
			tex = side->texMiddle();
		else if (first.type == ItemType::WallTop)
			tex = side->texUpper();
	}

	// Open texture browser
	tex = mapeditor::browseTexture(tex, type, map);
	if (!tex.empty())
	{
		bool mix = game::configuration().featureSupported(game::Feature::MixTexFlats);

		// Begin undo level
		context_.beginUndoRecord("Change Texture", true, false, false);

		// Apply to flats
		if (mix || type == mapeditor::TextureType::Flat)
		{
			for (auto& item : selection)
			{
				if (auto sector = item.asSector(map))
				{
					if (item.type == ItemType::Floor)
						sector->setFloorTexture(tex);
					else if (item.type == ItemType::Ceiling)
						sector->setCeilingTexture(tex);
				}
			}
		}

		// Apply to walls
		if (mix || type == mapeditor::TextureType::Texture)
		{
			for (auto& item : selection)
			{
				if (auto side = item.asSide(map))
				{
					if (item.type == ItemType::WallBottom)
						side->setTexLower(tex);
					else if (item.type == ItemType::WallMiddle)
						side->setTexMiddle(tex);
					else if (item.type == ItemType::WallTop)
						side->setTexUpper(tex);
				}
			}
		}

		// End undo level
		context_.endUndoRecord();
	}
}

// -----------------------------------------------------------------------------
// Clear the texture property
// -----------------------------------------------------------------------------
void Edit3D::deleteTexture() const
{
	auto& map = context_.map();
	for (auto& item : context_.selection().selectionOrHilight())
	{
		if (item.type == ItemType::Floor)
			map.sector(item.index)->setStringProperty("texturefloor", "-");
		else if (item.type == ItemType::Ceiling)
			map.sector(item.index)->setStringProperty("textureceiling", "-");
		else if (item.type == ItemType::WallBottom)
			map.side(item.index)->setStringProperty("texturebottom", "-");
		else if (item.type == ItemType::WallMiddle)
			map.side(item.index)->setStringProperty("texturemiddle", "-");
		else if (item.type == ItemType::WallTop)
			map.side(item.index)->setStringProperty("texturetop", "-");
	}
}

// -----------------------------------------------------------------------------
// Returns a list of all walls or flats adjacent to [item].
// Adjacent meaning connected and sharing a texture
// -----------------------------------------------------------------------------
vector<mapeditor::Item> Edit3D::getAdjacent(mapeditor::Item item) const
{
	vector<mapeditor::Item> list;

	// Check item
	if (item.index < 0 || item.type == ItemType::Thing)
		return list;

	// Flat
	if (item.type == ItemType::Floor || item.type == ItemType::Ceiling)
		getAdjacentFlats(item, list);

	// Wall
	else if (item.type != ItemType::Thing)
		getAdjacentWalls(item, list);

	return list;
}

// -----------------------------------------------------------------------------
// Returns true if the texture [part] of [side] matches [tex]
// -----------------------------------------------------------------------------
bool Edit3D::wallMatches(MapSide* side, ItemType part, string_view tex)
{
	// Check for blank texture where it isn't needed
	if (tex == MapSide::TEX_NONE)
	{
		auto line   = side->parentLine();
		int  needed = line->needsTexture();
		if (side == line->s1())
		{
			if (part == ItemType::WallTop && (needed & MapLine::Part::FrontUpper) == 0)
				return false;
			if (part == ItemType::WallMiddle && (needed & MapLine::Part::FrontMiddle) == 0)
				return false;
			if (part == ItemType::WallBottom && (needed & MapLine::Part::FrontLower) == 0)
				return false;
		}
		else if (side == line->s2())
		{
			if (part == ItemType::WallTop && (needed & MapLine::Part::BackUpper) == 0)
				return false;
			if (part == ItemType::WallMiddle && (needed & MapLine::Part::BackMiddle) == 0)
				return false;
			if (part == ItemType::WallBottom && (needed & MapLine::Part::BackLower) == 0)
				return false;
		}
	}

	// Check texture
	if (part == ItemType::WallTop && side->texUpper() != tex)
		return false;
	if (part == ItemType::WallMiddle && side->texMiddle() != tex)
		return false;
	if (part == ItemType::WallBottom && side->texLower() != tex)
		return false;

	return true;
}

// -----------------------------------------------------------------------------
// Adds all adjacent walls to [item] to [list].
// Adjacent meaning connected and sharing a texture
// -----------------------------------------------------------------------------
void Edit3D::getAdjacentWalls(mapeditor::Item item, vector<mapeditor::Item>& list) const
{
	// Add item to list if needed
	for (auto& list_item : list)
	{
		if (list_item.type == item.type && list_item.index == item.index)
			return;
	}
	list.push_back(item);

	// Get initial side
	auto side = item.asSide(context_.map());
	if (!side)
		return;

	// Get initial line
	auto line = side->parentLine();
	if (!line)
		return;

	// Get texture to match
	string tex;
	if (item.type == ItemType::WallBottom)
		tex = side->texLower();
	else if (item.type == ItemType::WallMiddle)
		tex = side->texMiddle();
	else
		tex = side->texUpper();

	// Go through attached lines (vertex 1)
	for (unsigned a = 0; a < line->v1()->nConnectedLines(); a++)
	{
		auto oline = line->v1()->connectedLine(a);
		if (!oline || oline == line)
			continue;

		// Get line sides
		auto side1 = oline->s1();
		auto side2 = oline->s2();

		// Front side
		if (side1)
		{
			// Upper texture
			if (wallMatches(side1, ItemType::WallTop, tex))
				getAdjacentWalls({ (int)side1->index(), ItemType::WallTop }, list);

			// Middle texture
			if (wallMatches(side1, ItemType::WallMiddle, tex))
				getAdjacentWalls({ (int)side1->index(), ItemType::WallMiddle }, list);

			// Lower texture
			if (wallMatches(side1, ItemType::WallBottom, tex))
				getAdjacentWalls({ (int)side1->index(), ItemType::WallBottom }, list);
		}

		// Back side
		if (side2)
		{
			// Upper texture
			if (wallMatches(side2, ItemType::WallTop, tex))
				getAdjacentWalls({ (int)side2->index(), ItemType::WallTop }, list);

			// Middle texture
			if (wallMatches(side2, ItemType::WallMiddle, tex))
				getAdjacentWalls({ (int)side2->index(), ItemType::WallMiddle }, list);

			// Lower texture
			if (wallMatches(side2, ItemType::WallBottom, tex))
				getAdjacentWalls({ (int)side2->index(), ItemType::WallBottom }, list);
		}
	}

	// Go through attached lines (vertex 2)
	for (unsigned a = 0; a < line->v2()->nConnectedLines(); a++)
	{
		auto oline = line->v2()->connectedLine(a);
		if (!oline || oline == line)
			continue;

		// Get line sides
		auto side1 = oline->s1();
		auto side2 = oline->s2();

		// Front side
		if (side1)
		{
			// Upper texture
			if (wallMatches(side1, ItemType::WallTop, tex))
				getAdjacentWalls({ (int)side1->index(), ItemType::WallTop }, list);

			// Middle texture
			if (wallMatches(side1, ItemType::WallMiddle, tex))
				getAdjacentWalls({ (int)side1->index(), ItemType::WallMiddle }, list);

			// Lower texture
			if (wallMatches(side1, ItemType::WallBottom, tex))
				getAdjacentWalls({ (int)side1->index(), ItemType::WallBottom }, list);
		}

		// Back side
		if (side2)
		{
			// Upper texture
			if (wallMatches(side2, ItemType::WallTop, tex))
				getAdjacentWalls({ (int)side2->index(), ItemType::WallTop }, list);

			// Middle texture
			if (wallMatches(side2, ItemType::WallMiddle, tex))
				getAdjacentWalls({ (int)side2->index(), ItemType::WallMiddle }, list);

			// Lower texture
			if (wallMatches(side2, ItemType::WallBottom, tex))
				getAdjacentWalls({ (int)side2->index(), ItemType::WallBottom }, list);
		}
	}
}

// -----------------------------------------------------------------------------
// Adds all walls and flats adjacent to [item] to [list].
// Adjacent meaning connected and sharing a texture
// -----------------------------------------------------------------------------
void Edit3D::getAdjacentFlats(mapeditor::Item item, vector<mapeditor::Item>& list) const
{
	// Check item
	if (item.index < 0 || (item.type != ItemType::Floor && item.type != ItemType::Ceiling))
		return;

	// Add item
	list.push_back(item);

	// Get initial sector
	auto sector = item.asSector(context_.map());
	if (!sector)
		return;

	// Go through sector lines
	vector<MapLine*> lines;
	sector->putLines(lines);
	for (auto& line : lines)
	{
		// Get sector on opposite side
		auto osector = (line->frontSector() == sector) ? line->backSector() : line->frontSector();

		// Skip if no sector
		if (!osector || osector == sector)
			continue;

		// Check for match
		Plane this_plane, other_plane;
		if (item.type == ItemType::Floor)
		{
			// Check sector floor texture
			if (osector->floor().texture != sector->floor().texture)
				continue;

			this_plane  = sector->floor().plane;
			other_plane = osector->floor().plane;
		}
		else
		{
			// Check sector ceiling texture
			if (osector->ceiling().texture != sector->ceiling().texture)
				continue;

			this_plane  = sector->ceiling().plane;
			other_plane = osector->ceiling().plane;
		}

		// Check that planes meet
		auto left  = line->v1()->position();
		auto right = line->v2()->position();

		double this_left_z  = this_plane.heightAt(left);
		double other_left_z = other_plane.heightAt(left);
		if (fabs(this_left_z - other_left_z) > 1)
			continue;

		double this_right_z  = this_plane.heightAt(right);
		double other_right_z = other_plane.heightAt(right);
		if (fabs(this_right_z - other_right_z) > 1)
			continue;

		// Check flat isn't already listed
		bool listed = false;
		for (auto& i : list)
		{
			if (i.type == item.type && i.index == (int)osector->index())
			{
				listed = true;
				break;
			}
		}

		// Recursively list adjacent flats
		if (!listed)
		{
			list.push_back(item);
			getAdjacentFlats({ (int)osector->index(), item.type }, list);
		}
	}
}

// -----------------------------------------------------------------------------
// Determine the height of the top end of the texture on the respective wall part
// of the given line using the given texture height.
// -----------------------------------------------------------------------------
int Edit3D::getTextureTopHeight(MapLine* firstLine, ItemType wallType, int tex_height) const
{
	assert(wallType == ItemType::WallBottom || wallType == ItemType::WallMiddle || wallType == ItemType::WallTop);
	if (wallType == ItemType::WallBottom)
	{
		const bool unpegged = game::configuration().lineBasicFlagSet("dontpegbottom", firstLine, context_.mapDesc().format);
		// If the "lower unpegged" flag is set: Top of texture is at the highest ceiling
		// Otherwise: Top of texture is at the highest floor
		if (unpegged)
			return firstLine->highestCeiling();
		else
			return firstLine->highestFloor();
	}
	else if (wallType == ItemType::WallMiddle)
	{
		const bool unpegged = game::configuration().lineBasicFlagSet("dontpegbottom", firstLine, context_.mapDesc().format);
		// If the "lower unpegged" flag is set: Top of texture is at the highest floor plus texture height
		// Otherwise: Top of texture is at the lowest ceiling
		if (unpegged)
			return firstLine->highestFloor() + tex_height;
		else
			return firstLine->lowestCeiling();
	}
	else
	{
		// wallType == ItemType::WallTop
		// If the "upper unpegged" flag is set: Top of texture is at the highest ceiling
		// Otherwise: Top of texture is at lowest ceiling plus texture height
		const bool unpegged = game::configuration().lineBasicFlagSet("dontpegtop", firstLine, context_.mapDesc().format);
		if (unpegged)
			return firstLine->highestCeiling();
		else
			return firstLine->lowestCeiling() + tex_height;
	}
}
