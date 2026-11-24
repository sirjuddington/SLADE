
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
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
#include "AlignmentEngine.h"
#include "Game/Configuration.h"
#include "General/UndoRedo.h"
#include "MapEditor/ItemSelection.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/Renderer/Renderer.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapSide.h"
#include "SLADEMap/MapObject/MapThing.h"
#include "SLADEMap/MapObject/MapVertex.h"
#include "SLADEMap/MapSpecials/MapSpecials.h"
#include "SLADEMap/SLADEMap.h"
#include "Utility/Vector.h"
#include <queue>
#include <set>

using namespace slade;
using namespace mapeditor;


// -----------------------------------------------------------------------------
//
// Edit3D Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Edit3D class constructor
// -----------------------------------------------------------------------------
Edit3D::Edit3D(MapEditContext& context) : context_{ &context }
{
	undo_manager_ = std::make_unique<UndoManager>(&context.map());
	copy_thing_   = std::make_unique<MapThing>();
}

// -----------------------------------------------------------------------------
// Selects all adjacent walls or flats to [item]
// -----------------------------------------------------------------------------
void Edit3D::selectAdjacent(Item item) const
{
	// Check item
	if (item.index < 0)
		return;

	// Select every adjacent item
	context_->selection().select(getAdjacent(item));
	context_->selectionUpdated();
}

