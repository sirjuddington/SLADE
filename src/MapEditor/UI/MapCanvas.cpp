
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MapCanvas.cpp
 * Description: MapCanvas class, the OpenGL canvas widget that the
 *              2d/3d map view is drawn on
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
#include "General/Clipboard.h"
#include "MapCanvas.h"
#include "MapEditor/Renderer/MapRenderer2D.h"
#include "MapEditor/Renderer/MCAnimations.h"
#include "MapEditor/Renderer/Overlays/MCOverlay.h"
#include "MapEditor/SectorBuilder.h"
#include "MapEditor/UI/Dialogs/ActionSpecialDialog.h"
#include "MapEditor/UI/Dialogs/SectorSpecialDialog.h"
#include "MapEditor/UI/Dialogs/ShowItemDialog.h"
#include "MapEditor/UI/MapEditorWindow.h"
#include "ObjectEditPanel.h"
#include "OpenGL/Drawing.h"
#include "Utility/MathStuff.h"

#undef None

using MapEditor::Mode;
using MapEditor::SectorMode;
using namespace MapEditor;


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Bool, selection_clear_click, false, CVAR_SAVE)
CVAR(Bool, property_edit_dclick, true, CVAR_SAVE)
CVAR(Bool, mlook_invert_y, false, CVAR_SAVE)
CVAR(Float, camera_3d_sensitivity_x, 1.0f, CVAR_SAVE)
CVAR(Float, camera_3d_sensitivity_y, 1.0f, CVAR_SAVE)

// for testing
PolygonSplitter splitter;
SectorBuilder sbuilder;


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Int, vertex_size)
EXTERN_CVAR(Bool, vertex_round)
EXTERN_CVAR(Float, render_max_dist)
EXTERN_CVAR(Int, render_3d_things)
EXTERN_CVAR(Int, render_3d_things_style)
EXTERN_CVAR(Int, render_3d_hilight)
EXTERN_CVAR(Bool, map_animate_hilight)
EXTERN_CVAR(Float, render_3d_brightness)


/* MapCanvas::MapCanvas
 * MapCanvas class constructor
 *******************************************************************/
MapCanvas::MapCanvas(wxWindow* parent, int id, MapEditContext* editor)
	: OGLCanvas(parent, id, false)
{
	// Init variables
	this->editor = editor;
	editor->setCanvas(this);
	fr_idle = 0;
	last_time = 0;
	mouse_selbegin = false;
	mouse_movebegin = false;
	mouse_locked = false;
	mouse_warp = false;

#ifdef USE_SFML_RENDERWINDOW
	setVerticalSyncEnabled(false);
#endif

	// Bind Events
	Bind(wxEVT_SIZE, &MapCanvas::onSize, this);
	Bind(wxEVT_KEY_DOWN, &MapCanvas::onKeyDown, this);
	Bind(wxEVT_KEY_UP, &MapCanvas::onKeyUp, this);
	Bind(wxEVT_LEFT_DOWN, &MapCanvas::onMouseDown, this);
	Bind(wxEVT_LEFT_DCLICK, &MapCanvas::onMouseDown, this);
	Bind(wxEVT_RIGHT_DOWN, &MapCanvas::onMouseDown, this);
	Bind(wxEVT_MIDDLE_DOWN, &MapCanvas::onMouseDown, this);
	Bind(wxEVT_AUX1_DOWN, &MapCanvas::onMouseDown, this);
	Bind(wxEVT_AUX2_DOWN, &MapCanvas::onMouseDown, this);
	Bind(wxEVT_LEFT_UP, &MapCanvas::onMouseUp, this);
	Bind(wxEVT_RIGHT_UP, &MapCanvas::onMouseUp, this);
	Bind(wxEVT_MIDDLE_UP, &MapCanvas::onMouseUp, this);
	Bind(wxEVT_AUX1_UP, &MapCanvas::onMouseUp, this);
	Bind(wxEVT_AUX2_UP, &MapCanvas::onMouseUp, this);
	Bind(wxEVT_MOTION, &MapCanvas::onMouseMotion, this);
	Bind(wxEVT_MOUSEWHEEL, &MapCanvas::onMouseWheel, this);
	Bind(wxEVT_LEAVE_WINDOW, &MapCanvas::onMouseLeave, this);
	Bind(wxEVT_ENTER_WINDOW, &MapCanvas::onMouseEnter, this);
	Bind(wxEVT_SET_FOCUS, &MapCanvas::onFocus, this);
	Bind(wxEVT_KILL_FOCUS, &MapCanvas::onFocus, this);
	Bind(wxEVT_TIMER, &MapCanvas::onRTimer, this);
#ifdef USE_SFML_RENDERWINDOW
	Bind(wxEVT_IDLE, &MapCanvas::onIdle, this);
#endif

	timer.Start(10, true);
}

