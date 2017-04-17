
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
#include "GameConfiguration/GameConfiguration.h"
#include "General/Clipboard.h"
#include "General/Console/Console.h"
#include "General/UndoRedo.h"
#include "MapChecks.h"
#include "MapEditContext.h"
#include "MapTextureManager.h"
#include "SectorBuilder.h"
#include "UI/MapCanvas.h"
#include "UI/MapEditorWindow.h"
#include "UndoSteps.h"
#include "Utility/MathStuff.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
double grid_sizes[] = { 0.05, 0.1, 0.25, 0.5, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536 };
CVAR(Bool, map_merge_undo_step, true, CVAR_SAVE)
CVAR(Bool, map_remove_invalid_lines, false, CVAR_SAVE)
CVAR(Bool, map_merge_lines_on_delete_vertex, false, CVAR_SAVE)
CVAR(Bool, selection_clear_move, true, CVAR_SAVE)


/*******************************************************************
 * MapEditContext CLASS FUNCTIONS
 *******************************************************************/
using MapEditor::Mode;
using MapEditor::SectorMode;

/* MapEditContext::MapEditContext
 * MapEditContext class constructor
 *******************************************************************/
MapEditContext::MapEditContext() :
	canvas_(nullptr),
	undo_manager_(new UndoManager(&map_)),
	us_create_delete_(nullptr),
	edit_mode_(Mode::Lines),
	selection_(this),
	grid_size_(9),
	sector_mode_(SectorMode::Both),
	grid_snap_(true),
	current_tag_(0),
	undo_modified_(false),
	undo_created_(false),
	undo_deleted_(false),
	move_item_closest_(0),
	line_draw_(*this),
	edit_3d_(*this),
	copy_thing_(nullptr),
	copy_sector_(nullptr),
	copy_line_(nullptr),
	player_start_dir_(0)
{
}

/* MapEditContext::~MapEditContext
 * MapEditContext class destructor
 *******************************************************************/
MapEditContext::~MapEditContext()
{
	if (copy_thing_) delete copy_thing_;
	if (copy_sector_) delete copy_sector_;
	if (copy_line_)
	{
		if(copy_line_->s1())
		{
			delete copy_line_->s1();
		}

		if(copy_line_->s2())
		{
			delete copy_line_->s2();
		}

		delete copy_line_;
	}
	delete undo_manager_;
	//delete undo_manager_3d;
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
		MapEditor::setUndoManager(undo_manager_);

	auto old_edit_mode = edit_mode_;

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
	selection_.migrate(old_edit_mode, edit_mode_);

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
		MapThing* cam = NULL;
		MapThing* pstart = NULL;
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
			canvas_->set3dCameraThing(cam);
		else if (pstart)
			canvas_->set3dCameraThing(pstart);

		// Reset rendering data
		canvas_->forceRefreshRenderer();
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
 * on the current edit mode
 *******************************************************************/
void MapEditContext::showItem(int index)
{
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
		//selection.push_back(index);
		selection_.select({ index, type });
		if (canvas_) canvas_->viewShowObject();
	}
}

/* MapEditContext::getModeString
 * Returns a string representation of the current edit mode
 *******************************************************************/
string MapEditContext::modeString(bool plural)
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

/* MapEditContext::updateTagged
 * Rebuilds tagged object lists based on the current hilight
 *******************************************************************/
