
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MapEditContext.cpp
 * Description: MapEditContext class - handles the map editing
 *              context for a map (selection, highlight, undo/redo,
 *              editing functions, etc.)
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
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "Game/Configuration.h"
#include "General/Clipboard.h"
#include "General/Console/Console.h"
#include "General/UndoRedo.h"
#include "MapChecks.h"
#include "MapEditContext.h"
#include "MapEditor/Renderer/Overlays/LineTextureOverlay.h"
#include "MapEditor/Renderer/Overlays/QuickTextureOverlay3d.h"
#include "MapEditor/Renderer/Overlays/SectorTextureOverlay.h"
#include "MapEditor/UI/Dialogs/ActionSpecialDialog.h"
#include "MapEditor/UI/Dialogs/SectorSpecialDialog.h"
#include "MapEditor/UI/Dialogs/ShowItemDialog.h"
#include "MapTextureManager.h"
#include "UI/MapCanvas.h"
#include "UI/MapEditorWindow.h"
#include "UndoSteps.h"

using MapEditor::Mode;
using MapEditor::SectorMode;


/*******************************************************************
 * VARIABLES
 *******************************************************************/
namespace
{
	double grid_sizes[] =
	{
		0.05,
		0.1,
		0.25,
		0.5,
		1,
		2,
		4,
		8,
		16,
		32,
		64,
		128,
		256,
		512,
		1024,
		2048,
		4096,
		8192,
		16384,
		32768,
		65536
	};
}
CVAR(Bool, info_overlay_3d, true, CVAR_SAVE)
CVAR(Int, map_bg_ms, 15, CVAR_SAVE)
CVAR(Bool, hilight_smooth, true, CVAR_SAVE)


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Int, flat_drawtype)


/*******************************************************************
 * MapEditContext CLASS FUNCTIONS
 *******************************************************************/

/* MapEditContext::MapEditContext
 * MapEditContext class constructor
 *******************************************************************/
MapEditContext::MapEditContext()
{
	undo_manager_ = std::make_unique<UndoManager>(&map_);
}

/* MapEditContext::~MapEditContext
 * MapEditContext class destructor
 *******************************************************************/
MapEditContext::~MapEditContext()
{
}

/* MapEditContext::setEditMode
 * Changes the current edit mode to [mode]
 *******************************************************************/
void MapEditContext::setEditMode(Mode mode)
{
	// Check if we are changing to the same mode
	if (mode == edit_mode_)
	{
		// Cycle sector edit mode
		if (mode == Mode::Sectors)
			cycleSectorEditMode();

		// Do nothing otherwise
		return;
	}

	// Clear 3d mode undo manager on exiting 3d mode
	if (edit_mode_ == Mode::Visual && mode != Mode::Visual)
	{
		undo_manager_->createMergedLevel(edit_3d_.undoManager(), "3D Mode Editing");
		edit_3d_.undoManager()->clear();
	}

	// Set undo manager for history panel
	if (mode == Mode::Visual && edit_mode_ != Mode::Visual)
		MapEditor::setUndoManager(edit_3d_.undoManager());
	else if (edit_mode_ == Mode::Visual && mode != Mode::Visual)
		MapEditor::setUndoManager(undo_manager_.get());

	edit_mode_prev_ = edit_mode_;

	// Set edit mode
	edit_mode_ = mode;
	sector_mode_ = SectorMode::Both;

	// Clear hilight and selection stuff
	selection_.clearHilight();
	tagged_sectors_.clear();
	tagged_lines_.clear();
	tagged_things_.clear();
	last_undo_level_ = "";

	// Transfer selection to the new mode, if possible
	selection_.migrate(edit_mode_prev_, edit_mode_);

	// Add editor message
	switch (edit_mode_)
	{
	case Mode::Vertices: addEditorMessage("Vertices mode"); break;
	case Mode::Lines:	addEditorMessage("Lines mode"); break;
	case Mode::Sectors:	addEditorMessage("Sectors mode (Normal)"); break;
	case Mode::Things:	addEditorMessage("Things mode"); break;
	case Mode::Visual:		addEditorMessage("3d mode"); break;
	default: break;
	};

	if (edit_mode_ != Mode::Visual)
		updateDisplay();
	updateStatusText();

	// Unlock mouse
	lockMouse(false);

	// Update toolbar
	if (mode != edit_mode_prev_) MapEditor::window()->removeAllCustomToolBars();
	if (mode == Mode::Vertices)
		SAction::fromId("mapw_mode_vertices")->setChecked();
	else if (mode == Mode::Lines)
		SAction::fromId("mapw_mode_lines")->setChecked();
	else if (mode == Mode::Sectors)
	{
		SAction::fromId("mapw_mode_sectors")->setChecked();

		// Sector mode toolbar
		if (edit_mode_prev_ != Mode::Sectors)
		{
			wxArrayString actions;
			actions.Add("mapw_sectormode_normal");
			actions.Add("mapw_sectormode_floor");
			actions.Add("mapw_sectormode_ceiling");
			MapEditor::window()->addCustomToolBar("Sector Mode", actions);
		}

		// Toggle current sector mode
		if (sector_mode_ == SectorMode::Both)
			SAction::fromId("mapw_sectormode_normal")->setChecked();
		else if (sector_mode_ == SectorMode::Floor)
			SAction::fromId("mapw_sectormode_floor")->setChecked();
		else if (sector_mode_ == SectorMode::Ceiling)
			SAction::fromId("mapw_sectormode_ceiling")->setChecked();
	}
	else if (mode == Mode::Things)
		SAction::fromId("mapw_mode_things")->setChecked();
	else if (mode == Mode::Visual)
	{
		SAction::fromId("mapw_mode_3d")->setChecked();
		KeyBind::releaseAll();
		lockMouse(true);
		renderer_.renderer3D().refresh();
	}
	MapEditor::window()->refreshToolBar();
}

/* MapEditContext::setSectorEditMode
 * Changes the current sector edit mode to [mode]
 *******************************************************************/
void MapEditContext::setSectorEditMode(SectorMode mode)
{
	// Set sector mode
	sector_mode_ = mode;

	// Editor message
	if (sector_mode_ == SectorMode::Both)
		addEditorMessage("Sectors mode (Normal)");
	else if (sector_mode_ == SectorMode::Floor)
		addEditorMessage("Sectors mode (Floors)");
	else
		addEditorMessage("Sectors mode (Ceilings)");

	updateStatusText();
}

/* MapEditContext::cycleSectorEditMode
 * Cycles to the next sector edit mode. Both -> Floors -> Ceilings
 *******************************************************************/
void MapEditContext::cycleSectorEditMode()
{
	switch (sector_mode_)
	{
		case SectorMode::Both: setSectorEditMode(SectorMode::Floor); break;
		case SectorMode::Floor: setSectorEditMode(SectorMode::Ceiling); break;
		default: setSectorEditMode(SectorMode::Both);
	}
}

/* MapEditContext::lockMouse
 * Locks/unlocks the mouse cursor. A locked cursor is invisible and
 * will be moved to the center of the canvas every frame
 *******************************************************************/
void MapEditContext::lockMouse(bool lock)
{
	mouse_locked_ = lock;
	canvas_->lockMouse(lock);
}

/* MapEditContext::update
 * Updates the current map editor state (hilight, animations, etc.)
 *******************************************************************/
