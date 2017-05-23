
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MapDisplayPrefsPanel.cpp
 * Description: Panel containing preference controls for the map
 *              editor 2d mode display
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
#include "MapDisplayPrefsPanel.h"


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, grid_dashed)
EXTERN_CVAR(Bool, vertex_round)
EXTERN_CVAR(Int, vertex_size)
EXTERN_CVAR(Int, vertices_always)
EXTERN_CVAR(Float, line_width)
EXTERN_CVAR(Bool, line_smooth)
EXTERN_CVAR(Int, thing_drawtype)
EXTERN_CVAR(Int, things_always)
EXTERN_CVAR(Bool, thing_force_dir)
EXTERN_CVAR(Bool, thing_overlay_square)
EXTERN_CVAR(Float, thing_shadow)
EXTERN_CVAR(Float, flat_brightness)
EXTERN_CVAR(Bool, sector_hilight_fill)
EXTERN_CVAR(Bool, sector_selected_fill)
EXTERN_CVAR(Bool, flat_ignore_light)
EXTERN_CVAR(Bool, line_tabs_always)
EXTERN_CVAR(Bool, map_animate_hilight)
EXTERN_CVAR(Bool, map_animate_selection)
EXTERN_CVAR(Bool, map_animate_tagged)
EXTERN_CVAR(Bool, line_fade)
EXTERN_CVAR(Bool, flat_fade)
EXTERN_CVAR(Int, map_crosshair)
EXTERN_CVAR(Bool, arrow_colour)
EXTERN_CVAR(Float, arrow_alpha)
EXTERN_CVAR(Bool, action_lines)
EXTERN_CVAR(Bool, map_show_help)
EXTERN_CVAR(Int, map_tex_filter)
EXTERN_CVAR(Bool, use_zeth_icons)
EXTERN_CVAR(Int, halo_width)
EXTERN_CVAR(Int, grid_64_style)


/*******************************************************************
 * MAPDISPLAYPREFSPANEL CLASS FUNCTIONS
 *******************************************************************/

/* MapDisplayPrefsPanel::MapDisplayPrefsPanel
 * MapDisplayPrefsPanel class constructor
 *******************************************************************/
MapDisplayPrefsPanel::MapDisplayPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create sizer
	wxBoxSizer* psizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(psizer);

	// Create frame+sizer
	wxStaticBox* frame = new wxStaticBox(this, -1, "Map Editor Display Preferences");
	wxStaticBoxSizer* fsizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	psizer->Add(fsizer, 1, wxEXPAND|wxALL, 4);

	// Create notebook
	stc_pages = STabCtrl::createControl(this);
	fsizer->Add(stc_pages, 1, wxEXPAND|wxALL, 4);

	// Setup tabs
	setupGeneralTab();
	setupVerticesTab();
	setupLinesTab();
	setupThingsTab();
	setupFlatsTab();

	Layout();
}

/* MapDisplayPrefsPanel::~MapDisplayPrefsPanel
 * MapDisplayPrefsPanel class destructor
 *******************************************************************/
MapDisplayPrefsPanel::~MapDisplayPrefsPanel()
{
}

/* MapDisplayPrefsPanel::setupGeneralTab
 * Sets up the general tab
 *******************************************************************/
