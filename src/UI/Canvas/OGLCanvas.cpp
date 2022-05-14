
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    OGLCanvas.cpp
// Description: OGLCanvas class. Abstract base class for all SLADE
//              wxGLCanvas-based UI elements
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
#include "OGLCanvas.h"
#include "App.h"
#include "OpenGL/Drawing.h"
#include "OpenGL/GLTexture.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Int, gl_depth_buffer_size)


// -----------------------------------------------------------------------------
//
// OGLCanvas Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// OGLCanvas class constructor, wxGLCanvas implementation
// -----------------------------------------------------------------------------
OGLCanvas::OGLCanvas(wxWindow* parent, int id, bool handle_timer, int timer_interval) :
	wxGLCanvas(parent, gl::getWxGLAttribs(), id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxWANTS_CHARS),
	timer_{ this },
	last_time_{ app::runTimer() }
{
	// Bind events
	Bind(wxEVT_PAINT, &OGLCanvas::onPaint, this);
	Bind(wxEVT_ERASE_BACKGROUND, &OGLCanvas::onEraseBackground, this);

	gl::Texture::resetBackgroundTexture();
}

// -----------------------------------------------------------------------------
// Sets the current gl context to the canvas' context, and creates it if it
// doesn't exist.
// Returns true if the context is valid, false otherwise
// -----------------------------------------------------------------------------
bool OGLCanvas::setContext()
{
	if (auto* context = gl::getContext(this))
	{
		context->SetCurrent(*this);
		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Initialises OpenGL settings for the GL canvas
// -----------------------------------------------------------------------------
void OGLCanvas::init()
{
	gl::init();

	const wxSize size = GetSize() * GetContentScaleFactor();
	glViewport(0, 0, size.x, size.y);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0);
	glShadeModel(GL_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glCullFace(GL_NONE);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_FOG);
	glEnable(GL_ALPHA_TEST);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(0, size.x, size.y, 0, -1, 100);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	init_done_ = true;
}

// -----------------------------------------------------------------------------
// Fills the canvas with a checkered pattern
// (generally used as the 'background' - to indicate transparency)
// -----------------------------------------------------------------------------
void OGLCanvas::drawCheckeredBackground() const
{
	// Save current matrix
	glPushMatrix();

	// Enable textures
	glEnable(GL_TEXTURE_2D);

	// Bind background texture
	gl::Texture::bind(gl::Texture::backgroundTexture());

	// Draw background
	const wxSize size = GetSize() * GetContentScaleFactor();
	Rectf        rect(0, 0, size.x, size.y);
	gl::setColour(ColRGBA::WHITE);
	glBegin(GL_QUADS);
	glTexCoord2d(rect.x1() * 0.0625, rect.y1() * 0.0625);
	glVertex2d(rect.x1(), rect.y1());
	glTexCoord2d(rect.x1() * 0.0625, rect.y2() * 0.0625);
	glVertex2d(rect.x1(), rect.y2());
	glTexCoord2d(rect.x2() * 0.0625, rect.y2() * 0.0625);
	glVertex2d(rect.x2(), rect.y2());
	glTexCoord2d(rect.x2() * 0.0625, rect.y1() * 0.0625);
	glVertex2d(rect.x2(), rect.y1());
	glEnd();

	// Disable textures
	glDisable(GL_TEXTURE_2D);

	// Restore previous matrix
	glPopMatrix();
}

// -----------------------------------------------------------------------------
// Activates the GL context for this canvas.
// Returns false if setting the active context failed
// -----------------------------------------------------------------------------
bool OGLCanvas::setActive()
{
	return setContext();
}

// -----------------------------------------------------------------------------
// Sets up the OpenGL matrices for generic 2d (ortho)
// -----------------------------------------------------------------------------
void OGLCanvas::setup2D() const
{
	// Setup the viewport
	const wxSize size = GetSize() * GetContentScaleFactor();
	glViewport(0, 0, size.x, size.y);

	// Setup the screen projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, size.x, size.y, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Clear
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Translate to inside of pixel (otherwise inaccuracies can occur on certain gl implementations)
	if (gl::accuracyTweak())
		glTranslatef(0.375f, 0.375f, 0);
}


// -----------------------------------------------------------------------------
//
// OGLCanvas Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the GL canvas has to be redrawn
// -----------------------------------------------------------------------------
void OGLCanvas::onPaint(wxPaintEvent& e)
{
	wxPaintDC dc(this);

	if (IsShown())
	{
		// Set context to this window
		if (!setActive())
			return;

		// Init if needed
		if (!init_done_)
			init();

		// Draw content
		gl::resetBlend();
		draw();
	}
}

// -----------------------------------------------------------------------------
// Called when the GL canvas background is to be erased
// (need to override this to do nothing or the canvas will flicker in wxMSW)
// -----------------------------------------------------------------------------
void OGLCanvas::onEraseBackground(wxEraseEvent& e)
{
	// Do nothing
}

// -----------------------------------------------------------------------------
// Called when the update timer ticks
// -----------------------------------------------------------------------------
void OGLCanvas::onTimer(wxTimerEvent& e)
{
	// Get time since last redraw
	long frametime = app::runTimer() - last_time_;
	last_time_     = app::runTimer();

	// Update/refresh
	update(frametime);
	Refresh();
}
