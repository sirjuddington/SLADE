
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Input.cpp
// Description: Input class - handles input for the map editor
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
#include "Input.h"
#include "App.h"
#include "Edit2D.h"
#include "Edit3D.h"
#include "Game/Configuration.h"
#include "General/Clipboard.h"
#include "General/KeyBind.h"
#include "General/SAction.h"
#include "LineDraw.h"
#include "MapEditor/ItemSelection.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/Renderer/MCAnimations.h"
#include "MapEditor/Renderer/MapRenderer3D.h"
#include "MapEditor/Renderer/Overlays/MCOverlay.h"
#include "MapEditor/Renderer/Renderer.h"
#include "MapEditor/UI/MapCanvas.h"
#include "MapEditor/UI/MapEditorWindow.h"
#include "MapEditor/UI/ObjectEditPanel.h"
#include "MoveObjects.h"
#include "ObjectEdit.h"
#include "OpenGL/Camera.h"
#include "OpenGL/View.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapThing.h"
#include "SLADEMap/MapObjectList/SectorList.h"
#include "SLADEMap/MapSpecials/MapSpecials.h"
#include "SLADEMap/SLADEMap.h"
#include "UI/UI.h"

using namespace slade;
using namespace mapeditor;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, property_edit_dclick, true, CVar::Flag::Save)
CVAR(Bool, selection_clear_click, false, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Int, flat_drawtype)
EXTERN_CVAR(Bool, map_show_selection_numbers)
EXTERN_CVAR(Float, render_3d_brightness)
EXTERN_CVAR(Bool, camera_3d_gravity)
EXTERN_CVAR(Int, render_3d_things)
EXTERN_CVAR(Int, render_3d_things_style)
EXTERN_CVAR(Int, render_3d_hilight)
EXTERN_CVAR(Bool, info_overlay_3d)


// -----------------------------------------------------------------------------
//
// Input Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Input class constructor
// -----------------------------------------------------------------------------
Input::Input(MapEditContext& context) : context_{ &context } {}

// -----------------------------------------------------------------------------
// Handles mouse movement to [new_x],[new_y] on the map editor view
// -----------------------------------------------------------------------------
bool Input::mouseMove(int new_x, int new_y)
{
	// Check if a full screen overlay is active
	if (context_->overlayActive())
	{
		context_->currentOverlay()->mouseMotion(new_x, new_y);
		return false;
	}

	// Panning
	if (panning_)
		context_->renderer().pan(mouse_pos_.x - new_x, -(mouse_pos_.y - new_y), true);

	// Update mouse variables
	mouse_pos_     = { new_x, new_y };
	mouse_pos_map_ = context_->renderer().view().canvasPos(mouse_pos_);

	// Update coordinates on status bar
	double mx = context_->snapToGrid(mouse_pos_map_.x, false);
	double my = context_->snapToGrid(mouse_pos_map_.y, false);
	string status_text;
	if (context_->mapDesc().format == MapFormat::UDMF)
		status_text = fmt::format("Position: ({:1.3f}, {:1.3f})", mx, my);
	else
		status_text = fmt::format("Position: ({}, {})", static_cast<int>(mx), static_cast<int>(my));
	mapeditor::setStatusText(status_text, 3);

	// Object edit
	auto edit_state = context_->objectEdit().state();
	if (mouse_state_ == MouseState::ObjectEdit)
	{
		// Do dragging if left mouse is down
		if (mouse_button_down_[Left] && edit_state != ObjectEdit::State::None)
		{
			auto& group = context_->objectEdit().group();

			if (context_->objectEdit().rotating())
			{
				// Rotate
				group.doRotate(mouse_down_pos_map_, mouse_pos_map_, !shift_down_);
				mapeditor::window()->objectEditPanel()->update(&group, true);
			}
			else
			{
				// Get dragged offsets
				double xoff = mouse_pos_map_.x - mouse_down_pos_map_.x;
				double yoff = mouse_pos_map_.y - mouse_down_pos_map_.y;

				// Snap to grid if shift not held down
				if (!shift_down_)
				{
					xoff = context_->snapToGrid(xoff);
					yoff = context_->snapToGrid(yoff);
				}

				if (edit_state == ObjectEdit::State::Move)
				{
					// Move objects
					group.doMove(xoff, yoff);
					mapeditor::window()->objectEditPanel()->update(&group);
				}
				else
				{
					// Scale objects
					group.doScale(
						xoff,
						yoff,
						context_->objectEdit().stateLeft(false),
						context_->objectEdit().stateTop(false),
						context_->objectEdit().stateRight(false),
						context_->objectEdit().stateBottom(false));
					mapeditor::window()->objectEditPanel()->update(&group);
				}
			}
		}
		else
			context_->objectEdit().determineState();

		return false;
	}

	// Check if we want to start a selection box
	if (mouse_drag_ == DragType::Selection
		&& glm::length(Vec2d(mouse_pos_.x - mouse_down_pos_.x, mouse_pos_.y - mouse_down_pos_.y)) > 16)
	{
		mouse_state_ = MouseState::Selection;
		mouse_drag_  = DragType::None;
	}

	// Check if we want to start moving
	if (mouse_drag_ == DragType::Move
		&& glm::length(Vec2d(mouse_pos_.x - mouse_down_pos_.x, mouse_pos_.y - mouse_down_pos_.y)) > 4)
	{
		mouse_state_ = MouseState::Move;
		mouse_drag_  = DragType::None;
		context_->moveObjects().begin(mouse_down_pos_map_);
		context_->renderer().forceUpdate(true, false);
	}

	// Check if we want to start mouselook
	if (mouse_drag_ == DragType::MouseLook
		&& glm::length(Vec2d(mouse_pos_.x - mouse_down_pos_.x, mouse_pos_.y - mouse_down_pos_.y)) > 4)
	{
		mouse_state_ = MouseState::MouseLook;
		mouse_drag_  = DragType::None;
		context_->canvas()->lockMouse(true);
	}

	// Check if we are in thing quick angle state
	if (mouse_state_ == MouseState::ThingAngle)
		context_->edit2D().thingQuickAngle(mouse_pos_map_);

	// Update shape drawing if needed
	if (mouse_state_ == MouseState::LineDraw && context_->lineDraw().state() == LineDraw::State::ShapeEdge)
		context_->lineDraw().updateShape(mouse_pos_map_);

	return true;
}