void MapDisplayPrefsPanel::setupGeneralTab()
{
	// Add tab
	wxPanel* panel = new wxPanel(stc_pages, -1);
	stc_pages->AddPage(panel, "General", true);
	wxBoxSizer* sz_border = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sz_border);
	wxGridBagSizer* gb_sizer = new wxGridBagSizer(8, 8);
	sz_border->Add(gb_sizer, 1, wxEXPAND|wxALL, 8);
	int row = 0;

	// Crosshair
	string ch[] = { "None", "Small", "Full" };
	choice_crosshair = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, 3, ch);
	gb_sizer->Add(new wxStaticText(panel, -1, "Cursor Crosshair:"), wxGBPosition(row, 0), wxGBSpan(1, 1), wxALIGN_CENTER_VERTICAL|wxRIGHT);
	gb_sizer->Add(choice_crosshair, wxGBPosition(row++, 1), wxGBSpan(1, 2), wxEXPAND);

	// Texture filter
	string filters[] = { "None", "Linear", "Linear (Mipmapped)", "None (Mipmapped)" };
	choice_tex_filter = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, 4, filters);
	gb_sizer->Add(new wxStaticText(panel, -1, "Texture Filtering:"), wxGBPosition(row, 0), wxGBSpan(1, 1), wxALIGN_CENTER_VERTICAL|wxRIGHT);
	gb_sizer->Add(choice_tex_filter, wxGBPosition(row++, 1), wxGBSpan(1, 2), wxEXPAND);

	// 64 grid
	string grid64[] = { "None", "Full", "Crosses" };
	choice_grid_64 = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, 3, grid64);
	gb_sizer->Add(new wxStaticText(panel, -1, "64 Grid:"), wxGBPosition(row, 0), wxGBSpan(1, 1), wxALIGN_CENTER_VERTICAL | wxRIGHT);
	gb_sizer->Add(choice_grid_64, wxGBPosition(row++, 1), wxGBSpan(1, 2), wxEXPAND);

	// Dashed grid
	cb_grid_dashed = new wxCheckBox(panel, -1, "Dashed grid");
	gb_sizer->Add(cb_grid_dashed, wxGBPosition(row++, 0), wxGBSpan(1, 2), wxEXPAND);

	// Always show line direction tabs
	cb_line_tabs_always = new wxCheckBox(panel, -1, "Always show line direction tabs");
	gb_sizer->Add(cb_line_tabs_always, wxGBPosition(row++, 0), wxGBSpan(1, 2), wxEXPAND);

	// Animate hilighted object
	cb_animate_hilight = new wxCheckBox(panel, -1, "Animated hilight");
	gb_sizer->Add(cb_animate_hilight, wxGBPosition(row++, 0), wxGBSpan(1, 2), wxEXPAND);

	// Animate selected objects
	cb_animate_selection = new wxCheckBox(panel, -1, "Animated selection");
	gb_sizer->Add(cb_animate_selection, wxGBPosition(row++, 0), wxGBSpan(1, 2), wxEXPAND);

	// Animate tagged objects
	cb_animate_tagged = new wxCheckBox(panel, -1, "Animated tag indicator");
	gb_sizer->Add(cb_animate_tagged, wxGBPosition(row++, 0), wxGBSpan(1, 2), wxEXPAND);

	// Show action lines
	cb_action_lines = new wxCheckBox(panel, -1, "Show Action Lines");
	cb_action_lines->SetToolTip("Show lines from an object with an action special to the tagged object(s) when highlighted");
	gb_sizer->Add(cb_action_lines, wxGBPosition(row++, 0), wxGBSpan(1, 2), wxEXPAND);

	// Show help text
	cb_show_help = new wxCheckBox(panel, -1, "Show Help Text");
	gb_sizer->Add(cb_show_help, wxGBPosition(row++, 0), wxGBSpan(1, 2), wxEXPAND);

	gb_sizer->AddGrowableCol(1, 1);
}

/* MapDisplayPrefsPanel::setupVerticesTab
 * Sets up the vertices tab
 *******************************************************************/
void MapDisplayPrefsPanel::setupVerticesTab()
{
	// Add tab
	wxPanel* panel = new wxPanel(stc_pages, -1);
	stc_pages->AddPage(panel, "Vertices");
	wxBoxSizer* sz_border = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sz_border);
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sz_border->Add(sizer, 1, wxEXPAND|wxALL, 4);

	// Vertex size
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxALL, 4);
	hbox->Add(new wxStaticText(panel, -1, "Vertex size: "), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 2);
	slider_vertex_size = new wxSlider(panel, -1, vertex_size, 2, 16, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS);
	hbox->Add(slider_vertex_size, 1, wxEXPAND);

	// When not in vertices mode
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxALL, 4);
	hbox->Add(new wxStaticText(panel, -1, "When not in vertices mode: "), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 2);
	string nonmodeshow[] = { "Hide", "Show", "Fade" };
	choice_vertices_always = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, 3, nonmodeshow);
	hbox->Add(choice_vertices_always, 1, wxEXPAND);

	// Round vertices
	cb_vertex_round = new wxCheckBox(panel, -1, "Round vertices");
	sizer->Add(cb_vertex_round, 0, wxEXPAND|wxALL, 4);
}

/* MapDisplayPrefsPanel::setupLinesTab
 * Sets up the lines tab
 *******************************************************************/
void MapDisplayPrefsPanel::setupLinesTab()
{
	// Add tab
	wxPanel* panel = new wxPanel(stc_pages, -1);
	stc_pages->AddPage(panel, "Lines");
	wxBoxSizer* sz_border = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sz_border);
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sz_border->Add(sizer, 1, wxEXPAND|wxALL, 4);

	// Line width
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxALL, 4);
	hbox->Add(new wxStaticText(panel, -1, "Line width: "), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 2);
	slider_line_width = new wxSlider(panel, -1, line_width*10, 10, 30, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS);
	hbox->Add(slider_line_width, 1, wxEXPAND);

	// Smooth lines
	cb_line_smooth = new wxCheckBox(panel, -1, "Smooth lines");
	sizer->Add(cb_line_smooth, 0, wxEXPAND|wxALL, 4);

	// Fade when not in lines mode
	cb_line_fade = new wxCheckBox(panel, -1, "Fade when not in lines mode");
	sizer->Add(cb_line_fade, 0, wxEXPAND|wxALL, 4);
}

