
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
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
#include "General/UI.h"
#include "OpenGL/Drawing.h"
#include "OpenGL/GLTexture.h"

#ifdef USE_SFML_RENDERWINDOW
#ifdef __WXGTK__
#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <gtk/gtkwidget.h>
#endif
#endif


// -----------------------------------------------------------------------------
//
// OGLCanvas Class Functions
//
// -----------------------------------------------------------------------------


#ifdef USE_SFML_RENDERWINDOW
// -----------------------------------------------------------------------------
// OGLCanvas class constructor, SFML implementation
// -----------------------------------------------------------------------------
OGLCanvas::OGLCanvas(wxWindow* parent, int id, bool handle_timer, int timer_interval) :
	wxControl(parent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxWANTS_CHARS),
	timer_(this)
{
	init_done_ = false;
	recreate_  = false;
	last_time_ = App::runTimer();

	if (handle_timer)
		timer_.Start(timer_interval);

	// Create SFML RenderWindow
	createSFML();

	// Bind events
	Bind(wxEVT_PAINT, &OGLCanvas::onPaint, this);
	Bind(wxEVT_ERASE_BACKGROUND, &OGLCanvas::onEraseBackground, this);
	// Bind(wxEVT_IDLE, &OGLCanvas::onIdle, this);
	if (handle_timer)
		Bind(wxEVT_TIMER, &OGLCanvas::onTimer, this);
	Bind(wxEVT_SIZE, &OGLCanvas::onResize, this);

	GLTexture::resetBgTex();
}
#else
// -----------------------------------------------------------------------------
// OGLCanvas class constructor, wxGLCanvas implementation
// -----------------------------------------------------------------------------
OGLCanvas::OGLCanvas(wxWindow* parent, int id, bool handle_timer, int timer_interval) :
	wxGLCanvas(parent, id, OpenGL::getWxGLAttribs(), wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxWANTS_CHARS),
	timer_(this)
{
	init_done_ = false;
	last_time_ = App::runTimer();

	// if (handle_timer)
	//	timer.Start(timer_interval);

	// Bind events
	Bind(wxEVT_PAINT, &OGLCanvas::onPaint, this);
	Bind(wxEVT_ERASE_BACKGROUND, &OGLCanvas::onEraseBackground, this);
	// Bind(wxEVT_IDLE, &OGLCanvas::onIdle, this);
	// if (handle_timer)
	//	Bind(wxEVT_TIMER, &OGLCanvas::onTimer, this);

	GLTexture::resetBgTex();
}
#endif


// -----------------------------------------------------------------------------
// OGLCanvas class constructor
// -----------------------------------------------------------------------------
OGLCanvas::~OGLCanvas() {}

// -----------------------------------------------------------------------------
// Sets the current gl context to the canvas' context, and creates it if it
// doesn't exist.
// Returns true if the context is valid, false otherwise
// -----------------------------------------------------------------------------
bool OGLCanvas::setContext()
{
#ifndef USE_SFML_RENDERWINDOW
	wxGLContext* context = OpenGL::getContext(this);

	if (context)
	{
		context->SetCurrent(*this);
		return true;
	}
	else
		return false;
#else
	return true;
#endif
}

void OGLCanvas::createSFML()
{
#ifdef USE_SFML_RENDERWINDOW
	// Code taken from SFML wxWidgets integration example
	sf::WindowHandle handle;
#ifdef __WXGTK__
	// GTK implementation requires to go deeper to find the
	// low-level X11 identifier of the widget
	gtk_widget_realize(m_wxwindow);
	gtk_widget_set_double_buffered(m_wxwindow, false);
	GdkWindow* Win = gtk_widget_get_window(m_wxwindow);
	XFlush(GDK_WINDOW_XDISPLAY(Win));
	// sf::RenderWindow::Create(GDK_WINDOW_XWINDOW(Win));
	handle = GDK_WINDOW_XWINDOW(Win);
#else
	handle = GetHandle();
#endif
	// Context settings
	sf::ContextSettings settings;
	settings.depthBits   = 32;
	settings.stencilBits = 8;
	sf::RenderWindow::create(handle, settings);
#endif
}

// -----------------------------------------------------------------------------
// Initialises OpenGL settings for the GL canvas
// -----------------------------------------------------------------------------
void OGLCanvas::init()
{
	OpenGL::init();

	glViewport(0, 0, GetSize().x, GetSize().y);
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

	glOrtho(0, GetSize().x, GetSize().y, 0, -1, 100);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	init_done_ = true;
}

// -----------------------------------------------------------------------------
// Fills the canvas with a checkered pattern
// (generally used as the 'background' - to indicate transparency)
// -----------------------------------------------------------------------------
void OGLCanvas::drawCheckeredBackground()
{
	// Save current matrix
	glPushMatrix();

	// Enable textures
	glEnable(GL_TEXTURE_2D);

	// Bind background texture
	GLTexture::bgTex().bind();

	// Draw background
	frect_t rect(0, 0, GetSize().x, GetSize().y);
	OpenGL::setColour(COL_WHITE);
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
// Places the canvas on top of a new wxPanel and returns the panel.
// This is sometimes needed to fix redraw problems in Windows XP
// -----------------------------------------------------------------------------
wxWindow* OGLCanvas::toPanel(wxWindow* parent)
{
#ifdef USE_SFML_RENDERWINDOW
#ifdef __WXGTK__
	// Reparenting the window causes a crash under gtk, so don't do it there
	// (this was only to fix a bug in winxp anyway)
	return this;
#endif
#endif

	// Create panel
	wxPanel* panel = new wxPanel(parent, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_SIMPLE);

	// Reparent
	Reparent(panel);

	// Create sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	panel->SetSizer(sizer);

	// Add to sizer
	sizer->Add(this, 1, wxEXPAND);

	return panel;
}

// -----------------------------------------------------------------------------
// Activates the GL context for this canvas.
// Returns false if setting the active context failed
// -----------------------------------------------------------------------------
bool OGLCanvas::setActive()
{
#ifdef USE_SFML_RENDERWINDOW
	if (!sf::RenderWindow::setActive())
		return false;

	Drawing::setRenderTarget(this);
	resetGLStates();
	setView(sf::View(sf::FloatRect(0.0f, 0.0f, GetSize().x, GetSize().y)));

	return true;
#else
	return setContext();
#endif
}

// -----------------------------------------------------------------------------
// Sets up the OpenGL matrices for generic 2d (ortho)
// -----------------------------------------------------------------------------
void OGLCanvas::setup2D()
{
	// Setup the viewport
	glViewport(0, 0, GetSize().x, GetSize().y);

	// Setup the screen projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, GetSize().x, GetSize().y, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Clear
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Translate to inside of pixel (otherwise inaccuracies can occur on certain gl implementations)
	if (OpenGL::accuracyTweak())
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

	if (recreate_)
	{
		createSFML();
		recreate_ = false;
	}

	if (IsShown())
	{
		// Set context to this window
		if (!setActive())
			return;

		// Init if needed
		if (!init_done_)
			init();

		// Draw content
		OpenGL::resetBlend();
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
	long frametime = App::runTimer() - last_time_;
	last_time_     = App::runTimer();

	// Update/refresh
	update(frametime);
	Refresh();
}

// -----------------------------------------------------------------------------
// Called when the GL canvas is resized
// -----------------------------------------------------------------------------
void OGLCanvas::onResize(wxSizeEvent& e)
{
#if (SFML_VERSION_MAJOR >= 2 && SFML_VERSION_MINOR >= 1) || __WXGTK__
	// Recreate SFML RenderWindow
	recreate_ = true;
#endif

	e.Skip();
}
