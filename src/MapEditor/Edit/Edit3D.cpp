
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    Edit3D.cpp
 * Description: Map Editor 3D mode editing functionality
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "Edit3D.h"
#include "General/UndoRedo.h"
#include "MapEditor/GameConfiguration/GameConfiguration.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapTextureManager.h"
#include "MapEditor/UI/MapCanvas.h"
#include "MapEditor/UndoSteps.h"
#include "Utility/MathStuff.h"

using MapEditor::ItemType;


/*******************************************************************
 * EDIT3D CLASS FUNCTIONS
 *******************************************************************/

/* Edit3D::Edit3D
 * Edit3D class constructor
 *******************************************************************/
Edit3D::Edit3D(MapEditContext& context) :
	context_{ context },
	link_light_{ false },
	link_offset_{ false }
{
	undo_manager_ = std::make_unique<UndoManager>(&context.map());
}

/* Edit3D::selectAdjacent
 * Selects all adjacent walls or flats to [item]
 *******************************************************************/
void Edit3D::selectAdjacent(MapEditor::Item item) const
{
	// Check item
	if (item.index < 0)
		return;

	// Select every adjacent item
	context_.selection().select(getAdjacent(item));
	context_.selectionUpdated();
}

/* Edit3D::changeSectorLight
 * Changes the light level of selected sectors by [amount]
 *******************************************************************/
