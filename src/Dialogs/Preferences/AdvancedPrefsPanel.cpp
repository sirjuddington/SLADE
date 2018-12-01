
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    AdvancedPrefsPanel.cpp
// Description: Panel containing 'advanced' preference controls, basically a
//              way to edit raw cvar values outside the slade.cfg file
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
#include "AdvancedPrefsPanel.h"


// -----------------------------------------------------------------------------
//
// AdvancedPrefsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// AdvancedPrefsPanel class constructor
// -----------------------------------------------------------------------------
AdvancedPrefsPanel::AdvancedPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	const wxColour& inactiveTextColour = wxSystemSettings::GetColour(wxSYS_COLOUR_INACTIVECAPTIONTEXT);

	// Add property grid
	pg_cvars_ = new wxPropertyGrid(
		this,
		-1,
		wxDefaultPosition,
		wxDefaultSize,
		wxPG_BOLD_MODIFIED | wxPG_SPLITTER_AUTO_CENTER | wxPG_TOOLTIPS | wxPG_HIDE_MARGIN);
	pg_cvars_->SetCaptionTextColour(inactiveTextColour);
	pg_cvars_->SetCellDisabledTextColour(inactiveTextColour);
	sizer->Add(pg_cvars_, 1, wxEXPAND);

	// Init property grid
	refreshPropGrid();

	Layout();
}

// -----------------------------------------------------------------------------
// AdvancedPrefsPanel class destructor
// -----------------------------------------------------------------------------
AdvancedPrefsPanel::~AdvancedPrefsPanel() {}

// -----------------------------------------------------------------------------
// Initialises panel controls
// -----------------------------------------------------------------------------
void AdvancedPrefsPanel::init()
{
	refreshPropGrid();
}

// -----------------------------------------------------------------------------
// Refreshes the cvars wxPropertyGrid
// -----------------------------------------------------------------------------
void AdvancedPrefsPanel::refreshPropGrid()
{
	// Clear
	pg_cvars_->Clear();

	// Get list of cvars
	vector<string> cvars;
	getCVarList(cvars);
	std::sort(cvars.begin(), cvars.end());

	for (unsigned a = 0; a < cvars.size(); a++)
	{
		// Get cvar
		CVar* cvar = getCVar(cvars[a]);

		// Add to grid depending on type
		if (cvar->type == CVAR_BOOLEAN)
			pg_cvars_->Append(new wxBoolProperty(cvars[a], cvars[a], cvar->GetValue().Bool));
		else if (cvar->type == CVAR_INTEGER)
			pg_cvars_->Append(new wxIntProperty(cvars[a], cvars[a], cvar->GetValue().Int));
		else if (cvar->type == CVAR_FLOAT)
			pg_cvars_->Append(new wxFloatProperty(cvars[a], cvars[a], cvar->GetValue().Float));
		else if (cvar->type == CVAR_STRING)
			pg_cvars_->Append(new wxStringProperty(cvars[a], cvars[a], S_FMT("%s", ((CStringCVar*)cvar)->value)));
	}

	// Set all bool properties to use checkboxes
	pg_cvars_->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);
}

// -----------------------------------------------------------------------------
// Applies preferences from the panel controls
// -----------------------------------------------------------------------------
void AdvancedPrefsPanel::applyPreferences()
{
	// Get list of cvars
	vector<string> cvars;
	getCVarList(cvars);

	for (unsigned a = 0; a < cvars.size(); a++)
	{
		// Get cvar
		CVar* cvar = getCVar(cvars[a]);

		// Check if cvar value was even modified
		if (!pg_cvars_->GetProperty(cvars[a])->HasFlag(wxPG_PROP_MODIFIED))
		{
			// If unmodified, it might still have been changed in another panel, so refresh it
			if (cvar->type == CVAR_BOOLEAN)
				pg_cvars_->SetPropertyValue(cvars[a], cvar->GetValue().Bool);
			else if (cvar->type == CVAR_INTEGER)
				pg_cvars_->SetPropertyValue(cvars[a], cvar->GetValue().Int);
			else if (cvar->type == CVAR_FLOAT)
				pg_cvars_->SetPropertyValue(cvars[a], cvar->GetValue().Float);
			else if (cvar->type == CVAR_STRING)
				pg_cvars_->SetPropertyValue(cvars[a], S_FMT("%s", ((CStringCVar*)cvar)->value));

			continue;
		}

		// Read value from grid depending on type
		wxVariant value = pg_cvars_->GetPropertyValue(cvars[a]);
		if (cvar->type == CVAR_INTEGER)
			*((CIntCVar*)cvar) = value.GetInteger();
		else if (cvar->type == CVAR_BOOLEAN)
			*((CBoolCVar*)cvar) = value.GetBool();
		else if (cvar->type == CVAR_FLOAT)
			*((CFloatCVar*)cvar) = value.GetDouble();
		else if (cvar->type == CVAR_STRING)
			*((CStringCVar*)cvar) = value.GetString();

		pg_cvars_->GetProperty(cvars[a])->SetModifiedStatus(false);
		pg_cvars_->Refresh();
		pg_cvars_->RefreshEditor();
	}
}
