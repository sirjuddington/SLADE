
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    EditingPrefsPanel.cpp
 * Description: Panel containing editing preference controls
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
#include "EditingPrefsPanel.h"


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, wad_force_uppercase)
EXTERN_CVAR(Int, autosave_entry_changes)
EXTERN_CVAR(Bool, percent_encoding)
EXTERN_CVAR(Bool, auto_entry_replace)

/*******************************************************************
 * EDITINGPREFSPANEL CLASS FUNCTIONS
 *******************************************************************/

/* EditingPrefsPanel::EditingPrefsPanel
 * EditingPrefsPanel class constructor
 *******************************************************************/
EditingPrefsPanel::EditingPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create sizer
	wxBoxSizer* psizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(psizer);

	// Create frame+sizer
	wxStaticBox* frame = new wxStaticBox(this, -1, "Editing Preferences");
	wxStaticBoxSizer* sizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	psizer->Add(sizer, 1, wxEXPAND|wxALL, 4);

	// Force uppercase
	cb_wad_force_uppercase = new wxCheckBox(this, -1, "Force uppercase entry names in Wad Archives");
	sizer->Add(cb_wad_force_uppercase, 0, wxEXPAND|wxLEFT|wxRIGHT, 4);

	// Percent encoding
	cb_zip_percent_encoding = new wxCheckBox(this, -1, "Use percent encoding if needed outside of Wad Archives");
	sizer->Add(cb_zip_percent_encoding, 0, wxEXPAND|wxLEFT|wxRIGHT, 4);

	// Automatically replace entries
	cb_auto_entry_replace = new wxCheckBox(this, -1, "Automatically replace entries with same name as drag-and-dropped files");
	sizer->Add(cb_auto_entry_replace, 0, wxEXPAND|wxLEFT|wxRIGHT, 4);

	// Unsaved entry changes
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxALL, 4);
	string choices[] = { "Don't Save", "Save", "Ask" };
	choice_entry_mod = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, 3, choices);
	hbox->Add(new wxStaticText(this, -1, "Action on unsaved entry changes:"), 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
	hbox->Add(choice_entry_mod, 0, wxEXPAND, 0);
}

/* EditingPrefsPanel::~EditingPrefsPanel
 * EditingPrefsPanel class destructor
 *******************************************************************/
EditingPrefsPanel::~EditingPrefsPanel()
{
}

/* EditingPrefsPanel::init
 * Initialises panel controls
 *******************************************************************/
void EditingPrefsPanel::init()
{
	cb_wad_force_uppercase->SetValue(wad_force_uppercase);
	cb_zip_percent_encoding->SetValue(percent_encoding);
	cb_auto_entry_replace->SetValue(auto_entry_replace);
	choice_entry_mod->SetSelection(autosave_entry_changes);
}

/* EditingPrefsPanel::applyPreferences
 * Applies preference values from the controls to CVARs
 *******************************************************************/
void EditingPrefsPanel::applyPreferences()
{
	wad_force_uppercase = cb_wad_force_uppercase->GetValue();
	percent_encoding = cb_zip_percent_encoding->GetValue();
	auto_entry_replace = cb_auto_entry_replace->GetValue();
	autosave_entry_changes = choice_entry_mod->GetSelection();
}
