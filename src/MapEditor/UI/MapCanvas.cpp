
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         https://slade.mancubus.net
// Filename:    MapCanvas.cpp
// Description: MapCanvas class, the OpenGL canvas widget that the 2d/3d map
//              view is drawn on
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
#include "MapCanvas.h"
#include "App.h"
#include "General/ColourConfiguration.h"
#include "Geometry/Geometry.h"
#include "MapEditor/Edit/Input.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/Renderer/Overlays/MCOverlay.h"
#include "MapEditor/Renderer/Renderer.h"
#include "MapEditor/SectorBuilder.h"
#include "OpenGL/Camera.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObjectList/LineList.h"
#include "SLADEMap/SLADEMap.h"
#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Mouse.hpp>

using namespace slade;
using namespace mapeditor;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Int, map_bg_ms, 15, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// MapCanvas Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapCanvas class constructor
// -----------------------------------------------------------------------------
MapCanvas::MapCanvas(wxWindow* parent, MapEditContext* context) :
	GLCanvas{ parent },
	context_{ context },
	sf_clock_{ new sf::Clock }
{
	// Init variables
	context_->setCanvas(this);

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
	timer_.Bind(wxEVT_TIMER, &MapCanvas::onRTimer, this);
	Bind(wxEVT_IDLE, &MapCanvas::onIdle, this);

	timer_.Start(map_bg_ms);
}

// -----------------------------------------------------------------------------
// Draw the current map (2d or 3d) and any overlays etc
// -----------------------------------------------------------------------------
void MapCanvas::draw()
{
	// if (!IsEnabled())
	//	return;

	setBackground(BGStyle::Colour, colourconfig::colour("map_background"));

	context_->renderer().setUIScale(GetDPIScaleFactor());
	context_->renderer().draw();
}

// -----------------------------------------------------------------------------
// Moves the mouse cursor to the center of the canvas
// -----------------------------------------------------------------------------
void MapCanvas::mouseToCenter()
{
	auto rect   = GetScreenRect();
	mouse_warp_ = true;
	sf::Mouse::setPosition(
		sf::Vector2i(rect.x + static_cast<int>(rect.width * 0.5), rect.y + static_cast<int>(rect.height * 0.5)));
}

// -----------------------------------------------------------------------------
// Locks/unlocks the mouse cursor.
// A locked cursor is invisible and will be moved to the center of the canvas
// every frame
// -----------------------------------------------------------------------------
void MapCanvas::lockMouse(bool lock)
{
	if (lock)
	{
		// Save current mouse position
		mouse_locked_pos_.x = sf::Mouse::getPosition().x;
		mouse_locked_pos_.y = sf::Mouse::getPosition().y;

		// Center mouse
		mouseToCenter();

		// Hide cursor
		wxImage img(32, 32, true);
		img.SetMask(true);
		img.SetMaskColour(0, 0, 0);
		SetCursor(wxCursor(img));

		log::info("Mouse locked, cursor was at ({}, {})", mouse_locked_pos_.x, mouse_locked_pos_.y);
	}
	else
	{
		// Show cursor
		SetCursor(wxNullCursor);

		// Move mouse back to original position (if it was initially moved to lock)
		if (mouse_locked_pos_.x != -1 && mouse_locked_pos_.y != -1)
		{
			log::info("Mouse unlocked, moving cursor back to ({}, {})", mouse_locked_pos_.x, mouse_locked_pos_.y);
			sf::Mouse::setPosition(sf::Vector2i(mouse_locked_pos_.x, mouse_locked_pos_.y));
			mouse_locked_pos_ = { -1, -1 };
		}
	}
}

// -----------------------------------------------------------------------------
// Handles 3d mode mouselook
// -----------------------------------------------------------------------------
void MapCanvas::mouseLook3d()
{
	// Check for 3d mode
	if (context_->editMode() != Mode::Visual /* || !context_->mouseLocked()*/)
		return;

	auto overlay_current = context_->currentOverlay();
	if (!overlay_current || !overlay_current->isActive() || (overlay_current && overlay_current->allow3dMlook()))
	{
		// Get relative mouse movement (scale with dpi on macOS and Linux)
		const bool   useScaleFactor = (app::platform() == app::MacOS || app::platform() == app::Linux);
		const double scale          = useScaleFactor ? GetContentScaleFactor() : 1.;
		const double threshold      = scale - 1.0;

		wxRealPoint mouse_pos = wxGetMousePosition();
		mouse_pos.x *= scale;
		mouse_pos.y *= scale;

		const wxRealPoint screen_pos = GetScreenPosition();
		const double      xpos       = mouse_pos.x - screen_pos.x;
		const double      ypos       = mouse_pos.y - screen_pos.y;

		const wxSize size = GetSize();
		const double xrel = xpos - floor(size.x * 0.5);
		const double yrel = ypos - floor(size.y * 0.5);

		if (fabs(xrel) > threshold || fabs(yrel) > threshold)
		{
			context_->camera3d().look(xrel, yrel);
			mouseToCenter();
		}
	}
}

