
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    PNGPrefsPanel.cpp
 * Description: Panel containing PNG tools preference controls
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
#include "PNGPrefsPanel.h"
#include <wx/filedlg.h>


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(String, path_pngout)
EXTERN_CVAR(String, path_pngcrush)
EXTERN_CVAR(String, path_deflopt)

/*******************************************************************
 * PNGPREFSPANEL CLASS FUNCTIONS
 *******************************************************************/

/* PNGPrefsPanel::PNGPrefsPanel
 * PNGPrefsPanel class constructor
 *******************************************************************/
PNGPrefsPanel::PNGPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create sizer
	wxBoxSizer* psizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(psizer);

	// Create frame+sizer
	wxStaticBox* frame = new wxStaticBox(this, -1, "PNG Preferences");
	wxStaticBoxSizer* sizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	psizer->Add(sizer, 1, wxEXPAND|wxALL, 4);

	// PNG tools paths
	// PNGout
	wxBoxSizer* hbox;
	sizer->Add(new wxStaticText(this, -1, "Location of PNGout:"), 0, wxALL, 4);
	hbox = new wxBoxSizer(wxHORIZONTAL);
	text_pngoutpath = new wxTextCtrl(this, -1, wxString(path_pngout));
	hbox->Add(text_pngoutpath, 1, wxEXPAND|wxRIGHT, 4);
	btn_browse_pngoutpath = new wxButton(this, -1, "Browse");
	hbox->Add(btn_browse_pngoutpath, 0, wxEXPAND);
	sizer->Add(hbox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// PNGCrush
	sizer->Add(new wxStaticText(this, -1, "Location of PNGCrush:"), 0, wxALL, 4);
	hbox = new wxBoxSizer(wxHORIZONTAL);
	text_pngcrushpath = new wxTextCtrl(this, -1, wxString(path_pngcrush));
	hbox->Add(text_pngcrushpath, 1, wxEXPAND|wxRIGHT, 4);
	btn_browse_pngcrushpath = new wxButton(this, -1, "Browse");
	hbox->Add(btn_browse_pngcrushpath, 0, wxEXPAND);
	sizer->Add(hbox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// DeflOpt
	sizer->Add(new wxStaticText(this, -1, "Location of DeflOpt:"), 0, wxALL, 4);
	hbox = new wxBoxSizer(wxHORIZONTAL);
	text_defloptpath = new wxTextCtrl(this, -1, wxString(path_deflopt));
	hbox->Add(text_defloptpath, 1, wxEXPAND|wxRIGHT, 4);
	btn_browse_defloptpath = new wxButton(this, -1, "Browse");
	hbox->Add(btn_browse_defloptpath, 0, wxEXPAND);
	sizer->Add(hbox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Bind events
	btn_browse_pngoutpath->Bind(wxEVT_BUTTON, &PNGPrefsPanel::onBtnBrowsePNGoutPath, this);
	btn_browse_pngcrushpath->Bind(wxEVT_BUTTON, &PNGPrefsPanel::onBtnBrowsePNGCrushPath, this);
	btn_browse_defloptpath->Bind(wxEVT_BUTTON, &PNGPrefsPanel::onBtnBrowseDeflOptPath, this);
}

/* PNGPrefsPanel::~PNGPrefsPanel
 * PNGPrefsPanel class destructor
 *******************************************************************/
PNGPrefsPanel::~PNGPrefsPanel()
{
}

/* PNGPrefsPanel::init
 * Initialises panel controls
 *******************************************************************/
void PNGPrefsPanel::init()
{
	text_pngoutpath->SetValue(wxString(path_pngout));
	text_pngcrushpath->SetValue(wxString(path_pngcrush));
	text_defloptpath->SetValue(wxString(path_deflopt));
}

/* PNGPrefsPanel::applyPreferences
 * Applies preferences from the panel controls
 *******************************************************************/
void PNGPrefsPanel::applyPreferences()
{
	path_pngout   = text_pngoutpath->GetValue();
	path_pngcrush = text_pngcrushpath->GetValue();
	path_deflopt  = text_defloptpath->GetValue();
}


/*******************************************************************
 * PNGPREFSPANEL CLASS EVENTS
 *******************************************************************/

/* PNGPrefsPanel::onBtnBrowsePNGoutPath
 * Called when the 'Browse' for PNGout path button is clicked
 *******************************************************************/
void PNGPrefsPanel::onBtnBrowsePNGoutPath(wxCommandEvent& e)
{
	// Setup acc executable file string
	string pngout_exe = "pngout";
#ifdef WIN32
	pngout_exe += ".exe";	// exe extension in windows
#endif

	// Open file dialog
	wxFileDialog fd(this, "Browse for PNGout Executable", wxEmptyString, pngout_exe, pngout_exe);
	if (fd.ShowModal() == wxID_OK)
		text_pngoutpath->SetValue(fd.GetPath());
}

/* PNGPrefsPanel::onBtnBrowsePNGCrushPath
 * Called when the 'Browse' for PNGout path button is clicked
 *******************************************************************/
void PNGPrefsPanel::onBtnBrowsePNGCrushPath(wxCommandEvent& e)
{
	// Setup acc executable file string
	string pngcrush_exe = "pngcrush";
#ifdef WIN32
	pngcrush_exe += ".exe";	// exe extension in windows
#endif

	// Open file dialog
	wxFileDialog fd(this, "Browse for PNGCrush Executable", wxEmptyString, pngcrush_exe, pngcrush_exe);
	if (fd.ShowModal() == wxID_OK)
		text_pngcrushpath->SetValue(fd.GetPath());
}

/* PNGPrefsPanel::onBtnBrowseDeflOptPath
 * Called when the 'Browse' for PNGout path button is clicked
 *******************************************************************/
void PNGPrefsPanel::onBtnBrowseDeflOptPath(wxCommandEvent& e)
{
	// Setup acc executable file string
	string deflopt_exe = "DeflOpt";
#ifdef WIN32
	deflopt_exe += ".exe";	// exe extension in windows
#endif

	// Open file dialog
	wxFileDialog fd(this, "Browse for DeflOpt Executable", wxEmptyString, deflopt_exe, deflopt_exe);
	if (fd.ShowModal() == wxID_OK)
		text_defloptpath->SetValue(fd.GetPath());
}