/* MapCanvas::~MapCanvas
 * MapCanvas class destructor
 *******************************************************************/
MapCanvas::~MapCanvas()
{
}

/* MapCanvas::draw
 * Draw the current map (2d or 3d) and any overlays etc
 *******************************************************************/
void MapCanvas::draw()
{
	if (!IsEnabled())
		return;

	editor->renderer().draw();

	SwapBuffers();

	glFinish();
}

/* MapCanvas::mouseToCenter
 * Moves the mouse cursor to the center of the canvas
 *******************************************************************/
void MapCanvas::mouseToCenter()
{
	wxRect rect = GetScreenRect();
	mouse_warp = true;
	sf::Mouse::setPosition(sf::Vector2i(rect.x + int(rect.width*0.5), rect.y + int(rect.height*0.5)));
}

/* MapCanvas::lockMouse
 * Locks/unlocks the mouse cursor. A locked cursor is invisible and
 * will be moved to the center of the canvas every frame
 *******************************************************************/
void MapCanvas::lockMouse(bool lock)
{
	mouse_locked = lock;
	if (lock)
	{
		// Center mouse
		mouseToCenter();

		// Hide cursor
		wxImage img(32, 32, true);
		img.SetMask(true);
		img.SetMaskColour(0, 0, 0);
		SetCursor(wxCursor(img));
#ifdef USE_SFML_RENDERWINDOW
		setMouseCursorVisible(false);
#endif
	}
	else
	{
		// Show cursor
		SetCursor(wxNullCursor);
#ifdef USE_SFML_RENDERWINDOW
		setMouseCursorVisible(true);
#endif
	}
}

/* MapCanvas::mouseLook3d
 * Handles 3d mode mouselook
 *******************************************************************/
void MapCanvas::mouseLook3d()
{
	// Check for 3d mode
	if (editor->editMode() == Mode::Visual && mouse_locked)
	{
		auto overlay_current = editor->currentOverlay();
		if (!overlay_current || !overlay_current->isActive() || (overlay_current && overlay_current->allow3dMlook()))
		{
			// Get relative mouse movement
			int xpos = wxGetMousePosition().x - GetScreenPosition().x;
			int ypos = wxGetMousePosition().y - GetScreenPosition().y;
			double xrel = xpos - int(GetSize().x * 0.5);
			double yrel = ypos - int(GetSize().y * 0.5);

			if (xrel != 0 || yrel != 0)
			{
				editor->renderer().renderer3D().cameraTurn(-xrel*0.1*camera_3d_sensitivity_x);
				if (mlook_invert_y)
					editor->renderer().renderer3D().cameraPitch(yrel*0.003*camera_3d_sensitivity_y);
				else
					editor->renderer().renderer3D().cameraPitch(-yrel*0.003*camera_3d_sensitivity_y);

				mouseToCenter();
				fr_idle = 0;
			}
		}
	}
}

/* MapCanvas::onKeyBindPress
 * Called when the key bind [name] is pressed
 *******************************************************************/