// -----------------------------------------------------------------------------
// Called when the key bind [name] is pressed
// -----------------------------------------------------------------------------
void MapCanvas::onKeyBindPress(string_view name)
{
	// Screenshot
	// TODO: Implement this?
#if 0
	if (name == "map_screenshot")
	{
		// Capture shot
		// auto shot = capture();
		sf::Texture tex;
		tex.create(RenderWindow::getSize().x, RenderWindow::getSize().y);
		tex.update(*this);
		auto shot = tex.copyToImage();

		// Remove alpha
		// sf::Image kinda sucks, shouldn't have to do it like this
		for (unsigned x = 0; x < shot.getSize().x; x++)
		{
			for (unsigned y = 0; y < shot.getSize().y; y++)
			{
				auto col = shot.getPixel(x, y);
				shot.setPixel(x, y, sf::Color(col.r, col.g, col.b, 255));
			}
		}

		// Save to file
		wxDateTime date;
		date.SetToCurrent();
		auto timestamp = date.FormatISOCombined('-');
		timestamp.Replace(":", "");
		auto filename = app::path(fmt::format("sladeshot-{}.png", wxutil::strToView(timestamp)), app::Dir::User);
		if (shot.saveToFile(filename))
		{
			// Editor message if the file is actually written, with full path
			context_->addEditorMessage(fmt::format("Screenshot taken ({})", filename));
		}
		else
		{
			// Editor message also if the file couldn't be written
			context_->addEditorMessage(fmt::format("Screenshot failed ({})", filename));
		}
	}
#endif
}

// -----------------------------------------------------------------------------
// Update and redraw the canvas if necessary
// -----------------------------------------------------------------------------
void MapCanvas::update()
{
	// Handle 3d mode mouselook
	if (mouse_looking_)
		mouseLook3d();

	// Get time since last redraw
	auto frametime = sf_clock_->getElapsedTime().asSeconds() * 1000.0;

	if (context_->update(frametime))
	{
		Refresh(false);
		sf_clock_->restart();
	}
}


// -----------------------------------------------------------------------------
//
// MapCanvas Class Events
//
// -----------------------------------------------------------------------------

// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppParameterMayBeConstPtrOrRef

