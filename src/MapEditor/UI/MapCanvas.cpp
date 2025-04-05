
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
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
#include "MapEditor/Renderer/Overlays/MCOverlay.h"
#include "MapEditor/SectorBuilder.h"
#include "OpenGL/Drawing.h"
#include "UI/WxUtils.h"
#include "Utility/MathStuff.h"

#if !defined( NO_WAYLAND )
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkwayland.h>
#include "wayland-pointer-constraints-unstable-v1.h"
#endif

using namespace slade;

using mapeditor::Mode;


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
MapCanvas::MapCanvas(wxWindow* parent, int id, MapEditContext* context) :
	OGLCanvas{ parent, id, false },
	context_{ context }
{
	// Init variables
	context_->setCanvas(this);
	last_time_ = 0;

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

	timer_.Start(map_bg_ms);
}

// -----------------------------------------------------------------------------
// Draw the current map (2d or 3d) and any overlays etc
// -----------------------------------------------------------------------------
void MapCanvas::draw()
{
	if (!IsEnabled())
		return;

	context_->renderer().draw();

	SwapBuffers();

	glFinish();
}

#if !defined( NO_WAYLAND )

static bool wayland_warp_pointer( GtkWidget* aWidget, GdkDisplay* aDisplay, GdkWindow* aWindow,
                                  GdkDevice* aPtrDev, int aX, int aY );

// GDK doesn't know if we've moved the cursor using Wayland pointer constraints.
// So emulate the actual position here
static wxPoint s_warped_from;
static wxPoint s_warped_to;

wxPoint MapCanvas::GetMousePosition()
{
    wxPoint wx_pos = wxGetMousePosition();

    if( wx_pos == s_warped_from )
    {
        return s_warped_to;
    }
    else
    {
        // Mouse has moved
        s_warped_from = wxPoint();
        s_warped_to = wxPoint();
    }

    return wx_pos;
}

static bool                               s_wl_initialized = false;
static struct wl_compositor*              s_wl_compositor = NULL;
static struct zwp_pointer_constraints_v1* s_wl_pointer_constraints = NULL;
static struct zwp_confined_pointer_v1*    s_wl_confined_pointer = NULL;
static struct wl_region*                  s_wl_confinement_region = NULL;
static bool                               s_wl_locked_flag = false;

static void handle_global( void* data, struct wl_registry* registry, uint32_t name,
                           const char* interface, uint32_t version )
{

    if( strcmp( interface, wl_compositor_interface.name ) == 0 )
    {
        s_wl_compositor = static_cast<wl_compositor*>(
                wl_registry_bind( registry, name, &wl_compositor_interface, version ) );
    }
    else if( strcmp( interface, zwp_pointer_constraints_v1_interface.name ) == 0 )
    {
        s_wl_pointer_constraints = static_cast<zwp_pointer_constraints_v1*>( wl_registry_bind(
                registry, name, &zwp_pointer_constraints_v1_interface, version ) );
    }
}

static void locked_handler( void* data, struct zwp_locked_pointer_v1* zwp_locked_pointer_v1 )
{
    s_wl_locked_flag = true;
}

static const struct zwp_locked_pointer_v1_listener locked_pointer_listener = {
    .locked = locked_handler,
};


static const struct wl_registry_listener registry_listener = {
    .global = handle_global,
};


/**
 * Initializes `compositor` and `pointer_constraints` global variables.
 */
static void initialize_wayland( wl_display* wldisp )
{
    if( s_wl_initialized )
    {
        return;
    }

    struct wl_registry* registry = wl_display_get_registry( wldisp );
    wl_registry_add_listener( registry, &registry_listener, NULL );
    wl_display_roundtrip( wldisp );
    s_wl_initialized = true;
}


struct zwp_locked_pointer_v1* s_wl_locked_pointer = NULL;

static int s_after_paint_handler_id = 0;

