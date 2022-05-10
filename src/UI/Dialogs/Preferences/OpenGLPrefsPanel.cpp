
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    OpenGLPrefsPanel.cpp
// Description: Panel containing OpenGL preference controls
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
#include "OpenGLPrefsPanel.h"
#include "OpenGL/Drawing.h"
#include "UI/Controls/NumberTextCtrl.h"
#include "UI/WxUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, gl_point_sprite)
EXTERN_CVAR(Bool, gl_vbo)
EXTERN_CVAR(Int, gl_font_size)


// -----------------------------------------------------------------------------
//
// OpenGLPrefsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// OpenGLPrefsPanel class constructor
// -----------------------------------------------------------------------------
OpenGLPrefsPanel::OpenGLPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent), last_font_size_{ gl_font_size }
{
	// Create sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	wxutil::layoutVertically(
		sizer,
		vector<wxObject*>{ cb_gl_point_sprite_ = new wxCheckBox(this, -1, "Enable point sprites if supported"),
						   cb_gl_use_vbo_      = new wxCheckBox(this, -1, "Use Vertex Buffer Objects if supported"),
						   wxutil::createLabelHBox(this, "Font Size:", ntc_font_size_ = new NumberTextCtrl(this)) },
		wxSizerFlags(0).Expand());

	cb_gl_point_sprite_->SetToolTip(
		"Only disable this if you are experiencing graphical glitches like things disappearing");
	ntc_font_size_->SetToolTip("The size of the font to use in OpenGL, eg. for info overlays in the map editor");
}

// -----------------------------------------------------------------------------
// Initialises panel controls
// -----------------------------------------------------------------------------
void OpenGLPrefsPanel::init()
{
	cb_gl_point_sprite_->SetValue(gl_point_sprite);
	cb_gl_use_vbo_->SetValue(gl_vbo);
	ntc_font_size_->setNumber(gl_font_size);
}

// -----------------------------------------------------------------------------
// Applies preference values from the controls to CVARs
// -----------------------------------------------------------------------------
void OpenGLPrefsPanel::applyPreferences()
{
	gl_point_sprite = cb_gl_point_sprite_->GetValue();
	gl_vbo          = cb_gl_use_vbo_->GetValue();
	gl_font_size    = ntc_font_size_->number();

	if (gl_font_size != last_font_size_)
		drawing::initFonts();

	last_font_size_ = gl_font_size;
}
