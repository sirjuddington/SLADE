
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
	tabs->AddPage(createEditingPanel(tabs), wxS("Editing"));
	tabs->AddPage(wxutil::createPadPanel(tabs, nodebuilders_panel_, padLarge()), wxS("Node Builders"));
	sizer->Add(tabs, wxSizerFlags(1).Expand());
}

// -----------------------------------------------------------------------------
// Loads settings from cvars into the controls
// -----------------------------------------------------------------------------
void MapGeneralSettingsPanel::loadSettings()
{
	text_max_backups_->setNumber(CVar::getInt("max_map_backups"));
	st_general_->loadSettings();
	st_editing_->loadSettings();
	nodebuilders_panel_->loadSettings();
}

// -----------------------------------------------------------------------------
// Applies settings from the panel controls to cvars
// -----------------------------------------------------------------------------
void MapGeneralSettingsPanel::applySettings()
{
	CVar::setInt("max_map_backups", text_max_backups_->number());
	st_general_->applySettings();
	st_editing_->applySettings();
	nodebuilders_panel_->applySettings();
}

// -----------------------------------------------------------------------------
// Creates the general tab panel
// -----------------------------------------------------------------------------
wxPanel* MapGeneralSettingsPanel::createGeneralPanel(wxWindow* parent)
{
	st_general_ = new SettingsTable(parent, true, "Map Saving");

	st_general_->addCheckBox("When saving a map, also save its parent archive", "save_archive_with_map");
	st_general_->addRadioButtons(
		"Compress SIDEDEFS|"
		"Applies to Doom and Hexen format maps only, which are limited to a maximum of 65535 sides",
		"map_compress_sides",
		{ "Never", "When necessary", "Always" });
	st_general_->addCustomControl(
		"Max backups to keep", text_max_backups_ = new NumberTextCtrl(st_general_), wxALIGN_CENTER_VERTICAL);

	// Selection
	st_general_->addSectionSeparator("Selection");
	st_general_->addCheckBox("Clear selection when nothing is clicked", "selection_clear_click");
	st_general_->addCheckBox("Clear selection after moving (dragging) map elements", "selection_clear_move");

	// Controls
	st_general_->addSectionSeparator("Controls");
	st_general_->addCheckBox("Double-click to edit properties", "property_edit_dclick");
	st_general_->addCheckBox("Invert mouse Y axis in 3D mode", "map3d_mlook_invert_y");

	return st_general_;
}

// -----------------------------------------------------------------------------
// Creates the editing tab panel
// -----------------------------------------------------------------------------
wxPanel* MapGeneralSettingsPanel::createEditingPanel(wxWindow* parent)
{
	st_editing_ = new SettingsTable(parent);

	st_editing_->addCheckBox("Automatically apply property panel changes", "mobj_props_auto_apply");

	st_editing_->addSectionSeparator("Map Architecture");
	st_editing_->addCheckBox("Create a 'Merge' undo step on move/edit map architecture", "map_merge_undo_step");
	st_editing_->addCheckBox("Remove any resulting invalid lines after deleting a sector", "map_remove_invalid_lines");
	st_editing_->addCheckBox("Merge lines when deleting a vertex", "map_merge_lines_on_delete_vertex");

	st_editing_->addSectionSeparator("Textures");
	st_editing_->addCheckBox("Automatically adjust texture offsets on split lines", "map_split_auto_offset");
	st_editing_->addCheckBox("Fill in missing textures when changing sector heights", "map_fill_missing_textures");

	return st_editing_;
}
