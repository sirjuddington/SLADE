
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
#include "MapCanvas.h"
#include "MapEditor/Renderer/Overlays/MCOverlay.h"
#include "MapEditor/SectorBuilder.h"
#include "OpenGL/Drawing.h"
#include "Utility/MathStuff.h"

using MapEditor::Mode;


/*******************************************************************
 * MAPCANVAS CLASS FUNCTIONS
 *******************************************************************/

/* MapCanvas::MapCanvas
 * MapCanvas class constructor
 *******************************************************************/
MapCanvas::MapCanvas(wxWindow* parent, int id, MapEditContext* context) :
	OGLCanvas{ parent, id, false },
	context_{ context }
{
	// Init variables
	context_->setCanvas(this);
	last_time = 0;

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

	context_->renderer().draw();

	SwapBuffers();

	glFinish();
}

/* MapCanvas::mouseToCenter
 * Moves the mouse cursor to the center of the canvas
 *******************************************************************/
void MapCanvas::mouseToCenter()
{
	wxRect rect = GetScreenRect();
	mouse_warp_ = true;
	sf::Mouse::setPosition(sf::Vector2i(rect.x + int(rect.width*0.5), rect.y + int(rect.height*0.5)));
}

/* MapCanvas::lockMouse
 * Locks/unlocks the mouse cursor. A locked cursor is invisible and
 * will be moved to the center of the canvas every frame
 *******************************************************************/
