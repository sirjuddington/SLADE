
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ColourConfigPanel.cpp
// Description: UI panel for selecting and editing colour configurations for
//              various parts of SLADE (see General/ColourConfiguration.cpp)
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
#include "ColourConfigPanel.h"
#include "General/ColourConfiguration.h"
#include "UI/Layout.h"
#include "Utility/StringUtils.h"

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the config cvar name for the given colour configuration [group]
// -----------------------------------------------------------------------------
namespace
{
string configCvar(ccfg::Group group)
{
	switch (group)
	{
	case ccfg::Group::MapPreview: return "ccfg_map_preview";
	case ccfg::Group::MapImage:   return "ccfg_map_image";
	case ccfg::Group::MapEditor:  return "ccfg_map_editor";
	default:                      return "";
	}
}
} // namespace


// -----------------------------------------------------------------------------
//
// ColourConfigPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ColourConfigPanel class constructor
// -----------------------------------------------------------------------------
ColourConfigPanel::ColourConfigPanel(
	wxWindow*           parent,
	colourconfig::Group group,
	bool                allow_customize_default,
	int                 border) :
	wxPanel(parent),
	group_(group),
	allow_customize_default_(allow_customize_default)
{
	// Create/populate config choice
	vector<string> map_preview_configs = { "None (Default)" };
	colourconfig::putConfigurationNames(group, map_preview_configs);
	map_preview_configs.push_back("Custom");
	choice_config_ = new wxChoice(this, -1);
	for (const auto& cname : map_preview_configs)
		choice_config_->Append(wxString::FromUTF8(cname));

	// Create other controls
	btn_customize_ = new wxButton(this, -1, wxS("Customize"));
	pg_colours_    = new wxPropertyGrid(this, -1, wxDefaultPosition, wxDefaultSize, wxPG_SPLITTER_AUTO_CENTER);

	// Setup layout
	auto lh    = LayoutHelper(this);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Add border if needed
	if (border > 0)
	{
		auto border_sizer = new wxBoxSizer(wxVERTICAL);
		sizer->Add(border_sizer, lh.sfWithBorder(1, wxALL, border).Expand());
		sizer = border_sizer;
	}

	// Config choice + customize button
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	hbox->Add(new wxStaticText(this, -1, wxS("Preset:")), lh.sfWithBorder(0, wxRIGHT).CenterVertical());
	hbox->Add(choice_config_, wxSizerFlags(1).Expand());
	hbox->Add(btn_customize_, lh.sfWithBorder(0, wxLEFT).Expand());
	sizer->Add(hbox, lh.sfWithBorder(0, wxBOTTOM).Expand());

	// Colour property grid
	sizer->Add(pg_colours_, wxSizerFlags(1).Expand());

	// Bind events
	choice_config_->Bind(wxEVT_CHOICE, &ColourConfigPanel::onConfigChanged, this);

	// 'Customize' button selects the custom configuration but keeps the current colours
	btn_customize_->Bind(
		wxEVT_BUTTON,
		[this](wxCommandEvent&)
		{
			choice_config_->SetSelection(choice_config_->GetCount() - 1);
			refreshPropGrid(false);
			btn_customize_->Hide();
			btn_customize_->GetContainingSizer()->Layout();
		});
}

// -----------------------------------------------------------------------------
// Loads the current colour configuration from the relevant cvar into the panel
// -----------------------------------------------------------------------------
void ColourConfigPanel::loadConfig()
{
	auto config = CVar::getString(configCvar(group_));

	if (config.empty() || strutil::equalCI(config, "default"))
	{
		// Default
		choice_config_->SetSelection(0);
		btn_customize_->Show(allow_customize_default_);
	}
	else if (strutil::equalCI(config, "custom"))
	{
		// Custom
		choice_config_->SetSelection(choice_config_->GetCount() - 1);
		btn_customize_->Hide();
	}
	else
	{
		// Built-in configuration (revert to default if not found)
		if (!choice_config_->SetStringSelection(wxString::FromUTF8(config)))
			choice_config_->SetSelection(0);
		else
			btn_customize_->Show();
	}

	colours_ = ccfg::allColours(group_);
	if (group_ == ccfg::Group::MapEditor)
		map_editor_theme_ = std::make_unique<ccfg::MapEditorTheme>(ccfg::mapEditorTheme());
	refreshPropGrid(choice_config_->GetSelection() < choice_config_->GetCount() - 1);
}