// -----------------------------------------------------------------------------
// Handles mouse [button] press at [x,y] on the map editor view.
// If [double_click] is true, this is a double-click event
// -----------------------------------------------------------------------------
bool Input::mouseDown(MouseButton button, int x, int y, bool double_click)
{
	// Update hilight
	if (mouse_state_ == MouseState::Normal)
		context_->selection().updateHilight(mouse_pos_map_, context_->renderer().view().scale().x);

	// Update mouse variables
	mouse_pos_                 = { x, y };
	mouse_pos_map_             = context_->renderer().view().canvasPos(mouse_pos_);
	mouse_down_pos_            = mouse_pos_;
	mouse_down_pos_map_        = mouse_pos_map_;
	mouse_button_down_[button] = true;
	mouse_drag_                = DragType::None;

	// Check if a full screen overlay is active
	if (context_->overlayActive())
	{
		// Left click
		if (button == Left)
			context_->currentOverlay()->mouseLeftClick();

		// Right click
		else if (button == Right)
			context_->currentOverlay()->mouseRightClick();

		return false;
	}

	// Left button
	if (button == Left)
	{
		// 3d mode
		if (context_->editMode() == Mode::Visual)
		{
			// If the mouse is unlocked, lock the mouse
			// if (!context_->mouseLocked())
			//	context_->lockMouse(true);
			// else
			{
				// Shift down, select all matching adjacent structures
				if (shift_down_)
					context_->edit3D().selectAdjacent(context_->hilightItem());

				// Otherwise toggle selection
				else
					context_->selection().toggleCurrent();
			}

			return false;
		}

		// Line drawing state, add line draw point
		if (mouse_state_ == MouseState::LineDraw)
		{
			// Snap point to nearest vertex if shift is held down
			bool nearest_vertex = false;
			if (shift_down_)
				nearest_vertex = true;

			// Line drawing
			if (context_->lineDraw().state() == LineDraw::State::Line)
			{
				if (context_->lineDraw().addPoint(mouse_down_pos_map_, nearest_vertex))
				{
					// If line drawing finished, revert to normal state
					mouse_state_ = MouseState::Normal;
				}
			}

			// Shape drawing
			else
			{
				if (context_->lineDraw().state() == LineDraw::State::ShapeOrigin)
				{
					// Set shape origin
					context_->lineDraw().setShapeOrigin(mouse_down_pos_map_, nearest_vertex);
					context_->lineDraw().setState(LineDraw::State::ShapeEdge);
				}
				else
				{
					// Finish shape draw
					context_->lineDraw().end(true);
					mapeditor::window()->showShapeDrawPanel(false);
					mouse_state_ = MouseState::Normal;
				}
			}
		}

		// Paste state, accept paste
		else if (mouse_state_ == MouseState::Paste)
		{
			context_->edit2D().paste(mouse_pos_map_);
			if (!shift_down_)
				mouse_state_ = MouseState::Normal;
		}

		// Sector tagging state
		else if (mouse_state_ == MouseState::TagSectors)
		{
			context_->tagSectorAt(mouse_pos_map_);
		}

		else if (mouse_state_ == MouseState::Normal)
		{
			// Double click to edit the current selection
			if (double_click && property_edit_dclick)
			{
				context_->edit2D().editObjectProperties();
				if (context_->selection().size() == 1)
					context_->selection().clear();
			}
			// Begin box selection if shift is held down, otherwise toggle selection on hilighted object
			else if (shift_down_)
				mouse_state_ = MouseState::Selection;
			else
			{
				if (!context_->selection().toggleCurrent(selection_clear_click))
					mouse_drag_ = DragType::Selection;
			}
		}
	}

	// Right button
	else if (button == Right)
	{
		// 3d mode
		if (context_->editMode() == Mode::Visual)
		{
			// Begin mouselook on drag
			mouse_drag_ = DragType::MouseLook;
		}

		// Remove line draw point if in line drawing state
		else if (mouse_state_ == MouseState::LineDraw)
		{
			// Line drawing
			if (context_->lineDraw().state() == LineDraw::State::Line)
				context_->lineDraw().removePoint();

			// Shape drawing
			else if (context_->lineDraw().state() == LineDraw::State::ShapeEdge)
			{
				context_->lineDraw().end(false);
				context_->lineDraw().setState(LineDraw::State::ShapeOrigin);
			}
		}

		// Normal state
		else if (mouse_state_ == MouseState::Normal)
		{
			// Begin move if something is selected/hilighted
			if (context_->selection().hasHilightOrSelection())
				mouse_drag_ = DragType::Move;
		}
	}

	// Any other mouse button (let keybind system handle it)
	else
		KeyBind::keyPressed(Keypress(mouseButtonKBName(button), alt_down_, ctrl_down_, shift_down_));

	return true;
}

