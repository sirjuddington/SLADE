
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SplashWindow.cpp
// Description: The SLADE splash window. Shows the SLADE logo, a message, and
//              an optional progress bar (with its own message)
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
#include "SplashWindow.h"
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "General/UI.h"
#include <wx/app.h>

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
wxBitmap bm_logo;
int      img_width  = 300;
int      img_height = 204;
bool     init_done  = false;
} // namespace

//#if __WXGTK__
//CVAR(Int, splash_refresh_ms, 100, CVar::Flag::Save)
//#else
CVAR(Int, splash_refresh_ms, 20, CVar::Flag::Save)
//#endif


// -----------------------------------------------------------------------------
//
// SplashWindow Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SplashWindow class constructor
// -----------------------------------------------------------------------------
SplashWindow::SplashWindow(wxWindow* parent) :
	wxMiniFrame{ parent,        -1,
				 wxEmptyString, wxDefaultPosition,
				 wxDefaultSize, wxBORDER_NONE | (parent ? wxFRAME_FLOAT_ON_PARENT : 0) }
{
	// Init
	wxMiniFrame::SetBackgroundStyle(wxBG_STYLE_PAINT);
	wxMiniFrame::SetBackgroundColour(wxColour(180, 186, 200));
	wxMiniFrame::SetDoubleBuffered(true);

	// Bind events
	Bind(wxEVT_PAINT, &SplashWindow::onPaint, this);

	// Hide initially
	wxTopLevelWindow::Show(false);
}

// -----------------------------------------------------------------------------
// Changes the splash window message
// -----------------------------------------------------------------------------
void SplashWindow::setMessage(string_view message)
{
	message_ = message;
	forceRedraw();
}

// -----------------------------------------------------------------------------
// Changes the progress bar message
// -----------------------------------------------------------------------------
void SplashWindow::setProgressMessage(string_view message)
{
	message_progress_ = message;
	forceRedraw();
}

// -----------------------------------------------------------------------------
// Sets the progress bar level, where 0.0f is 0% and 1.0f is 100%.
// A negative value indicates 'indefinite' progress. It is safe to call this
// very rapidly as it will only redraw the window once every 20ms no matter how
// often it is called
// -----------------------------------------------------------------------------
void SplashWindow::setProgress(float progress)
{
	progress_ = progress;

	// Refresh if last redraw was > [splash_refresh_ms] ago
	if (timer_.Time() >= splash_refresh_ms)
		forceRedraw(false);
}

// -----------------------------------------------------------------------------
// Sets up the splash window
// -----------------------------------------------------------------------------
void SplashWindow::init()
{
	if (init_done)
		return;

	// Load logo image
	auto tempfile = app::path("temp.png", app::Dir::Temp);
	auto logo     = app::archiveManager().programResourceArchive()->entry("logo.png");
	if (logo)
	{
		logo->exportFile(tempfile);

		wxImage img;
		img.LoadFile(wxString::FromUTF8(tempfile), wxBITMAP_TYPE_PNG);
		if (ui::scaleFactor() != 1.)
			img = img.Scale(ui::scalePx(img.GetWidth()), ui::scalePx(img.GetHeight()), wxIMAGE_QUALITY_BICUBIC);

		bm_logo = wxBitmap(img);
	}

	img_width  = ui::scalePx(300);
	img_height = ui::scalePx(204);

	// Clean up
	wxRemoveFile(wxString::FromUTF8(tempfile));
	init_done = true;
}

// -----------------------------------------------------------------------------
// Shows the splash window with [message].
// If [progress] is true, a progress bar will also be shown
// -----------------------------------------------------------------------------
void SplashWindow::show(string_view message, bool progress)
{
	// Setup progress bar
	int rheight = img_height;
	if (progress)
	{
		show_progress_ = true;
		setProgress(0.0f);
		rheight += ui::scalePx(10);
	}
	else
		show_progress_ = false;

	// Show & init window
#ifndef __WXGTK__
	SetInitialSize({ img_width, rheight });
#else
	SetInitialSize({ img_width + ui::scalePx(6), rheight + ui::scalePx(6) });
#endif
	setMessage(message);
	Show();
	CentreOnParent();
	forceRedraw();
}

