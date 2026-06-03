
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapGeneralSettingsPanel.cpp
// Description: Panel containing general map editor settings controls
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
#include "MapGeneralSettingsPanel.h"
#include "NodeBuildersSettingsPanel.h"
#include "UI/Controls/NumberTextCtrl.h"
#include "UI/Controls/STabCtrl.h"
#include "UI/Controls/SettingsTable.h"
#include "UI/UI.h"
#include "UI/WxUtils.h"

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// MapGeneralSettingsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapGeneralSettingsPanel class constructor
// -----------------------------------------------------------------------------
MapGeneralSettingsPanel::MapGeneralSettingsPanel(wxWindow* parent) : SettingsPanel(parent)
{
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	nodebuilders_panel_ = new NodeBuildersSettingsPanel(this);

	auto tabs = STabCtrl::createControl(this);
	tabs->AddPage(createGeneralPanel(tabs), wxS("General"));
	tabs->AddPage(wxutil::createPadPanel(tabs, nodebuilders_panel_, padLarge()), wxS("Node Builders"));
	sizer->Add(tabs, wxSizerFlags(1).Expand());
}

// -----------------------------------------------------------------------------
// Loads settings from cvars into the controls
// -----------------------------------------------------------------------------
void MapGeneralSettingsPanel::loadSettings()
{
	text_max_backups_->setNumber(CVar::getInt("max_map_backups"));
	settings_table_->loadSettings();
	nodebuilders_panel_->loadSettings();
}

// -----------------------------------------------------------------------------
// Applies settings from the panel controls to cvars
// -----------------------------------------------------------------------------
void MapGeneralSettingsPanel::applySettings()
{
	CVar::setInt("max_map_backups", text_max_backups_->number());
	settings_table_->applySettings();
	nodebuilders_panel_->applySettings();
}

// -----------------------------------------------------------------------------
// Creates the general tab panel
// -----------------------------------------------------------------------------
wxPanel* MapGeneralSettingsPanel::createGeneralPanel(wxWindow* parent)
{
	settings_table_ = new SettingsTable(parent);

	settings_table_->addCheckBox("When saving a map, also save its parent archive", "save_archive_with_map");
	settings_table_->addRadioButtons(
		"Compress SIDEDEFS|"
		"Applies to Doom and Hexen format maps only, which are limited to a maximum of 65535 sides",
		"map_compress_sides",
		{ "Never", "When necessary", "Always" });
	settings_table_->addCustomControl(
		"Max backups to keep", text_max_backups_ = new NumberTextCtrl(settings_table_), wxALIGN_CENTER_VERTICAL);

	// Selection
	settings_table_->addSectionSeparator("Selection");
	settings_table_->addCheckBox("Clear selection when nothing is clicked", "selection_clear_click");
	settings_table_->addCheckBox("Clear selection after moving (dragging) map elements", "selection_clear_move");

	// Editing
	settings_table_->addSectionSeparator("Editing");
	settings_table_->addCheckBox("Create a 'Merge' undo level on move/edit map architecture", "map_merge_undo_step");
	settings_table_->addCheckBox("Remove any resulting invalid lines on sector delete", "map_remove_invalid_lines");
	settings_table_->addCheckBox("Merge lines when deleting a vertex", "map_merge_lines_on_delete_vertex");
	settings_table_->addCheckBox("Automatically offset split lines", "map_split_auto_offset");
	settings_table_->addCheckBox("Automatically apply property panel changes", "mobj_props_auto_apply");

	// Controls
	settings_table_->addSectionSeparator("Controls");
	settings_table_->addCheckBox("Double-click to edit properties", "property_edit_dclick");
	settings_table_->addCheckBox("Invert mouse Y axis in 3D mode", "map3d_mlook_invert_y");

	return settings_table_;
}