void MapCanvas::onKeyBindPress(string name)
{
	// Screenshot
#ifdef USE_SFML_RENDERWINDOW
	if (name == "map_screenshot")
	{
		// Capture shot
		sf::Image shot = capture();

		// Remove alpha
		// sf::Image kinda sucks, shouldn't have to do it like this
		for (unsigned x = 0; x < shot.getSize().x; x++)
		{
			for (unsigned y = 0; y < shot.getSize().y; y++)
			{
				sf::Color col = shot.getPixel(x, y);
				shot.setPixel(x, y, sf::Color(col.r, col.g, col.b, 255));
			}
		}

		// Save to file
		wxDateTime date;
		date.SetToCurrent();
		string timestamp = date.FormatISOCombined('-');
		timestamp.Replace(":", "");
		string filename = App::path(S_FMT("sladeshot-%s.png", timestamp), App::Dir::User);
		if (shot.saveToFile(UTF8(filename)))
		{
			// Editor message if the file is actually written, with full path
			editor->addEditorMessage(S_FMT("Screenshot taken (%s)", filename));
		}
		else
		{
			// Editor message also if the file couldn't be written
			editor->addEditorMessage(S_FMT("Screenshot failed (%s)", filename));
		}

	}
#endif

	// Handle keybinds depending on mode
	if (editor->editMode() == Mode::Visual)
	{
		// Release mouse cursor
		if (name == "me3d_release_mouse")
			lockMouse(false);
	}
}


/*******************************************************************
 * MAPCANVAS CLASS EVENTS
 *******************************************************************/

/* MapCanvas::onSize
 * Called when the canvas is resized
 *******************************************************************/
void MapCanvas::onSize(wxSizeEvent& e)
{
	// Update screen limits
	editor->renderer().setViewSize(GetSize().x, GetSize().y);

	e.Skip();
}

/* MapCanvas::onKeyDown
 * Called when a key is pressed within the canvas
 *******************************************************************/
void MapCanvas::onKeyDown(wxKeyEvent& e)
{
	// Set current modifiers
	editor->input().updateKeyModifiersWx(e.GetModifiers());

	// Send to overlay if active
	if (editor->overlayActive())
		editor->currentOverlay()->keyDown(KeyBind::keyName(e.GetKeyCode()));

	// Let keybind system handle it
	KeyBind::keyPressed(KeyBind::asKeyPress(e.GetKeyCode(), e.GetModifiers()));

	// Testing
	if (Global::debug)
	{
		if (e.GetKeyCode() == WXK_F6)
		{
			Polygon2D poly;
			sf::Clock clock;
			LOG_MESSAGE(1, "Generating polygons...");
			for (unsigned a = 0; a < editor->map().nSectors(); a++)
			{
				if (!poly.openSector(editor->map().getSector(a)))
					LOG_MESSAGE(1, "Splitting failed for sector %d", a);
			}
			//int ms = clock.GetElapsedTime() * 1000;
			//LOG_MESSAGE(1, "Polygon generation took %dms", ms);
		}
		if (e.GetKeyCode() == WXK_F7)
		{
			// Get nearest line
			int nearest = editor->map().nearestLine(editor->input().mousePosMap(), 999999);
			MapLine* line = editor->map().getLine(nearest);
			if (line)
			{
				// Determine line side
				double side = MathStuff::lineSide(editor->input().mousePosMap(), line->seg());
				if (side >= 0)
					sbuilder.traceSector(&(editor->map()), line, true);
				else
					sbuilder.traceSector(&(editor->map()), line, false);
			}
		}
		if (e.GetKeyCode() == WXK_F5)
		{
			// Get nearest line
			int nearest = editor->map().nearestLine(editor->input().mousePosMap(), 999999);
			MapLine* line = editor->map().getLine(nearest);

			// Get sectors
			MapSector* sec1 = editor->map().getLineSideSector(line, true);
			MapSector* sec2 = editor->map().getLineSideSector(line, false);
			int i1 = -1;
			int i2 = -1;
			if (sec1) i1 = sec1->getIndex();
			if (sec2) i2 = sec2->getIndex();

			editor->addEditorMessage(S_FMT("Front %d Back %d", i1, i2));
		}
		if (e.GetKeyCode() == WXK_F5 && editor->editMode() == Mode::Sectors)
		{
			splitter.setVerbose(true);
			splitter.clear();
			splitter.openSector(editor->selection().hilightedSector());
			Polygon2D temp;
			splitter.doSplitting(&temp);
		}
	}

	// Update cursor in object edit mode
	//if (mouse_state == Input::MouseState::ObjectEdit)
	//	determineObjectEditState();

#ifndef __WXMAC__
	// Skipping events on OS X doesn't do anything but causes
	// sound alert (a.k.a. error beep) on every key press
	if (e.GetKeyCode() != WXK_UP &&
		e.GetKeyCode() != WXK_DOWN &&
		e.GetKeyCode() != WXK_LEFT &&
		e.GetKeyCode() != WXK_RIGHT &&
		e.GetKeyCode() != WXK_NUMPAD_UP &&
		e.GetKeyCode() != WXK_NUMPAD_DOWN &&
		e.GetKeyCode() != WXK_NUMPAD_LEFT &&
		e.GetKeyCode() != WXK_NUMPAD_RIGHT)
		e.Skip();
#endif // !__WXMAC__
}