void MapEditContext::updateTagged()
{
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
			MapSector* back = NULL;
			MapSector* front = NULL;
			int needs_tag, tag, arg2, arg3, arg4, arg5, tid;
			// Line specials have front and possibly back sectors
			if (edit_mode_ == Mode::Lines)
			{
				MapLine* line = map_.getLine(hilight_item);
				if (line->s2()) back = line->s2()->getSector();
				if (line->s1()) front = line->s1()->getSector();
				needs_tag = theGameConfiguration->actionSpecial(line->intProperty("special"))->needsTag();
				tag = line->intProperty("arg0");
				arg2 = line->intProperty("arg1");
				arg3 = line->intProperty("arg2");
				arg4 = line->intProperty("arg3");
				arg5 = line->intProperty("arg4");
				tid = 0;

				// Hexen and UDMF things can have specials too
			}
			else /* edit_mode == Mode::Things */
			{
				MapThing* thing = map_.getThing(hilight_item);
				if (theGameConfiguration->thingType(thing->getType())->getFlags() & THING_SCRIPT)
					needs_tag = AS_TT_NO;
				else
				{
					needs_tag = theGameConfiguration->thingType(thing->getType())->needsTag();
					if (!needs_tag)
						needs_tag = theGameConfiguration->actionSpecial(thing->intProperty("special"))->needsTag();
					tag = thing->intProperty("arg0");
					arg2 = thing->intProperty("arg1");
					arg3 = thing->intProperty("arg2");
					arg4 = thing->intProperty("arg3");
					arg5 = thing->intProperty("arg4");
					tid = thing->intProperty("id");
				}
			}

			// Sector tag
			if (needs_tag == AS_TT_SECTOR ||
					(needs_tag == AS_TT_SECTOR_AND_BACK && tag > 0))
				map_.getSectorsByTag(tag, tagged_sectors_);

			// Backside sector (for local doors)
			else if ((needs_tag == AS_TT_SECTOR_BACK || needs_tag == AS_TT_SECTOR_AND_BACK) && back)
				tagged_sectors_.push_back(back);

			// Sector tag *or* backside sector (for zdoom local doors)
			else if (needs_tag == AS_TT_SECTOR_OR_BACK)
			{
				if (tag > 0)
					map_.getSectorsByTag(tag, tagged_sectors_);
				else if (back)
					tagged_sectors_.push_back(back);
			}

			// Thing ID
			else if (needs_tag == AS_TT_THING)
				map_.getThingsById(tag, tagged_things_);

			// Line ID
			else if (needs_tag == AS_TT_LINE)
				map_.getLinesById(tag, tagged_lines_);

			// ZDoom quirkiness
			else if (needs_tag)
			{
				switch (needs_tag)
				{
				case AS_TT_1THING_2SECTOR:
				case AS_TT_1THING_3SECTOR:
				case AS_TT_1SECTOR_2THING:
				{
					int thingtag = (needs_tag == AS_TT_1SECTOR_2THING) ? arg2 : tag;
					int sectag = (needs_tag == AS_TT_1SECTOR_2THING) ? tag :
								 (needs_tag == AS_TT_1THING_2SECTOR) ? arg2 : arg3;
					if ((thingtag | sectag) == 0)
						break;
					else if (thingtag == 0)
						map_.getSectorsByTag(sectag, tagged_sectors_);
					else if (sectag == 0)
						map_.getThingsById(thingtag, tagged_things_);
					else // neither thingtag nor sectag are 0
						map_.getThingsByIdInSectorTag(thingtag, sectag, tagged_things_);
				}	break;
				case AS_TT_1THING_2THING_3THING:
					if (arg3) map_.getThingsById(arg3, tagged_things_);
				case AS_TT_1THING_2THING:
					if (arg2) map_.getThingsById(arg2, tagged_things_);
				case AS_TT_1THING_4THING:
					if (tag ) map_.getThingsById(tag, tagged_things_);
				case AS_TT_4THING:
					if (needs_tag == AS_TT_1THING_4THING || needs_tag == AS_TT_4THING)
						if (arg4) map_.getThingsById(arg4, tagged_things_);
					break;
				case AS_TT_5THING:
					if (arg5) map_.getThingsById(arg5, tagged_things_);
					break;
				case AS_TT_LINE_NEGATIVE:
					if (tag ) map_.getLinesById(abs(tag), tagged_lines_);
					break;
				case AS_TT_1LINEID_2LINE:
					if (arg2) map_.getLinesById(arg2, tagged_lines_);
					break;
				case AS_TT_1LINE_2SECTOR:
					if (tag ) map_.getLinesById(tag, tagged_lines_);
					if (arg2) map_.getSectorsByTag(arg2, tagged_sectors_);
					break;
				case AS_TT_1SECTOR_2THING_3THING_5THING:
					if (arg5) map_.getThingsById(arg5, tagged_things_);
					if (arg3) map_.getThingsById(arg3, tagged_things_);
				case AS_TT_1SECTOR_2SECTOR_3SECTOR_4SECTOR:
					if (arg4) map_.getSectorsByTag(arg4, tagged_sectors_);
					if (arg3) map_.getSectorsByTag(arg3, tagged_sectors_);
				case AS_TT_1SECTOR_2SECTOR:
					if (arg2) map_.getSectorsByTag(arg2, tagged_sectors_);
					if (tag ) map_.getSectorsByTag(tag , tagged_sectors_);
					break;
				case AS_TT_SECTOR_2IS3_LINE:
					if (tag)
					{
						if (arg2 == 3) map_.getLinesById(tag, tagged_lines_);
						else map_.getSectorsByTag(tag, tagged_sectors_);
					}
					break;
				default:
					// This is to handle interpolation specials and patrol specials
					if (tid) map_.getThingsById(tid, tagged_things_, 0, needs_tag);
					
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

	if (canvas_)
		canvas_->animateSelectionChange(selection_);

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
 * Used for pasting.  Given an [origin] point and the current [mouse_pos],
 * snaps in such a way that the mouse is a number of grid units away from the
 * origin.
 *******************************************************************/
fpoint2_t MapEditContext::relativeSnapToGrid(fpoint2_t origin, fpoint2_t mouse_pos)
{
	fpoint2_t delta = mouse_pos - origin;
	delta.x = snapToGrid(delta.x, false);
	delta.y = snapToGrid(delta.y, false);
	return origin + delta;
}

/* MapEditContext::beginMode
 * Begins a move operation, starting from [mouse_pos]
 *******************************************************************/
bool MapEditContext::beginMove(fpoint2_t mouse_pos)
{
	// Check if we have any selection or hilight
	if (!selection_.hasHilightOrSelection())
		return false;

	// Begin move operation
	move_origin_ = mouse_pos;
	move_items_ = selection_.selectionOrHilight();

	// Get list of vertices being moved (if any)
	vector<MapVertex*> move_verts;
	if (edit_mode_ != Mode::Things)
	{
		// Vertices mode
		if (edit_mode_ == Mode::Vertices)
		{
			for (auto& item : move_items_)
				move_verts.push_back(map_.getVertex(item.index));
		}

		// Lines mode
		else if (edit_mode_ == Mode::Lines)
		{
			for (auto& item : move_items_)
			{
				// Duplicate vertices shouldn't matter here
				move_verts.push_back(map_.getLine(item.index)->v1());
				move_verts.push_back(map_.getLine(item.index)->v2());
			}
		}

		// Sectors mode
		else if (edit_mode_ == Mode::Sectors)
		{
			for (auto& item : move_items_)
				map_.getSector(item.index)->getVertices(move_verts);
		}
	}

	// Filter out map objects being moved
	if (edit_mode_ == Mode::Things)
	{
		// Filter moving things
		for (auto& item : move_items_)
			map_.getThing(item.index)->filter(true);
	}
	else
	{
		// Filter moving lines
		for (auto vertex : move_verts)
		{
			for (unsigned l = 0; l < vertex->nConnectedLines(); l++)
				vertex->connectedLine(l)->filter(true);
		}
	}

	return true;
}

/* MapEditContext::doMove
 * Updates the current move operation (moving from start to [move_pos])
 *******************************************************************/
void MapEditContext::doMove(fpoint2_t mouse_pos)
{
	// Special case: single vertex or thing
	if (move_items_.size() == 1 && (edit_mode_ == Mode::Vertices || edit_mode_ == Mode::Things))
	{
		// Get new position
		double nx = mouse_pos.x;
		double ny = mouse_pos.y;
		if (grid_snap_)
		{
			nx = snapToGrid(nx);
			ny = snapToGrid(ny);
		}

		// Update move vector
		if (edit_mode_ == Mode::Vertices)
			move_vec_.set(
				nx - map_.getVertex(move_items_[0].index)->xPos(),
				ny - map_.getVertex(move_items_[0].index)->yPos()
			);
		else if (edit_mode_ == Mode::Things)
			move_vec_.set(
				nx - map_.getThing(move_items_[0].index)->xPos(),
				ny - map_.getThing(move_items_[0].index)->yPos()
			);

		return;
	}

	// Get amount moved
	double dx = mouse_pos.x - move_origin_.x;
	double dy = mouse_pos.y - move_origin_.y;

	// Update move vector
	if (grid_snap_)
		move_vec_.set(snapToGrid(dx), snapToGrid(dy));
	else
		move_vec_.set(dx, dy);
}

/* MapEditContext::endMove
 * Ends a move operation and applies change if [accept] is true
 *******************************************************************/
void MapEditContext::endMove(bool accept)
{
	long move_time = App::runTimer();

	// Un-filter objects
	for (unsigned a = 0; a < map_.nLines(); a++)
		map_.getLine(a)->filter(false);
	for (unsigned a = 0; a < map_.nThings(); a++)
		map_.getThing(a)->filter(false);

	// Move depending on edit mode
	if (edit_mode_ == Mode::Things && accept)
	{
		// Move things
		beginUndoRecord("Move Things", true, false, false);
		for (auto& item : move_items_)
		{
			MapThing* t = map_.getThing(item.index);
			undo_manager_->recordUndoStep(new MapEditor::PropertyChangeUS(t));
			map_.moveThing(item.index, t->xPos() + move_vec_.x, t->yPos() + move_vec_.y);
		}
		endUndoRecord(true);
	}
	else if (accept)
	{
		// Any other edit mode we're technically moving vertices
		beginUndoRecord(S_FMT("Move %s", modeString()));

		// Get list of vertices being moved
		vector<uint8_t> move_verts(map_.nVertices());
		memset(&move_verts[0], 0, map_.nVertices());

		if (edit_mode_ == Mode::Vertices)
		{
			for (auto& item : move_items_)
				move_verts[item.index] = 1;
		}
		else if (edit_mode_ == Mode::Lines)
		{
			for (auto& item : move_items_)
			{
				MapLine* line = map_.getLine(item.index);
				if (line->v1()) move_verts[line->v1()->getIndex()] = 1;
				if (line->v2()) move_verts[line->v2()->getIndex()] = 1;
			}
		}
		else if (edit_mode_ == Mode::Sectors)
		{
			vector<MapVertex*> sv;
			for (auto& item : move_items_)
				map_.getSector(item.index)->getVertices(sv);

			for (auto vertex : sv)
				move_verts[vertex->getIndex()] = 1;
		}

		// Move vertices
		vector<MapVertex*> moved_verts;
		for (unsigned a = 0; a < map_.nVertices(); a++)
		{
			if (!move_verts[a])
				continue;
			fpoint2_t np(map_.getVertex(a)->xPos() + move_vec_.x, map_.getVertex(a)->yPos() + move_vec_.y);
			map_.moveVertex(a, np.x, np.y);
			moved_verts.push_back(map_.getVertex(a));
		}

		// Begin extra 'Merge' undo step if wanted
		if (map_merge_undo_step)
		{
			endUndoRecord(true);
			beginUndoRecord("Merge");
		}

		// Do merge
		bool merge = map_.mergeArch(moved_verts);

		endUndoRecord(merge || !map_merge_undo_step);
	}

	// Clear selection
	if (accept && selection_clear_move)
		selection_.clearSelection();

	// Clear moving items
	move_items_.clear();

	// Update map item indices
	map_.refreshIndices();
}

#if 0
/* MapEditContext::mergeLines
 * Currently unused
 *******************************************************************/
void MapEditContext::mergeLines(long move_time, vector<fpoint2_t>& merge_points)
{
	// Merge vertices and split lines
	for (unsigned a = 0; a < merge_points.size(); a++)
	{
		MapVertex* v = map_.mergeVerticesPoint(merge_points[a].x, merge_points[a].y);
		if (v) map_.splitLinesAt(v, 1);
	}

	// Split lines overlapping vertices
	for (unsigned a = 0; a < map_.nLines(); a++)
	{
		MapLine* line = map_.getLine(a);
		if (line->modifiedTime() >= move_time)
		{
			MapVertex* split = map_.lineCrossVertex(line->x1(), line->y1(), line->x2(), line->y2());
			if (split)
			{
				map_.splitLine(line, split);
				a = 0;
			}
		}
	}

	// Merge lines
	for (unsigned a = 0; a < map_.nLines(); a++)
	{
		if (map_.getLine(a)->modifiedTime() >= move_time)
		{
			if (map_.mergeLine(a) > 0 && a < map_.nLines())
			{
				map_.getLine(a)->clearUnneededTextures();
				a = 0;
			}
		}
	}

	// Remove any resulting zero-length lines
	map_.removeZeroLengthLines();

}
#endif

/* MapEditContext::splitLine
 * Splits the line closest to [x,y] at the closest point on the line
 *******************************************************************/
void MapEditContext::splitLine(double x, double y, double min_dist)
{
	fpoint2_t point(x, y);

	// Get the closest line
	int lindex = map_.nearestLine(point, min_dist);
	MapLine* line = map_.getLine(lindex);

	// Do nothing if no line is close enough
	if (!line)
		return;

	// Begin recording undo level
	beginUndoRecord("Split Line", true, true, false);

	// Get closest point on the line
	fpoint2_t closest = MathStuff::closestPointOnLine(point, line->seg());

	// Create vertex there
	MapVertex* vertex = map_.createVertex(closest.x, closest.y);

	// Do line split
	map_.splitLine(line, vertex);

	// Finish recording undo level
	endUndoRecord();
}

/* MapEditContext::flipLines
 * Flips all selected lines, and sides if [sides] is true
 *******************************************************************/
void MapEditContext::flipLines(bool sides)
{
	// Get selected/hilighted line(s)
	auto lines = selection_.selectedLines();
	if (lines.empty())
		return;

	// Go through list
	undo_manager_->beginRecord("Flip Line");
	for (unsigned a = 0; a < lines.size(); a++)
	{
		undo_manager_->recordUndoStep(new MapEditor::PropertyChangeUS(lines[a]));
		lines[a]->flip(sides);
	}
	undo_manager_->endRecord(true);

	// Update display
	updateDisplay();
}

/* MapEditContext::correctLineSectors
 * Attempts to correct sector references on all selected lines
 *******************************************************************/
void MapEditContext::correctLineSectors()
{
	// Get selected/hilighted line(s)
	auto lines = selection_.selectedLines();
	if (lines.empty())
		return;

	beginUndoRecord("Correct Line Sectors");

	bool changed = false;
	for (unsigned a = 0; a < lines.size(); a++)
	{
		if (map_.correctLineSectors(lines[a]))
			changed = true;
	}

	endUndoRecord(changed);

	// Update display
	if (changed)
	{
		addEditorMessage("Corrected Sector references");
		updateDisplay();
	}
}

/* MapEditContext::changeSectorHeight
 * Changes floor and/or ceiling heights on all selected sectors by
 * [amount]
 *******************************************************************/
void MapEditContext::changeSectorHeight(int amount, bool floor, bool ceiling)
{
	// Do nothing if not in sectors mode
	if (edit_mode_ != Mode::Sectors)
		return;

	// Get selected sectors (if any)
	auto selection = selection_.selectedSectors();
	if (selection.empty())
		return;

	// If we're modifying both heights, take sector_mode into account
	if (floor && ceiling)
	{
		if (sector_mode_ == SectorMode::Floor)
			ceiling = false;
		if (sector_mode_ == SectorMode::Ceiling)
			floor = false;
	}

	// Begin record undo level
	beginUndoRecordLocked("Change Sector Height", true, false, false);

	// Go through selection
	for (unsigned a = 0; a < selection.size(); a++)
	{
		// Change floor height
		if (floor)
		{
			int height = selection[a]->intProperty("heightfloor");
			selection[a]->setIntProperty("heightfloor", height + amount);
		}

		// Change ceiling height
		if (ceiling)
		{
			int height = selection[a]->intProperty("heightceiling");
			selection[a]->setIntProperty("heightceiling", height + amount);
		}
	}

	// End record undo level
	endUndoRecord();

	// Add editor message
	string what = "";
	if (floor && !ceiling)
		what = "Floor";
	else if (!floor && ceiling)
		what = "Ceiling";
	else
		what = "Floor and ceiling";
	string inc = "increased";
	if (amount < 0)
	{
		inc = "decreased";
		amount = -amount;
	}
	addEditorMessage(S_FMT("%s height %s by %d", what, inc, amount));

	// Update display
	updateDisplay();
}

/* MapEditContext::changeSectorLight
 * Changes the light level for all selected sectors, increments if
 * [up] is true, decrements otherwise
 *******************************************************************/
void MapEditContext::changeSectorLight(bool up, bool fine)
{
	// Do nothing if not in sectors mode
	if (edit_mode_ != Mode::Sectors)
		return;

	// Get selected sectors (if any)
	auto selection = selection_.selectedSectors();
	if (selection.empty())
		return;

	// Begin record undo level
	beginUndoRecordLocked("Change Sector Light", true, false, false);

	// Go through selection
	for (unsigned a = 0; a < selection.size(); a++)
	{
		// Get current light
		int light = selection[a]->intProperty("lightlevel");

		// Increment/decrement
		if (up)
			light = fine ? light+1 : theGameConfiguration->upLightLevel(light);
		else
			light = fine ? light-1 : theGameConfiguration->downLightLevel(light);

		// Change light level
		selection[a]->setIntProperty("lightlevel", light);
	}

	// End record undo level
	endUndoRecord();

	// Add editor message
	string dir = up ? "increased" : "decreased";
	int amount = fine ? 1 : theGameConfiguration->lightLevelInterval();
	addEditorMessage(S_FMT("Light level %s by %d", dir, amount));

	// Update display
	updateDisplay();
}

/* MapEditContext::changeSectorTexture
 * Depending on the current sector edit mode, either opens the
 * sector texture overlay (normal) or browses for the ceiling or
 * floor texture (ceiling/floor edit mode)
 *******************************************************************/
void MapEditContext::changeSectorTexture()
{
	// Determine the initial texture
	string texture = "";
	string browser_title;
	string undo_name;

	// Get selected sectors
	auto selection = selection_.selectedSectors();
	if (selection.empty())
		return;

	// Check edit mode
	if (sector_mode_ == SectorMode::Floor)
	{
		texture = selection[0]->stringProperty("texturefloor");
		browser_title = "Browse Floor Texture";
		undo_name = "Change Floor Texture";
	}
	else if (sector_mode_ == SectorMode::Ceiling)
	{
		texture = selection[0]->stringProperty("textureceiling");
		browser_title = "Browse Ceiling Texture";
		undo_name = "Change Ceiling Texture";
	}
	else
	{
		canvas_->openSectorTextureOverlay(selection);
		return;
	}

	// Lock hilight
	bool hl_lock = selection_.hilightLocked();
	selection_.lockHilight();

	// Open texture browser
	string selected_tex = MapEditor::browseTexture(texture, 1, map_, browser_title);
	if (!selected_tex.empty())
	{
		// Set texture depending on edit mode
		beginUndoRecord(undo_name, true, false, false);
		for (unsigned a = 0; a < selection.size(); a++)
		{
			if (sector_mode_ == SectorMode::Floor)
				selection[a]->setStringProperty("texturefloor", selected_tex);
			else if (sector_mode_ == SectorMode::Ceiling)
				selection[a]->setStringProperty("textureceiling", selected_tex);
		}
		endUndoRecord();
	}

	// Unlock hilight if needed
	selection_.lockHilight(hl_lock);
	//canvas_->renderer2D()->clearTextureCache(); // TODO: This
}

/* MapEditContext::joinSectors
 * Joins all selected sectors. If [remove_lines] is true, all
 * resulting lines with both sides set to the joined sector are
 * removed
 *******************************************************************/
void MapEditContext::joinSectors(bool remove_lines)
{
	// Check edit mode
	if (edit_mode_ != Mode::Sectors)
		return;

	// Get sectors to merge
	auto sectors = selection_.selectedSectors(false);
	if (sectors.size() < 2) // Need at least 2 sectors to join
		return;

	// Get 'target' sector
	auto target = sectors[0];

	// Clear selection
	selection_.clearSelection();

	// Init list of lines
	vector<MapLine*> lines;

	// Begin recording undo level
	beginUndoRecord("Join/Merge Sectors", true, false, true);

	// Go through merge sectors
	for (unsigned a = 1; a < sectors.size(); a++)
	{
		// Go through sector sides
		auto sector = sectors[a];
		while (sector->connectedSides().size() > 0)
		{
			// Set sector
			auto side = sector->connectedSides()[0];
			side->setSector(target);

			// Add line to list if not already there
			bool exists = false;
			for (unsigned l = 0; l < lines.size(); l++)
			{
				if (side->getParentLine() == lines[l])
				{
					exists = true;
					break;
				}
			}
			if (!exists)
				lines.push_back(side->getParentLine());
		}

		// Delete sector
		map_.removeSector(sector);
	}

	// Remove any changed lines that now have the target sector on both sides (if needed)
	int nlines = 0;
	vector<MapVertex*> verts;
	if (remove_lines)
	{
		for (unsigned a = 0; a < lines.size(); a++)
		{
			if (lines[a]->frontSector() == target && lines[a]->backSector() == target)
			{
				VECTOR_ADD_UNIQUE(verts, lines[a]->v1());
				VECTOR_ADD_UNIQUE(verts, lines[a]->v2());
				map_.removeLine(lines[a]);
				nlines++;
			}
		}
	}

	// Remove any resulting detached vertices
	for (unsigned a = 0; a < verts.size(); a++)
	{
		if (verts[a]->nConnectedLines() == 0)
			map_.removeVertex(verts[a]);
	}

	// Finish recording undo level
	endUndoRecord();

	// Editor message
	if (nlines == 0)
		addEditorMessage(S_FMT("Joined %lu Sectors", sectors.size()));
	else
		addEditorMessage(S_FMT("Joined %lu Sectors (removed %d Lines)", sectors.size(), nlines));
}

/* MapEditContext::changeThingType
 * Opens the thing type browser for the currently selected thing(s)
 *******************************************************************/
void MapEditContext::changeThingType()
{
	// Get selected things (if any)
	auto selection = selection_.selectedThings();

	// Do nothing if no selection or hilight
	if (selection.size() == 0)
		return;

	// Browse thing type
	int newtype = MapEditor::browseThingType(selection[0]->getType(), map_);
	if (newtype >= 0)
	{
		// Go through selection
		beginUndoRecord("Thing Type Change", true, false, false);
		for (unsigned a = 0; a < selection.size(); a++)
			selection[a]->setIntProperty("type", newtype);
		endUndoRecord(true);

		// Add editor message
		string type_name = theGameConfiguration->thingType(newtype)->getName();
		if (selection.size() == 1)
			addEditorMessage(S_FMT("Changed type to \"%s\"", type_name));
		else
			addEditorMessage(S_FMT("Changed %lu things to type \"%s\"", selection.size(), type_name));

		// Update display
		updateDisplay();
	}
}

/* MapEditContext::thingQuickAngle
 * Sets the angle of all selected things to face toward [mouse_pos]
 *******************************************************************/
void MapEditContext::thingQuickAngle(fpoint2_t mouse_pos)
{
	// Do nothing if not in things mode
	if (edit_mode_ != Mode::Things)
		return;

	beginUndoRecord("Thing Quick Angle Change", true, false, false);
	for (auto thing : selection_.selectedThings())
		thing->setAnglePoint(mouse_pos);
	endUndoRecord(true);
}

/* MapEditContext::mirror
 * Mirror selected objects horizontally or vertically depending on
 * [x_axis]
 *******************************************************************/
void MapEditContext::mirror(bool x_axis)
{
	// Mirror things
	if (edit_mode_ == Mode::Things)
	{
		// Begin undo level
		beginUndoRecord("Mirror Things", true, false, false);

		// Get things to mirror
		auto things = selection_.selectedThings();

		// Get midpoint
		bbox_t bbox;
		for (unsigned a = 0; a < things.size(); a++)
			bbox.extend(things[a]->xPos(), things[a]->yPos());

		// Mirror
		for (unsigned a = 0; a < things.size(); a++)
		{
			// Position
			if (x_axis)
				map_.moveThing(things[a]->getIndex(), bbox.mid_x() - (things[a]->xPos() - bbox.mid_x()), things[a]->yPos());
			else
				map_.moveThing(things[a]->getIndex(), things[a]->xPos(), bbox.mid_y() - (things[a]->yPos() - bbox.mid_y()));

			// Direction
			int angle = things[a]->getAngle();
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
			things[a]->setIntProperty("angle", angle);
		}
		endUndoRecord(true);
	}

	// Mirror map architecture
	else if (edit_mode_ != Mode::Visual)
	{
		// Begin undo level
		beginUndoRecord("Mirror Map Architecture", true, false, false);

		// Get vertices to mirror
		vector<MapVertex*> vertices;
		vector<MapLine*> lines;
		if (edit_mode_ == Mode::Vertices)
			vertices = selection_.selectedVertices();
		else if (edit_mode_ == Mode::Lines)
		{
			auto sel = selection_.selectedLines();
			for (auto line : sel)
			{
				VECTOR_ADD_UNIQUE(vertices, line->v1());
				VECTOR_ADD_UNIQUE(vertices, line->v2());
				lines.push_back(line);
			}
		}
		else if (edit_mode_ == Mode::Sectors)
		{
			auto sectors = selection_.selectedSectors();
			for (auto sector : sectors)
			{
				sector->getVertices(vertices);
				sector->getLines(lines);
			}
		}

		// Get midpoint
		bbox_t bbox;
		for (unsigned a = 0; a < vertices.size(); a++)
			bbox.extend(vertices[a]->xPos(), vertices[a]->yPos());

		// Mirror vertices
		for (unsigned a = 0; a < vertices.size(); a++)
		{
			// Position
			if (x_axis)
				map_.moveVertex(vertices[a]->getIndex(), bbox.mid_x() - (vertices[a]->xPos() - bbox.mid_x()), vertices[a]->yPos());
			else
				map_.moveVertex(vertices[a]->getIndex(), vertices[a]->xPos(), bbox.mid_y() - (vertices[a]->yPos() - bbox.mid_y()));
		}

		// Flip lines (just swap vertices)
		for (unsigned a = 0; a < lines.size(); a++)
			lines[a]->flip(false);

		endUndoRecord(true);
	}
}

/* MapEditContext::editObjectProperties
 * Opens a dialog containing a MapObjectPropsPanel to edit properties
 * for all selected (or hilighted) objects
 *******************************************************************/
void MapEditContext::editObjectProperties()
{
	auto selection = selection_.selectedObjects();
	if (selection.empty())
		return;

	// Begin recording undo level
	beginUndoRecord(S_FMT("Property Edit (%s)", modeString(false)));
	for (auto item : selection)
		recordPropertyChangeUndoStep(item);

	bool done = MapEditor::editObjectProperties(selection);
	if (done)
	{
		//renderer_2d->forceUpdate(fade_lines); // TODO: This once the 2d renderer is moved here
		updateDisplay();

		if (edit_mode_ == Mode::Things)
			copyProperties(selection[0]);
	}

	// End undo level
	endUndoRecord(done);
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
}

/* MapEditContext::createObject
 * Creates an object (depending on edit mode) at [x,y]
 *******************************************************************/
void MapEditContext::createObject(double x, double y)
{
	// Vertices mode
	if (edit_mode_ == Mode::Vertices)
	{
		// If there are less than 2 vertices currently selected, just create a vertex at x,y
		if (selection_.size() < 2)
			createVertex(x, y);
		else
		{
			// Otherwise, create lines between selected vertices
			beginUndoRecord("Create Lines", false, true, false);
			auto vertices = selection_.selectedVertices(false);
			for (unsigned a = 0; a < vertices.size() - 1; a++)
				map_.createLine(vertices[a], vertices[a+1]);
			endUndoRecord(true);

			// Editor message
			addEditorMessage(S_FMT("Created %lu line(s)", selection_.size() - 1));

			// Clear selection
			selection_.clearSelection();
		}

		return;
	}

	// Sectors mode
	if (edit_mode_ == Mode::Sectors)
	{
		// Sector
		if (map_.nLines() > 0)
		{
			createSector(x, y);
		}
		else
		{
			// Just create a vertex
			createVertex(x, y);
			setEditMode(Mode::Lines);
		}
		return;
	}

	// Things mode
	if (edit_mode_ == Mode::Things)
	{
		createThing(x, y);
		return;
	}
}

/* MapEditContext::createVertex
 * Creates a new vertex at [x,y]
 *******************************************************************/
void MapEditContext::createVertex(double x, double y)
{
	// Snap coordinates to grid if necessary
	if (grid_snap_)
	{
		x = snapToGrid(x);
		y = snapToGrid(y);
	}

	// Create vertex
	beginUndoRecord("Create Vertex", true, true, false);
	MapVertex* vertex = map_.createVertex(x, y, 2);
	endUndoRecord(true);

	// Editor message
	if (vertex)
		addEditorMessage(S_FMT("Created vertex at (%d, %d)", (int)vertex->xPos(), (int)vertex->yPos()));
}

/* MapEditContext::createThing
 * Creates a new thing at [x,y]
 *******************************************************************/
void MapEditContext::createThing(double x, double y)
{
	// Snap coordinates to grid if necessary
	if (grid_snap_)
	{
		x = snapToGrid(x);
		y = snapToGrid(y);
	}

	// Begin undo step
	beginUndoRecord("Create Thing", false, true, false);

	// Create thing
	MapThing* thing = map_.createThing(x, y);

	// Setup properties
	theGameConfiguration->applyDefaults(thing, map_.currentFormat() == MAP_UDMF);
	if (copy_thing_ && thing)
	{
		// Copy type and angle from the last copied thing
		thing->setIntProperty("type", copy_thing_->getType());
		thing->setIntProperty("angle", copy_thing_->getAngle());
	}

	// End undo step
	endUndoRecord(true);

	// Editor message
	if (thing)
		addEditorMessage(S_FMT("Created thing at (%d, %d)", (int)thing->xPos(), (int)thing->yPos()));
}

/* MapEditContext::createSector
 * Creates a new sector at [x,y]
 *******************************************************************/
void MapEditContext::createSector(double x, double y)
{
	fpoint2_t point(x, y);

	// Find nearest line
	int nearest = map_.nearestLine(point, 99999999);
	MapLine* line = map_.getLine(nearest);
	if (!line)
		return;

	// Determine side
	double side = MathStuff::lineSide(point, line->seg());

	// Get sector to copy if we're in sectors mode
	MapSector* sector_copy = nullptr;
	if (edit_mode_ == Mode::Sectors && selection_.size() > 0)
		sector_copy = map_.getSector(selection_.begin()->index);

	// Run sector builder
	SectorBuilder builder;
	bool ok;
	if (side >= 0)
		ok = builder.traceSector(&map_, line, true);
	else
		ok = builder.traceSector(&map_, line, false);

	// Do nothing if sector was already valid
	if (builder.isValidSector())
		return;

	// Create sector from builder result if needed
	if (ok)
	{
		beginUndoRecord("Create Sector", true, true, false);
		builder.createSector(nullptr, sector_copy);

		// Flash
		if (canvas_)
			canvas_->animateSelectionChange({ (int)map_.nSectors() - 1, MapEditor::ItemType::Sector });
	}

	// Set some sector defaults from game configuration if needed
	if (!sector_copy && ok)
	{
		MapSector* n_sector = map_.getSector(map_.nSectors()-1);
		if (n_sector->getCeilingTex().IsEmpty())
			theGameConfiguration->applyDefaults(n_sector, map_.currentFormat() == MAP_UDMF);
	}

	// Editor message
	if (ok)
	{
		addEditorMessage(S_FMT("Created sector #%lu", map_.nSectors() - 1));
		endUndoRecord(true);
	}
	else
		addEditorMessage("Sector creation failed: " + builder.getError());
}

/* MapEditContext::deleteObject
 * Deletes all selected objects, depending on edit mode
 *******************************************************************/
void MapEditContext::deleteObject()
{
	vector<MapObject*> objects_prop_change;

	// Vertices mode
	if (edit_mode_ == Mode::Vertices)
	{
		// Get selected vertices
		auto verts = selection_.selectedVertices();
		int index = -1;
		if (verts.size() == 1)
			index = verts[0]->getIndex();

		// Begin undo step
		beginUndoRecord("Delete Vertices", map_merge_lines_on_delete_vertex, false, true);

		// Delete them (if any)
		for (unsigned a = 0; a < verts.size(); a++)
			map_.removeVertex(verts[a], map_merge_lines_on_delete_vertex);

		// Remove detached vertices
		map_.removeDetachedVertices();

		// Editor message
		if (verts.size() == 1)
			addEditorMessage(S_FMT("Deleted vertex #%d", index));
		else if (verts.size() > 1)
			addEditorMessage(S_FMT("Deleted %lu vertices", verts.size()));
	}

	// Lines mode
	else if (edit_mode_ == Mode::Lines)
	{
		// Get selected lines
		auto lines = selection_.selectedLines();
		int index = -1;
		if (lines.size() == 1)
			index = lines[0]->getIndex();

		// Begin undo step
		beginUndoRecord("Delete Lines", false, false, true);

		// Delete them (if any)
		for (unsigned a = 0; a < lines.size(); a++)
			map_.removeLine(lines[a]);

		// Remove detached vertices
		map_.removeDetachedVertices();

		// Editor message
		if (lines.size() == 1)
			addEditorMessage(S_FMT("Deleted line #%d", index));
		else if (lines.size() > 1)
			addEditorMessage(S_FMT("Deleted %lu lines", lines.size()));
	}

	// Sectors mode
	else if (edit_mode_ == Mode::Sectors)
	{
		// Get selected sectors
		auto sectors = selection_.selectedSectors();
		int index = -1;
		if (sectors.size() == 1)
			index = sectors[0]->getIndex();

		// Begin undo step
		beginUndoRecord("Delete Sectors", true, false, true);

		// Delete them (if any), and keep lists of connected lines and sides
		vector<MapSide*> connected_sides;
		vector<MapLine*> connected_lines;
		for (unsigned a = 0; a < sectors.size(); a++)
		{
			for (unsigned s = 0; s < sectors[a]->connectedSides().size(); s++)
				connected_sides.push_back(sectors[a]->connectedSides()[s]);
			sectors[a]->getLines(connected_lines);

			map_.removeSector(sectors[a]);
		}

		// Remove all connected sides
		for (unsigned a = 0; a < connected_sides.size(); a++)
		{
			// Before removing the side, check if we should flip the line
			MapLine* line = connected_sides[a]->getParentLine();
			if (connected_sides[a] == line->s1() && line->s2())
				line->flip();

			map_.removeSide(connected_sides[a]);
		}

		// Remove resulting invalid lines
		if (map_remove_invalid_lines)
		{
			for (unsigned a = 0; a < connected_lines.size(); a++)
			{
				if (!connected_lines[a]->s1() && !connected_lines[a]->s2())
					map_.removeLine(connected_lines[a]);
			}
		}

		// Try to fill in textures on any lines that just became one-sided
		for (unsigned a = 0; a < connected_lines.size(); a++)
		{
			MapLine* line = connected_lines[a];
			MapSide* side;
			if (line->s1() && !line->s2())
				side = line->s1();
			else if (!line->s1() && line->s2())
				side = line->s2();
			else
				continue;

			if (side->getTexMiddle() != "-")
				continue;

			// Inherit textures from upper or lower
			if (side->getTexUpper() != "-")
				side->setStringProperty("texturemiddle", side->getTexUpper());
			else if (side->getTexLower() != "-")
				side->setStringProperty("texturemiddle", side->getTexLower());

			// Clear any existing textures, which are no longer visible
			side->setStringProperty("texturetop", "-");
			side->setStringProperty("texturebottom", "-");
		}

		// Editor message
		if (sectors.size() == 1)
			addEditorMessage(S_FMT("Deleted sector #%d", index));
		else if (sectors.size() > 1)
			addEditorMessage(S_FMT("Deleted %lu sector", sectors.size()));

		// Remove detached vertices
		map_.removeDetachedVertices();
	}

	// Things mode
	else if (edit_mode_ == Mode::Things)
	{
		// Get selected things
		auto things = selection_.selectedThings();
		int index = -1;
		if (things.size() == 1)
			index = things[0]->getIndex();

		// Begin undo step
		beginUndoRecord("Delete Things", false, false, true);

		// Delete them (if any)
		for (unsigned a = 0; a < things.size(); a++)
			map_.removeThing(things[a]);

		// Editor message
		if (things.size() == 1)
			addEditorMessage(S_FMT("Deleted thing #%d", index));
		else if (things.size() > 1)
			addEditorMessage(S_FMT("Deleted %lu things", things.size()));
	}

	// Record undo step
	endUndoRecord(true);

	// Clear hilight and selection
	selection_.clearSelection();
	selection_.clearHilight();
}

/* MapEditContext::beginObjectEdit
 * Begins an object edit operation
 *******************************************************************/
bool MapEditContext::beginObjectEdit()
{
	// Things mode
	if (edit_mode_ == Mode::Things)
	{
		// Get selected things
		auto edit_objects = selection_.selectedObjects();

		// Setup object group
		edit_object_group_.clear();
		for (unsigned a = 0; a < edit_objects.size(); a++)
			edit_object_group_.addThing((MapThing*)edit_objects[a]);

		// Filter objects
		edit_object_group_.filterObjects(true);
	}
	else
	{
		vector<MapObject*> edit_objects;

		// Vertices mode
		if (edit_mode_ == Mode::Vertices)
		{
			// Get selected vertices
			edit_objects = selection_.selectedObjects();
		}

		// Lines mode
		else if (edit_mode_ == Mode::Lines)
		{
			// Get vertices of selected lines
			auto lines = selection_.selectedLines();
			for (unsigned a = 0; a < lines.size(); a++)
			{
				VECTOR_ADD_UNIQUE(edit_objects, lines[a]->v1());
				VECTOR_ADD_UNIQUE(edit_objects, lines[a]->v2());
			}
		}

		// Sectors mode
		else if (edit_mode_ == Mode::Sectors)
		{
			// Get vertices of selected sectors
			auto sectors = selection_.selectedSectors();
			for (unsigned a = 0; a < sectors.size(); a++)
				sectors[a]->getVertices(edit_objects);
		}

		// Setup object group
		edit_object_group_.clear();
		for (unsigned a = 0; a < edit_objects.size(); a++)
			edit_object_group_.addVertex((MapVertex*)edit_objects[a]);
		edit_object_group_.addConnectedLines();

		// Filter objects
		edit_object_group_.filterObjects(true);
	}

	if (edit_object_group_.empty())
		return false;

	MapEditor::showObjectEditPanel(true, &edit_object_group_);

	return true;
}

/* MapEditContext::endObjectEdit
 * Ends the object edit operation and applies changes if [accept] is
 * true
 *******************************************************************/
void MapEditContext::endObjectEdit(bool accept)
{
	// Un-filter objects
	edit_object_group_.filterObjects(false);

	// Apply change if accepted
	if (accept)
	{
		// Begin recording undo level
		beginUndoRecord(S_FMT("Edit %s", modeString()));

		// Apply changes
		edit_object_group_.applyEdit();

		// Do merge
		bool merge = true;
		if (edit_mode_ != Mode::Things)
		{
			// Begin extra 'Merge' undo step if wanted
			if (map_merge_undo_step)
			{
				endUndoRecord(true);
				beginUndoRecord("Merge");
			}

			vector<MapVertex*> vertices;
			edit_object_group_.getVertices(vertices);
			merge = map_.mergeArch(vertices);
		}

		// Clear selection
		selection_.clearSelection();

		endUndoRecord(merge || !map_merge_undo_step);
	}

	MapEditor::showObjectEditPanel(false, nullptr);
}

/* MapEditContext::copyProperties
 * Copies the properties from [object] to be used for paste/create
 *******************************************************************/
void MapEditContext::copyProperties(MapObject* object)
{
	// Do nothing if no selection or hilight
	if (!selection_.hasHilightOrSelection())
		return;

	// Sectors mode
	if (edit_mode_ == Mode::Sectors)
	{
		// Create copy sector if needed
		if (!copy_sector_)
			copy_sector_ = new MapSector(nullptr);

		// Copy selection/hilight properties
		if (selection_.size() > 0)
			copy_sector_->copy(map_.getSector(selection_[0].index));
		else if (selection_.hasHilight())
			copy_sector_->copy(selection_.hilightedSector());

		// Editor message
		if (!object)
			addEditorMessage("Copied sector properties");
	}

	// Things mode
	else if (edit_mode_ == Mode::Things)
	{
		// Create copy thing if needed
		if (!copy_thing_)
			copy_thing_ = new MapThing(nullptr);

		// Copy given object properties (if any)
		if (object && object->getObjType() == MOBJ_THING)
			copy_thing_->copy(object);
		else
		{
			// Otherwise copy selection/hilight properties
			if (selection_.size() > 0)
				copy_thing_->copy(map_.getThing(selection_[0].index));
			else if (selection_.hasHilight())
				copy_thing_->copy(selection_.hilightedThing());
			else
				return;
		}

		// Editor message
		if (!object)
			addEditorMessage("Copied thing properties");
	}

	else if (edit_mode_ == Mode::Lines)
	{
		if (!copy_line_)
			copy_line_ = new MapLine(nullptr, nullptr, new MapSide(nullptr, nullptr), new MapSide(nullptr, nullptr), nullptr);

		if (selection_.size() > 0)
			copy_line_->copy(map_.getLine(selection_[0].index));
		else if (selection_.hasHilight())
			copy_line_->copy(selection_.hilightedLine());

		if(!object)
			addEditorMessage("Copied line properties");
	}
}

/* MapEditContext::pasteProperties
 * Pastes previously copied properties to all selected objects
 *******************************************************************/
void MapEditContext::pasteProperties()
{
	// Do nothing if no selection or hilight
	if (!selection_.hasHilightOrSelection())
		return;

	// Sectors mode
	if (edit_mode_ == Mode::Sectors)
	{
		// Do nothing if no properties have been copied
		if (!copy_sector_)
			return;

		// Paste properties to selection/hilight
		beginUndoRecord("Paste Sector Properties", true, false, false);
		for (auto sector : selection_.selectedSectors())
			sector->copy(copy_sector_);
		endUndoRecord();

		// Editor message
		addEditorMessage("Pasted sector properties");
	}

	// Things mode
	if (edit_mode_ == Mode::Things)
	{
		// Do nothing if no properties have been copied
		if (!copy_thing_)
			return;

		// Paste properties to selection/hilight
		beginUndoRecord("Paste Thing Properties", true, false, false);
		for (auto thing : selection_.selectedThings())
		{
			// Paste properties (but keep position)
			double x = thing->xPos();
			double y = thing->yPos();
			thing->copy(copy_thing_);
			thing->setFloatProperty("x", x);
			thing->setFloatProperty("y", y);
		}
		endUndoRecord();

		// Editor message
		addEditorMessage("Pasted thing properties");
	}

	// Lines mode
	else if (edit_mode_ == Mode::Lines)
	{
		// Do nothing if no properties have been copied
		if (!copy_line_)
			return;

		// Paste properties to selection/hilight
		beginUndoRecord("Paste Line Properties", true, false, false);
		for (auto line : selection_.selectedLines())
			line->copy(copy_line_);
		endUndoRecord();

		// Editor message
		addEditorMessage("Pasted line properties");
	}

	// Update display
	updateDisplay();
}

/* MapEditContext::copy
 * Copies all selected objects
 *******************************************************************/
void MapEditContext::copy()
{
	// Can't copy/paste vertices (no point)
	if (edit_mode_ == Mode::Vertices)
	{
		//addEditorMessage("Copy/Paste not supported for vertices");
		return;
	}

	// Clear current clipboard contents
	theClipboard->clear();

	// Copy lines
	if (edit_mode_ == Mode::Lines || edit_mode_ == Mode::Sectors)
	{
		// Get selected lines
		auto lines = selection_.selectedLines();

		// Add to clipboard
		auto c = new MapArchClipboardItem();
		c->addLines(lines);
		theClipboard->addItem(c);

		// Editor message
		addEditorMessage(S_FMT("Copied %s", c->getInfo()));
	}

	// Copy things
	else if (edit_mode_ == Mode::Things)
	{
		// Get selected things
		auto things = selection_.selectedThings();

		// Add to clipboard
		auto c = new MapThingsClipboardItem();
		c->addThings(things);
		theClipboard->addItem(c);

		// Editor message
		addEditorMessage(S_FMT("Copied %s", c->getInfo()));
	}
}

/* MapEditContext::paste
 * Pastes previously copied objects at [mouse_pos]
 *******************************************************************/
void MapEditContext::paste(fpoint2_t mouse_pos)
{
	// Go through clipboard items
	for (unsigned a = 0; a < theClipboard->nItems(); a++)
	{
		// Map architecture
		if (theClipboard->getItem(a)->getType() == CLIPBOARD_MAP_ARCH)
		{
			beginUndoRecord("Paste Map Architecture");
			auto p = (MapArchClipboardItem*)theClipboard->getItem(a);
			// Snap the geometry in such a way that it stays in the same
			// position relative to the grid
			fpoint2_t pos = relativeSnapToGrid(p->getMidpoint(), mouse_pos);
			auto new_verts = p->pasteToMap(&map_, pos);
			map_.mergeArch(new_verts);
			addEditorMessage(S_FMT("Pasted %s", p->getInfo()));
			endUndoRecord(true);
		}

		// Things
		else if (theClipboard->getItem(a)->getType() == CLIPBOARD_MAP_THINGS)
		{
			beginUndoRecord("Paste Things", false, true, false);
			auto p = (MapThingsClipboardItem*)theClipboard->getItem(a);
			// Snap the geometry in such a way that it stays in the same
			// position relative to the grid
			fpoint2_t pos = relativeSnapToGrid(p->getMidpoint(), mouse_pos);
			p->pasteToMap(&map_, pos);
			addEditorMessage(S_FMT("Pasted %s", p->getInfo()));
			endUndoRecord(true);
		}
	}
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
			selection_.clearSelection();
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
			copy();

		else
			handled = false;

		if (handled)
			return handled;
	}

	// --- Sector mode keybinds ---
	if (key.StartsWith("me2d_sector") && edit_mode_ == Mode::Sectors)
	{
		// Height changes
		if		(key == "me2d_sector_floor_up8")	changeSectorHeight(8, true, false);
		else if (key == "me2d_sector_floor_up")		changeSectorHeight(1, true, false);
		else if (key == "me2d_sector_floor_down8")	changeSectorHeight(-8, true, false);
		else if (key == "me2d_sector_floor_down")	changeSectorHeight(-1, true, false);
		else if (key == "me2d_sector_ceil_up8")		changeSectorHeight(8, false, true);
		else if (key == "me2d_sector_ceil_up")		changeSectorHeight(1, false, true);
		else if (key == "me2d_sector_ceil_down8")	changeSectorHeight(-8, false, true);
		else if (key == "me2d_sector_ceil_down")	changeSectorHeight(-1, false, true);
		else if (key == "me2d_sector_height_up8")	changeSectorHeight(8, true, true);
		else if (key == "me2d_sector_height_up")	changeSectorHeight(1, true, true);
		else if (key == "me2d_sector_height_down8")	changeSectorHeight(-8, true, true);
		else if (key == "me2d_sector_height_down")	changeSectorHeight(-1, true, true);

		// Light changes
		else if (key == "me2d_sector_light_up16")	changeSectorLight(true, false);
		else if (key == "me2d_sector_light_up")		changeSectorLight(true, true);
		else if (key == "me2d_sector_light_down16")	changeSectorLight(false, false);
		else if (key == "me2d_sector_light_down")	changeSectorLight(false, true);

		// Join
		else if (key == "me2d_sector_join")			joinSectors(true);
		else if (key == "me2d_sector_join_keep")	joinSectors(false);

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
			selection_.clearSelection();
			addEditorMessage("Selection cleared");
		}

		// Toggle linked light levels
		else if (key == "me3d_light_toggle_link")
		{
			if (!is_udmf || !theGameConfiguration->udmfFlatLighting())
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
			if (!is_udmf || !theGameConfiguration->udmfTextureOffsets())
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
		canvas_->updateInfoOverlay();
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

	MapEditor::windowWx()->CallAfter(&MapEditorWindow::SetStatusText, mode, 1);

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

	MapEditor::windowWx()->CallAfter(&MapEditorWindow::SetStatusText, grid, 2);
}

/* MapEditContext::beginUndoRecord
 * Begins recording undo level [name]. [mod] is true if the operation
 * about to begin will modify object properties, [create/del] are
 * true if it will create or delete objects
 *******************************************************************/
void MapEditContext::beginUndoRecord(string name, bool mod, bool create, bool del)
{
	// Setup
	UndoManager* manager = (edit_mode_ == Mode::Visual) ? edit_3d_.undoManager() : undo_manager_;
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
	UndoManager* manager = (edit_mode_ == Mode::Visual) ? edit_3d_.undoManager() : undo_manager_;

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
	UndoManager* manager = (edit_mode_ == Mode::Visual) ? edit_3d_.undoManager() : undo_manager_;
	manager->recordUndoStep(new MapEditor::PropertyChangeUS(object));
}

/* MapEditContext::doUndo
 * Undoes the current undo level
 *******************************************************************/
void MapEditContext::doUndo()
{
	// Clear selection first, since part of it may become invalid
	selection_.clearSelection();

	// Undo
	int time = App::runTimer() - 1;
	UndoManager* manager = (edit_mode_ == Mode::Visual) ? edit_3d_.undoManager() : undo_manager_;
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
	selection_.clearSelection();

	// Redo
	int time = App::runTimer() - 1;
	UndoManager* manager = (edit_mode_ == Mode::Visual) ? edit_3d_.undoManager() : undo_manager_;
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

	fpoint2_t campos = canvas_->get3dCameraPos();
	pstart->setPos(campos.x, campos.y);
	pstart->setAnglePoint(campos + canvas_->get3dCameraDir());
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
