
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ACSPrefsPanel.cpp
// Description: Panel containing ACS script preference controls
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
#include "ACSPrefsPanel.h"
#include "General/UI.h"
#include "UI/Controls/FileLocationPanel.h"
#include "UI/WxUtils.h"
#include "Utility/SFileDialog.h"


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(String, path_acc)
EXTERN_CVAR(String, path_acc_libs)
EXTERN_CVAR(Bool, acc_always_show_output)


// -----------------------------------------------------------------------------
//
// ACSPrefsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ACSPrefsPanel class constructor
// -----------------------------------------------------------------------------
ACSPrefsPanel::ACSPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create controls
	flp_acc_path_ = new FileLocationPanel(
		this,
		path_acc,
		true,
		"Browse For ACC Executable",
		SFileDialog::executableExtensionString(),
		SFileDialog::executableFileName("acc") + ";" + SFileDialog::executableFileName("bcc"));
	list_inc_paths_        = new wxListBox(this, -1, wxDefaultPosition, wxSize(-1, UI::scalePx(200)));
	btn_incpath_add_       = new wxButton(this, -1, "Add");
	btn_incpath_remove_    = new wxButton(this, -1, "Remove");
	cb_always_show_output_ = new wxCheckBox(this, -1, "Always Show Compiler Output");

	setupLayout();

	// Bind events
	btn_incpath_add_->Bind(wxEVT_BUTTON, &ACSPrefsPanel::onBtnAddIncPath, this);
	btn_incpath_remove_->Bind(wxEVT_BUTTON, &ACSPrefsPanel::onBtnRemoveIncPath, this);
}

// -----------------------------------------------------------------------------
// Initialises panel controls
// -----------------------------------------------------------------------------
void ACSPrefsPanel::init()
{
	flp_acc_path_->setLocation(path_acc);
	cb_always_show_output_->SetValue(acc_always_show_output);

	// Populate include paths list
	list_inc_paths_->Set(wxSplit(path_acc_libs, ';'));
}

// -----------------------------------------------------------------------------
// Applies preferences from the panel controls
// -----------------------------------------------------------------------------
void ACSPrefsPanel::applyPreferences()
{
	path_acc = WxUtils::strToView(flp_acc_path_->location());

	// Build include paths string
	wxString paths_string;
	auto     lib_paths = list_inc_paths_->GetStrings();
	for (const auto& lib_path : lib_paths)
		paths_string += lib_path + ";";
	if (paths_string.EndsWith(";"))
		paths_string.RemoveLast(1);

	path_acc_libs          = WxUtils::strToView(paths_string);
	acc_always_show_output = cb_always_show_output_->GetValue();
}

// -----------------------------------------------------------------------------
// Lays out the controls on the panel
// -----------------------------------------------------------------------------
void ACSPrefsPanel::setupLayout()
{
	// Create sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// ACC.exe path
	sizer->Add(
		WxUtils::createLabelVBox(this, "Location of acc executable:", flp_acc_path_),
		0,
		wxEXPAND | wxBOTTOM,
		UI::pad());

	// Include paths
	sizer->Add(new wxStaticText(this, -1, "Include Paths:"), 0, wxEXPAND, UI::pad());
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 1, wxEXPAND | wxBOTTOM, UI::pad());
	hbox->Add(list_inc_paths_, 1, wxEXPAND | wxRIGHT, UI::pad());

	// Add include path
	auto vbox = new wxBoxSizer(wxVERTICAL);
	hbox->Add(vbox, 0, wxEXPAND);
	vbox->Add(btn_incpath_add_, 0, wxEXPAND | wxBOTTOM, UI::pad());

	// Remove include path
	vbox->Add(btn_incpath_remove_, 0, wxEXPAND | wxBOTTOM, UI::pad());

	// 'Always Show Output' checkbox
	sizer->Add(cb_always_show_output_, 0, wxEXPAND);
}


// -----------------------------------------------------------------------------
//
// ACSPrefsPanel Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the 'Add' include path button is clicked
// -----------------------------------------------------------------------------
void ACSPrefsPanel::onBtnAddIncPath(wxCommandEvent& e)
{
	wxDirDialog dlg(this, "Browse for ACC Include Path");
	if (dlg.ShowModal() == wxID_OK)
	{
		list_inc_paths_->Append(dlg.GetPath());
	}
}

// -----------------------------------------------------------------------------
// Called when the 'Remove' include path button is clicked
// -----------------------------------------------------------------------------
void ACSPrefsPanel::onBtnRemoveIncPath(wxCommandEvent& e)
{
	if (list_inc_paths_->GetSelection() >= 0)
		list_inc_paths_->Delete(list_inc_paths_->GetSelection());
}
