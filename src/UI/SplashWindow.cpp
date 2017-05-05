
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    SplashWindow.cpp
 * Description: The SLADE splash window. Shows the SLADE logo, a
 *              message, and an optional progress bar (with its
 *              own message)
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
#include "MainEditor/MainEditor.h"
#include "SplashWindow.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
wxBitmap	SplashWindow::bm_logo;
int			SplashWindow::width = 300;
int			SplashWindow::height = 204;
bool		SplashWindow::init_done = false;


/*******************************************************************
 * SPLASHWINDOW CLASS FUNCTIONS
 *******************************************************************/

/* SplashWindow::SplashWindow
 * SplashWindow class constructor
 *******************************************************************/
SplashWindow::SplashWindow()
	: wxMiniFrame(nullptr, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
{
	// Init
	show_progress = false;
	progress = 0.0f;
	progress_indefinite_anim = 0.0f;
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetBackgroundColour(wxColour(180, 186, 200));

	// Bind events
	Bind(wxEVT_PAINT, &SplashWindow::onPaint, this);

	// Hide initially
	Show(false);
}

/* SplashWindow::~SplashWindow
 * SplashWindow class destructor
 *******************************************************************/
SplashWindow::~SplashWindow()
{
}

/* SplashWindow::setMessage
 * Changes the splash window message
 *******************************************************************/
void SplashWindow::setMessage(string message)
{
	this->message = message;
	forceRedraw();
}

/* SplashWindow::setProgressMessage
 * Changes the progress bar message
 *******************************************************************/
void SplashWindow::setProgressMessage(string message)
{
	message_progress = message;
	forceRedraw();
}

/* SplashWindow::setProgress
 * Sets the progress bar level, where 0.0f is 0% and 1.0f is 100%.
 * A negative value indicates 'indefinite' progress. It is safe to
 * call this very rapidly as it will only redraw the window once
 * every 20ms no matter how often it is called
 *******************************************************************/
void SplashWindow::setProgress(float progress)
{
	this->progress = progress;

	// Refresh if last redraw was > 20ms ago
	if (timer.Time() >= 20)
		forceRedraw();
}

/* SplashWindow::init
 * Sets up the splash window
 *******************************************************************/
void SplashWindow::init()
{
	if (init_done)
		return;

	// Load logo image
	string tempfile = App::path("temp.png", App::Dir::Temp);
	ArchiveEntry* logo = theArchiveManager->programResourceArchive()->getEntry("logo.png");
	if (logo)
	{
		logo->exportFile(tempfile);
		bm_logo.LoadFile(tempfile, wxBITMAP_TYPE_PNG);
	}

	// Clean up
	wxRemoveFile(tempfile);
	init_done = true;
}

/* SplashWindow::show
 * Shows the splash window with [message]. If [progress] is true, a
 * progress bar will also be shown
 *******************************************************************/
void SplashWindow::show(string message, bool progress, wxWindow* parent)
{
	// Setup progress bar
	int rheight = height;
	if (progress)
	{
		show_progress = true;
		setProgress(0.0f);
		rheight += 10;
	}
	else
		show_progress = false;

	// Set parent
	if (!parent && App::isInitialised())
		SetParent(MainEditor::windowWx());
	else
		SetParent(parent);

	// Show & init window
#ifndef __WXGTK__
	SetInitialSize(wxSize(width, rheight));
#else
	SetInitialSize(wxSize(width + 6, rheight + 6));
#endif
	setMessage(message);
	Show();
	CentreOnParent();
	forceRedraw();
}

/* SplashWindow::hide
 * Hides the splash window
 *******************************************************************/
void SplashWindow::hide()
{
	// Close
	Show(false);
	Close(true);
}

/* SplashWindow::forceRedraw
 * Forces the splash window to redraw itself
 *******************************************************************/
void SplashWindow::forceRedraw()
{
	Refresh();
	Update();
}


/*******************************************************************
 * SPLASHWINDOW CLASS EVENTS
 *******************************************************************/

/* SplashWindow::onPaint
 * Handles drawing the splash window
 *******************************************************************/
void SplashWindow::onPaint(wxPaintEvent& e)
{
	// Create device context
	wxAutoBufferedPaintDC dc(this);

	// Draw border
	dc.SetBrush(wxBrush(wxColour(180, 186, 200)));
	dc.SetPen(wxPen(wxColour(100, 104, 116)));
	dc.DrawRectangle(0, 0, width, height);

	// Draw SLADE logo
	if (bm_logo.IsOk())
		dc.DrawBitmap(bm_logo, 0, 0, true);

	// Setup text
	wxFont font(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Calibri");
	dc.SetFont(font);

	// Draw version
	string vers = "v" + Global::version;
	wxSize text_size = dc.GetTextExtent(vers);
	wxCoord x = width - text_size.GetWidth() - 8;
	wxCoord y = 190 - text_size.GetHeight();
	dc.DrawText(vers, x, y);

	// Draw message
	font.SetPointSize(10);
	font.SetWeight(wxFONTWEIGHT_BOLD);
	dc.SetFont(font);
	text_size = dc.GetTextExtent(message);
	x = (width*0.5) - int((double)text_size.GetWidth() * 0.5);
	y = (height-4) - text_size.GetHeight();
	dc.DrawText(message, x, y);

	// Draw progress bar if necessary
	if (show_progress)
	{
		// Setup progress bar
		wxRect rect_pbar(0, height-4, width, 14);

		// Draw background
		dc.SetBrush(wxBrush(wxColour(40, 40, 56)));
		dc.DrawRectangle(rect_pbar);

		// Draw meter
		if (progress >= 0)
		{
			rect_pbar.SetRight(progress * width);
			rect_pbar.Deflate(1, 1);
			dc.SetBrush(wxBrush(wxColour(100, 120, 255)));
			dc.SetPen(*wxTRANSPARENT_PEN);
			dc.DrawRectangle(rect_pbar);
		}
		else
		{
			// Negative progress means indefinite, show animation

			// Determine gradient colours
			float interval = progress_indefinite_anim * 2;
			if (interval > 1.0f)
				interval = 2.0f - interval;

			wxColor left(100 - (60.0f*interval), 120 - (80.0f*interval), 255 - (199.0f*interval));
			wxColor right(40 + (60.0f*interval), 40 + (80.0f*interval), 56 + (199.0f*interval));

			// Draw the animated meter
			rect_pbar.Deflate(1, 1);
			dc.GradientFillLinear(rect_pbar, left, right);

			// Increase animation counter
			progress_indefinite_anim += 0.02f;
			if (progress_indefinite_anim > 1.0f)
				progress_indefinite_anim = 0.0f;
		}

		// Draw text
		font.SetPointSize(8);
		dc.SetFont(font);
		text_size = dc.GetTextExtent(message_progress);
		x = (width*0.5) - int((double)text_size.GetWidth() * 0.5);
		y = height-4;
		dc.SetTextForeground(wxColour(200, 210, 255));
		dc.DrawText(message_progress, x, y);
	}

	timer.Start();
}