// -----------------------------------------------------------------------------
// Handles mouse [button] release on the map editor view
// -----------------------------------------------------------------------------
bool Input::mouseUp(MouseButton button)
{
	// Update mouse variables
	mouse_button_down_[button] = false;

	// Check if a full screen overlay is active
	if (context_->overlayActive())
		return false;

	// Left button
	if (button == Left)
	{
		mouse_drag_ = DragType::None;

		// If we're ending a box selection
		if (mouse_state_ == MouseState::Selection)
		{
			// Reset mouse state
			mouse_state_ = MouseState::Normal;

			// Select
			context_->selection().selectWithin(
				{ glm::min(mouse_down_pos_map_.x, mouse_pos_map_.x),
				  glm::min(mouse_down_pos_map_.y, mouse_pos_map_.y),
				  glm::max(mouse_down_pos_map_.x, mouse_pos_map_.x),
				  glm::max(mouse_down_pos_map_.y, mouse_pos_map_.y) },
				shift_down_);

			// Begin selection box fade animation
			context_->renderer().addAnimation(
				std::make_unique<MCASelboxFader>(app::runTimer(), mouse_down_pos_map_, mouse_pos_map_));
		}

		// If we're in object edit mode
		if (mouse_state_ == MouseState::ObjectEdit)
			context_->objectEdit().group().resetPositions();
	}

	// Right button
	else if (button == Right)
	{
		mouse_drag_ = DragType::None;

		if (mouse_state_ == MouseState::Move)
		{
			context_->moveObjects().end();
			mouse_state_ = MouseState::Normal;
			context_->renderer().forceUpdate(true, false);
		}

		// Paste state, cancel paste
		else if (mouse_state_ == MouseState::Paste)
			mouse_state_ = MouseState::Normal;

		// Mouselook state, unlock mouse cursor
		else if (mouse_state_ == MouseState::MouseLook)
		{
			context_->canvas()->lockMouse(false);
			mouse_state_ = MouseState::Normal;
		}

		else if (mouse_state_ == MouseState::Normal)
			mapeditor::openContextMenu();
	}

	// Any other mouse button (let keybind system handle it)
	else if (mouse_state_ != MouseState::Selection)
		KeyBind::keyReleased(mouseButtonKBName(button));

	return true;
}

