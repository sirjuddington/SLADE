
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
#include "WxStuff.h"
#include "OpenGLPrefsPanel.h"


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, gl_tex_enable_np2)
EXTERN_CVAR(Bool, gl_point_sprite)
EXTERN_CVAR(Bool, gl_vbo)


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
	sizer->Add(cb_gl_point_sprite, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Enable VBOs
	cb_gl_use_vbo = new wxCheckBox(this, -1, "Use Vertex Buffer Objects if supported");
	sizer->Add(cb_gl_use_vbo, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);
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
}

/* OpenGLPrefsPanel::applyPreferences
 * Applies preference values from the controls to CVARs
 *******************************************************************/
void OpenGLPrefsPanel::applyPreferences()
{
	gl_tex_enable_np2 = cb_gl_np2->GetValue();
	gl_point_sprite = cb_gl_point_sprite->GetValue();
	gl_vbo = cb_gl_use_vbo->GetValue();
}
