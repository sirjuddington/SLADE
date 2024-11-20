
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapDisplaySettingsPanel.cpp
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
#include "MapDisplaySettingsPanel.h"
#include "UI/Controls/NumberSlider.h"
#include "UI/Controls/RadioButtonPanel.h"
#include "UI/Controls/STabCtrl.h"
#include "UI/Layout.h"
#include "UI/WxUtils.h"

using namespace slade;
using namespace ui;


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
EXTERN_CVAR(Int, things_always)
EXTERN_CVAR(Bool, thing_force_dir)
EXTERN_CVAR(Bool, thing_overlay_square)
EXTERN_CVAR(Float, thing_shadow)
EXTERN_CVAR(Int, thing_shape)
EXTERN_CVAR(Bool, thing_sprites)
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
EXTERN_CVAR(Bool, action_lines)
EXTERN_CVAR(Bool, map_show_help)
EXTERN_CVAR(Int, map_tex_filter)
EXTERN_CVAR(Bool, use_zeth_icons)
EXTERN_CVAR(Int, halo_width)
EXTERN_CVAR(Int, grid_64_style)
EXTERN_CVAR(Bool, grid_show_origin)
EXTERN_CVAR(Float, thing_light_intensity)


// -----------------------------------------------------------------------------
//
// MapDisplaySettingsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapDisplaySettingsPanel class constructor
// -----------------------------------------------------------------------------
MapDisplaySettingsPanel::MapDisplaySettingsPanel(wxWindow* parent) : SettingsPanel(parent)
{
	// Create sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Create notebook
	auto tabs = STabCtrl::createControl(this);
	sizer->Add(tabs, wxSizerFlags(1).Expand());

	// Setup tabs
	auto lh = LayoutHelper(this);
	tabs->AddPage(createGeneralPanel(tabs, lh), "General");
	tabs->AddPage(createVerticesPanel(tabs, lh), "Vertices");
	tabs->AddPage(createLinesPanel(tabs, lh), "Lines");
	tabs->AddPage(createThingsPanel(tabs, lh), "Things");
	tabs->AddPage(createSectorsPanel(tabs, lh), "Sectors");

	wxWindowBase::Layout();
}

// -----------------------------------------------------------------------------
// Creates the general tab panel
// -----------------------------------------------------------------------------
wxPanel* MapDisplaySettingsPanel::createGeneralPanel(wxWindow* parent, const LayoutHelper& lh)
{
	auto panel     = new wxPanel(parent, -1);
	auto sz_border = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sz_border);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	sz_border->Add(sizer, lh.sfWithLargeBorder(1).Expand());
	// int row = 0;

	// Create controls
	rbp_crosshair_        = new RadioButtonPanel(panel, { "None", "Small", "Full" }, "Cursor crosshair:");
	rbp_grid_64_          = new RadioButtonPanel(panel, { "None", "Full", "Crosses" }, "64x64 grid:");
	cb_grid_dashed_       = new wxCheckBox(panel, -1, "Dashed grid");
	cb_grid_show_origin_  = new wxCheckBox(panel, -1, "Hilight origin (0,0) on grid");
	cb_line_tabs_always_  = new wxCheckBox(panel, -1, "Always show line direction tabs");
	cb_animate_hilight_   = new wxCheckBox(panel, -1, "Animated hilight");
	cb_animate_selection_ = new wxCheckBox(panel, -1, "Animated selection");
	cb_animate_tagged_    = new wxCheckBox(panel, -1, "Animated tag indicator");
	cb_action_lines_      = new wxCheckBox(panel, -1, "Show action lines");
	cb_show_help_         = new wxCheckBox(panel, -1, "Show help text");

	cb_action_lines_->SetToolTip(
		"Show lines from an object with an action special to the tagged object(s) when highlighted");

	// General
	lh.layoutVertically(
		sizer, { rbp_crosshair_, cb_line_tabs_always_, cb_action_lines_, cb_show_help_ }, wxSizerFlags(0).Expand());

	// Grid
	sizer->AddSpacer(lh.padXLarge());
	sizer->Add(wxutil::createSectionSeparator(panel, "Grid"), lh.sfWithBorder(0, wxBOTTOM).Expand());
	lh.layoutVertically(
		sizer, { rbp_grid_64_, cb_grid_dashed_, cb_grid_show_origin_ }, lh.sfWithBorder(0, wxLEFT).Expand());

	// Animation
	sizer->AddSpacer(lh.padXLarge());
	sizer->Add(wxutil::createSectionSeparator(panel, "Animation"), lh.sfWithBorder(0, wxBOTTOM).Expand());
	lh.layoutVertically(
		sizer, { cb_animate_hilight_, cb_animate_selection_, cb_animate_tagged_ }, lh.sfWithBorder(0, wxLEFT).Expand());

	return panel;
}