// -----------------------------------------------------------------------------
// Handles mouse wheel movement depending on [up]
// -----------------------------------------------------------------------------
void Input::mouseWheel(bool up, double amount)
{
	mouse_wheel_speed_ = amount;

	if (up)
	{
		KeyBind::keyPressed(Keypress("mwheelup", alt_down_, ctrl_down_, shift_down_));

		// Send to overlay if active
		if (context_->overlayActive())
			context_->currentOverlay()->keyDown("mwheelup");

		KeyBind::keyReleased("mwheelup");
	}
	else
	{
		KeyBind::keyPressed(Keypress("mwheeldown", alt_down_, ctrl_down_, shift_down_));

		// Send to overlay if active
		if (context_->overlayActive())
			context_->currentOverlay()->keyDown("mwheeldown");

		KeyBind::keyReleased("mwheeldown");
	}
}

// -----------------------------------------------------------------------------
// Handles the mouse pointer leaving the map editor view
// -----------------------------------------------------------------------------
void Input::mouseLeave()
{
	// Stop panning
	if (panning_)
	{
		panning_ = false;
		context_->setCursor(ui::MouseCursor::Normal);
	}
}

// -----------------------------------------------------------------------------
// Updates key modifier variables based on a wxWidgets key modifier bit field
// -----------------------------------------------------------------------------
void Input::updateKeyModifiersWx(int modifiers)
{
	updateKeyModifiers((modifiers & wxMOD_SHIFT) > 0, (modifiers & wxMOD_CONTROL) > 0, (modifiers & wxMOD_ALT) > 0);
}

// -----------------------------------------------------------------------------
// Handles [key] being pressed in the map editor
// -----------------------------------------------------------------------------
bool Input::keyDown(string_view key) const
{
	// Send to overlay if active
	if (context_->overlayActive())
		context_->currentOverlay()->keyDown(key);

	// Let keybind system handle it
	return KeyBind::keyPressed({ key, alt_down_, ctrl_down_, shift_down_ });
}

// -----------------------------------------------------------------------------
// Handles [key] being released in the map editor
// -----------------------------------------------------------------------------
bool Input::keyUp(string_view key) const
{
	// Let keybind system handle it
	return KeyBind::keyReleased(key);
}

// -----------------------------------------------------------------------------
// Called when the key bind [name] is pressed
// -----------------------------------------------------------------------------
void Input::onKeyBindPress(string_view name)
{
	// Check if an overlay is active
	if (context_->overlayActive())
	{
		// Accept edit
		if (name == "map_edit_accept")
		{
			context_->closeCurrentOverlay();
			context_->renderer().renderer3D().enableHilight(true);
			context_->renderer().renderer3D().enableSelection(true);
		}

		// Cancel edit
		else if (name == "map_edit_cancel")
		{
			context_->closeCurrentOverlay(true);
			context_->renderer().renderer3D().enableHilight(true);
			context_->renderer().renderer3D().enableSelection(true);
		}

		// Nothing else
		return;
	}

	// Toggle 3d mode
	if (name == "map_toggle_3d")
	{
		if (context_->editMode() == Mode::Visual)
			context_->setPrevEditMode();
		else
			context_->setEditMode(Mode::Visual);
	}

	// Send to edit context first
	if (mouse_state_ == MouseState::Normal)
	{
		if (context_->handleKeyBind(name, mouse_pos_map_))
			return;
	}

	// Handle keybinds depending on mode
	if (context_->editMode() == Mode::Visual)
		handleKeyBind3d(name);
	else
	{
		handleKeyBind2dView(name);
		handleKeyBind2d(name);
	}
}

// -----------------------------------------------------------------------------
// Called when the key bind [name] is released
// -----------------------------------------------------------------------------
void Input::onKeyBindRelease(string_view name)
{
	if (name == "me2d_pan_view" && panning_)
	{
		panning_ = false;
		if (mouse_state_ == MouseState::Normal)
			context_->selection().updateHilight(mouse_pos_map_, context_->renderer().view().scale().x);
		context_->setCursor(ui::MouseCursor::Normal);
	}

	else if (name == "me2d_thing_quick_angle" && mouse_state_ == MouseState::ThingAngle)
	{
		mouse_state_ = MouseState::Normal;
		context_->endUndoRecord(true);
		context_->selection().updateHilight(mouse_pos_map_, context_->renderer().view().scale().x);
	}
}

