
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
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
#include "UI/Controls/STabCtrl.h"
#include "UI/Controls/SettingsTable.h"
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
EXTERN_CVAR(Int, toolbar_size)
EXTERN_CVAR(Int, elist_icon_size)


// ----------------------------------------------------------------------------
//
// Functions
//
// ----------------------------------------------------------------------------
namespace
{
// ----------------------------------------------------------------------------
// Returns an appropriate icon size for the selected size option, allowing
// custom size within range if one is currently set
// ----------------------------------------------------------------------------
int iconSize(int current, int selection)
{
	// Small (16), allow from 1-16
	if (selection == 0 && current > 16)
		return 16;

	// Medium (24), allow from 17-24
	if (selection == 1 && (current > 24 || current <= 16))
		return 24;

	// Large (32), allow anything above 24
	if (selection == 2 && current <= 24)
		return 32;

	return current;
}
} // namespace


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

	auto tabs       = STabCtrl::createControl(this);
	settings_table_ = new SettingsTable(tabs, true, "Appearance");
	colour_panel_   = new ColourSettingsPanel(this);

	setupInterfaceSettingsTable();

	tabs->AddPage(settings_table_, wxS("Interface"));
	tabs->AddPage(wxutil::createPadPanel(tabs, colour_panel_, padLarge()), wxS("Colours && Theme"));
	sizer->Add(tabs, wxSizerFlags(1).Expand());
}

// ----------------------------------------------------------------------------
// Loads settings from cvars into the controls
// ----------------------------------------------------------------------------
void InterfaceSettingsPanel::loadSettings()
{
	// Toolbar icon size
	if (toolbar_size <= 16)
		choice_toolbar_size_->Select(0);
	else if (toolbar_size <= 24)
		choice_toolbar_size_->Select(1);
	else
		choice_toolbar_size_->Select(2);

	// Toolbar icon set
	auto iconset_general = wxString::FromUTF8(CVar::getString("iconset_general"));
	choice_toolbar_iconset_->SetSelection(0);
	for (unsigned a = 0; a < choice_toolbar_iconset_->GetCount(); a++)
		if (choice_toolbar_iconset_->GetString(a) == iconset_general)
		{
			choice_toolbar_iconset_->SetSelection(a);
			break;
		}

	// Entrylist icon set
	auto iconset_entry_list = wxString::FromUTF8(CVar::getString("iconset_entry_list"));
	choice_iconset_entry_->SetSelection(0);
	for (unsigned a = 0; a < choice_iconset_entry_->GetCount(); a++)
		if (choice_iconset_entry_->GetString(a) == iconset_entry_list)
		{
			choice_iconset_entry_->SetSelection(a);
			break;
		}

	// Entrylist icon size
	if (elist_icon_size <= 16)
		choice_elist_icon_size_->Select(0);
	else if (elist_icon_size <= 24)
		choice_elist_icon_size_->Select(1);
	else
		choice_elist_icon_size_->Select(2);

	settings_table_->loadSettings();
	colour_panel_->loadSettings();
}

// ----------------------------------------------------------------------------
// Applies settings from the controls to cvars
// ----------------------------------------------------------------------------
void InterfaceSettingsPanel::applySettings()
{
	// Toolbar icons
	CVar::set(
		"iconset_general", choice_toolbar_iconset_->GetString(choice_toolbar_iconset_->GetSelection()).utf8_string());
	toolbar_size = iconSize(toolbar_size, choice_toolbar_size_->GetSelection());

	// Entry List
	CVar::set(
		"iconset_entry_list", choice_iconset_entry_->GetString(choice_iconset_entry_->GetSelection()).utf8_string());
	elist_icon_size = iconSize(elist_icon_size, choice_elist_icon_size_->GetSelection());

	settings_table_->applySettings();
	colour_panel_->applySettings();
}

// ----------------------------------------------------------------------------
// Creates the Interface tab panel
// ----------------------------------------------------------------------------
void InterfaceSettingsPanel::setupInterfaceSettingsTable()
{
	auto lh = LayoutHelper(settings_table_);

	// Create controls
	choice_toolbar_iconset_ = new wxChoice(
		settings_table_, -1, wxDefaultPosition, wxDefaultSize, wxutil::arrayStringStd(icons::iconSets(icons::General)));
	choice_toolbar_size_ = new wxChoice(
		settings_table_, -1, wxDefaultPosition, wxDefaultSize, wxutil::arrayStringStd({ "16x16", "24x24", "32x32" }));
	choice_iconset_entry_ = new wxChoice(
		settings_table_, -1, wxDefaultPosition, wxDefaultSize, wxutil::arrayStringStd(icons::iconSets(icons::Entry)));
	choice_elist_icon_size_ = new wxChoice(
		settings_table_, -1, wxDefaultPosition, wxDefaultSize, wxutil::arrayStringStd({ "16x16", "24x24", "32x32" }));

	// Appearance
#ifdef __WXMSW__
	settings_table_->addRadioButtons(
		"Application theme|"
		"Dark theme is only supported on Windows 10 20H1 or later",
		"win_darkmode",
		{ "Light", "System", "Dark" });
#endif
	settings_table_->addCheckBox("Use monospace font in lists", "list_font_monospace");
	settings_table_->addCheckBox("Condensed tabs", "tabs_condensed");
	settings_table_->addCustomSizer(
		"Toolbar icons", lh.layoutHorizontally({ choice_toolbar_iconset_, choice_toolbar_size_ }));

	// Entry List
	settings_table_->addSectionSeparator("Entry List");
	settings_table_->addCheckBox("Colour background by entry type", "elist_type_bgcol");
	settings_table_->addCustomSizer("Icons", lh.layoutHorizontally({ choice_iconset_entry_, choice_elist_icon_size_ }));
	settings_table_->addSpinControl("Row padding", "elist_icon_padding", 0, 4, 1);

	// Data Grid
	settings_table_->addSectionSeparator("Data Grid");
	settings_table_->addSpinControl("Row padding", "edata_row_padding", 0, 4, 0);
	settings_table_->addSpinControl("Column padding", "edata_col_padding", 0, 4, 0);
	settings_table_->addCheckBox("Use monospace font in data grid", "edata_font_monospace");
}
