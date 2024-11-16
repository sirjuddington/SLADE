
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ScriptSettingsPanel.cpp
// Description: Panel containing script-related settings controls
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
#include "ScriptSettingsPanel.h"
#include "UI/Controls/FileLocationPanel.h"
#include "UI/Layout.h"
#include "UI/WxUtils.h"
#include "Utility/SFileDialog.h"

using namespace slade;
using namespace ui;


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
// ScriptSettingsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ScriptSettingsPanel class constructor
// -----------------------------------------------------------------------------
ScriptSettingsPanel::ScriptSettingsPanel(wxWindow* parent) : SettingsPanel(parent)
{
	// Create controls
	flp_acc_path_ = new FileLocationPanel(
		this,
		path_acc,
		true,
		"Browse For ACC Executable",
		filedialog::executableExtensionString(),
		filedialog::executableFileName("acc") + ";" + filedialog::executableFileName("bcc"));
	list_inc_paths_        = new wxListBox(this, -1, wxDefaultPosition, wxSize(-1, FromDIP(200)));
	btn_incpath_add_       = new wxButton(this, -1, "Add");
	btn_incpath_remove_    = new wxButton(this, -1, "Remove");
	cb_always_show_output_ = new wxCheckBox(this, -1, "Always Show Compiler Output");

	setupLayout();

	// Bind events
	btn_incpath_add_->Bind(wxEVT_BUTTON, &ScriptSettingsPanel::onBtnAddIncPath, this);
	btn_incpath_remove_->Bind(wxEVT_BUTTON, &ScriptSettingsPanel::onBtnRemoveIncPath, this);
}

// -----------------------------------------------------------------------------
// Initialises panel controls
// -----------------------------------------------------------------------------
void ScriptSettingsPanel::loadSettings()
{
	flp_acc_path_->setLocation(path_acc);
	cb_always_show_output_->SetValue(acc_always_show_output);

	// Populate include paths list
	list_inc_paths_->Set(wxSplit(path_acc_libs, ';'));
}

// -----------------------------------------------------------------------------
// Applies preferences from the panel controls
// -----------------------------------------------------------------------------
void ScriptSettingsPanel::applySettings()
{
	path_acc = wxutil::strToView(flp_acc_path_->location());

	// Build include paths string
	wxString paths_string;
	auto     lib_paths = list_inc_paths_->GetStrings();
	for (const auto& lib_path : lib_paths)
		paths_string += lib_path + ";";
	if (paths_string.EndsWith(";"))
		paths_string.RemoveLast(1);

	path_acc_libs          = wxutil::strToView(paths_string);
	acc_always_show_output = cb_always_show_output_->GetValue();
}

// -----------------------------------------------------------------------------
// Lays out the controls on the panel
// -----------------------------------------------------------------------------
void ScriptSettingsPanel::setupLayout()
{
	auto lh = ui::LayoutHelper(this);

	// Create sizer
	auto m_sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(m_sizer);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	m_sizer->Add(sizer, wxSizerFlags(1).Expand());

	// ACC.exe path
	sizer->Add(
		wxutil::createLabelVBox(this, "Location of acc executable:", flp_acc_path_),
		lh.sfWithBorder(0, wxBOTTOM).Expand());

	// Include paths
	sizer->Add(new wxStaticText(this, -1, "Include Paths:"), wxSizerFlags().Expand());
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, lh.sfWithBorder(1, wxBOTTOM).Expand());
	hbox->Add(list_inc_paths_, lh.sfWithBorder(1, wxRIGHT).Expand());

	// Add include path
	auto vbox = new wxBoxSizer(wxVERTICAL);
	hbox->Add(vbox, wxSizerFlags().Expand());
	vbox->Add(btn_incpath_add_, lh.sfWithBorder(0, wxBOTTOM).Expand());

	// Remove include path
	vbox->Add(btn_incpath_remove_, lh.sfWithBorder(0, wxBOTTOM).Expand());

	// 'Always Show Output' checkbox
	sizer->Add(cb_always_show_output_, wxSizerFlags().Expand());
}


// -----------------------------------------------------------------------------
//
// ScriptSettingsPanel Class Events
//
// -----------------------------------------------------------------------------

// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppParameterMayBeConstPtrOrRef

// -----------------------------------------------------------------------------
// Called when the 'Add' include path button is clicked
// -----------------------------------------------------------------------------
void ScriptSettingsPanel::onBtnAddIncPath(wxCommandEvent& e)
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
void ScriptSettingsPanel::onBtnRemoveIncPath(wxCommandEvent& e)
{
	if (list_inc_paths_->GetSelection() >= 0)
		list_inc_paths_->Delete(list_inc_paths_->GetSelection());
}
