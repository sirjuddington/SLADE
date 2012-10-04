
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2012 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ACSPrefsPanel.cpp
 * Description: Panel containing ACS script preference controls
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
#include "ACSPrefsPanel.h"
#include <wx/filedlg.h>


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(String, path_acc)


/*******************************************************************
 * ACSPREFSPANEL CLASS FUNCTIONS
 *******************************************************************/

/* ACSPrefsPanel::ACSPrefsPanel
 * ACSPrefsPanel class constructor
 *******************************************************************/
ACSPrefsPanel::ACSPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent) {
	// Create sizer
	wxBoxSizer* psizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(psizer);

	// Create frame+sizer
	wxStaticBox *frame = new wxStaticBox(this, -1, "ACS Preferences");
	wxStaticBoxSizer *sizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	psizer->Add(sizer, 1, wxEXPAND|wxALL, 4);

	// ACC.exe path
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(new wxStaticText(this, -1, "Location of acc executable:"), 0, wxALL, 4);
	text_accpath = new wxTextCtrl(this, -1, wxString(path_acc));
	hbox->Add(text_accpath, 1, wxEXPAND|wxRIGHT, 4);
	btn_browse_accpath = new wxButton(this, -1, "Browse");
	hbox->Add(btn_browse_accpath, 0, wxEXPAND);
	sizer->Add(hbox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Bind events
	btn_browse_accpath->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &ACSPrefsPanel::onBtnBrowseACCPath, this);
}

/* ACSPrefsPanel::~ACSPrefsPanel
 * ACSPrefsPanel class destructor
 *******************************************************************/
ACSPrefsPanel::~ACSPrefsPanel() {
}

/* ACSPrefsPanel::init
 * Initialises panel controls
 *******************************************************************/
void ACSPrefsPanel::init() {
	text_accpath->SetValue(wxString(path_acc));
}

/* ACSPrefsPanel::applyPreferences
 * Applies preferences from the panel controls
 *******************************************************************/
void ACSPrefsPanel::applyPreferences() {
	path_acc = text_accpath->GetValue();
}


/*******************************************************************
 * ACSPREFSPANEL CLASS EVENTS
 *******************************************************************/

/* ACSPrefsPanel::onBtnBrowseACCPath
 * Called when the 'Browse' for ACC path button is clicked
 *******************************************************************/
void ACSPrefsPanel::onBtnBrowseACCPath(wxCommandEvent& e) {
	// Setup acc executable file string
	string acc_exe = "acc";
#ifdef WIN32
	acc_exe += ".exe";	// exe extension in windows
#endif

	// Open file dialog
	wxFileDialog fd(this, "Browse for ACC Executable", wxEmptyString, acc_exe, acc_exe);
	if (fd.ShowModal() == wxID_OK)
		text_accpath->SetValue(fd.GetPath());
}