bool MapEditContext::update(long frametime)
{
	// Ignore if we aren't ready to update
	if (frametime < next_frame_length_)
		return false;

	// Set initial time (ms) until next update
	// This will be set lower if animations are active
	next_frame_length_ = overlayActive() ? 2 : map_bg_ms;

	// Get frame time multiplier
	double mult = (double)frametime / 10.0f;

	// 3d mode
	if (edit_mode_ == Mode::Visual && !overlayActive())
	{
		// Update camera
		if (input_.updateCamera3d(mult))
			next_frame_length_ = 2;

		// Update status bar
		auto pos = renderer_.renderer3D().camPosition();
		MapEditor::setStatusText(S_FMT("Position: (%d, %d, %d)", (int)pos.x, (int)pos.y, (int)pos.z), 3);
		
		// Update hilight
		MapEditor::Item hl{ -1, MapEditor::ItemType::Any };
		if (!selection_.hilightLocked())
		{
			auto old_hl = selection_.hilight();
			hl = renderer_.renderer3D().determineHilight();
			if (selection_.setHilight(hl))
			{
				// Update 3d info overlay
				if (info_overlay_3d && hl.index >= 0)
				{
					info_3d_.update(hl.index, hl.type, &map_);
					info_showing_ = true;
				}
				else
					info_showing_ = false;

				// Animation
				renderer_.animateHilightChange(old_hl);
			}
		}
	}

	// 2d mode
	else
	{
		// Update hilight if needed
		auto prev_hl = selection_.hilight();
		if (input_.mouseState() == MapEditor::Input::MouseState::Normal/* && !mouse_movebegin*/)
		{
			auto old_hl = selection_.hilightedObject();
			if (selection_.updateHilight(input_.mousePosMap(), renderer_.view().scale()) && hilight_smooth)
				renderer_.animateHilightChange({}, old_hl);
		}

		// Do item moving if needed
		if (input_.mouseState() == MapEditor::Input::MouseState::Move)
			move_objects_.update(input_.mousePosMap());

		// Check if we have to update the info overlay
		if (selection_.hilight() != prev_hl)
		{
			// Update info overlay depending on edit mode
			updateInfoOverlay();
			info_showing_ = selection_.hasHilight();
		}
	}

	// Update overlay animation (if active)
	if (overlayActive())
		overlay_current_->update(frametime);

	// Update animations
	renderer_.updateAnimations(mult);
	if (renderer_.animationsActive())
		next_frame_length_ = 2;

	return true;
}

/* MapEditContext::openMap
 * Opens [map]
 *******************************************************************/
bool MapEditContext::openMap(Archive::mapdesc_t map)
{
	LOG_MESSAGE(1, "Opening map %s", map.name);
	if (!this->map_.readMap(map))
		return false;

	// Find camera thing
	if (canvas_)
	{
		MapThing* cam = nullptr;
		MapThing* pstart = nullptr;
		for (unsigned a = 0; a < this->map_.nThings(); a++)
		{
			MapThing* thing = this->map_.getThing(a);
			if (thing->getType() == 32000)
				cam = thing;
			if (thing->getType() == 1)
				pstart = thing;

			if (cam)
				break;
		}

		// Set canvas 3d camera
		if (cam)
			renderer_.setCameraThing(cam);
		else if (pstart)
			renderer_.setCameraThing(pstart);

		// Reset rendering data
		forceRefreshRenderer();
	}

	edit_3d_.setLinked(true, true);

	updateStatusText();
	updateThingLists();

	// Process specials
	this->map_.mapSpecials()->processMapSpecials(&(this->map_));

	return true;
}

/* MapEditContext::clearMap
 * Clears and resets the map
 *******************************************************************/
void MapEditContext::clearMap()
{
	// Clear map
	map_.clearMap();

	// Clear selection
	selection_.clear();
	selection_.clearHilight();
	edit_3d_.setLinked(true, true);

	// Clear undo manager
	undo_manager_->clear();
	last_undo_level_ = "";

	// Clear other data
	pathed_things_.clear();
}

/* MapEditContext::showItem
 * Moves and zooms the view to show the object at [index], depending
 * on the current edit mode. If [index] is negative, show the
 * current selection or hilight instead
 *******************************************************************/
void MapEditContext::showItem(int index)
{
	// Show current selection/hilight if index is not specified
	if (index < 0)
	{
		renderer_.viewFitToObjects(selection_.selectedObjects());
		return;
	}

	selection_.clear();
	int max;
	MapEditor::ItemType type;
	switch (edit_mode_)
	{
	case Mode::Vertices: type = MapEditor::ItemType::Vertex; max = map_.nVertices(); break;
	case Mode::Lines: type = MapEditor::ItemType::Line; max = map_.nLines(); break;
	case Mode::Sectors: type = MapEditor::ItemType::Sector; max = map_.nSectors(); break;
	case Mode::Things: type = MapEditor::ItemType::Thing; max = map_.nThings(); break;
	default: return;
	}

	if (index < max)
	{
		selection_.select({ index, type });
		renderer_.viewFitToObjects(selection_.selectedObjects(false));
	}
}

/* MapEditContext::getModeString
 * Returns a string representation of the current edit mode
 *******************************************************************/
string MapEditContext::modeString(bool plural) const
{
	switch (edit_mode_)
	{
	case Mode::Vertices:	return plural ? "Vertices" : "Vertex";
	case Mode::Lines:		return plural ? "Lines" : "Line";
	case Mode::Sectors:		return plural ? "Sectors" : "Sector";
	case Mode::Things:		return plural ? "Things" : "Thing";
	case Mode::Visual:		return "3D";
	};

	return plural ? "Items" : "Object";
}

/* MapEditContext::updateThingLists
 * Rebuilds thing info lists (pathed things, etc.)
 *******************************************************************/
void MapEditContext::updateThingLists()
{
	pathed_things_.clear();
	map_.getPathedThings(pathed_things_);
	map_.setThingsUpdated();
}

/* MapEditContext::setCursor
 * Sets the cursor on the canvas to [cursor]
 *******************************************************************/
void MapEditContext::setCursor(UI::MouseCursor cursor) const
{
	UI::setCursor(canvas_, cursor);
}

/* MapEditContext::forceRefreshRenderer
 * Forces a full refresh of the 2d/3d renderers
 *******************************************************************/
void MapEditContext::forceRefreshRenderer()
{
	// Update 3d mode info overlay if needed
	if (edit_mode_ == Mode::Visual)
	{
		auto hl = renderer_.renderer3D().determineHilight();
		info_3d_.update(hl.index, hl.type, &map_);
	}

	if (!canvas_->setActive())
		return;

	renderer_.forceUpdate();
}

/* MapEditContext::updateTagged
 * Rebuilds tagged object lists based on the current hilight
 *******************************************************************/