// -----------------------------------------------------------------------------
// Hides the splash window
// -----------------------------------------------------------------------------
void SplashWindow::hide()
{
	// Close
	Show(false);
	Close(true);
}

// -----------------------------------------------------------------------------
// Forces the splash window to redraw itself
// -----------------------------------------------------------------------------
void SplashWindow::forceRedraw(bool yield_for_ui)
{
	Refresh();
	Update();

	// Spin the event loop once, to ensure we get our paint events.
	if (yield_for_ui)
		wxTheApp->SafeYieldFor(nullptr, wxEVT_CATEGORY_UI);
}


// -----------------------------------------------------------------------------
//
// SplashWindow Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Handles drawing the splash window
// -----------------------------------------------------------------------------
void SplashWindow::onPaint(wxPaintEvent& e)
{
	// Create device context
	wxAutoBufferedPaintDC dc(this);

	// Draw border
	dc.SetBrush(wxBrush(wxColour(180, 186, 200)));
	dc.SetPen(wxPen(wxColour(100, 104, 116)));
	dc.DrawRectangle(0, 0, img_width, img_height);

	// Draw SLADE logo
	if (bm_logo.IsOk())
		dc.DrawBitmap(bm_logo, 0, 0, true);

	// Setup text
	wxFont font(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxS("Calibri"));
	dc.SetFont(font);
	dc.SetTextForeground(*wxBLACK);

	// Draw version
	string vers      = "v" + app::version().toString();
	auto   text_size = dc.GetTextExtent(wxString::FromUTF8(vers));
	auto   x         = img_width - text_size.GetWidth() - ui::scalePx(8);
	auto   y         = ui::scalePx(190) - text_size.GetHeight();
	dc.DrawText(wxString::FromUTF8(vers), x, y);

	// Draw message
	font.SetPointSize(10);
	font.SetWeight(wxFONTWEIGHT_BOLD);
	dc.SetFont(font);
	text_size = dc.GetTextExtent(wxString::FromUTF8(message_));
	x         = (img_width * 0.5) - int((double)text_size.GetWidth() * 0.5);
	y         = (img_height - 4) - text_size.GetHeight();
	dc.DrawText(wxString::FromUTF8(message_), x, y);

	// Draw progress bar if necessary
	if (show_progress_)
	{
		// Setup progress bar
		wxRect rect_pbar(0, img_height - ui::scalePx(4), img_width, ui::scalePx(14));

		// Draw background
		dc.SetBrush(wxBrush(wxColour(40, 40, 56)));
		dc.DrawRectangle(rect_pbar);

		// Draw meter
		if (progress_ >= 0)
		{
			rect_pbar.SetRight(progress_ * img_width);
			rect_pbar.Deflate(1, 1);
			dc.SetBrush(wxBrush(wxColour(100, 120, 255)));
			dc.SetPen(*wxTRANSPARENT_PEN);
			dc.DrawRectangle(rect_pbar);
		}
		else
		{
			// Negative progress means indefinite, show animation

			// Determine gradient colours
			float interval = progress_indefinite_anim_ * 2;
			if (interval > 1.0f)
				interval = 2.0f - interval;

			wxColor left(100 - (60.0f * interval), 120 - (80.0f * interval), 255 - (199.0f * interval));
			wxColor right(40 + (60.0f * interval), 40 + (80.0f * interval), 56 + (199.0f * interval));

			// Draw the animated meter
			rect_pbar.Deflate(1, 1);
			dc.GradientFillLinear(rect_pbar, left, right);

			// Increase animation counter
			progress_indefinite_anim_ += 0.02f;
			if (progress_indefinite_anim_ > 1.0f)
				progress_indefinite_anim_ = 0.0f;
		}

		// Draw text
		font.SetPointSize(8);
		dc.SetFont(font);
		text_size = dc.GetTextExtent(wxString::FromUTF8(message_progress_));
		x         = (img_width * 0.5) - int((double)text_size.GetWidth() * 0.5);
		y         = img_height - ui::scalePx(4);
		dc.SetTextForeground(wxColour(200, 210, 255));
		dc.DrawText(wxString::FromUTF8(message_progress_), x, y);
	}

	timer_.Start();
}