// -----------------------------------------------------------------------------
// Handles 2d mode view-related keybinds
// (can generally be used no matter the current editor state)
// -----------------------------------------------------------------------------
void Input::handleKeyBind2dView(string_view name)
{
	// Pan left
	if (name == "me2d_left")
		context_->renderer().pan(-128, 0, true);

	// Pan right
	else if (name == "me2d_right")
		context_->renderer().pan(128, 0, true);

	// Pan up
	else if (name == "me2d_up")
		context_->renderer().pan(0, 128, true);

	// Pan down
	else if (name == "me2d_down")
		context_->renderer().pan(0, -128, true);

	// Zoom out
	else if (name == "me2d_zoom_out")
		context_->renderer().zoom(0.8);

	// Zoom in
	else if (name == "me2d_zoom_in")
		context_->renderer().zoom(1.25);

	// Zoom out (follow mouse)
	if (name == "me2d_zoom_out_m")
		context_->renderer().zoom(1.0 - (0.2 * mouse_wheel_speed_), true);

	// Zoom in (follow mouse)
	else if (name == "me2d_zoom_in_m")
		context_->renderer().zoom(1.0 + (0.25 * mouse_wheel_speed_), true);

	// Zoom in (show object)
	else if (name == "me2d_show_object")
		context_->showItem(-1);

	// Zoom out (full map)
	else if (name == "me2d_show_all")
		context_->renderer().viewFitToMap();

	// Pan view
	else if (name == "me2d_pan_view")
	{
		mouse_down_pos_ = mouse_pos_;
		panning_        = true;
		if (mouse_state_ == MouseState::Normal)
			context_->selection().clearHilight();
		context_->setCursor(ui::MouseCursor::Move);
	}

	// Increment grid
	else if (name == "me2d_grid_inc")
		context_->incrementGrid();

	// Decrement grid
	else if (name == "me2d_grid_dec")
		context_->decrementGrid();
}

