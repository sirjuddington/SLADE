
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ColourPrefsPanel.cpp
 * Description: Panel containing colour preference controls
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
#include "ColourPrefsPanel.h"
#include "General/ColourConfiguration.h"
#include "MainEditor/MainEditor.h"
#include "MapEditor/MapEditor.h"


/*******************************************************************
 * COLOURPREFSPANEL CLASS FUNCTIONS
 *******************************************************************/

/* ColourPrefsPanel::ColourPrefsPanel
 * ColourPrefsPanel class constructor
 *******************************************************************/
ColourPrefsPanel::ColourPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create sizer
	wxBoxSizer* psizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(psizer);

	// Create frame+sizer
	wxStaticBox* frame = new wxStaticBox(this, -1, "Colour Preferences");
	wxStaticBoxSizer* sizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	psizer->Add(sizer, 1, wxEXPAND|wxALL, 4);

	// Configurations list
	vector<string> cnames;
	ColourConfiguration::getConfigurationNames(cnames);
	choice_configs = new wxChoice(this, -1);
	for (unsigned a = 0; a < cnames.size(); a++)
		choice_configs->Append(cnames[a]);

	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxALL, 4);
	hbox->Add(new wxStaticText(this, -1, "Preset:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
	hbox->Add(choice_configs, 1, wxEXPAND, 0);

	// Create property grid
	pg_colours = new wxPropertyGrid(this, -1, wxDefaultPosition, wxDefaultSize, wxPG_BOLD_MODIFIED|wxPG_SPLITTER_AUTO_CENTER|wxPG_TOOLTIPS);
	sizer->Add(pg_colours, 1, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Load colour config into grid
	refreshPropGrid();

	// Bind events
	choice_configs->Bind(wxEVT_CHOICE, &ColourPrefsPanel::onChoicePresetSelected, this);

	Layout();
}

/* ColourPrefsPanel::~ColourPrefsPanel
 * ColourPrefsPanel class destructor
 *******************************************************************/
ColourPrefsPanel::~ColourPrefsPanel()
{
}

/* ColourPrefsPanel::init
 * Initialises panel controls
 *******************************************************************/
void ColourPrefsPanel::init()
{
	refreshPropGrid();
}

/* ColourPrefsPanel::refreshPropGrid
 * Refreshes the colour configuration wxPropertyGrid
 *******************************************************************/
void ColourPrefsPanel::refreshPropGrid()
{
	// Clear grid
	pg_colours->Clear();

	// Get (sorted) list of colours
	vector<string> colours;
	ColourConfiguration::getColourNames(colours);
	std::sort(colours.begin(), colours.end());

	// Add colours to property grid
	for (unsigned a = 0; a < colours.size(); a++)
	{
		// Get colour definition
		cc_col_t cdef = ColourConfiguration::getColDef(colours[a]);

		// Get/create group
		wxPGProperty* group = pg_colours->GetProperty(cdef.group);
		if (!group)
			group = pg_colours->Append(new wxPropertyCategory(cdef.group));

		// Add colour
		wxPGProperty* colour = pg_colours->AppendIn(group, new wxColourProperty(cdef.name, colours[a], WXCOL(cdef.colour)));

		// Add extra colour properties
		wxPGProperty* opacity = pg_colours->AppendIn(colour, new wxIntProperty("Opacity (0-255)", "alpha", cdef.colour.a));
		pg_colours->AppendIn(colour, new wxBoolProperty("Additive", "additive", (cdef.colour.blend == 1)));
		pg_colours->Collapse(colour);

		// Set opacity limits
		opacity->SetAttribute("Max", 255);
		opacity->SetAttribute("Min", 0);
	}

	// Add theme options to property grid
	wxPGProperty* g_theme = pg_colours->Append(new wxPropertyCategory("Map Editor Theme"));
	pg_colours->AppendIn(g_theme, new wxFloatProperty("Line Hilight Width Multiplier", "line_hilight_width", ColourConfiguration::getLineHilightWidth()));
	pg_colours->AppendIn(g_theme, new wxFloatProperty("Line Selection Width Multiplier", "line_selection_width", ColourConfiguration::getLineSelectionWidth()));
	pg_colours->AppendIn(g_theme, new wxFloatProperty("Flat Fade", "flat_alpha", ColourConfiguration::getFlatAlpha()));

	// Set all bool properties to use checkboxes
	pg_colours->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);
}

/* ColourPrefsPanel::applyPreferences
 * Applies preferences from the panel controls
 *******************************************************************/
void ColourPrefsPanel::applyPreferences()
{
	// Get list of all colours
	vector<string> colours;
	ColourConfiguration::getColourNames(colours);

	for (unsigned a = 0; a < colours.size(); a++)
	{
		// Get colour definition
		cc_col_t cdef = ColourConfiguration::getColDef(colours[a]);

		string cdef_path = cdef.group;
		cdef_path += ".";
		cdef_path += colours[a];

		// Get properties from grid
		wxIntProperty* p_alpha = (wxIntProperty*)pg_colours->GetProperty(cdef_path + ".alpha");
		wxBoolProperty* p_add = (wxBoolProperty*)pg_colours->GetProperty(cdef_path + ".additive");

		if (p_alpha && p_add)
		{
			// Getting the colour out of a wxColourProperty is retarded
			wxVariant v = pg_colours->GetPropertyValue(cdef_path);
			wxColour col;
			col << v; // wut?

			// Get alpha
			int alpha = p_alpha->GetValue().GetInteger();
			if (alpha > 255) a = 255;
			if (alpha < 0) a = 0;

			// Get blend
			int blend = 0;
			if (p_add->GetValue().GetBool())
				blend = 1;

			// Set the colour
			ColourConfiguration::setColour(colours[a], COLWX(col), alpha, blend);

			// Clear modified status
			pg_colours->GetProperty(cdef_path)->SetModifiedStatus(false);
			p_alpha->SetModifiedStatus(false);
			p_add->SetModifiedStatus(false);
		}
	}

	ColourConfiguration::setLineHilightWidth((double)pg_colours->GetProperty("line_hilight_width")->GetValue());
	pg_colours->GetProperty("line_hilight_width")->SetModifiedStatus(false);
	ColourConfiguration::setLineSelectionWidth((double)pg_colours->GetProperty("line_selection_width")->GetValue());
	pg_colours->GetProperty("line_selection_width")->SetModifiedStatus(false);
	ColourConfiguration::setFlatAlpha((double)pg_colours->GetProperty("flat_alpha")->GetValue());
	pg_colours->GetProperty("flat_alpha")->SetModifiedStatus(false);

	pg_colours->Refresh();
	pg_colours->RefreshEditor();
	MainEditor::windowWx()->Refresh();
	MapEditor::forceRefresh(true);
}


/*******************************************************************
 * COLOURPREFSPANEL CLASS EVENTS
 *******************************************************************/

/* ColourPrefsPanel::onChoicePresetSelected
 * Called when the 'preset' dropdown choice is changed
 *******************************************************************/
void ColourPrefsPanel::onChoicePresetSelected(wxCommandEvent& e)
{
	string config = choice_configs->GetStringSelection();
	ColourConfiguration::readConfiguration(config);
	refreshPropGrid();
	MapEditor::forceRefresh(true);
}