void MapEditContext::updateTagged()
{
	using Game::TagType;

	// Clear tagged lists
	tagged_sectors_.clear();
	tagged_lines_.clear();
	tagged_things_.clear();

	tagging_lines_.clear();
	tagging_things_.clear();

	// Special
	int hilight_item = selection_.hilight().index;
	if (hilight_item >= 0)
	{
		// Gather affecting objects
		int type, tag = 0, ttype = 0;
		if (edit_mode_ == Mode::Lines)
		{
			type = SLADEMap::LINEDEFS;
			tag = map_.getLine(hilight_item)->intProperty("id");
		}
		else if (edit_mode_ == Mode::Things)
		{
			type = SLADEMap::THINGS;
			tag = map_.getThing(hilight_item)->intProperty("id");
			ttype = map_.getThing(hilight_item)->getType();
		}
		else if (edit_mode_ == Mode::Sectors)
		{
			type = SLADEMap::SECTORS;
			tag = map_.getSector(hilight_item)->intProperty("id");
		}
		if (tag)
		{
			map_.getTaggingLinesById(tag, type, tagging_lines_);
			map_.getTaggingThingsById(tag, type, tagging_things_, ttype);
		}

		// Gather affected objects
		if (edit_mode_ == Mode::Lines || edit_mode_ == Mode::Things)
		{
			MapSector* back = nullptr;
			MapSector* front = nullptr;
			int tag, arg2, arg3, arg4, arg5, tid;
			auto needs_tag = TagType::None;
			// Line specials have front and possibly back sectors
			if (edit_mode_ == Mode::Lines)
			{
				MapLine* line = map_.getLine(hilight_item);
				if (line->s2()) back = line->s2()->getSector();
				if (line->s1()) front = line->s1()->getSector();
				needs_tag = Game::configuration().actionSpecial(line->intProperty("special")).needsTag();
				tag = line->intProperty("arg0");
				arg2 = line->intProperty("arg1");
				arg3 = line->intProperty("arg2");
				arg4 = line->intProperty("arg3");
				arg5 = line->intProperty("arg4");
				tid = 0;

				// Hexen and UDMF things can have specials too
			}
			else // edit_mode == Mode::Things
			{
				MapThing* thing = map_.getThing(hilight_item);
				if (Game::configuration().thingType(thing->getType()).flags() & Game::ThingType::FLAG_SCRIPT)
					needs_tag = TagType::None;
				else
				{
					needs_tag = Game::configuration().thingType(thing->getType()).needsTag();
					if (needs_tag == TagType::None)
						needs_tag = Game::configuration().actionSpecial(thing->intProperty("special")).needsTag();
					tag = thing->intProperty("arg0");
					arg2 = thing->intProperty("arg1");
					arg3 = thing->intProperty("arg2");
					arg4 = thing->intProperty("arg3");
					arg5 = thing->intProperty("arg4");
					tid = thing->intProperty("id");
				}
			}

			// Sector tag
			if (needs_tag == TagType::Sector || (needs_tag == TagType::SectorAndBack && tag > 0))
				map_.getSectorsByTag(tag, tagged_sectors_);

			// Backside sector (for local doors)
			else if ((needs_tag == TagType::Back || needs_tag == TagType::SectorAndBack) && back)
				tagged_sectors_.push_back(back);

			// Sector tag *or* backside sector (for zdoom local doors)
			else if (needs_tag == TagType::SectorOrBack)
			{
				if (tag > 0)
					map_.getSectorsByTag(tag, tagged_sectors_);
				else if (back)
					tagged_sectors_.push_back(back);
			}

			// Thing ID
			else if (needs_tag == TagType::Thing)
				map_.getThingsById(tag, tagged_things_);

			// Line ID
			else if (needs_tag == TagType::Line)
				map_.getLinesById(tag, tagged_lines_);

			// ZDoom quirkiness
			else if (needs_tag != TagType::None)
			{
				switch (needs_tag)
				{
				case TagType::Thing1Sector2:
				case TagType::Thing1Sector3:
				case TagType::Sector1Thing2:
				{
					int thingtag = (needs_tag == TagType::Sector1Thing2) ? arg2 : tag;
					int sectag = (needs_tag == TagType::Sector1Thing2) ? tag :
								 (needs_tag == TagType::Thing1Sector2) ? arg2 : arg3;
					if ((thingtag | sectag) == 0)
						break;
					else if (thingtag == 0)
						map_.getSectorsByTag(sectag, tagged_sectors_);
					else if (sectag == 0)
						map_.getThingsById(thingtag, tagged_things_);
					else // neither thingtag nor sectag are 0
						map_.getThingsByIdInSectorTag(thingtag, sectag, tagged_things_);
				}	break;
				case TagType::Thing1Thing2Thing3:
					if (arg3) map_.getThingsById(arg3, tagged_things_);
				case TagType::Thing1Thing2:
					if (arg2) map_.getThingsById(arg2, tagged_things_);
				case TagType::Thing1Thing4:
					if (tag ) map_.getThingsById(tag, tagged_things_);
				case TagType::Thing4:
					if (needs_tag == TagType::Thing1Thing4 || needs_tag == TagType::Thing4)
						if (arg4) map_.getThingsById(arg4, tagged_things_);
					break;
				case TagType::Thing5:
					if (arg5) map_.getThingsById(arg5, tagged_things_);
					break;
				case TagType::LineNegative:
					if (tag ) map_.getLinesById(abs(tag), tagged_lines_);
					break;
				case TagType::LineId1Line2:
					if (arg2) map_.getLinesById(arg2, tagged_lines_);
					break;
				case TagType::Line1Sector2:
					if (tag ) map_.getLinesById(tag, tagged_lines_);
					if (arg2) map_.getSectorsByTag(arg2, tagged_sectors_);
					break;
				case TagType::Sector1Thing2Thing3Thing5:
					if (arg5) map_.getThingsById(arg5, tagged_things_);
					if (arg3) map_.getThingsById(arg3, tagged_things_);
				case TagType::Sector1Sector2Sector3Sector4:
					if (arg4) map_.getSectorsByTag(arg4, tagged_sectors_);
					if (arg3) map_.getSectorsByTag(arg3, tagged_sectors_);
				case TagType::Sector1Sector2:
					if (arg2) map_.getSectorsByTag(arg2, tagged_sectors_);
					if (tag ) map_.getSectorsByTag(tag , tagged_sectors_);
					break;
				case TagType::Sector2Is3Line:
					if (tag)
					{
						if (arg2 == 3) map_.getLinesById(tag, tagged_lines_);
						else map_.getSectorsByTag(tag, tagged_sectors_);
					}
					break;
				case TagType::Patrol:
					if (tid) map_.getThingsById(tid, tagged_things_, 0, 9047);
					break;
				case TagType::Interpolation:
					if (tid) map_.getThingsById(tid, tagged_things_, 0, 9075);
					break;
				}
			}
		}
	}
}

/* MapEditContext::selectionUpdated
 * Called when the selection is updated, updates the properties panel
 *******************************************************************/
void MapEditContext::selectionUpdated()
{
	// Open selected objects in properties panel
	auto selected = selection_.selectedObjects();
	MapEditor::openMultiObjectProperties(selected);

	last_undo_level_ = "";

	renderer_.animateSelectionChange(selection_);

	updateStatusText();
}

/* MapEditContext::gridSize
 * Returns the current grid size
 *******************************************************************/
double MapEditContext::gridSize()
{
	return grid_sizes[grid_size_];
}

/* MapEditContext::incrementGrid
 * Increments the grid size
 *******************************************************************/
void MapEditContext::incrementGrid()
{
	grid_size_++;
	if (grid_size_ > 20)
		grid_size_ = 20;

	addEditorMessage(S_FMT("Grid Size: %dx%d", (int)gridSize(), (int)gridSize()));
	updateStatusText();
}

/* MapEditContext::decrementGrid
 * Decrements the grid size
 *******************************************************************/
void MapEditContext::decrementGrid()
{
	grid_size_--;
	int mingrid = (map_.currentFormat() == MAP_UDMF) ? 0 : 4;
	if (grid_size_ < mingrid)
		grid_size_ = mingrid;

	addEditorMessage(S_FMT("Grid Size: %dx%d", (int)gridSize(), (int)gridSize()));
	updateStatusText();
}

/* MapEditContext::snapToGrid
 * Returns the nearest grid point to [position], unless snap to grid
 * is disabled. If [force] is true, grid snap setting is ignored
 *******************************************************************/
double MapEditContext::snapToGrid(double position, bool force)
{
	if (!force && !grid_snap_)
	{
		if (map_.currentFormat() == MAP_UDMF)
			return position;
		else
			return ceil(position - 0.5);
	}

	return ceil(position / gridSize() - 0.5) * gridSize();
}

/* MapEditContext::relativeSnapToGrid
 * Used for pasting. Given an [origin] point and the current
 * [mouse_pos], snaps in such a way that the mouse is a number of
 * grid units away from the origin.
 *******************************************************************/
fpoint2_t MapEditContext::relativeSnapToGrid(fpoint2_t origin, fpoint2_t mouse_pos)
{
	fpoint2_t delta = mouse_pos - origin;
	delta.x = snapToGrid(delta.x, false);
	delta.y = snapToGrid(delta.y, false);
	return origin + delta;
}

/* MapEditContext::beginTagEdit
 * Begins a tag edit operation
 *******************************************************************/
int MapEditContext::beginTagEdit()
{
	// Check lines mode
	if (edit_mode_ != Mode::Lines)
		return 0;

	// Get selected lines
	auto lines = selection_.selectedLines();
	if (lines.size() == 0)
		return 0;

	// Get current tag
	int tag = lines[0]->intProperty("arg0");
	if (tag == 0)
		tag = map_.findUnusedSectorTag();
	current_tag_ = tag;

	// Clear tagged lists
	tagged_lines_.clear();
	tagged_sectors_.clear();
	tagged_things_.clear();

	// Sector tag (for now, 2 will be thing id tag)
	for (unsigned a = 0; a < map_.nSectors(); a++)
	{
		auto sector = map_.getSector(a);
		if (sector->intProperty("id") == current_tag_)
			tagged_sectors_.push_back(sector);
	}
	return 1;
}

