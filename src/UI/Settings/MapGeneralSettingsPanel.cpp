
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
#include "Map3DSettingsPanel.h"
#include "NodeBuildersSettingsPanel.h"
#include "UI/Controls/NumberTextCtrl.h"
#include "UI/Controls/RadioButtonPanel.h"
#include "UI/Controls/STabCtrl.h"
#include "UI/Layout.h"
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
	cb_selection_clear_click_->SetValue(CVar::getBool("selection_clear_click"));
	cb_selection_clear_move_->SetValue(CVar::getBool("selection_clear_move"));
	cb_property_edit_dclick_->SetValue(CVar::getBool("property_edit_dclick"));
	cb_merge_undo_step_->SetValue(CVar::getBool("map_merge_undo_step"));
	cb_props_auto_apply_->SetValue(CVar::getBool("mobj_props_auto_apply"));
	cb_remove_invalid_lines_->SetValue(CVar::getBool("map_remove_invalid_lines"));
	cb_merge_lines_vertex_delete_->SetValue(CVar::getBool("map_merge_lines_on_delete_vertex"));
	cb_split_auto_offset_->SetValue(CVar::getBool("map_split_auto_offset"));
	text_max_backups_->setNumber(CVar::getInt("max_map_backups"));
	cb_save_archive_with_map_->SetValue(CVar::getBool("save_archive_with_map"));
	cb_3d_invert_y_->SetValue(CVar::getBool("map3d_mlook_invert_y"));
	rbp_compress_sides_->setSelection(CVar::getInt("map_compress_sides"));

	nodebuilders_panel_->loadSettings();
}

// -----------------------------------------------------------------------------
// Applies settings from the panel controls to cvars
// -----------------------------------------------------------------------------
void MapGeneralSettingsPanel::applySettings()
{
	CVar::setBool("selection_clear_click", cb_selection_clear_click_->GetValue());
	CVar::setBool("selection_clear_move", cb_selection_clear_move_->GetValue());
	CVar::setBool("property_edit_dclick", cb_property_edit_dclick_->GetValue());
	CVar::setBool("map_merge_undo_step", cb_merge_undo_step_->GetValue());
	CVar::setBool("mobj_props_auto_apply", cb_props_auto_apply_->GetValue());
	CVar::setBool("map_remove_invalid_lines", cb_remove_invalid_lines_->GetValue());
	CVar::setBool("map_merge_lines_on_delete_vertex", cb_merge_lines_vertex_delete_->GetValue());
	CVar::setBool("map_split_auto_offset", cb_split_auto_offset_->GetValue());
	CVar::setInt("max_map_backups", text_max_backups_->number());
	CVar::setBool("save_archive_with_map", cb_save_archive_with_map_->GetValue());
	CVar::setBool("map3d_mlook_invert_y", cb_3d_invert_y_->GetValue());
	CVar::setInt("map_compress_sides", rbp_compress_sides_->getSelection());

	nodebuilders_panel_->applySettings();
}

// -----------------------------------------------------------------------------
// Creates the general tab panel
// -----------------------------------------------------------------------------
wxPanel* MapGeneralSettingsPanel::createGeneralPanel(wxWindow* parent)
{
	auto panel     = new wxPanel(parent);
	auto lh        = LayoutHelper(panel);
	auto sz_border = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sz_border);

	// Create controls
	cb_save_archive_with_map_ = new wxCheckBox(panel, -1, wxS("When saving a map, also save its parent archive"));
	rbp_compress_sides_ = new RadioButtonPanel(panel, { "Never", "When necessary", "Always" }, "Compress SIDEDEFS:");
	cb_selection_clear_click_ = new wxCheckBox(panel, -1, wxS("Clear selection when nothing is clicked"));
	cb_selection_clear_move_  = new wxCheckBox(panel, -1, wxS("Clear selection after moving (dragging) map elements"));
	cb_property_edit_dclick_  = new wxCheckBox(panel, -1, wxS("Double-click to edit properties"));
	cb_merge_undo_step_  = new wxCheckBox(panel, -1, wxS("Create a 'Merge' undo level on move/edit map architecture"));
	cb_props_auto_apply_ = new wxCheckBox(panel, -1, wxS("Automatically apply property panel changes"));
	cb_remove_invalid_lines_ = new wxCheckBox(panel, -1, wxS("Remove any resulting invalid lines on sector delete"));
	cb_merge_lines_vertex_delete_ = new wxCheckBox(panel, -1, wxS("Merge lines when deleting a vertex"));
	cb_split_auto_offset_         = new wxCheckBox(panel, -1, wxS("Automatically offset split lines"));
	text_max_backups_             = new NumberTextCtrl(panel);
	cb_3d_invert_y_               = new wxCheckBox(panel, -1, wxS("Invert mouse Y axis in 3D mode"));

	// Layout
	auto sizer = new wxBoxSizer(wxVERTICAL);
	sz_border->Add(sizer, lh.sfWithLargeBorder(1).Expand());

	sizer->Add(cb_save_archive_with_map_, lh.sfWithBorder(0, wxBOTTOM).Expand());
	sizer->Add(rbp_compress_sides_, lh.sfWithBorder(0, wxBOTTOM).Expand());

	// Selection
	sizer->Add(wxutil::createSectionSeparator(panel, "Selection"), lh.sfWithBorder(0, wxBOTTOM).Expand());
	lh.layoutVertically(sizer, { cb_selection_clear_click_, cb_selection_clear_move_ }, lh.sfWithBorder(0, wxLEFT));

	// Editing
	sizer->AddSpacer(lh.padXLarge());
	sizer->Add(wxutil::createSectionSeparator(panel, "Editing"), lh.sfWithBorder(0, wxBOTTOM).Expand());
	lh.layoutVertically(
		sizer,
		{ cb_merge_undo_step_,
		  cb_remove_invalid_lines_,
		  cb_merge_lines_vertex_delete_,
		  cb_split_auto_offset_,
		  cb_props_auto_apply_ },
		lh.sfWithBorder(0, wxLEFT));

	// Controls
	sizer->AddSpacer(lh.padXLarge());
	sizer->Add(wxutil::createSectionSeparator(panel, "Controls"), lh.sfWithBorder(0, wxBOTTOM).Expand());
	lh.layoutVertically(sizer, { cb_property_edit_dclick_, cb_3d_invert_y_ }, lh.sfWithBorder(0, wxLEFT));

	// Backups
	sizer->AddSpacer(lh.padXLarge());
	sizer->Add(wxutil::createSectionSeparator(panel, "Backups"), lh.sfWithBorder(0, wxBOTTOM).Expand());
	lh.layoutVertically(
		sizer,
		{ wxutil::createLabelHBox(panel, "Max backups to keep:", text_max_backups_) },
		lh.sfWithBorder(0, wxLEFT));

	return panel;
}
