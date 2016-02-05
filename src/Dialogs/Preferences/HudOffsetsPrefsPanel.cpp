
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    HudOffsetsPrefsPanel.cpp
 * Description: Panel containing preference controls for the 'hud'
 *              gfx offsets mode
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
#include "UI/WxStuff.h"
#include "HudOffsetsPrefsPanel.h"


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, hud_bob)
EXTERN_CVAR(Bool, hud_center)
EXTERN_CVAR(Bool, hud_statusbar)
EXTERN_CVAR(Bool, hud_wide)


/*******************************************************************
 * HUDOFFSETSPREFSPANEL CLASS FUNCTIONS
 *******************************************************************/

/* HudOffsetsPrefsPanel::HudOffsetsPrefsPanel
 * HudOffsetsPrefsPanel class constructor
 *******************************************************************/
 HudOffsetsPrefsPanel::HudOffsetsPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create sizer
	wxBoxSizer* psizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(psizer);

	// Create frame+sizer
	wxStaticBox* frame = new wxStaticBox(this, -1, "HUD Offsets Preferences");
	wxStaticBoxSizer* sizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	psizer->Add(sizer, 1, wxEXPAND|wxALL, 4);

	// Show weapon bob line
	cb_hud_bob = new wxCheckBox(this, -1, "Show weapon bob outline");
	sizer->Add(cb_hud_bob, 0, wxEXPAND|wxALL, 4);

	// Show center line
	cb_hud_center = new wxCheckBox(this, -1, "Show center line");
	sizer->Add(cb_hud_center, 0, wxEXPAND|wxALL, 4);

	// Show status bar line
	cb_hud_statusbar = new wxCheckBox(this, -1, "Show status bar lines");
	sizer->Add(cb_hud_statusbar, 0, wxEXPAND|wxALL, 4);

	// Show widescreen borders
	cb_hud_wide = new wxCheckBox(this, -1, "Show widescreen borders");
	sizer->Add(cb_hud_wide, 0, wxEXPAND|wxALL, 4);
}

/* HudOffsetsPrefsPanel::~HudOffsetsPrefsPanel
 * HudOffsetsPrefsPanel class destructor
 *******************************************************************/
HudOffsetsPrefsPanel::~HudOffsetsPrefsPanel()
{
}

/* HudOffsetsPrefsPanel::init
 * Initialises panel controls
 *******************************************************************/
void HudOffsetsPrefsPanel::init()
{
	cb_hud_bob->SetValue(hud_bob);
	cb_hud_center->SetValue(hud_center);
	cb_hud_statusbar->SetValue(hud_statusbar);
	cb_hud_wide->SetValue(hud_wide);
}

/* HudOffsetsPrefsPanel::applyPreferences
 * Applies preference values from the controls to CVARs
 *******************************************************************/
void HudOffsetsPrefsPanel::applyPreferences()
{
	hud_bob = cb_hud_bob->GetValue();
	hud_center = cb_hud_center->GetValue();
	hud_statusbar = cb_hud_statusbar->GetValue();
	hud_wide = cb_hud_wide->GetValue();
}