/* MapCanvas::onKeyUp
 * Called when a key is released within the canvas
 *******************************************************************/
void MapCanvas::onKeyUp(wxKeyEvent& e)
{
	// Set current modifiers
	editor->input().updateKeyModifiersWx(e.GetModifiers());

	// Let keybind system handle it
	KeyBind::keyReleased(KeyBind::keyName(e.GetKeyCode()));

	e.Skip();
}

/* MapCanvas::onMouseDown
 * Called when a mouse button is pressed within the canvas
 *******************************************************************/
void MapCanvas::onMouseDown(wxMouseEvent& e)
{
	auto mouse_state = editor->input().mouseState();

	// Update hilight
	if (mouse_state == Input::MouseState::Normal)
		editor->selection().updateHilight(editor->input().mousePosMap(), editor->renderer().view().scale());

	// Update mouse variables
	editor->input().mouseDown();
	//mouse_downpos.set(e.GetX(), e.GetY());
	//mouse_downpos_m.set(editor->renderer().translateX(e.GetX()), editor->renderer().translateY(e.GetY()));

	// Check if a full screen overlay is active
	if (editor->overlayActive())
	{
		// Left click
		if (e.LeftDown())
			editor->currentOverlay()->mouseLeftClick();

		// Right click
		else if (e.RightDown())
			editor->currentOverlay()->mouseRightClick();

		return;
	}

	// Left button
	if (e.LeftDown() || e.LeftDClick())
	{
		// 3d mode
		if (editor->editMode() == Mode::Visual)
		{
			// If the mouse is unlocked, lock the mouse
			if (!mouse_locked)
			{
				mouseToCenter();
				lockMouse(true);
			}
			else
			{
				if (e.ShiftDown())	// Shift down, select all matching adjacent structures
					editor->edit3D().selectAdjacent(editor->hilightItem());
				else	// Toggle selection
					editor->selection().toggleCurrent();
			}

			return;
		}

		// Line drawing state, add line draw point
		if (mouse_state == Input::MouseState::LineDraw)
		{
			// Snap point to nearest vertex if shift is held down
			bool nearest_vertex = false;
			if (e.GetModifiers() & wxMOD_SHIFT)
				nearest_vertex = true;

			// Line drawing
			if (editor->lineDraw().state() == LineDraw::State::Line)
			{
				if (editor->lineDraw().addPoint(editor->input().mouseDownPosMap(), nearest_vertex))
				{
					// If line drawing finished, revert to normal state
					editor->input().setMouseState(Input::MouseState::Normal);
				}
			}

			// Shape drawing
			else
			{
				if (editor->lineDraw().state() == LineDraw::State::ShapeOrigin)
				{
					// Set shape origin
					editor->lineDraw().setShapeOrigin(editor->input().mouseDownPosMap(), nearest_vertex);
					editor->lineDraw().setState(LineDraw::State::ShapeEdge);
				}
				else
				{
					// Finish shape draw
					editor->lineDraw().end(true);
					MapEditor::window()->showShapeDrawPanel(false);
					editor->input().setMouseState(Input::MouseState::Normal);
				}
			}
		}

		// Paste state, accept paste
		else if (mouse_state == Input::MouseState::Paste)
		{
			editor->edit2D().paste(editor->input().mousePosMap());
			if (!e.ShiftDown())
				editor->input().setMouseState(Input::MouseState::Normal);
		}

		// Sector tagging state
		else if (mouse_state == Input::MouseState::TagSectors)
		{
			editor->tagSectorAt(editor->input().mousePosMap().x, editor->input().mousePosMap().y);
		}

		else if (mouse_state == Input::MouseState::Normal)
		{
			// Double click to edit the current selection
			if (e.LeftDClick() && property_edit_dclick)
			{
				editor->edit2D().editObjectProperties();
				if (editor->selection().size() == 1)
					editor->selection().clear();
			}
			// Begin box selection if shift is held down, otherwise toggle selection on hilighted object
			else if (e.ShiftDown())
				editor->input().setMouseState(Input::MouseState::Selection);
			else
				mouse_selbegin = !editor->selection().toggleCurrent(selection_clear_click);
		}
	}

	// Right button
	else if (e.RightDown())
	{
		// 3d mode
		if (editor->editMode() == Mode::Visual)
		{
			// Get selection or hilight
			auto sel = editor->selection().selectionOrHilight();
			if (!sel.empty())
			{
				// Check type
				if (sel[0].type == MapEditor::ItemType::Thing)
					editor->edit2D().changeThingType();
				else
					editor->edit3D().changeTexture();
			}
		}

		// Remove line draw point if in line drawing state
		if (mouse_state == Input::MouseState::LineDraw)
		{
			// Line drawing
			if (editor->lineDraw().state() == LineDraw::State::Line)
				editor->lineDraw().removePoint();

			// Shape drawing
			else if (editor->lineDraw().state() == LineDraw::State::ShapeEdge)
			{
				editor->lineDraw().end(false);
				editor->lineDraw().setState(LineDraw::State::ShapeOrigin);
			}
		}

		// Normal state
		else if (mouse_state == Input::MouseState::Normal)
		{
			// Begin move if something is selected/hilighted
			if (editor->selection().hasHilightOrSelection())
				mouse_movebegin = true;
			//else if (editor->editMode() == Mode::Vertices)
			//	editor->splitLine(editor->input().mousePosMap().x, editor->input().mousePosMap().y, 16/editor->renderer().viewScale());
		}
	}

	// Any other mouse button (let keybind system handle it)
	else// if (mouse_state == Input::MouseState::Normal)
		KeyBind::keyPressed(keypress_t(KeyBind::mbName(e.GetButton()), e.AltDown(), e.CmdDown(), e.ShiftDown()));

	// Set focus
	SetFocus();

	e.Skip();
}