void MapCanvas::lockMouse(bool lock)
{
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
	if (context_->editMode() == Mode::Visual && context_->mouseLocked())
	{
		auto overlay_current = context_->currentOverlay();
		if (!overlay_current || !overlay_current->isActive() || (overlay_current && overlay_current->allow3dMlook()))
		{
			// Get relative mouse movement
			int xpos = wxGetMousePosition().x - GetScreenPosition().x;
			int ypos = wxGetMousePosition().y - GetScreenPosition().y;
			double xrel = xpos - int(GetSize().x * 0.5);
			double yrel = ypos - int(GetSize().y * 0.5);

			if (xrel != 0 || yrel != 0)
			{
				context_->renderer().renderer3D().cameraLook(xrel, yrel);
				mouseToCenter();
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
			context_->addEditorMessage(S_FMT("Screenshot taken (%s)", filename));
		}
		else
		{
			// Editor message also if the file couldn't be written
			context_->addEditorMessage(S_FMT("Screenshot failed (%s)", filename));
		}

	}
#endif
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
	context_->renderer().setViewSize(GetSize().x, GetSize().y);

	e.Skip();
}

/* MapCanvas::onKeyDown
 * Called when a key is pressed within the canvas
 *******************************************************************/
void MapCanvas::onKeyDown(wxKeyEvent& e)
{
	// Send to editor
	context_->input().updateKeyModifiersWx(e.GetModifiers());
	context_->input().keyDown(KeyBind::keyName(e.GetKeyCode()));

	// Testing
	if (Global::debug)
	{
		if (e.GetKeyCode() == WXK_F6)
		{
			Polygon2D poly;
			sf::Clock clock;
			LOG_MESSAGE(1, "Generating polygons...");
			for (unsigned a = 0; a < context_->map().nSectors(); a++)
			{
				if (!poly.openSector(context_->map().getSector(a)))
					LOG_MESSAGE(1, "Splitting failed for sector %d", a);
			}
			//int ms = clock.GetElapsedTime() * 1000;
			//LOG_MESSAGE(1, "Polygon generation took %dms", ms);
		}
		if (e.GetKeyCode() == WXK_F7)
		{
			// Get nearest line
			int nearest = context_->map().nearestLine(context_->input().mousePosMap(), 999999);
			MapLine* line = context_->map().getLine(nearest);
			if (line)
			{
				SectorBuilder sbuilder;

				// Determine line side
				double side = MathStuff::lineSide(context_->input().mousePosMap(), line->seg());
				if (side >= 0)
					sbuilder.traceSector(&(context_->map()), line, true);
				else
					sbuilder.traceSector(&(context_->map()), line, false);
			}
		}
		if (e.GetKeyCode() == WXK_F5)
		{
			// Get nearest line
			int nearest = context_->map().nearestLine(context_->input().mousePosMap(), 999999);
			MapLine* line = context_->map().getLine(nearest);

			// Get sectors
			MapSector* sec1 = context_->map().getLineSideSector(line, true);
			MapSector* sec2 = context_->map().getLineSideSector(line, false);
			int i1 = -1;
			int i2 = -1;
			if (sec1) i1 = sec1->getIndex();
			if (sec2) i2 = sec2->getIndex();

			context_->addEditorMessage(S_FMT("Front %d Back %d", i1, i2));
		}
		if (e.GetKeyCode() == WXK_F5 && context_->editMode() == Mode::Sectors)
		{
			PolygonSplitter splitter;
			splitter.setVerbose(true);
			splitter.openSector(context_->selection().hilightedSector());
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
	// Send to editor
	context_->input().updateKeyModifiersWx(e.GetModifiers());
	context_->input().keyUp(KeyBind::keyName(e.GetKeyCode()));

	e.Skip();
}

/* MapCanvas::onMouseDown
 * Called when a mouse button is pressed within the canvas
 *******************************************************************/
void MapCanvas::onMouseDown(wxMouseEvent& e)
{
	using namespace MapEditor;

	// Send to editor context
	bool skip = true;
	if (e.LeftDown())
		skip = context_->input().mouseDown(Input::MouseButton::Left);
	else if (e.LeftDClick())
		skip = context_->input().mouseDown(Input::MouseButton::Left, true);
	else if (e.RightDown())
		skip = context_->input().mouseDown(Input::MouseButton::Right);
	else if (e.RightDClick())
		skip = context_->input().mouseDown(Input::MouseButton::Right, true);
	else if (e.MiddleDown())
		skip = context_->input().mouseDown(Input::MouseButton::Middle);
	else if (e.MiddleDClick())
		skip = context_->input().mouseDown(Input::MouseButton::Middle, true);
	else if (e.Aux1Down())
		skip = context_->input().mouseDown(Input::MouseButton::Mouse4);
	else if (e.Aux1DClick())
		skip = context_->input().mouseDown(Input::MouseButton::Mouse4, true);
	else if (e.Aux2Down())
		skip = context_->input().mouseDown(Input::MouseButton::Mouse5);
	else if (e.Aux2DClick())
		skip = context_->input().mouseDown(Input::MouseButton::Mouse5, true);

	if (skip)
	{
		// Set focus
		SetFocus();

		e.Skip();
	}
}

/* MapCanvas::onMouseUp
 * Called when a mouse button is released within the canvas
 *******************************************************************/
void MapCanvas::onMouseUp(wxMouseEvent& e)
{
	using namespace MapEditor;

	// Send to editor context
	bool skip = true;
	if (e.LeftUp())
		skip = context_->input().mouseUp(Input::MouseButton::Left);
	else if (e.RightUp())
		skip = context_->input().mouseUp(Input::MouseButton::Right);
	else if (e.MiddleUp())
		skip = context_->input().mouseUp(Input::MouseButton::Middle);
	else if (e.Aux1Up())
		skip = context_->input().mouseUp(Input::MouseButton::Mouse4);
	else if (e.Aux2Up())
		skip = context_->input().mouseUp(Input::MouseButton::Mouse5);

	if (skip)
		e.Skip();
}

/* MapCanvas::onMouseMotion
 * Called when the mouse cursor is moved within the canvas
 *******************************************************************/
void MapCanvas::onMouseMotion(wxMouseEvent& e)
{
	// Ignore if it was generated by a mouse pointer warp
	if (mouse_warp_)
	{
		mouse_warp_ = false;
		e.Skip();
		return;
	}

	// Update mouse variables
	if (!context_->input().mouseMove(e.GetX(), e.GetY()))
		return;

	e.Skip();
}

/* MapCanvas::onMouseWheel
 * Called when the mouse wheel is moved
 *******************************************************************/
void MapCanvas::onMouseWheel(wxMouseEvent& e)
{
#ifdef __WXOSX__
	double mwheel_rotation = (double)e.GetWheelRotation() / (double)e.GetWheelDelta();
	if (mwheel_rotation < 0)
		mwheel_rotation = 0 - mwheel_rotation;
#else
	double mwheel_rotation = 1;
#endif

	if (mwheel_rotation < 0.001)
		return;

	context_->input().mouseWheel(e.GetWheelRotation() > 0, mwheel_rotation);
}

/* MapCanvas::onMouseLeave
 * Called when the mouse cursor leaves the canvas
 *******************************************************************/
void MapCanvas::onMouseLeave(wxMouseEvent& e)
{
	context_->input().mouseLeave();

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
	long frametime = (sf_clock_.getElapsedTime().asMilliseconds()) - last_time;

	if (context_->update(frametime))
	{
		last_time = (sf_clock_.getElapsedTime().asMilliseconds());
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
	long frametime = (sf_clock_.getElapsedTime().asMilliseconds()) - last_time;

	if (context_->update(frametime))
	{
		last_time = (sf_clock_.getElapsedTime().asMilliseconds());
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
		if (context_->editMode() == Mode::Visual)
			lockMouse(true);
	}
	else if (e.GetEventType() == wxEVT_KILL_FOCUS)
		lockMouse(false);
}
