
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    EditingSettingsPanel.cpp
// Description: Panel containing editor related settings controls
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
#include "EditingSettingsPanel.h"
#include "ExternalEditorsSettingsPanel.h"
#include "UI/Controls/STabCtrl.h"
#include "UI/Controls/SettingsTable.h"
#include "UI/UI.h"
#include "UI/WxUtils.h"

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// EditingSettingsPanel Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// EditingSettingsPanel class constructor
// -----------------------------------------------------------------------------
EditingSettingsPanel::EditingSettingsPanel(wxWindow* parent) : SettingsPanel(parent)
{
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	ext_editors_panel_ = new ExternalEditorsSettingsPanel(this);

	auto tabs = STabCtrl::createControl(this);
	tabs->AddPage(createArchiveEditorPanel(tabs), wxS("Archive Editor"));
	tabs->AddPage(wxutil::createPadPanel(tabs, ext_editors_panel_, padLarge()), wxS("External Editors"));
	sizer->Add(tabs, wxSizerFlags(1).Expand());
}

// -----------------------------------------------------------------------------
// Loads settings from cvars into the controls
// -----------------------------------------------------------------------------
void EditingSettingsPanel::loadSettings()
{
	settings_table_->loadSettings();
	ext_editors_panel_->loadSettings();
}

// -----------------------------------------------------------------------------
// Applies settings from the panel controls to cvars
// -----------------------------------------------------------------------------
void EditingSettingsPanel::applySettings()
{
	settings_table_->applySettings();
	ext_editors_panel_->applySettings();
}

// -----------------------------------------------------------------------------
// Creates the archive editor settings panel
// -----------------------------------------------------------------------------
wxPanel* EditingSettingsPanel::createArchiveEditorPanel(wxWindow* parent)
{
	settings_table_ = new SettingsTable(parent, true, "Behaviour");

	// Editor behaviour
	settings_table_->addCheckBox(
		"Automatically replace entries with same name as drag-and-dropped files", "auto_entry_replace");
	settings_table_->addCheckBox("Ignore directories when filtering by name", "elist_filter_dirs");
	settings_table_->addCheckBox("Show confirmation dialog on deleting an entry", "confirm_entry_delete");
	settings_table_->addCheckBox("Show confirmation dialog on reverting entry changes", "confirm_entry_revert");
	settings_table_->addRadioButtons(
		"Action on unsaved entry changes", "autosave_entry_changes", { "Don't Save", "Save", "Ask" });
	settings_table_->addRadioButtons(
		"Action on external directory changes",
		"dir_archive_change_action",
		{ "Ignore Changes", "Apply Changes", "Ask" });

	// Entry naming
	settings_table_->addSectionSeparator("Entry Naming");
	settings_table_->addCheckBox("Force uppercase entry names in Wad Archives", "wad_force_uppercase");
	settings_table_->addCheckBox("Use percent encoding if needed outside of Wad Archives", "percent_encoding");

	return settings_table_;
}