static void on_frame_clock_after_paint( GdkFrameClock* clock, GtkWidget* widget )
{
    if( s_wl_locked_pointer )
    {
        zwp_locked_pointer_v1_destroy( s_wl_locked_pointer );
        s_wl_locked_pointer = NULL;

        g_signal_handler_disconnect( (gpointer) clock, s_after_paint_handler_id );
        s_after_paint_handler_id = 0;

        // restore confinement
        if( s_wl_confinement_region != NULL )
        {
            GdkDisplay* disp = gtk_widget_get_display( widget );
            GdkSeat*    seat = gdk_display_get_default_seat( disp );
            GdkDevice*  ptrdev = gdk_seat_get_pointer( seat );
            GdkWindow*  window = gtk_widget_get_window( widget );

            wl_display* wldisp = gdk_wayland_display_get_wl_display( disp );
            wl_surface* wlsurf = gdk_wayland_window_get_wl_surface( window );
            wl_pointer* wlptr = gdk_wayland_device_get_wl_pointer( ptrdev );

            s_wl_confined_pointer = zwp_pointer_constraints_v1_confine_pointer(
                    s_wl_pointer_constraints, wlsurf, wlptr, s_wl_confinement_region,
                    ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_ONESHOT );

            wl_display_roundtrip( wldisp );
        }
    }
}

static bool wayland_warp_pointer( GtkWidget* aWidget, GdkDisplay* aDisplay, GdkWindow* aWindow,
                                  GdkDevice* aPtrDev, int aX, int aY )
{
    wl_display* wldisp = gdk_wayland_display_get_wl_display( aDisplay );
    wl_surface* wlsurf = gdk_wayland_window_get_wl_surface( aWindow );
    wl_pointer* wlptr = gdk_wayland_device_get_wl_pointer( aPtrDev );

    initialize_wayland( wldisp );

    if( s_wl_locked_pointer )
    {
        // This shouldn't happen but let's be safe
        zwp_locked_pointer_v1_destroy( s_wl_locked_pointer );
        wl_display_roundtrip( wldisp );
        s_wl_locked_pointer = NULL;
    }

    // wl_surface_commit causes an assert on GNOME, but has to be called on KDE
    // before destroying the locked pointer, so wait until GDK commits the surface.
    GdkFrameClock* frame_clock = gdk_window_get_frame_clock( aWindow );
    s_after_paint_handler_id = g_signal_connect_after(
            frame_clock, "after-paint", G_CALLBACK( on_frame_clock_after_paint ), aWidget );

    // temporary disable confinement to allow pointer warping
    if( s_wl_confinement_region && s_wl_confined_pointer )
    {
        zwp_confined_pointer_v1_destroy( s_wl_confined_pointer );
        wl_display_roundtrip( wldisp );
        s_wl_confined_pointer = NULL;
    }

    s_wl_locked_flag = false;

    s_wl_locked_pointer =
            zwp_pointer_constraints_v1_lock_pointer( s_wl_pointer_constraints, wlsurf, wlptr, NULL,
                                                     ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_ONESHOT );

    zwp_locked_pointer_v1_add_listener(s_wl_locked_pointer, &locked_pointer_listener, NULL);

    gint wx, wy;
    gtk_widget_translate_coordinates( aWidget, gtk_widget_get_toplevel( aWidget ), 0, 0, &wx, &wy );

    zwp_locked_pointer_v1_set_cursor_position_hint( s_wl_locked_pointer, wl_fixed_from_int( aX + wx ),
                                                    wl_fixed_from_int( aY + wy ) );

    // Don't call wl_surface_commit, wait for GDK because of an assert on GNOME.
    wl_display_roundtrip( wldisp ); // To receive "locked" event.
    gtk_widget_queue_draw( aWidget ); // Needed on some GNOME environment to trigger
                                      // the "after-paint" event handler.

    return s_wl_locked_flag;
}


#endif