/* MapCanvas::onMouseUp
 * Called when a mouse button is released within the canvas
 *******************************************************************/
void MapCanvas::onMouseUp(wxMouseEvent& e)
{
	auto mouse_state = editor->input().mouseState();
	auto mouse_downpos_m = editor->input().mouseDownPosMap();

	// Clear mouse down position
	editor->input().mouseUp();

	// Check if a full screen overlay is active
	if (editor->overlayActive())
	{
		return;
	}

	// Left button
	if (e.LeftUp())
	{
		mouse_selbegin = false;

		// If we're ending a box selection
		if (mouse_state == Input::MouseState::Selection)
		{
			// Reset mouse state
			editor->input().setMouseState(Input::MouseState::Normal);

			// Select
			editor->selection().selectWithin(
				frect_t(
					min(mouse_downpos_m.x, editor->input().mousePosMap().x),
					min(mouse_downpos_m.y, editor->input().mousePosMap().y),
					max(mouse_downpos_m.x, editor->input().mousePosMap().x),
					max(mouse_downpos_m.y, editor->input().mousePosMap().y)),
				e.ShiftDown()
			);

			// Begin selection box fade animation
			editor->renderer().addAnimation(
				std::make_unique<MCASelboxFader>(
					App::runTimer(),
					mouse_downpos_m,
					editor->input().mousePosMap()
			));
		}

		// If we're in object edit mode
		if (mouse_state == Input::MouseState::ObjectEdit)
			editor->objectEdit().group().resetPositions();
	}

	// Right button
	else if (e.RightUp())
	{
		mouse_movebegin = false;

		if (mouse_state == Input::MouseState::Move)
		{
			editor->moveObjects().end();
			editor->input().setMouseState(Input::MouseState::Normal);
			editor->renderer().renderer2D().forceUpdate();
		}

		// Paste state, cancel paste
		else if (mouse_state == Input::MouseState::Paste)
		{
			editor->input().setMouseState(Input::MouseState::Normal);
		}

		else if (mouse_state == Input::MouseState::Normal)
		{
			// Context menu
			wxMenu menu_context;

			// Set 3d camera
			SAction::fromId("mapw_camera_set")->addToMenu(&menu_context, true);

			// Run from here
			SAction::fromId("mapw_run_map_here")->addToMenu(&menu_context, true);

			// Mode-specific
			bool object_selected = editor->selection().hasHilightOrSelection();
			if (editor->editMode() == Mode::Vertices)
			{
				menu_context.AppendSeparator();
				SAction::fromId("mapw_vertex_create")->addToMenu(&menu_context, true);
			}
			else if (editor->editMode() == Mode::Lines)
			{
				if (object_selected)
				{
					menu_context.AppendSeparator();
					SAction::fromId("mapw_line_changetexture")->addToMenu(&menu_context, true);
					SAction::fromId("mapw_line_changespecial")->addToMenu(&menu_context, true);
					SAction::fromId("mapw_line_tagedit")->addToMenu(&menu_context, true);
					SAction::fromId("mapw_line_flip")->addToMenu(&menu_context, true);
					SAction::fromId("mapw_line_correctsectors")->addToMenu(&menu_context, true);
				}
			}
			else if (editor->editMode() == Mode::Things)
			{
				menu_context.AppendSeparator();

				if (object_selected)
					SAction::fromId("mapw_thing_changetype")->addToMenu(&menu_context, true);

				SAction::fromId("mapw_thing_create")->addToMenu(&menu_context, true);
			}
			else if (editor->editMode() == Mode::Sectors)
			{
				if (object_selected)
				{
					SAction::fromId("mapw_sector_changetexture")->addToMenu(&menu_context, true);
					SAction::fromId("mapw_sector_changespecial")->addToMenu(&menu_context, true);
					if (editor->selection().size() > 1)
					{
						SAction::fromId("mapw_sector_join")->addToMenu(&menu_context, true);
						SAction::fromId("mapw_sector_join_keep")->addToMenu(&menu_context, true);
					}
				}

				SAction::fromId("mapw_sector_create")->addToMenu(&menu_context, true);
			}

			if (object_selected)
			{
				// General edit
				menu_context.AppendSeparator();
				SAction::fromId("mapw_edit_objects")->addToMenu(&menu_context, true);
				SAction::fromId("mapw_mirror_x")->addToMenu(&menu_context, true);
				SAction::fromId("mapw_mirror_y")->addToMenu(&menu_context, true);

				// Properties
				menu_context.AppendSeparator();
				SAction::fromId("mapw_item_properties")->addToMenu(&menu_context, true);
			}

			PopupMenu(&menu_context);
		}
	}

	// Any other mouse button (let keybind system handle it)
	else if (mouse_state != Input::MouseState::Selection)
		KeyBind::keyReleased(KeyBind::mbName(e.GetButton()));

	e.Skip();
}