// -----------------------------------------------------------------------------
// Creates the vertices tab panel
// -----------------------------------------------------------------------------
wxPanel* MapDisplaySettingsPanel::createVerticesPanel(wxWindow* parent, const LayoutHelper& lh)
{
	auto panel     = new wxPanel(parent, -1);
	auto sz_border = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sz_border);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	sz_border->Add(sizer, lh.sfWithLargeBorder(1).Expand());

	slider_vertex_size_  = new NumberSlider(panel, 2, 16, 1);
	rbp_vertices_always_ = new RadioButtonPanel(panel, { "Hide", "Show", "Fade" }, "When not in vertices mode:");
	cb_vertex_round_     = new wxCheckBox(panel, -1, "Round vertices");

	lh.layoutVertically(
		sizer,
		{ cb_vertex_round_, wxutil::createLabelHBox(panel, "Vertex Size:", slider_vertex_size_), rbp_vertices_always_ },
		wxSizerFlags());

	return panel;
}

// -----------------------------------------------------------------------------
// Creates the lines tab panel
// -----------------------------------------------------------------------------
wxPanel* MapDisplaySettingsPanel::createLinesPanel(wxWindow* parent, const LayoutHelper& lh)
{
	auto panel     = new wxPanel(parent, -1);
	auto sz_border = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sz_border);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	sz_border->Add(sizer, lh.sfWithLargeBorder(1).Expand());

	lh.layoutVertically(
		sizer,
		{ wxutil::createLabelHBox(
			  panel, "Line thickness:", slider_line_width_ = new NumberSlider(panel, 10, 30, 1, true, 10)),
		  cb_line_smooth_ = new wxCheckBox(panel, -1, "Smooth lines"),
		  cb_line_fade_   = new wxCheckBox(panel, -1, "Fade when not in lines mode") },
		wxSizerFlags());

	return panel;
}

