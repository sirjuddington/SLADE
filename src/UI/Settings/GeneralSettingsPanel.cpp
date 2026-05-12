
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
#include "UI/Layout.h"
#include "UI/UI.h"
#include "UI/WxUtils.h"

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, show_start_page)
EXTERN_CVAR(Bool, close_archive_with_tab)
EXTERN_CVAR(Bool, auto_open_wads_root)
EXTERN_CVAR(Bool, update_check)
EXTERN_CVAR(Bool, update_check_beta)
EXTERN_CVAR(Bool, confirm_exit)
EXTERN_CVAR(Bool, backup_archives)
EXTERN_CVAR(Bool, archive_dir_ignore_hidden)


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
	tabs->AddPage(createProgramSettingsPanel(tabs), wxS("Program"));
	tabs->AddPage(wxutil::createPadPanel(tabs, base_resource_panel_, padLarge()), wxS("Base Resource Archive"));
	sizer->Add(tabs, wxSizerFlags(1).Expand());

	base_resource_panel_->Show();
}

// -----------------------------------------------------------------------------
// Loads settings from cvars into the controls
// -----------------------------------------------------------------------------
void GeneralSettingsPanel::loadSettings()
{
	cb_show_start_page_->SetValue(show_start_page);
	cb_confirm_exit_->SetValue(confirm_exit);
	cb_update_check_->SetValue(update_check);
	cb_update_check_beta_->SetValue(update_check_beta);
	cb_close_archive_with_tab_->SetValue(close_archive_with_tab);
	cb_auto_open_wads_root_->SetValue(auto_open_wads_root);
	cb_backup_archives_->SetValue(backup_archives);
	cb_archive_dir_ignore_hidden_->SetValue(archive_dir_ignore_hidden);

	base_resource_panel_->loadSettings();
}

// -----------------------------------------------------------------------------
// Applies settings from the panel controls to cvars
// -----------------------------------------------------------------------------
void GeneralSettingsPanel::applySettings()
{
	show_start_page           = cb_show_start_page_->GetValue();
	confirm_exit              = cb_confirm_exit_->GetValue();
	update_check              = cb_update_check_->GetValue();
	update_check_beta         = cb_update_check_beta_->GetValue();
	close_archive_with_tab    = cb_close_archive_with_tab_->GetValue();
	auto_open_wads_root       = cb_auto_open_wads_root_->GetValue();
	backup_archives           = cb_backup_archives_->GetValue();
	archive_dir_ignore_hidden = cb_archive_dir_ignore_hidden_->GetValue();

	base_resource_panel_->applySettings();
}

// -----------------------------------------------------------------------------
// Creates the program settings panel
// -----------------------------------------------------------------------------
wxPanel* GeneralSettingsPanel::createProgramSettingsPanel(wxWindow* parent)
{
	auto panel = new wxPanel(parent);
	auto lh    = LayoutHelper(panel);

	// Create controls
	cb_show_start_page_           = new wxCheckBox(panel, -1, wxS("Show the Start Page on startup"));
	cb_confirm_exit_              = new wxCheckBox(panel, -1, wxS("Show confirmation dialog on exit"));
	cb_update_check_              = new wxCheckBox(panel, -1, wxS("Check for updates on startup"));
	cb_update_check_beta_         = new wxCheckBox(panel, -1, wxS("Include beta versions when checking for updates"));
	cb_close_archive_with_tab_    = new wxCheckBox(panel, -1, wxS("Close archive when its tab is closed"));
	cb_auto_open_wads_root_       = new wxCheckBox(panel, -1, wxS("Automatically open nested Wad Archives"));
	cb_backup_archives_           = new wxCheckBox(panel, -1, wxS("Backup archives before saving"));
	cb_archive_dir_ignore_hidden_ = new wxCheckBox(panel, -1, wxS("Ignore hidden files in directories"));

	// Layout
	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);
	auto vbox = new wxBoxSizer(wxVERTICAL);
	sizer->Add(vbox, lh.sfWithLargeBorder(1).Expand());

	lh.layoutVertically(vbox, { cb_show_start_page_, cb_confirm_exit_, cb_update_check_, cb_update_check_beta_ });

	// Archive
	vbox->AddSpacer(lh.padXLarge());
	vbox->Add(wxutil::createSectionSeparator(panel, "Archives"), lh.sfWithBorder(0, wxBOTTOM).Expand());
	lh.layoutVertically(
		vbox,
		{ cb_close_archive_with_tab_, cb_auto_open_wads_root_, cb_backup_archives_, cb_archive_dir_ignore_hidden_ },
		lh.sfWithBorder(0, wxLEFT));

#ifndef __WXMSW__
	cb_update_check_->Hide();
	cb_update_check_beta_->Hide();
#endif

	return panel;
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