// -----------------------------------------------------------------------------
// Called when the canvas is resized
// -----------------------------------------------------------------------------
void MapCanvas::onSize(wxSizeEvent& e)
{
	// Update screen limits
	const wxSize size = GetSize() * GetContentScaleFactor();
	context_->renderer().setViewSize(size.x, size.y);

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when a key is pressed within the canvas
// -----------------------------------------------------------------------------
void MapCanvas::onKeyDown(wxKeyEvent& e)
{
	// Send to editor
	context_->input().updateKeyModifiersWx(e.GetModifiers());
	context_->input().keyDown(KeyBind::keyName(e.GetKeyCode()));

	// Testing
	if (global::debug)
	{
		// if (e.GetKeyCode() == WXK_F6)
		//{
		//	Polygon2D poly;
		//	sf::Clock clock;
		//	log::info(1, "Generating polygons...");
		//	for (unsigned a = 0; a < context_->map().nSectors(); a++)
		//	{
		//		if (!poly.openSector(context_->map().sector(a)))
		//			log::info(1, "Splitting failed for sector {}", a);
		//	}
		//	// int ms = clock.GetElapsedTime() * 1000;
		//	// log::info(1, "Polygon generation took %dms", ms);
		// }
		if (e.GetKeyCode() == WXK_F7)
		{
			// Get nearest line
			auto line = context_->map().lines().nearest(context_->input().mousePosMap(), 999999);
			if (line)
			{
				SectorBuilder sbuilder;

				// Determine line side
				double side = geometry::lineSide(context_->input().mousePosMap(), line->seg());
				if (side >= 0)
					sbuilder.traceSector(&(context_->map()), line, true);
				else
					sbuilder.traceSector(&(context_->map()), line, false);
			}
		}
		if (e.GetKeyCode() == WXK_F5)
		{
			// Get nearest line
			auto line = context_->map().lines().nearest(context_->input().mousePosMap(), 999999);
			if (!line)
				return;

			// Get sectors
			auto sec1 = context_->map().lineSideSector(line, true);
			auto sec2 = context_->map().lineSideSector(line, false);
			int  i1   = -1;
			int  i2   = -1;
			if (sec1)
				i1 = sec1->index();
			if (sec2)
				i2 = sec2->index();

			context_->addEditorMessage(fmt::format("Front {} Back {}", i1, i2));
		}
		/*if (e.GetKeyCode() == WXK_F5 && context_->editMode() == Mode::Sectors)
		{
			PolygonSplitter splitter;
			splitter.setVerbose(true);
			splitter.openSector(context_->selection().hilightedSector());
			Polygon2D temp;
			splitter.doSplitting(&temp);
		}*/
	}

	// Update cursor in object edit mode
	// if (mouse_state == Input::MouseState::ObjectEdit)
	//	determineObjectEditState();

#ifndef __WXMAC__
	// Skipping events on OS X doesn't do anything but causes
	// sound alert (a.k.a. error beep) on every key press
	if (e.GetKeyCode() != WXK_UP && e.GetKeyCode() != WXK_DOWN && e.GetKeyCode() != WXK_LEFT
		&& e.GetKeyCode() != WXK_RIGHT && e.GetKeyCode() != WXK_NUMPAD_UP && e.GetKeyCode() != WXK_NUMPAD_DOWN
		&& e.GetKeyCode() != WXK_NUMPAD_LEFT && e.GetKeyCode() != WXK_NUMPAD_RIGHT)
		e.Skip();
#endif // !__WXMAC__
}

// -----------------------------------------------------------------------------
// Called when a key is released within the canvas
// -----------------------------------------------------------------------------
void MapCanvas::onKeyUp(wxKeyEvent& e)
{
	// Send to editor
	context_->input().updateKeyModifiersWx(e.GetModifiers());
	context_->input().keyUp(KeyBind::keyName(e.GetKeyCode()));

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when a mouse button is pressed within the canvas
// -----------------------------------------------------------------------------
void MapCanvas::onMouseDown(wxMouseEvent& e)
{
	using namespace mapeditor;

	// Send to editor context
	bool skip = true;
	context_->input().updateKeyModifiersWx(e.GetModifiers());
	if (e.LeftDown())
		skip = context_->input().mouseDown(Input::MouseButton::Left);
	else if (e.LeftDClick())
		skip = context_->input().mouseDown(Input::MouseButton::Left, true);
	else if (e.RightDown())
	{
		if (context_->editMode() == Mode::Visual)
		{
			lockMouse(true);
			mouse_looking_ = true;
		}
		else
			skip = context_->input().mouseDown(Input::MouseButton::Right);
	}
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

// -----------------------------------------------------------------------------
// Called when a mouse button is released within the canvas
// -----------------------------------------------------------------------------
void MapCanvas::onMouseUp(wxMouseEvent& e)
{
	using namespace mapeditor;

	// Send to editor context
	bool skip = true;
	context_->input().updateKeyModifiersWx(e.GetModifiers());
	if (e.LeftUp())
		skip = context_->input().mouseUp(Input::MouseButton::Left);
	else if (e.RightUp())
	{
		lockMouse(false);
		mouse_looking_ = false;
	}
	// skip = context_->input().mouseUp(Input::MouseButton::Right);
	else if (e.MiddleUp())
		skip = context_->input().mouseUp(Input::MouseButton::Middle);
	else if (e.Aux1Up())
		skip = context_->input().mouseUp(Input::MouseButton::Mouse4);
	else if (e.Aux2Up())
		skip = context_->input().mouseUp(Input::MouseButton::Mouse5);

	if (skip)
		e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the mouse cursor is moved within the canvas
// -----------------------------------------------------------------------------
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
	if (!context_->input().mouseMove(e.GetX() * GetContentScaleFactor(), e.GetY() * GetContentScaleFactor()))
		return;

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the mouse wheel is moved
// -----------------------------------------------------------------------------
void MapCanvas::onMouseWheel(wxMouseEvent& e)
{
	// wxGTK has a bug causing duplicate wheel events, but the duplicates have
	// exactly the same timestamp, which makes them easy to detect
	if (e.GetTimestamp() == last_wheel_timestamp_)
		return;
	else
		last_wheel_timestamp_ = e.GetTimestamp();

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

// -----------------------------------------------------------------------------
// Called when the mouse cursor leaves the canvas
// -----------------------------------------------------------------------------
void MapCanvas::onMouseLeave(wxMouseEvent& e)
{
	context_->input().mouseLeave();
	mouse_looking_ = false;

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the mouse cursor enters the canvas
// -----------------------------------------------------------------------------
void MapCanvas::onMouseEnter(wxMouseEvent& e)
{
	// Set focus
	// SetFocus();

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the canvas is idle
// -----------------------------------------------------------------------------
void MapCanvas::onIdle(wxIdleEvent& e)
{
	update();
}

// -----------------------------------------------------------------------------
// Called when the canvas timer is triggered
// -----------------------------------------------------------------------------
void MapCanvas::onRTimer(wxTimerEvent& e)
{
	update();
}

// -----------------------------------------------------------------------------
// Called when the canvas loses or gains focus
// -----------------------------------------------------------------------------
void MapCanvas::onFocus(wxFocusEvent& e)
{
	if (e.GetEventType() == wxEVT_SET_FOCUS)
	{
		// if (context_->editMode() == Mode::Visual)
		//	context_->lockMouse(true);
	}
	else if (e.GetEventType() == wxEVT_KILL_FOCUS)
		context_->lockMouse(false);
}
