
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
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
#include "General/Console/Console.h"
#include "UI/SplashWindow.h"
#include "Utility/StringUtils.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace UI
{
std::unique_ptr<SplashWindow> splash_window;
bool                          splash_enabled = true;

// Pixel sizes/scale
double scale = 1.;
int    px_pad_small;
int    px_pad;
int    px_pad_min;
int    px_splitter;
int    px_spin_width;

} // namespace UI


// -----------------------------------------------------------------------------
//
// UI Namespace Functions
//
// -----------------------------------------------------------------------------
namespace UI
{
// -----------------------------------------------------------------------------
// Returns true when called from the main (UI) thread
// -----------------------------------------------------------------------------
bool isMainThread()
{
	return App::mainThreadId() == std::this_thread::get_id();
}
} // namespace UI

// -----------------------------------------------------------------------------
// Initialises UI metric values based on [scale]
// -----------------------------------------------------------------------------
void UI::init(double scale)
{
	UI::scale    = scale;
	px_pad_small = 8 * scale;
	px_pad       = 12 * scale;
	px_pad_min   = 3 * scale;
	px_splitter  = 10 * scale;
	if (App::platform() == App::Platform::Linux)
		px_spin_width = -1;
	else
		px_spin_width = 64 * scale;
}

// -----------------------------------------------------------------------------
// Enables or disables the splash window depending on [enable]
// -----------------------------------------------------------------------------
void UI::enableSplash(bool enable)
{
	splash_enabled = enable;
}

// -----------------------------------------------------------------------------
// Shows the splash window with [message].
// If [progress] is true, the progress bar is displayed
// -----------------------------------------------------------------------------
void UI::showSplash(std::string_view message, bool progress, wxWindow* parent)
{
	if (!splash_enabled || !isMainThread())
		return;

	if (!splash_window)
	{
		SplashWindow::init();
		splash_window = std::make_unique<SplashWindow>();
	}

	splash_window->show(std::string{ message }, progress, parent);
}

// -----------------------------------------------------------------------------
// Hides the splash window
// -----------------------------------------------------------------------------
void UI::hideSplash()
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
void UI::updateSplash()
{
	if (splash_window && isMainThread())
		splash_window->forceRedraw();
}

// -----------------------------------------------------------------------------
// Returns the current splash window progress
// -----------------------------------------------------------------------------
float UI::getSplashProgress()
{
	return splash_window ? splash_window->progress() : 0.0f;
}

// -----------------------------------------------------------------------------
// Sets the splash window [message]
// -----------------------------------------------------------------------------
void UI::setSplashMessage(std::string_view message)
{
	if (splash_window && isMainThread())
		splash_window->setMessage(std::string{ message });
}

// -----------------------------------------------------------------------------
// Sets the splash window progress bar [message]
// -----------------------------------------------------------------------------
void UI::setSplashProgressMessage(std::string_view message)
{
	if (splash_window && isMainThread())
		splash_window->setProgressMessage(std::string{ message });
}

// -----------------------------------------------------------------------------
// Sets the splash window [progress]
// -----------------------------------------------------------------------------
void UI::setSplashProgress(float progress)
{
	if (splash_window && isMainThread())
		splash_window->setProgress(progress);
}

// -----------------------------------------------------------------------------
// Sets the mouse cursor for [window]
// -----------------------------------------------------------------------------
void UI::setCursor(wxWindow* window, MouseCursor cursor)
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
double UI::scaleFactor()
{
	return scale;
}

// -----------------------------------------------------------------------------
// Returns a UI metric size (eg. padding).
// Use this for UI sizes like padding, spin control widths etc. to keep things
// consistent
// -----------------------------------------------------------------------------
int UI::px(Size size)
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
// Returns [px] scaled by the current scaling factor (in pixels)
// -----------------------------------------------------------------------------
int UI::scalePx(int px)
{
	return px * scale;
}

// -----------------------------------------------------------------------------
// Returns the standard padding size in pixels
// -----------------------------------------------------------------------------
int UI::pad()
{
	return px_pad_small;
}

// -----------------------------------------------------------------------------
// Returns the standard large padding size in pixels
// -----------------------------------------------------------------------------
int UI::padLarge()
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
		UI::hideSplash();
	else if (args.size() == 1)
		UI::showSplash(args[0]);
	else
	{
		UI::showSplash(args[0], true);
		float prog = wxStringUtils::toFloat(args[1]);
		UI::setSplashProgress(prog);
		UI::setSplashProgressMessage(fmt::format("Progress {}", args[1]));
	}
}
