
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SettingsTable.cpp
// Description: A panel that lays out multiple settings with a label on the left
//              and the setting control on the right for each one. Settings can
//              be linked with a cvar which will be loaded into the control when
//              loadSettings() is called, and written back to the cvar when
//              applySettings() is called. Can also add custom control settings,
//              where loading/applying needs to be handled manually outside this
//              table.
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
#include "SettingsTable.h"
#include "RadioButtonPanel.h"
#include "UI/Layout.h"
#include "UI/WxUtils.h"

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// Structs
//
// -----------------------------------------------------------------------------
namespace slade::ui
{
// -----------------------------------------------------------------------------
// SettingRow (and derived structs)
//
// Base struct for a setting row, which may be linked to a cvar.
// Derived structs implement different control types (checkbox, radio buttons,
// text box, etc.) and handle loading/applying to cvars of different types.
// -----------------------------------------------------------------------------
struct SettingRow
{
	string cvar;

	SettingRow(const string& cvar) : cvar(cvar) {}
	virtual ~SettingRow() = default;

	virtual void load() {}
	virtual void apply() {}
};
struct CheckSettingRow : SettingRow
{
	wxCheckBox* check_box = nullptr;

	CheckSettingRow(const string& cvar, wxCheckBox* check_box) : SettingRow(cvar), check_box(check_box) {}

	void load() override { check_box->SetValue(CVar::getBool(cvar)); }
	void apply() override { CVar::setBool(cvar, check_box->GetValue()); }
};
struct RadioSettingRow : SettingRow
{
	RadioButtonPanel* radio_button_panel = nullptr;

	RadioSettingRow(const string& cvar, RadioButtonPanel* radio_button_panel) :
		SettingRow(cvar),
		radio_button_panel(radio_button_panel)
	{
	}

	void load() override { radio_button_panel->setSelection(CVar::getInt(cvar)); }
	void apply() override { CVar::setInt(cvar, radio_button_panel->getSelection()); }
};
struct TextSettingRow : SettingRow
{
	wxTextCtrl* text_ctrl = nullptr;

	TextSettingRow(const string& cvar, wxTextCtrl* text_ctrl) : SettingRow(cvar), text_ctrl(text_ctrl) {}

	void load() override { text_ctrl->SetValue(wxString::FromUTF8(CVar::getString(cvar))); }
	void apply() override { CVar::set(cvar, text_ctrl->GetValue().utf8_string()); }
};
} // namespace slade::ui


// -----------------------------------------------------------------------------
//
// SettingsTable Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// SettingsTable class constructor
// -----------------------------------------------------------------------------
SettingsTable::SettingsTable(wxWindow* parent) : wxPanel(parent)
{
	layout_ = new LayoutHelper(this);
	sizer_  = new wxGridBagSizer(layout_->pad(), layout_->pad());
	SetSizer(sizer_);
}

// -----------------------------------------------------------------------------
// Adds a section separator with the given [label] to the settings table
// -----------------------------------------------------------------------------
void SettingsTable::addSectionSeparator(const string& label)
{
	auto row = sizer_->GetRows();
	sizer_->Add(wxutil::createSectionSeparator(this, label), { row, 0 }, { 1, 2 }, wxEXPAND | wxTOP, layout_->pad());
	if (row == 0)
		sizer_->AddGrowableCol(1, 1);
}

// -----------------------------------------------------------------------------
// Adds a checkbox setting with the given [label] linked to the given boolean
// [cvar]
// -----------------------------------------------------------------------------
void SettingsTable::addCheckBox(const string& label, const string& cvar)
{
	auto row = sizer_->GetRows();
	addLabel(label, row);
	auto cb = new wxCheckBox(this, wxID_ANY, wxEmptyString);
	sizer_->Add(cb, { row, 1 }, { 1, 1 }, wxEXPAND | wxALIGN_CENTER_VERTICAL);
	settings_.push_back(std::make_unique<CheckSettingRow>(cvar, cb));
	if (row == 0)
		sizer_->AddGrowableCol(1, 1);
}

// -----------------------------------------------------------------------------
// Adds a set of radio buttons with the given [label] linked to the given int
// [cvar]
// -----------------------------------------------------------------------------
void SettingsTable::addRadioButtons(const string& label, const string& cvar, const vector<string>& options)
{
	auto row = sizer_->GetRows();
	addLabel(label, row);
	auto rbp = new RadioButtonPanel(this, options);
	sizer_->Add(rbp, { row, 1 }, { 1, 1 }, wxEXPAND | wxALIGN_CENTER_VERTICAL);
	settings_.push_back(std::make_unique<RadioSettingRow>(cvar, rbp));
	if (row == 0)
		sizer_->AddGrowableCol(1, 1);
}

// -----------------------------------------------------------------------------
// Adds a text box with the given [label] linked to the given string [cvar]
// -----------------------------------------------------------------------------
void SettingsTable::addTextBox(const string& label, const string& cvar)
{
	auto row = sizer_->GetRows();
	addLabel(label, row);
	auto text_ctrl = new wxTextCtrl(this, wxID_ANY);
	sizer_->Add(text_ctrl, { row, 1 }, { 1, 1 }, wxEXPAND | wxALIGN_CENTER_VERTICAL);
	settings_.push_back(std::make_unique<TextSettingRow>(cvar, text_ctrl));
	if (row == 0)
		sizer_->AddGrowableCol(1, 1);
}

// -----------------------------------------------------------------------------
// Adds a custom [control] with the given [label] and layout [flags].
// Loading/applying of the control's value to/from cvars needs to be handled
// manually outside this table.
// -----------------------------------------------------------------------------
void SettingsTable::addCustomControl(const string& label, wxWindow* control, int flags)
{
	auto row = sizer_->GetRows();
	addLabel(label, row);
	sizer_->Add(control, { row, 1 }, { 1, 1 }, flags);
	if (row == 0)
		sizer_->AddGrowableCol(1, 1);
}

// -----------------------------------------------------------------------------
// Loads settings from cvars into the controls
// -----------------------------------------------------------------------------
void SettingsTable::loadSettings() const
{
	for (auto& setting : settings_)
		setting->load();
}

// -----------------------------------------------------------------------------
// Applies settings from the controls to cvars
// -----------------------------------------------------------------------------
void SettingsTable::applySettings() const
{
	for (auto& setting : settings_)
		setting->apply();
}

// -----------------------------------------------------------------------------
// Adds a label control with the given [label] at the given [row]
// -----------------------------------------------------------------------------
void SettingsTable::addLabel(const string& label, int row)
{
	auto label_ctrl = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(label));
	label_ctrl->Wrap(FromDIP(300));
	sizer_->Add(label_ctrl, { row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
}
