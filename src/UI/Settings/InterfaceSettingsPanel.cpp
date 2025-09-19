
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    InterfaceSettingsPanel.cpp
// Description: Panel containing interface setting controls
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


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#include "InterfaceSettingsPanel.h"
#include "ColourSettingsPanel.h"
#include "Graphics/Icons.h"
#include "UI/Controls/RadioButtonPanel.h"
#include "UI/Controls/STabCtrl.h"
#include "UI/Layout.h"
#include "UI/UI.h"
#include "UI/WxUtils.h"

using namespace slade;
using namespace ui;


// ----------------------------------------------------------------------------
//
// External Variables
//
// ----------------------------------------------------------------------------
EXTERN_CVAR(Bool, list_font_monospace)
EXTERN_CVAR(Bool, elist_type_bgcol)
EXTERN_CVAR(Int, toolbar_size)
EXTERN_CVAR(Bool, am_file_browser_tab) // keep? move?
EXTERN_CVAR(String, iconset_general)
EXTERN_CVAR(String, iconset_entry_list)
EXTERN_CVAR(Bool, tabs_condensed)
EXTERN_CVAR(Int, elist_icon_size)
EXTERN_CVAR(Int, elist_icon_padding)
EXTERN_CVAR(Int, win_darkmode)


// ----------------------------------------------------------------------------
//
// InterfaceSettingsPanel Class Functions
//
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// InterfaceSettingsPanel class constructor
// ----------------------------------------------------------------------------
InterfaceSettingsPanel::InterfaceSettingsPanel(wxWindow* parent) : SettingsPanel(parent)
{
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	colour_panel_ = new ColourSettingsPanel(this);

	auto tabs = STabCtrl::createControl(this);
	tabs->AddPage(createInterfacePanel(tabs), wxS("Interface"));
	tabs->AddPage(wxutil::createPadPanel(tabs, colour_panel_, padLarge()), wxS("Colours && Theme"));
	sizer->Add(tabs, wxSizerFlags(1).Expand());
}

// ----------------------------------------------------------------------------
// Loads settings from cvars into the controls
// ----------------------------------------------------------------------------
void InterfaceSettingsPanel::loadSettings()
{
	cb_monospace_list_->SetValue(list_font_monospace);
	cb_elist_bgcol_->SetValue(elist_type_bgcol);
	cb_condensed_tabs_->SetValue(tabs_condensed);
	spin_elist_icon_pad_->SetValue(elist_icon_padding);

	if (toolbar_size <= 16)
		choice_toolbar_size_->Select(0);
	else if (toolbar_size <= 24)
		choice_toolbar_size_->Select(1);
	else
		choice_toolbar_size_->Select(2);

	choice_toolbar_iconset_->SetSelection(0);
	for (unsigned a = 0; a < choice_toolbar_iconset_->GetCount(); a++)
		if (choice_toolbar_iconset_->GetString(a) == iconset_general)
		{
			choice_toolbar_iconset_->SetSelection(a);
			break;
		}

	choice_iconset_entry_->SetSelection(0);
	for (unsigned a = 0; a < choice_iconset_entry_->GetCount(); a++)
		if (choice_iconset_entry_->GetString(a) == iconset_entry_list)
		{
			choice_iconset_entry_->SetSelection(a);
			break;
		}

	if (elist_icon_size <= 16)
		choice_elist_icon_size_->Select(0);
	else if (elist_icon_size <= 24)
		choice_elist_icon_size_->Select(1);
	else
		choice_elist_icon_size_->Select(2);

	rbp_windows_darkmode_->setSelection(win_darkmode);

	colour_panel_->loadSettings();
}

// ----------------------------------------------------------------------------
// Applies settings from the controls to cvars
// ----------------------------------------------------------------------------
void InterfaceSettingsPanel::applySettings()
{
	win_darkmode        = rbp_windows_darkmode_->getSelection();
	list_font_monospace = cb_monospace_list_->GetValue();
	tabs_condensed      = cb_condensed_tabs_->GetValue();

	// Toolbar icons
	iconset_general = choice_toolbar_iconset_->GetString(choice_toolbar_iconset_->GetSelection()).utf8_string();
	if (choice_toolbar_size_->GetSelection() == 0)
		toolbar_size = 16;
	else if (choice_toolbar_size_->GetSelection() == 1)
		toolbar_size = 24;
	else
		toolbar_size = 32;

	// Entry List
	iconset_entry_list = choice_iconset_entry_->GetString(choice_iconset_entry_->GetSelection()).utf8_string();
	elist_icon_padding = spin_elist_icon_pad_->GetValue();
	if (choice_elist_icon_size_->GetSelection() == 0)
		elist_icon_size = 16;
	else if (choice_elist_icon_size_->GetSelection() == 1)
		elist_icon_size = 24;
	else
		elist_icon_size = 32;
	elist_type_bgcol = cb_elist_bgcol_->GetValue();

	colour_panel_->applySettings();
}

