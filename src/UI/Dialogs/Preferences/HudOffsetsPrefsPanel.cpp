
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    HudOffsetsPrefsPanel.cpp
// Description: Panel containing preference controls for the 'hud' gfx offsets
//              mode
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
#include "HudOffsetsPrefsPanel.h"
#include "UI/Layout.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, hud_bob)
EXTERN_CVAR(Bool, hud_center)
EXTERN_CVAR(Bool, hud_statusbar)
EXTERN_CVAR(Bool, hud_wide)


// -----------------------------------------------------------------------------
//
// HudOffsetsPrefsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// HudOffsetsPrefsPanel class constructor
// -----------------------------------------------------------------------------
HudOffsetsPrefsPanel::HudOffsetsPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	ui::LayoutHelper(this).layoutVertically(
		sizer,
		{ cb_hud_bob_       = new wxCheckBox(this, -1, "Show weapon bob outline"),
		  cb_hud_center_    = new wxCheckBox(this, -1, "Show center line"),
		  cb_hud_statusbar_ = new wxCheckBox(this, -1, "Show status bar lines"),
		  cb_hud_wide_      = new wxCheckBox(this, -1, "Show widescreen borders") },
		wxSizerFlags(0).Expand());
}

// -----------------------------------------------------------------------------
// Initialises panel controls
// -----------------------------------------------------------------------------
void HudOffsetsPrefsPanel::init()
{
	cb_hud_bob_->SetValue(hud_bob);
	cb_hud_center_->SetValue(hud_center);
	cb_hud_statusbar_->SetValue(hud_statusbar);
	cb_hud_wide_->SetValue(hud_wide);
}

// -----------------------------------------------------------------------------
// Applies preference values from the controls to CVARs
// -----------------------------------------------------------------------------
void HudOffsetsPrefsPanel::applyPreferences()
{
	hud_bob       = cb_hud_bob_->GetValue();
	hud_center    = cb_hud_center_->GetValue();
	hud_statusbar = cb_hud_statusbar_->GetValue();
	hud_wide      = cb_hud_wide_->GetValue();
}
