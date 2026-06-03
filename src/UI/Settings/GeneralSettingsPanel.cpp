
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    GeneralSettingsPanel.cpp
// Description: Panel containing general settings controls
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
#include "GeneralSettingsPanel.h"
#include "BaseResourceArchiveSettingsPanel.h"
#include "UI/Controls/STabCtrl.h"
#include "UI/Controls/SettingsTable.h"
#include "UI/Layout.h"
#include "UI/UI.h"
#include "UI/WxUtils.h"

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// GeneralSettingsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// GeneralSettingsPanel class constructor
// -----------------------------------------------------------------------------
GeneralSettingsPanel::GeneralSettingsPanel(wxWindow* parent) : SettingsPanel(parent)
{
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	base_resource_panel_ = new BaseResourceArchiveSettingsPanel(this);

	auto tabs = STabCtrl::createControl(this);
	createProgramSettingsTable(tabs);
	tabs->AddPage(settings_table_, wxS("Program"));
	tabs->AddPage(wxutil::createPadPanel(tabs, base_resource_panel_, padLarge()), wxS("Base Resource Archive"));
	sizer->Add(tabs, wxSizerFlags(1).Expand());

	base_resource_panel_->Show();
}

// -----------------------------------------------------------------------------
// Loads settings from cvars into the controls
// -----------------------------------------------------------------------------
void GeneralSettingsPanel::loadSettings()
{
	settings_table_->loadSettings();
	base_resource_panel_->loadSettings();
}

// -----------------------------------------------------------------------------
// Applies settings from the panel controls to cvars
// -----------------------------------------------------------------------------
void GeneralSettingsPanel::applySettings()
{
	settings_table_->applySettings();
	base_resource_panel_->applySettings();
}

// -----------------------------------------------------------------------------
// Creates the program settings panel
// -----------------------------------------------------------------------------
void GeneralSettingsPanel::createProgramSettingsTable(wxWindow* parent)
{
	settings_table_ = new SettingsTable(parent);

	settings_table_->addCheckBox("Show the Start Page on startup", "show_start_page");
	settings_table_->addCheckBox("Show confirmation dialog on exit", "confirm_exit");
#ifdef __WXMSW__
	settings_table_->addCheckBox("Check for updates on startup", "update_check");
	settings_table_->addCheckBox("Include beta versions when checking for updates", "update_check_beta");
#endif
	settings_table_->addSectionSeparator("Archives");
	settings_table_->addCheckBox("Close archive when its tab is closed", "close_archive_with_tab");
	settings_table_->addCheckBox(
		"Automatically open nested Wad Archives|"
		"When opening a zip or directory, automatically open all wad entries in the root directory",
		"auto_open_wads_root");
	settings_table_->addCheckBox("Backup archives before saving", "backup_archives");
	settings_table_->addCheckBox(
		"Ignore hidden files in directories|"
		"When opening a directory, ignore any files or subdirectories beginning with a '.'",
		"archive_dir_ignore_hidden");
}

// -----------------------------------------------------------------------------
// Creates the base resource archive settings panel
// -----------------------------------------------------------------------------
wxPanel* GeneralSettingsPanel::createBaseResourceArchivePanel(wxWindow* parent)
{
	auto panel = new wxPanel(parent);
	auto lh    = LayoutHelper(panel);

	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	base_resource_panel_ = new BaseResourceArchiveSettingsPanel(panel);
	sizer->Add(base_resource_panel_, lh.sfWithLargeBorder(1).Expand());

	return panel;
}