// ----------------------------------------------------------------------------
// Creates the Interface tab panel
// ----------------------------------------------------------------------------
wxPanel* InterfaceSettingsPanel::createInterfacePanel(wxWindow* parent)
{
	auto panel = new wxPanel(parent, -1);
	auto lh    = LayoutHelper(panel);

	// Create controls

	// Appearance
	vector<string> darkmodes = { "Off", "Use System Setting", "On" };
	rbp_windows_darkmode_    = new RadioButtonPanel(panel, darkmodes, "Use dark UI theme if supported:");
	cb_monospace_list_       = new wxCheckBox(panel, -1, wxS("Use monospace font in lists"));
	cb_condensed_tabs_       = new wxCheckBox(panel, -1, wxS("Condensed tabs"));
	vector<string> sizes     = { "16x16", "24x24", "32x32" };
	choice_toolbar_size_     = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, wxutil::arrayStringStd(sizes));
	auto sets_toolbar        = wxutil::arrayStringStd(icons::iconSets(icons::General));
	choice_toolbar_iconset_  = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, sets_toolbar);

	// Entry List
	cb_elist_bgcol_           = new wxCheckBox(panel, -1, wxS("Colour entry list item background by entry type"));
	auto sets_entry           = wxutil::arrayStringStd(icons::iconSets(icons::Entry));
	choice_iconset_entry_     = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, sets_entry);
	vector<string> icon_sizes = { "16x16", "24x24", "32x32" };
	choice_elist_icon_size_   = new wxChoice(
        panel, -1, wxDefaultPosition, wxDefaultSize, wxutil::arrayStringStd(icon_sizes));
	spin_elist_icon_pad_ = new wxSpinCtrl(
		panel, -1, wxS("1"), wxDefaultPosition, lh.spinSize(), wxSP_ARROW_KEYS | wxTE_PROCESS_ENTER, 0, 4, 1);
	vector<string> tree_styles = { "Tree", "Flat List" };

	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	auto vbox = new wxBoxSizer(wxVERTICAL);
	sizer->Add(vbox, lh.sfWithLargeBorder(0).Expand());

	// Appearance settings
	vbox->Add(wxutil::createSectionSeparator(panel, "Appearance"), lh.sfWithBorder(0, wxBOTTOM).Expand());
	vbox->Add(layoutAppearanceSettings(panel), lh.sfWithBorder(0, wxLEFT));

	// Entry List settings
	vbox->AddSpacer(lh.padXLarge());
	vbox->Add(wxutil::createSectionSeparator(panel, "Entry List"), lh.sfWithBorder(0, wxBOTTOM).Expand());
	vbox->Add(layoutEntryListSettings(panel), lh.sfWithBorder(0, wxLEFT));

	return panel;
}

// ----------------------------------------------------------------------------
// Creates layout sizer for the Appearance settings
// ----------------------------------------------------------------------------
wxSizer* InterfaceSettingsPanel::layoutAppearanceSettings(wxWindow* panel) const
{
	auto sizer = new wxGridBagSizer(pad(panel), padLarge(panel));

	int row = 0;
#ifdef __WXMSW__
	sizer->Add(rbp_windows_darkmode_, { row++, 0 }, { 1, 3 }, wxEXPAND);
#else
	rbp_windows_darkmode_->Hide();
#endif
	sizer->Add(new wxStaticText(panel, -1, wxS("Toolbar icon set:")), { row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(choice_toolbar_iconset_, { row, 1 }, { 1, 1 }, wxEXPAND);
	sizer->Add(choice_toolbar_size_, { row++, 2 }, { 1, 1 }, wxEXPAND);
	sizer->Add(cb_monospace_list_, { row++, 0 }, { 1, 3 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(cb_condensed_tabs_, { row, 0 }, { 1, 3 }, wxALIGN_CENTER_VERTICAL);

	sizer->AddGrowableCol(1);
	sizer->AddGrowableCol(2);

	return sizer;
}

// ----------------------------------------------------------------------------
// Creates layout sizer for the Entry List settings
// ----------------------------------------------------------------------------
wxSizer* InterfaceSettingsPanel::layoutEntryListSettings(wxWindow* panel) const
{
	auto sizer = new wxGridBagSizer(pad(panel), padLarge(panel));

	auto row = 0;
	sizer->Add(new wxStaticText(panel, -1, wxS("Icon set:")), { row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(choice_iconset_entry_, { row, 1 }, { 1, 1 }, wxEXPAND);
	sizer->Add(choice_elist_icon_size_, { row++, 2 }, { 1, 1 }, wxEXPAND);
	sizer->Add(new wxStaticText(panel, -1, wxS("Row spacing:")), { row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(spin_elist_icon_pad_, { row++, 1 }, { 1, 2 }, wxEXPAND);
	sizer->Add(cb_elist_bgcol_, { row++, 0 }, { 1, 3 }, wxALIGN_CENTER_VERTICAL);

	sizer->AddGrowableCol(1);

	return sizer;
}
