
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapEditorPrefsPanel.cpp
// Description: Panel containing preference controls for the map editor
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
#include "MapEditorPrefsPanel.h"
#include "UI/Controls/NumberTextCtrl.h"
#include "UI/WxUtils.h"


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, scroll_smooth)
EXTERN_CVAR(Bool, selection_clear_click)
EXTERN_CVAR(Bool, selection_clear_move)
EXTERN_CVAR(Bool, property_edit_dclick)
EXTERN_CVAR(Bool, map_merge_undo_step)
EXTERN_CVAR(Bool, mobj_props_auto_apply)
EXTERN_CVAR(Bool, map_remove_invalid_lines)
EXTERN_CVAR(Int, max_map_backups)
EXTERN_CVAR(Bool, map_merge_lines_on_delete_vertex)
EXTERN_CVAR(Bool, map_split_auto_offset)


// -----------------------------------------------------------------------------
//
// MapEditorPrefsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapEditorPrefsPanel class constructor
// -----------------------------------------------------------------------------
MapEditorPrefsPanel::MapEditorPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	WxUtils::layoutVertically(
		sizer,
		vector<wxObject*>{
			cb_scroll_smooth_         = new wxCheckBox(this, -1, "Enable smooth scrolling"),
			cb_selection_clear_click_ = new wxCheckBox(this, -1, "Clear selection when nothing is clicked"),
			cb_selection_clear_move_ = new wxCheckBox(this, -1, "Clear selection after moving (dragging) map elements"),
			cb_property_edit_dclick_ = new wxCheckBox(this, -1, "Double-click to edit properties"),
			cb_merge_undo_step_ = new wxCheckBox(this, -1, "Create a 'Merge' undo level on move/edit map architecture"),
			cb_props_auto_apply_     = new wxCheckBox(this, -1, "Automatically apply property panel changes"),
			cb_remove_invalid_lines_ = new wxCheckBox(this, -1, "Remove any resulting invalid lines on sector delete"),
			cb_merge_lines_vertex_delete_ = new wxCheckBox(this, -1, "Merge connected lines when deleting a vertex"),
			cb_split_auto_offset_         = new wxCheckBox(this, -1, "Automatic x-offset on line split"),
			WxUtils::createLabelHBox(this, "Max backups to keep:", text_max_backups_ = new NumberTextCtrl(this)) },
		wxSizerFlags(0).Expand());

	Layout();
}

// -----------------------------------------------------------------------------
// MapEditorPrefsPanel class destructor
// -----------------------------------------------------------------------------
MapEditorPrefsPanel::~MapEditorPrefsPanel() {}

// -----------------------------------------------------------------------------
// Initialises panel controls
// -----------------------------------------------------------------------------
void MapEditorPrefsPanel::init()
{
	cb_scroll_smooth_->SetValue(scroll_smooth);
	cb_selection_clear_click_->SetValue(selection_clear_click);
	cb_selection_clear_move_->SetValue(selection_clear_move);
	cb_property_edit_dclick_->SetValue(property_edit_dclick);
	cb_merge_undo_step_->SetValue(map_merge_undo_step);
	cb_props_auto_apply_->SetValue(mobj_props_auto_apply);
	cb_remove_invalid_lines_->SetValue(map_remove_invalid_lines);
	cb_merge_lines_vertex_delete_->SetValue(map_merge_lines_on_delete_vertex);
	cb_split_auto_offset_->SetValue(map_split_auto_offset);
	text_max_backups_->setNumber(max_map_backups);
}

// -----------------------------------------------------------------------------
// Applies preference values from the controls to CVARs
// -----------------------------------------------------------------------------
void MapEditorPrefsPanel::applyPreferences()
{
	scroll_smooth                    = cb_scroll_smooth_->GetValue();
	selection_clear_click            = cb_selection_clear_click_->GetValue();
	selection_clear_move             = cb_selection_clear_move_->GetValue();
	property_edit_dclick             = cb_property_edit_dclick_->GetValue();
	map_merge_undo_step              = cb_merge_undo_step_->GetValue();
	mobj_props_auto_apply            = cb_props_auto_apply_->GetValue();
	map_remove_invalid_lines         = cb_remove_invalid_lines_->GetValue();
	map_merge_lines_on_delete_vertex = cb_merge_lines_vertex_delete_->GetValue();
	map_split_auto_offset            = cb_split_auto_offset_->GetValue();
	max_map_backups                  = text_max_backups_->number();
}
