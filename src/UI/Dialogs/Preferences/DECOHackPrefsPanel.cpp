
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    DECOHackPrefsPanel.cpp
// Description: Panel containing DECOHack preference controls
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
#include "DECOHackPrefsPanel.h"
#include "General/UI.h"
#include "UI/Controls/FileLocationPanel.h"
#include "UI/WxUtils.h"
#include "Utility/SFileDialog.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(String, path_decohack)
EXTERN_CVAR(String, path_decohack_libs)
EXTERN_CVAR(Bool, decohack_always_show_output)


// -----------------------------------------------------------------------------
//
// DECOHackPrefsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// DECOHackPrefsPanel class constructor
// -----------------------------------------------------------------------------
DECOHackPrefsPanel::DECOHackPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create controls
	flp_decohack_path_ = new FileLocationPanel(
		this,
		path_decohack,
		true,
		"Browse For DoomTools Jar",
		"Jar Files *.jar",
		"doomtools.jar");
	list_inc_paths_        = new wxListBox(this, -1, wxDefaultPosition, wxSize(-1, ui::scalePx(200)));
	btn_incpath_add_       = new wxButton(this, -1, "Add");
	btn_incpath_remove_    = new wxButton(this, -1, "Remove");
	cb_always_show_output_ = new wxCheckBox(this, -1, "Always Show Compiler Output");

	setupLayout();

	// Bind events
	btn_incpath_add_->Bind(wxEVT_BUTTON, &DECOHackPrefsPanel::onBtnAddIncPath, this);
	btn_incpath_remove_->Bind(wxEVT_BUTTON, &DECOHackPrefsPanel::onBtnRemoveIncPath, this);
}

// -----------------------------------------------------------------------------
// Initialises panel controls
// -----------------------------------------------------------------------------
void DECOHackPrefsPanel::init()
{
	flp_decohack_path_->setLocation(path_decohack);
	cb_always_show_output_->SetValue(decohack_always_show_output);

	// Populate include paths list
	list_inc_paths_->Set(wxSplit(path_decohack_libs, ';'));
}

// -----------------------------------------------------------------------------
// Applies preferences from the panel controls
// -----------------------------------------------------------------------------
void DECOHackPrefsPanel::applyPreferences()
{
	path_decohack = wxutil::strToView(flp_decohack_path_->location());

	// Build include paths string
	wxString paths_string;
	auto     lib_paths = list_inc_paths_->GetStrings();
	for (const auto& lib_path : lib_paths)
		paths_string += lib_path + ";";
	if (paths_string.EndsWith(";"))
		paths_string.RemoveLast(1);

	path_decohack_libs          = wxutil::strToView(paths_string);
	decohack_always_show_output = cb_always_show_output_->GetValue();
}

// -----------------------------------------------------------------------------
// Lays out the controls on the panel
// -----------------------------------------------------------------------------
void DECOHackPrefsPanel::setupLayout()
{
	// Create sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// doomtools.jar path
	sizer->Add(
		wxutil::createLabelVBox(this, "Location of DoomTools jar:", flp_decohack_path_), 0, wxEXPAND | wxBOTTOM, ui::pad());

	// Include paths
	sizer->Add(new wxStaticText(this, -1, "Include Paths:"), 0, wxEXPAND, ui::pad());
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 1, wxEXPAND | wxBOTTOM, ui::pad());
	hbox->Add(list_inc_paths_, 1, wxEXPAND | wxRIGHT, ui::pad());

	// Add include path
	auto vbox = new wxBoxSizer(wxVERTICAL);
	hbox->Add(vbox, 0, wxEXPAND);
	vbox->Add(btn_incpath_add_, 0, wxEXPAND | wxBOTTOM, ui::pad());

	// Remove include path
	vbox->Add(btn_incpath_remove_, 0, wxEXPAND | wxBOTTOM, ui::pad());

	// 'Always Show Output' checkbox
	sizer->Add(cb_always_show_output_, 0, wxEXPAND);
}


// -----------------------------------------------------------------------------
//
// DECOHackPrefsPanel Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the 'Add' include path button is clicked
// -----------------------------------------------------------------------------
void DECOHackPrefsPanel::onBtnAddIncPath(wxCommandEvent& e)
{
	wxDirDialog dlg(this, "Browse for DECOHack Include Path");
	if (dlg.ShowModal() == wxID_OK)
	{
		list_inc_paths_->Append(dlg.GetPath());
	}
}

// -----------------------------------------------------------------------------
// Called when the 'Remove' include path button is clicked
// -----------------------------------------------------------------------------
void DECOHackPrefsPanel::onBtnRemoveIncPath(wxCommandEvent& e)
{
	if (list_inc_paths_->GetSelection() >= 0)
		list_inc_paths_->Delete(list_inc_paths_->GetSelection());
}