// -----------------------------------------------------------------------------
// Applies the current configuration to the relevant cvar and colour config
// -----------------------------------------------------------------------------
bool ColourConfigPanel::apply()
{
	// Map Preview configuration
	if (choice_config_->GetSelection() == 0)
	{
		// Default
		CVar::set(configCvar(group_), "");
		ccfg::applyDefaultConfiguration(group_);
	}
	else if (choice_config_->GetSelection() == choice_config_->GetCount() - 1)
	{
		// Custom
		CVar::set(configCvar(group_), "custom");
		applyCustomColours();
		return ccfg::setCustomConfiguration(group_, colours_, map_editor_theme_.get(), true);
	}
	else
	{
		// Built-in configuration
		auto config = choice_config_->GetStringSelection().utf8_string();
		CVar::set(configCvar(group_), config);
		return ccfg::applyConfiguration(group_, config);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Refreshes the property grid with the current colours
// -----------------------------------------------------------------------------
void ColourConfigPanel::refreshPropGrid(bool readonly) const
{
	pg_colours_->Clear();

	// Add colour properties to the property grid
	for (const auto& cdef : colours_)
	{
		// Get or create group (if any)
		wxPGProperty* group = nullptr;
		if (!cdef.group.empty())
		{
			group = pg_colours_->GetProperty(wxString::FromUTF8(cdef.group));
			if (!group)
				group = pg_colours_->Append(new wxPropertyCategory(wxString::FromUTF8(cdef.group)));
		}

		// Add colour
		auto col_prop = new wxColourProperty(wxString::FromUTF8(cdef.name), wxString::FromUTF8(cdef.id), cdef.colour);
		auto colour   = group ? pg_colours_->AppendIn(group, col_prop) : pg_colours_->Append(col_prop);

		// Add opacity
		auto opacity = pg_colours_->AppendIn(
			colour, new wxIntProperty(wxS("Opacity (0-255)"), wxS("alpha"), cdef.colour.a));

		// Add 'additive' property (map editor only)
		if (group_ == ccfg::Group::MapEditor)
		{
			auto additive = pg_colours_->AppendIn(
				colour, new wxBoolProperty(wxS("Additive"), wxS("add"), cdef.blend_additive));
			pg_colours_->SetPropertyReadOnly(additive, readonly);
			pg_colours_->SetPropertyAttribute(additive, wxPG_BOOL_USE_CHECKBOX, true);
		}

		// Set opacity limits
		opacity->SetAttribute(wxS("Max"), 255);
		opacity->SetAttribute(wxS("Min"), 0);

		pg_colours_->SetPropertyReadOnly(colour, readonly);
		pg_colours_->SetPropertyReadOnly(opacity, readonly);
		pg_colours_->Collapse(colour);
	}

	// Add map 'theme' properties if map editor config
	if (group_ == ccfg::Group::MapEditor && map_editor_theme_)
	{
		auto theme_group        = pg_colours_->Append(new wxPropertyCategory(wxS("Theme")));
		auto line_hilight_width = pg_colours_->AppendIn(
			theme_group,
			new wxFloatProperty(
				wxS("Line Highlight Width"), wxS("line_hilight_width"), map_editor_theme_->line_hilight_width));
		auto line_selection_width = pg_colours_->AppendIn(
			theme_group,
			new wxFloatProperty(
				wxS("Line Selection Width"), wxS("line_selection_width"), map_editor_theme_->line_selection_width));
		auto flat_alpha = pg_colours_->AppendIn(
			theme_group, new wxFloatProperty(wxS("Flat Alpha"), wxS("flat_alpha"), map_editor_theme_->flat_alpha));

		pg_colours_->SetPropertyReadOnly(line_hilight_width, readonly);
		pg_colours_->SetPropertyReadOnly(line_selection_width, readonly);
		pg_colours_->SetPropertyReadOnly(flat_alpha, readonly);
	}
}

// -----------------------------------------------------------------------------
// Applies the current colours (and map editor theme values, if any) from the
// property grid to member variables
// -----------------------------------------------------------------------------
void ColourConfigPanel::applyCustomColours()
{
	for (auto& cdef : colours_)
	{
		// Get properties from grid
		auto p_alpha = dynamic_cast<wxIntProperty*>(pg_colours_->GetProperty(wxString::FromUTF8(cdef.id + ".alpha")));
		if (!p_alpha)
			continue;

		// Colour
		auto     v = pg_colours_->GetPropertyValue(wxString::FromUTF8(cdef.id));
		wxColour col;
		col << v;

		// Alpha
		int alpha = p_alpha->GetValue().GetInteger();
		if (alpha > 255)
			alpha = 255;
		if (alpha < 0)
			alpha = 0;

		cdef.colour = { col.Red(), col.Green(), col.Blue(), static_cast<u8>(alpha) };

		// Additive blend (map editor only)
		if (group_ == ccfg::Group::MapEditor)
		{
			auto p_additive = dynamic_cast<wxBoolProperty*>(
				pg_colours_->GetProperty(wxString::FromUTF8(cdef.id + ".add")));
			if (p_additive)
				cdef.blend_additive = p_additive->GetValue().GetBool();
		}
	}

	if (group_ == ccfg::Group::MapEditor && map_editor_theme_)
	{
		auto p_line_hilight_width = dynamic_cast<wxFloatProperty*>(pg_colours_->GetProperty(wxS("line_hilight_width")));
		if (p_line_hilight_width)
			map_editor_theme_->line_hilight_width = p_line_hilight_width->GetValue().GetDouble();

		auto p_line_selection_width = dynamic_cast<wxFloatProperty*>(
			pg_colours_->GetProperty(wxS("line_selection_width")));
		if (p_line_selection_width)
			map_editor_theme_->line_selection_width = p_line_selection_width->GetValue().GetDouble();

		auto p_flat_alpha = dynamic_cast<wxFloatProperty*>(pg_colours_->GetProperty(wxS("flat_alpha")));
		if (p_flat_alpha)
			map_editor_theme_->flat_alpha = p_flat_alpha->GetValue().GetDouble();
	}
}


// -----------------------------------------------------------------------------
//
// ColourConfigPanel Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the config choice is changed
// -----------------------------------------------------------------------------
void ColourConfigPanel::onConfigChanged(wxCommandEvent& e)
{
	if (choice_config_->GetSelection() == 0)
	{
		// Default configuration selected
		btn_customize_->Show(allow_customize_default_);
		ccfg::loadDefaultConfiguration(group_, colours_, map_editor_theme_.get());
		refreshPropGrid(true);
	}
	else if (choice_config_->GetSelection() == choice_config_->GetCount() - 1)
	{
		// Custom config
		btn_customize_->Hide();
		ccfg::loadCustomConfiguration(group_, colours_, map_editor_theme_.get());
		refreshPropGrid(false);
	}
	else
	{
		// Built-in configuration selected
		btn_customize_->Show();
		auto config = choice_config_->GetStringSelection().utf8_string();
		ccfg::loadConfiguration(group_, config, colours_, map_editor_theme_.get());
		refreshPropGrid(true);
	}

	btn_customize_->GetContainingSizer()->Layout();
}
