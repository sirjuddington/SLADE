
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapDisplayPrefsPanel.cpp
// Description: Panel containing preference controls for the map editor 2d mode
//              display
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
#include "MapDisplayPrefsPanel.h"
#include "UI/WxUtils.h"


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
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
EXTERN_CVAR(Bool, grid_show_origin)


// -----------------------------------------------------------------------------
//
// MapDisplayPrefsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapDisplayPrefsPanel class constructor
// -----------------------------------------------------------------------------
MapDisplayPrefsPanel::MapDisplayPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Create notebook
	stc_pages_ = STabCtrl::createControl(this);
	sizer->Add(stc_pages_, 1, wxEXPAND);

	// Setup tabs
	setupGeneralTab();
	setupVerticesTab();
	setupLinesTab();
	setupThingsTab();
	setupFlatsTab();

	wxWindowBase::Layout();
}

// -----------------------------------------------------------------------------
// Sets up the general tab
// -----------------------------------------------------------------------------
void MapDisplayPrefsPanel::setupGeneralTab()
{
	// Add tab
	auto panel = new wxPanel(stc_pages_, -1);
	stc_pages_->AddPage(panel, "General", true);
	auto sz_border = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sz_border);
	auto gb_sizer = new wxGridBagSizer(UI::pad(), UI::pad());
	sz_border->Add(gb_sizer, 1, wxEXPAND | wxALL, UI::padLarge());
	int row = 0;

	// Crosshair
	choice_crosshair_ = new wxChoice(panel, -1);
	choice_crosshair_->Set(WxUtils::arrayString({ "None", "Small", "Full" }));
	gb_sizer->Add(new wxStaticText(panel, -1, "Cursor Crosshair:"), { row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(choice_crosshair_, { row++, 1 }, { 1, 2 }, wxEXPAND);

	// Texture filter
	choice_tex_filter_ = new wxChoice(panel, -1);
	choice_tex_filter_->Set(WxUtils::arrayString({ "None", "Linear", "Linear (Mipmapped)", "None (Mipmapped)" }));
	gb_sizer->Add(new wxStaticText(panel, -1, "Texture Filtering:"), { row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(choice_tex_filter_, { row++, 1 }, { 1, 2 }, wxEXPAND);

	// 64 grid
	choice_grid_64_ = new wxChoice(panel, -1);
	choice_grid_64_->Set(WxUtils::arrayString({ "None", "Full", "Crosses" }));
	gb_sizer->Add(new wxStaticText(panel, -1, "64 Grid:"), { row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(choice_grid_64_, { row++, 1 }, { 1, 2 }, wxEXPAND);

	// Dashed grid
	cb_grid_dashed_ = new wxCheckBox(panel, -1, "Dashed grid");
	gb_sizer->Add(cb_grid_dashed_, { row++, 0 }, { 1, 2 }, wxEXPAND);

	// Hilight origin on grid
	cb_grid_show_origin_ = new wxCheckBox(panel, -1, "Hilight origin (0,0) on grid");
	gb_sizer->Add(cb_grid_show_origin_, { row++, 0 }, { 1, 2 }, wxEXPAND);

	// Always show line direction tabs
	cb_line_tabs_always_ = new wxCheckBox(panel, -1, "Always show line direction tabs");
	gb_sizer->Add(cb_line_tabs_always_, { row++, 0 }, { 1, 2 }, wxEXPAND);

	// Animate hilighted object
	cb_animate_hilight_ = new wxCheckBox(panel, -1, "Animated hilight");
	gb_sizer->Add(cb_animate_hilight_, { row++, 0 }, { 1, 2 }, wxEXPAND);

	// Animate selected objects
	cb_animate_selection_ = new wxCheckBox(panel, -1, "Animated selection");
	gb_sizer->Add(cb_animate_selection_, { row++, 0 }, { 1, 2 }, wxEXPAND);

	// Animate tagged objects
	cb_animate_tagged_ = new wxCheckBox(panel, -1, "Animated tag indicator");
	gb_sizer->Add(cb_animate_tagged_, { row++, 0 }, { 1, 2 }, wxEXPAND);

	// Show action lines
	cb_action_lines_ = new wxCheckBox(panel, -1, "Show Action Lines");
	cb_action_lines_->SetToolTip(
		"Show lines from an object with an action special to the tagged object(s) when highlighted");
	gb_sizer->Add(cb_action_lines_, { row++, 0 }, { 1, 2 }, wxEXPAND);

	// Show help text
	cb_show_help_ = new wxCheckBox(panel, -1, "Show Help Text");
	gb_sizer->Add(cb_show_help_, { row++, 0 }, { 1, 2 }, wxEXPAND);

	gb_sizer->AddGrowableCol(1, 1);
}

// -----------------------------------------------------------------------------
// Sets up the vertices tab
// -----------------------------------------------------------------------------
void MapDisplayPrefsPanel::setupVerticesTab()
{
	// Add tab
	auto panel = new wxPanel(stc_pages_, -1);
	stc_pages_->AddPage(panel, "Vertices");
	auto sz_border = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sz_border);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	sz_border->Add(sizer, 1, wxEXPAND | wxALL, UI::padLarge());

	slider_vertex_size_ = new wxSlider(panel, -1, vertex_size, 2, 16, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS);
	choice_vertices_always_ = new wxChoice(panel, -1);
	choice_vertices_always_->Set(WxUtils::arrayString({ "Hide", "Show", "Fade" }));
	cb_vertex_round_ = new wxCheckBox(panel, -1, "Round vertices");

	WxUtils::layoutVertically(
		sizer,
		{ WxUtils::createLabelHBox(panel, "Vertex Size:", slider_vertex_size_),
		  WxUtils::createLabelHBox(panel, "When not in vertices mode:", choice_vertices_always_),
		  cb_vertex_round_ },
		wxSizerFlags(0).Expand());
}

// -----------------------------------------------------------------------------
// Sets up the lines tab
// -----------------------------------------------------------------------------
void MapDisplayPrefsPanel::setupLinesTab()
{
	// Add tab
	auto panel = new wxPanel(stc_pages_, -1);
	stc_pages_->AddPage(panel, "Lines");
	auto sz_border = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sz_border);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	sz_border->Add(sizer, 1, wxEXPAND | wxALL, UI::padLarge());

	WxUtils::layoutVertically(
		sizer,
		{ WxUtils::createLabelHBox(
			  panel,
			  "Line width:",
			  slider_line_width_ = new wxSlider(
				  panel, -1, line_width * 10, 10, 30, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS)),
		  cb_line_smooth_ = new wxCheckBox(panel, -1, "Smooth lines"),
		  cb_line_fade_   = new wxCheckBox(panel, -1, "Fade when not in lines mode") },
		wxSizerFlags(0).Expand());
}

// -----------------------------------------------------------------------------
// Sets up the things tab
// -----------------------------------------------------------------------------
void MapDisplayPrefsPanel::setupThingsTab()
{
	// Add tab
	auto panel = new wxPanel(stc_pages_, -1);
	stc_pages_->AddPage(panel, "Things");
	auto sz_border = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sz_border);
	auto gb_sizer = new wxGridBagSizer(UI::pad(), UI::pad());
	sz_border->Add(gb_sizer, 1, wxEXPAND | wxALL, UI::padLarge());
	int row = 0;

	// Thing style
	gb_sizer->Add(new wxStaticText(panel, -1, "Thing style: "), { row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	choice_thing_drawtype_ = new wxChoice(panel, -1);
	choice_thing_drawtype_->Set(
		WxUtils::arrayString({ "Square", "Round", "Sprite", "Square + Sprite", "Framed Sprite" }));
	gb_sizer->Add(choice_thing_drawtype_, { row++, 1 }, { 1, 1 }, wxEXPAND);

	// When not in things mode
	gb_sizer->Add(
		new wxStaticText(panel, -1, "When not in things mode: "), { row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	choice_things_always_ = new wxChoice(panel, -1);
	choice_things_always_->Set(WxUtils::arrayString({ "Hide", "Show", "Fade" }));
	gb_sizer->Add(choice_things_always_, { row++, 1 }, { 1, 1 }, wxEXPAND);

	// Shadow opacity
	gb_sizer->Add(new wxStaticText(panel, -1, "Thing shadow opacity: "), { row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	slider_thing_shadow_ = new wxSlider(
		panel, -1, thing_shadow * 10, 0, 10, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS);
	gb_sizer->Add(slider_thing_shadow_, { row++, 1 }, { 1, 1 }, wxEXPAND);

	// Arrow opacity
	gb_sizer->Add(
		new wxStaticText(panel, -1, "Thing angle arrow opacity: "), { row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	slider_thing_arrow_alpha_ = new wxSlider(
		panel, -1, thing_shadow * 10, 0, 10, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS);
	gb_sizer->Add(slider_thing_arrow_alpha_, { row++, 1 }, { 1, 1 }, wxEXPAND);

	// Halo width
	gb_sizer->Add(new wxStaticText(panel, -1, "Halo extra width: "), { row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	slider_halo_width_ = new wxSlider(panel, -1, halo_width, 0, 10, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS);
	gb_sizer->Add(slider_halo_width_, { row++, 1 }, { 1, 1 }, wxEXPAND);

	// Always show angles
	cb_thing_force_dir_ = new wxCheckBox(panel, -1, "Always show thing angles");
	gb_sizer->Add(cb_thing_force_dir_, { row++, 0 }, { 1, 2 }, wxEXPAND);

	// Colour angle arrows
	cb_thing_arrow_colour_ = new wxCheckBox(panel, -1, "Colour thing angle arrows");
	gb_sizer->Add(cb_thing_arrow_colour_, { row++, 0 }, { 1, 2 }, wxEXPAND);

	// Force square hilight/selection
	cb_thing_overlay_square_ = new wxCheckBox(panel, -1, "Force square thing hilight/selection overlay");
	gb_sizer->Add(cb_thing_overlay_square_, { row++, 0 }, { 1, 2 }, wxEXPAND);

	// Use zeth icons
	cb_use_zeth_icons_ = new wxCheckBox(panel, -1, "Use ZETH thing type icons");
	gb_sizer->Add(cb_use_zeth_icons_, { row++, 0 }, { 1, 2 }, wxEXPAND);

	gb_sizer->AddGrowableCol(1, 1);
}

// -----------------------------------------------------------------------------
// Sets up the sectors tab
// -----------------------------------------------------------------------------
void MapDisplayPrefsPanel::setupFlatsTab()
{
	// Add tab
	auto panel = new wxPanel(stc_pages_, -1);
	stc_pages_->AddPage(panel, "Sectors");
	auto sz_border = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sz_border);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	sz_border->Add(sizer, 1, wxEXPAND | wxALL, UI::padLarge());

	WxUtils::layoutVertically(
		sizer,
		{ WxUtils::createLabelHBox(
			  panel,
			  "Flat brightness:",
			  slider_flat_brightness_ = new wxSlider(
				  panel, -1, flat_brightness * 10, 0, 10, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS)),
		  cb_flat_ignore_light_    = new wxCheckBox(panel, -1, "Flats ignore sector brightness"),
		  cb_sector_hilight_fill_  = new wxCheckBox(panel, -1, "Filled sector hilight"),
		  cb_sector_selected_fill_ = new wxCheckBox(panel, -1, "Filled sector selection"),
		  cb_flat_fade_            = new wxCheckBox(panel, -1, "Fade flats when not in sectors mode") },
		wxSizerFlags(0).Expand());
}

// -----------------------------------------------------------------------------
// Initialises panel controls
// -----------------------------------------------------------------------------
void MapDisplayPrefsPanel::init()
{
	cb_vertex_round_->SetValue(vertex_round);
	cb_line_smooth_->SetValue(line_smooth);
	cb_line_tabs_always_->SetValue(line_tabs_always);
	choice_thing_drawtype_->SetSelection(thing_drawtype);
	cb_thing_force_dir_->SetValue(thing_force_dir);
	cb_thing_overlay_square_->SetValue(thing_overlay_square);
	cb_thing_arrow_colour_->SetValue(arrow_colour);
	cb_flat_ignore_light_->SetValue(flat_ignore_light);
	cb_sector_hilight_fill_->SetValue(sector_hilight_fill);
	cb_sector_selected_fill_->SetValue(sector_selected_fill);
	cb_animate_hilight_->SetValue(map_animate_hilight);
	cb_animate_selection_->SetValue(map_animate_selection);
	cb_animate_tagged_->SetValue(map_animate_tagged);
	choice_vertices_always_->SetSelection(vertices_always);
	choice_things_always_->SetSelection(things_always);
	cb_line_fade_->SetValue(line_fade);
	cb_flat_fade_->SetValue(flat_fade);
	cb_grid_dashed_->SetValue(grid_dashed);
	slider_vertex_size_->SetValue(vertex_size);
	slider_line_width_->SetValue(line_width * 10);
	slider_thing_shadow_->SetValue(thing_shadow * 10);
	slider_thing_arrow_alpha_->SetValue(arrow_alpha * 10);
	slider_flat_brightness_->SetValue(flat_brightness * 10);
	choice_crosshair_->Select(map_crosshair);
	cb_action_lines_->SetValue(action_lines);
	cb_show_help_->SetValue(map_show_help);
	choice_tex_filter_->Select(map_tex_filter);
	cb_use_zeth_icons_->SetValue(use_zeth_icons);
	slider_halo_width_->SetValue(halo_width);
	choice_grid_64_->SetSelection(grid_64_style);
	cb_grid_show_origin_->SetValue(grid_show_origin);
}

// -----------------------------------------------------------------------------
// Applies preference values from the controls to CVARs
// -----------------------------------------------------------------------------
void MapDisplayPrefsPanel::applyPreferences()
{
	grid_dashed           = cb_grid_dashed_->GetValue();
	vertex_round          = cb_vertex_round_->GetValue();
	vertex_size           = slider_vertex_size_->GetValue();
	line_width            = (float)slider_line_width_->GetValue() * 0.1f;
	line_smooth           = cb_line_smooth_->GetValue();
	line_tabs_always      = cb_line_tabs_always_->GetValue();
	thing_drawtype        = choice_thing_drawtype_->GetSelection();
	thing_force_dir       = cb_thing_force_dir_->GetValue();
	thing_overlay_square  = cb_thing_overlay_square_->GetValue();
	thing_shadow          = (float)slider_thing_shadow_->GetValue() * 0.1f;
	arrow_colour          = cb_thing_arrow_colour_->GetValue();
	arrow_alpha           = (float)slider_thing_arrow_alpha_->GetValue() * 0.1f;
	flat_brightness       = (float)slider_flat_brightness_->GetValue() * 0.1f;
	flat_ignore_light     = cb_flat_ignore_light_->GetValue();
	sector_hilight_fill   = cb_sector_hilight_fill_->GetValue();
	sector_selected_fill  = cb_sector_selected_fill_->GetValue();
	map_animate_hilight   = cb_animate_hilight_->GetValue();
	map_animate_selection = cb_animate_selection_->GetValue();
	map_animate_tagged    = cb_animate_tagged_->GetValue();
	vertices_always       = choice_vertices_always_->GetSelection();
	things_always         = choice_things_always_->GetSelection();
	line_fade             = cb_line_fade_->GetValue();
	flat_fade             = cb_flat_fade_->GetValue();
	map_crosshair         = choice_crosshair_->GetSelection();
	action_lines          = cb_action_lines_->GetValue();
	map_show_help         = cb_show_help_->GetValue();
	map_tex_filter        = choice_tex_filter_->GetSelection();
	use_zeth_icons        = cb_use_zeth_icons_->GetValue();
	halo_width            = slider_halo_width_->GetValue();
	grid_64_style         = choice_grid_64_->GetSelection();
	grid_show_origin      = cb_grid_show_origin_->GetValue();
}
