
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
#include "UI/Controls/STabCtrl.h"
#include "UI/Layout.h"
#include "UI/WxUtils.h"
#include "Utility/FileUtils.h"
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
EXTERN_CVAR(String, path_decohack)
EXTERN_CVAR(String, path_java)
EXTERN_CVAR(Bool, decohack_always_show_output)


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
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	auto tabs = STabCtrl::createControl(this);
	tabs->AddPage(createACSPanel(tabs), wxS("ACS"));
	tabs->AddPage(createDECOHackPanel(tabs), wxS("DECOHack"));
	sizer->Add(tabs, wxSizerFlags(1).Expand());

	// Bind events
	btn_incpath_add_->Bind(wxEVT_BUTTON, &ScriptSettingsPanel::onBtnAddIncPath, this);
	btn_incpath_remove_->Bind(wxEVT_BUTTON, &ScriptSettingsPanel::onBtnRemoveIncPath, this);
}

// -----------------------------------------------------------------------------
// Loads settings from cvars into the controls
// -----------------------------------------------------------------------------
void ScriptSettingsPanel::loadSettings()
{
	// Try to find acc executable if path is not set
	static string detected_acc_path;
	if (path_acc.value.empty())
	{
		if (detected_acc_path.empty())
			detected_acc_path = fileutil::findExecutable("acc", "acs/acc");

		path_acc = detected_acc_path;
	}

	// Try to find java executable if path is not set
	static string detected_java_path;
	if (path_java.value.empty())
	{
		if (detected_java_path.empty())
			detected_java_path = fileutil::findExecutable("java");

		path_java = detected_java_path;
	}

	flp_acc_path_->setLocation(path_acc);
	cb_always_show_output_->SetValue(acc_always_show_output);
	flp_decohack_path_->setLocation(path_decohack);
	flp_java_path_->setLocation(path_java);
	cb_always_show_output_dh_->SetValue(decohack_always_show_output);

	// Populate include paths list
	list_inc_paths_->Set(wxSplit(path_acc_libs, ';'));
}

// -----------------------------------------------------------------------------
// Applies settings from the panel controls to cvars
// -----------------------------------------------------------------------------
void ScriptSettingsPanel::applySettings()
{
	// Build include paths string
	string paths_string;
	auto   lib_paths = list_inc_paths_->GetStrings();
	for (const auto& lib_path : lib_paths)
		paths_string += lib_path.utf8_string() + ";";
	if (!paths_string.empty() && paths_string.back() == ';')
		paths_string.pop_back();

	path_acc      = flp_acc_path_->location();
	path_acc_libs = paths_string;
	path_decohack = flp_decohack_path_->location();
	path_java     = flp_java_path_->location();

	acc_always_show_output      = cb_always_show_output_->GetValue();
	decohack_always_show_output = cb_always_show_output_dh_->GetValue();
}

// -----------------------------------------------------------------------------
// Creates the ACS settings panel
// -----------------------------------------------------------------------------
wxPanel* ScriptSettingsPanel::createACSPanel(wxWindow* parent)
{
	auto panel = new wxPanel(parent);

	// Create controls
	flp_acc_path_ = new FileLocationPanel(
		panel,
		path_acc,
		true,
		"Browse For ACC Executable",
		filedialog::executableExtensionString(),
		filedialog::executableFileName("acc") + ";" + filedialog::executableFileName("bcc"));
	list_inc_paths_        = new wxListBox(panel, -1, wxDefaultPosition, wxSize(-1, FromDIP(200)));
	btn_incpath_add_       = new wxButton(panel, -1, wxS("Add"));
	btn_incpath_remove_    = new wxButton(panel, -1, wxS("Remove"));
	cb_always_show_output_ = new wxCheckBox(panel, -1, wxS("Always Show Compiler Output"));

	// Create sizer
	auto lh      = ui::LayoutHelper(panel);
	auto m_sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(m_sizer);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	m_sizer->Add(sizer, lh.sfWithLargeBorder(1).Expand());

	// ACC.exe path
	sizer->Add(
		wxutil::createLabelVBox(panel, "Location of acc executable:", flp_acc_path_),
		lh.sfWithBorder(0, wxBOTTOM).Expand());

	// Include paths
	sizer->Add(new wxStaticText(panel, -1, wxS("Include Paths:")), wxSizerFlags().Expand());
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

	return panel;
}

// -----------------------------------------------------------------------------
// Creates the DECOHack settings panel
// -----------------------------------------------------------------------------
wxPanel* ScriptSettingsPanel::createDECOHackPanel(wxWindow* parent)
{
	auto panel = new wxPanel(parent);

	// Create controls
	flp_java_path_ = new FileLocationPanel(
		panel,
		path_java,
		true,
		"Browse For Java Executable",
		filedialog::executableExtensionString(),
		filedialog::executableFileName("java"));
	flp_decohack_path_ = new FileLocationPanel(
		panel, path_decohack, true, "Browse For DoomTools Jar", "Jar Files|*.jar", "doomtools.jar");
	cb_always_show_output_dh_ = new wxCheckBox(panel, -1, wxS("Always Show Compiler Output"));

	// Create sizer
	auto lh      = ui::LayoutHelper(panel);
	auto m_sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(m_sizer);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	m_sizer->Add(sizer, lh.sfWithLargeBorder(1).Expand());

	// java path
	sizer->Add(
		wxutil::createLabelVBox(panel, "Location of Java executable:", flp_java_path_),
		lh.sfWithBorder(0, wxBOTTOM).Expand());

	// doomtools.jar path
	sizer->Add(
		wxutil::createLabelVBox(panel, "Location of DoomTools jar:", flp_decohack_path_),
		lh.sfWithBorder(0, wxBOTTOM).Expand());

	// 'Always Show Output' checkbox
	sizer->Add(cb_always_show_output_dh_, wxSizerFlags().Expand());

	return panel;
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
	wxDirDialog dlg(this, wxS("Browse for ACC Include Path"));
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