/* MapEditContext::tagSectorAt
 * Applies the current tag edit tag to the sector at [x,y], or clears
 * the sector tag if it is already the same
 *******************************************************************/
void MapEditContext::tagSectorAt(double x, double y)
{
	fpoint2_t point(x, y);

	int index = map_.sectorAt(point);
	if (index < 0)
		return;

	auto sector = map_.getSector(index);
	for (unsigned a = 0; a < tagged_sectors_.size(); a++)
	{
		// Check if already tagged
		if (tagged_sectors_[a] == sector)
		{
			// Un-tag
			tagged_sectors_[a] = tagged_sectors_.back();
			tagged_sectors_.pop_back();
			addEditorMessage(S_FMT("Untagged sector %u", sector->getIndex()));
			return;
		}
	}

	// Tag
	tagged_sectors_.push_back(sector);
	addEditorMessage(S_FMT("Tagged sector %u", sector->getIndex()));
}

/* MapEditContext::endTagEdit
 * Ends the tag edit operation and applies changes if [accept] is true
 *******************************************************************/
void MapEditContext::endTagEdit(bool accept)
{
	// Get selected lines
	auto lines = selection_.selectedLines();

	if (accept)
	{
		// Begin undo level
		beginUndoRecord("Tag Edit", true, false, false);

		// Clear sector tags
		for (unsigned a = 0; a < map_.nSectors(); a++)
		{
			MapSector* sector = map_.getSector(a);
			if (sector->intProperty("id") == current_tag_)
				sector->setIntProperty("id", 0);
		}

		// If nothing selected, clear line tags
		if (tagged_sectors_.size() == 0)
			current_tag_ = 0;

		// Set line tags (in case of multiple selection)
		for (unsigned a = 0; a < lines.size(); a++)
			lines[a]->setIntProperty("arg0", current_tag_);

		// Set sector tags
		for (unsigned a = 0; a < tagged_sectors_.size(); a++)
			tagged_sectors_[a]->setIntProperty("id", current_tag_);

		// Editor message
		if (tagged_sectors_.size() == 0)
			addEditorMessage("Cleared tags");
		else
			addEditorMessage(S_FMT("Set tag %d", current_tag_));

		endUndoRecord(true);
	}
	else
		addEditorMessage("Tag edit cancelled");

	updateTagged();
	setFeatureHelp({});
}

/* MapEditContext::getEditorMessage
 * Returns the current editor message at [index]
 *******************************************************************/
string MapEditContext::editorMessage(int index)
{
	// Check index
	if (index < 0 || index >= (int)editor_messages_.size())
		return "";

	return editor_messages_[index].message;
}

/* MapEditContext::getEditorMessageTime
 * Returns the amount of time the editor message at [index] has been
 * active
 *******************************************************************/
long MapEditContext::editorMessageTime(int index)
{
	// Check index
	if (index < 0 || index >= (int)editor_messages_.size())
		return -1;

	return App::runTimer() - editor_messages_[index].act_time;
}

/* MapEditContext::addEditorMessage
 * Adds an editor message, removing the oldest if needed
 *******************************************************************/
void MapEditContext::addEditorMessage(string message)
{
	// Remove oldest message if there are too many active
	if (editor_messages_.size() >= 4)
		editor_messages_.erase(editor_messages_.begin());

	// Add message to list
	EditorMessage msg;
	msg.message = message;
	msg.act_time = App::runTimer();
	editor_messages_.push_back(msg);
}

void MapEditContext::setFeatureHelp(const vector<string>& lines)
{
	feature_help_lines_.clear();
	feature_help_lines_ = lines;

	Log::debug("Set Feature Help Text:");
	for (auto& l : feature_help_lines_)
		Log::debug(l);
}

/* MapEditContext::handleKeyBind
 * Handles the keybind [key]
 *******************************************************************/