// -----------------------------------------------------------------------------
// Creates the things tab panel
// -----------------------------------------------------------------------------
wxPanel* MapDisplaySettingsPanel::createThingsPanel(wxWindow* parent, const LayoutHelper& lh)
{
	auto panel     = new wxPanel(parent, -1);
	auto sz_border = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sz_border);
	auto sizer = new wxGridBagSizer(lh.pad(), lh.pad());
	sz_border->Add(sizer, lh.sfWithLargeBorder(1).Expand());

	auto dp = wxDefaultPosition;
	auto ds = wxDefaultSize;

	// Create controls
	rbp_thing_shape_         = new RadioButtonPanel(panel, { "Round", "Square" });
	cb_thing_sprites_        = new wxCheckBox(panel, -1, "Show Sprites");
	slider_thing_shadow_     = new NumberSlider(panel, 0, 10, 1, true, 10);
	cb_thing_force_dir_      = new wxCheckBox(panel, -1, "Always show direction arrows");
	cb_thing_overlay_square_ = new wxCheckBox(panel, -1, "Force square hilight/selection overlay");
	cb_use_zeth_icons_       = new wxCheckBox(panel, -1, "Use ZETH thing type icons");
	slider_light_intensity_  = new NumberSlider(panel, 1, 10, 1, true, 10);
	rbp_things_always_       = new RadioButtonPanel(panel, { "Hide", "Show", "Fade" }, "When not in things mode:");

	int row = 0;
	sizer->Add(new wxStaticText(panel, -1, "Thing shape: "), { row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(rbp_thing_shape_, { row, 1 }, { 1, 1 }, wxEXPAND);
	sizer->Add(cb_thing_sprites_, { row++, 2 }, { 1, 1 }, wxEXPAND);
	sizer->Add(new wxStaticText(panel, -1, "Thing shadow opacity: "), { row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(slider_thing_shadow_, { row++, 1 }, { 1, 2 }, wxEXPAND);
	sizer->Add(
		new wxStaticText(panel, -1, "Point light preview intensity: "), { row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(slider_light_intensity_, { row++, 1 }, { 1, 2 }, wxEXPAND);
	sizer->Add(cb_thing_force_dir_, { row++, 0 }, { 1, 3 }, wxEXPAND);
	sizer->Add(cb_thing_overlay_square_, { row++, 0 }, { 1, 3 }, wxEXPAND);
	sizer->Add(cb_use_zeth_icons_, { row++, 0 }, { 1, 3 }, wxEXPAND);
	sizer->Add(rbp_things_always_, { row++, 0 }, { 1, 3 }, wxEXPAND);

	return panel;
}

// -----------------------------------------------------------------------------
// Creates the sectors tab panel
// -----------------------------------------------------------------------------
wxPanel* MapDisplaySettingsPanel::createSectorsPanel(wxWindow* parent, const LayoutHelper& lh)
{
	// Add tab
	auto panel     = new wxPanel(parent, -1);
	auto sz_border = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sz_border);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	sz_border->Add(sizer, lh.sfWithLargeBorder(1).Expand());

	lh.layoutVertically(
		sizer,
		{ wxutil::createLabelHBox(
			  panel, "Flat brightness:", slider_flat_brightness_ = new NumberSlider(panel, 0, 10, 1, true, 10)),
		  cb_flat_ignore_light_    = new wxCheckBox(panel, -1, "Flats ignore sector brightness"),
		  cb_sector_hilight_fill_  = new wxCheckBox(panel, -1, "Filled sector hilight"),
		  cb_sector_selected_fill_ = new wxCheckBox(panel, -1, "Filled sector selection"),
		  cb_flat_fade_            = new wxCheckBox(panel, -1, "Fade flats when not in sectors mode") },
		wxSizerFlags());

	return panel;
}

// -----------------------------------------------------------------------------
// Initialises panel controls
// -----------------------------------------------------------------------------
void MapDisplaySettingsPanel::loadSettings()
{
	cb_vertex_round_->SetValue(vertex_round);
	cb_line_smooth_->SetValue(line_smooth);
	cb_line_tabs_always_->SetValue(line_tabs_always);
	rbp_thing_shape_->setSelection(thing_shape);
	cb_thing_sprites_->SetValue(thing_sprites);
	cb_thing_force_dir_->SetValue(thing_force_dir);
	cb_thing_overlay_square_->SetValue(thing_overlay_square);
	cb_flat_ignore_light_->SetValue(flat_ignore_light);
	cb_sector_hilight_fill_->SetValue(sector_hilight_fill);
	cb_sector_selected_fill_->SetValue(sector_selected_fill);
	cb_animate_hilight_->SetValue(map_animate_hilight);
	cb_animate_selection_->SetValue(map_animate_selection);
	cb_animate_tagged_->SetValue(map_animate_tagged);
	rbp_vertices_always_->setSelection(vertices_always);
	rbp_things_always_->setSelection(things_always);
	cb_line_fade_->SetValue(line_fade);
	cb_flat_fade_->SetValue(flat_fade);
	cb_grid_dashed_->SetValue(grid_dashed);
	slider_vertex_size_->setValue(vertex_size);
	slider_line_width_->setDecimalValue(line_width);
	slider_thing_shadow_->setDecimalValue(thing_shadow);
	slider_flat_brightness_->setDecimalValue(flat_brightness);
	rbp_crosshair_->setSelection(map_crosshair);
	cb_action_lines_->SetValue(action_lines);
	cb_show_help_->SetValue(map_show_help);
	// choice_tex_filter_->Select(map_tex_filter);
	cb_use_zeth_icons_->SetValue(use_zeth_icons);
	// slider_halo_width_->SetValue(halo_width);
	rbp_grid_64_->setSelection(grid_64_style);
	cb_grid_show_origin_->SetValue(grid_show_origin);
	slider_light_intensity_->setDecimalValue(thing_light_intensity);
}

// -----------------------------------------------------------------------------
// Applies preference values from the controls to CVARs
// -----------------------------------------------------------------------------
void MapDisplaySettingsPanel::applySettings()
{
	grid_dashed           = cb_grid_dashed_->GetValue();
	vertex_round          = cb_vertex_round_->GetValue();
	vertex_size           = slider_vertex_size_->value();
	line_width            = slider_line_width_->decimalValue();
	line_smooth           = cb_line_smooth_->GetValue();
	line_tabs_always      = cb_line_tabs_always_->GetValue();
	thing_shape           = rbp_thing_shape_->getSelection();
	thing_sprites         = cb_thing_sprites_->GetValue();
	thing_force_dir       = cb_thing_force_dir_->GetValue();
	thing_overlay_square  = cb_thing_overlay_square_->GetValue();
	thing_shadow          = slider_thing_shadow_->decimalValue();
	flat_brightness       = slider_flat_brightness_->decimalValue();
	flat_ignore_light     = cb_flat_ignore_light_->GetValue();
	sector_hilight_fill   = cb_sector_hilight_fill_->GetValue();
	sector_selected_fill  = cb_sector_selected_fill_->GetValue();
	map_animate_hilight   = cb_animate_hilight_->GetValue();
	map_animate_selection = cb_animate_selection_->GetValue();
	map_animate_tagged    = cb_animate_tagged_->GetValue();
	vertices_always       = rbp_vertices_always_->getSelection();
	things_always         = rbp_things_always_->getSelection();
	line_fade             = cb_line_fade_->GetValue();
	flat_fade             = cb_flat_fade_->GetValue();
	map_crosshair         = rbp_crosshair_->getSelection();
	action_lines          = cb_action_lines_->GetValue();
	map_show_help         = cb_show_help_->GetValue();
	// map_tex_filter        = choice_tex_filter_->GetSelection();
	use_zeth_icons = cb_use_zeth_icons_->GetValue();
	// halo_width            = slider_halo_width_->GetValue();
	grid_64_style         = rbp_grid_64_->getSelection();
	grid_show_origin      = cb_grid_show_origin_->GetValue();
	thing_light_intensity = slider_light_intensity_->decimalValue();
}
