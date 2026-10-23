
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ColourSettingsPanel.cpp
// Description: Panel containing colour settings controls
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
#include "ColourSettingsPanel.h"
#include "General/ColourConfiguration.h"
#include "MainEditor/MainEditor.h"
#include "MapEditor/MapEditor.h"
#include "UI/Layout.h"
#include "UI/WxUtils.h"
#include "Utility/PropertyList.h"
#include "Utility/StringUtils.h"

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// ColourSettingsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ColourSettingsPanel class constructor
// -----------------------------------------------------------------------------
ColourSettingsPanel::ColourSettingsPanel(wxWindow* parent) : SettingsPanel(parent)
{
	auto lh = LayoutHelper(this);

	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	auto gb_sizer = new wxGridBagSizer(lh.pad(), lh.pad());
	sizer->AddSpacer(lh.pad());
	sizer->Add(gb_sizer, lh.sfWithLargeBorder(1, wxLEFT | wxRIGHT | wxBOTTOM).Expand());

	gb_sizer->Add(new wxStaticText(this, -1, wxS("Map Preview")), { 0, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);

	// Map preview configurations
	vector<string> map_preview_configs = { "Default (System)" };
	colourconfig::putConfigurationNames(colourconfig::Group::MapPreview, map_preview_configs);
	map_preview_configs.push_back("Custom");
	choice_map_preview_cfg_ = new wxChoice(this, -1);
	for (const auto& cname : map_preview_configs)
		choice_map_preview_cfg_->Append(wxString::FromUTF8(cname));
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	hbox->Add(choice_map_preview_cfg_, wxSizerFlags(1).Expand());
	btn_map_preview_customize_ = new wxButton(this, -1, wxS("Customize"));
	hbox->Add(btn_map_preview_customize_, lh.sfWithBorder(0, wxLEFT).Expand());
	gb_sizer->Add(hbox, { 0, 1 }, { 1, 1 }, wxEXPAND);

	// Map preview property grid
	pg_map_preview_ = new wxPropertyGrid(this, -1, wxDefaultPosition, wxDefaultSize, wxPG_SPLITTER_AUTO_CENTER);
	gb_sizer->Add(pg_map_preview_, { 1, 1 }, { 1, 1 }, wxEXPAND);

	gb_sizer->Add(new wxStaticText(this, -1, wxS("Map Image Export")), { 2, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);

	// Map image configurations
	vector<string> map_image_configs = { "Default" };
	colourconfig::putConfigurationNames(colourconfig::Group::MapImage, map_image_configs);
	map_image_configs.push_back("Custom");
	choice_map_image_cfg_ = new wxChoice(this, -1);
	for (const auto& cname : map_image_configs)
		choice_map_image_cfg_->Append(wxString::FromUTF8(cname));
	hbox = new wxBoxSizer(wxHORIZONTAL);
	hbox->Add(choice_map_image_cfg_, wxSizerFlags(1).Expand());
	btn_map_image_customize_ = new wxButton(this, -1, wxS("Customize"));
	hbox->Add(btn_map_image_customize_, lh.sfWithBorder(0, wxLEFT).Expand());
	gb_sizer->Add(hbox, { 2, 1 }, { 1, 1 }, wxEXPAND);

	// Map image property grid
	pg_map_image_ = new wxPropertyGrid(this, -1, wxDefaultPosition, wxDefaultSize, wxPG_SPLITTER_AUTO_CENTER);
	gb_sizer->Add(pg_map_image_, { 3, 1 }, { 1, 1 }, wxEXPAND);

	gb_sizer->AddGrowableCol(1, 1);
	gb_sizer->AddGrowableRow(1, 1);
	gb_sizer->AddGrowableRow(3, 1);

	// Bind events
	choice_map_preview_cfg_->Bind(wxEVT_CHOICE, &ColourSettingsPanel::onMapPreviewConfigChanged, this);
	choice_map_image_cfg_->Bind(wxEVT_CHOICE, &ColourSettingsPanel::onMapImageConfigChanged, this);

	wxWindowBase::Layout();
}

// -----------------------------------------------------------------------------
// Loads settings from cvars into the controls
// -----------------------------------------------------------------------------
void ColourSettingsPanel::loadSettings()
{
	// Map preview configuration
	auto map_preview_cfg = CVar::getString("ccfg_map_preview");
	if (map_preview_cfg.empty() || strutil::equalCI(map_preview_cfg, "default"))
		choice_map_preview_cfg_->SetSelection(0); // Default
	else if (strutil::equalCI(map_preview_cfg, "custom"))
		choice_map_preview_cfg_->SetSelection(choice_map_preview_cfg_->GetCount() - 1); // Custom
	else
	{
		// Built-in configuration (revert to default if not found)
		if (!choice_map_preview_cfg_->SetStringSelection(wxString::FromUTF8(map_preview_cfg)))
			choice_map_preview_cfg_->SetSelection(0);
	}

	// Map image configuration
	auto map_image_cfg = CVar::getString("ccfg_map_image");
	if (map_image_cfg.empty() || strutil::equalCI(map_image_cfg, "default"))
		choice_map_image_cfg_->SetSelection(0); // Default
	else if (strutil::equalCI(map_image_cfg, "custom"))
		choice_map_image_cfg_->SetSelection(choice_map_image_cfg_->GetCount() - 1); // Custom
	else
	{
		// Built-in configuration (revert to default if not found)
		if (!choice_map_image_cfg_->SetStringSelection(wxString::FromUTF8(map_image_cfg)))
			choice_map_image_cfg_->SetSelection(0);
	}

	// Load colours
	colours_map_preview_ = ccfg::allColours(ccfg::Group::MapPreview);
	colours_map_image_   = ccfg::allColours(ccfg::Group::MapImage);
	refreshPropGrids();
}

// -----------------------------------------------------------------------------
// Refreshes the colour configuration wxPropertyGrid
// -----------------------------------------------------------------------------
void ColourSettingsPanel::refreshPropGrids(bool map_preview, bool map_image, bool readonly) const
{
	// Clear grids
	if (map_preview)
		pg_map_preview_->Clear();
	if (map_image)
		pg_map_image_->Clear();

	static auto add_to_pg = [&](wxPropertyGrid* pg, const colourconfig::Colour& cdef)
	{
		// Add colour
		auto colour = pg->Append(
			new wxColourProperty(wxString::FromUTF8(cdef.name), wxString::FromUTF8(cdef.id), cdef.colour));

		// Add opacity
		auto opacity = pg->AppendIn(colour, new wxIntProperty(wxS("Opacity (0-255)"), wxS("alpha"), cdef.colour.a));

		// Set opacity limits
		opacity->SetAttribute(wxS("Max"), 255);
		opacity->SetAttribute(wxS("Min"), 0);

		pg->SetPropertyReadOnly(colour, readonly);
		pg->SetPropertyReadOnly(opacity, readonly);
		pg->Collapse(colour);
	};

	// Load colours to grids
	if (map_preview)
		for (auto cdef : colours_map_preview_)
			add_to_pg(pg_map_preview_, cdef);
	if (map_image)
		for (auto cdef : colours_map_image_)
			add_to_pg(pg_map_image_, cdef);

	// // Add theme options to property grid
	// auto g_theme = pg_colours_->Append(new wxPropertyCategory(wxS("Map Editor Theme")));
	// pg_colours_->AppendIn(
	// 	g_theme,
	// 	new wxFloatProperty(
	// 		wxS("Line Highlight Width Multiplier"), wxS("line_hilight_width"), colourconfig::lineHilightWidth()));
	// pg_colours_->AppendIn(
	// 	g_theme,
	// 	new wxFloatProperty(
	// 		wxS("Line Selection Width Multiplier"), wxS("line_selection_width"), colourconfig::lineSelectionWidth()));
	// pg_colours_->AppendIn(g_theme, new wxFloatProperty(wxS("Flat Fade"), wxS("flat_alpha"),
	// colourconfig::flatAlpha()));
	//
	// // Set all bool properties to use checkboxes
	// pg_colours_->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);
}

void ColourSettingsPanel::onMapPreviewConfigChanged(wxCommandEvent& e)
{
	if (choice_map_preview_cfg_->GetSelection() == 0)
	{
		// Default configuration selected
		btn_map_preview_customize_->Show();
		ccfg::loadDefaults(ccfg::Group::MapPreview, colours_map_preview_);
		refreshPropGrids(true, false);
	}
	else if (choice_map_preview_cfg_->GetSelection() == choice_map_preview_cfg_->GetCount() - 1)
	{
		// TODO: Custom config
		btn_map_preview_customize_->Hide();
		refreshPropGrids(false, true, false);
	}
	else
	{
		// Built-in configuration selected
		btn_map_preview_customize_->Show();
		auto config = choice_map_preview_cfg_->GetStringSelection().utf8_string();
		ccfg::readConfiguration(ccfg::Group::MapPreview, config, colours_map_preview_);
		refreshPropGrids(true, false);
		pg_map_preview_->Enable(false);
	}

	btn_map_preview_customize_->GetContainingSizer()->Layout();
}

void ColourSettingsPanel::onMapImageConfigChanged(wxCommandEvent& e)
{
	if (choice_map_image_cfg_->GetSelection() == 0)
	{
		// Default configuration selected
		btn_map_image_customize_->Show();
		ccfg::loadDefaults(ccfg::Group::MapImage, colours_map_image_);
		refreshPropGrids(false, true);
	}
	else if (choice_map_image_cfg_->GetSelection() == choice_map_image_cfg_->GetCount() - 1)
	{
		// TODO: Custom config
		btn_map_image_customize_->Hide();
		refreshPropGrids(false, true, false);
	}
	else
	{
		// Built-in configuration selected
		btn_map_image_customize_->Show();
		auto config = choice_map_image_cfg_->GetStringSelection().utf8_string();
		ccfg::readConfiguration(ccfg::Group::MapImage, config, colours_map_image_);
		refreshPropGrids(false, true);
		pg_map_image_->Enable(false);
	}

	btn_map_image_customize_->GetContainingSizer()->Layout();
}

// -----------------------------------------------------------------------------
// Applies settings from the panel controls to cvars
// -----------------------------------------------------------------------------
void ColourSettingsPanel::applySettings()
{
	// Map Preview configuration
	if (choice_map_preview_cfg_->GetSelection() == 0)
	{
		CVar::set("ccfg_map_preview", "");
		ccfg::loadDefaults(ccfg::Group::MapPreview);
	}
	else if (choice_map_preview_cfg_->GetSelection() == choice_map_preview_cfg_->GetCount() - 1)
	{
		// TODO: Custom
		CVar::set("ccfg_map_preview", "custom");
	}
	else
	{
		auto config = choice_map_preview_cfg_->GetStringSelection().utf8_string();
		CVar::set("ccfg_map_preview", config);
		ccfg::loadConfiguration(ccfg::Group::MapPreview, config);
	}

	// Map Image configuration
	if (choice_map_image_cfg_->GetSelection() == 0)
	{
		CVar::set("ccfg_map_image", "");
		ccfg::loadDefaults(ccfg::Group::MapImage);
	}
	else if (choice_map_image_cfg_->GetSelection() == choice_map_image_cfg_->GetCount() - 1)
	{
		// TODO: Custom
		CVar::set("ccfg_map_image", "custom");
	}
	else
	{
		auto config = choice_map_image_cfg_->GetStringSelection().utf8_string();
		CVar::set("ccfg_map_image", config);
		ccfg::loadConfiguration(ccfg::Group::MapImage, config);
	}

	// Get list of all colours
	// auto colours = ccfg::allColours();

	// for (unsigned a = 0; a < colours.size(); a++)
	{
		// Get colour definition
		// auto cdef = colours[a];

		// auto cdef_path = cdef.group;
		// cdef_path += ".";
		// cdef_path += cdef.name;
		//
		// // Get properties from grid
		// auto p_alpha = dynamic_cast<wxIntProperty*>(pg_colours_->GetProperty(wxString::FromUTF8(cdef_path +
		// ".alpha"))); auto p_add   = dynamic_cast<wxBoolProperty*>(
		//           pg_colours_->GetProperty(wxString::FromUTF8(cdef_path + ".additive")));
		//
		// if (p_alpha && p_add)
		// {
		// 	// Getting the colour out of a wxColourProperty is retarded
		// 	auto     v = pg_colours_->GetPropertyValue(wxString::FromUTF8(cdef_path));
		// 	wxColour col;
		// 	col << v; // wut?
		//
		// 	// Get alpha
		// 	int alpha = p_alpha->GetValue().GetInteger();
		// 	if (alpha > 255)
		// 		a = 255;
		// 	if (alpha < 0)
		// 		a = 0;
		//
		// 	// Get blend
		// 	int blend = 0;
		// 	if (p_add->GetValue().GetBool())
		// 		blend = 1;
		//
		// 	// TODO: Set the colour
		// 	// colourconfig::setColour(colours[a], {}, col.Red(), col.Green(), col.Blue(), alpha, blend);
		//
		// 	// Clear modified status
		// 	pg_colours_->GetProperty(wxString::FromUTF8(cdef_path))->SetModifiedStatus(false);
		// 	p_alpha->SetModifiedStatus(false);
		// 	p_add->SetModifiedStatus(false);
		// }
	}

	// colourconfig::setLineHilightWidth(pg_colours_->GetProperty(wxS("line_hilight_width"))->GetValue());
	// pg_colours_->GetProperty(wxS("line_hilight_width"))->SetModifiedStatus(false);
	// colourconfig::setLineSelectionWidth(pg_colours_->GetProperty(wxS("line_selection_width"))->GetValue());
	// pg_colours_->GetProperty(wxS("line_selection_width"))->SetModifiedStatus(false);
	// colourconfig::setFlatAlpha(pg_colours_->GetProperty(wxS("flat_alpha"))->GetValue());
	// pg_colours_->GetProperty(wxS("flat_alpha"))->SetModifiedStatus(false);
	//
	// pg_colours_->Refresh();
	// pg_colours_->RefreshEditor();
	maineditor::windowWx()->Refresh();
	mapeditor::forceRefresh(true);
}