bool MapEditContext::handleKeyBind(string key, fpoint2_t position)
{
	// --- General keybinds ---

	bool handled = true;
	if (edit_mode_ != Mode::Visual)
	{
		// Increment grid
		if (key == "me2d_grid_inc")
			incrementGrid();

		// Decrement grid
		else if (key == "me2d_grid_dec")
			decrementGrid();

		// Toggle grid snap
		else if (key == "me2d_grid_toggle_snap")
		{
			grid_snap_ = !grid_snap_;
			if (grid_snap_)
				addEditorMessage("Grid Snapping On");
			else
				addEditorMessage("Grid Snapping Off");
			updateStatusText();
		}

		// Select all
		else if (key == "select_all")
			selection_.selectAll();

		// Clear selection
		else if (key == "me2d_clear_selection")
		{
			selection_.clear();
			addEditorMessage("Selection cleared");
		}

		// Lock/unlock hilight
		else if (key == "me2d_lock_hilight")
		{
			// Toggle lock
			selection_.lockHilight(!selection_.hilightLocked());

			// Add editor message
			if (selection_.hilightLocked())
				addEditorMessage("Locked current hilight");
			else
				addEditorMessage("Unlocked hilight");
		}

		// Copy
		else if (key == "copy")
			edit_2d_.copy();

		else
			handled = false;

		if (handled)
			return handled;
	}

	// --- Sector mode keybinds ---
	if (key.StartsWith("me2d_sector") && edit_mode_ == Mode::Sectors)
	{
		// Height changes
		if		(key == "me2d_sector_floor_up8")	edit_2d_.changeSectorHeight(8, true, false);
		else if (key == "me2d_sector_floor_up")		edit_2d_.changeSectorHeight(1, true, false);
		else if (key == "me2d_sector_floor_down8")	edit_2d_.changeSectorHeight(-8, true, false);
		else if (key == "me2d_sector_floor_down")	edit_2d_.changeSectorHeight(-1, true, false);
		else if (key == "me2d_sector_ceil_up8")		edit_2d_.changeSectorHeight(8, false, true);
		else if (key == "me2d_sector_ceil_up")		edit_2d_.changeSectorHeight(1, false, true);
		else if (key == "me2d_sector_ceil_down8")	edit_2d_.changeSectorHeight(-8, false, true);
		else if (key == "me2d_sector_ceil_down")	edit_2d_.changeSectorHeight(-1, false, true);
		else if (key == "me2d_sector_height_up8")	edit_2d_.changeSectorHeight(8, true, true);
		else if (key == "me2d_sector_height_up")	edit_2d_.changeSectorHeight(1, true, true);
		else if (key == "me2d_sector_height_down8")	edit_2d_.changeSectorHeight(-8, true, true);
		else if (key == "me2d_sector_height_down")	edit_2d_.changeSectorHeight(-1, true, true);

		// Light changes
		else if (key == "me2d_sector_light_up16")	edit_2d_.changeSectorLight(true, false);
		else if (key == "me2d_sector_light_up")		edit_2d_.changeSectorLight(true, true);
		else if (key == "me2d_sector_light_down16")	edit_2d_.changeSectorLight(false, false);
		else if (key == "me2d_sector_light_down")	edit_2d_.changeSectorLight(false, true);

		// Join
		else if (key == "me2d_sector_join")			edit_2d_.joinSectors(true);
		else if (key == "me2d_sector_join_keep")	edit_2d_.joinSectors(false);

		else
			return false;
	}

	// --- 3d mode keybinds ---
	else if (key.StartsWith("me3d_") && edit_mode_ == Mode::Visual)
	{
		// Check is UDMF
		bool is_udmf = map_.currentFormat() == MAP_UDMF;

		// Clear selection
		if (key == "me3d_clear_selection")
		{
			selection_.clear();
			addEditorMessage("Selection cleared");
		}

		// Toggle linked light levels
		else if (key == "me3d_light_toggle_link")
		{
			if (!is_udmf || !Game::configuration().featureSupported(Game::UDMFFeature::FlatLighting))
				addEditorMessage("Unlinked light levels not supported in this game configuration");
			else
			{
				if (edit_3d_.toggleLightLink())
					addEditorMessage("Flat light levels linked");
				else
					addEditorMessage("Flat light levels unlinked");
			}
		}

		// Toggle linked offsets
		else if (key == "me3d_wall_toggle_link_ofs")
		{
			if (!is_udmf || !Game::configuration().featureSupported(Game::UDMFFeature::TextureOffsets))
				addEditorMessage("Unlinked wall offsets not supported in this game configuration");
			else
			{
				if (edit_3d_.toggleOffsetLink())
					addEditorMessage("Wall offsets linked");
				else
					addEditorMessage("Wall offsets unlinked");
			}
		}

		// Copy/paste
		else if (key == "me3d_copy_tex_type")	edit_3d_.copy(Edit3D::CopyType::TexType);
		else if (key == "me3d_paste_tex_type")	edit_3d_.paste(Edit3D::CopyType::TexType);
		else if (key == "me3d_paste_tex_adj")	edit_3d_.floodFill(Edit3D::CopyType::TexType);

		// Light changes
		else if	(key == "me3d_light_up16")		edit_3d_.changeSectorLight(16);
		else if (key == "me3d_light_up")		edit_3d_.changeSectorLight(1);
		else if (key == "me3d_light_down16")	edit_3d_.changeSectorLight(-16);
		else if (key == "me3d_light_down")		edit_3d_.changeSectorLight(-1);

		// Wall/Flat offset changes
		else if	(key == "me3d_xoff_up8")	edit_3d_.changeOffset(8, true);
		else if	(key == "me3d_xoff_up")		edit_3d_.changeOffset(1, true);
		else if	(key == "me3d_xoff_down8")	edit_3d_.changeOffset(-8, true);
		else if	(key == "me3d_xoff_down")	edit_3d_.changeOffset(-1, true);
		else if	(key == "me3d_yoff_up8")	edit_3d_.changeOffset(8, false);
		else if	(key == "me3d_yoff_up")		edit_3d_.changeOffset(1, false);
		else if	(key == "me3d_yoff_down8")	edit_3d_.changeOffset(-8, false);
		else if	(key == "me3d_yoff_down")	edit_3d_.changeOffset(-1, false);

		// Height changes
		else if	(key == "me3d_flat_height_up8")		edit_3d_.changeSectorHeight(8);
		else if	(key == "me3d_flat_height_up")		edit_3d_.changeSectorHeight(1);
		else if	(key == "me3d_flat_height_down8")	edit_3d_.changeSectorHeight(-8);
		else if	(key == "me3d_flat_height_down")	edit_3d_.changeSectorHeight(-1);

		// Thing height changes
		else if (key == "me3d_thing_up")	edit_3d_.changeThingZ(1);
		else if (key == "me3d_thing_up8")	edit_3d_.changeThingZ(8);
		else if (key == "me3d_thing_down")	edit_3d_.changeThingZ(-1);
		else if (key == "me3d_thing_down8")	edit_3d_.changeThingZ(-8);

		// Generic height change
		else if (key == "me3d_generic_up8")		edit_3d_.changeHeight(8);
		else if (key == "me3d_generic_up")		edit_3d_.changeHeight(1);
		else if (key == "me3d_generic_down8")	edit_3d_.changeHeight(-8);
		else if (key == "me3d_generic_down")	edit_3d_.changeHeight(-1);

		// Wall/Flat scale changes
		else if (key == "me3d_scalex_up_l" && is_udmf)		edit_3d_.changeScale(1, true);
		else if (key == "me3d_scalex_up_s" && is_udmf)		edit_3d_.changeScale(0.1, true);
		else if (key == "me3d_scalex_down_l" && is_udmf)	edit_3d_.changeScale(-1, true);
		else if (key == "me3d_scalex_down_s" && is_udmf)	edit_3d_.changeScale(-0.1, true);
		else if (key == "me3d_scaley_up_l" && is_udmf)		edit_3d_.changeScale(1, false);
		else if (key == "me3d_scaley_up_s" && is_udmf)		edit_3d_.changeScale(0.1, false);
		else if (key == "me3d_scaley_down_l" && is_udmf)	edit_3d_.changeScale(-1, false);
		else if (key == "me3d_scaley_down_s" && is_udmf)	edit_3d_.changeScale(-0.1, false);

		// Auto-align
		else if (key == "me3d_wall_autoalign_x")
			edit_3d_.autoAlignX(selection_.hilight());

		// Reset wall offsets
		else if (key == "me3d_wall_reset")
			edit_3d_.resetOffsets();

		// Toggle lower unpegged
		else if (key == "me3d_wall_unpeg_lower")
			edit_3d_.toggleUnpegged(true);

		// Toggle upper unpegged
		else if (key == "me3d_wall_unpeg_upper")
			edit_3d_.toggleUnpegged(false);

		// Remove thing
		else if (key == "me3d_thing_remove")
			edit_3d_.deleteThing();

		else
			return false;
	}
	else
		return false;

	return true;
}

/* MapEditContext::updateDisplay
 * Updates the map object properties panel and current info overlay
 * from the current hilight/selection
 *******************************************************************/
void MapEditContext::updateDisplay()
{
	// Update map object properties panel
	auto selection = selection_.selectedObjects();
	MapEditor::openMultiObjectProperties(selection);

	// Update canvas info overlay
	if (canvas_)
	{
		updateInfoOverlay();
		canvas_->Refresh();
	}
}

/* MapEditContext::updateStatusText
 * Updates the window status bar text (mode, grid, etc.)
 *******************************************************************/
void MapEditContext::updateStatusText()
{
	// Edit mode
	string mode = "Mode: ";
	switch (edit_mode_)
	{
	case Mode::Vertices: mode += "Vertices"; break;
	case Mode::Lines: mode += "Lines"; break;
	case Mode::Sectors: mode += "Sectors"; break;
	case Mode::Things: mode += "Things"; break;
	case Mode::Visual: mode += "3D"; break;
	}

	if (edit_mode_ == Mode::Sectors)
	{
		switch (sector_mode_)
		{
		case SectorMode::Both: mode += " (Normal)"; break;
		case SectorMode::Floor: mode += " (Floors)"; break;
		case SectorMode::Ceiling: mode += " (Ceilings)"; break;
		}
	}

	if (edit_mode_ != Mode::Visual && selection_.size() > 0)
	{
		mode += S_FMT(" (%lu selected)", selection_.size());
	}

	MapEditor::setStatusText(mode, 1);

	// Grid
	string grid;
	if (gridSize() < 1)
		grid = S_FMT("Grid: %1.2fx%1.2f", gridSize(), gridSize());
	else
		grid = S_FMT("Grid: %dx%d", (int)gridSize(), (int)gridSize());

	if (grid_snap_)
		grid += " (Snapping ON)";
	else
		grid += " (Snapping OFF)";

	MapEditor::setStatusText(grid, 2);
}

/* MapEditContext::beginUndoRecord
 * Begins recording undo level [name]. [mod] is true if the operation
 * about to begin will modify object properties, [create/del] are
 * true if it will create or delete objects
 *******************************************************************/
void MapEditContext::beginUndoRecord(string name, bool mod, bool create, bool del)
{
	// Setup
	UndoManager* manager = (edit_mode_ == Mode::Visual) ? edit_3d_.undoManager() : undo_manager_.get();
	if (manager->currentlyRecording())
		return;
	undo_modified_ = mod;
	undo_deleted_ = del;
	undo_created_ = create;

	// Begin recording
	manager->beginRecord(name);

	// Init map/objects for recording
	if (undo_modified_)
		MapObject::beginPropBackup(App::runTimer());
	if (undo_deleted_ || undo_created_)
		us_create_delete_ = new MapEditor::MapObjectCreateDeleteUS();

	// Make sure all modified objects will be picked up
	wxMilliSleep(5);

	last_undo_level_ = "";
}