// -----------------------------------------------------------------------------
// Handles 2d mode key binds
// -----------------------------------------------------------------------------
void Input::handleKeyBind2d(string_view name)
{
	// --- Line Drawing ---
	if (mouse_state_ == MouseState::LineDraw)
	{
		// Accept line draw
		if (name == "map_edit_accept")
		{
			mouse_state_ = MouseState::Normal;
			context_->lineDraw().end();
			mapeditor::window()->showShapeDrawPanel(false);
		}

		// Cancel line draw
		else if (name == "map_edit_cancel")
		{
			mouse_state_ = MouseState::Normal;
			context_->lineDraw().end(false);
			mapeditor::window()->showShapeDrawPanel(false);
		}
	}

	// --- Paste ---
	else if (mouse_state_ == MouseState::Paste)
	{
		// Accept paste
		if (name == "map_edit_accept")
		{
			mouse_state_ = MouseState::Normal;
			context_->edit2D().paste(mouse_pos_map_);
		}

		// Cancel paste
		else if (name == "map_edit_cancel")
			mouse_state_ = MouseState::Normal;
	}

	// --- Tag edit ---
	else if (mouse_state_ == MouseState::TagSectors)
	{
		// Accept
		if (name == "map_edit_accept")
		{
			mouse_state_ = MouseState::Normal;
			context_->endTagEdit(true);
		}

		// Cancel
		else if (name == "map_edit_cancel")
		{
			mouse_state_ = MouseState::Normal;
			context_->endTagEdit(false);
		}
	}
	else if (mouse_state_ == MouseState::TagThings)
	{
		// Accept
		if (name == "map_edit_accept")
		{
			mouse_state_ = MouseState::Normal;
			context_->endTagEdit(true);
		}

		// Cancel
		else if (name == "map_edit_cancel")
		{
			mouse_state_ = MouseState::Normal;
			context_->endTagEdit(false);
		}
	}

	// --- Moving ---
	else if (mouse_state_ == MouseState::Move)
	{
		// Move toggle
		if (name == "me2d_move")
		{
			context_->moveObjects().end();
			mouse_state_ = MouseState::Normal;
			context_->renderer().forceUpdate(true, false);
		}

		// Accept move
		else if (name == "map_edit_accept")
		{
			context_->moveObjects().end();
			mouse_state_ = MouseState::Normal;
			context_->renderer().forceUpdate(true, false);
		}

		// Cancel move
		else if (name == "map_edit_cancel")
		{
			context_->moveObjects().end(false);
			mouse_state_ = MouseState::Normal;
			context_->renderer().forceUpdate(true, false);
		}
	}

	// --- Object Edit ---
	else if (mouse_state_ == MouseState::ObjectEdit)
	{
		// Accept edit
		if (name == "map_edit_accept")
		{
			context_->objectEdit().end(true);
			mouse_state_ = MouseState::Normal;
			context_->renderer().forceUpdate(true, false);
			context_->setCursor(ui::MouseCursor::Normal);
		}

		// Cancel edit
		else if (name == "map_edit_cancel" || name == "me2d_begin_object_edit")
		{
			context_->objectEdit().end(false);
			mouse_state_ = MouseState::Normal;
			context_->renderer().forceUpdate(true, false);
			context_->setCursor(ui::MouseCursor::Normal);
		}
	}

	// --- Normal mouse state ---
	else if (mouse_state_ == MouseState::Normal)
	{
		// --- All edit modes ---

		// Vertices mode
		if (name == "me2d_mode_vertices")
			context_->setEditMode(Mode::Vertices);

		// Lines mode
		else if (name == "me2d_mode_lines")
			context_->setEditMode(Mode::Lines);

		// Sectors mode
		else if (name == "me2d_mode_sectors")
			context_->setEditMode(Mode::Sectors);

		// Things mode
		else if (name == "me2d_mode_things")
			context_->setEditMode(Mode::Things);

		// 3d mode at mouse cursor
		else if (name == "me2d_mode_3d_at_mouse")
		{
			context_->move3dCameraToCursor();
			context_->setEditMode(Mode::Visual);
		}

		// Cycle flat type
		if (name == "me2d_flat_type")
		{
			flat_drawtype = flat_drawtype + 1;
			if (flat_drawtype > 2)
				flat_drawtype = 0;

			// Editor message and toolbar update
			switch (flat_drawtype)
			{
			case 0:  SAction::fromId("mapw_flat_none")->setChecked(); break;
			case 1:  SAction::fromId("mapw_flat_untextured")->setChecked(); break;
			case 2:  SAction::fromId("mapw_flat_textured")->setChecked(); break;
			default: break;
			}
		}

		// Move items (toggle)
		else if (name == "me2d_move")
		{
			if (context_->moveObjects().begin(mouse_pos_map_))
			{
				mouse_state_ = MouseState::Move;
				context_->renderer().forceUpdate(true, false);
			}
		}

		// Edit items
		else if (name == "me2d_begin_object_edit")
			context_->objectEdit().begin();

		// Split line
		else if (name == "me2d_split_line")
			context_->edit2D().splitLine(
				mouse_pos_map_.x, mouse_pos_map_.y, 16 / context_->renderer().view().scale().x);

		// Begin line drawing
		else if (name == "me2d_begin_linedraw")
			context_->lineDraw().begin();

		// Begin shape drawing
		else if (name == "me2d_begin_shapedraw")
			context_->lineDraw().begin(true);

		// Create object
		else if (name == "me2d_create_object")
		{
			// If in lines mode, begin line drawing
			if (context_->editMode() == Mode::Lines)
			{
				context_->lineDraw().setState(LineDraw::State::Line);
				mouse_state_ = MouseState::LineDraw;
			}
			else
				context_->edit2D().createObject(mouse_pos_map_);
		}

		// Delete object
		else if (name == "me2d_delete_object")
			context_->edit2D().deleteObject();

		// Copy properties
		else if (name == "me2d_copy_properties")
			context_->edit2D().copyProperties();

		// Paste properties
		else if (name == "me2d_paste_properties")
			context_->edit2D().pasteProperties();

		// Paste object(s)
		else if (name == "paste")
		{
			// Check if any data is copied
			ClipboardItem* item = nullptr;
			for (unsigned a = 0; a < app::clipboard().size(); a++)
			{
				if (app::clipboard().item(a)->type() == ClipboardItem::Type::MapArchitecture
					|| app::clipboard().item(a)->type() == ClipboardItem::Type::MapThings)
				{
					item = app::clipboard().item(a);
					break;
				}
			}

			// Begin paste if appropriate data exists
			if (item)
				mouse_state_ = MouseState::Paste;
		}

		// Toggle selection numbers
		else if (name == "me2d_toggle_selection_numbers")
		{
			map_show_selection_numbers = !map_show_selection_numbers;

			if (map_show_selection_numbers)
				context_->addEditorMessage("Selection numbers enabled");
			else
				context_->addEditorMessage("Selection numbers disabled");
		}

		// Mirror
		else if (name == "me2d_mirror_x")
			context_->edit2D().mirror(true);
		else if (name == "me2d_mirror_y")
			context_->edit2D().mirror(false);

		// Object Properties
		else if (name == "me2d_object_properties")
			context_->edit2D().editObjectProperties();


		// --- Lines edit mode ---
		if (context_->editMode() == Mode::Lines)
		{
			// Change line texture
			if (name == "me2d_line_change_texture")
				context_->openLineTextureOverlay();

			// Flip line
			else if (name == "me2d_line_flip")
				context_->edit2D().flipLines();

			// Flip line (no sides)
			else if (name == "me2d_line_flip_nosides")
				context_->edit2D().flipLines(false);

			// Edit line tags
			else if (name == "me2d_line_tag_edit")
			{
				if (context_->beginTagEdit() > 0)
				{
					mouse_state_ = MouseState::TagSectors;

					// Setup help text
					auto key_accept = KeyBind::bind("map_edit_accept").keysAsString();
					auto key_cancel = KeyBind::bind("map_edit_cancel").keysAsString();
					context_->setFeatureHelp(
						{ "Tag Edit",
						  fmt::format("{} = Accept", key_accept),
						  fmt::format("{} = Cancel", key_cancel),
						  "Left Click = Toggle tagged sector" });
				}
			}
		}


		// --- Things edit mode ---
		else if (context_->editMode() == Mode::Things)
		{
			// Change type
			if (name == "me2d_thing_change_type")
				context_->edit2D().changeThingType();

			// Quick angle
			else if (name == "me2d_thing_quick_angle")
			{
				if (mouse_state_ == MouseState::Normal)
				{
					if (context_->selection().hasHilightOrSelection())
						context_->beginUndoRecord("Thing Direction Change", true, false, false);

					mouse_state_ = MouseState::ThingAngle;
				}
			}

			// Rotate Clockwise
			else if (name == "me2d_thing_rotate_clockwise")
			{
				context_->beginUndoRecord("Rotate Things Clockwise", true, false, false);
				auto things  = context_->selection().selectedThings(true);
				bool success = false;
				for (auto& thing : things)
				{
					short angle = thing->angle() - 45;
					if (angle < 0)
						angle += 360;
					thing->setAngle(angle);
					success = true;
				}
				context_->endUndoRecord(success);
			}

			// Rotate Counterclockwise
			else if (name == "me2d_thing_rotate_counterclockwise")
			{
				context_->beginUndoRecord("Rotate Things Counterclockwise", true, false, false);
				auto things  = context_->selection().selectedThings(true);
				bool success = false;
				for (auto& thing : things)
				{
					thing->setAngle((thing->angle() + 45) % 360);
					success = true;
				}
				context_->endUndoRecord(success);
			}
		}


		// --- Sectors edit mode ---
		else if (context_->editMode() == Mode::Sectors)
		{
			// Change sector texture
			if (name == "me2d_sector_change_texture")
				context_->edit2D().changeSectorTexture();
		}
	}
}