// -----------------------------------------------------------------------------
// Changes the light level of selected sectors by [amount]
// -----------------------------------------------------------------------------
void Edit3D::changeSectorLight(int amount) const
{
	// Get items to process
	vector<Item> items;
	auto&        selection_3d = context_->selection();
	auto         hilight_3d   = context_->hilightItem();
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
	context_->beginUndoRecordLocked("Change Sector Light", true, false, false);

	// Go through items
	std::set<MapObject*> processed;
	for (auto& item : items)
	{
		// Wall
		if (item.type == ItemType::WallBottom || item.type == ItemType::WallMiddle || item.type == ItemType::WallTop)
		{
			// Get side
			auto side = item.asSide(context_->map());
			if (!side)
				continue;
			auto sector = side->sector();
			if (!sector)
				continue;

			if (link_light_)
			{
				// Ignore if sector already processed
				if (processed.contains(sector))
					continue;
				processed.insert(sector);
			}
			else
			{
				// Ignore if side already processed
				if (processed.contains(side))
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
			auto s     = context_->map().sector(item.index);
			auto where = map::SectorPart::Interior;
			if (item.type == ItemType::Floor && !link_light_)
				where = map::SectorPart::Floor;
			else if (item.type == ItemType::Ceiling && !link_light_)
				where = map::SectorPart::Ceiling;

			// Check for decrease when light = 255
			if (s->lightAt(where) == 255 && amount < -1)
				amount++;

			// Ignore if sector already processed
			if (link_light_)
			{
				if (processed.contains(s))
					continue;
				processed.insert(s);
			}

			// Change light level
			s->changeLight(amount, where);
		}
	}

	// End undo level
	context_->endUndoRecord();

	// Editor message
	if (!items.empty())
	{
		if (amount > 0)
			context_->addEditorMessage(fmt::format("Light increased by {}", amount));
		else
			context_->addEditorMessage(fmt::format("Light decreased by {}", -amount));
	}
}

// -----------------------------------------------------------------------------
// Changes the offset of selected walls by [amount].
// X axis if [x] is true, otherwise Y axis
// -----------------------------------------------------------------------------
void Edit3D::changeOffset(int amount, bool x) const
{
	// Get items to process
	vector<Item> items;
	auto&        selection_3d = context_->selection();
	auto         hilight_3d   = context_->hilightItem();
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
	context_->beginUndoRecordLocked("Change Offset", true, false, false);

	// Go through items
	vector<int> done;
	bool        changed = false;
	for (auto& item : items)
	{
		int index = item.real_index >= 0 ? item.real_index : item.index;

		// Wall
		if (item.type >= ItemType::WallTop && item.type <= ItemType::WallBottom)
		{
			auto side = context_->map().side(index);

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
			auto sector = context_->map().sector(item.index);

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
	context_->endUndoRecord(changed);

	// Editor message
	if (!items.empty() && changed)
	{
		char axis = 'X';
		if (!x)
			axis = 'Y';

		if (amount > 0)
			context_->addEditorMessage(fmt::format("{} offset increased by {}", axis, amount));
		else
			context_->addEditorMessage(fmt::format("{} offset decreased by {}", axis, -amount));
	}
}

// -----------------------------------------------------------------------------
// Changes the height of the selected 3d mode flats by [amount]
// (walls selected will change ceiling height)
// -----------------------------------------------------------------------------
void Edit3D::changeSectorHeight(int amount) const
{
	// Get items to process
	vector<Item> items;
	auto&        selection_3d = context_->selection();
	auto         hilight_3d   = context_->hilightItem();
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
	context_->beginUndoRecordLocked("Change Sector Height", true, false, false);

	// Go through items
	vector<int>      ceilings;
	bool             floors_changed = false;
	vector<MapLine*> lines;
	for (auto& item : items)
	{
		// Wall (ceiling only for now)
		if (item.type == ItemType::WallBottom || item.type == ItemType::WallMiddle || item.type == ItemType::WallTop)
		{
			// Get sector
			auto sector = context_->map().side(item.index)->sector();
			if (!sector)
				continue;

			// Check this sector's ceiling hasn't already been changed
			int index = sector->index();
			if (VECTOR_EXISTS(ceilings, index))
				continue;

			// Change height
			sector->setCeilingHeight(sector->ceiling().height + amount);

			// Set to changed
			ceilings.push_back(index);

			// Add sector lines for later processing
			for (auto s : sector->connectedSides())
				vectorAddUnique(lines, s->parentLine());
		}

		// Floor or ceiling
		else if (item.type == ItemType::Floor || item.type == ItemType::Ceiling)
		{
			bool floor = (item.type == ItemType::Floor);

			// Get sector
			MapSector* sector = context_->map().sector(item.index);
			if (!sector)
				continue;

			if (floor)
			{
				// Change height
				sector->setFloorHeight(sector->floor().height + amount);
				floors_changed = true;
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

			// Add sector lines for later processing
			for (auto s : sector->connectedSides())
				vectorAddUnique(lines, s->parentLine());
		}
	}

	if (CVar::getBool("map_fill_missing_textures"))
	{
		// Find adjacent textures to use (separate for upper/lower since we don't
		// want eg. an upper texture propogating to a 'disconnected' lower texture)
		auto tex_default = game::configuration().defaultString(MapObject::Type::Side, "texturemiddle");
		auto tex_upper = context_->map().adjacentLineTexture(lines, MapLine::Part::AnyUpper | MapLine::Part::AnyMiddle);
		auto tex_lower = context_->map().adjacentLineTexture(lines, MapLine::Part::AnyLower | MapLine::Part::AnyMiddle);

		// Fill any resulting missing textures
		bool mix_tex_flats = game::configuration().featureSupported(game::Feature::MixTexFlats);
		bool found_upper   = tex_upper != MapSide::TEX_NONE;
		bool found_lower   = tex_lower != MapSide::TEX_NONE;
		for (auto line : lines)
		{
			line->fillMissingTextures(
				!ceilings.empty(), false, found_upper ? tex_upper : tex_default, mix_tex_flats && !found_upper);
			line->fillMissingTextures(
				false, floors_changed, found_lower ? tex_lower : tex_default, mix_tex_flats && !found_lower);
		}
	}

	// End undo level
	context_->endUndoRecord();

	// Editor message
	if (!items.empty())
	{
		if (amount > 0)
			context_->addEditorMessage(fmt::format("Height increased by {}", amount));
		else
			context_->addEditorMessage(fmt::format("Height decreased by {}", -amount));
	}
}

// -----------------------------------------------------------------------------
// Aligns texture offsets beginning from the wall selection [start]
// -----------------------------------------------------------------------------
void Edit3D::autoAlign(Item start, AlignType align_type) const
{
	// Check align type
	if (align_type != AlignType::AlignX && align_type != AlignType::AlignY && align_type != AlignType::AlignXY)
	{
		log::error("Edit3D::autoAlign: alignType is invalid");
		return;
	}

	// Check start is a wall
	if (start.type != ItemType::WallBottom && start.type != ItemType::WallMiddle && start.type != ItemType::WallTop)
	{
		log::error("Edit3D::autoAlign: start item is not a wall");
		return;
	}

	// Begin undo level
	string axis_names;
	switch (align_type)
	{
	case AlignType::AlignX:  axis_names = "X axis"; break;
	case AlignType::AlignY:  axis_names = "Y axis"; break;
	case AlignType::AlignXY: axis_names = "X and Y axis"; break;
	}
	context_->beginUndoRecord("Auto Align on " + axis_names, true, false, false);

	auto            align_target = (offsetsLinked() ? AlignTarget::OverallAlignment : AlignTarget::PerPartAlignment);
	AlignmentEngine alignment_engine(context_, align_type, align_target);
	bool            success = alignment_engine.run(start);

	// End undo level (abort, if alignment failed)
	context_->endUndoRecord(success);

	if (success)
		// Editor message
		context_->addEditorMessage("Auto-aligned on " + axis_names);
}

// -----------------------------------------------------------------------------
// Resets offsets and scaling for the currently selected wall(s)
// -----------------------------------------------------------------------------
void Edit3D::resetOffsets() const
{
	using game::UDMFFeature;

	// Get items to process
	vector<Item> walls;
	vector<Item> flats;
	vector<Item> things;
	auto&        selection_3d = context_->selection();
	auto         hilight_3d   = context_->hilightItem();
	if (selection_3d.empty())
	{
		if (hilight_3d.type == ItemType::WallTop
			|| hilight_3d.type == ItemType::WallBottom
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
			if (item.type == ItemType::WallTop
				|| item.type == ItemType::WallBottom
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
	context_->beginUndoRecord("Reset Offsets", true, false, false);

	// Go through walls
	for (auto& wall : walls)
	{
		auto side = wall.asSide(context_->map());
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
		if (context_->mapDesc().format == MapFormat::UDMF
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
	if (context_->mapDesc().format == MapFormat::UDMF)
	{
		for (auto& flat : flats)
		{
			auto sector = flat.asSector(context_->map());
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
	if (context_->mapDesc().format != MapFormat::Doom)
	{
		for (auto& item : things)
		{
			auto thing = item.asThing(context_->map());
			if (!thing)
				continue;

			// Reset height
			if (context_->mapDesc().format != MapFormat::UDMF)
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
	context_->endUndoRecord();

	// Editor message
	if (context_->mapDesc().format == MapFormat::UDMF
		&& (game::configuration().featureSupported(UDMFFeature::FlatScaling)
			|| game::configuration().featureSupported(UDMFFeature::SideScaling)
			|| game::configuration().featureSupported(UDMFFeature::TextureScaling)))
		context_->addEditorMessage("Offsets and scaling reset");
	else
		context_->addEditorMessage("Offsets reset");
}

// -----------------------------------------------------------------------------
// Toggles the lower/upper unpegged flag for selected walls depending on [lower]
// -----------------------------------------------------------------------------
void Edit3D::toggleUnpegged(bool lower) const
{
	auto& selection_3d = context_->selection();
	auto  hilight_3d   = context_->hilightItem();
	if (selection_3d.empty() && hilight_3d.index < 0)
		return;

	// Get items to process
	vector<Item> items;
	if (selection_3d.empty())
	{
		if (hilight_3d.type == ItemType::WallTop
			|| hilight_3d.type == ItemType::WallBottom
			|| hilight_3d.type == ItemType::WallMiddle)
			items.push_back(hilight_3d);
	}
	else
	{
		for (auto& item : selection_3d)
		{
			if (item.type == ItemType::WallTop
				|| item.type == ItemType::WallBottom
				|| item.type == ItemType::WallMiddle)
				items.push_back(item);
		}
	}
	if (items.empty())
		return;

	// Begin undo level
	string undo_type = lower ? "Toggle Lower Unpegged" : "Toggle Upper Unpegged";
	context_->beginUndoRecord(undo_type);

	// Go through items
	vector<MapLine*> processed_lines;
	for (auto& item : items)
	{
		auto side = item.asSide(context_->map());
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
		context_->recordPropertyChangeUndoStep(line);
		if (lower)
		{
			bool unpegged = game::configuration().lineBasicFlagSet("dontpegbottom", line, context_->mapDesc().format);
			game::configuration().setLineBasicFlag("dontpegbottom", line, context_->map().currentFormat(), !unpegged);
		}
		else
		{
			bool unpegged = game::configuration().lineBasicFlagSet("dontpegtop", line, context_->mapDesc().format);
			game::configuration().setLineBasicFlag("dontpegtop", line, context_->map().currentFormat(), !unpegged);
		}
	}

	// End undo level
	context_->endUndoRecord(true);

	// Editor message
	if (lower)
		context_->addEditorMessage("Lower Unpegged flag toggled");
	else
		context_->addEditorMessage("Upper Unpegged flag toggled");
}

// -----------------------------------------------------------------------------
// Copies the currently hilighted 3d wall/flat/thing
// -----------------------------------------------------------------------------
void Edit3D::copy(CopyType type)
{
	auto  hl  = context_->selection().hilight();
	auto& map = context_->map();

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
			copy_thing_->copy(thing);
	}

	// Flash
	context_->renderer().animateSelectionChange(hl);

	// Editor message
	if (type == CopyType::TexType)
	{
		if (hl.type == ItemType::Thing)
			context_->addEditorMessage("Copied Thing Type");
		else
			context_->addEditorMessage("Copied Texture");
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
	context_->beginUndoRecord(ptype);

	// Go through items
	auto& selection = context_->selection();
	for (auto& item : selection.selectionOrHilight())
	{
		// Wall
		if (auto side = item.asSide(context_->map()))
		{
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
		else if (auto sector = item.asSector(context_->map()))
		{
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
			if (auto thing = item.asThing(context_->map()))
			{
				// Type
				if (type == CopyType::TexType)
					thing->setType(copy_thing_->type());
			}
		}
	}

	// Editor message
	if (type == CopyType::TexType)
	{
		if (selection.hilight().type == ItemType::Thing)
			context_->addEditorMessage("Pasted Thing Type");
		else
			context_->addEditorMessage("Pasted Texture");
	}

	context_->endUndoRecord(true);
}

// -----------------------------------------------------------------------------
// Pastes previously copied wall/flat info to selection and all adjacent
// walls/flats
// -----------------------------------------------------------------------------
void Edit3D::floodFill(CopyType type) const
{
	// Get items to paste to
	auto& selection = context_->selection();
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
	context_->beginUndoRecord(ptype);

	// Go through items
	for (auto& item : items)
	{
		// Wall
		if (auto side = item.asSide(context_->map()))
		{
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
		else if (auto sector = item.asSector(context_->map()))
		{
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
		context_->addEditorMessage("Floodfilled Texture");
	}

	context_->endUndoRecord(true);
}

// -----------------------------------------------------------------------------
// Opens the thing type browser for the currently selected 3d mode things
// -----------------------------------------------------------------------------
void Edit3D::changeThingType(bool force_highlight) const
{
	// Get things to edit
	auto items = context_->selection().selectionOrHilight();
	if (force_highlight && !vectorContains(items, context_->selection().hilight()))
		if (context_->selection().hilight().type == ItemType::Thing)
			items.push_back(context_->selection().hilight());

	if (items.empty())
		return;

	// Find first selected thing
	MapThing* first_thing = nullptr;
	for (auto& item : items)
		if (auto thing = item.asThing(context_->map()))
		{
			first_thing = thing;
			break;
		}

	// Browse for new thing type
	auto type = first_thing->type();
	if (int new_type = mapeditor::browseThingType(type, context_->map()); new_type >= 0)
	{
		// Begin undo level
		context_->beginUndoRecordLocked("Change Thing Type", true, false, false);

		// Go through items
		for (auto& item : items)
		{
			if (auto thing = item.asThing(context_->map()))
			{
				context_->recordPropertyChangeUndoStep(thing);
				thing->setType(new_type);
			}
		}

		// End undo level
		context_->endUndoRecord();

		// Editor message
		if (items.size() == 1)
			context_->addEditorMessage("Thing type changed");
		else
			context_->addEditorMessage(fmt::format("{} Thing types changed", items.size()));
	}
}

// -----------------------------------------------------------------------------
// Changes the Z height of selected 3d mode things by [amount]
// -----------------------------------------------------------------------------
void Edit3D::changeThingZ(int amount) const
{
	// Ignore for doom format
	if (context_->map().currentFormat() == MapFormat::Doom)
		return;

	// Go through 3d selection
	auto& selection_3d = context_->selection();
	for (auto& item : selection_3d)
	{
		// Check if thing
		if (auto thing = item.asThing(context_->map()))
		{
			// Change z height
			context_->recordPropertyChangeUndoStep(thing);
			double z = thing->zPos();
			z += amount;
			thing->setZ(z);
		}
	}
}

// -----------------------------------------------------------------------------
// Deletes any selected 3d mode things
// -----------------------------------------------------------------------------
void Edit3D::deleteThing() const
{
	// Get thing indices to delete
	vector<unsigned> delete_things;
	for (const auto& thing : context_->selection().selectedThings())
		delete_things.push_back(thing->index());

	if (delete_things.empty())
		return;

	context_->beginUndoRecord("Delete Thing(s)", false, false, true);
	context_->map().removeThings(delete_things);
	context_->endUndoRecord();

	context_->clearSelection();
}

// -----------------------------------------------------------------------------
// Changes scaling for the currently selected walls/flats.
// X scale if [x] is true, y scale otherwise
// -----------------------------------------------------------------------------
void Edit3D::changeScale(double amount, bool x) const
{
	using game::UDMFFeature;

	// Get items to process
	vector<Item> items;
	auto&        selection_3d = context_->selection();
	auto         hilight_3d   = context_->hilightItem();
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
	context_->beginUndoRecordLocked("Change Scale", true, false, false);

	// Go through selection
	for (auto& item : items)
	{
		bool is_wall = false;

		// Wall
		if (game::configuration().featureSupported(UDMFFeature::SideScaling)
			|| game::configuration().featureSupported(UDMFFeature::TextureScaling))
		{
			if (auto side = item.asSide(context_->map()))
			{
				is_wall = true;

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
		if (!is_wall && game::configuration().featureSupported(UDMFFeature::FlatScaling))
		{
			if (auto sector = item.asSector(context_->map()))
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
	context_->endUndoRecord(true);

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
	vector<Item> items;
	auto&        selection_3d = context_->selection();
	auto         hilight_3d   = context_->hilightItem();
	auto&        map          = context_->map();
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
	context_->beginUndoRecordLocked("Change Height", true, false, false);

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
			side->setFloatProperty(ofs, offset + static_cast<float>(amount));
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
	context_->endUndoRecord();

	// Editor message
	if (!items.empty())
	{
		if (amount > 0)
			context_->addEditorMessage(fmt::format("Height increased by {}", amount));
		else
			context_->addEditorMessage(fmt::format("Height decreased by {}", -amount));
	}
}

// -----------------------------------------------------------------------------
// Opens the texture browser for the currently selected 3d mode walls and/or
// floors
// -----------------------------------------------------------------------------
void Edit3D::changeTexture(bool force_highlight) const
{
	// Check for selection or hilight
	auto selection = context_->selection().selectionOrHilight();
	if (force_highlight && !vectorContains(selection, context_->selection().hilight()))
	{
		auto base_type = baseItemType(context_->selection().hilight().type);
		if (base_type == ItemType::Side || base_type == ItemType::Sector)
			selection.push_back(context_->selection().hilight());
	}
	if (selection.empty())
		return;

	// Get initial texture
	string tex;
	auto   type  = TextureType::Texture;
	auto&  first = selection[0];
	auto&  map   = context_->map();
	if (auto sector = first.asSector(map))
	{
		type = TextureType::Flat;

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
		context_->beginUndoRecord("Change Texture", true, false, false);

		// Apply to flats
		if (mix || type == TextureType::Flat)
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
		if (mix || type == TextureType::Texture)
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
		context_->endUndoRecord();
	}
}

// -----------------------------------------------------------------------------
// Clear the texture property
// -----------------------------------------------------------------------------
void Edit3D::deleteTexture() const
{
	// Begin undo level
	context_->beginUndoRecord("Delete Texture", true, false, false);

	auto& map = context_->map();
	for (auto& item : context_->selection().selectionOrHilight())
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

	// End undo level
	context_->endUndoRecord();
}

// -----------------------------------------------------------------------------
// Browse to change the current highlight's texture (if it's a wall or flat), or
// type (if it's a thing)
// -----------------------------------------------------------------------------
void Edit3D::changeTextureOrType() const
{
	if (!context_->selection().hasHilight())
		return;

	if (auto hl = context_->selection().hilight(); hl.type == ItemType::Thing)
		changeThingType(true);
	else
		changeTexture(true);
}

// -----------------------------------------------------------------------------
// Opens a dialog containing a MapObjectPropsPanel to edit properties for all
// selected (or hilighted) walls' lines
// -----------------------------------------------------------------------------
void Edit3D::editWallProperties() const
{
	auto selection = context_->selection().selectedItems();
	if (selection.empty() && context_->selection().hasHilight())
		selection.push_back(context_->selection().hilight());
	if (selection.empty())
		return;

	vector<MapObject*> lines;
	for (auto item : selection)
	{
		if (item.type == ItemType::WallTop || item.type == ItemType::WallMiddle || item.type == ItemType::WallBottom)
		{
			if (auto side = item.asSide(context_->map()))
				vectorAddUnique(lines, static_cast<MapObject*>(side->parentLine()));
		}
	}

	// Begin recording undo level
	context_->beginUndoRecord("Property Edit (Line)");
	for (auto item : lines)
		context_->recordPropertyChangeUndoStep(item);

	bool done = mapeditor::editObjectProperties(lines);
	if (done)
	{
		context_->renderer().forceUpdate();
		context_->updateDisplay();
	}

	// End undo level
	context_->endUndoRecord(done);
}

// -----------------------------------------------------------------------------
// Opens a dialog containing a MapObjectPropsPanel to edit properties for all
// selected (or hilighted) flats' sectors
// -----------------------------------------------------------------------------
void Edit3D::editFlatProperties() const
{
	auto selection = context_->selection().selectedItems();
	if (selection.empty() && context_->selection().hasHilight())
		selection.push_back(context_->selection().hilight());
	if (selection.empty())
		return;

	vector<MapObject*> sectors;
	for (auto item : selection)
	{
		if (item.type == ItemType::Floor || item.type == ItemType::Ceiling)
		{
			if (auto sector = item.asSector(context_->map()))
				vectorAddUnique(sectors, static_cast<MapObject*>(sector));
		}
	}

	// Begin recording undo level
	context_->beginUndoRecord("Property Edit (Sector)");
	for (auto item : sectors)
		context_->recordPropertyChangeUndoStep(item);

	bool done = mapeditor::editObjectProperties(sectors);
	if (done)
	{
		context_->renderer().forceUpdate();
		context_->updateDisplay();
	}

	// End undo level
	context_->endUndoRecord(done);
}

// -----------------------------------------------------------------------------
// Opens a dialog containing a MapObjectPropsPanel to edit properties for all
// selected (or hilighted) things
// -----------------------------------------------------------------------------
void Edit3D::editThingProperties() const
{
	auto selection = context_->selection().selectedItems();
	if (selection.empty() && context_->selection().hasHilight())
		selection.push_back(context_->selection().hilight());
	if (selection.empty())
		return;

	vector<MapObject*> things;
	for (auto item : selection)
	{
		if (item.type == ItemType::Thing)
		{
			if (auto thing = item.asThing(context_->map()))
				vectorAddUnique(things, static_cast<MapObject*>(thing));
		}
	}

	// Begin recording undo level
	context_->beginUndoRecord("Property Edit (Thing)");
	for (auto item : things)
		context_->recordPropertyChangeUndoStep(item);

	bool done = mapeditor::editObjectProperties(things);
	if (done)
	{
		context_->renderer().forceUpdate();
		context_->updateDisplay();
	}

	// End undo level
	context_->endUndoRecord(done);
}

// -----------------------------------------------------------------------------
// Returns a list of all walls or flats adjacent to [item].
// Adjacent meaning connected and sharing a texture
// -----------------------------------------------------------------------------
vector<Item> Edit3D::getAdjacent(Item item) const
{
	vector<Item> list;

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
bool Edit3D::wallMatches(const MapSide* side, ItemType part, string_view tex)
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
void Edit3D::getAdjacentWalls(Item start_item, vector<Item>& list) const
{
	// Keep a set of visited items to avoid infinite loops
	std::set<std::pair<int, ItemType>> visited;

	// Initialize visited set with items already in the list
	for (auto& i : list)
		visited.insert({ i.index, i.type });

	// Queue for breadth-first search of adjacent walls
	std::queue<Item> queue;
	if (!visited.contains({ start_item.index, start_item.type }))
		queue.push(start_item);

	while (!queue.empty())
	{
		auto item = queue.front();
		queue.pop();

		// Skip if already visited
		if (visited.contains({ item.index, item.type }))
			continue;
		visited.insert({ item.index, item.type });
		list.push_back(item);

		// Get side
		auto side = item.asSide(context_->map());
		if (!side)
			continue;

		// Get line
		auto line = side->parentLine();
		if (!line)
			continue;

		// Get texture to check for match
		string tex;
		if (item.type == ItemType::WallBottom)
			tex = side->texLower();
		else if (item.type == ItemType::WallMiddle)
			tex = side->texMiddle();
		else
			tex = side->texUpper();

		// Lambda to enqueue all matching wall parts from a given side
		auto enqueue_side = [&](const MapSide* s)
		{
			if (!s)
				return;
			if (wallMatches(s, ItemType::WallTop, tex))
				queue.emplace(static_cast<int>(s->index()), ItemType::WallTop);
			if (wallMatches(s, ItemType::WallMiddle, tex))
				queue.emplace(static_cast<int>(s->index()), ItemType::WallMiddle);
			if (wallMatches(s, ItemType::WallBottom, tex))
				queue.emplace(static_cast<int>(s->index()), ItemType::WallBottom);
		};

		// Go through attached lines (vertex 1)
		for (unsigned a = 0; a < line->v1()->nConnectedLines(); a++)
		{
			auto oline = line->v1()->connectedLine(a);
			if (!oline || oline == line)
				continue;
			enqueue_side(oline->s1());
			enqueue_side(oline->s2());
		}

		// Go through attached lines (vertex 2)
		for (unsigned a = 0; a < line->v2()->nConnectedLines(); a++)
		{
			auto oline = line->v2()->connectedLine(a);
			if (!oline || oline == line)
				continue;
			enqueue_side(oline->s1());
			enqueue_side(oline->s2());
		}
	}
}

// -----------------------------------------------------------------------------
// Adds all walls and flats adjacent to [item] to [list].
// Adjacent meaning connected and sharing a texture
// -----------------------------------------------------------------------------
void Edit3D::getAdjacentFlats(Item item, vector<Item>& list) const
{
	// Check item
	if (item.index < 0 || (item.type != ItemType::Floor && item.type != ItemType::Ceiling))
		return;

	// Add item
	list.push_back(item);

	// Get initial sector
	auto sector = item.asSector(context_->map());
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
			if (i.type == item.type && std::cmp_equal(i.index, osector->index()))
			{
				listed = true;
				break;
			}
		}

		// Recursively list adjacent flats
		if (!listed)
		{
			list.push_back(item);
			getAdjacentFlats({ static_cast<int>(osector->index()), item.type }, list);
		}
	}
}