/* MapEditContext::beginUndoRecordLocked
 * Same as beginUndoRecord, except that subsequent calls to this
 * will not record another undo level if [name] is the same as last
 * (used for repeated operations like offset changes etc. so that
 * 5 offset changes to the same object only create 1 undo level)
 *******************************************************************/
void MapEditContext::beginUndoRecordLocked(string name, bool mod, bool create, bool del)
{
	if (name != last_undo_level_)
	{
		beginUndoRecord(name, mod, create, del);
		last_undo_level_ = name;
	}
}

/* MapEditContext::endUndoRecord
 * Finish recording undo level. Discarded if [success] is false
 *******************************************************************/
void MapEditContext::endUndoRecord(bool success)
{
	UndoManager* manager = (edit_mode_ == Mode::Visual) ? edit_3d_.undoManager() : undo_manager_.get();

	if (manager->currentlyRecording())
	{
		// Record necessary undo steps
		MapObject::beginPropBackup(-1);
		bool modified = false;
		bool created_deleted = false;
		if (undo_modified_)
			modified = manager->recordUndoStep(new MapEditor::MultiMapObjectPropertyChangeUS());
		if (undo_created_ || undo_deleted_)
		{
			((MapEditor::MapObjectCreateDeleteUS*)us_create_delete_)->checkChanges();
			created_deleted = manager->recordUndoStep(us_create_delete_);
		}

		// End recording
		manager->endRecord(success && (modified || created_deleted));
	}
	updateThingLists();
	us_create_delete_ = nullptr;
	map_.recomputeSpecials();
}

/* MapEditContext::recordPropertyChangeUndoStep
 * Records an object property change undo step for [object]
 *******************************************************************/
void MapEditContext::recordPropertyChangeUndoStep(MapObject* object)
{
	UndoManager* manager = (edit_mode_ == Mode::Visual) ? edit_3d_.undoManager() : undo_manager_.get();
	manager->recordUndoStep(new MapEditor::PropertyChangeUS(object));
}

/* MapEditContext::doUndo
 * Undoes the current undo level
 *******************************************************************/
void MapEditContext::doUndo()
{
	// Clear selection first, since part of it may become invalid
	selection_.clear();

	// Undo
	int time = App::runTimer() - 1;
	UndoManager* manager = (edit_mode_ == Mode::Visual) ? edit_3d_.undoManager() : undo_manager_.get();
	string undo_name = manager->undo();

	// Editor message
	if (undo_name != "")
	{
		addEditorMessage(S_FMT("Undo: %s", undo_name));

		// Refresh stuff
		//updateTagged();
		map_.rebuildConnectedLines();
		map_.rebuildConnectedSides();
		map_.setGeometryUpdated();
		map_.updateGeometryInfo(time);
		last_undo_level_ = "";
	}
	updateThingLists();
	map_.recomputeSpecials();
}

/* MapEditContext::doRedo
 * Redoes the next undo level
 *******************************************************************/
void MapEditContext::doRedo()
{
	// Clear selection first, since part of it may become invalid
	selection_.clear();

	// Redo
	int time = App::runTimer() - 1;
	UndoManager* manager = (edit_mode_ == Mode::Visual) ? edit_3d_.undoManager() : undo_manager_.get();
	string undo_name = manager->redo();

	// Editor message
	if (undo_name != "")
	{
		addEditorMessage(S_FMT("Redo: %s", undo_name));

		// Refresh stuff
		//updateTagged();
		map_.rebuildConnectedLines();
		map_.rebuildConnectedSides();
		map_.setGeometryUpdated();
		map_.updateGeometryInfo(time);
		last_undo_level_ = "";
	}
	updateThingLists();
	map_.recomputeSpecials();
}

bool MapEditContext::overlayActive()
{
	if (!overlay_current_)
		return false;
	else
		return overlay_current_->isActive();
}

/* MapEditContext::swapPlayerStart3d
 * Moves the player 1 start thing to the current position and
 * direction of the 3d mode camera
 *******************************************************************/
void MapEditContext::swapPlayerStart3d()
{
	// Find player 1 start
	MapThing* pstart = nullptr;
	for (unsigned a = 0; a < map_.nThings(); a++)
		if (map_.getThing(a)->getType() == 1)
		{
			pstart = map_.getThing(a);
			break;
		}
	if (!pstart)
		return;

	// Save existing player start pos+dir
	player_start_pos_.set(pstart->point());
	player_start_dir_ = pstart->getAngle();

	fpoint2_t campos = renderer_.cameraPos2D();
	pstart->setPos(campos.x, campos.y);
	pstart->setAnglePoint(campos + renderer_.cameraDir2D());
}

/* MapEditContext::swapPlayerStart2d
 * Moves the player 1 start thing to [pos]
 *******************************************************************/
void MapEditContext::swapPlayerStart2d(fpoint2_t pos)
{
	// Find player 1 start
	MapThing* pstart = nullptr;
	for (unsigned a = 0; a < map_.nThings(); a++)
		if (map_.getThing(a)->getType() == 1)
		{
			pstart = map_.getThing(a);
			break;
		}
	if (!pstart)
		return;

	// Save existing player start pos+dir
	player_start_pos_.set(pstart->point());
	player_start_dir_ = pstart->getAngle();

	pstart->setPos(pos.x, pos.y);
}

/* MapEditContext::resetPlayerStart
 * Resets the player 1 start thing to its original position
 *******************************************************************/
void MapEditContext::resetPlayerStart()
{
	// Find player 1 start
	MapThing* pstart = nullptr;
	for (unsigned a = 0; a < map_.nThings(); a++)
		if (map_.getThing(a)->getType() == 1)
		{
			pstart = map_.getThing(a);
			break;
		}
	if (!pstart)
		return;

	pstart->setPos(player_start_pos_.x, player_start_pos_.y);
	pstart->setIntProperty("angle", player_start_dir_);
}

/* MapEditContext::openSectorTextureOverlay
 * Opens the sector texture selection overlay
 *******************************************************************/
void MapEditContext::openSectorTextureOverlay(vector<MapSector*>& sectors)
{
	overlay_current_ = std::make_unique<SectorTextureOverlay>();
	((SectorTextureOverlay*)overlay_current_.get())->openSectors(sectors);
}

void MapEditContext::openQuickTextureOverlay()
{
	if (QuickTextureOverlay3d::ok(selection_))
	{
		overlay_current_ = std::make_unique<QuickTextureOverlay3d>(this);

		renderer_.renderer3D().enableHilight(false);
		renderer_.renderer3D().enableSelection(false);
		selection_.lockHilight(true);
	}
}

void MapEditContext::openLineTextureOverlay()
{
	// Get selection
	auto lines = selection_.selectedLines();

	// Open line texture overlay if anything is selected
	if (lines.size() > 0)
	{
		overlay_current_ = std::make_unique<LineTextureOverlay>();
		((LineTextureOverlay*)overlay_current_.get())->openLines(lines);
	}
}

void MapEditContext::closeCurrentOverlay(bool cancel) const
{
	if (overlay_current_ && overlay_current_->isActive())
		overlay_current_->close(cancel);
}

/* MapEditContext::updateInfoOverlay
 * Updates the current info overlay, depending on edit mode
 *******************************************************************/
void MapEditContext::updateInfoOverlay()
{
	// Update info overlay depending on edit mode
	switch (edit_mode_)
	{
	case Mode::Vertices:	info_vertex_.update(selection_.hilightedVertex()); break;
	case Mode::Lines:		info_line_.update(selection_.hilightedLine()); break;
	case Mode::Sectors:		info_sector_.update(selection_.hilightedSector()); break;
	case Mode::Things:		info_thing_.update(selection_.hilightedThing()); break;
	default: break;
	}
}

/* MapEditContext::drawInfoOverlay
 * Draws the current info overlay
 *******************************************************************/