// -----------------------------------------------------------------------------
// Handles 3d mode key binds
// -----------------------------------------------------------------------------
void Input::handleKeyBind3d(string_view name) const
{
	// Escape from 3D mode
	if (name == "map_edit_cancel")
		context_->setPrevEditMode();

	// Toggle fog
	else if (name == "me3d_toggle_fog")
	{
		bool fog = context_->renderer().renderer3D().fogEnabled();
		context_->renderer().renderer3D().enableFog(!fog);
		if (fog)
			context_->addEditorMessage("Fog disabled");
		else
			context_->addEditorMessage("Fog enabled");
	}

	// Toggle fullbright
	else if (name == "me3d_toggle_fullbright")
	{
		bool fb = context_->renderer().renderer3D().fullbrightEnabled();
		context_->renderer().renderer3D().enableFullbright(!fb);
		if (fb)
			context_->addEditorMessage("Fullbright disabled");
		else
			context_->addEditorMessage("Fullbright enabled");
	}

	// Adjust brightness
	else if (name == "me3d_adjust_brightness")
	{
		render_3d_brightness = render_3d_brightness + 0.1;
		if (render_3d_brightness > 2.0)
		{
			render_3d_brightness = 1.0;
		}
		context_->addEditorMessage(fmt::format("Brightness set to {:1.1f}", static_cast<double>(render_3d_brightness)));
	}

	// Toggle gravity
	else if (name == "me3d_toggle_gravity")
	{
		camera_3d_gravity = !camera_3d_gravity;
		if (!camera_3d_gravity)
			context_->addEditorMessage("Gravity disabled");
		else
			context_->addEditorMessage("Gravity enabled");
	}

	// Release mouse cursor
	else if (name == "me3d_release_mouse")
		context_->lockMouse(false);

	// Toggle things
	else if (name == "me3d_toggle_things")
	{
		// Change thing display type
		render_3d_things = render_3d_things + 1;
		if (render_3d_things > 2)
			render_3d_things = 0;

		// Editor message
		if (render_3d_things == 0)
			context_->addEditorMessage("Things disabled");
		else if (render_3d_things == 1)
			context_->addEditorMessage("Things enabled: All");
		else
			context_->addEditorMessage("Things enabled: Decorations only");
	}

	// Change thing render style
	else if (name == "me3d_thing_style")
	{
		// Change thing display style
		render_3d_things_style = render_3d_things_style + 1;
		if (render_3d_things_style > 2)
			render_3d_things_style = 0;

		// Editor message
		if (render_3d_things_style == 0)
			context_->addEditorMessage("Thing render style: Sprites only");
		else if (render_3d_things_style == 1)
			context_->addEditorMessage("Thing render style: Sprites + Ground boxes");
		else
			context_->addEditorMessage("Thing render style: Sprites + Full boxes");
	}

	// Toggle hilight
	else if (name == "me3d_toggle_hilight")
	{
		// Change hilight type
		render_3d_hilight = render_3d_hilight + 1;
		if (render_3d_hilight > 2)
			render_3d_hilight = 0;

		// Editor message
		if (render_3d_hilight == 0)
			context_->addEditorMessage("Hilight disabled");
		else if (render_3d_hilight == 1)
			context_->addEditorMessage("Hilight enabled: Outline");
		else if (render_3d_hilight == 2)
			context_->addEditorMessage("Hilight enabled: Solid");
	}

	// Toggle info overlay
	else if (name == "me3d_toggle_info")
		info_overlay_3d = !info_overlay_3d;

	// Quick texture
	else if (name == "me3d_quick_texture")
		context_->openQuickTextureOverlay();

	// Send to map editor
	else
		context_->handleKeyBind(name, mouse_pos_map_);
}