void Edit3D::changeSectorLight(int amount) const
{
	// Get items to process
	vector<MapEditor::Item> items;
	auto& selection_3d = context_.selection();
	auto hilight_3d = context_.hilightItem();
	if (selection_3d.empty() && hilight_3d.index >= 0 && hilight_3d.type != ItemType::Thing)
		items.push_back(hilight_3d);
	else
	{
		for (unsigned a = 0; a < selection_3d.size(); a++)
		{
			if (selection_3d[a].type != ItemType::Thing)
				items.push_back(selection_3d[a]);
		}
	}
	if (items.empty())
		return;

	// Begin undo level
	context_.beginUndoRecordLocked("Change Sector Light", true, false, false);

	// Go through items
	std::set<MapObject*> processed;
	for (unsigned a = 0; a < items.size(); a++)
	{
		// Wall
		if (items[a].type == ItemType::WallBottom ||
			items[a].type == ItemType::WallMiddle ||
			items[a].type == ItemType::WallTop)
		{
			// Get side
			auto side = context_.map().getSide(items[a].index);
			if (!side) continue;
			auto sector = side->getSector();
			if (!sector) continue;

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
			int current_light = side->getLight();
			if (current_light == 255 && amount < -1)
				amount++;

			// Change wall or sector light level
			if (link_light_)
				sector->changeLight(amount);
			else
				side->changeLight(amount);
		}

		// Flat
		if (items[a].type == ItemType::Floor || items[a].type == ItemType::Ceiling)
		{
			// Get sector
			auto s = context_.map().getSector(items[a].index);
			int where = 0;
			if (items[a].type == ItemType::Floor && !link_light_)
			where = 1;
			else if (items[a].type == ItemType::Ceiling && !link_light_)
			where = 2;

			// Check for decrease when light = 255
			if (s->getLight(where) == 255 && amount < -1)
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
	if (items.size() > 0)
	{
		if (amount > 0)
			context_.addEditorMessage(S_FMT("Light increased by %d", amount));
		else
			context_.addEditorMessage(S_FMT("Light decreased by %d", -amount));
	}
}

/* Edit3D::changeOffset
 * Changes the offset of selected walls by [amount]. X axis if [x] is
 * true, otherwise Y axis
 *******************************************************************/
void Edit3D::changeOffset(int amount, bool x) const
{
	// Get items to process
	vector<MapEditor::Item> items;
	auto& selection_3d = context_.selection();
	auto hilight_3d = context_.hilightItem();
	if (selection_3d.empty())
	{
		if (hilight_3d.index >= 0 && hilight_3d.type != ItemType::Thing)
			items.push_back(hilight_3d);
	}
	else
	{
		for (unsigned a = 0; a < selection_3d.size(); a++)
		{
			if (selection_3d[a].type != ItemType::Thing)
				items.push_back(selection_3d[a]);
		}
	}
	if (items.empty())
		return;

	// Begin undo level
	context_.beginUndoRecordLocked("Change Offset", true, false, false);

	// Go through items
	vector<int> done;
	bool changed = false;
	for (unsigned a = 0; a < items.size(); a++)
	{
		// Wall
		if (items[a].type >= ItemType::WallTop && items[a].type <= ItemType::WallBottom)
		{
			MapSide* side = context_.map().getSide(items[a].index);

			// If offsets are linked, just change the whole side offset
			if (link_offset_)
			{
				// Check we haven't processed this side already
				if (VECTOR_EXISTS(done, items[a].index))
					continue;

				// Change the appropriate offset
				if (x)
				{
					int offset = side->intProperty("offsetx");
					side->setIntProperty("offsetx", offset + amount);
				}
				else
				{
					int offset = side->intProperty("offsety");
					side->setIntProperty("offsety", offset + amount);
				}

				// Add to done list
				done.push_back(items[a].index);
			}

			// Unlinked offsets
			else
			{
				// Build property string (offset[x/y]_[top/mid/bottom])
				string ofs = "offsetx";
				if (!x) ofs = "offsety";
				if (items[a].type == ItemType::WallBottom)
					ofs += "_bottom";
				else if (items[a].type == ItemType::WallTop)
					ofs += "_top";
				else
					ofs += "_mid";

				// Change the offset
				int offset = side->floatProperty(ofs);
				side->setFloatProperty(ofs, offset + amount);
			}

			changed = true;
		}

		// Flat (UDMF only)
		else
		{
			MapSector* sector = context_.map().getSector(items[a].index);

			if (theGameConfiguration->udmfFlatPanning())
			{
				if (items[a].type == ItemType::Floor)
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
				else if (items[a].type == ItemType::Ceiling)
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
	if (items.size() > 0 && changed)
	{
		string axis = "X";
		if (!x) axis = "Y";

		if (amount > 0)
			context_.addEditorMessage(S_FMT("%s offset increased by %d", axis, amount));
		else
			context_.addEditorMessage(S_FMT("%s offset decreased by %d", axis, -amount));
	}
}

/* Edit3D::changeSectorHeight
 * Changes the height of the selected 3d mode flats by [amount]
 * (walls selected will change ceiling height)
 *******************************************************************/
void Edit3D::changeSectorHeight(int amount) const
{
	// Get items to process
	vector<MapEditor::Item> items;
	auto& selection_3d = context_.selection();
	auto hilight_3d = context_.hilightItem();
	if (selection_3d.empty() && hilight_3d.type != ItemType::Thing && hilight_3d.index >= 0)
		items.push_back(hilight_3d);
	else
	{
		for (unsigned a = 0; a < selection_3d.size(); a++)
		{
			if (selection_3d[a].type != ItemType::Thing)
				items.push_back(selection_3d[a]);
		}
	}
	if (items.empty())
		return;

	// Begin undo level
	context_.beginUndoRecordLocked("Change Sector Height", true, false, false);

	// Go through items
	vector<int> ceilings;
	for (unsigned a = 0; a < items.size(); a++)
	{
		// Wall (ceiling only for now)
		if (items[a].type == ItemType::WallBottom ||
			items[a].type == ItemType::WallMiddle ||
			items[a].type == ItemType::WallTop)
		{
			// Get sector
			auto sector = context_.map().getSide(items[a].index)->getSector();

			// Check this sector's ceiling hasn't already been changed
			int index = sector->getIndex();
			if (VECTOR_EXISTS(ceilings, index))
				continue;

			// Change height
			int height = sector->intProperty("heightceiling");
			sector->setIntProperty("heightceiling", height + amount);

			// Set to changed
			ceilings.push_back(index);
		}

		// Floor
		else if (items[a].type == ItemType::Floor)
		{
			// Get sector
			auto sector = context_.map().getSector(items[a].index);

			// Change height
			sector->setFloorHeight(sector->getFloorHeight() + amount);
		}

		// Ceiling
		else if (items[a].type == ItemType::Ceiling)
		{
			// Get sector
			auto sector = context_.map().getSector(items[a].index);

			// Check this sector's ceiling hasn't already been changed
			bool done = false;
			int index = sector->getIndex();
			for (unsigned b = 0; b < ceilings.size(); b++)
			{
				if (ceilings[b] == index)
				{
					done = true;
					break;
				}
			}
			if (done)
				continue;

			// Change height
			sector->setCeilingHeight(sector->getCeilingHeight() + amount);

			// Set to changed
			ceilings.push_back(sector->getIndex());
		}
	}

	// End undo level
	context_.endUndoRecord();

	// Editor message
	if (items.size() > 0)
	{
		if (amount > 0)
			context_.addEditorMessage(S_FMT("Height increased by %d", amount));
		else
			context_.addEditorMessage(S_FMT("Height decreased by %d", -amount));
	}
}

/* Edit3D::autoAlignX
 * Aligns X offsets beginning from the wall selection [start]
 *******************************************************************/
void Edit3D::autoAlignX(MapEditor::Item start) const
{
	// Check start is a wall
	if (start.type != ItemType::WallBottom && start.type != ItemType::WallMiddle && start.type != ItemType::WallTop)
		return;

	// Get starting side
	auto side = context_.map().getSide(start.index);
	if (!side) return;

	// Get texture to match
	string tex;
	if (start.type == ItemType::WallBottom)
		tex = side->stringProperty("texturebottom");
	else if (start.type == ItemType::WallMiddle)
		tex = side->stringProperty("texturemiddle");
	else if (start.type == ItemType::WallTop)
		tex = side->stringProperty("texturetop");

	// Don't try to auto-align a missing texture (every line on the map will
	// probably match)
	if (tex == "-")
		return;

	// Get texture width
	auto gl_tex = MapEditor::textureManager().getTexture(tex, theGameConfiguration->mixTexFlats());
	int tex_width = -1;
	if (gl_tex)
		tex_width = gl_tex->getWidth();

	// Init aligned wall list
	vector<MapEditor::Item> walls_done;

	// Begin undo level
	context_.beginUndoRecord("Auto Align X", true, false, false);

	// Do alignment
	doAlignX(side, side->intProperty("offsetx"), tex, walls_done, tex_width);

	// End undo level
	context_.endUndoRecord();

	// Editor message
	context_.addEditorMessage("Auto-aligned on X axis");
}

/* Edit3D::resetOffsets
 * Resets offsets and scaling for the currently selected wall(s)
 *******************************************************************/
void Edit3D::resetOffsets() const
{
	// Get items to process
	vector<MapEditor::Item> walls;
	vector<MapEditor::Item> flats;
	vector<MapEditor::Item> things;
	auto& selection_3d = context_.selection();
	auto hilight_3d = context_.hilightItem();
	if (selection_3d.size() == 0)
	{
		if (hilight_3d.type == ItemType::WallTop ||
			hilight_3d.type == ItemType::WallBottom ||
			hilight_3d.type == ItemType::WallMiddle)
			walls.push_back(hilight_3d);
		else if (hilight_3d.type == ItemType::Floor || hilight_3d.type == ItemType::Ceiling)
			flats.push_back(hilight_3d);
		else if (hilight_3d.type == ItemType::Thing)
			things.push_back(hilight_3d);
	}
	else
	{
		for (unsigned a = 0; a < selection_3d.size(); a++)
		{
			if (selection_3d[a].type == ItemType::WallTop ||
				selection_3d[a].type == ItemType::WallBottom ||
				selection_3d[a].type == ItemType::WallMiddle)
				walls.push_back(selection_3d[a]);
			else if (selection_3d[a].type == ItemType::Floor || selection_3d[a].type == ItemType::Ceiling)
				flats.push_back(selection_3d[a]);
			else if (selection_3d[a].type == ItemType::Thing)
				things.push_back(selection_3d[a]);
		}
	}
	if (walls.size() == 0 && flats.size() == 0 && things.size() == 0)
		return;

	// Begin undo level
	context_.beginUndoRecord("Reset Offsets", true, false, false);

	// Go through walls
	for (unsigned a = 0; a < walls.size(); a++)
	{
		auto side = context_.map().getSide(walls[a].index);
		if (!side) continue;

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
			if (walls[a].type == ItemType::WallTop)
			{
				side->setFloatProperty("offsetx_top", 0);
				side->setFloatProperty("offsety_top", 0);
			}
			else if (walls[a].type == ItemType::WallMiddle)
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
		if (context_.mapDesc().format == MAP_UDMF && theGameConfiguration->udmfTextureScaling())
		{
			if (walls[a].type == ItemType::WallTop)
			{
				side->setFloatProperty("scalex_top", 1);
				side->setFloatProperty("scaley_top", 1);
			}
			else if (walls[a].type == ItemType::WallMiddle)
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
	if (context_.mapDesc().format == MAP_UDMF)
	{
		for (unsigned a = 0; a < flats.size(); a++)
		{
			auto sector = context_.map().getSector(flats[a].index);
			if (!sector) continue;

			string plane;
			if (flats[a].type == ItemType::Floor)
				plane = "floor";
			else
				plane = "ceiling";

			// Reset offsets, scale, and rotation
			if (theGameConfiguration->udmfFlatPanning())
			{
				sector->setFloatProperty("xpanning" + plane, 0);
				sector->setFloatProperty("ypanning" + plane, 0);
			}
			if (theGameConfiguration->udmfFlatScaling())
			{
				sector->setFloatProperty("xscale" + plane, 1);
				sector->setFloatProperty("yscale" + plane, 1);
			}
			if (theGameConfiguration->udmfFlatRotation())
				sector->setFloatProperty("rotation" + plane, 0);
		}
	}

	// Go through things
	if (context_.mapDesc().format != MAP_DOOM)
	{
		for (unsigned a = 0; a < things.size(); ++a)
		{
			auto thing = context_.map().getThing(things[a].index);
			if (!thing) continue;

			// Reset height
			if (context_.mapDesc().format != MAP_UDMF)
				thing->setIntProperty("height", 0);
			else
			{
				thing->setFloatProperty("height", 0);
				// Reset scale
				if (theGameConfiguration->udmfThingScaling())
				{
					thing->setFloatProperty("scalex", 1);
					thing->setFloatProperty("scaley", 1);
					thing->setFloatProperty("scale", 1);
				}
				// Reset non-angle rotations
				if (theGameConfiguration->udmfThingRotation())
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
	if (context_.mapDesc().format == MAP_UDMF && (theGameConfiguration->udmfFlatScaling() ||
		theGameConfiguration->udmfSideScaling() || theGameConfiguration->udmfTextureScaling()))
		context_.addEditorMessage("Offsets and scaling reset");
	else
		context_.addEditorMessage("Offsets reset");
}

/* Edit3D::toggleUnpegged
 * Toggles the lower/upper unpegged flag for selected walls depending
 * on [lower]
 *******************************************************************/
void Edit3D::toggleUnpegged(bool lower) const
{
	auto& selection_3d = context_.selection();
	auto hilight_3d = context_.hilightItem();
	if (selection_3d.size() == 0 && hilight_3d.index < 0)
		return;

	// Get items to process
	vector<MapEditor::Item> items;
	if (selection_3d.size() == 0)
	{
		if (hilight_3d.type == ItemType::WallTop ||
			hilight_3d.type == ItemType::WallBottom ||
			hilight_3d.type == ItemType::WallMiddle)
			items.push_back(hilight_3d);
	}
	else
	{
		for (unsigned a = 0; a < selection_3d.size(); a++)
		{
			if (selection_3d[a].type == ItemType::WallTop ||
				selection_3d[a].type == ItemType::WallBottom ||
				selection_3d[a].type == ItemType::WallMiddle)
				items.push_back(selection_3d[a]);
		}
	}
	if (items.size() == 0)
		return;

	// Begin undo level
	string undo_type = lower ? "Toggle Lower Unpegged" : "Toggle Upper Unpegged";
	undo_manager_->beginRecord(undo_type);

	// Go through items
	vector<MapLine*> processed_lines;
	for (unsigned a = 0; a < items.size(); a++)
	{
		// Get line
		auto line = context_.map().getSide(items[a].index)->getParentLine();
		if (!line) continue;

		// Skip if line already processed
		if (VECTOR_EXISTS(processed_lines, line))
			continue;
		else
			processed_lines.push_back(line);

		// Toggle flag
		context_.recordPropertyChangeUndoStep(line);
		if (lower)
		{
			bool unpegged = theGameConfiguration->lineBasicFlagSet(
				"dontpegbottom",
				line,
				context_.mapDesc().format
			);
			theGameConfiguration->setLineBasicFlag(
				"dontpegbottom",
				line,
				context_.map().currentFormat(),
				!unpegged
			);
		}
		else
		{
			bool unpegged = theGameConfiguration->lineBasicFlagSet(
				"dontpegtop",
				line,
				context_.mapDesc().format
			);
			theGameConfiguration->setLineBasicFlag(
				"dontpegtop",
				line,
				context_.map().currentFormat(),
				!unpegged
			);
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

/* MapEditContext::copy3d
 * Copies the currently hilighted 3d wall/flat/thing
 *******************************************************************/
void Edit3D::copy(CopyType type)
{
	auto hl = context_.selection().hilight();
	auto& map = context_.map();

	// Check hilight
	if (hl.index < 0)
		return;

	// Upper wall
	else if (hl.type == ItemType::WallTop)
	{
		// Texture
		if (type == CopyType::TexType)
			copy_texture_ = map.getSide(hl.index)->stringProperty("texturetop");
	}

	// Middle wall
	else if (hl.type == ItemType::WallMiddle)
	{
		// Texture
		if (type == CopyType::TexType)
			copy_texture_ = map.getSide(hl.index)->stringProperty("texturemiddle");
	}

	// Lower wall
	else if (hl.type == ItemType::WallBottom)
	{
		// Texture
		if (type == CopyType::TexType)
			copy_texture_ = map.getSide(hl.index)->stringProperty("texturebottom");
	}

	// Floor
	else if (hl.type == ItemType::Floor)
	{
		// Texture
		if (type == CopyType::TexType)
			copy_texture_ = map.getSector(hl.index)->getFloorTex();
	}

	// Ceiling
	else if (hl.type == ItemType::Ceiling)
	{
		// Texture
		if (type == CopyType::TexType)
			copy_texture_ = map.getSector(hl.index)->getCeilingTex();
	}

	// Thing
	else if (hl.type == ItemType::Thing)
	{
		copy_thing_.copy(map.getThing(hl.index));
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

/* MapEditContext::paste3d
 * Pastes previously copied wall/flat/thing info to selection
 *******************************************************************/
void Edit3D::paste(CopyType type)
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
		if (MapEditor::baseItemType(item.type) == ItemType::Side)
		{
			MapSide* side = context_.map().getSide(item.index);
			undo_manager_->recordUndoStep(new MapEditor::PropertyChangeUS(side));

			// Upper wall
			if (item.type == ItemType::WallTop)
			{
				// Texture
				if (type == CopyType::TexType)
					side->setStringProperty("texturetop", copy_texture_);
			}

				// Middle wall
			else if (item.type == ItemType::WallMiddle)
			{
				// Texture
				if (type == CopyType::TexType)
					side->setStringProperty("texturemiddle", copy_texture_);
			}

				// Lower wall
			else if (item.type == ItemType::WallBottom)
			{
				// Texture
				if (type == CopyType::TexType)
					side->setStringProperty("texturebottom", copy_texture_);
			}
		}

		// Flat
		else if (item.type == ItemType::Floor || item.type == ItemType::Ceiling)
		{
			MapSector* sector = context_.map().getSector(item.index);
			undo_manager_->recordUndoStep(new MapEditor::PropertyChangeUS(sector));

			// Floor
			if (item.type == ItemType::Floor)
			{
				// Texture
				if (type == CopyType::TexType)
					sector->setStringProperty("texturefloor", copy_texture_);
			}

			// Ceiling
			if (item.type == ItemType::Ceiling)
			{
				// Texture
				if (type == CopyType::TexType)
					sector->setStringProperty("textureceiling", copy_texture_);
			}
		}

		// Thing
		else if (item.type == ItemType::Thing)
		{
			MapThing* thing = context_.map().getThing(item.index);
			undo_manager_->recordUndoStep(new MapEditor::PropertyChangeUS(thing));

			// Type
			if (type == CopyType::TexType)
				thing->setIntProperty("type", copy_thing_.getType());
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

/* MapEditContext::floodFill3d
 * Pastes previously copied wall/flat/thing info to selection and ad
 *******************************************************************/
void Edit3D::floodFill(CopyType type)
{
	// Get items to paste to
	auto& selection = context_.selection();
	auto items = getAdjacent(selection.hilight());

	// Restrict floodfill to selection, if any
	if (selection.size() > 0)
	{
		for (auto i = items.begin(); i < items.end(); ++i)
		{
			bool found = false;
			for (unsigned a = 0; a < selection.size(); a++)
			{
				if (selection[a].type == i->type && selection[a].index == i->index)
				{
					found = true;
					break;
				}
			}
			if (!found)
				items.erase(i);
		}
	}

	// Begin undo step
	string ptype = "Floodfill textures";
	undo_manager_->beginRecord(ptype);

	// Go through items
	for (unsigned a = 0; a < items.size(); a++)
	{
		// Wall
		if (items[a].type == ItemType::WallTop ||
			items[a].type == ItemType::WallMiddle ||
			items[a].type == ItemType::WallBottom)
		{
			auto side = context_.map().getSide(items[a].index);
			undo_manager_->recordUndoStep(new MapEditor::PropertyChangeUS(side));

			// Upper wall
			if (items[a].type == ItemType::WallTop)
			{
				// Texture
				if (type == CopyType::TexType)
					side->setStringProperty("texturetop", copy_texture_);
			}

				// Middle wall
			else if (items[a].type == ItemType::WallMiddle)
			{
				// Texture
				if (type == CopyType::TexType)
					side->setStringProperty("texturemiddle", copy_texture_);
			}

				// Lower wall
			else if (items[a].type == ItemType::WallBottom)
			{
				// Texture
				if (type == CopyType::TexType)
					side->setStringProperty("texturebottom", copy_texture_);
			}
		}

		// Flat
		else if (items[a].type == ItemType::Floor || items[a].type == ItemType::Ceiling)
		{
			MapSector* sector = context_.map().getSector(items[a].index);
			undo_manager_->recordUndoStep(new MapEditor::PropertyChangeUS(sector));

			// Floor
			if (items[a].type == ItemType::Floor)
			{
				// Texture
				if (type == CopyType::TexType)
					sector->setStringProperty("texturefloor", copy_texture_);
			}

			// Ceiling
			if (items[a].type == ItemType::Ceiling)
			{
				// Texture
				if (type == CopyType::TexType)
					sector->setStringProperty("textureceiling", copy_texture_);
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

/* Edit3D::changeThingZ
 * Changes the Z height of selected 3d mode things by [amount]
 *******************************************************************/
void Edit3D::changeThingZ(int amount) const
{
	// Ignore for doom format
	if (context_.map().currentFormat() == MAP_DOOM)
		return;

	// Go through 3d selection
	auto& selection_3d = context_.selection();
	for (unsigned a = 0; a < selection_3d.size(); a++)
	{
		// Check if thing
		if (selection_3d[a].type == ItemType::Thing)
		{
			MapThing* thing = context_.map().getThing(selection_3d[a].index);
			if (thing)
			{
				// Change z height
				context_.recordPropertyChangeUndoStep(thing);
				double z = thing->intProperty("height");
				z += amount;
				thing->setIntProperty("height", z);
			}
		}
	}
}

/* Edit3D::deleteThing
 * Deletes any selected 3d mode things
 *******************************************************************/
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

/* Edit3D::changeScale
 * Changes scaling for the currently selected walls/flats, x scale if
 * [x] is true, y scale otherwise
 *******************************************************************/
void Edit3D::changeScale(double amount, bool x) const
{
	// Get items to process
	vector<MapEditor::Item> items;
	auto& selection_3d = context_.selection();
	auto hilight_3d = context_.hilightItem();
	if (selection_3d.empty())
	{
		if (hilight_3d.index >= 0 && hilight_3d.type != ItemType::Thing)
			items.push_back(hilight_3d);
	}
	else
	{
		for (unsigned a = 0; a < selection_3d.size(); a++)
		{
			if (selection_3d[a].type != ItemType::Thing)
				items.push_back(selection_3d[a]);
		}
	}
	if (items.empty())
		return;

	// Begin undo level
	context_.beginUndoRecordLocked("Change Scale", true, false, false);

	// Go through selection
	for (unsigned a = 0; a < items.size(); a++)
	{
		// Wall
		if (items[a].type >= ItemType::WallTop && items[a].type <= ItemType::WallBottom &&
			(theGameConfiguration->udmfSideScaling() || theGameConfiguration->udmfTextureScaling()))
		{
			auto side = context_.map().getSide(items[a].index);

			// Build property string (offset[x/y]_[top/mid/bottom])
			string ofs = "scalex";
			if (!x) ofs = "scaley";
			if (theGameConfiguration->udmfTextureScaling())
			{
				if (items[a].type == ItemType::WallBottom)
					ofs += "_bottom";
				else if (items[a].type == ItemType::WallTop)
					ofs += "_top";
				else
					ofs += "_mid";
			}

			// Change the offset
			double scale = side->floatProperty(ofs);
			if (scale + amount > 0)
				side->setFloatProperty(ofs, scale + amount);
		}

		// Flat (UDMF only)
		else if (theGameConfiguration->udmfFlatScaling())
		{
			auto sector = context_.map().getSector(items[a].index);

			// Build property string
			string prop = x ? "xscale" : "yscale";
			prop += (items[a].type == ItemType::Floor) ? "floor" : "ceiling";

			// Set
			double scale = sector->floatProperty(prop);
			if (scale + amount > 0)
				sector->setFloatProperty(prop, scale + amount);
		}
	}

	// End undo record
	context_.endUndoRecord(true);

	// Editor message
}

/* Edit3D::changeHeight
 * Changes the height of objects, depending on type:
 * Things: Z height
 * Flat: height
 * Wall: vertical offset
 *******************************************************************/
void Edit3D::changeHeight(int amount) const
{
	// Get items to process
	vector<MapEditor::Item> items;
	auto& selection_3d = context_.selection();
	auto hilight_3d = context_.hilightItem();
	auto& map = context_.map();
	if (selection_3d.empty() && hilight_3d.index >= 0)
	{
		if (hilight_3d.type != ItemType::Thing || map.currentFormat() != MAP_DOOM)
			items.push_back(hilight_3d);
	}
	else for (unsigned a = 0; a < selection_3d.size(); a++)
	{
		if (selection_3d[a].type != ItemType::Thing || map.currentFormat() != MAP_DOOM)
			items.push_back(selection_3d[a]);
	}
	if (items.empty())
		return;

	// Begin undo level
	context_.beginUndoRecordLocked("Change Height", true, false, false);

	// Go through items
	for (unsigned a = 0; a < items.size(); a++)
	{
		auto type = items[a].type;

		// Thing
		if (type == ItemType::Thing)
		{
			MapThing* thing = map.getThing(items[a].index);
			if (thing)
			{
				double z = thing->intProperty("height");
				z += amount;
				thing->setIntProperty("height", z);
			}
		}

		// Wall
		if (type == ItemType::WallBottom ||
			type == ItemType::WallMiddle ||
			type == ItemType::WallTop)
		{
			auto side = map.getSide(items[a].index);

			if (side)
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
				else if (items[a].type == ItemType::WallBottom)
					ofs += "_bottom";
				else if (items[a].type == ItemType::WallTop)
					ofs += "_top";
				else
					ofs += "_mid";

				// Change the offset
				float offset = side->floatProperty(ofs);
				side->setFloatProperty(ofs, offset + amount);
			}
		}

		// Floor
		else if (type == ItemType::Floor)
		{
			// Get sector
			auto sector = map.getSector(items[a].index);

			// Change height
			if (sector)
				sector->setFloorHeight(sector->getFloorHeight() + amount);
		}

		// Ceiling
		else if (type == ItemType::Ceiling)
		{
			// Get sector
			auto sector = map.getSector(items[a].index);

			// Change height
			if (sector)
				sector->setCeilingHeight(sector->getCeilingHeight() + amount);
		}
	}

	// End undo level
	context_.endUndoRecord();

	// Editor message
	if (items.size() > 0)
	{
		if (amount > 0)
			context_.addEditorMessage(S_FMT("Height increased by %d", amount));
		else
			context_.addEditorMessage(S_FMT("Height decreased by %d", -amount));
	}
}

/* Edit3D::changeTexture
 * Opens the texture browser for the currently selected 3d mode walls
 * and/or floors
 *******************************************************************/
void Edit3D::changeTexture() const
{
	// Check for selection or hilight
	auto selection = context_.selection().selectionOrHilight();
	if (selection.empty())
		return;

	// Get initial texture
	string tex;
	int type = 0;
	auto& first = selection[0];
	auto& map = context_.map();
	if (first.type == MapEditor::ItemType::Floor)
	{
		tex = map.getSector(first.index)->getFloorTex();
		type = 1;
	}
	else if (first.type == MapEditor::ItemType::Ceiling)
	{
		tex = map.getSector(first.index)->getCeilingTex();
		type = 1;
	}
	else if (first.type == MapEditor::ItemType::WallBottom)
		tex = map.getSide(first.index)->stringProperty("texturebottom");
	else if (first.type == MapEditor::ItemType::WallMiddle)
		tex = map.getSide(first.index)->stringProperty("texturemiddle");
	else if (first.type == MapEditor::ItemType::WallTop)
		tex = map.getSide(first.index)->stringProperty("texturetop");

	// Open texture browser
	tex = MapEditor::browseTexture(tex, type, map);
	if (!tex.empty())
	{
		bool mix = theGameConfiguration->mixTexFlats();
		MapEditor::Item hl = context_.hilightItem();

		// Begin undo level
		context_.beginUndoRecord("Change Texture", true, false, false);

		// Apply to flats
		if (mix || type == 1)
		{
			// Selection
			if (selection.size() > 0)
			{
				for (unsigned a = 0; a < selection.size(); a++)
				{
					if (selection[a].type == MapEditor::ItemType::Floor)
						map.getSector(selection[a].index)->setStringProperty("texturefloor", tex);
					else if (selection[a].type == MapEditor::ItemType::Ceiling)
						map.getSector(selection[a].index)->setStringProperty("textureceiling", tex);
				}
			}
			else if (hl.index >= 0)
			{
				// Hilight if no selection
				if (hl.type == MapEditor::ItemType::Floor)
					map.getSector(hl.index)->setStringProperty("texturefloor", tex);
				else if (hl.type == MapEditor::ItemType::Ceiling)
					map.getSector(hl.index)->setStringProperty("textureceiling", tex);
			}
		}

		// Apply to walls
		if (mix || type == 0)
		{
			// Selection
			if (selection.size() > 0)
			{
				for (unsigned a = 0; a < selection.size(); a++)
				{
					if (selection[a].type == MapEditor::ItemType::WallBottom)
						map.getSide(selection[a].index)->setStringProperty("texturebottom", tex);
					else if (selection[a].type == MapEditor::ItemType::WallMiddle)
						map.getSide(selection[a].index)->setStringProperty("texturemiddle", tex);
					else if (selection[a].type == MapEditor::ItemType::WallTop)
						map.getSide(selection[a].index)->setStringProperty("texturetop", tex);
				}
			}
			else if (hl.index >= 0)
			{
				// Hilight if no selection
				if (hl.type == MapEditor::ItemType::WallBottom)
					map.getSide(hl.index)->setStringProperty("texturebottom", tex);
				else if (hl.type == MapEditor::ItemType::WallMiddle)
					map.getSide(hl.index)->setStringProperty("texturemiddle", tex);
				else if (hl.type == MapEditor::ItemType::WallTop)
					map.getSide(hl.index)->setStringProperty("texturetop", tex);
			}
		}

		// End undo level
		context_.endUndoRecord();
	}
}

/* Edit3D::getAdjacent
 * Returns a list of all walls or flats adjacent to [item]. Adjacent
 * meaning connected and sharing a texture
 *******************************************************************/
vector<MapEditor::Item> Edit3D::getAdjacent(MapEditor::Item item) const
{
	vector<MapEditor::Item> list;

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

/* Edit3D::wallMatches
 * Returns true if the texture [part] of [side] matches [tex]
 *******************************************************************/
bool Edit3D::wallMatches(MapSide* side, ItemType part, string tex)
{
	// Check for blank texture where it isn't needed
	if (tex == "-")
	{
		auto line = side->getParentLine();
		int needed = line->needsTexture();
		if (side == line->s1())
		{
			if (part == ItemType::WallTop && (needed & TEX_FRONT_UPPER) == 0)
				return false;
			if (part == ItemType::WallMiddle && (needed & TEX_FRONT_MIDDLE) == 0)
				return false;
			if (part == ItemType::WallBottom && (needed & TEX_FRONT_LOWER) == 0)
				return false;
		}
		else if (side == line->s2())
		{
			if (part == ItemType::WallTop && (needed & TEX_BACK_UPPER) == 0)
				return false;
			if (part == ItemType::WallMiddle && (needed & TEX_BACK_MIDDLE) == 0)
				return false;
			if (part == ItemType::WallBottom && (needed & TEX_BACK_LOWER) == 0)
				return false;
		}
	}

	// Check texture
	if (part == ItemType::WallTop && side->stringProperty("texturetop") != tex)
		return false;
	if (part == ItemType::WallMiddle && side->stringProperty("texturemiddle") != tex)
		return false;
	if (part == ItemType::WallBottom && side->stringProperty("texturebottom") != tex)
		return false;

	return true;
}

/* Edit3D::getAdjacentWalls
 * Adds all adjacent walls to [item] to [list]. Adjacent meaning
 * connected and sharing a texture
 *******************************************************************/
void Edit3D::getAdjacentWalls(MapEditor::Item item, vector<MapEditor::Item>& list) const
{
	// Add item to list if needed
	for (unsigned a = 0; a < list.size(); a++)
	{
		if (list[a].type == item.type && list[a].index == item.index)
			return;
	}
	list.push_back(item);

	// Get initial side
	auto side = context_.map().getSide(item.index);
	if (!side)
		return;

	// Get initial line
	auto line = side->getParentLine();
	if (!line)
		return;

	// Get texture to match
	string tex;
	if (item.type == ItemType::WallBottom)
		tex = side->stringProperty("texturebottom");
	else if (item.type == ItemType::WallMiddle)
		tex = side->stringProperty("texturemiddle");
	else
		tex = side->stringProperty("texturetop");

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
				getAdjacentWalls({ (int)side1->getIndex(), ItemType::WallTop }, list);

			// Middle texture
			if (wallMatches(side1, ItemType::WallMiddle, tex))
				getAdjacentWalls({ (int)side1->getIndex(), ItemType::WallMiddle }, list);

			// Lower texture
			if (wallMatches(side1, ItemType::WallBottom, tex))
				getAdjacentWalls({ (int)side1->getIndex(), ItemType::WallBottom }, list);
		}

		// Back side
		if (side2)
		{
			// Upper texture
			if (wallMatches(side2, ItemType::WallTop, tex))
				getAdjacentWalls({ (int)side2->getIndex(), ItemType::WallTop }, list);

			// Middle texture
			if (wallMatches(side2, ItemType::WallMiddle, tex))
				getAdjacentWalls({ (int)side2->getIndex(), ItemType::WallMiddle }, list);

			// Lower texture
			if (wallMatches(side2, ItemType::WallBottom, tex))
				getAdjacentWalls({ (int)side2->getIndex(), ItemType::WallBottom }, list);
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
				getAdjacentWalls({ (int)side1->getIndex(), ItemType::WallTop }, list);

			// Middle texture
			if (wallMatches(side1, ItemType::WallMiddle, tex))
				getAdjacentWalls({ (int)side1->getIndex(), ItemType::WallMiddle }, list);

			// Lower texture
			if (wallMatches(side1, ItemType::WallBottom, tex))
				getAdjacentWalls({ (int)side1->getIndex(), ItemType::WallBottom }, list);
		}

		// Back side
		if (side2)
		{
			// Upper texture
			if (wallMatches(side2, ItemType::WallTop, tex))
				getAdjacentWalls({ (int)side2->getIndex(), ItemType::WallTop }, list);

			// Middle texture
			if (wallMatches(side2, ItemType::WallMiddle, tex))
				getAdjacentWalls({ (int)side2->getIndex(), ItemType::WallMiddle }, list);

			// Lower texture
			if (wallMatches(side2, ItemType::WallBottom, tex))
				getAdjacentWalls({ (int)side2->getIndex(), ItemType::WallBottom }, list);
		}
	}
}

/* Edit3D::getAdjacentFlats
 * Adds all walls and flats adjacent to [item] to [list]. Adjacent
 * meaning connected and sharing a texture
 *******************************************************************/
void Edit3D::getAdjacentFlats(MapEditor::Item item, vector<MapEditor::Item>& list) const
{
	// Check item
	if (item.index < 0 || (item.type != ItemType::Floor && item.type != ItemType::Ceiling))
		return;

	// Add item
	list.push_back(item);

	// Get initial sector
	auto sector = context_.map().getSector(item.index);
	if (!sector) return;

	// Go through sector lines
	vector<MapLine*> lines;
	sector->getLines(lines);
	for (auto& line : lines)
	{
		// Get sector on opposite side
		auto osector = (line->frontSector() == sector) ? line->backSector() : line->frontSector();

		// Skip if no sector
		if (!osector || osector == sector)
			continue;

		// Check for match
		plane_t this_plane, other_plane;
		if (item.type == ItemType::Floor)
		{
			// Check sector floor texture
			if (osector->getFloorTex() != sector->getFloorTex())
				continue;

			this_plane = sector->getFloorPlane();
			other_plane = osector->getFloorPlane();
		}
		else
		{
			// Check sector ceiling texture
			if (osector->getCeilingTex() != sector->getCeilingTex())
				continue;

			this_plane = sector->getCeilingPlane();
			other_plane = osector->getCeilingPlane();
		}

		// Check that planes meet
		auto left = line->v1()->getPoint(0);
		auto right = line->v2()->getPoint(0);

		double this_left_z = this_plane.height_at(left);
		double other_left_z = other_plane.height_at(left);
		if (fabs(this_left_z - other_left_z) > 1)
			continue;

		double this_right_z = this_plane.height_at(right);
		double other_right_z = other_plane.height_at(right);
		if (fabs(this_right_z - other_right_z) > 1)
			continue;

		// Check flat isn't already listed
		bool listed = false;
		for (auto& i : list)
		{
			if (i.type == item.type && i.index == osector->getIndex())
			{
				listed = true;
				break;
			}
		}

		// Recursively list adjacent flats
		if (!listed)
		{
			list.push_back(item);
			getAdjacentFlats({ (int)osector->getIndex(), item.type }, list);
		}
	}
}

/* Edit3D::doAlignX
 * Recursive function to align textures on the x axis
 *******************************************************************/
void Edit3D::doAlignX(MapSide* side, int offset, string tex, vector<MapEditor::Item>& walls_done, int tex_width)
{
	// Check if this wall has already been processed
	for (unsigned a = 0; a < walls_done.size(); a++)
	{
		if (walls_done[a].index == side->getIndex())
			return;
	}

	// Add to 'done' list
	walls_done.push_back({ (int)side->getIndex(), ItemType::WallMiddle });

	// Wrap offset
	if (tex_width > 0)
	{
		while (offset >= tex_width)
			offset -= tex_width;
	}

	// Set offset
	side->setIntProperty("offsetx", offset);

	// Get 'next' vertex
	auto line = side->getParentLine();
	auto vertex = line->v2();
	if (side == line->s2())
		vertex = line->v1();

	// Get integral length of line
	int intlen = MathStuff::round(line->getLength());

	// Go through connected lines
	for (unsigned a = 0; a < vertex->nConnectedLines(); a++)
	{
		auto l = vertex->connectedLine(a);

		// First side
		auto s = l->s1();
		if (s)
		{
			// Check for matching texture
			if (s->stringProperty("texturetop") == tex ||
				s->stringProperty("texturemiddle") == tex ||
				s->stringProperty("texturebottom") == tex)
				doAlignX(s, offset + intlen, tex, walls_done, tex_width);
		}

		// Second side
		s = l->s2();
		if (s)
		{
			// Check for matching texture
			if (s->stringProperty("texturetop") == tex ||
				s->stringProperty("texturemiddle") == tex ||
				s->stringProperty("texturebottom") == tex)
				doAlignX(s, offset + intlen, tex, walls_done, tex_width);
		}
	}
}