/* MapDisplayPrefsPanel::setupThingsTab
 * Sets up the things tab
 *******************************************************************/
void MapDisplayPrefsPanel::setupThingsTab()
{
	// Add tab
	wxPanel* panel = new wxPanel(stc_pages, -1);
	stc_pages->AddPage(panel, "Things");
	wxBoxSizer* sz_border = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sz_border);
	wxGridBagSizer* gb_sizer = new wxGridBagSizer(8, 8);
	sz_border->Add(gb_sizer, 1, wxEXPAND|wxALL, 8);
	int row = 0;

	// Thing style
	gb_sizer->Add(new wxStaticText(panel, -1, "Thing style: "), wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	string t_types[] = { "Square", "Round", "Sprite", "Square + Sprite", "Framed Sprite" };
	choice_thing_drawtype = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, 5, t_types);
	gb_sizer->Add(choice_thing_drawtype, wxGBPosition(row++, 1), wxDefaultSpan, wxEXPAND);

	// When not in things mode
	gb_sizer->Add(new wxStaticText(panel, -1, "When not in things mode: "), wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	string nonmodeshow[] = { "Hide", "Show", "Fade" };
	choice_things_always = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, 3, nonmodeshow);
	gb_sizer->Add(choice_things_always, wxGBPosition(row++, 1), wxDefaultSpan, wxEXPAND);

	// Shadow opacity
	gb_sizer->Add(new wxStaticText(panel, -1, "Thing shadow opacity: "), wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	slider_thing_shadow = new wxSlider(panel, -1, thing_shadow*10, 0, 10, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS);
	gb_sizer->Add(slider_thing_shadow, wxGBPosition(row++, 1), wxDefaultSpan, wxEXPAND);

	// Arrow opacity
	gb_sizer->Add(new wxStaticText(panel, -1, "Thing angle arrow opacity: "), wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	slider_thing_arrow_alpha = new wxSlider(panel, -1, thing_shadow*10, 0, 10, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS);
	gb_sizer->Add(slider_thing_arrow_alpha, wxGBPosition(row++, 1), wxDefaultSpan, wxEXPAND);

	// Halo width
	gb_sizer->Add(new wxStaticText(panel, -1, "Halo extra width: "), wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	slider_halo_width = new wxSlider(panel, -1, halo_width, 0, 10, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS);
	gb_sizer->Add(slider_halo_width, wxGBPosition(row++, 1), wxDefaultSpan, wxEXPAND);

	// Always show angles
	cb_thing_force_dir = new wxCheckBox(panel, -1, "Always show thing angles");
	gb_sizer->Add(cb_thing_force_dir, wxGBPosition(row++, 0), wxGBSpan(1, 2), wxEXPAND);

	// Colour angle arrows
	cb_thing_arrow_colour = new wxCheckBox(panel, -1, "Colour thing angle arrows");
	gb_sizer->Add(cb_thing_arrow_colour, wxGBPosition(row++, 0), wxGBSpan(1, 2), wxEXPAND);

	// Force square hilight/selection
	cb_thing_overlay_square = new wxCheckBox(panel, -1, "Force square thing hilight/selection overlay");
	gb_sizer->Add(cb_thing_overlay_square, wxGBPosition(row++, 0), wxGBSpan(1, 2), wxEXPAND);

	// Use zeth icons
	cb_use_zeth_icons = new wxCheckBox(panel, -1, "Use ZETH thing type icons");
	gb_sizer->Add(cb_use_zeth_icons, wxGBPosition(row++, 0), wxGBSpan(1, 2), wxEXPAND);

	gb_sizer->AddGrowableCol(1, 1);
}

/* MapDisplayPrefsPanel::setupFlatsTab
 * Sets up the sectors tab
 *******************************************************************/
void MapDisplayPrefsPanel::setupFlatsTab()
{
	// Add tab
	wxPanel* panel = new wxPanel(stc_pages, -1);
	stc_pages->AddPage(panel, "Sectors");
	wxBoxSizer* sz_border = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sz_border);
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sz_border->Add(sizer, 1, wxEXPAND|wxALL, 4);

	// Flat brightness
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxALL, 4);
	hbox->Add(new wxStaticText(panel, -1, "Flat brightness: "), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 2);
	slider_flat_brightness = new wxSlider(panel, -1, flat_brightness*10, 0, 10, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS);
	hbox->Add(slider_flat_brightness, 1, wxEXPAND);

	// Ignore sector light
	cb_flat_ignore_light = new wxCheckBox(panel, -1, "Flats ignore sector brightness");
	sizer->Add(cb_flat_ignore_light, 0, wxEXPAND|wxALL, 4);

	// Fill sector hilight
	cb_sector_hilight_fill = new wxCheckBox(panel, -1, "Filled sector hilight");
	sizer->Add(cb_sector_hilight_fill, 0, wxEXPAND|wxALL, 4);

	// Fill sector selection
	cb_sector_selected_fill = new wxCheckBox(panel, -1, "Filled sector selection");
	sizer->Add(cb_sector_selected_fill, 0, wxEXPAND|wxALL, 4);

	// Fade when not in sectors mode
	cb_flat_fade = new wxCheckBox(panel, -1, "Fade flats when not in sectors mode");
	sizer->Add(cb_flat_fade, 0, wxEXPAND|wxALL, 4);
}