/* MapCanvas::onMouseMotion
 * Called when the mouse cursor is moved within the canvas
 *******************************************************************/
void MapCanvas::onMouseMotion(wxMouseEvent& e)
{
	auto mouse_pos = editor->input().mousePos();
	auto mouse_downpos = editor->input().mouseDownPos();
	auto mouse_downpos_m = editor->input().mouseDownPosMap();

	// Ignore if it was generated by a mouse pointer warp
	if (mouse_warp)
	{
		mouse_warp = false;
		e.Skip();
		return;
	}

	// Check if a full screen overlay is active
	if (editor->overlayActive())
	{
		editor->currentOverlay()->mouseMotion(e.GetX(), e.GetY());
		return;
	}

	// panning
	if (editor->input().panning())
		editor->renderer().pan(mouse_pos.x - e.GetX(), -(mouse_pos.y - e.GetY()), true);

	// Update mouse variables
	editor->input().mouseMove(e.GetX(), e.GetY());
	mouse_pos = editor->input().mousePos();
	//mouse_pos.set(e.GetX(), e.GetY());
	//editor->input().mousePosMap().set(editor->renderer().translateX(e.GetX()), editor->renderer().translateY(e.GetY()));

	// Update coordinates on status bar
	double mx = editor->input().mousePosMap().x;
	double my = editor->input().mousePosMap().y;
	if (editor->gridSnap())
	{
		mx = editor->snapToGrid(mx);
		my = editor->snapToGrid(my);
	}
	string status_text;
	if (MapEditor::editContext().mapDesc().format == MAP_UDMF)
		status_text = S_FMT("Position: (%1.3f, %1.3f)", mx, my);
	else
		status_text = S_FMT("Position: (%d, %d)", (int)mx, (int)my);
	MapEditor::window()->CallAfter(&MapEditorWindow::SetStatusText, status_text, 3);

	// Object edit
	auto edit_state = editor->objectEdit().state();
	if (editor->input().mouseState() == Input::MouseState::ObjectEdit)
	{
		// Do dragging if left mouse is down
		if (e.LeftIsDown() && edit_state != ObjectEdit::State::None)
		{
			if (editor->objectEdit().rotating())
			{
				// Rotate
				editor->objectEdit().group().doRotate(mouse_downpos_m, editor->input().mousePosMap(), !e.ShiftDown());
				MapEditor::window()->objectEditPanel()->update(&editor->objectEdit().group(), true);
			}
			else
			{
				// Get dragged offsets
				double xoff = editor->input().mousePosMap().x - mouse_downpos_m.x;
				double yoff = editor->input().mousePosMap().y - mouse_downpos_m.y;

				// Snap to grid if shift not held down
				if (!e.ShiftDown())
				{
					xoff = editor->snapToGrid(xoff);
					yoff = editor->snapToGrid(yoff);
				}

				if (edit_state == ObjectEdit::State::Move)
				{
					// Move objects
					editor->objectEdit().group().doMove(xoff, yoff);
					MapEditor::window()->objectEditPanel()->update(&editor->objectEdit().group());
				}
				else
				{
					// Scale objects
					editor->objectEdit().group().doScale(
						xoff,
						yoff,
						editor->objectEdit().stateLeft(false),
						editor->objectEdit().stateTop(false),
						editor->objectEdit().stateRight(false),
						editor->objectEdit().stateBottom(false)
					);
					MapEditor::window()->objectEditPanel()->update(&editor->objectEdit().group());
				}
			}
		}
		else
			editor->objectEdit().determineState();

		return;
	}

	// Check if we want to start a selection box
	if (mouse_selbegin && fpoint2_t(mouse_pos.x - mouse_downpos.x, mouse_pos.y - mouse_downpos.y).magnitude() > 16)
		editor->input().setMouseState(Input::MouseState::Selection);

	// Check if we want to start moving
	if (mouse_movebegin && fpoint2_t(mouse_pos.x - mouse_downpos.x, mouse_pos.y - mouse_downpos.y).magnitude() > 4)
	{
		mouse_movebegin = false;
		editor->moveObjects().begin(mouse_downpos_m);
		editor->input().setMouseState(Input::MouseState::Move);
		editor->renderer().renderer2D().forceUpdate();
	}

	// Check if we are in thing quick angle state
	if (editor->input().mouseState() == Input::MouseState::ThingAngle)
		editor->edit2D().thingQuickAngle(editor->input().mousePosMap());

	// Update shape drawing if needed
	if (editor->input().mouseState() == Input::MouseState::LineDraw && editor->lineDraw().state() == LineDraw::State::ShapeEdge)
		editor->lineDraw().updateShape(editor->input().mousePosMap());

	e.Skip();
}