void MapEditContext::drawInfoOverlay(const point2_t& size, float alpha)
{
	switch (edit_mode_)
	{
	case Mode::Vertices:
		info_vertex_.draw(size.y, size.x, alpha); return;
	case Mode::Lines:
		info_line_.draw(size.y, size.x, alpha); return;
	case Mode::Sectors:
		info_sector_.draw(size.y, size.x, alpha); return;
	case Mode::Things:
		info_thing_.draw(size.y, size.x, alpha); return;
	case Mode::Visual:
		info_3d_.draw(size.y, size.x, size.x * 0.5, alpha); return;
	}
}

/* MapEditContext::handleAction
 * Handles an SAction [id]. Returns true if the action was handled
 * here
 *******************************************************************/
bool MapEditContext::handleAction(string id)
{
	using namespace MapEditor;

	auto mouse_state = input_.mouseState();

	// Skip if canvas not shown
	if (!canvas_->IsShown())
		return false;

	// Skip if overlay is active
	if (overlayActive())
		return false;

	// Vertices mode
	if (id == "mapw_mode_vertices")
	{
		setEditMode(Mode::Vertices);
		return true;
	}

	// Lines mode
	else if (id == "mapw_mode_lines")
	{
		setEditMode(Mode::Lines);
		return true;
	}

	// Sectors mode
	else if (id == "mapw_mode_sectors")
	{
		setEditMode(Mode::Sectors);
		return true;
	}

	// Things mode
	else if (id == "mapw_mode_things")
	{
		setEditMode(Mode::Things);
		return true;
	}

	// 3d mode
	else if (id == "mapw_mode_3d")
	{
		canvas_->SetFocusFromKbd();
		canvas_->SetFocus();
		setEditMode(Mode::Visual);
		return true;
	}

	// 'None' (wireframe) flat type
	else if (id == "mapw_flat_none")
	{
		flat_drawtype = 0;
		return true;
	}

	// 'Untextured' flat type
	else if (id == "mapw_flat_untextured")
	{
		flat_drawtype = 1;
		return true;
	}

	// 'Textured' flat type
	else if (id == "mapw_flat_textured")
	{
		flat_drawtype = 2;
		return true;
	}

	// Normal sector edit mode
	else if (id == "mapw_sectormode_normal")
	{
		setSectorEditMode(SectorMode::Both);
		return true;
	}

	// Floors sector edit mode
	else if (id == "mapw_sectormode_floor")
	{
		setSectorEditMode(SectorMode::Floor);
		return true;
	}

	// Ceilings sector edit mode
	else if (id == "mapw_sectormode_ceiling")
	{
		setSectorEditMode(SectorMode::Ceiling);
		return true;
	}

	// Begin line drawing
	else if (id == "mapw_draw_lines" && mouse_state == Input::MouseState::Normal)
	{
		line_draw_.begin();
		return true;
	}

	// Begin shape drawing
	else if (id == "mapw_draw_shape" && mouse_state == Input::MouseState::Normal)
	{
		line_draw_.begin(true);
		return true;
	}

	// Begin object edit
	else if (id == "mapw_edit_objects" && mouse_state == Input::MouseState::Normal)
	{
		object_edit_.begin();
		return true;
	}

	// Show full map
	else if (id == "mapw_show_fullmap")
	{
		renderer_.viewFitToMap();
		return true;
	}

	// Show item
	else if (id == "mapw_show_item")
	{
		// Setup dialog
		ShowItemDialog dlg(MapEditor::windowWx());
		switch (editMode())
		{
		case Mode::Vertices:
			dlg.setType(MOBJ_VERTEX); break;
		case Mode::Lines:
			dlg.setType(MOBJ_LINE); break;
		case Mode::Sectors:
			dlg.setType(MOBJ_SECTOR); break;
		case Mode::Things:
			dlg.setType(MOBJ_THING); break;
		default:
			return true;
		}

		// Show dialog
		if (dlg.ShowModal() == wxID_OK)
		{
			// Get entered index
			int index = dlg.getIndex();
			if (index < 0)
				return true;

			// Set appropriate edit mode
			bool side = false;
			switch (dlg.getType())
			{
			case MOBJ_VERTEX:
				setEditMode(Mode::Vertices); break;
			case MOBJ_LINE:
				setEditMode(Mode::Lines); break;
			case MOBJ_SIDE:
				setEditMode(Mode::Lines); side = true; break;
			case MOBJ_SECTOR:
				setEditMode(Mode::Sectors); break;
			case MOBJ_THING:
				setEditMode(Mode::Things); break;
			default:
				break;
			}

			// If side, get its parent line
			if (side)
			{
				MapSide* s = map_.getSide(index);
				if (s && s->getParentLine())
					index = s->getParentLine()->getIndex();
				else
					index = -1;
			}

			// Show the item
			if (index > -1)
				showItem(index);
		}

		return true;
	}

	// Mirror Y
	else if (id == "mapw_mirror_y")
	{
		edit_2d_.mirror(false);
		return true;
	}

	// Mirror X
	else if (id == "mapw_mirror_x")
	{
		edit_2d_.mirror(true);
		return true;
	}


	// --- Context menu ---

	// Move 3d mode camera
	else if (id == "mapw_camera_set")
	{
		fpoint3_t pos(input().mousePosMap());
		MapSector* sector = map_.getSector(map_.sectorAt(input_.mousePosMap()));
		if (sector)
			pos.z = sector->getFloorHeight() + 40;
		renderer_.renderer3D().cameraSetPosition(pos);
		return true;
	}

	// Edit item properties
	else if (id == "mapw_item_properties")
		edit_2d_.editObjectProperties();

	// --- Vertex context menu ---

	// Create vertex
	else if (id == "mapw_vertex_create")
	{
		edit_2d_.createVertex(input_.mousePosMap().x, input_.mousePosMap().y);
		return true;
	}

	// --- Line context menu ---

	// Change line texture
	else if (id == "mapw_line_changetexture")
	{
		openLineTextureOverlay();
		return true;
	}

	// Change line special
	else if (id == "mapw_line_changespecial")
	{
		// Get selection
		auto selection = selection_.selectedObjects();

		// Open action special selection dialog
		if (selection.size() > 0)
		{
			ActionSpecialDialog dlg(MapEditor::windowWx(), true);
			dlg.openLines(selection);
			if (dlg.ShowModal() == wxID_OK)
			{
				beginUndoRecord("Change Line Special", true, false, false);
				dlg.applyTo(selection, true);
				endUndoRecord();
				renderer_.renderer2D().forceUpdate();
			}
		}

		return true;
	}

	// Tag to
	else if (id == "mapw_line_tagedit")
	{
		if (beginTagEdit() > 0)
		{
			input_.setMouseState(Input::MouseState::TagSectors);

			// Setup help text
			string key_accept = KeyBind::getBind("map_edit_accept").keysAsString();
			string key_cancel = KeyBind::getBind("map_edit_cancel").keysAsString();
			setFeatureHelp({
				"Tag Edit",
				S_FMT("%s = Accept", key_accept),
				S_FMT("%s = Cancel", key_cancel),
				"Left Click = Toggle tagged sector"
			});
		}

		return true;
	}

	// Correct sectors
	else if (id == "mapw_line_correctsectors")
	{
		edit_2d_.correctLineSectors();
		return true;
	}

	// Flip
	else if (id == "mapw_line_flip")
	{
		edit_2d_.flipLines();
		return true;
	}

	// --- Thing context menu ---

	// Change thing type
	else if (id == "mapw_thing_changetype")
	{
		edit_2d_.changeThingType();
		return true;
	}

	// Create thing
	else if (id == "mapw_thing_create")
	{
		edit_2d_.createThing(input_.mouseDownPosMap().x, input_.mouseDownPosMap().y);
		return true;
	}

	// --- Sector context menu ---

	// Change sector texture
	else if (id == "mapw_sector_changetexture")
	{
		edit_2d_.changeSectorTexture();
		return true;
	}

	// Change sector special
	else if (id == "mapw_sector_changespecial")
	{
		// Get selection
		auto selection = selection_.selectedSectors();

		// Open sector special selection dialog
		if (selection.size() > 0)
		{
			SectorSpecialDialog dlg(MapEditor::windowWx());
			dlg.setup(selection[0]->intProperty("special"));
			if (dlg.ShowModal() == wxID_OK)
			{
				// Set specials of selected sectors
				int special = dlg.getSelectedSpecial();
				beginUndoRecord("Change Sector Special", true, false, false);
				for (auto sector : selection)
					sector->setIntProperty("special", special);
				endUndoRecord();
			}
		}
	}

	// Create sector
	else if (id == "mapw_sector_create")
	{
		edit_2d_.createSector(input_.mouseDownPosMap().x, input_.mouseDownPosMap().y);
		return true;
	}

	// Merge sectors
	else if (id == "mapw_sector_join")
	{
		edit_2d_.joinSectors(false);
		return true;
	}

	// Join sectors
	else if (id == "mapw_sector_join_keep")
	{
		edit_2d_.joinSectors(true);
		return true;
	}

	// Not handled here
	return false;
}


