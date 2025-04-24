
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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

	// Create sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Configurations list
	vector<string> cnames;
	colourconfig::putConfigurationNames(cnames);
	choice_configs_ = new wxChoice(this, -1);
	for (const auto& cname : cnames)
		choice_configs_->Append(wxString::FromUTF8(cname));
	sizer->Add(wxutil::createLabelHBox(this, "Preset:", choice_configs_), wxSizerFlags().Expand());
	sizer->AddSpacer(lh.pad());

	const auto& inactiveTextColour = wxSystemSettings::GetColour(wxSYS_COLOUR_INACTIVECAPTIONTEXT);

	// Create property grid
	pg_colours_ = new wxPropertyGrid(
		this, -1, wxDefaultPosition, wxDefaultSize, wxPG_BOLD_MODIFIED | wxPG_SPLITTER_AUTO_CENTER | wxPG_TOOLTIPS);
	pg_colours_->SetCaptionTextColour(inactiveTextColour);
	pg_colours_->SetCellDisabledTextColour(inactiveTextColour);
	sizer->Add(pg_colours_, wxSizerFlags(1).Expand());

	// Load colour config into grid
	refreshPropGrid();

	// Bind events
	choice_configs_->Bind(
		wxEVT_CHOICE,
		[&](wxCommandEvent&)
		{
			auto config = choice_configs_->GetStringSelection().utf8_string();
			colourconfig::readConfiguration(config);
			refreshPropGrid();
			mapeditor::forceRefresh(true);
		});

	wxWindowBase::Layout();
}

// -----------------------------------------------------------------------------
// Loads settings from cvars into the controls
// -----------------------------------------------------------------------------
void ColourSettingsPanel::loadSettings()
{
	refreshPropGrid();
}

// -----------------------------------------------------------------------------
// Refreshes the colour configuration wxPropertyGrid
// -----------------------------------------------------------------------------
void ColourSettingsPanel::refreshPropGrid() const
{
	// Clear grid
	pg_colours_->Clear();

	// Get (sorted) list of colours
	vector<string> colours;
	colourconfig::putColourNames(colours);
	std::sort(colours.begin(), colours.end());

	// Add colours to property grid
	for (const auto& name : colours)
	{
		// Get colour definition
		auto cdef = colourconfig::colDef(name);

		// Get/create group
		auto group = pg_colours_->GetProperty(wxString::FromUTF8(cdef.group));
		if (!group)
			group = pg_colours_->Append(new wxPropertyCategory(wxString::FromUTF8(cdef.group)));

		// Add colour
		auto colour = pg_colours_->AppendIn(
			group, new wxColourProperty(wxString::FromUTF8(cdef.name), wxString::FromUTF8(name), cdef.colour));

		// Add extra colour properties
		auto opacity = pg_colours_->AppendIn(
			colour, new wxIntProperty(wxS("Opacity (0-255)"), wxS("alpha"), cdef.colour.a));
		pg_colours_->AppendIn(colour, new wxBoolProperty(wxS("Additive"), wxS("additive"), cdef.blend_additive));
		pg_colours_->Collapse(colour);

		// Set opacity limits
		opacity->SetAttribute(wxS("Max"), 255);
		opacity->SetAttribute(wxS("Min"), 0);
	}

	// Add theme options to property grid
	auto g_theme = pg_colours_->Append(new wxPropertyCategory(wxS("Map Editor Theme")));
	pg_colours_->AppendIn(
		g_theme,
		new wxFloatProperty(
			wxS("Line Hilight Width Multiplier"), wxS("line_hilight_width"), colourconfig::lineHilightWidth()));
	pg_colours_->AppendIn(
		g_theme,
		new wxFloatProperty(
			wxS("Line Selection Width Multiplier"), wxS("line_selection_width"), colourconfig::lineSelectionWidth()));
	pg_colours_->AppendIn(g_theme, new wxFloatProperty(wxS("Flat Fade"), wxS("flat_alpha"), colourconfig::flatAlpha()));

	// Set all bool properties to use checkboxes
	pg_colours_->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);
}

// -----------------------------------------------------------------------------
// Applies settings from the panel controls to cvars
// -----------------------------------------------------------------------------
void ColourSettingsPanel::applySettings()
{
	// Get list of all colours
	vector<string> colours;
	colourconfig::putColourNames(colours);

	for (unsigned a = 0; a < colours.size(); a++)
	{
		// Get colour definition
		auto cdef = colourconfig::colDef(colours[a]);

		auto cdef_path = cdef.group;
		cdef_path += ".";
		cdef_path += colours[a];

		// Get properties from grid
		auto p_alpha = dynamic_cast<wxIntProperty*>(pg_colours_->GetProperty(wxString::FromUTF8(cdef_path + ".alpha")));
		auto p_add   = dynamic_cast<wxBoolProperty*>(
            pg_colours_->GetProperty(wxString::FromUTF8(cdef_path + ".additive")));

		if (p_alpha && p_add)
		{
			// Getting the colour out of a wxColourProperty is retarded
			auto     v = pg_colours_->GetPropertyValue(wxString::FromUTF8(cdef_path));
			wxColour col;
			col << v; // wut?

			// Get alpha
			int alpha = p_alpha->GetValue().GetInteger();
			if (alpha > 255)
				a = 255;
			if (alpha < 0)
				a = 0;

			// Get blend
			int blend = 0;
			if (p_add->GetValue().GetBool())
				blend = 1;

			// Set the colour
			colourconfig::setColour(colours[a], col.Red(), col.Green(), col.Blue(), alpha, blend);

			// Clear modified status
			pg_colours_->GetProperty(wxString::FromUTF8(cdef_path))->SetModifiedStatus(false);
			p_alpha->SetModifiedStatus(false);
			p_add->SetModifiedStatus(false);
		}
	}

	colourconfig::setLineHilightWidth(pg_colours_->GetProperty(wxS("line_hilight_width"))->GetValue());
	pg_colours_->GetProperty(wxS("line_hilight_width"))->SetModifiedStatus(false);
	colourconfig::setLineSelectionWidth(pg_colours_->GetProperty(wxS("line_selection_width"))->GetValue());
	pg_colours_->GetProperty(wxS("line_selection_width"))->SetModifiedStatus(false);
	colourconfig::setFlatAlpha(pg_colours_->GetProperty(wxS("flat_alpha"))->GetValue());
	pg_colours_->GetProperty(wxS("flat_alpha"))->SetModifiedStatus(false);

	pg_colours_->Refresh();
	pg_colours_->RefreshEditor();
	maineditor::windowWx()->Refresh();
	mapeditor::forceRefresh(true);
}