/* MapDisplayPrefsPanel::init
 * Initialises panel controls
 *******************************************************************/
void MapDisplayPrefsPanel::init()
{
	cb_vertex_round->SetValue(vertex_round);
	cb_line_smooth->SetValue(line_smooth);
	cb_line_tabs_always->SetValue(line_tabs_always);
	choice_thing_drawtype->SetSelection(thing_drawtype);
	cb_thing_force_dir->SetValue(thing_force_dir);
	cb_thing_overlay_square->SetValue(thing_overlay_square);
	cb_thing_arrow_colour->SetValue(arrow_colour);
	cb_flat_ignore_light->SetValue(flat_ignore_light);
	cb_sector_hilight_fill->SetValue(sector_hilight_fill);
	cb_sector_selected_fill->SetValue(sector_selected_fill);
	cb_animate_hilight->SetValue(map_animate_hilight);
	cb_animate_selection->SetValue(map_animate_selection);
	cb_animate_tagged->SetValue(map_animate_tagged);
	choice_vertices_always->SetSelection(vertices_always);
	choice_things_always->SetSelection(things_always);
	cb_line_fade->SetValue(line_fade);
	cb_flat_fade->SetValue(flat_fade);
	cb_grid_dashed->SetValue(grid_dashed);
	slider_vertex_size->SetValue(vertex_size);
	slider_line_width->SetValue(line_width * 10);
	slider_thing_shadow->SetValue(thing_shadow * 10);
	slider_thing_arrow_alpha->SetValue(arrow_alpha * 10);
	slider_flat_brightness->SetValue(flat_brightness * 10);
	choice_crosshair->Select(map_crosshair);
	cb_action_lines->SetValue(action_lines);
	cb_show_help->SetValue(map_show_help);
	choice_tex_filter->Select(map_tex_filter);
	cb_use_zeth_icons->SetValue(use_zeth_icons);
	slider_halo_width->SetValue(halo_width);
	choice_grid_64->SetSelection(grid_64_style);
}

/* MapDisplayPrefsPanel::applyPreferences
 * Applies preference values from the controls to CVARs
 *******************************************************************/
void MapDisplayPrefsPanel::applyPreferences()
{
	grid_dashed = cb_grid_dashed->GetValue();
	vertex_round = cb_vertex_round->GetValue();
	vertex_size = slider_vertex_size->GetValue();
	line_width = (float)slider_line_width->GetValue() * 0.1f;
	line_smooth = cb_line_smooth->GetValue();
	line_tabs_always = cb_line_tabs_always->GetValue();
	thing_drawtype = choice_thing_drawtype->GetSelection();
	thing_force_dir = cb_thing_force_dir->GetValue();
	thing_overlay_square = cb_thing_overlay_square->GetValue();
	thing_shadow = (float)slider_thing_shadow->GetValue() * 0.1f;
	arrow_colour = cb_thing_arrow_colour->GetValue();
	arrow_alpha = (float)slider_thing_arrow_alpha->GetValue() * 0.1f;
	flat_brightness = (float)slider_flat_brightness->GetValue() * 0.1f;
	flat_ignore_light = cb_flat_ignore_light->GetValue();
	sector_hilight_fill = cb_sector_hilight_fill->GetValue();
	sector_selected_fill = cb_sector_selected_fill->GetValue();
	map_animate_hilight = cb_animate_hilight->GetValue();
	map_animate_selection = cb_animate_selection->GetValue();
	map_animate_tagged = cb_animate_tagged->GetValue();
	vertices_always = choice_vertices_always->GetSelection();
	things_always = choice_things_always->GetSelection();
	line_fade = cb_line_fade->GetValue();
	flat_fade = cb_flat_fade->GetValue();
	map_crosshair = choice_crosshair->GetSelection();
	action_lines = cb_action_lines->GetValue();
	map_show_help = cb_show_help->GetValue();
	map_tex_filter = choice_tex_filter->GetSelection();
	use_zeth_icons = cb_use_zeth_icons->GetValue();
	halo_width = slider_halo_width->GetValue();
	grid_64_style = choice_grid_64->GetSelection();
}