/*******************************************************************
 * CONSOLE COMMANDS
 *******************************************************************/

CONSOLE_COMMAND(m_show_item, 1, true)
{
	int index = atoi(CHR(args[0]));
	MapEditor::editContext().showItem(index);
}

CONSOLE_COMMAND(m_check, 0, true)
{
	if (args.empty())
	{
		Log::console("Usage: m_check <check1> <check2> ...");
		Log::console("Available map checks:");
		Log::console("missing_tex: Check for missing textures");
		Log::console("special_tags: Check for missing action special tags");
		Log::console("intersecting_lines: Check for intersecting lines");
		Log::console("overlapping_lines: Check for overlapping lines");
		Log::console("overlapping_things: Check for overlapping things");
		Log::console("unknown_textures: Check for unknown wall textures");
		Log::console("unknown_flats: Check for unknown floor/ceiling textures");
		Log::console("unknown_things: Check for unknown thing types");
		Log::console("stuck_things: Check for things stuck in walls");
		Log::console("sector_references: Check for wrong sector references");
		Log::console("all: Run all checks");

		return;
	}

	auto map = &(MapEditor::editContext().map());
	auto texman = &(MapEditor::textureManager());

	// Get checks to run
	vector<MapCheck*> checks;
	for (unsigned a = 0; a < args.size(); a++)
	{
		string id = args[a].Lower();
		unsigned n = checks.size();

		if (id == "missing_tex" || id == "all")
			checks.push_back(MapCheck::missingTextureCheck(map));
		if (id == "special_tags" || id == "all")
			checks.push_back(MapCheck::specialTagCheck(map));
		if (id == "intersecting_lines" || id == "all")
			checks.push_back(MapCheck::intersectingLineCheck(map));
		if (id == "overlapping_lines" || id == "all")
			checks.push_back(MapCheck::overlappingLineCheck(map));
		if (id == "overlapping_things" || id == "all")
			checks.push_back(MapCheck::overlappingThingCheck(map));
		if (id == "unknown_textures" || id == "all")
			checks.push_back(MapCheck::unknownTextureCheck(map, texman));
		if (id == "unknown_flats" || id == "all")
			checks.push_back(MapCheck::unknownFlatCheck(map, texman));
		if (id == "unknown_things" || id == "all")
			checks.push_back(MapCheck::unknownThingTypeCheck(map));
		if (id == "stuck_things" || id == "all")
			checks.push_back(MapCheck::stuckThingsCheck(map));
		if (id == "sector_references" || id == "all")
			checks.push_back(MapCheck::sectorReferenceCheck(map));
		
		if (n == checks.size())
			Log::console(S_FMT("Unknown check \"%s\"", id));
	}

	// Run checks
	for (unsigned a = 0; a < checks.size(); a++)
	{
		// Run
		Log::console(checks[a]->progressText());
		checks[a]->doCheck();

		// Check if no problems found
		if (checks[a]->nProblems() == 0)
			Log::console(checks[a]->problemDesc(0));

		// List problem details
		for (unsigned b = 0; b < checks[a]->nProblems(); b++)
			Log::console(checks[a]->problemDesc(b));

		// Clean up
		delete checks[a];
	}
}





// testing stuff

CONSOLE_COMMAND(m_test_sector, 0, false)
{
	sf::Clock clock;
	SLADEMap& map = MapEditor::editContext().map();
	for (unsigned a = 0; a < map.nThings(); a++)
		map.sectorAt(map.getThing(a)->point());
	long ms = clock.getElapsedTime().asMilliseconds();
	LOG_MESSAGE(1, "Took %ldms", ms);
}

CONSOLE_COMMAND(m_test_mobj_backup, 0, false)
{
	sf::Clock clock;
	sf::Clock totalClock;
	SLADEMap& map = MapEditor::editContext().map();
	mobj_backup_t* backup = new mobj_backup_t();

	// Vertices
	for (unsigned a = 0; a < map.nVertices(); a++)
		map.getVertex(a)->backup(backup);
	LOG_MESSAGE(1, "Vertices: %dms", clock.getElapsedTime().asMilliseconds());

	// Lines
	clock.restart();
	for (unsigned a = 0; a < map.nLines(); a++)
		map.getLine(a)->backup(backup);
	LOG_MESSAGE(1, "Lines: %dms", clock.getElapsedTime().asMilliseconds());

	// Sides
	clock.restart();
	for (unsigned a = 0; a < map.nSides(); a++)
		map.getSide(a)->backup(backup);
	LOG_MESSAGE(1, "Sides: %dms", clock.getElapsedTime().asMilliseconds());

	// Sectors
	clock.restart();
	for (unsigned a = 0; a < map.nSectors(); a++)
		map.getSector(a)->backup(backup);
	LOG_MESSAGE(1, "Sectors: %dms", clock.getElapsedTime().asMilliseconds());

	// Things
	clock.restart();
	for (unsigned a = 0; a < map.nThings(); a++)
		map.getThing(a)->backup(backup);
	LOG_MESSAGE(1, "Things: %dms", clock.getElapsedTime().asMilliseconds());

	LOG_MESSAGE(1, "Total: %dms", totalClock.getElapsedTime().asMilliseconds());
}

CONSOLE_COMMAND(m_vertex_attached, 1, false)
{
	MapVertex* vertex = MapEditor::editContext().map().getVertex(atoi(CHR(args[0])));
	if (vertex)
	{
		LOG_MESSAGE(1, "Attached lines:");
		for (unsigned a = 0; a < vertex->nConnectedLines(); a++)
			LOG_MESSAGE(1, "Line #%lu", vertex->connectedLine(a)->getIndex());
	}
}

CONSOLE_COMMAND(m_n_polys, 0, false)
{
	SLADEMap& map = MapEditor::editContext().map();
	int npoly = 0;
	for (unsigned a = 0; a < map.nSectors(); a++)
		npoly += map.getSector(a)->getPolygon()->nSubPolys();

	Log::console(S_FMT("%d polygons total", npoly));
}

CONSOLE_COMMAND(mobj_info, 1, false)
{
	long id;
	args[0].ToLong(&id);

	MapObject* obj = MapEditor::editContext().map().getObjectById(id);
	if (!obj)
		Log::console("Object id out of range");
	else
	{
		mobj_backup_t bak;
		obj->backup(&bak);
		Log::console(S_FMT("Object %d: %s #%lu", id, obj->getTypeName(), obj->getIndex()));
		Log::console("Properties:");
		Log::console(bak.properties.toString());
		Log::console("Properties (internal):");
		Log::console(bak.props_internal.toString());
	}
}

//CONSOLE_COMMAND(m_test_save, 1, false) {
//	vector<ArchiveEntry*> entries;
//	theMapEditor->MapEditContext().getMap().writeDoomMap(entries);
//	WadArchive temp;
//	temp.addNewEntry("MAP01");
//	for (unsigned a = 0; a < entries.size(); a++)
//		temp.addEntry(entries[a]);
//	temp.save(args[0]);
//	temp.close();
//}
