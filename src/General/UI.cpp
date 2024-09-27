
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    UI.cpp
// Description: Misc. UI-related stuff
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
#include "UI.h"
#include "App.h"
#include "General/Console.h"
#include "UI/SplashWindow.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace slade::ui
{
unique_ptr<SplashWindow> splash_window;
bool                     splash_enabled = true;

// Pixel sizes/scale
double scale = 1.;
int    px_pad_small;
int    px_pad;
int    px_pad_min;
int    px_splitter;
int    px_spin_width;

} // namespace slade::ui


// -----------------------------------------------------------------------------
//
// UI Namespace Functions
//
// -----------------------------------------------------------------------------
namespace slade::ui
{
// -----------------------------------------------------------------------------
// Returns true when called from the main (UI) thread
// -----------------------------------------------------------------------------
bool isMainThread()
{
	return app::mainThreadId() == std::this_thread::get_id();
}
} // namespace slade::ui

// -----------------------------------------------------------------------------
// Initialises UI metric values based on [scale]
// -----------------------------------------------------------------------------
void ui::init(double scale)
{
	splash_window = std::make_unique<SplashWindow>();

#ifdef __WXMSW__
	scale = splash_window->GetDPIScaleFactor();
#endif

	ui::scale    = scale;
	px_pad_small = 8 * scale;
	px_pad       = 12 * scale;
	px_pad_min   = 3 * scale;
	px_splitter  = 10 * scale;
	if (app::platform() == app::Platform::Linux)
		px_spin_width = -1;
	else
		px_spin_width = 64 * scale;

	SplashWindow::init();
}

// -----------------------------------------------------------------------------
// Enables or disables the splash window depending on [enable]
// -----------------------------------------------------------------------------
void ui::enableSplash(bool enable)
{
	splash_enabled = enable;
}

// -----------------------------------------------------------------------------
// Shows the splash window with [message].
// If [progress] is true, the progress bar is displayed
// -----------------------------------------------------------------------------
void ui::showSplash(string_view message, bool progress, wxWindow* parent)
{
	if (!splash_enabled || !isMainThread())
		return;

	// The splash window uses wxFRAME_FLOAT_ON_PARENT, which can't be added or removed dynamically
	// on all platforms, so if the parent changes the splash window must be recreated
	if (splash_window && splash_window->GetParent() != parent)
	{
		splash_window->Destroy();
		splash_window = nullptr;
	}

	if (!splash_window)
	{
		SplashWindow::init();
		splash_window = std::make_unique<SplashWindow>(parent);
	}

	splash_window->show(wxString{ message.data(), message.size() }, progress);
}

// -----------------------------------------------------------------------------
// Hides the splash window
// -----------------------------------------------------------------------------
void ui::hideSplash()
{
	if (splash_window && isMainThread())
	{
		splash_window->hide();
		splash_window.reset();
	}
}

// -----------------------------------------------------------------------------
// Redraws the splash window
// -----------------------------------------------------------------------------
void ui::updateSplash()
{
	if (splash_window && isMainThread())
		splash_window->forceRedraw();
}

// -----------------------------------------------------------------------------
// Returns the current splash window progress
// -----------------------------------------------------------------------------
float ui::getSplashProgress()
{
	return splash_window ? splash_window->progress() : 0.0f;
}

// -----------------------------------------------------------------------------
// Sets the splash window [message]
// -----------------------------------------------------------------------------
void ui::setSplashMessage(string_view message)
{
	if (splash_window && isMainThread())
		splash_window->setMessage(wxString{ message.data(), message.size() });
}

// -----------------------------------------------------------------------------
// Sets the splash window progress bar [message]
// -----------------------------------------------------------------------------
void ui::setSplashProgressMessage(string_view message)
{
	if (splash_window && isMainThread())
		splash_window->setProgressMessage(wxString{ message.data(), message.size() });
}

// -----------------------------------------------------------------------------
// Sets the splash window [progress]
// -----------------------------------------------------------------------------
void ui::setSplashProgress(float progress)
{
	if (splash_window && isMainThread())
		splash_window->setProgress(progress);
}

// -----------------------------------------------------------------------------
// Sets the mouse cursor for [window]
// -----------------------------------------------------------------------------
void ui::setCursor(wxWindow* window, MouseCursor cursor)
{
	switch (cursor)
	{
	case MouseCursor::Hand: window->SetCursor(wxCursor(wxCURSOR_HAND)); break;
	case MouseCursor::Move: window->SetCursor(wxCursor(wxCURSOR_SIZING)); break;
	case MouseCursor::Cross: window->SetCursor(wxCursor(wxCURSOR_CROSS)); break;
	case MouseCursor::SizeNS: window->SetCursor(wxCursor(wxCURSOR_SIZENS)); break;
	case MouseCursor::SizeWE: window->SetCursor(wxCursor(wxCURSOR_SIZEWE)); break;
	case MouseCursor::SizeNESW: window->SetCursor(wxCursor(wxCURSOR_SIZENESW)); break;
	case MouseCursor::SizeNWSE: window->SetCursor(wxCursor(wxCURSOR_SIZENWSE)); break;
	default: window->SetCursor(wxNullCursor);
	}
}

// -----------------------------------------------------------------------------
// Returns the UI scaling factor
// -----------------------------------------------------------------------------
double ui::scaleFactor()
{
	return scale;
}

// -----------------------------------------------------------------------------
// Returns a UI metric size (eg. padding).
// Use this for UI sizes like padding, spin control widths etc. to keep things
// consistent
// -----------------------------------------------------------------------------
int ui::px(Size size)
{
	switch (size)
	{
	case Size::PadLarge: return px_pad;
	case Size::Pad: return px_pad_small;
	case Size::PadMinimum: return px_pad_min;
	case Size::Splitter: return px_splitter;
	case Size::SpinCtrlWidth: return px_spin_width;
	default: return 0;
	}
}

// -----------------------------------------------------------------------------
// Returns [px] scaled by the current scaling factor (in pixels).
// Will use the lower bound when converting to int
// -----------------------------------------------------------------------------
int ui::scalePx(int px)
{
	return px * scale;
}

// -----------------------------------------------------------------------------
// Returns [px] scaled by the current scaling factor (in pixels)
// Will use the upper bound when converting to int
// -----------------------------------------------------------------------------
int ui::scalePxU(int px)
{
	return static_cast<int>(std::ceil(static_cast<double>(px) * scale));
}

// -----------------------------------------------------------------------------
// Returns the standard padding size in pixels
// -----------------------------------------------------------------------------
int ui::pad()
{
	return px_pad_small;
}

// -----------------------------------------------------------------------------
// Returns the standard large padding size in pixels
// -----------------------------------------------------------------------------
int ui::padLarge()
{
	return px_pad;
}


// -----------------------------------------------------------------------------
//
// Console Commands
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Shows the splash screen with the given message, or hides it if no message is
// given
// -----------------------------------------------------------------------------
CONSOLE_COMMAND(splash, 0, false)
{
	if (args.empty())
		ui::hideSplash();
	else if (args.size() == 1)
		ui::showSplash(args[0]);
	else
	{
		ui::showSplash(args[0], true);
		float prog = strutil::asFloat(args[1]);
		ui::setSplashProgress(prog);
		ui::setSplashProgressMessage(fmt::format("Progress {}", args[1]));
	}
}
