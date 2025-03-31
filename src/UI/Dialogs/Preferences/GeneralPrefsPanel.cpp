
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    GeneralPrefsPanel.cpp
// Description: Panel containing general preference controls
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
#include "GeneralPrefsPanel.h"
#include "UI/WxUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, close_archive_with_tab)
EXTERN_CVAR(Bool, archive_load_data)
EXTERN_CVAR(Bool, auto_open_wads_root)
EXTERN_CVAR(Bool, update_check)
EXTERN_CVAR(Bool, update_check_beta)
EXTERN_CVAR(Bool, confirm_exit)
EXTERN_CVAR(Bool, backup_archives)
EXTERN_CVAR(Bool, archive_dir_ignore_hidden)


// -----------------------------------------------------------------------------
//
// GeneralPrefsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// GeneralPrefsPanel class constructor
// -----------------------------------------------------------------------------
GeneralPrefsPanel::GeneralPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create + Layout controls
	SetSizer(
		wxutil::layoutVertically(
			{
				cb_archive_load_      = new wxCheckBox(this, -1, "Load all archive entry data to memory when opened"),
				cb_archive_close_tab_ = new wxCheckBox(this, -1, "Close archive when its tab is closed"),
				cb_wads_root_         = new wxCheckBox(this, -1, "Auto open nested wad archives"),
				cb_backup_archives_   = new wxCheckBox(this, -1, "Back up archives"),
				cb_archive_dir_ignore_hidden_ = new wxCheckBox(this, -1, "Ignore hidden files in directories"),
				new wxStaticLine(this, -1, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL),
#ifdef __WXMSW__
				cb_update_check_      = new wxCheckBox(this, -1, "Check for updates on startup"),
				cb_update_check_beta_ = new wxCheckBox(this, -1, "Include beta versions when checking for updates"),
#endif
				cb_confirm_exit_ = new wxCheckBox(this, -1, "Show confirmation dialog on exit"),
			}));

	cb_wads_root_->SetToolTip(
		"When opening a zip or directory, automatically open all wad entries in the root directory");

	cb_archive_dir_ignore_hidden_->SetToolTip(
		"When opening a directory, ignore any files or subdirectories beginning with a '.'");
}

// -----------------------------------------------------------------------------
// Initialises panel controls
// -----------------------------------------------------------------------------
void GeneralPrefsPanel::init()
{
	cb_archive_load_->SetValue(archive_load_data);
	cb_archive_close_tab_->SetValue(close_archive_with_tab);
	cb_wads_root_->SetValue(auto_open_wads_root);
#ifdef __WXMSW__
	cb_update_check_->SetValue(update_check);
	cb_update_check_beta_->SetValue(update_check_beta);
#endif
	cb_confirm_exit_->SetValue(confirm_exit);
	cb_backup_archives_->SetValue(backup_archives);
	cb_archive_dir_ignore_hidden_->SetValue(archive_dir_ignore_hidden);
}

// -----------------------------------------------------------------------------
// Applies preference values from the controls to CVARs
// -----------------------------------------------------------------------------
void GeneralPrefsPanel::applyPreferences()
{
	archive_load_data      = cb_archive_load_->GetValue();
	close_archive_with_tab = cb_archive_close_tab_->GetValue();
	auto_open_wads_root    = cb_wads_root_->GetValue();
#ifdef __WXMSW__
	update_check      = cb_update_check_->GetValue();
	update_check_beta = cb_update_check_beta_->GetValue();
#endif
	confirm_exit              = cb_confirm_exit_->GetValue();
	backup_archives           = cb_backup_archives_->GetValue();
	archive_dir_ignore_hidden = cb_archive_dir_ignore_hidden_->GetValue();
}
