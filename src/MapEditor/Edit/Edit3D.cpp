
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
#include "MapEditor/SLADEMap/MapLine.h"
#include "MapEditor/SLADEMap/MapSide.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/GameConfiguration/GameConfiguration.h"
#include "Utility/MathStuff.h"
#include "MapEditor/MapTextureManager.h"
#include "General/UndoRedo.h"


/*******************************************************************
 * EDIT3D CLASS FUNCTIONS
 *******************************************************************/

/* Edit3D::Edit3D
 * Edit3D class constructor
 *******************************************************************/
Edit3D::Edit3D(MapEditContext& context) :
	context{ context },
	link_light{ false },
	link_offset{ false }
{
	undo_manager = std::make_unique<UndoManager>(&context.getMap());
}

/* Edit3D::selectAdjacent
 * Selects all adjacent walls or flats to [item]
 *******************************************************************/
void Edit3D::selectAdjacent(MapEditor::Item item) const
{
	// Check item
	if (item.index < 0)
		return;

	// Select every list item
	for (auto& i : getAdjacent(item))
		context.selection().select(i);
}

/* Edit3D::changeSectorLight
 * Changes the light level of selected sectors by [amount]
 *******************************************************************/
void Edit3D::changeSectorLight(int amount) const
{
	// Get items to process
	vector<MapEditor::Item> items;
	auto& selection_3d = context.selection();
	auto hilight_3d = context.hilightItem();
	if (selection_3d.empty() && hilight_3d.index >= 0 && hilight_3d.type != MapEditor::ItemType::Thing)
		items.push_back(hilight_3d);
	else
	{
		for (unsigned a = 0; a < selection_3d.size(); a++)
		{
			if (selection_3d[a].type != MapEditor::ItemType::Thing)
				items.push_back(selection_3d[a]);
		}
	}
	if (items.empty())
		return;

	// Begin undo level
	context.beginUndoRecordLocked("Change Sector Light", true, false, false);

	// Go through items
	std::set<MapObject*> processed;
	for (unsigned a = 0; a < items.size(); a++)
	{
		// Wall
		if (items[a].type == MapEditor::ItemType::WallBottom ||
			items[a].type == MapEditor::ItemType::WallMiddle ||
			items[a].type == MapEditor::ItemType::WallTop)
		{
			// Get side
			auto side = context.getMap().getSide(items[a].index);
			if (!side) continue;
			auto sector = side->getSector();
			if (!sector) continue;

			if (link_light)
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
			if (link_light)
				sector->changeLight(amount);
			else
				side->changeLight(amount);
		}

		// Flat
		if (items[a].type == MapEditor::ItemType::Floor || items[a].type == MapEditor::ItemType::Ceiling)
		{
			// Get sector
			auto s = context.getMap().getSector(items[a].index);
			int where = 0;
			if (items[a].type == MapEditor::ItemType::Floor && !link_light)
			where = 1;
			else if (items[a].type == MapEditor::ItemType::Ceiling && !link_light)
			where = 2;

			// Check for decrease when light = 255
			if (s->getLight(where) == 255 && amount < -1)
				amount++;

			// Ignore if sector already processed
			if (link_light)
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
	context.endUndoRecord();

	// Editor message
	if (items.size() > 0)
	{
		if (amount > 0)
			context.addEditorMessage(S_FMT("Light increased by %d", amount));
		else
			context.addEditorMessage(S_FMT("Light decreased by %d", -amount));
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
	auto& selection_3d = context.selection();
	auto hilight_3d = context.hilightItem();
	if (selection_3d.empty())
	{
		if (hilight_3d.index >= 0 && hilight_3d.type != MapEditor::ItemType::Thing)
			items.push_back(hilight_3d);
	}
	else
	{
		for (unsigned a = 0; a < selection_3d.size(); a++)
		{
			if (selection_3d[a].type != MapEditor::ItemType::Thing)
				items.push_back(selection_3d[a]);
		}
	}
	if (items.empty())
		return;

	// Begin undo level
	context.beginUndoRecordLocked("Change Offset", true, false, false);

	// Go through items
	vector<int> done;
	bool changed = false;
	for (unsigned a = 0; a < items.size(); a++)
	{
		// Wall
		if (items[a].type >= MapEditor::ItemType::WallTop && items[a].type <= MapEditor::ItemType::WallBottom)
		{
			MapSide* side = context.getMap().getSide(items[a].index);

			// If offsets are linked, just change the whole side offset
			if (link_offset)
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
				if (items[a].type == MapEditor::ItemType::WallBottom)
					ofs += "_bottom";
				else if (items[a].type == MapEditor::ItemType::WallTop)
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
			MapSector* sector = context.getMap().getSector(items[a].index);

			if (theGameConfiguration->udmfFlatPanning())
			{
				if (items[a].type == MapEditor::ItemType::Floor)
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
				else if (items[a].type == MapEditor::ItemType::Ceiling)
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
	context.endUndoRecord(changed);

	// Editor message
	if (items.size() > 0 && changed)
	{
		string axis = "X";
		if (!x) axis = "Y";

		if (amount > 0)
			context.addEditorMessage(S_FMT("%s offset increased by %d", axis, amount));
		else
			context.addEditorMessage(S_FMT("%s offset decreased by %d", axis, -amount));
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
	auto& selection_3d = context.selection();
	auto hilight_3d = context.hilightItem();
	if (selection_3d.empty() && hilight_3d.type != MapEditor::ItemType::Thing && hilight_3d.index >= 0)
		items.push_back(hilight_3d);
	else
	{
		for (unsigned a = 0; a < selection_3d.size(); a++)
		{
			if (selection_3d[a].type != MapEditor::ItemType::Thing)
				items.push_back(selection_3d[a]);
		}
	}
	if (items.empty())
		return;

	// Begin undo level
	context.beginUndoRecordLocked("Change Sector Height", true, false, false);

	// Go through items
	vector<int> ceilings;
	for (unsigned a = 0; a < items.size(); a++)
	{
		// Wall (ceiling only for now)
		if (items[a].type == MapEditor::ItemType::WallBottom ||
			items[a].type == MapEditor::ItemType::WallMiddle ||
			items[a].type == MapEditor::ItemType::WallTop)
		{
			// Get sector
			auto sector = context.getMap().getSide(items[a].index)->getSector();

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
		else if (items[a].type == MapEditor::ItemType::Floor)
		{
			// Get sector
			auto sector = context.getMap().getSector(items[a].index);

			// Change height
			sector->setFloorHeight(sector->getFloorHeight() + amount);
		}

		// Ceiling
		else if (items[a].type == MapEditor::ItemType::Ceiling)
		{
			// Get sector
			auto sector = context.getMap().getSector(items[a].index);

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
	context.endUndoRecord();

	// Editor message
	if (items.size() > 0)
	{
		if (amount > 0)
			context.addEditorMessage(S_FMT("Height increased by %d", amount));
		else
			context.addEditorMessage(S_FMT("Height decreased by %d", -amount));
	}
}

/* Edit3D::autoAlignX
 * Aligns X offsets beginning from the wall selection [start]
 *******************************************************************/
void Edit3D::autoAlignX(MapEditor::Item start) const
{
	// Check start is a wall
	if (start.type != MapEditor::ItemType::WallBottom && start.type != MapEditor::ItemType::WallMiddle && start.type != MapEditor::ItemType::WallTop)
		return;

	// Get starting side
	auto side = context.getMap().getSide(start.index);
	if (!side) return;

	// Get texture to match
	string tex;
	if (start.type == MapEditor::ItemType::WallBottom)
		tex = side->stringProperty("texturebottom");
	else if (start.type == MapEditor::ItemType::WallMiddle)
		tex = side->stringProperty("texturemiddle");
	else if (start.type == MapEditor::ItemType::WallTop)
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
	context.beginUndoRecord("Auto Align X", true, false, false);

	// Do alignment
	doAlignX(side, side->intProperty("offsetx"), tex, walls_done, tex_width);

	// End undo level
	context.endUndoRecord();

	// Editor message
	context.addEditorMessage("Auto-aligned on X axis");
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
	auto& selection_3d = context.selection();
	auto hilight_3d = context.hilightItem();
	if (selection_3d.size() == 0)
	{
		if (hilight_3d.type == MapEditor::ItemType::WallTop ||
			hilight_3d.type == MapEditor::ItemType::WallBottom ||
			hilight_3d.type == MapEditor::ItemType::WallMiddle)
			walls.push_back(hilight_3d);
		else if (hilight_3d.type == MapEditor::ItemType::Floor || hilight_3d.type == MapEditor::ItemType::Ceiling)
			flats.push_back(hilight_3d);
		else if (hilight_3d.type == MapEditor::ItemType::Thing)
			things.push_back(hilight_3d);
	}
	else
	{
		for (unsigned a = 0; a < selection_3d.size(); a++)
		{
			if (selection_3d[a].type == MapEditor::ItemType::WallTop ||
				selection_3d[a].type == MapEditor::ItemType::WallBottom ||
				selection_3d[a].type == MapEditor::ItemType::WallMiddle)
				walls.push_back(selection_3d[a]);
			else if (selection_3d[a].type == MapEditor::ItemType::Floor || selection_3d[a].type == MapEditor::ItemType::Ceiling)
				flats.push_back(selection_3d[a]);
			else if (selection_3d[a].type == MapEditor::ItemType::Thing)
				things.push_back(selection_3d[a]);
		}
	}
	if (walls.size() == 0 && flats.size() == 0 && things.size() == 0)
		return;

	// Begin undo level
	context.beginUndoRecord("Reset Offsets", true, false, false);

	// Go through walls
	for (unsigned a = 0; a < walls.size(); a++)
	{
		auto side = context.getMap().getSide(walls[a].index);
		if (!side) continue;

		// Reset offsets
		if (link_offset)
		{
			// If offsets are linked, reset base offsets
			side->setIntProperty("offsetx", 0);
			side->setIntProperty("offsety", 0);
		}
		else
		{
			// Otherwise, reset offsets for the current wall part
			if (walls[a].type == MapEditor::ItemType::WallTop)
			{
				side->setFloatProperty("offsetx_top", 0);
				side->setFloatProperty("offsety_top", 0);
			}
			else if (walls[a].type == MapEditor::ItemType::WallMiddle)
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
		if (context.mapDesc().format == MAP_UDMF && theGameConfiguration->udmfTextureScaling())
		{
			if (walls[a].type == MapEditor::ItemType::WallTop)
			{
				side->setFloatProperty("scalex_top", 1);
				side->setFloatProperty("scaley_top", 1);
			}
			else if (walls[a].type == MapEditor::ItemType::WallMiddle)
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
	if (context.mapDesc().format == MAP_UDMF)
	{
		for (unsigned a = 0; a < flats.size(); a++)
		{
			auto sector = context.getMap().getSector(flats[a].index);
			if (!sector) continue;

			string plane;
			if (flats[a].type == MapEditor::ItemType::Floor)
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
	if (context.mapDesc().format != MAP_DOOM)
	{
		for (unsigned a = 0; a < things.size(); ++a)
		{
			auto thing = context.getMap().getThing(things[a].index);
			if (!thing) continue;

			// Reset height
			if (context.mapDesc().format != MAP_UDMF)
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
	context.endUndoRecord();

	// Editor message
	if (context.mapDesc().format == MAP_UDMF && (theGameConfiguration->udmfFlatScaling() ||
		theGameConfiguration->udmfSideScaling() || theGameConfiguration->udmfTextureScaling()))
		context.addEditorMessage("Offsets and scaling reset");
	else
		context.addEditorMessage("Offsets reset");
}

/* Edit3D::toggleUnpegged
 * Toggles the lower/upper unpegged flag for selected walls depending
 * on [lower]
 *******************************************************************/
void Edit3D::toggleUnpegged(bool lower) const
{
	auto& selection_3d = context.selection();
	auto hilight_3d = context.hilightItem();
	if (selection_3d.size() == 0 && hilight_3d.index < 0)
		return;

	// Get items to process
	vector<MapEditor::Item> items;
	if (selection_3d.size() == 0)
	{
		if (hilight_3d.type == MapEditor::ItemType::WallTop ||
			hilight_3d.type == MapEditor::ItemType::WallBottom ||
			hilight_3d.type == MapEditor::ItemType::WallMiddle)
			items.push_back(hilight_3d);
	}
	else
	{
		for (unsigned a = 0; a < selection_3d.size(); a++)
		{
			if (selection_3d[a].type == MapEditor::ItemType::WallTop ||
				selection_3d[a].type == MapEditor::ItemType::WallBottom ||
				selection_3d[a].type == MapEditor::ItemType::WallMiddle)
				items.push_back(selection_3d[a]);
		}
	}
	if (items.size() == 0)
		return;

	// Begin undo level
	string undo_type = lower ? "Toggle Lower Unpegged" : "Toggle Upper Unpegged";
	undo_manager->beginRecord(undo_type);

	// Go through items
	vector<MapLine*> processed_lines;
	for (unsigned a = 0; a < items.size(); a++)
	{
		// Get line
		auto line = context.getMap().getSide(items[a].index)->getParentLine();
		if (!line) continue;

		// Skip if line already processed
		if (VECTOR_EXISTS(processed_lines, line))
			continue;
		else
			processed_lines.push_back(line);

		// Toggle flag
		context.recordPropertyChangeUndoStep(line);
		if (lower)
		{
			bool unpegged = theGameConfiguration->lineBasicFlagSet(
				"dontpegbottom",
				line,
				context.mapDesc().format
			);
			theGameConfiguration->setLineBasicFlag(
				"dontpegbottom",
				line,
				context.getMap().currentFormat(),
				!unpegged
			);
		}
		else
		{
			bool unpegged = theGameConfiguration->lineBasicFlagSet(
				"dontpegtop",
				line,
				context.mapDesc().format
			);
			theGameConfiguration->setLineBasicFlag(
				"dontpegtop",
				line,
				context.getMap().currentFormat(),
				!unpegged
			);
		}
	}

	// End undo level
	undo_manager->endRecord(true);

	// Editor message
	if (lower)
		context.addEditorMessage("Lower Unpegged flag toggled");
	else
		context.addEditorMessage("Upper Unpegged flag toggled");
}

/* Edit3D::changeThingZ
 * Changes the Z height of selected 3d mode things by [amount]
 *******************************************************************/
void Edit3D::changeThingZ(int amount) const
{
	// Ignore for doom format
	if (context.getMap().currentFormat() == MAP_DOOM)
		return;

	// Go through 3d selection
	auto& selection_3d = context.selection();
	for (unsigned a = 0; a < selection_3d.size(); a++)
	{
		// Check if thing
		if (selection_3d[a].type == MapEditor::ItemType::Thing)
		{
			MapThing* thing = context.getMap().getThing(selection_3d[a].index);
			if (thing)
			{
				// Change z height
				context.recordPropertyChangeUndoStep(thing);
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
	context.beginUndoRecord("Delete Thing", false, false, true);

	// Go through 3d selection
	auto& selection_3d = context.selection();
	for (auto& item : selection_3d)
	{
		// Check if thing
		if (item.type == MapEditor::ItemType::Thing)
			context.getMap().removeThing(item.index);
	}

	context.endUndoRecord();
}

/* Edit3D::changeScale
 * Changes scaling for the currently selected walls/flats, x scale if
 * [x] is true, y scale otherwise
 *******************************************************************/
void Edit3D::changeScale(double amount, bool x) const
{
	// Get items to process
	vector<MapEditor::Item> items;
	auto& selection_3d = context.selection();
	auto hilight_3d = context.hilightItem();
	if (selection_3d.empty())
	{
		if (hilight_3d.index >= 0 && hilight_3d.type != MapEditor::ItemType::Thing)
			items.push_back(hilight_3d);
	}
	else
	{
		for (unsigned a = 0; a < selection_3d.size(); a++)
		{
			if (selection_3d[a].type != MapEditor::ItemType::Thing)
				items.push_back(selection_3d[a]);
		}
	}
	if (items.empty())
		return;

	// Begin undo level
	context.beginUndoRecordLocked("Change Scale", true, false, false);

	// Go through selection
	for (unsigned a = 0; a < items.size(); a++)
	{
		// Wall
		if (items[a].type >= MapEditor::ItemType::WallTop && items[a].type <= MapEditor::ItemType::WallBottom &&
			(theGameConfiguration->udmfSideScaling() || theGameConfiguration->udmfTextureScaling()))
		{
			auto side = context.getMap().getSide(items[a].index);

			// Build property string (offset[x/y]_[top/mid/bottom])
			string ofs = "scalex";
			if (!x) ofs = "scaley";
			if (theGameConfiguration->udmfTextureScaling())
			{
				if (items[a].type == MapEditor::ItemType::WallBottom)
					ofs += "_bottom";
				else if (items[a].type == MapEditor::ItemType::WallTop)
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
			auto sector = context.getMap().getSector(items[a].index);

			// Build property string
			string prop = x ? "xscale" : "yscale";
			prop += (items[a].type == MapEditor::ItemType::Floor) ? "floor" : "ceiling";

			// Set
			double scale = sector->floatProperty(prop);
			if (scale + amount > 0)
				sector->setFloatProperty(prop, scale + amount);
		}
	}

	// End undo record
	context.endUndoRecord(true);

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
	auto& selection_3d = context.selection();
	auto hilight_3d = context.hilightItem();
	auto& map = context.getMap();
	if (selection_3d.empty() && hilight_3d.index >= 0)
	{
		if (hilight_3d.type != MapEditor::ItemType::Thing || map.currentFormat() != MAP_DOOM)
			items.push_back(hilight_3d);
	}
	else for (unsigned a = 0; a < selection_3d.size(); a++)
	{
		if (selection_3d[a].type != MapEditor::ItemType::Thing || map.currentFormat() != MAP_DOOM)
			items.push_back(selection_3d[a]);
	}
	if (items.empty())
		return;

	// Begin undo level
	context.beginUndoRecordLocked("Change Height", true, false, false);

	// Go through items
	for (unsigned a = 0; a < items.size(); a++)
	{
		auto type = items[a].type;

		// Thing
		if (type == MapEditor::ItemType::Thing)
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
		if (type == MapEditor::ItemType::WallBottom ||
			type == MapEditor::ItemType::WallMiddle ||
			type == MapEditor::ItemType::WallTop)
		{
			auto side = map.getSide(items[a].index);

			if (side)
			{
				string ofs = "offsety";

				// If offsets are linked, just change the whole side offset
				if (link_offset)
				{
					int offset = side->intProperty(ofs);
					side->setIntProperty(ofs, offset + amount);
					continue;
				}
				// Unlinked offsets, build string (offsety_[top/mid/bottom])
				else if (items[a].type == MapEditor::ItemType::WallBottom)
					ofs += "_bottom";
				else if (items[a].type == MapEditor::ItemType::WallTop)
					ofs += "_top";
				else
					ofs += "_mid";

				// Change the offset
				float offset = side->floatProperty(ofs);
				side->setFloatProperty(ofs, offset + amount);
			}
		}

		// Floor
		else if (type == MapEditor::ItemType::Floor)
		{
			// Get sector
			auto sector = map.getSector(items[a].index);

			// Change height
			if (sector)
				sector->setFloorHeight(sector->getFloorHeight() + amount);
		}

		// Ceiling
		else if (type == MapEditor::ItemType::Ceiling)
		{
			// Get sector
			auto sector = map.getSector(items[a].index);

			// Change height
			if (sector)
				sector->setCeilingHeight(sector->getCeilingHeight() + amount);
		}
	}

	// End undo level
	context.endUndoRecord();

	// Editor message
	if (items.size() > 0)
	{
		if (amount > 0)
			context.addEditorMessage(S_FMT("Height increased by %d", amount));
		else
			context.addEditorMessage(S_FMT("Height decreased by %d", -amount));
	}
}

/* Edit3D::wallMatches
 * Returns true if the texture [part] of [side] matches [tex]
 *******************************************************************/
bool Edit3D::wallMatches(MapSide* side, MapEditor::ItemType part, string tex)
{
	// Check for blank texture where it isn't needed
	if (tex == "-")
	{
		auto line = side->getParentLine();
		int needed = line->needsTexture();
		if (side == line->s1())
		{
			if (part == MapEditor::ItemType::WallTop && (needed & TEX_FRONT_UPPER) == 0)
				return false;
			if (part == MapEditor::ItemType::WallMiddle && (needed & TEX_FRONT_MIDDLE) == 0)
				return false;
			if (part == MapEditor::ItemType::WallBottom && (needed & TEX_FRONT_LOWER) == 0)
				return false;
		}
		else if (side == line->s2())
		{
			if (part == MapEditor::ItemType::WallTop && (needed & TEX_BACK_UPPER) == 0)
				return false;
			if (part == MapEditor::ItemType::WallMiddle && (needed & TEX_BACK_MIDDLE) == 0)
				return false;
			if (part == MapEditor::ItemType::WallBottom && (needed & TEX_BACK_LOWER) == 0)
				return false;
		}
	}

	// Check texture
	if (part == MapEditor::ItemType::WallTop && side->stringProperty("texturetop") != tex)
		return false;
	if (part == MapEditor::ItemType::WallMiddle && side->stringProperty("texturemiddle") != tex)
		return false;
	if (part == MapEditor::ItemType::WallBottom && side->stringProperty("texturebottom") != tex)
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
	auto side = context.getMap().getSide(item.index);
	if (!side)
		return;

	// Get initial line
	auto line = side->getParentLine();
	if (!line)
		return;

	// Get texture to match
	string tex;
	if (item.type == MapEditor::ItemType::WallBottom)
		tex = side->stringProperty("texturebottom");
	else if (item.type == MapEditor::ItemType::WallMiddle)
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
			if (wallMatches(side1, MapEditor::ItemType::WallTop, tex))
				getAdjacentWalls({ (int)side1->getIndex(), MapEditor::ItemType::WallTop }, list);

			// Middle texture
			if (wallMatches(side1, MapEditor::ItemType::WallMiddle, tex))
				getAdjacentWalls({ (int)side1->getIndex(), MapEditor::ItemType::WallMiddle }, list);

			// Lower texture
			if (wallMatches(side1, MapEditor::ItemType::WallBottom, tex))
				getAdjacentWalls({ (int)side1->getIndex(), MapEditor::ItemType::WallBottom }, list);
		}

		// Back side
		if (side2)
		{
			// Upper texture
			if (wallMatches(side2, MapEditor::ItemType::WallTop, tex))
				getAdjacentWalls({ (int)side2->getIndex(), MapEditor::ItemType::WallTop }, list);

			// Middle texture
			if (wallMatches(side2, MapEditor::ItemType::WallMiddle, tex))
				getAdjacentWalls({ (int)side2->getIndex(), MapEditor::ItemType::WallMiddle }, list);

			// Lower texture
			if (wallMatches(side2, MapEditor::ItemType::WallBottom, tex))
				getAdjacentWalls({ (int)side2->getIndex(), MapEditor::ItemType::WallBottom }, list);
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
			if (wallMatches(side1, MapEditor::ItemType::WallTop, tex))
				getAdjacentWalls({ (int)side1->getIndex(), MapEditor::ItemType::WallTop }, list);

			// Middle texture
			if (wallMatches(side1, MapEditor::ItemType::WallMiddle, tex))
				getAdjacentWalls({ (int)side1->getIndex(), MapEditor::ItemType::WallMiddle }, list);

			// Lower texture
			if (wallMatches(side1, MapEditor::ItemType::WallBottom, tex))
				getAdjacentWalls({ (int)side1->getIndex(), MapEditor::ItemType::WallBottom }, list);
		}

		// Back side
		if (side2)
		{
			// Upper texture
			if (wallMatches(side2, MapEditor::ItemType::WallTop, tex))
				getAdjacentWalls({ (int)side2->getIndex(), MapEditor::ItemType::WallTop }, list);

			// Middle texture
			if (wallMatches(side2, MapEditor::ItemType::WallMiddle, tex))
				getAdjacentWalls({ (int)side2->getIndex(), MapEditor::ItemType::WallMiddle }, list);

			// Lower texture
			if (wallMatches(side2, MapEditor::ItemType::WallBottom, tex))
				getAdjacentWalls({ (int)side2->getIndex(), MapEditor::ItemType::WallBottom }, list);
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
	if (item.index < 0 || (item.type != MapEditor::ItemType::Floor && item.type != MapEditor::ItemType::Ceiling))
		return;

	// Add item
	list.push_back(item);

	// Get initial sector
	auto sector = context.getMap().getSector(item.index);
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
		if (item.type == MapEditor::ItemType::Floor)
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

/* Edit3D::getAdjacent
 * Returns a list of all walls or flats adjacent to [item]. Adjacent 
 * meaning connected and sharing a texture
 *******************************************************************/
vector<MapEditor::Item> Edit3D::getAdjacent(MapEditor::Item item) const
{
	vector<MapEditor::Item> list;

	// Check item
	if (item.index < 0 || item.type == MapEditor::ItemType::Thing)
		return list;

	// Flat
	if (item.type == MapEditor::ItemType::Floor || item.type == MapEditor::ItemType::Ceiling)
		getAdjacentFlats(item, list);

	// Wall
	else if (item.type != MapEditor::ItemType::Thing)
		getAdjacentWalls(item, list);

	return list;
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
	walls_done.push_back({ (int)side->getIndex(), MapEditor::ItemType::WallMiddle });

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
