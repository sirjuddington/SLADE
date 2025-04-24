
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    AdvancedSettingsPanel.cpp
// Description: Panel containing 'advanced' settings controls, basically a
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
#include "AdvancedSettingsPanel.h"
#include "UI/Layout.h"
#include "UI/WxUtils.h"

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// AdvancedSettingsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// AdvancedSettingsPanel class constructor
// -----------------------------------------------------------------------------
AdvancedSettingsPanel::AdvancedSettingsPanel(wxWindow* parent) : SettingsPanel(parent)
{
	// Create sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	const auto& inactiveTextColour = wxSystemSettings::GetColour(wxSYS_COLOUR_INACTIVECAPTIONTEXT);

	// Disclaimer
	sizer->Add(
		new wxStaticText(
			this,
			-1,
			wxS("Warning: Only modify these values if you know what you are doing!\n"
				"Most of these settings can be changed more safely from the other sections.")),
		LayoutHelper(this).sfWithLargeBorder(0, wxBOTTOM).Expand());

	// Add property grid
	pg_cvars_ = new wxPropertyGrid(
		this,
		-1,
		wxDefaultPosition,
		wxDefaultSize,
		wxPG_BOLD_MODIFIED | wxPG_SPLITTER_AUTO_CENTER | wxPG_TOOLTIPS | wxPG_HIDE_MARGIN);
	pg_cvars_->SetCaptionTextColour(inactiveTextColour);
	pg_cvars_->SetCellDisabledTextColour(inactiveTextColour);
	sizer->Add(pg_cvars_, wxSizerFlags(1).Expand());

	// Init property grid
	refreshPropGrid();

	wxPanel::Layout();
}

// -----------------------------------------------------------------------------
// Refreshes the cvars wxPropertyGrid
// -----------------------------------------------------------------------------
void AdvancedSettingsPanel::refreshPropGrid() const
{
	// Clear
	pg_cvars_->Clear();

	// Get list of cvars
	vector<string> cvars;
	CVar::putList(cvars);
	std::sort(cvars.begin(), cvars.end());

	for (const auto& name : cvars)
	{
		// Get cvar
		auto cvar = CVar::get(name);

		// Add to grid depending on type
		auto name_wx = wxString::FromUTF8(name);
		if (cvar->type == CVar::Type::Boolean)
			pg_cvars_->Append(new wxBoolProperty(name_wx, name_wx, cvar->getValue().Bool));
		else if (cvar->type == CVar::Type::Integer)
			pg_cvars_->Append(new wxIntProperty(name_wx, name_wx, cvar->getValue().Int));
		else if (cvar->type == CVar::Type::Float)
			pg_cvars_->Append(new wxFloatProperty(name_wx, name_wx, cvar->getValue().Float));
		else if (cvar->type == CVar::Type::String)
			pg_cvars_->Append(
				new wxStringProperty(name_wx, name_wx, wxString::FromUTF8(static_cast<CStringCVar*>(cvar)->value)));
	}

	// Set all bool properties to use checkboxes
	pg_cvars_->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);
}

// -----------------------------------------------------------------------------
// Applies settings from the panel controls to cvars
// -----------------------------------------------------------------------------
void AdvancedSettingsPanel::applySettings()
{
	// Get list of cvars
	vector<string> cvars;
	CVar::putList(cvars);

	for (const auto& name : cvars)
	{
		// Get cvar
		auto cvar = CVar::get(name);

		// Check if cvar value was even modified
		auto name_wx = wxString::FromUTF8(name);
		if (!pg_cvars_->GetProperty(name_wx)->HasFlag(wxPG_PROP_MODIFIED))
		{
			// If unmodified, it might still have been changed in another panel, so refresh it
			if (cvar->type == CVar::Type::Boolean)
				pg_cvars_->SetPropertyValue(name_wx, cvar->getValue().Bool);
			else if (cvar->type == CVar::Type::Integer)
				pg_cvars_->SetPropertyValue(name_wx, cvar->getValue().Int);
			else if (cvar->type == CVar::Type::Float)
				pg_cvars_->SetPropertyValue(name_wx, cvar->getValue().Float);
			else if (cvar->type == CVar::Type::String)
				pg_cvars_->SetPropertyValue(name_wx, wxString::FromUTF8(static_cast<CStringCVar*>(cvar)->value));

			continue;
		}

		// Read value from grid depending on type
		auto value = pg_cvars_->GetPropertyValue(name_wx);
		if (cvar->type == CVar::Type::Integer)
			*dynamic_cast<CIntCVar*>(cvar) = value.GetInteger();
		else if (cvar->type == CVar::Type::Boolean)
			*dynamic_cast<CBoolCVar*>(cvar) = value.GetBool();
		else if (cvar->type == CVar::Type::Float)
			*dynamic_cast<CFloatCVar*>(cvar) = value.GetDouble();
		else if (cvar->type == CVar::Type::String)
			*((CStringCVar*)cvar) = value.GetString().utf8_string();

		pg_cvars_->GetProperty(name_wx)->SetModifiedStatus(false);
	}

	pg_cvars_->Refresh();
	pg_cvars_->RefreshEditor();
}
