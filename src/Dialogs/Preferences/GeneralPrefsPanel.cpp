
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    GeneralPrefsPanel.cpp
 * Description: Panel containing general preference controls
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
#include "GeneralPrefsPanel.h"



/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, close_archive_with_tab)
EXTERN_CVAR(Bool, archive_load_data)
EXTERN_CVAR(Bool, auto_open_wads_root)
EXTERN_CVAR(Bool, update_check)
EXTERN_CVAR(Bool, update_check_beta)
EXTERN_CVAR(Bool, confirm_exit)
EXTERN_CVAR(Bool, backup_archives)


/*******************************************************************
 * GENERALPREFSPANEL CLASS FUNCTIONS
 *******************************************************************/

/* GeneralPrefsPanel::GeneralPrefsPanel
 * GeneralPrefsPanel class constructor
 *******************************************************************/
GeneralPrefsPanel::GeneralPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create sizer
	wxBoxSizer* psizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(psizer);

	// Create frame+sizer
	wxStaticBox* frame = new wxStaticBox(this, -1, "General Preferences");
	wxStaticBoxSizer* sizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	psizer->Add(sizer, 1, wxEXPAND|wxALL, 4);

	// Load on open archive
	cb_archive_load = new wxCheckBox(this, -1, "Load all archive entry data to memory when opened");
	sizer->Add(cb_archive_load, 0, wxEXPAND|wxALL, 4);

	// Close archive with tab
	cb_archive_close_tab = new wxCheckBox(this, -1, "Close archive when its tab is closed");
	sizer->Add(cb_archive_close_tab, 0, wxEXPAND|wxALL, 4);

	// Auto open wads in root
	cb_wads_root = new wxCheckBox(this, -1, "Auto open nested wad archives");
	cb_wads_root->SetToolTip("When opening a zip or folder archive, automatically open all wad entries in the root directory");
	sizer->Add(cb_wads_root, 0, wxEXPAND|wxALL, 4);

#ifdef __WXMSW__
	// Check for updates
	cb_update_check = new wxCheckBox(this, -1, "Check for updates on startup");
	sizer->Add(cb_update_check, 0, wxEXPAND|wxALL, 4);

	// Check for beta updates
	cb_update_check_beta = new wxCheckBox(this, -1, "Include beta versions when checking for updates");
	sizer->Add(cb_update_check_beta, 0, wxEXPAND|wxALL, 4);
#endif

	// Confirm exit
	cb_confirm_exit = new wxCheckBox(this, -1, "Show confirmation dialog on exit");
	sizer->Add(cb_confirm_exit, 0, wxEXPAND|wxALL, 4);

	// Back up archives
	cb_backup_archives = new wxCheckBox(this, -1, "Back up archives");
	sizer->Add(cb_backup_archives, 0, wxEXPAND|wxALL, 4);
}

/* GeneralPrefsPanel::~GeneralPrefsPanel
 * GeneralPrefsPanel class destructor
 *******************************************************************/
GeneralPrefsPanel::~GeneralPrefsPanel()
{
}

/* GeneralPrefsPanel::init
 * Initialises panel controls
 *******************************************************************/
void GeneralPrefsPanel::init()
{
	cb_archive_load->SetValue(archive_load_data);
	cb_archive_close_tab->SetValue(close_archive_with_tab);
	cb_wads_root->SetValue(auto_open_wads_root);
#ifdef __WXMSW__
	cb_update_check->SetValue(update_check);
	cb_update_check_beta->SetValue(update_check_beta);
#endif
	cb_confirm_exit->SetValue(confirm_exit);
	cb_backup_archives->SetValue(backup_archives);
}

/* GeneralPrefsPanel::applyPreferences
 * Applies preference values from the controls to CVARs
 *******************************************************************/
void GeneralPrefsPanel::applyPreferences()
{
	archive_load_data = cb_archive_load->GetValue();
	close_archive_with_tab = cb_archive_close_tab->GetValue();
	auto_open_wads_root = cb_wads_root->GetValue();
#ifdef __WXMSW__
	update_check = cb_update_check->GetValue();
	update_check_beta = cb_update_check_beta->GetValue();
#endif
	confirm_exit = cb_confirm_exit->GetValue();
	backup_archives = cb_backup_archives->GetValue();
}
