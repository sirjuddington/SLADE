
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MapDisplayPrefsPanel.cpp
 * Description: Panel containing preference controls for the map
 *              editor
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
#include "MapEditorPrefsPanel.h"
#include "UI/NumberTextCtrl.h"


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
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


/*******************************************************************
 * MAPEDITORPREFSPANEL CLASS FUNCTIONS
 *******************************************************************/

/* MapEditorPrefsPanel::MapEditorPrefsPanel
 * MapEditorPrefsPanel class constructor
 *******************************************************************/
MapEditorPrefsPanel::MapEditorPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create sizer
	wxBoxSizer* psizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(psizer);

	// Create frame+sizer
	wxStaticBox* frame = new wxStaticBox(this, -1, "Map Editor Preferences");
	wxStaticBoxSizer* sizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	psizer->Add(sizer, 1, wxEXPAND|wxALL, 4);

	// Smooth scroll
	cb_scroll_smooth = new wxCheckBox(this, -1, "Enable smooth scrolling");
	sizer->Add(cb_scroll_smooth, 0, wxEXPAND|wxALL, 4);

	// Clear selection on click
	cb_selection_clear_click = new wxCheckBox(this, -1, "Clear selection when nothing is clicked");
	sizer->Add(cb_selection_clear_click, 0, wxEXPAND|wxALL, 4);

	// Clear selection after dragging
	cb_selection_clear_move = new wxCheckBox(this, -1, "Clear selection after moving (dragging) map elements");
	sizer->Add(cb_selection_clear_move, 0, wxEXPAND|wxALL, 4);

	// Double-click to edit selection properties
	cb_property_edit_dclick = new wxCheckBox(this, -1, "Double-click to edit properties");
	sizer->Add(cb_property_edit_dclick, 0, wxEXPAND|wxALL, 4);

	// Merge undo step
	cb_merge_undo_step = new wxCheckBox(this, -1, "Create a 'Merge' undo level on move/edit map architecture");
	sizer->Add(cb_merge_undo_step, 0, wxEXPAND|wxALL, 4);

	// Auto apply property changes
	cb_props_auto_apply = new wxCheckBox(this, -1, "Automatically apply property panel changes");
	sizer->Add(cb_props_auto_apply, 0, wxEXPAND|wxALL, 4);

	// Auto remove invalid lines
	cb_remove_invalid_lines = new wxCheckBox(this, -1, "Remove any resulting invalid lines on sector delete");
	sizer->Add(cb_remove_invalid_lines, 0, wxEXPAND|wxALL, 4);

	// Merge lines on vertex delete
	cb_merge_lines_vertex_delete = new wxCheckBox(this, -1, "Merge connected lines when deleting a vertex");
	sizer->Add(cb_merge_lines_vertex_delete, 0, wxEXPAND|wxALL, 4);

	// Auto offset on line split
	cb_split_auto_offset = new wxCheckBox(this, -1, "Automatic x-offset on line split");
	sizer->Add(cb_split_auto_offset, 0, wxEXPAND|wxALL, 4);

	// Maximum backups
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxALL, 4);
	hbox->Add(new wxStaticText(this, -1, "Max backups to keep:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
	text_max_backups = new NumberTextCtrl(this);
	hbox->Add(text_max_backups, 0, wxEXPAND);

	Layout();
}

/* MapEditorPrefsPanel::~MapEditorPrefsPanel
 * MapEditorPrefsPanel class destructor
 *******************************************************************/
MapEditorPrefsPanel::~MapEditorPrefsPanel()
{
}

/* MapEditorPrefsPanel::init
 * Initialises panel controls
 *******************************************************************/
void MapEditorPrefsPanel::init()
{
	cb_scroll_smooth->SetValue(scroll_smooth);
	cb_selection_clear_click->SetValue(selection_clear_click);
	cb_selection_clear_move->SetValue(selection_clear_move);
	cb_property_edit_dclick->SetValue(property_edit_dclick);
	cb_merge_undo_step->SetValue(map_merge_undo_step);
	cb_props_auto_apply->SetValue(mobj_props_auto_apply);
	cb_remove_invalid_lines->SetValue(map_remove_invalid_lines);
	cb_merge_lines_vertex_delete->SetValue(map_merge_lines_on_delete_vertex);
	cb_split_auto_offset->SetValue(map_split_auto_offset);
	text_max_backups->setNumber(max_map_backups);
}

/* MapEditorPrefsPanel::applyPreferences
 * Applies preference values from the controls to CVARs
 *******************************************************************/
void MapEditorPrefsPanel::applyPreferences()
{
	scroll_smooth = cb_scroll_smooth->GetValue();
	selection_clear_click = cb_selection_clear_click->GetValue();
	selection_clear_move = cb_selection_clear_move->GetValue();
	property_edit_dclick = cb_property_edit_dclick->GetValue();
	map_merge_undo_step = cb_merge_undo_step->GetValue();
	mobj_props_auto_apply = cb_props_auto_apply->GetValue();
	map_remove_invalid_lines = cb_remove_invalid_lines->GetValue();
	map_merge_lines_on_delete_vertex = cb_merge_lines_vertex_delete->GetValue();
	map_split_auto_offset = cb_split_auto_offset->GetValue();
	max_map_backups = text_max_backups->getNumber();
}
