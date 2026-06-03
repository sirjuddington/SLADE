
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
#include "NumberSlider.h"
#include "RadioButtonPanel.h"
#include "UI/Layout.h"
#include "UI/WxUtils.h"
#include "Utility/StringUtils.h"

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
// Base struct for a setting row in the table, which may be linked to a cvar.
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
struct SpinSettingRow : SettingRow
{
	wxSpinCtrl* spin_ctrl = nullptr;

	SpinSettingRow(const string& cvar, wxSpinCtrl* spin_ctrl) : SettingRow(cvar), spin_ctrl(spin_ctrl) {}

	void load() override { spin_ctrl->SetValue(CVar::getInt(cvar)); }
	void apply() override { CVar::setInt(cvar, spin_ctrl->GetValue()); }
};
struct SliderSettingRow : SettingRow
{
	NumberSlider* slider  = nullptr;
	bool          decimal = false;

	SliderSettingRow(const string& cvar, NumberSlider* slider, bool decimal) :
		SettingRow(cvar),
		slider(slider),
		decimal(decimal)
	{
	}

	void load() override
	{
		if (decimal)
			slider->setDecimalValue(CVar::getFloat(cvar));
		else
			slider->setValue(CVar::getInt(cvar));
	}

	void apply() override
	{
		if (decimal)
			CVar::setFloat(cvar, slider->decimalValue());
		else
			CVar::setInt(cvar, slider->value());
	}
};
} // namespace slade::ui


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Creates a horizontal sizer containing the given [control] with a tooltip
// label to the right of it (with [pad] spacing between).
// The tooltip label is a small (i) symbol which shows the given [tooltip] text
// when hovered over.
// -----------------------------------------------------------------------------
wxSizer* createTipSizer(wxWindow* parent, wxWindow* control, const string& tooltip, int pad = 0)
{
	// Create tooltip label with (i) symbol
	auto st_tip = new wxStaticText(parent, wxID_ANY, wxString::FromUTF8("\u24D8"));
	st_tip->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_HOTLIGHT));
	st_tip->SetFont(st_tip->GetFont().MakeLarger());
	st_tip->SetToolTip(wxString::FromUTF8(tooltip));

	// Layout to right of control
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	hbox->Add(control, wxSizerFlags(1).Align(wxALIGN_CENTER_VERTICAL));
	hbox->Add(st_tip, wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL).Border(wxLEFT, pad));

	return hbox;
}
} // namespace


// -----------------------------------------------------------------------------
//
// SettingsTable Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SettingsTable class constructor
// -----------------------------------------------------------------------------
SettingsTable::SettingsTable(wxWindow* parent, bool border, const string& top_section_title) : wxPanel(parent)
{
	layout_ = new LayoutHelper(this);
	sizer_  = new wxGridBagSizer(layout_->pad(), layout_->padLarge());

	if (border)
	{
		auto sizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(sizer);
		if (top_section_title.empty())
			sizer->Add(sizer_, layout_->sfWithLargeBorder(1).Expand());
		else
		{
			sizer->AddSpacer(layout_->pad());
			sizer->Add(sizer_, layout_->sfWithLargeBorder(1, wxLEFT | wxRIGHT | wxBOTTOM).Expand());
		}
	}
	else
		SetSizer(sizer_);

	if (!top_section_title.empty())
		addSectionSeparator(top_section_title, true);

	Bind(
		wxEVT_SIZE,
		[this](wxSizeEvent& event)
		{
			if (sizer_->GetCols() > 1 && !sizer_->IsColGrowable(1))
				sizer_->AddGrowableCol(1, 1);
			event.Skip();
		});
}

// -----------------------------------------------------------------------------
// Adds a section separator with the given [label] to the settings table
// -----------------------------------------------------------------------------
void SettingsTable::addSectionSeparator(const string& label, bool top)
{
	auto row = sizer_->GetRows();
	if (top)
		sizer_->Add(new wxStaticText(this, -1, wxString::FromUTF8(label)), { row, 0 }, { 1, 2 }, wxEXPAND);
	else
		sizer_->Add(
			wxutil::createSectionSeparator(this, label), { row, 0 }, { 1, 2 }, wxEXPAND | wxTOP, layout_->pad());
	in_section_ = true;
}

