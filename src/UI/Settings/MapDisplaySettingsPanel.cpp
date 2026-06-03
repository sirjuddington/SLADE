
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapDisplaySettingsPanel.cpp
// Description: Panel containing settings controls for the map editor 2d mode
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
#include "UI/Controls/RadioButtonPanel.h"
#include "UI/Controls/STabCtrl.h"
#include "UI/Controls/SettingsTable.h"
#include "UI/Layout.h"

using namespace slade;
using namespace ui;


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
	tabs->AddPage(createGeneralPanel(tabs, lh), wxS("General"));
	tabs->AddPage(createVerticesPanel(tabs, lh), wxS("Vertices"));
	tabs->AddPage(createLinesPanel(tabs, lh), wxS("Lines"));
	tabs->AddPage(createThingsPanel(tabs, lh), wxS("Things"));
	tabs->AddPage(createSectorsPanel(tabs, lh), wxS("Sectors"));
	tabs->AddPage(create3DPanel(tabs, lh), wxS("3D"));

	wxWindowBase::Layout();
}

// -----------------------------------------------------------------------------
// Creates the general tab panel
// -----------------------------------------------------------------------------
wxPanel* MapDisplaySettingsPanel::createGeneralPanel(wxWindow* parent, const LayoutHelper& lh)
{
	st_general_ = new SettingsTable(parent);

	// General
	st_general_->addRadioButtons("Cursor crosshair", "map2d_crosshair", { "None", "Small", "Full" });
	st_general_->addRadioButtons("Texture filter", "map_tex_filter", { "None", "Linear", "Mipmapped" });
	st_general_->addCheckBox(
		"Show action lines|"
		"Show lines from an object with an action special to the tagged object(s) when highlighted",
		"map2d_action_lines");
	st_general_->addCheckBox("Show help text", "map_show_help");
	st_general_->addCheckBox("Show FPS counter", "map_showfps");

	// Grid
	st_general_->addSectionSeparator("Grid");
	st_general_->addRadioButtons("64x64 grid", "map2d_64grid_style", { "None", "Full", "Crosses" });
	st_general_->addCheckBox("Dashed grid", "map2d_grid_dashed");
	st_general_->addCheckBox("Hilight origin (0,0) on grid", "map2d_grid_show_origin");

	// Animation
	st_general_->addSectionSeparator("Animation");
	st_general_->addCheckBox("Animated hilight", "map_animate_hilight");
	st_general_->addCheckBox("Animated selection", "map_animate_selection");
	st_general_->addCheckBox("Animated tag indicator", "map_animate_tagged");

	return st_general_;
}

// -----------------------------------------------------------------------------
// Creates the vertices tab panel
// -----------------------------------------------------------------------------
wxPanel* MapDisplaySettingsPanel::createVerticesPanel(wxWindow* parent, const LayoutHelper& lh)
{
	st_vertices_ = new SettingsTable(parent);

	st_vertices_->addCheckBox("Round vertices", "map2d_vertex_round");
	st_vertices_->addSlider("Vertex size", "map2d_vertex_size", false, 2, 16, 1);
	st_vertices_->addRadioButtons("When not in vertices mode", "map2d_vertices_always", { "Hide", "Show", "Fade" });

	return st_vertices_;
}

// -----------------------------------------------------------------------------
// Creates the lines tab panel
// -----------------------------------------------------------------------------
wxPanel* MapDisplaySettingsPanel::createLinesPanel(wxWindow* parent, const LayoutHelper& lh)
{
	st_lines_ = new SettingsTable(parent);

	st_lines_->addCheckBox("Smooth lines", "map2d_line_smooth");
	st_lines_->addCheckBox("Always show line direction tabs", "map2d_line_tabs_always");
	st_lines_->addSlider("Line thickness", "map2d_line_width", true, 10, 30, 1, 10);
	st_lines_->addRadioButtons(
		"Fade when not in lines mode", "map2d_line_fade", { "No", "Always", "Vertex mode only" });

	return st_lines_;
}

