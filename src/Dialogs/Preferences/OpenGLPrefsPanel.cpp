
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    OpenGLPrefsPanel.cpp
 * Description: Panel containing OpenGL preference controls
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
#include "OpenGLPrefsPanel.h"
#include "OpenGL/Drawing.h"
#include "UI/NumberTextCtrl.h"
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/statbox.h>
#include <wx/stattext.h>


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, gl_tex_enable_np2)
EXTERN_CVAR(Bool, gl_point_sprite)
EXTERN_CVAR(Bool, gl_vbo)
EXTERN_CVAR(Float, gl_ui_scale)


/*******************************************************************
 * OPENGLPREFSPANEL CLASS FUNCTIONS
 *******************************************************************/

/* OpenGLPrefsPanel::OpenGLPrefsPanel
 * OpenGLPrefsPanel class constructor
 *******************************************************************/
 OpenGLPrefsPanel::OpenGLPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create sizer
	wxBoxSizer* psizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(psizer);

	// Create frame+sizer
	wxStaticBox* frame = new wxStaticBox(this, -1, "OpenGL Preferences");
	wxStaticBoxSizer* sizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	psizer->Add(sizer, 1, wxEXPAND|wxALL, 4);

	// Enable np2 textures
	cb_gl_np2 = new wxCheckBox(this, -1, "Enable Non-power-of-two textures if supported");
	sizer->Add(cb_gl_np2, 0, wxEXPAND|wxALL, 4);

	// Enable point sprites
	cb_gl_point_sprite = new wxCheckBox(this, -1, "Enable point sprites if supported");
	cb_gl_point_sprite->SetToolTip("Only disable this if you are experiencing graphical glitches like things disappearing");
	sizer->Add(cb_gl_point_sprite, 0, wxEXPAND|wxALL, 4);

	// Enable VBOs
	cb_gl_use_vbo = new wxCheckBox(this, -1, "Use Vertex Buffer Objects if supported");
	sizer->Add(cb_gl_use_vbo, 0, wxEXPAND|wxALL, 4);

	// OpenGL font size
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxALL, 4);
	hbox->Add(new wxStaticText(this, -1, "UI Scale: "), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
	hbox->Add(ntc_ui_scale = new NumberTextCtrl(this, true), 1, wxRIGHT, 2);
	hbox->Add(new wxStaticText(this, -1, "x"), 1, wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT);
	ntc_ui_scale->SetToolTip("Scale factor for the OpenGL UI and fonts, eg. info overlays in the map editor");

	last_ui_scale = gl_ui_scale;
}

/* OpenGLPrefsPanel::~OpenGLPrefsPanel
 * OpenGLPrefsPanel class destructor
 *******************************************************************/
OpenGLPrefsPanel::~OpenGLPrefsPanel()
{
}

/* OpenGLPrefsPanel::init
 * Initialises panel controls
 *******************************************************************/
void OpenGLPrefsPanel::init()
{
	cb_gl_np2->SetValue(gl_tex_enable_np2);
	cb_gl_point_sprite->SetValue(gl_point_sprite);
	cb_gl_use_vbo->SetValue(gl_vbo);
	ntc_ui_scale->setDecNumber(gl_ui_scale);
}

/* OpenGLPrefsPanel::applyPreferences
 * Applies preference values from the controls to CVARs
 *******************************************************************/
void OpenGLPrefsPanel::applyPreferences()
{
	gl_tex_enable_np2 = cb_gl_np2->GetValue();
	gl_point_sprite = cb_gl_point_sprite->GetValue();
	gl_vbo = cb_gl_use_vbo->GetValue();
	gl_ui_scale = ntc_ui_scale->getDecNumber();

	if (gl_ui_scale != last_ui_scale)
		Drawing::initFonts();

	last_ui_scale = gl_ui_scale;
}
