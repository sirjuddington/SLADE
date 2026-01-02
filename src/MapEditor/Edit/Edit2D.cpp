
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Edit2D.cpp
// Description: Map Editor 2D modes (vertices/lines/etc) editing functionality
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
#include "Edit2D.h"
#include "App.h"
#include "Game/Configuration.h"
#include "General/Clipboard.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/SectorBuilder.h"
#include "MapEditor/UndoSteps.h"
#include "Utility/MathStuff.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, map_merge_lines_on_delete_vertex, false, CVar::Flag::Save)
CVAR(Bool, map_remove_invalid_lines, false, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// Edit2D Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Edit2D class constructor
// -----------------------------------------------------------------------------
Edit2D::Edit2D(MapEditContext& context) :
	context_{ context },
	copy_line_{ nullptr, nullptr, &copy_side_front_, &copy_side_back_ }
{
}

// -----------------------------------------------------------------------------
// Mirror selected objects horizontally or vertically depending on [x_axis]
// -----------------------------------------------------------------------------
void Edit2D::mirror(bool x_axis) const
{
	using mapeditor::Mode;

	// Mirror things
	if (context_.editMode() == Mode::Things)
	{
		// Begin undo level
		context_.beginUndoRecord("Mirror Things", true, false, false);

		// Get things to mirror
		auto things = context_.selection().selectedThings();

		// Get midpoint
		BBox bbox;
		for (auto& thing : things)
			bbox.extend(thing->xPos(), thing->yPos());

		// Mirror
		for (auto& thing : things)
		{
			// Position
			if (x_axis)
				thing->move({ bbox.midX() - (thing->xPos() - bbox.midX()), thing->yPos() });
			else
				thing->move({ thing->xPos(), bbox.midY() - (thing->yPos() - bbox.midY()) });

			// Direction
			int angle = thing->angle();
			if (x_axis)
			{
				angle += 90;
				angle = 360 - angle;
				angle -= 90;
			}
			else
				angle = 360 - angle;
			while (angle < 0)
				angle += 360;
			thing->setAngle(angle);
		}
		context_.endUndoRecord(true);
	}

	// Mirror map architecture
	else if (context_.editMode() != Mode::Visual)
	{
		// Begin undo level
		context_.beginUndoRecord("Mirror Map Architecture", true, false, false);

		// Get vertices to mirror
		vector<MapVertex*> vertices;
		vector<MapLine*>   lines;
		if (context_.editMode() == Mode::Vertices)
			vertices = context_.selection().selectedVertices();
		else if (context_.editMode() == Mode::Lines)
		{
			auto sel = context_.selection().selectedLines();
			for (auto line : sel)
			{
				VECTOR_ADD_UNIQUE(vertices, line->v1());
				VECTOR_ADD_UNIQUE(vertices, line->v2());
				lines.push_back(line);
			}
		}
		else if (context_.editMode() == Mode::Sectors)
		{
			auto sectors = context_.selection().selectedSectors();
			for (auto sector : sectors)
			{
				sector->putVertices(vertices);
				sector->putLines(lines);
			}
		}

		// Get midpoint
		BBox bbox;
		for (auto& vertex : vertices)
			bbox.extend(vertex->xPos(), vertex->yPos());

		// Mirror vertices
		for (auto& vertex : vertices)
		{
			// Position
			if (x_axis)
				vertex->move(bbox.midX() - (vertex->xPos() - bbox.midX()), vertex->yPos());
			else
				vertex->move(vertex->xPos(), bbox.midY() - (vertex->yPos() - bbox.midY()));
		}

		// Flip lines (just swap vertices)
		for (auto& line : lines)
			line->flip(false);

		context_.endUndoRecord(true);
	}
}

// -----------------------------------------------------------------------------
// Opens a dialog containing a MapObjectPropsPanel to edit properties for all
// selected (or hilighted) objects
// -----------------------------------------------------------------------------
void Edit2D::editObjectProperties()
{
	auto selection = context_.selection().selectedObjects();
	if (selection.empty())
		return;

	// Begin recording undo level
	context_.beginUndoRecord(fmt::format("Property Edit ({})", context_.modeString(false)));
	for (auto item : selection)
		context_.recordPropertyChangeUndoStep(item);

	bool done = mapeditor::editObjectProperties(selection);
	if (done)
	{
		context_.renderer().forceUpdate();
		context_.updateDisplay();

		if (context_.editMode() == mapeditor::Mode::Things && selection[0]->objType() == MapObject::Type::Thing)
		{
			copy_thing_.copy(selection[0]);
			thing_copied_ = true;
		}
	}

	// End undo level
	context_.endUndoRecord(done);
}

// -----------------------------------------------------------------------------
// Splits the line closest to [x,y] at the closest point on the line
// -----------------------------------------------------------------------------
void Edit2D::splitLine(double x, double y, double min_dist) const
{
	Vec2d point(x, y);

	// Get the closest line
	auto line = context_.map().lines().nearest(point, min_dist);

	// Do nothing if no line is close enough
	if (!line)
		return;

	// Begin recording undo level
	context_.beginUndoRecord("Split Line", true, true, false);

	// Get closest point on the line
	auto closest = math::closestPointOnLine(point, line->seg());

	// Create vertex there
	auto vertex = context_.map().createVertex(closest);

	// Do line split
	context_.map().splitLine(line, vertex);

	// Finish recording undo level
	context_.endUndoRecord();
}

// -----------------------------------------------------------------------------
// Flips all selected lines, and sides if [sides] is true
// -----------------------------------------------------------------------------
void Edit2D::flipLines(bool sides) const
{
	// Get selected/hilighted line(s)
	auto lines = context_.selection().selectedLines();
	if (lines.empty())
		return;

	// Go through list
	context_.undoManager()->beginRecord("Flip Line");
	for (auto& line : lines)
	{
		context_.undoManager()->recordUndoStep(std::make_unique<mapeditor::PropertyChangeUS>(line));
		line->flip(sides);
	}
	context_.undoManager()->endRecord(true);

	// Update display
	context_.updateDisplay();
}

// -----------------------------------------------------------------------------
// Attempts to correct sector references on all selected lines
// -----------------------------------------------------------------------------
void Edit2D::correctLineSectors() const
{
	// Get selected/hilighted line(s)
	auto lines = context_.selection().selectedLines();
	if (lines.empty())
		return;

	context_.beginUndoRecord("Correct Line Sectors");

	bool changed = false;
	for (auto& line : lines)
	{
		if (context_.map().correctLineSectors(line))
			changed = true;
	}

	context_.endUndoRecord(changed);

	// Update display
	if (changed)
	{
		context_.addEditorMessage("Corrected Sector references");
		context_.updateDisplay();
	}
}

// -----------------------------------------------------------------------------
// Changes floor and/or ceiling heights on all selected sectors by [amount]
// -----------------------------------------------------------------------------
void Edit2D::changeSectorHeight(int amount, bool floor, bool ceiling) const
{
	// Do nothing if not in sectors mode
	if (context_.editMode() != mapeditor::Mode::Sectors)
		return;

	// Get selected sectors (if any)
	auto selection = context_.selection().selectedSectors();
	if (selection.empty())
		return;

	// If we're modifying both heights, take sector_mode into account
	if (floor && ceiling)
	{
		if (context_.sectorEditMode() == mapeditor::SectorMode::Floor)
			ceiling = false;
		if (context_.sectorEditMode() == mapeditor::SectorMode::Ceiling)
			floor = false;
	}

	// Begin record undo level
	context_.beginUndoRecordLocked("Change Sector Height", true, false, false);

	// Go through selection
	for (auto& sector : selection)
	{
		// Change floor height
		if (floor)
			sector->setFloorHeight(sector->floor().height + amount);

		// Change ceiling height
		if (ceiling)
			sector->setCeilingHeight(sector->ceiling().height + amount);
	}

	// End record undo level
	context_.endUndoRecord();

	// Add editor message
	string what;
	if (floor && !ceiling)
		what = "Floor";
	else if (!floor && ceiling)
		what = "Ceiling";
	else
		what = "Floor and ceiling";
	string inc = "increased";
	if (amount < 0)
	{
		inc    = "decreased";
		amount = -amount;
	}
	context_.addEditorMessage(fmt::format("{} height {} by {}", what, inc, amount));

	// Update display
	context_.updateDisplay();
}

// -----------------------------------------------------------------------------
// Changes the light level for all selected sectors.
// Increments if [up] is true, decrements otherwise
// -----------------------------------------------------------------------------
void Edit2D::changeSectorLight(bool up, bool fine) const
{
	// Do nothing if not in sectors mode
	if (context_.editMode() != mapeditor::Mode::Sectors)
		return;

	// Get selected sectors (if any)
	auto selection = context_.selection().selectedSectors();
	if (selection.empty())
		return;

	// Begin record undo level
	context_.beginUndoRecordLocked("Change Sector Light", true, false, false);

	// Go through selection
	for (auto& sector : selection)
	{
		// Get current light
		int light = sector->lightLevel();

		// Increment/decrement
		if (up)
			light = fine ? light + 1 : game::configuration().upLightLevel(light);
		else
			light = fine ? light - 1 : game::configuration().downLightLevel(light);

		// Change light level
		sector->setLightLevel(light);
	}

	// End record undo level
	context_.endUndoRecord();

	// Add editor message
	int amount = fine ? 1 : game::configuration().lightLevelInterval();
	context_.addEditorMessage(fmt::format("Light level {} by {}", up ? "increased" : "decreased", amount));

	// Update display
	context_.updateDisplay();
}

// -----------------------------------------------------------------------------
// Depending on the current sector edit mode, either opens the sector texture
// overlay (normal) or browses for the ceiling or floor texture (ceiling/floor
// edit mode)
// -----------------------------------------------------------------------------
void Edit2D::changeSectorTexture() const
{
	using mapeditor::SectorMode;

	// Get selected sectors
	auto selection = context_.selection().selectedSectors();
	if (selection.empty())
		return;

	// Determine the initial texture
	string texture, browser_title, undo_name;
	auto   mode = context_.sectorEditMode();
	if (mode == SectorMode::Floor)
	{
		texture       = selection[0]->floor().texture;
		browser_title = "Browse Floor Texture";
		undo_name     = "Change Floor Texture";
	}
	else if (mode == SectorMode::Ceiling)
	{
		texture       = selection[0]->ceiling().texture;
		browser_title = "Browse Ceiling Texture";
		undo_name     = "Change Ceiling Texture";
	}
	else
	{
		context_.openSectorTextureOverlay(selection);
		return;
	}

	// Lock hilight
	bool hl_lock = context_.selection().hilightLocked();
	context_.selection().lockHilight();

	// Open texture browser
	auto selected_tex = mapeditor::browseTexture(texture, mapeditor::TextureType::Flat, context_.map(), browser_title);
	if (!selected_tex.empty())
	{
		// Set texture depending on edit mode
		context_.beginUndoRecord(undo_name, true, false, false);
		for (auto& sector : selection)
		{
			if (mode == SectorMode::Floor)
				sector->setFloorTexture(selected_tex);
			else if (mode == SectorMode::Ceiling)
				sector->setCeilingTexture(selected_tex);
		}
		context_.endUndoRecord();
	}

	// Unlock hilight if needed
	context_.selection().lockHilight(hl_lock);
	context_.renderer().renderer2D().clearTextureCache();
}

// -----------------------------------------------------------------------------
// Joins all selected sectors. If [remove_lines] is true, all resulting lines
// with both sides set to the joined sector are removed
// -----------------------------------------------------------------------------
void Edit2D::joinSectors(bool remove_lines) const
{
	// Check edit mode
	if (context_.editMode() != mapeditor::Mode::Sectors)
		return;

	// Get sectors to merge
	auto sectors = context_.selection().selectedSectors(false);
	if (sectors.size() < 2) // Need at least 2 sectors to join
		return;

	// Get 'target' sector
	auto target = sectors[0];

	// Clear selection
	context_.selection().clear();

	// Init list of lines
	vector<MapLine*> lines;

	// Begin recording undo level
	context_.beginUndoRecord("Join/Merge Sectors", true, false, true);

	// Go through merge sectors
	for (unsigned a = 1; a < sectors.size(); a++)
	{
		// Go through sector sides
		auto sector = sectors[a];
		while (!sector->connectedSides().empty())
		{
			// Set sector
			auto side = sector->connectedSides()[0];
			side->setSector(target);

			// Add line to list if not already there
			bool exists = false;
			for (auto& line : lines)
			{
				if (side->parentLine() == line)
				{
					exists = true;
					break;
				}
			}
			if (!exists)
				lines.push_back(side->parentLine());
		}

		// Delete sector
		context_.map().removeSector(sector);
	}

	// Remove any changed lines that now have the target sector on both sides (if needed)
	int                nlines = 0;
	vector<MapVertex*> verts;
	if (remove_lines)
	{
		for (auto& line : lines)
		{
			if (line->frontSector() == target && line->backSector() == target)
			{
				VECTOR_ADD_UNIQUE(verts, line->v1());
				VECTOR_ADD_UNIQUE(verts, line->v2());
				context_.map().removeLine(line);
				nlines++;
			}
		}
	}

	// Remove any resulting detached vertices
	for (auto& vert : verts)
	{
		if (vert->nConnectedLines() == 0)
			context_.map().removeVertex(vert);
	}

	// Finish recording undo level
	context_.endUndoRecord();

	// Editor message
	if (nlines == 0)
		context_.addEditorMessage(fmt::format("Joined {} Sectors", sectors.size()));
	else
		context_.addEditorMessage(fmt::format("Joined {} Sectors (removed {} Lines)", sectors.size(), nlines));
}

// -----------------------------------------------------------------------------
// Opens the thing type browser for the currently selected thing(s)
// -----------------------------------------------------------------------------
void Edit2D::changeThingType()
{
	// Get selected things (if any)
	auto selection = context_.selection().selectedThings();

	// Do nothing if no selection or hilight
	if (selection.empty())
		return;

	// Browse thing type
	int newtype = mapeditor::browseThingType(selection[0]->type(), context_.map());
	if (newtype >= 0)
	{
		// Go through selection
		context_.beginUndoRecord("Thing Type Change", true, false, false);
		for (auto& thing : selection)
			thing->setType(newtype);
		context_.endUndoRecord(true);

		// Add editor message
		auto type_name = game::configuration().thingType(newtype).name();
		if (selection.size() == 1)
			context_.addEditorMessage(fmt::format("Changed type to \"{}\"", type_name));
		else
			context_.addEditorMessage(fmt::format("Changed {} things to type \"{}\"", selection.size(), type_name));

		// Update 'copy' thing with new type
		copy_thing_.setIntProperty("type", newtype);

		// Update display
		context_.updateDisplay();
	}
}

// -----------------------------------------------------------------------------
// Sets the angle of all selected things to face toward [mouse_pos]
// -----------------------------------------------------------------------------
void Edit2D::thingQuickAngle(Vec2d mouse_pos) const
{
	// Do nothing if not in things mode
	if (context_.editMode() != mapeditor::Mode::Things)
		return;

	for (auto thing : context_.selection().selectedThings())
		thing->setAnglePoint(mouse_pos);
}

// -----------------------------------------------------------------------------
// Copies all selected objects
// -----------------------------------------------------------------------------
void Edit2D::copy() const
{
	using mapeditor::Mode;
	auto mode = context_.editMode();

	// Can't copy/paste vertices (no point)
	if (mode == Mode::Vertices)
	{
		// addEditorMessage("Copy/Paste not supported for vertices");
		return;
	}

	// Clear current clipboard contents
	app::clipboard().clear();

	// Copy lines
	if (mode == Mode::Lines || mode == Mode::Sectors)
	{
		// Get selected lines
		vector<MapLine*> lines;
		if (mode == Mode::Lines)
			lines = context_.selection().selectedLines();
		else if (mode == Mode::Sectors)
		{
			for (auto sector : context_.selection().selectedSectors())
				sector->putLines(lines);
		}

		// Add to clipboard
		auto c = std::make_unique<MapArchClipboardItem>();
		c->addLines(lines);
		auto info = c->info();
		app::clipboard().add(std::move(c));

		// Editor message
		context_.addEditorMessage(fmt::format("Copied {}", info));
	}

	// Copy things
	else if (mode == Mode::Things)
	{
		// Get selected things
		auto things = context_.selection().selectedThings();

		// Add to clipboard
		auto c = std::make_unique<MapThingsClipboardItem>();
		c->addThings(things);
		auto info = c->info();
		app::clipboard().add(std::move(c));

		// Editor message
		context_.addEditorMessage(fmt::format("Copied {}", info));
	}
}

// -----------------------------------------------------------------------------
// Pastes previously copied objects at [mouse_pos]
// -----------------------------------------------------------------------------
void Edit2D::paste(Vec2d mouse_pos) const
{
	// Go through clipboard items
	for (unsigned a = 0; a < app::clipboard().size(); a++)
	{
		// Map architecture
		if (app::clipboard().item(a)->type() == ClipboardItem::Type::MapArchitecture)
		{
			context_.beginUndoRecord("Paste Map Architecture");
			auto clip = dynamic_cast<MapArchClipboardItem*>(app::clipboard().item(a));
			// Snap the geometry in such a way that it stays in the same position relative to the grid
			auto pos       = context_.relativeSnapToGrid(clip->midpoint(), mouse_pos);
			auto new_verts = clip->pasteToMap(&context_.map(), pos);
			context_.map().mergeArch(new_verts);
			context_.addEditorMessage(fmt::format("Pasted {}", clip->info()));
			context_.endUndoRecord(true);
		}

		// Things
		else if (app::clipboard().item(a)->type() == ClipboardItem::Type::MapThings)
		{
			context_.beginUndoRecord("Paste Things", false, true, false);
			auto clip = dynamic_cast<MapThingsClipboardItem*>(app::clipboard().item(a));
			// Snap the geometry in such a way that it stays in the same position relative to the grid
			auto pos = context_.relativeSnapToGrid(clip->midpoint(), mouse_pos);
			clip->pasteToMap(&context_.map(), pos);
			context_.addEditorMessage(fmt::format("Pasted {}", clip->info()));
			context_.endUndoRecord(true);
		}
	}
}

// -----------------------------------------------------------------------------
// Copies the properties from the first selected or current hilighted item
// -----------------------------------------------------------------------------
void Edit2D::copyProperties()
{
	// Get MapObject to copy from
	auto copy_object = context_.selection().firstSelectedOrHilight().asObject(context_.map());
	if (!copy_object)
		return;

	// Sectors mode
	if (context_.editMode() == mapeditor::Mode::Sectors)
	{
		copy_sector_.copy(copy_object);
		sector_copied_ = true;

		context_.addEditorMessage(fmt::format("Copied sector #{} properties", copy_object->index()));
	}

	// Things mode
	else if (context_.editMode() == mapeditor::Mode::Things)
	{
		copy_thing_.copy(copy_object);
		thing_copied_ = true;

		context_.addEditorMessage(fmt::format("Copied thing #{} properties", copy_object->index()));
	}

	// Lines mode
	else if (context_.editMode() == mapeditor::Mode::Lines)
	{
		copy_line_.copy(copy_object);
		line_copied_ = true;

		context_.addEditorMessage(fmt::format("Copied line #{} properties", copy_object->index()));
	}
}

// -----------------------------------------------------------------------------
// Pastes previously copied properties to all selected objects
// -----------------------------------------------------------------------------
void Edit2D::pasteProperties()
{
	// Do nothing if no selection or hilight
	if (!context_.selection().hasHilightOrSelection())
		return;

	// Sectors mode
	if (context_.editMode() == mapeditor::Mode::Sectors)
	{
		// Do nothing if no properties have been copied
		if (!sector_copied_)
			return;

		// Paste properties to selection/hilight
		context_.beginUndoRecord("Paste Sector Properties", true, false, false);
		for (auto sector : context_.selection().selectedSectors())
			sector->copy(&copy_sector_);
		context_.endUndoRecord();

		// Editor message
		context_.addEditorMessage("Pasted sector properties");
	}

	// Things mode
	if (context_.editMode() == mapeditor::Mode::Things)
	{
		// Do nothing if no properties have been copied
		if (!thing_copied_)
			return;

		// Paste properties to selection/hilight
		context_.beginUndoRecord("Paste Thing Properties", true, false, false);
		for (auto thing : context_.selection().selectedThings())
		{
			// Paste properties (but keep position)
			double x = thing->xPos();
			double y = thing->yPos();
			thing->copy(&copy_thing_);
			thing->setFloatProperty("x", x);
			thing->setFloatProperty("y", y);
		}
		context_.endUndoRecord();

		// Editor message
		context_.addEditorMessage("Pasted thing properties");
	}

	// Lines mode
	else if (context_.editMode() == mapeditor::Mode::Lines)
	{
		// Do nothing if no properties have been copied
		if (!line_copied_)
			return;

		// Paste properties to selection/hilight
		context_.beginUndoRecord("Paste Line Properties", true, false, false);
		for (auto line : context_.selection().selectedLines())
			line->copy(&copy_line_);
		context_.endUndoRecord();

		// Editor message
		context_.addEditorMessage("Pasted line properties");
	}

	// Update display
	context_.updateDisplay();
}

// -----------------------------------------------------------------------------
// Creates an object (depending on edit mode) at [x,y]
// -----------------------------------------------------------------------------
void Edit2D::createObject(Vec2d pos) const
{
	using mapeditor::Mode;

	// Vertices mode
	if (context_.editMode() == Mode::Vertices)
	{
		// If there are less than 2 vertices currently selected, just create a vertex at x,y
		if (context_.selection().size() < 2)
			createVertex(pos);
		else
		{
			// Otherwise, create lines between selected vertices
			context_.beginUndoRecord("Create Lines", false, true, false);
			auto vertices = context_.selection().selectedVertices(false);
			for (unsigned a = 0; a < vertices.size() - 1; a++)
				context_.map().createLine(vertices[a], vertices[a + 1]);
			context_.endUndoRecord(true);

			// Editor message
			context_.addEditorMessage(fmt::format("Created {} line(s)", context_.selection().size() - 1));

			// Clear selection
			context_.selection().clear();
		}
	}

	// Sectors mode
	else if (context_.editMode() == Mode::Sectors)
	{
		// Sector
		if (context_.map().nLines() > 0)
		{
			createSector(pos);
		}
		else
		{
			// Just create a vertex
			createVertex(pos);
			context_.setEditMode(Mode::Lines);
		}
	}

	// Things mode
	else if (context_.editMode() == Mode::Things)
		createThing(pos);
}

// -----------------------------------------------------------------------------
// Creates a new vertex at [x,y]
// -----------------------------------------------------------------------------
void Edit2D::createVertex(Vec2d pos) const
{
	// Snap coordinates to grid if necessary
	pos.x = context_.snapToGrid(pos.x, false);
	pos.y = context_.snapToGrid(pos.y, false);

	// Create vertex
	context_.beginUndoRecord("Create Vertex", true, true, false);
	auto vertex = context_.map().createVertex(pos, 2);
	context_.endUndoRecord(true);

	// Editor message
	if (vertex)
		context_.addEditorMessage(fmt::format("Created vertex at ({}, {})", (int)vertex->xPos(), (int)vertex->yPos()));
}

// -----------------------------------------------------------------------------
// Creates a new thing at [x,y]
// -----------------------------------------------------------------------------
void Edit2D::createThing(Vec2d pos) const
{
	// Snap coordinates to grid if necessary
	pos.x = context_.snapToGrid(pos.x, false);
	pos.y = context_.snapToGrid(pos.y, false);

	// Begin undo step
	context_.beginUndoRecord("Create Thing", false, true, false);

	// Create thing
	auto thing = context_.map().createThing(pos);

	// Setup properties
	game::configuration().applyDefaults(thing, context_.map().currentFormat() == MapFormat::UDMF);
	if (thing_copied_ && thing)
	{
		// Copy type and angle from the last copied thing
		thing->setType(copy_thing_.type());
		thing->setAngle(copy_thing_.angle());
	}

	// End undo step
	context_.endUndoRecord(true);

	// Editor message
	if (thing)
		context_.addEditorMessage(fmt::format("Created thing at ({}, {})", (int)thing->xPos(), (int)thing->yPos()));
}

// -----------------------------------------------------------------------------
// Creates a new sector at [x,y]
// -----------------------------------------------------------------------------
void Edit2D::createSector(Vec2d pos) const
{
	auto& map = context_.map();

	// Find nearest line
	auto line = map.lines().nearest(pos, 99999999);
	if (!line)
		return;

	// Determine side
	double side = math::lineSide(pos, line->seg());

	// Get sector to copy if we're in sectors mode
	MapSector* sector_copy = nullptr;
	if (context_.editMode() == mapeditor::Mode::Sectors && !context_.selection().empty())
		sector_copy = context_.selection()[0].asSector(map);

	// Run sector builder
	SectorBuilder builder;
	bool          ok;
	if (side >= 0)
		ok = builder.traceSector(&map, line, true);
	else
		ok = builder.traceSector(&map, line, false);

	// Do nothing if sector was already valid
	if (builder.isValidSector())
		return;

	// Create sector from builder result if needed
	if (ok)
	{
		context_.beginUndoRecord("Create Sector", true, true, false);
		builder.createSector(nullptr, sector_copy);

		// Flash
		context_.renderer().animateSelectionChange({ (int)map.nSectors() - 1, mapeditor::ItemType::Sector });
	}

	// Set some sector defaults from game configuration if needed
	if (!sector_copy && ok)
	{
		auto new_sector = map.sector(map.nSectors() - 1);
		if (new_sector->ceiling().texture.empty())
			game::configuration().applyDefaults(new_sector, map.currentFormat() == MapFormat::UDMF);
	}

	// Editor message
	if (ok)
	{
		context_.addEditorMessage(fmt::format("Created sector #{}", map.nSectors() - 1));
		context_.endUndoRecord(true);
	}
	else
		context_.addEditorMessage("Sector creation failed: " + builder.error());
}

// -----------------------------------------------------------------------------
// Deletes all selected objects, depending on edit mode
// -----------------------------------------------------------------------------
void Edit2D::deleteObject() const
{
	using mapeditor::Mode;

	switch (context_.editMode())
	{
	case Mode::Vertices: deleteVertex(); break;
	case Mode::Lines: deleteLine(); break;
	case Mode::Sectors: deleteSector(); break;
	case Mode::Things: deleteThing(); break;
	default: return;
	}

	// Record undo step
	context_.endUndoRecord(true);
}

// -----------------------------------------------------------------------------
// Deletes all selected vertices
// -----------------------------------------------------------------------------
void Edit2D::deleteVertex() const
{
	// Get selected vertices
	auto verts = context_.selection().selectedVertices();
	int  index = -1;
	if (verts.size() == 1)
		index = verts[0]->index();

	// Clear hilight and selection
	context_.selection().clear();
	context_.selection().clearHilight();

	// Begin undo step
	context_.beginUndoRecord("Delete Vertices", map_merge_lines_on_delete_vertex, false, true);

	// Delete them (if any)
	for (auto& vertex : verts)
		context_.map().removeVertex(vertex, map_merge_lines_on_delete_vertex);

	// Remove detached vertices
	context_.map().removeDetachedVertices();

	// Editor message
	if (verts.size() == 1)
		context_.addEditorMessage(fmt::format("Deleted vertex #{}", index));
	else if (verts.size() > 1)
		context_.addEditorMessage(fmt::format("Deleted {} vertices", verts.size()));
}

// -----------------------------------------------------------------------------
// Deletes all selected lines
// -----------------------------------------------------------------------------
void Edit2D::deleteLine() const
{
	// Get selected lines
	auto lines = context_.selection().selectedLines();
	int  index = -1;
	if (lines.size() == 1)
		index = lines[0]->index();

	// Clear hilight and selection
	context_.selection().clear();
	context_.selection().clearHilight();

	// Begin undo step
	context_.beginUndoRecord("Delete Lines", false, false, true);

	// Delete them (if any)
	for (auto& line : lines)
		context_.map().removeLine(line);

	// Remove detached vertices
	context_.map().removeDetachedVertices();

	// Editor message
	if (lines.size() == 1)
		context_.addEditorMessage(fmt::format("Deleted line #{}", index));
	else if (lines.size() > 1)
		context_.addEditorMessage(fmt::format("Deleted {} lines", lines.size()));
}

// -----------------------------------------------------------------------------
// Deletes all selected things
// -----------------------------------------------------------------------------
void Edit2D::deleteThing() const
{
	// Get selected things
	auto things = context_.selection().selectedThings();
	int  index  = -1;
	if (things.size() == 1)
		index = things[0]->index();

	// Clear hilight and selection
	context_.selection().clear();
	context_.selection().clearHilight();

	// Begin undo step
	context_.beginUndoRecord("Delete Things", false, false, true);

	// Delete them (if any)
	for (auto& thing : things)
		context_.map().removeThing(thing);

	// Editor message
	if (things.size() == 1)
		context_.addEditorMessage(fmt::format("Deleted thing #{}", index));
	else if (things.size() > 1)
		context_.addEditorMessage(fmt::format("Deleted {} things", things.size()));
}

// -----------------------------------------------------------------------------
// Deletes all selected sectors
// -----------------------------------------------------------------------------
void Edit2D::deleteSector() const
{
	// Get selected sectors
	auto sectors = context_.selection().selectedSectors();
	int  index   = -1;
	if (sectors.size() == 1)
		index = sectors[0]->index();

	// Clear hilight and selection
	context_.selection().clear();
	context_.selection().clearHilight();

	// Begin undo step
	context_.beginUndoRecord("Delete Sectors", true, false, true);

	// Delete them (if any), and keep lists of connected lines and sides
	vector<MapSide*> connected_sides;
	vector<MapLine*> connected_lines;
	for (auto sector : sectors)
	{
		for (auto side : sector->connectedSides())
			connected_sides.push_back(side);
		sector->putLines(connected_lines);
	}

	// Remove all connected sides
	for (auto side : connected_sides)
	{
		// Before removing the side, check if we should flip the line
		auto line = side->parentLine();
		if (side == line->s1() && line->s2())
			line->flip();

		context_.map().removeSide(side);
	}

	// Remove resulting invalid lines
	if (map_remove_invalid_lines)
	{
		for (auto line : connected_lines)
		{
			if (!line->s1() && !line->s2())
				context_.map().removeLine(line);
		}
	}

	// Try to fill in textures on any lines that just became one-sided
	for (auto line : connected_lines)
	{
		MapSide* side;
		if (line->s1() && !line->s2())
			side = line->s1();
		else if (!line->s1() && line->s2())
			side = line->s2();
		else
			continue;

		if (side->texMiddle() != MapSide::TEX_NONE)
			continue;

		// Inherit textures from upper or lower
		if (side->texUpper() != MapSide::TEX_NONE)
			side->setTexMiddle(side->texUpper());
		else if (side->texLower() != MapSide::TEX_NONE)
			side->setTexMiddle(side->texLower());

		// If there still isn't a texture, find an adjacent texture to use
		if (side->texMiddle() == MapSide::TEX_NONE)
		{
			auto adj_tex = context_.map().adjacentLineTexture(line->v1());
			if (adj_tex == MapSide::TEX_NONE)
				adj_tex = context_.map().adjacentLineTexture(line->v2());

			if (adj_tex != MapSide::TEX_NONE)
				side->setTexMiddle(adj_tex);
		}

		// Clear any existing textures, which are no longer visible
		side->setTexUpper(MapSide::TEX_NONE);
		side->setTexLower(MapSide::TEX_NONE);
	}

	// Editor message
	if (sectors.size() == 1)
		context_.addEditorMessage(fmt::format("Deleted sector #{}", index));
	else if (sectors.size() > 1)
		context_.addEditorMessage(fmt::format("Deleted {} sector", sectors.size()));

	// Remove detached vertices
	context_.map().removeDetachedVertices();
}