/* MapCanvas::onMouseWheel
 * Called when the mouse wheel is moved
 *******************************************************************/
void MapCanvas::onMouseWheel(wxMouseEvent& e)
{
#ifdef __WXOSX__
	mwheel_rotation = (double)e.GetWheelRotation() / (double)e.GetWheelDelta();
	if (mwheel_rotation < 0)
		mwheel_rotation = 0 - mwheel_rotation;
#else
	mwheel_rotation = 1;
#endif

	if (mwheel_rotation < 0.001)
		return;

	editor->input().mouseWheel(e.GetWheelRotation() > 0, mwheel_rotation);
}

/* MapCanvas::onMouseLeave
 * Called when the mouse cursor leaves the canvas
 *******************************************************************/
void MapCanvas::onMouseLeave(wxMouseEvent& e)
{
	// Stop panning
	if (editor->input().panning())
	{
		editor->input().setPanning(false);
		SetCursor(wxNullCursor);
	}

	e.Skip();
}

/* MapCanvas::onMouseEnter
 * Called when the mouse cursor enters the canvas
 *******************************************************************/
void MapCanvas::onMouseEnter(wxMouseEvent& e)
{
	// Set focus
	//SetFocus();

	e.Skip();
}

/* MapCanvas::onIdle
 * Called when the canvas is idle
 *******************************************************************/
