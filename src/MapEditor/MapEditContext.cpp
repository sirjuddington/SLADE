
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapEditContext.cpp
// Description: MapEditContext class - handles the map editing
//              context for a map (selection, highlight, undo/redo,
//              editing functions, etc.)
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
#include "MapEditContext.h"
#include "App.h"
#include "Game/Configuration.h"
#include "General/Clipboard.h"
#include "General/Console.h"
#include "General/UndoRedo.h"
#include "MapChecks.h"
#include "MapEditor/Renderer/Overlays/InfoOverlay3d.h"
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
#include "Utility/StringUtils.h"

using namespace slade;

using mapeditor::Input;
using mapeditor::Mode;
using mapeditor::SectorMode;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
double grid_sizes[] = { 0.05, 0.1, 0.25, 0.5,  1,    2,    4,    8,     16,    32,   64,
						128,  256, 512,  1024, 2048, 4096, 8192, 16384, 32768, 65536 };
}
CVAR(Bool, info_overlay_3d, true, CVar::Flag::Save)
CVAR(Bool, hilight_smooth, true, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Int, flat_drawtype)
EXTERN_CVAR(Bool, thing_preview_lights)


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Template function to find something in an associative map.
// M::mapped_type should be default constructible, or just provide
// a value for the third argument (the default value if not found).
// This really only works for value types right now, like maps to pointers.
// -----------------------------------------------------------------------------
template<typename M>
#if defined(_MSC_VER) && (_MSC_VER < 1500)
// MSVC++ 2005 will give error C2899 if attempting to compile "typename M::mapped_type()".
typename M::mapped_type findInMap(M& m, const typename M::key_type& k, typename M::mapped_type def = M::mapped_type())
{
#else
// On the other hand, other compilers will fail if this typename isn't there.
typename M::mapped_type findInMap(
	M&                          m,
	const typename M::key_type& k,
	typename M::mapped_type     def = typename M::mapped_type())
{
#endif
	typename M::iterator i = m.find(k);
	if (i == m.end())
	{
		return const_cast<typename M::mapped_type&>(def);
	}
	else
	{
		return i->second;
	}
}


// -----------------------------------------------------------------------------
//
// MapEditContext Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapEditContext class constructor
// -----------------------------------------------------------------------------
MapEditContext::MapEditContext()
{
	undo_manager_ = std::make_unique<UndoManager>(&map_);
}

// -----------------------------------------------------------------------------
// Changes the current edit mode to [mode]
// -----------------------------------------------------------------------------
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

	// Clear 3d mode undo manager etc on exiting 3d mode
	if (edit_mode_ == Mode::Visual && mode != Mode::Visual)
	{
		info_3d_.reset();
		undo_manager_->createMergedLevel(edit_3d_.undoManager(), "3D Mode Editing");
		edit_3d_.undoManager()->clear();
	}

	// Set undo manager for history panel
	if (mode == Mode::Visual && edit_mode_ != Mode::Visual)
		mapeditor::setUndoManager(edit_3d_.undoManager());
	else if (edit_mode_ == Mode::Visual && mode != Mode::Visual)
		mapeditor::setUndoManager(undo_manager_.get());

	edit_mode_prev_ = edit_mode_;

	// Set edit mode
	edit_mode_   = mode;
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
	case Mode::Lines: addEditorMessage("Lines mode"); break;
	case Mode::Sectors: addEditorMessage("Sectors mode (Normal)"); break;
	case Mode::Things: addEditorMessage("Things mode"); break;
	case Mode::Visual: addEditorMessage("3d mode"); break;
	};

	if (edit_mode_ != Mode::Visual)
		updateDisplay();
	updateStatusText();

	// Unlock mouse
	lockMouse(false);

	// Update toolbar
	if (mode != edit_mode_prev_)
		mapeditor::window()->removeAllCustomToolBars();
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
			mapeditor::window()->addCustomToolBar(
				"Sector Mode", { "mapw_sectormode_normal", "mapw_sectormode_floor", "mapw_sectormode_ceiling" });
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
	{
		SAction::fromId("mapw_mode_things")->setChecked();

		mapeditor::window()->addCustomToolBar("Things Mode", { "mapw_thing_light_previews" });

		SAction::fromId("mapw_thing_light_previews")->setChecked(thing_preview_lights);
	}
	else if (mode == Mode::Visual)
	{
		SAction::fromId("mapw_mode_3d")->setChecked();
		KeyBind::releaseAll();
		lockMouse(true);
		renderer_.renderer3D().refresh();
	}
	mapeditor::window()->refreshToolBar();
}

// -----------------------------------------------------------------------------
// Changes the current sector edit mode to [mode]
// -----------------------------------------------------------------------------
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
	forceRefreshRenderer();
}

// -----------------------------------------------------------------------------
// Cycles to the next sector edit mode. Both -> Floors -> Ceilings
// -----------------------------------------------------------------------------
void MapEditContext::cycleSectorEditMode()
{
	switch (sector_mode_)
	{
	case SectorMode::Both: setSectorEditMode(SectorMode::Floor); break;
	case SectorMode::Floor: setSectorEditMode(SectorMode::Ceiling); break;
	default: setSectorEditMode(SectorMode::Both);
	}
}

// -----------------------------------------------------------------------------
// Locks/unlocks the mouse cursor. A locked cursor is invisible and will be
// moved to the center of the canvas every frame
// -----------------------------------------------------------------------------
void MapEditContext::lockMouse(bool lock)
{
	mouse_locked_ = lock;
	canvas_->lockMouse(lock);
}