// -----------------------------------------------------------------------------
// Creates the things tab panel
// -----------------------------------------------------------------------------
wxPanel* MapDisplaySettingsPanel::createThingsPanel(wxWindow* parent, const LayoutHelper& lh)
{
	st_things_ = new SettingsTable(parent);

	st_things_->addRadioButtons("Thing shape", "map2d_thing_shape", { "Round", "Square", "Sprite Only" });
	st_things_->addCheckBox("Show sprites on round or square things", "map2d_thing_sprites");
	st_things_->addSlider("Thing shadow opacity", "map2d_thing_shadow", true, 0, 10, 1, 10);
	st_things_->addCheckBox("Always show direction arrows", "map2d_thing_force_dir");
	st_things_->addCheckBox("Force square hilight/selection overlay", "map2d_thing_overlay_square");
	st_things_->addCheckBox("Use ZETH thing type icons", "use_zeth_icons");
	st_things_->addRadioButtons("When not in things mode", "map2d_things_always", { "Hide", "Show", "Fade" });

	return st_things_;
}

// -----------------------------------------------------------------------------
// Creates the sectors tab panel
// -----------------------------------------------------------------------------
wxPanel* MapDisplaySettingsPanel::createSectorsPanel(wxWindow* parent, const LayoutHelper& lh)
{
	st_sectors_ = new SettingsTable(parent);

	st_sectors_->addSlider("Flat brightness", "map2d_flat_brightness", true, 0, 10, 1, 10);
	st_sectors_->addCheckBox("Flats ignore sector brightness", "map2d_flat_ignore_light");
	st_sectors_->addCheckBox("Filled sector hilight", "map2d_sector_hilight_fill");
	st_sectors_->addCheckBox("Filled sector selection", "map2d_sector_selected_fill");
	st_sectors_->addCheckBox("Fade flats when not in sectors mode", "map2d_flat_fade");

	return st_sectors_;
}

// -----------------------------------------------------------------------------
// Creates the 3D mode tab panel
// -----------------------------------------------------------------------------
wxPanel* MapDisplaySettingsPanel::create3DPanel(wxWindow* parent, const LayoutHelper& lh)
{
	st_3d_ = new SettingsTable(parent);

	st_3d_->addSlider("Camera FOV", "map3d_fov", false, 70, 120, 5);
	st_3d_->addSlider("Max dynamic lights", "map3d_lights_max", false, 32, 512, 32);
	// st_3d_->addCheckBox("Show distance under crosshair", "map3d_crosshair_show_distance");
	st_3d_->addCustomControl(
		"Item highlight:", rbp_3d_highlight_ = new RadioButtonPanel(st_3d_, { "Fill", "Outline", "Both" }));
	st_3d_->addCheckBox("Fake contrast on orthogonal walls", "map3d_fake_contrast");

	return st_3d_;
}

// -----------------------------------------------------------------------------
// Loads settings from cvars into the controls
// -----------------------------------------------------------------------------
void MapDisplaySettingsPanel::loadSettings()
{
	// 3D Highlight
	auto map3d_highlight_fill    = CVar::getBool("map3d_highlight_fill");
	auto map3d_highlight_outline = CVar::getBool("map3d_highlight_outline");
	if (map3d_highlight_fill && map3d_highlight_outline)
		rbp_3d_highlight_->setSelection(2);
	else if (map3d_highlight_outline)
		rbp_3d_highlight_->setSelection(1);
	else
		rbp_3d_highlight_->setSelection(0);

	st_general_->loadSettings();
	st_vertices_->loadSettings();
	st_lines_->loadSettings();
	st_things_->loadSettings();
	st_sectors_->loadSettings();
	st_3d_->loadSettings();
}

// -----------------------------------------------------------------------------
// Applies settings from the panel controls to cvars
// -----------------------------------------------------------------------------
void MapDisplaySettingsPanel::applySettings()
{
	// 3D Highlight
	switch (rbp_3d_highlight_->getSelection())
	{
	case 1:
		CVar::setBool("map3d_highlight_fill", false);
		CVar::setBool("map3d_highlight_outline", true);
		break;
	case 2:
		CVar::setBool("map3d_highlight_fill", true);
		CVar::setBool("map3d_highlight_outline", true);
		break;
	default:
		CVar::setBool("map3d_highlight_fill", true);
		CVar::setBool("map3d_highlight_outline", false);
		break;
	}

	st_general_->applySettings();
	st_vertices_->applySettings();
	st_lines_->applySettings();
	st_things_->applySettings();
	st_sectors_->applySettings();
	st_3d_->applySettings();
}