void MapCanvas::onIdle(wxIdleEvent& e)
{
	// Handle 3d mode mouselook
	mouseLook3d();

	// Get time since last redraw
	long frametime = (sfclock.getElapsedTime().asMilliseconds()) - last_time;

	if (editor->update(frametime))
	{
		last_time = (sfclock.getElapsedTime().asMilliseconds());
		Refresh();
	}
}

/* MapCanvas::onRTimer
 * Called when the canvas timer is triggered
 *******************************************************************/
void MapCanvas::onRTimer(wxTimerEvent& e)
{
	// Handle 3d mode mouselook
	mouseLook3d();

	// Get time since last redraw
	long frametime = (sfclock.getElapsedTime().asMilliseconds()) - last_time;

	if (editor->update(frametime))
	{
		last_time = (sfclock.getElapsedTime().asMilliseconds());
		Refresh();
	}

	timer.Start(-1, true);
}

/* MapCanvas::onFocus
 * Called when the canvas loses or gains focus
 *******************************************************************/
void MapCanvas::onFocus(wxFocusEvent& e)
{
	if (e.GetEventType() == wxEVT_SET_FOCUS)
	{
		if (editor->editMode() == Mode::Visual)
			lockMouse(true);
	}
	else if (e.GetEventType() == wxEVT_KILL_FOCUS)
		lockMouse(false);
}