// -----------------------------------------------------------------------------
// Updates the 3d mode camera depending on what keybinds are currently pressed
// -----------------------------------------------------------------------------
bool Input::updateCamera3d(double mult) const
{
	// --- Check for held-down keys ---
	bool   moving = false;
	double speed  = shift_down_ ? mult * 8 : mult * 4;
	auto&  camera = context_->renderer().camera();
	auto   sector = context_->map().sectors().atPos({ camera.position().x, camera.position().y });

	// Camera forward
	if (KeyBind::isPressed("me3d_camera_forward"))
	{
		camera.move(speed, !camera_3d_gravity || !sector);
		moving = true;
	}

	// Camera backward
	if (KeyBind::isPressed("me3d_camera_back"))
	{
		camera.move(-speed, !camera_3d_gravity || !sector);
		moving = true;
	}

	// Camera left (strafe)
	if (KeyBind::isPressed("me3d_camera_left"))
	{
		camera.strafe(-speed);
		moving = true;
	}

	// Camera right (strafe)
	if (KeyBind::isPressed("me3d_camera_right"))
	{
		camera.strafe(speed);
		moving = true;
	}

	// Camera up
	if (KeyBind::isPressed("me3d_camera_up"))
	{
		camera.moveUp(speed);
		moving = true;
	}

	// Camera down
	if (KeyBind::isPressed("me3d_camera_down"))
	{
		camera.moveUp(-speed);
		moving = true;
	}

	// Camera turn left
	if (KeyBind::isPressed("me3d_camera_turn_left"))
	{
		camera.turn(shift_down_ ? mult * 2 : mult);
		moving = true;
	}

	// Camera turn right
	if (KeyBind::isPressed("me3d_camera_turn_right"))
	{
		camera.turn(shift_down_ ? -mult * 2 : -mult);
		moving = true;
	}

	// Apply gravity to camera if needed
	if (camera_3d_gravity && sector)
	{
		auto height = context_->map().mapSpecials().sectorFloorHeightAt(*sector, camera.position());
		if (camera.applyGravity(height, game::configuration().playerEyeHeight(), mult))
			moving = true;
	}

	return moving;
}

// -----------------------------------------------------------------------------
// Returns the KeyBind name for the given mouse [button]
// -----------------------------------------------------------------------------
string Input::mouseButtonKBName(MouseButton button)
{
	switch (button)
	{
	case Left:   return "mouse1";
	case Right:  return "mouse2";
	case Middle: return "mouse3";
	case Mouse4: return "mouse4";
	case Mouse5: return "mouse5";
	default:     return fmt::format("mouse{}", static_cast<int>(button));
	}
}