// -----------------------------------------------------------------------------
// Adds a checkbox setting with the given [label] linked to the given boolean
// [cvar]
// -----------------------------------------------------------------------------
void SettingsTable::addCheckBox(const string& label, const string& cvar)
{
	// Get label text and optional tooltip
	string text = label;
	string tooltip;
	if (strutil::contains(label, "|"))
	{
		text    = strutil::beforeFirst(label, "|");
		tooltip = strutil::afterFirst(label, "|");
	}

	auto row      = sizer_->GetRows();
	auto cb       = new wxCheckBox(this, wxID_ANY, wxString::FromUTF8(text));
	auto left_pad = in_section_ ? layout_->padLarge() : 0;

	if (!tooltip.empty())
		sizer_->Add(
			createTipSizer(this, cb, tooltip), { row, 0 }, { 1, 2 }, wxALIGN_CENTER_VERTICAL | wxLEFT, left_pad);
	else
		sizer_->Add(cb, { row, 0 }, { 1, 2 }, wxALIGN_CENTER_VERTICAL | wxLEFT, left_pad);

	settings_.push_back(std::make_unique<CheckSettingRow>(cvar, cb));
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
}

// -----------------------------------------------------------------------------
// Adds a spin control with the given [label] linked to the given int [cvar],
// constructed with the given [min], [max] and [initial] values
// -----------------------------------------------------------------------------
void SettingsTable::addSpinControl(const string& label, const string& cvar, int min, int max, int initial)
{
	auto row = sizer_->GetRows();
	addLabel(label, row);
	auto spin_ctrl = new wxSpinCtrl(
		this,
		wxID_ANY,
		wxEmptyString,
		wxDefaultPosition,
		layout_->spinSize(),
		wxSP_ARROW_KEYS | wxTE_PROCESS_ENTER,
		min,
		max,
		initial);
	sizer_->Add(spin_ctrl, { row, 1 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	settings_.push_back(std::make_unique<SpinSettingRow>(cvar, spin_ctrl));
}

// -----------------------------------------------------------------------------
// Adds a slider with the given [label] linked to the given int/float [cvar],
// constructed with the given [min], [max], [step] and [scale] values.
// If [decimal] is true, the slider will be treated as a float with the given
// [scale] factor (see NumberSlider)
// -----------------------------------------------------------------------------
void SettingsTable::addSlider(
	const string& label,
	const string& cvar,
	bool          decimal,
	int           min,
	int           max,
	int           step,
	int           scale)
{
	auto row = sizer_->GetRows();
	addLabel(label, row);
	auto slider = new NumberSlider(this, min, max, step, decimal, scale);
	sizer_->Add(slider, { row, 1 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	settings_.push_back(std::make_unique<SliderSettingRow>(cvar, slider, decimal));
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
}

// -----------------------------------------------------------------------------
// Adds a custom [sizer] with the given [label] and layout [flags].
// Loading/applying of the control's value to/from cvars needs to be handled
// manually outside this table.
// -----------------------------------------------------------------------------
void SettingsTable::addCustomSizer(const string& label, wxSizer* sizer, int flags)
{
	auto row = sizer_->GetRows();
	addLabel(label, row);
	sizer_->Add(sizer, { row, 1 }, { 1, 1 }, flags);
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
	// Get label text and optional tooltip
	string tooltip;
	string text = label;
	if (strutil::contains(label, "|"))
	{
		tooltip = strutil::afterFirst(label, "|");
		text    = strutil::beforeFirst(label, "|");
	}

	auto wxst     = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(text));
	auto left_pad = in_section_ ? layout_->padLarge() : 0;
	wxst->Wrap(FromDIP(300) - left_pad);

	if (!tooltip.empty())
		sizer_->Add(
			createTipSizer(this, wxst, tooltip, layout_->padSmall()),
			{ row, 0 },
			{ 1, 1 },
			wxALIGN_CENTER_VERTICAL | wxLEFT,
			left_pad);
	else
		sizer_->Add(wxst, { row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL | wxLEFT, left_pad);
}