// -----------------------------------------------------------------------------
// Updates the current map editor state (hilight, animations, etc.)
// -----------------------------------------------------------------------------
bool MapEditContext::update(long frametime)
{
	// Force an update if animations are active
	if (renderer_.animationsActive() || selection_.hasHilight())
		next_frame_length_ = 2;

	// Ignore if we aren't ready to update
	if (frametime < next_frame_length_)
		return false;

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
		mapeditor::setStatusText(fmt::format("Position: ({}, {}, {})", (int)pos.x, (int)pos.y, (int)pos.z), 3);

		// Update hilight
		mapeditor::Item hl{ -1, mapeditor::ItemType::Any };
		if (!selection_.hilightLocked())
		{
			auto old_hl = selection_.hilight();
			hl          = renderer_.renderer3D().determineHilight();
			if (selection_.setHilight(hl))
			{
				// Update 3d info overlay
				if (info_overlay_3d && hl.index >= 0)
				{
					info_3d_.update(hl, &map_);
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
		if (input_.mouseState() == mapeditor::Input::MouseState::Normal /* && !mouse_movebegin*/)
		{
			auto old_hl = selection_.hilightedObject();
			if (selection_.updateHilight(input_.mousePosMap(), renderer_.view().scale()) && hilight_smooth)
				renderer_.animateHilightChange({}, old_hl);
		}

		// Do item moving if needed
		if (input_.mouseState() == mapeditor::Input::MouseState::Move)
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

	return true;
}

// -----------------------------------------------------------------------------
// Opens [map]
// -----------------------------------------------------------------------------
bool MapEditContext::openMap(Archive::MapDesc map)
{
	log::info("Opening map {}", map.name);
	if (!map_.readMap(map))
		return false;

	// Find camera thing
	if (canvas_)
	{
		MapThing* cam    = nullptr;
		MapThing* pstart = nullptr;
		for (unsigned a = 0; a < map_.nThings(); a++)
		{
			auto thing = map_.thing(a);
			if (thing->type() == 32000)
				cam = thing;
			if (thing->type() == 1)
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
	map_.mapSpecials()->processMapSpecials(&(map_));

	return true;
}

// -----------------------------------------------------------------------------
// Clears and resets the map
// -----------------------------------------------------------------------------
void MapEditContext::clearMap()
{
	// Clear selection
	selection_.clear();
	selection_.clearHilight();

	// Reset state
	edit_3d_.setLinked(true, true);
	input_.setMouseState(mapeditor::Input::MouseState::Normal);
	mapeditor::resetObjectPropertiesPanel();

	// Clear undo manager
	undo_manager_->clear();
	last_undo_level_ = "";

	// Clear other data
	updateTagged();
	info_3d_.reset();
	renderer_.clearAnimations();

	// Clear map
	map_.clearMap();
}

// -----------------------------------------------------------------------------
// Moves and zooms the view to show the object at [index], depending on the
// current edit mode. If [index] is negative, show the current selection or
// hilight instead
// -----------------------------------------------------------------------------
void MapEditContext::showItem(int index)
{
	// Show current selection/hilight if index is not specified
	if (index < 0)
	{
		renderer_.viewFitToObjects(selection_.selectedObjects());
		return;
	}

	selection_.clear();
	int                 max;
	mapeditor::ItemType type;
	switch (edit_mode_)
	{
	case Mode::Vertices:
		type = mapeditor::ItemType::Vertex;
		max  = map_.nVertices();
		break;
	case Mode::Lines:
		type = mapeditor::ItemType::Line;
		max  = map_.nLines();
		break;
	case Mode::Sectors:
		type = mapeditor::ItemType::Sector;
		max  = map_.nSectors();
		break;
	case Mode::Things:
		type = mapeditor::ItemType::Thing;
		max  = map_.nThings();
		break;
	default: return;
	}

	if (index < max)
	{
		selection_.select({ index, type });
		renderer_.viewFitToObjects(selection_.selectedObjects(false));
	}
}

// -----------------------------------------------------------------------------
// Returns a string representation of the current edit mode
// -----------------------------------------------------------------------------
string MapEditContext::modeString(bool plural) const
{
	switch (edit_mode_)
	{
	case Mode::Vertices: return plural ? "Vertices" : "Vertex";
	case Mode::Lines: return plural ? "Lines" : "Line";
	case Mode::Sectors: return plural ? "Sectors" : "Sector";
	case Mode::Things: return plural ? "Things" : "Thing";
	case Mode::Visual: return "3D";
	};

	return plural ? "Items" : "Object";
}

// -----------------------------------------------------------------------------
// Rebuilds thing info lists (pathed things, etc.)
// -----------------------------------------------------------------------------
void MapEditContext::updateThingLists()
{
	pathed_things_.clear();
	map_.things().putAllPathed(pathed_things_);
	map_.setThingsUpdated();
}

// -----------------------------------------------------------------------------
// Sets the cursor on the canvas to [cursor]
// -----------------------------------------------------------------------------
void MapEditContext::setCursor(ui::MouseCursor cursor) const
{
	ui::setCursor(canvas_, cursor);
}

// -----------------------------------------------------------------------------
// Forces a full refresh of the 2d/3d renderers
// -----------------------------------------------------------------------------
void MapEditContext::forceRefreshRenderer()
{
	// Update 3d mode info overlay if needed
	if (edit_mode_ == Mode::Visual)
	{
		auto hl = renderer_.renderer3D().determineHilight();
		info_3d_.update(hl, &map_);
	}

	if (!canvas_->setActive())
		return;

	renderer_.forceUpdate();
}

// -----------------------------------------------------------------------------
// Rebuilds tagged object lists based on the current hilight
// -----------------------------------------------------------------------------
void MapEditContext::updateTagged()
{
	using game::TagType;

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
		int type = 0, tag = 0, ttype = 0;
		if (edit_mode_ == Mode::Lines)
		{
			type = SLADEMap::LINEDEFS;
			tag  = map_.line(hilight_item)->id();
		}
		else if (edit_mode_ == Mode::Things)
		{
			type  = SLADEMap::THINGS;
			tag   = map_.thing(hilight_item)->id();
			ttype = map_.thing(hilight_item)->type();
		}
		else if (edit_mode_ == Mode::Sectors)
		{
			type = SLADEMap::SECTORS;
			tag  = map_.sector(hilight_item)->tag();
		}
		if (tag)
		{
			map_.lines().putAllTaggingWithId(tag, type, tagging_lines_);
			map_.things().putAllTaggingWithId(tag, type, tagging_things_, ttype);
		}

		// Gather affected objects
		if (edit_mode_ == Mode::Lines || edit_mode_ == Mode::Things)
		{
			MapSector* back  = nullptr;
			MapSector* front = nullptr;
			int        tag, arg2, arg3, arg4, arg5, tid;
			auto       needs_tag = TagType::None;

			// Line specials have front and possibly back sectors
			tag = arg2 = arg3 = arg4 = arg5 = tid = 0;
			if (edit_mode_ == Mode::Lines)
			{
				auto line = map_.line(hilight_item);
				if (line->s2())
					back = line->s2()->sector();
				if (line->s1())
					front = line->s1()->sector();
				needs_tag = game::configuration().actionSpecial(line->special()).needsTag();
				tag       = line->arg(0);
				arg2      = line->arg(1);
				arg3      = line->arg(2);
				arg4      = line->arg(3);
				arg5      = line->arg(4);
				tid       = 0;

				// Hexen and UDMF things can have specials too
			}
			else // edit_mode == Mode::Things
			{
				auto thing = map_.thing(hilight_item);
				if (game::configuration().thingType(thing->type()).flags() & game::ThingType::Flags::Script)
					needs_tag = TagType::None;
				else
				{
					needs_tag = game::configuration().thingType(thing->type()).needsTag();
					if (needs_tag == TagType::None)
						needs_tag = game::configuration().actionSpecial(thing->special()).needsTag();
					tag  = thing->arg(0);
					arg2 = thing->arg(1);
					arg3 = thing->arg(2);
					arg4 = thing->arg(3);
					arg5 = thing->arg(4);
					tid  = thing->id();
				}
			}

			// Sector tag
			if (needs_tag == TagType::Sector || (needs_tag == TagType::SectorAndBack && tag > 0))
				map_.sectors().putAllWithId(tag, tagged_sectors_);

			// Backside sector (for local doors)
			else if ((needs_tag == TagType::Back || needs_tag == TagType::SectorAndBack) && back)
				tagged_sectors_.push_back(back);

			// Sector tag *or* backside sector (for zdoom local doors)
			else if (needs_tag == TagType::SectorOrBack)
			{
				if (tag > 0)
					map_.sectors().putAllWithId(tag, tagged_sectors_);
				else if (back)
					tagged_sectors_.push_back(back);
			}

			// Thing ID
			else if (needs_tag == TagType::Thing)
				map_.things().putAllWithId(tag, tagged_things_);

			// Line ID
			else if (needs_tag == TagType::Line)
				map_.lines().putAllWithId(tag, tagged_lines_);

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
					int sectag   = (needs_tag == TagType::Sector1Thing2)   ? tag
								   : (needs_tag == TagType::Thing1Sector2) ? arg2
																		   : arg3;
					if ((thingtag | sectag) == 0)
						break;
					else if (thingtag == 0)
						map_.sectors().putAllWithId(sectag, tagged_sectors_);
					else if (sectag == 0)
						map_.things().putAllWithId(thingtag, tagged_things_);
					else // neither thingtag nor sectag are 0
						map_.putThingsWithIdInSectorTag(thingtag, sectag, tagged_things_);
				}
				break;
				case TagType::Thing1Thing2Thing3:
					if (arg3)
						map_.things().putAllWithId(arg3, tagged_things_);
				case TagType::Thing1Thing2:
					if (arg2)
						map_.things().putAllWithId(arg2, tagged_things_);
				case TagType::Thing1Thing4:
					if (tag)
						map_.things().putAllWithId(tag, tagged_things_);
				case TagType::Thing4:
					if (needs_tag == TagType::Thing1Thing4 || needs_tag == TagType::Thing4)
						if (arg4)
							map_.things().putAllWithId(arg4, tagged_things_);
					break;
				case TagType::Thing5:
					if (arg5)
						map_.things().putAllWithId(arg5, tagged_things_);
					break;
				case TagType::LineNegative:
					if (tag)
						map_.lines().putAllWithId(abs(tag), tagged_lines_);
					break;
				case TagType::LineId1Line2:
					if (arg2)
						map_.lines().putAllWithId(arg2, tagged_lines_);
					break;
				case TagType::Line1Sector2:
					if (tag)
						map_.lines().putAllWithId(tag, tagged_lines_);
					if (arg2)
						map_.sectors().putAllWithId(arg2, tagged_sectors_);
					break;
				case TagType::Sector1Thing2Thing3Thing5:
					if (arg5)
						map_.things().putAllWithId(arg5, tagged_things_);
					if (arg3)
						map_.things().putAllWithId(arg3, tagged_things_);
				case TagType::Sector1Sector2Sector3Sector4:
					if (arg4)
						map_.sectors().putAllWithId(arg4, tagged_sectors_);
					if (arg3)
						map_.sectors().putAllWithId(arg3, tagged_sectors_);
				case TagType::Sector1Sector2:
					if (arg2)
						map_.sectors().putAllWithId(arg2, tagged_sectors_);
					if (tag)
						map_.sectors().putAllWithId(tag, tagged_sectors_);
					break;
				case TagType::Sector2Is3Line:
					if (tag)
					{
						if (arg2 == 3)
							map_.lines().putAllWithId(tag, tagged_lines_);
						else
							map_.sectors().putAllWithId(tag, tagged_sectors_);
					}
					break;
				case TagType::Patrol:
					if (tid)
						map_.things().putAllWithId(tid, tagged_things_, 0, 9047);
					break;
				case TagType::Interpolation:
					if (tid)
						map_.things().putAllWithId(tid, tagged_things_, 0, 9075);
					break;
				default: break;
				}
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Called when the selection is updated, updates the properties panel
// -----------------------------------------------------------------------------
void MapEditContext::selectionUpdated()
{
	// Open selected objects in properties panel
	auto selected = selection_.selectedObjects();
	mapeditor::openMultiObjectProperties(selected);

	last_undo_level_ = "";

	renderer_.animateSelectionChange(selection_);

	updateStatusText();
}

// -----------------------------------------------------------------------------
// Returns the current grid size
// -----------------------------------------------------------------------------
double MapEditContext::gridSize() const
{
	return grid_sizes[grid_size_];
}

// -----------------------------------------------------------------------------
// Increments the grid size
// -----------------------------------------------------------------------------
void MapEditContext::incrementGrid()
{
	grid_size_++;
	if (grid_size_ > 20)
		grid_size_ = 20;

	addEditorMessage(fmt::format("Grid Size: {}x{}", (int)gridSize(), (int)gridSize()));
	updateStatusText();
}

// -----------------------------------------------------------------------------
// Decrements the grid size
// -----------------------------------------------------------------------------
void MapEditContext::decrementGrid()
{
	grid_size_--;
	int mingrid = (map_.currentFormat() == MapFormat::UDMF) ? 0 : 4;
	if (grid_size_ < mingrid)
		grid_size_ = mingrid;

	addEditorMessage(fmt::format("Grid Size: {}x{}", (int)gridSize(), (int)gridSize()));
	updateStatusText();
}

// -----------------------------------------------------------------------------
// Returns the nearest grid point to [position], unless snap to grid is
// disabled. If [force] is true, grid snap setting is ignored
// -----------------------------------------------------------------------------
double MapEditContext::snapToGrid(double position, bool force) const
{
	if (!force && !grid_snap_)
	{
		if (map_.currentFormat() == MapFormat::UDMF)
			return position;
		else
			return ceil(position - 0.5);
	}

	return ceil(position / gridSize() - 0.5) * gridSize();
}

// -----------------------------------------------------------------------------
// Used for pasting. Given an [origin] point and the current [mouse_pos], snaps
// in such a way that the mouse is a number of grid units away from the origin.
// -----------------------------------------------------------------------------
Vec2d MapEditContext::relativeSnapToGrid(Vec2d origin, Vec2d mouse_pos) const
{
	auto delta = mouse_pos - origin;
	delta.x    = snapToGrid(delta.x, false);
	delta.y    = snapToGrid(delta.y, false);
	return origin + delta;
}

// -----------------------------------------------------------------------------
// Begins a tag edit operation
// -----------------------------------------------------------------------------
int MapEditContext::beginTagEdit()
{
	// Check lines mode
	if (edit_mode_ != Mode::Lines)
		return 0;

	// Get selected lines
	auto lines = selection_.selectedLines();
	if (lines.empty())
		return 0;

	// Get current tag
	int tag = lines[0]->arg(0);
	if (tag == 0)
		tag = map_.sectors().firstFreeId();
	current_tag_ = tag;

	// Clear tagged lists
	tagged_lines_.clear();
	tagged_sectors_.clear();
	tagged_things_.clear();

	// Sector tag (for now, 2 will be thing id tag)
	for (unsigned a = 0; a < map_.nSectors(); a++)
	{
		auto sector = map_.sector(a);
		if (sector->tag() == current_tag_)
			tagged_sectors_.push_back(sector);
	}
	return 1;
}

// -----------------------------------------------------------------------------
// Applies the current tag edit tag to the sector at [x,y], or clears the
// sector tag if it is already the same
// -----------------------------------------------------------------------------
void MapEditContext::tagSectorAt(Vec2d pos)
{
	auto sector = map_.sectors().atPos(pos);
	if (!sector)
		return;

	for (unsigned a = 0; a < tagged_sectors_.size(); a++)
	{
		// Check if already tagged
		if (tagged_sectors_[a] == sector)
		{
			// Un-tag
			tagged_sectors_[a] = tagged_sectors_.back();
			tagged_sectors_.pop_back();
			addEditorMessage(fmt::format("Untagged sector {}", sector->index()));
			return;
		}
	}

	// Tag
	tagged_sectors_.push_back(sector);
	addEditorMessage(fmt::format("Tagged sector {}", sector->index()));
}

// -----------------------------------------------------------------------------
// Ends the tag edit operation and applies changes if [accept] is true
// -----------------------------------------------------------------------------
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
			auto sector = map_.sector(a);
			if (sector->tag() == current_tag_)
				sector->setTag(0);
		}

		// If nothing selected, clear line tags
		if (tagged_sectors_.empty())
			current_tag_ = 0;

		// Set line tags (in case of multiple selection)
		for (auto& line : lines)
			line->setArg(0, current_tag_);

		// Set sector tags
		for (auto& sector : tagged_sectors_)
			sector->setTag(current_tag_);

		// Editor message
		if (tagged_sectors_.empty())
			addEditorMessage("Cleared tags");
		else
			addEditorMessage(fmt::format("Set tag {}", current_tag_));

		endUndoRecord(true);
	}
	else
		addEditorMessage("Tag edit cancelled");

	updateTagged();
	setFeatureHelp({});
}

// -----------------------------------------------------------------------------
// Returns the current editor message at [index]
// -----------------------------------------------------------------------------
const string& MapEditContext::editorMessage(int index)
{
	// Check index
	if (index < 0 || index >= (int)editor_messages_.size())
		return strutil::EMPTY;

	return editor_messages_[index].message;
}

// -----------------------------------------------------------------------------
// Returns the amount of time the editor message at [index] has been active
// -----------------------------------------------------------------------------
long MapEditContext::editorMessageTime(int index)
{
	// Check index
	if (index < 0 || index >= (int)editor_messages_.size())
		return -1;

	return app::runTimer() - editor_messages_[index].act_time;
}

// -----------------------------------------------------------------------------
// Adds an editor message, removing the oldest if needed
// -----------------------------------------------------------------------------
void MapEditContext::addEditorMessage(string_view message)
{
	// Remove oldest message if there are too many active
	if (editor_messages_.size() >= 10)
		editor_messages_.erase(editor_messages_.begin());

	// Add message to list
	editor_messages_.emplace_back(message, app::runTimer());
}

// -----------------------------------------------------------------------------
// Sets the feature help text to display [lines]
// -----------------------------------------------------------------------------
void MapEditContext::setFeatureHelp(const vector<string>& lines)
{
	feature_help_lines_.clear();
	feature_help_lines_ = lines;

	log::debug("Set Feature Help Text:");
	for (auto& l : feature_help_lines_)
		log::debug(l);
}

// -----------------------------------------------------------------------------
// Handles the keybind [key]
// -----------------------------------------------------------------------------
bool MapEditContext::handleKeyBind(string_view key, Vec2d position)
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
	if (strutil::startsWith(key, "me2d_sector") && edit_mode_ == Mode::Sectors)
	{
		// Height changes
		if (key == "me2d_sector_floor_up8")
			edit_2d_.changeSectorHeight(8, true, false);
		else if (key == "me2d_sector_floor_up")
			edit_2d_.changeSectorHeight(1, true, false);
		else if (key == "me2d_sector_floor_down8")
			edit_2d_.changeSectorHeight(-8, true, false);
		else if (key == "me2d_sector_floor_down")
			edit_2d_.changeSectorHeight(-1, true, false);
		else if (key == "me2d_sector_ceil_up8")
			edit_2d_.changeSectorHeight(8, false, true);
		else if (key == "me2d_sector_ceil_up")
			edit_2d_.changeSectorHeight(1, false, true);
		else if (key == "me2d_sector_ceil_down8")
			edit_2d_.changeSectorHeight(-8, false, true);
		else if (key == "me2d_sector_ceil_down")
			edit_2d_.changeSectorHeight(-1, false, true);
		else if (key == "me2d_sector_height_up8")
			edit_2d_.changeSectorHeight(8, true, true);
		else if (key == "me2d_sector_height_up")
			edit_2d_.changeSectorHeight(1, true, true);
		else if (key == "me2d_sector_height_down8")
			edit_2d_.changeSectorHeight(-8, true, true);
		else if (key == "me2d_sector_height_down")
			edit_2d_.changeSectorHeight(-1, true, true);

		// Light changes
		else if (key == "me2d_sector_light_up16")
			edit_2d_.changeSectorLight(true, false);
		else if (key == "me2d_sector_light_up")
			edit_2d_.changeSectorLight(true, true);
		else if (key == "me2d_sector_light_down16")
			edit_2d_.changeSectorLight(false, false);
		else if (key == "me2d_sector_light_down")
			edit_2d_.changeSectorLight(false, true);

		// Join
		else if (key == "me2d_sector_join")
			edit_2d_.joinSectors(true);
		else if (key == "me2d_sector_join_keep")
			edit_2d_.joinSectors(false);

		else
			return false;
	}

	// --- 3d mode keybinds ---
	else if (strutil::startsWith(key, "me3d_") && edit_mode_ == Mode::Visual)
	{
		// Check is UDMF
		bool is_udmf = map_.currentFormat() == MapFormat::UDMF;

		// Clear selection
		if (key == "me3d_clear_selection")
		{
			selection_.clear();
			addEditorMessage("Selection cleared");
		}

		// Toggle linked light levels
		else if (key == "me3d_light_toggle_link")
		{
			if (!is_udmf || !game::configuration().featureSupported(game::UDMFFeature::FlatLighting))
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
			if (!is_udmf || !game::configuration().featureSupported(game::UDMFFeature::TextureOffsets))
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
		else if (key == "me3d_copy_tex_type")
			edit_3d_.copy(Edit3D::CopyType::TexType);
		else if (key == "me3d_paste_tex_type")
			edit_3d_.paste(Edit3D::CopyType::TexType);
		else if (key == "me3d_paste_tex_adj")
			edit_3d_.floodFill(Edit3D::CopyType::TexType);

		// Delete texture
		else if (key == "me3d_delete_texture")
			edit_3d_.deleteTexture();

		// Light changes
		else if (key == "me3d_light_up16")
			edit_3d_.changeSectorLight(16);
		else if (key == "me3d_light_up")
			edit_3d_.changeSectorLight(1);
		else if (key == "me3d_light_down16")
			edit_3d_.changeSectorLight(-16);
		else if (key == "me3d_light_down")
			edit_3d_.changeSectorLight(-1);

		// Wall/Flat offset changes
		else if (key == "me3d_xoff_up8")
			edit_3d_.changeOffset(8, true);
		else if (key == "me3d_xoff_up")
			edit_3d_.changeOffset(1, true);
		else if (key == "me3d_xoff_down8")
			edit_3d_.changeOffset(-8, true);
		else if (key == "me3d_xoff_down")
			edit_3d_.changeOffset(-1, true);
		else if (key == "me3d_yoff_up8")
			edit_3d_.changeOffset(8, false);
		else if (key == "me3d_yoff_up")
			edit_3d_.changeOffset(1, false);
		else if (key == "me3d_yoff_down8")
			edit_3d_.changeOffset(-8, false);
		else if (key == "me3d_yoff_down")
			edit_3d_.changeOffset(-1, false);

		// Height changes
		else if (key == "me3d_flat_height_up8")
			edit_3d_.changeSectorHeight(8);
		else if (key == "me3d_flat_height_up")
			edit_3d_.changeSectorHeight(1);
		else if (key == "me3d_flat_height_down8")
			edit_3d_.changeSectorHeight(-8);
		else if (key == "me3d_flat_height_down")
			edit_3d_.changeSectorHeight(-1);

		// Thing height changes
		else if (key == "me3d_thing_up")
			edit_3d_.changeThingZ(1);
		else if (key == "me3d_thing_up8")
			edit_3d_.changeThingZ(8);
		else if (key == "me3d_thing_down")
			edit_3d_.changeThingZ(-1);
		else if (key == "me3d_thing_down8")
			edit_3d_.changeThingZ(-8);

		// Generic height change
		else if (key == "me3d_generic_up8")
			edit_3d_.changeHeight(8);
		else if (key == "me3d_generic_up")
			edit_3d_.changeHeight(1);
		else if (key == "me3d_generic_down8")
			edit_3d_.changeHeight(-8);
		else if (key == "me3d_generic_down")
			edit_3d_.changeHeight(-1);

		// Wall/Flat scale changes
		else if (key == "me3d_scalex_up_l" && is_udmf)
			edit_3d_.changeScale(1, true);
		else if (key == "me3d_scalex_up_s" && is_udmf)
			edit_3d_.changeScale(0.1, true);
		else if (key == "me3d_scalex_down_l" && is_udmf)
			edit_3d_.changeScale(-1, true);
		else if (key == "me3d_scalex_down_s" && is_udmf)
			edit_3d_.changeScale(-0.1, true);
		else if (key == "me3d_scaley_up_l" && is_udmf)
			edit_3d_.changeScale(1, false);
		else if (key == "me3d_scaley_up_s" && is_udmf)
			edit_3d_.changeScale(0.1, false);
		else if (key == "me3d_scaley_down_l" && is_udmf)
			edit_3d_.changeScale(-1, false);
		else if (key == "me3d_scaley_down_s" && is_udmf)
			edit_3d_.changeScale(-0.1, false);

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

// -----------------------------------------------------------------------------
// Updates the map object properties panel and current info overlay from the
// current hilight/selection
// -----------------------------------------------------------------------------
void MapEditContext::updateDisplay()
{
	// Update map object properties panel
	auto selection = selection_.selectedObjects();
	mapeditor::openMultiObjectProperties(selection);

	// Update canvas info overlay
	if (canvas_)
	{
		updateInfoOverlay();
		canvas_->Refresh();
	}
}

// -----------------------------------------------------------------------------
// Updates the window status bar text (mode, grid, etc.)
// -----------------------------------------------------------------------------
void MapEditContext::updateStatusText() const
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

	if (edit_mode_ != Mode::Visual && !selection_.empty())
		mode += fmt::format(" ({} selected)", (int)selection_.size());

	mapeditor::setStatusText(mode, 1);

	// Grid
	string grid;
	if (gridSize() < 1)
		grid = fmt::format("Grid: {:1.2f}x{:1.2f}", gridSize(), gridSize());
	else
	{
		auto gsize = static_cast<int>(round(gridSize()));
		grid       = fmt::format("Grid: {}x{}", gsize, gsize);
	}

	if (grid_snap_)
		grid += " (Snapping ON)";
	else
		grid += " (Snapping OFF)";

	mapeditor::setStatusText(grid, 2);
}

// -----------------------------------------------------------------------------
// Begins recording undo level [name]. [mod] is true if the operation about to
// begin will modify object properties, [create/del] are true if it will create
// or delete objects
// -----------------------------------------------------------------------------
void MapEditContext::beginUndoRecord(string_view name, bool mod, bool create, bool del)
{
	// Setup
	UndoManager* manager = (edit_mode_ == Mode::Visual) ? edit_3d_.undoManager() : undo_manager_.get();
	if (manager->currentlyRecording())
		return;
	undo_modified_ = mod;
	undo_deleted_  = del;
	undo_created_  = create;

	// Begin recording
	manager->beginRecord(name);

	// Init map/objects for recording
	if (undo_modified_)
		MapObject::beginPropBackup(app::runTimer());
	if (undo_deleted_ || undo_created_)
		us_create_delete_ = std::make_unique<mapeditor::MapObjectCreateDeleteUS>();

	// Make sure all modified objects will be picked up
	wxMilliSleep(5);

	last_undo_level_ = "";
}

// -----------------------------------------------------------------------------
// Same as beginUndoRecord, except that subsequent calls to this will not
// record another undo level if [name] is the same as last (used for repeated
// operations like offset changes etc. so that 5 offset changes to the same
// object only create 1 undo level)
// -----------------------------------------------------------------------------
void MapEditContext::beginUndoRecordLocked(string_view name, bool mod, bool create, bool del)
{
	if (name != last_undo_level_)
	{
		beginUndoRecord(name, mod, create, del);
		last_undo_level_ = name;
	}
}

// -----------------------------------------------------------------------------
// Finish recording undo level. Discarded if [success] is false
// -----------------------------------------------------------------------------
void MapEditContext::endUndoRecord(bool success)
{
	auto manager = (edit_mode_ == Mode::Visual) ? edit_3d_.undoManager() : undo_manager_.get();

	if (manager->currentlyRecording())
	{
		// Record necessary undo steps
		MapObject::beginPropBackup(-1);
		bool modified        = false;
		bool created_deleted = false;
		if (undo_modified_)
			modified = manager->recordUndoStep(std::make_unique<mapeditor::MultiMapObjectPropertyChangeUS>());
		if (undo_created_ || undo_deleted_)
		{
			auto ustep = dynamic_cast<mapeditor::MapObjectCreateDeleteUS*>(us_create_delete_.get());
			ustep->checkChanges();
			created_deleted = manager->recordUndoStep(std::move(us_create_delete_));
		}

		// End recording
		manager->endRecord(success && (modified || created_deleted));
	}
	updateThingLists();
	us_create_delete_.reset(nullptr);
	map_.recomputeSpecials();
}

// -----------------------------------------------------------------------------
// Records an object property change undo step for [object]
// -----------------------------------------------------------------------------
void MapEditContext::recordPropertyChangeUndoStep(MapObject* object) const
{
	auto manager = (edit_mode_ == Mode::Visual) ? edit_3d_.undoManager() : undo_manager_.get();
	manager->recordUndoStep(std::make_unique<mapeditor::PropertyChangeUS>(object));
}

// -----------------------------------------------------------------------------
// Undoes the current undo level
// -----------------------------------------------------------------------------
void MapEditContext::doUndo()
{
	// Don't undo if the input state isn't normal
	if (input_.mouseState() != Input::MouseState::Normal)
		return;

	// Clear selection first, since part of it may become invalid
	selection_.clear();

	// Undo
	auto time      = app::runTimer() - 1;
	auto manager   = (edit_mode_ == Mode::Visual) ? edit_3d_.undoManager() : undo_manager_.get();
	auto undo_name = manager->undo();

	// Editor message
	if (!undo_name.empty())
	{
		addEditorMessage(fmt::format("Undo: {}", undo_name));

		// Refresh stuff
		// updateTagged();
		map_.rebuildConnectedLines();
		map_.rebuildConnectedSides();
		map_.setGeometryUpdated();
		map_.updateGeometryInfo(time);
		last_undo_level_ = "";
	}
	updateThingLists();
	map_.recomputeSpecials();
}

// -----------------------------------------------------------------------------
// Redoes the next undo level
// -----------------------------------------------------------------------------
void MapEditContext::doRedo()
{
	// Don't redo if the input state isn't normal
	if (input_.mouseState() != Input::MouseState::Normal)
		return;

	// Clear selection first, since part of it may become invalid
	selection_.clear();

	// Redo
	int  time      = app::runTimer() - 1;
	auto manager   = (edit_mode_ == Mode::Visual) ? edit_3d_.undoManager() : undo_manager_.get();
	auto undo_name = manager->redo();

	// Editor message
	if (!undo_name.empty())
	{
		addEditorMessage(fmt::format("Redo: {}", undo_name));

		// Refresh stuff
		// updateTagged();
		map_.rebuildConnectedLines();
		map_.rebuildConnectedSides();
		map_.setGeometryUpdated();
		map_.updateGeometryInfo(time);
		last_undo_level_ = "";
	}
	updateThingLists();
	map_.recomputeSpecials();
}

// -----------------------------------------------------------------------------
// Returns true if a fullscreen overlay is currently active
// -----------------------------------------------------------------------------
bool MapEditContext::overlayActive() const
{
	if (!overlay_current_)
		return false;
	else
		return overlay_current_->isActive();
}

// -----------------------------------------------------------------------------
// Moves the player 1 start thing to the current position and direction of the
// 3d mode camera
// -----------------------------------------------------------------------------
void MapEditContext::swapPlayerStart3d()
{
	// Find player 1 start
	MapThing* pstart = nullptr;
	for (int a = map_.nThings() - 1; a >= 0; a--)
		if (map_.thing(a)->type() == 1)
		{
			pstart = map_.thing(a);
			break;
		}
	if (!pstart)
		return;

	// Save existing player start pos+dir
	player_start_pos_.set(pstart->position());
	player_start_dir_ = pstart->angle();

	auto campos = renderer_.cameraPos2D();
	pstart->move(campos, false);
	pstart->setAnglePoint(campos + renderer_.cameraDir2D(), false);
}

// -----------------------------------------------------------------------------
// Moves the player 1 start thing to [pos]
// -----------------------------------------------------------------------------
void MapEditContext::swapPlayerStart2d(Vec2d pos)
{
	// Find player 1 start
	MapThing* pstart = nullptr;
	for (int a = map_.nThings() - 1; a >= 0; a--)
		if (map_.thing(a)->type() == 1)
		{
			pstart = map_.thing(a);
			break;
		}
	if (!pstart)
		return;

	// Save existing player start pos+dir
	player_start_pos_.set(pstart->position());
	player_start_dir_ = pstart->angle();

	pstart->move(pos, false);
}

// -----------------------------------------------------------------------------
// Resets the player 1 start thing to its original position
// -----------------------------------------------------------------------------
void MapEditContext::resetPlayerStart() const
{
	// Find player 1 start
	MapThing* pstart = nullptr;
	for (int a = map_.nThings() - 1; a >= 0; a--)
		if (map_.thing(a)->type() == 1)
		{
			pstart = map_.thing(a);
			break;
		}
	if (!pstart)
		return;

	pstart->move(player_start_pos_, false);
	pstart->setAngle(player_start_dir_, false);
}

// -----------------------------------------------------------------------------
// Opens the sector texture selection overlay
// -----------------------------------------------------------------------------
void MapEditContext::openSectorTextureOverlay(vector<MapSector*>& sectors)
{
	overlay_current_ = std::make_unique<SectorTextureOverlay>();
	dynamic_cast<SectorTextureOverlay*>(overlay_current_.get())->openSectors(sectors);
}

// -----------------------------------------------------------------------------
// Opens the quick texture selection (3d mode) overlay
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// Opens the line texture selection overlay
// -----------------------------------------------------------------------------
void MapEditContext::openLineTextureOverlay()
{
	// Get selection
	auto lines = selection_.selectedLines();

	// Open line texture overlay if anything is selected
	if (!lines.empty())
	{
		overlay_current_ = std::make_unique<LineTextureOverlay>();
		dynamic_cast<LineTextureOverlay*>(overlay_current_.get())->openLines(lines);
	}
}

// -----------------------------------------------------------------------------
// Closes the current fullscreen overlay
// -----------------------------------------------------------------------------
void MapEditContext::closeCurrentOverlay(bool cancel) const
{
	if (overlay_current_ && overlay_current_->isActive())
		overlay_current_->close(cancel);
}

// -----------------------------------------------------------------------------
// Updates the current info overlay, depending on edit mode
// -----------------------------------------------------------------------------
void MapEditContext::updateInfoOverlay()
{
	// Update info overlay depending on edit mode
	switch (edit_mode_)
	{
	case Mode::Vertices: info_vertex_.update(selection_.hilightedVertex()); break;
	case Mode::Lines: info_line_.update(selection_.hilightedLine()); break;
	case Mode::Sectors: info_sector_.update(selection_.hilightedSector()); break;
	case Mode::Things: info_thing_.update(selection_.hilightedThing()); break;
	default: break;
	}
}

// -----------------------------------------------------------------------------
// Draws the current info overlay
// -----------------------------------------------------------------------------
void MapEditContext::drawInfoOverlay(const Vec2i& size, float alpha)
{
	switch (edit_mode_)
	{
	case Mode::Vertices: info_vertex_.draw(size.y, size.x, alpha); return;
	case Mode::Lines: info_line_.draw(size.y, size.x, alpha); return;
	case Mode::Sectors: info_sector_.draw(size.y, size.x, alpha); return;
	case Mode::Things: info_thing_.draw(size.y, size.x, alpha); return;
	case Mode::Visual: info_3d_.draw(size.y, size.x, size.x * 0.5, alpha); return;
	}
}

// -----------------------------------------------------------------------------
// Handles an SAction [id]. Returns true if the action was handled here
// -----------------------------------------------------------------------------
bool MapEditContext::handleAction(string_view id)
{
	using namespace mapeditor;

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
		addEditorMessage("Flats: None");
		updateStatusText();
		forceRefreshRenderer();
		return true;
	}

	// 'Untextured' flat type
	else if (id == "mapw_flat_untextured")
	{
		flat_drawtype = 1;
		addEditorMessage("Flats: Untextured");
		updateStatusText();
		forceRefreshRenderer();
		return true;
	}

	// 'Textured' flat type
	else if (id == "mapw_flat_textured")
	{
		flat_drawtype = 2;
		addEditorMessage("Flats: Textured");
		updateStatusText();
		forceRefreshRenderer();
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

	// Clear selection
	else if (id == "mapw_clear_selection")
	{
		selection_.clear();
		addEditorMessage("Selection cleared");
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
		ShowItemDialog dlg(mapeditor::windowWx());
		switch (editMode())
		{
		case Mode::Vertices: dlg.setType(MapObject::Type::Vertex); break;
		case Mode::Lines: dlg.setType(MapObject::Type::Line); break;
		case Mode::Sectors: dlg.setType(MapObject::Type::Sector); break;
		case Mode::Things: dlg.setType(MapObject::Type::Thing); break;
		default: return true;
		}

		// Show dialog
		if (dlg.ShowModal() == wxID_OK)
		{
			// Get entered index
			int index = dlg.index();
			if (index < 0)
				return true;

			// Set appropriate edit mode
			bool side = false;
			switch (dlg.type())
			{
			case MapObject::Type::Vertex: setEditMode(Mode::Vertices); break;
			case MapObject::Type::Line: setEditMode(Mode::Lines); break;
			case MapObject::Type::Side:
				setEditMode(Mode::Lines);
				side = true;
				break;
			case MapObject::Type::Sector: setEditMode(Mode::Sectors); break;
			case MapObject::Type::Thing: setEditMode(Mode::Things); break;
			default: break;
			}

			// If side, get its parent line
			if (side)
			{
				auto s = map_.side(index);
				if (s && s->parentLine())
					index = s->parentLine()->index();
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
		// Mirroring sectors breaks Edit Objects functionality
		if (input_.mouseState() != mapeditor::Input::MouseState::ObjectEdit)
		{
			edit_2d_.mirror(false);
			return true;
		}
	}

	// Mirror X
	else if (id == "mapw_mirror_x")
	{
		// Mirroring sectors breaks Edit Objects functionality
		if (input_.mouseState() != mapeditor::Input::MouseState::ObjectEdit)
		{
			edit_2d_.mirror(true);
			return true;
		}
	}

	// Increment grid
	else if (id == "mapw_grid_increment")
		incrementGrid();

	// Decrement grid
	else if (id == "mapw_grid_decrement")
		decrementGrid();

	// Toggle grid snap
	else if (id == "mapw_grid_snap")
	{
		grid_snap_ = !grid_snap_;
		if (grid_snap_)
			addEditorMessage("Grid Snapping On");
		else
			addEditorMessage("Grid Snapping Off");
		updateStatusText();
	}


	// --- Context menu ---

	// Move 3d mode camera
	else if (id == "mapw_camera_set")
	{
		Vec3d pos    = input().mousePosMap();
		auto  sector = map_.sectors().atPos(input_.mousePosMap());
		if (sector)
			pos.z = sector->floor().plane.heightAt(pos.x, pos.y) + 40;
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
		edit_2d_.createVertex(input_.mousePosMap());
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
		if (!selection.empty())
		{
			ActionSpecialDialog dlg(mapeditor::windowWx(), true);
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
			auto key_accept = KeyBind::bind("map_edit_accept").keysAsString();
			auto key_cancel = KeyBind::bind("map_edit_cancel").keysAsString();
			setFeatureHelp({ "Tag Edit",
							 fmt::format("{} = Accept", key_accept),
							 fmt::format("{} = Cancel", key_cancel),
							 "Left Click = Toggle tagged sector" });
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
		edit_2d_.createThing(input_.mouseDownPosMap());
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
		if (!selection.empty())
		{
			SectorSpecialDialog dlg(mapeditor::windowWx());
			dlg.setup(selection[0]->special());
			if (dlg.ShowModal() == wxID_OK)
			{
				// Set specials of selected sectors
				int special = dlg.getSelectedSpecial();
				beginUndoRecord("Change Sector Special", true, false, false);
				for (auto sector : selection)
					sector->setSpecial(special);
				endUndoRecord();
			}
		}
	}

	// Create sector
	else if (id == "mapw_sector_create")
	{
		edit_2d_.createSector(input_.mouseDownPosMap());
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


// -----------------------------------------------------------------------------
//
// MapArchClipboardItem Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Copies [lines] and all related map structures
// -----------------------------------------------------------------------------
void MapArchClipboardItem::addLines(const vector<MapLine*>& lines)
{
	// Get sectors and sides to copy
	vector<MapSector*> copy_sectors;
	vector<MapSide*>   copy_sides;
	for (auto line : lines)
	{
		auto s1 = line->s1();
		auto s2 = line->s2();

		// Front side
		if (s1)
		{
			copy_sides.push_back(s1);
			if (std::find(copy_sectors.begin(), copy_sectors.end(), s1->sector()) == copy_sectors.end())
				copy_sectors.push_back(s1->sector());
		}

		// Back side
		if (s2)
		{
			copy_sides.push_back(s2);
			if (std::find(copy_sectors.begin(), copy_sectors.end(), s2->sector()) == copy_sectors.end())
				copy_sectors.push_back(s2->sector());
		}
	}

	// Copy sectors
	for (auto& sector : copy_sectors)
	{
		auto copy = std::make_unique<MapSector>();
		copy->copy(sector);
		sectors_.push_back(std::move(copy));
	}

	// Copy sides
	for (auto& side : copy_sides)
	{
		auto copy = std::make_unique<MapSide>();
		copy->copy(side);

		// Set relative sector
		for (unsigned b = 0; b < copy_sectors.size(); b++)
		{
			if (side->sector() == copy_sectors[b])
			{
				copy->setSector(sectors_[b].get());
				break;
			}
		}

		sides_.push_back(std::move(copy));
	}

	// Get vertices to copy (and determine midpoint)
	double             min_x = 9999999;
	double             max_x = -9999999;
	double             min_y = 9999999;
	double             max_y = -9999999;
	vector<MapVertex*> copy_verts;
	for (auto line : lines)
	{
		auto v1 = line->v1();
		auto v2 = line->v2();

		// Add vertices to copy list
		if (std::find(copy_verts.begin(), copy_verts.end(), v1) == copy_verts.end())
			copy_verts.push_back(v1);
		if (std::find(copy_verts.begin(), copy_verts.end(), v2) == copy_verts.end())
			copy_verts.push_back(v2);

		// Update min/max
		if (v1->xPos() < min_x)
			min_x = v1->xPos();
		if (v1->xPos() > max_x)
			max_x = v1->xPos();
		if (v1->yPos() < min_y)
			min_y = v1->yPos();
		if (v1->yPos() > max_y)
			max_y = v1->yPos();
		if (v2->xPos() < min_x)
			min_x = v2->xPos();
		if (v2->xPos() > max_x)
			max_x = v2->xPos();
		if (v2->yPos() < min_y)
			min_y = v2->yPos();
		if (v2->yPos() > max_y)
			max_y = v2->yPos();
	}

	// Determine midpoint
	double mid_x = min_x + ((max_x - min_x) * 0.5);
	double mid_y = min_y + ((max_y - min_y) * 0.5);
	midpoint_.set(mid_x, mid_y);

	// Copy vertices
	for (auto& vertex : copy_verts)
	{
		auto copy = std::make_unique<MapVertex>(vertex->position() - midpoint_);
		copy->copy(vertex);
		vertices_.push_back(std::move(copy));
	}

	// Copy lines
	for (auto line : lines)
	{
		// Get relative sides
		MapSide* s1       = nullptr;
		MapSide* s2       = nullptr;
		bool     s1_found = false;
		bool     s2_found = !(line->s2());
		for (unsigned i = 0; i < copy_sides.size(); i++)
		{
			if (line->s1() == copy_sides[i])
			{
				s1       = sides_[i].get();
				s1_found = true;
			}
			if (line->s2() == copy_sides[i])
			{
				s2       = sides_[i].get();
				s2_found = true;
			}

			if (s1_found && s2_found)
				break;
		}

		// Get relative vertices
		MapVertex* v1 = nullptr;
		MapVertex* v2 = nullptr;
		for (unsigned i = 0; i < copy_verts.size(); i++)
		{
			if (line->v1() == copy_verts[i])
				v1 = vertices_[i].get();
			if (line->v2() == copy_verts[i])
				v2 = vertices_[i].get();

			if (v1 && v2)
				break;
		}

		// Copy line
		auto copy = std::make_unique<MapLine>(v1, v2, s1, s2);
		copy->copy(line);
		lines_.push_back(std::move(copy));
	}
}

// -----------------------------------------------------------------------------
// Returns a string with info on what items are copied
// -----------------------------------------------------------------------------
string MapArchClipboardItem::info() const
{
	return fmt::format(
		"{} Vertices, {} Lines, {} Sides and {} Sectors",
		vertices_.size(),
		lines_.size(),
		sides_.size(),
		sectors_.size());
}

// -----------------------------------------------------------------------------
// Pastes copied architecture to [map] at [position]
// -----------------------------------------------------------------------------
vector<MapVertex*> MapArchClipboardItem::pasteToMap(SLADEMap* map, Vec2d position)
{
	std::map<MapVertex*, MapVertex*> vertMap;
	std::map<MapSector*, MapSector*> sectMap;
	std::map<MapSide*, MapSide*>     sideMap;
	// Not used yet...
	// std::map<MapLine*, MapLine*> lineMap;

	// Add vertices
	vector<MapVertex*> new_verts;
	for (auto& vertex : vertices_)
	{
		new_verts.push_back(map->createVertex(position + vertex->position()));
		new_verts.back()->copy(vertex.get());
		vertMap[vertex.get()] = new_verts.back();
	}

	// Add sectors
	for (auto& sector : sectors_)
	{
		auto new_sector = map->createSector();
		new_sector->copy(sector.get());
		sectMap[sector.get()] = new_sector;
	}

	// Add sides
	// int first_new_side = map->nSides();
	for (auto& side : sides_)
	{
		// Get relative sector
		auto sector = findInMap(sectMap, side->sector());

		auto new_side = map->createSide(sector);
		new_side->copy(side.get());
		sideMap[side.get()] = new_side;
	}

	// Add lines
	// int first_new_line = map->nLines();
	for (auto& line : lines_)
	{
		// Get relative vertices
		auto v1 = findInMap(vertMap, line->v1());
		auto v2 = findInMap(vertMap, line->v2());

		if (!v1)
		{
			log::info(1, "no v1");
			continue;
		}
		if (!v2)
		{
			log::info(1, "no v2");
		}

		auto newline = map->createLine(v1, v2, true);
		newline->copy(line.get());

		// Set relative sides
		auto newS1 = findInMap(sideMap, line->s1());
		auto newS2 = findInMap(sideMap, line->s2());
		if (newS1)
			newline->setS1(newS1);
		if (newS2)
			newline->setS2(newS2);

		// Set important flags (needed when copying from Doom/Hexen format to UDMF)
		// Won't be needed when proper map format conversion stuff is implemented
		game::configuration().setLineBasicFlag("twosided", newline, map->currentFormat(), (newS1 && newS2));
		game::configuration().setLineBasicFlag("blocking", newline, map->currentFormat(), !newS2);
	}

	// TODO:
	// - Split lines
	// - Merge lines

	//// Fix sector references
	//// TODO: figure out what lines are 'outside' on copy, only fix said lines
	// for (unsigned a = first_new_line; a < map->nLines(); a++)
	//{
	//	MapLine* line = map->getLine(a);
	//	MapSector* sec1 = map->getLineSideSector(line, true);
	//	MapSector* sec2 = map->getLineSideSector(line, false);
	//	int i1 = -1;
	//	int i2 = -2;
	//	if (sec1) i1 = sec1->getIndex();
	//	if (sec2) i2 = sec2->getIndex();
	//	map->setLineSector(a, i1, true);
	//	map->setLineSector(a, i2, false);
	//}

	return new_verts;
}

// -----------------------------------------------------------------------------
// Adds all copied lines to [list]
// -----------------------------------------------------------------------------
void MapArchClipboardItem::putLines(vector<MapLine*>& list)
{
	for (auto& line : lines_)
		list.push_back(line.get());
}


// -----------------------------------------------------------------------------
//
// MapThingsClipboardItem Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Copies [things]
// -----------------------------------------------------------------------------
void MapThingsClipboardItem::addThings(vector<MapThing*>& things)
{
	// Copy things
	double min_x = 99999999;
	double min_y = 99999999;
	double max_x = -99999999;
	double max_y = -99999999;
	for (auto& thing : things)
	{
		auto copy_thing = std::make_unique<MapThing>();
		copy_thing->copy(thing);
		things_.push_back(std::move(copy_thing));

		if (thing->xPos() < min_x)
			min_x = thing->xPos();
		if (thing->yPos() < min_y)
			min_y = thing->yPos();
		if (thing->xPos() > max_x)
			max_x = thing->xPos();
		if (thing->yPos() > max_y)
			max_y = thing->yPos();
	}

	// Get midpoint
	double mid_x = min_x + ((max_x - min_x) * 0.5);
	double mid_y = min_y + ((max_y - min_y) * 0.5);
	midpoint_.set(mid_x, mid_y);

	// Adjust thing positions
	for (auto& thing : things_)
		thing->move(thing->position() - midpoint_);
}

// -----------------------------------------------------------------------------
// Returns a string with info on what items are copied
// -----------------------------------------------------------------------------
string MapThingsClipboardItem::info() const
{
	return fmt::format("{} Things", things_.size());
}

// -----------------------------------------------------------------------------
// Pastes copied things to [map] at [position]
// -----------------------------------------------------------------------------
void MapThingsClipboardItem::pasteToMap(SLADEMap* map, Vec2d position)
{
	for (auto& thing : things_)
	{
		auto newthing = map->createThing({ 0., 0. });
		newthing->copy(thing.get());
		newthing->move(position + thing->position());
	}
}

// -----------------------------------------------------------------------------
// Adds all copied things to [list]
// -----------------------------------------------------------------------------
void MapThingsClipboardItem::putThings(vector<MapThing*>& list)
{
	for (auto& thing : things_)
		list.push_back(thing.get());
}


// -----------------------------------------------------------------------------
//
// Console Commands
//
// -----------------------------------------------------------------------------

CONSOLE_COMMAND(m_show_item, 1, true)
{
	int index = strutil::asInt(args[0]);
	mapeditor::editContext().showItem(index);
}

CONSOLE_COMMAND(m_check, 0, true)
{
	if (args.empty())
	{
		log::console("Usage: m_check <check1> <check2> ...");

		log::console("Available map checks:");
		for (auto a = 0; a < MapCheck::NumStandardChecks; ++a)
			log::console(fmt::format(
				"{}: Check for {}",
				MapCheck::standardCheckId(static_cast<MapCheck::StandardCheck>(a)),
				MapCheck::standardCheckDesc(static_cast<MapCheck::StandardCheck>(a))));

		log::console("all: Run all checks");

		return;
	}

	auto map    = &(mapeditor::editContext().map());
	auto texman = &(mapeditor::textureManager());

	// Get checks to run
	vector<unique_ptr<MapCheck>> checks;

	// Check for 'all'
	bool all = false;
	for (auto& arg : args)
		if (strutil::equalCI(arg, "all"))
		{
			all = true;
			break;
		}

	if (all)
	{
		for (auto a = 0; a < MapCheck::NumStandardChecks; ++a)
			checks.push_back(MapCheck::standardCheck(static_cast<MapCheck::StandardCheck>(a), map, texman));
	}
	else
	{
		for (auto& arg : args)
		{
			auto check = MapCheck::standardCheck(strutil::lower(arg), map, texman);
			if (check)
				checks.push_back(std::move(check));
			else
				log::console(fmt::format("Unknown check \"{}\"", arg));
		}
	}

	// Run checks
	for (auto& check : checks)
	{
		// Run
		log::console(check->progressText());
		check->doCheck();

		// Check if no problems found
		if (check->nProblems() == 0)
			log::console(check->problemDesc(0));

		// List problem details
		for (unsigned b = 0; b < check->nProblems(); b++)
			log::console(check->problemDesc(b));
	}
}




// testing stuff

CONSOLE_COMMAND(m_test_sector, 0, false)
{
	sf::Clock clock;
	SLADEMap& map = mapeditor::editContext().map();
	for (unsigned a = 0; a < map.nThings(); a++)
		map.sectors().atPos(map.thing(a)->position());
	long ms = clock.getElapsedTime().asMilliseconds();
	log::info("Took {}ms", ms);
}

CONSOLE_COMMAND(m_test_mobj_backup, 0, false)
{
	sf::Clock clock;
	sf::Clock totalClock;
	SLADEMap& map    = mapeditor::editContext().map();
	auto      backup = new MapObject::Backup();

	// Vertices
	for (unsigned a = 0; a < map.nVertices(); a++)
		map.vertex(a)->backupTo(backup);
	log::info("Vertices: {}ms", clock.getElapsedTime().asMilliseconds());

	// Lines
	clock.restart();
	for (unsigned a = 0; a < map.nLines(); a++)
		map.line(a)->backupTo(backup);
	log::info("Lines: {}ms", clock.getElapsedTime().asMilliseconds());

	// Sides
	clock.restart();
	for (unsigned a = 0; a < map.nSides(); a++)
		map.side(a)->backupTo(backup);
	log::info("Sides: {}ms", clock.getElapsedTime().asMilliseconds());

	// Sectors
	clock.restart();
	for (unsigned a = 0; a < map.nSectors(); a++)
		map.sector(a)->backupTo(backup);
	log::info("Sectors: {}ms", clock.getElapsedTime().asMilliseconds());

	// Things
	clock.restart();
	for (unsigned a = 0; a < map.nThings(); a++)
		map.thing(a)->backupTo(backup);
	log::info("Things: {}ms", clock.getElapsedTime().asMilliseconds());

	log::info("Total: {}ms", totalClock.getElapsedTime().asMilliseconds());
}

CONSOLE_COMMAND(m_vertex_attached, 1, false)
{
	MapVertex* vertex = mapeditor::editContext().map().vertex(atoi(args[0].c_str()));
	if (vertex)
	{
		log::info(1, "Attached lines:");
		for (unsigned a = 0; a < vertex->nConnectedLines(); a++)
			log::info("Line #{}", vertex->connectedLine(a)->index());
	}
}

CONSOLE_COMMAND(m_n_polys, 0, false)
{
	SLADEMap& map   = mapeditor::editContext().map();
	int       npoly = 0;
	for (unsigned a = 0; a < map.nSectors(); a++)
		npoly += map.sector(a)->polygon()->nSubPolys();

	log::console(fmt::format("{} polygons total", npoly));
}

CONSOLE_COMMAND(mobj_info, 1, false)
{
	int id = strutil::asInt(args[0]);

	auto obj = mapeditor::editContext().map().mapData().getObjectById(id);
	if (!obj)
		log::console("Object id out of range");
	else
	{
		MapObject::Backup bak;
		obj->backupTo(&bak);
		log::console(fmt::format("Object {}: {} #{}", id, obj->typeName(), obj->index()));
		log::console("Properties:");
		log::console(bak.properties.toString());
		log::console("Properties (internal):");
		log::console(bak.props_internal.toString());
	}
}