// -----------------------------------------------------------------------------
// Moves the mouse cursor to the center of the canvas
// -----------------------------------------------------------------------------
void MapCanvas::mouseToCenter()
{
	auto rect   = GetScreenRect();
	mouse_warp_ = true;

#if !defined( NO_WAYLAND )
    GtkWidget* widget = static_cast<GtkWidget*>( this->GetHandle() );

    GdkWindow* wind = gtk_widget_get_window( widget );
    GdkDisplay* disp = gdk_window_get_display( wind );
    GdkSeat*    seat = gdk_display_get_default_seat( disp );
    GdkDevice*  ptrdev = gdk_seat_get_pointer( seat );

    if( GDK_IS_WAYLAND_DISPLAY( disp ) )
      {
        wxPoint    initialPos = wxGetMousePosition();
        GdkWindow* win = this->GTKGetDrawingWindow();

        if( wayland_warp_pointer( widget, disp, win, ptrdev, rect.width * 0.5, rect.height * 0.5 ) )
          {
            s_warped_from = initialPos;
            s_warped_to = this->ClientToScreen( wxPoint( rect.width * 0.5, rect.height * 0.5 ) );

          }
      }
#endif

	sf::Mouse::setPosition(sf::Vector2i(rect.x + int(rect.width * 0.5), rect.y + int(rect.height * 0.5)));
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
		// Center mouse
		mouseToCenter();

		// Hide cursor
		wxImage img(32, 32, true);
		img.SetMask(true);
		img.SetMaskColour(0, 0, 0);
		SetCursor(wxCursor(img));
	}
	else
	{
		// Show cursor
		SetCursor(wxNullCursor);
	}
}

// -----------------------------------------------------------------------------
// Handles 3d mode mouselook
// -----------------------------------------------------------------------------
void MapCanvas::mouseLook3d()
{
	// Check for 3d mode
	if (context_->editMode() == Mode::Visual && context_->mouseLocked())
	{
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
				context_->renderer().renderer3D().cameraLook(xrel, yrel);
				mouseToCenter();
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Called when the key bind [name] is pressed
// -----------------------------------------------------------------------------
void MapCanvas::onKeyBindPress(string_view name)
{
	// Screenshot (TODO: Reimplement)
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
//
// MapCanvas Class Events
//
// -----------------------------------------------------------------------------


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
		if (e.GetKeyCode() == WXK_F6)
		{
			Polygon2D poly;
			sf::Clock clock;
			log::info(1, "Generating polygons...");
			for (unsigned a = 0; a < context_->map().nSectors(); a++)
			{
				if (!poly.openSector(context_->map().sector(a)))
					log::info(1, wxString::Format("Splitting failed for sector %d", a));
			}
			// int ms = clock.GetElapsedTime() * 1000;
			// log::info(1, "Polygon generation took %dms", ms);
		}
		if (e.GetKeyCode() == WXK_F7)
		{
			// Get nearest line
			auto line = context_->map().lines().nearest(context_->input().mousePosMap(), 999999);
			if (line)
			{
				SectorBuilder sbuilder;

				// Determine line side
				double side = math::lineSide(context_->input().mousePosMap(), line->seg());
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
	// Handle 3d mode mouselook
	mouseLook3d();

	// Get time since last redraw
	long frametime = (sf_clock_.getElapsedTime().asMilliseconds()) - last_time_;

	if (context_->update(frametime))
	{
		last_time_ = (sf_clock_.getElapsedTime().asMilliseconds());
		Refresh();
	}
}

// -----------------------------------------------------------------------------
// Called when the canvas timer is triggered
// -----------------------------------------------------------------------------
void MapCanvas::onRTimer(wxTimerEvent& e)
{
	// Handle 3d mode mouselook
	mouseLook3d();

	// Get time since last redraw
	long frametime = (sf_clock_.getElapsedTime().asMilliseconds()) - last_time_;

	if (context_->update(frametime))
	{
		last_time_ = (sf_clock_.getElapsedTime().asMilliseconds());
		Refresh();
	}
}

// -----------------------------------------------------------------------------
// Called when the canvas loses or gains focus
// -----------------------------------------------------------------------------
void MapCanvas::onFocus(wxFocusEvent& e)
{
	if (e.GetEventType() == wxEVT_SET_FOCUS)
	{
		if (context_->editMode() == Mode::Visual)
			context_->lockMouse(true);
	}
	else if (e.GetEventType() == wxEVT_KILL_FOCUS)
		context_->lockMouse(false);
}
