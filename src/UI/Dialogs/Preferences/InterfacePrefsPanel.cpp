
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    InterfacePrefsPanel.cpp
// Description: Panel containing interface preference controls
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
#include "InterfacePrefsPanel.h"
#include "General/UI.h"
#include "Graphics/Icons.h"
#include "UI/Controls/STabCtrl.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, size_as_string)
EXTERN_CVAR(Bool, elist_filter_dirs)
EXTERN_CVAR(Bool, show_start_page)
EXTERN_CVAR(Bool, swap_epanel_bars)
EXTERN_CVAR(Bool, context_submenus)
EXTERN_CVAR(Bool, list_font_monospace)
EXTERN_CVAR(Bool, elist_type_bgcol)
EXTERN_CVAR(Int, toolbar_size)
EXTERN_CVAR(Int, tab_style)
EXTERN_CVAR(Bool, am_file_browser_tab)
EXTERN_CVAR(String, iconset_general)
EXTERN_CVAR(String, iconset_entry_list)
EXTERN_CVAR(Bool, tabs_condensed)
EXTERN_CVAR(Bool, web_dark_theme)
EXTERN_CVAR(Int, elist_icon_size)
EXTERN_CVAR(Int, elist_icon_padding)
EXTERN_CVAR(Bool, elist_no_tree)
EXTERN_CVAR(Int, win_darkmode)


// -----------------------------------------------------------------------------
//
// InterfacePrefsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// InterfacePrefsPanel class constructor
// -----------------------------------------------------------------------------
InterfacePrefsPanel::InterfacePrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create sizer
	auto psizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(psizer);

	// Add tabs
	auto stc_tabs = STabCtrl::createControl(this);
	psizer->Add(stc_tabs, 1, wxEXPAND);
	stc_tabs->AddPage(setupGeneralTab(stc_tabs), "General");
	stc_tabs->AddPage(setupEntryListTab(stc_tabs), "Entry List");
}

// -----------------------------------------------------------------------------
// Initialises panel controls
// -----------------------------------------------------------------------------
void InterfacePrefsPanel::init()
{
	cb_size_as_string_->SetValue(size_as_string);
	cb_filter_dirs_->SetValue(!elist_filter_dirs);
	cb_list_monospace_->SetValue(list_font_monospace);
	cb_start_page_->SetValue(show_start_page);
	cb_context_submenus_->SetValue(context_submenus);
	cb_elist_bgcol_->SetValue(elist_type_bgcol);
	cb_file_browser_->SetValue(am_file_browser_tab);
	cb_condensed_tabs_->SetValue(tabs_condensed);
	cb_web_dark_theme_->SetValue(web_dark_theme);
	spin_elist_icon_pad_->SetValue(elist_icon_padding);

	if (toolbar_size <= 16)
		choice_toolbar_size_->Select(0);
	else if (toolbar_size <= 24)
		choice_toolbar_size_->Select(1);
	else
		choice_toolbar_size_->Select(2);

	choice_iconset_general_->SetSelection(0);
	for (unsigned a = 0; a < choice_iconset_general_->GetCount(); a++)
		if (choice_iconset_general_->GetString(a) == iconset_general)
		{
			choice_iconset_general_->SetSelection(a);
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

	choice_elist_tree_style_->Select(elist_no_tree ? 1 : 0);

	choice_windows_darkmode_->SetSelection(win_darkmode);
}

// -----------------------------------------------------------------------------
// Applies preference values from the controls to CVARs
// -----------------------------------------------------------------------------
void InterfacePrefsPanel::applyPreferences()
{
	size_as_string      = cb_size_as_string_->GetValue();
	elist_filter_dirs   = !cb_filter_dirs_->GetValue();
	list_font_monospace = cb_list_monospace_->GetValue();
	show_start_page     = cb_start_page_->GetValue();
	context_submenus    = cb_context_submenus_->GetValue();
	elist_type_bgcol    = cb_elist_bgcol_->GetValue();
	am_file_browser_tab = cb_file_browser_->GetValue();
	tabs_condensed      = cb_condensed_tabs_->GetValue();
	web_dark_theme      = cb_web_dark_theme_->GetValue();
	elist_icon_padding  = spin_elist_icon_pad_->GetValue();

	if (choice_toolbar_size_->GetSelection() == 0)
		toolbar_size = 16;
	else if (choice_toolbar_size_->GetSelection() == 1)
		toolbar_size = 24;
	else
		toolbar_size = 32;

	iconset_general    = wxutil::strToView(choice_iconset_general_->GetString(choice_iconset_general_->GetSelection()));
	iconset_entry_list = wxutil::strToView(choice_iconset_entry_->GetString(choice_iconset_entry_->GetSelection()));

	if (choice_elist_icon_size_->GetSelection() == 0)
		elist_icon_size = 16;
	else if (choice_elist_icon_size_->GetSelection() == 1)
		elist_icon_size = 24;
	else
		elist_icon_size = 32;

	elist_no_tree = choice_elist_tree_style_->GetSelection() == 1;

	win_darkmode = choice_windows_darkmode_->GetSelection();
}

// -----------------------------------------------------------------------------
// Creates and returns the panel for the 'General' tab
// -----------------------------------------------------------------------------
wxPanel* InterfacePrefsPanel::setupGeneralTab(wxWindow* stc_tabs)
{
	auto panel = new wxPanel(stc_tabs, -1);

	// Create controls
	cb_start_page_           = new wxCheckBox(panel, -1, "Show Start Page on Startup");
	wxString darkmodes[]     = { "Off", "Use System Setting", "On" };
	choice_windows_darkmode_ = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, 3, darkmodes);
	choice_windows_darkmode_->SetToolTip("Only supported on Windows 10 20H1 or later");
	cb_web_dark_theme_       = new wxCheckBox(panel, -1, "Use dark theme for web content *");
	cb_web_dark_theme_->SetToolTip("Use a dark theme for web content eg. the Start Page and Online Documentation");
	cb_file_browser_        = new wxCheckBox(panel, -1, "Show File Browser tab in the Archive Manager panel *");
	cb_list_monospace_      = new wxCheckBox(panel, -1, "Use monospaced font for lists");
	cb_condensed_tabs_      = new wxCheckBox(panel, -1, "Condensed tabs *");
	wxString sizes[]        = { "16x16", "24x24", "32x32" };
	choice_toolbar_size_    = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, 3, sizes);
	auto sets               = wxutil::arrayStringStd(icons::iconSets(icons::General));
	choice_iconset_general_ = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, sets);

	// Layout
	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);
	auto gb_sizer = new wxGridBagSizer(ui::pad(), ui::pad());
	sizer->Add(gb_sizer, 1, wxALL | wxEXPAND, ui::padLarge());

	int row = 0;
	gb_sizer->Add(cb_start_page_, { row++, 0 }, { 1, 2 }, wxEXPAND);
#if defined(__WXMSW__) && wxCHECK_VERSION(3, 3, 0)
	gb_sizer->Add(new wxStaticText(panel, -1, "Use dark UI theme if supported:"), { row, 0 }, { 1, 1 }, wxALIGN_CENTRE_VERTICAL);
	gb_sizer->Add(choice_windows_darkmode_, { row, 1 }, { 1, 1 }, wxEXPAND);
	gb_sizer->Add(new wxStaticText(panel, -1, "*"), { row++, 2 }, { 1, 1 }, wxALIGN_CENTRE_VERTICAL);
#else
	choice_windows_darkmode_->Show(false);
#endif
	gb_sizer->Add(cb_web_dark_theme_, { row++, 0 }, { 1, 2 }, wxEXPAND);
	gb_sizer->Add(cb_file_browser_, { row++, 0 }, { 1, 2 }, wxEXPAND);
	gb_sizer->Add(cb_list_monospace_, { row++, 0 }, { 1, 2 }, wxEXPAND);
	gb_sizer->Add(cb_condensed_tabs_, { row++, 0 }, { 1, 2 }, wxEXPAND);
	gb_sizer->Add(new wxStaticText(panel, -1, "Toolbar icon size:"), { row, 0 }, { 1, 1 }, wxALIGN_CENTRE_VERTICAL);
	gb_sizer->Add(choice_toolbar_size_, { row, 1 }, { 1, 1 }, wxEXPAND);
	gb_sizer->Add(new wxStaticText(panel, -1, "*"), { row++, 2 }, { 1, 1 }, wxALIGN_CENTRE_VERTICAL);
	gb_sizer->Add(new wxStaticText(panel, -1, "Icons:"), { row, 0 }, { 1, 1 }, wxALIGN_CENTRE_VERTICAL);
	gb_sizer->Add(choice_iconset_general_, { row, 1 }, { 1, 1 }, wxEXPAND);
	gb_sizer->Add(new wxStaticText(panel, -1, "*"), { row++, 2 }, { 1, 1 }, wxALIGN_CENTRE_VERTICAL);

	gb_sizer->AddGrowableCol(1, 1);
	sizer->Add(new wxStaticText(panel, -1, "* requires restart to take effect"), 0, wxALL | wxALIGN_RIGHT, ui::pad());

	return panel;
}

// -----------------------------------------------------------------------------
// Creates and returns the panel for the 'Entry List' tab
// -----------------------------------------------------------------------------
wxPanel* InterfacePrefsPanel::setupEntryListTab(wxWindow* stc_tabs)
{
	auto panel = new wxPanel(stc_tabs, -1);

	// Create controls
	cb_size_as_string_      = new wxCheckBox(panel, -1, "Show entry size as a string with units");
	cb_filter_dirs_         = new wxCheckBox(panel, -1, "Ignore directories when filtering by name");
	cb_elist_bgcol_         = new wxCheckBox(panel, -1, "Colour entry list item background by entry type");
	cb_context_submenus_    = new wxCheckBox(panel, -1, "Group related entry context menu items into submenus");
	auto sets               = wxutil::arrayStringStd(icons::iconSets(icons::Entry));
	choice_iconset_entry_   = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, sets);
	wxString icon_sizes[]   = { "16x16", "24x24", "32x32" };
	choice_elist_icon_size_ = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, 3, icon_sizes);
	spin_elist_icon_pad_    = new wxSpinCtrl(
        panel,
        -1,
        "1",
        wxDefaultPosition,
        { ui::px(ui::Size::SpinCtrlWidth), -1 },
        wxSP_ARROW_KEYS | wxTE_PROCESS_ENTER,
        0,
        4,
        1);
	wxString tree_styles[]   = { "Tree", "Flat List" };
	choice_elist_tree_style_ = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, 2, tree_styles);
	choice_elist_tree_style_->SetToolTip("The list style to use when the archive supports folders");

	// Layout
	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);
	auto gb_sizer = new wxGridBagSizer(ui::pad(), ui::pad());
	sizer->Add(gb_sizer, 1, wxALL | wxEXPAND, ui::padLarge());

	int row = 0;
	gb_sizer->Add(cb_size_as_string_, { row++, 0 }, { 1, 4 }, wxEXPAND);
	gb_sizer->Add(cb_filter_dirs_, { row++, 0 }, { 1, 4 }, wxEXPAND);
	gb_sizer->Add(cb_context_submenus_, { row++, 0 }, { 1, 4 }, wxEXPAND);
	gb_sizer->Add(cb_elist_bgcol_, { row++, 0 }, { 1, 4 }, wxEXPAND);
	gb_sizer->Add(new wxStaticText(panel, -1, "Folder List Style:"), { row, 0 }, { 1, 1 }, wxALIGN_CENTRE_VERTICAL);
	gb_sizer->Add(choice_elist_tree_style_, { row, 1 }, { 1, 3 }, wxEXPAND);
	gb_sizer->Add(new wxStaticText(panel, -1, "*"), { row++, 4 }, { 1, 1 }, wxALIGN_CENTRE_VERTICAL);
	gb_sizer->Add(new wxStaticText(panel, -1, "Icon size:"), { row, 0 }, { 1, 1 }, wxALIGN_CENTRE_VERTICAL);
	gb_sizer->Add(choice_elist_icon_size_, { row, 1 }, { 1, 1 }, wxEXPAND);
	gb_sizer->Add(new wxStaticText(panel, -1, "Padding:"), { row, 2 }, { 1, 1 }, wxALIGN_CENTRE_VERTICAL);
	gb_sizer->Add(spin_elist_icon_pad_, { row, 3 }, { 1, 1 }, wxEXPAND);
	gb_sizer->Add(new wxStaticText(panel, -1, "*"), { row++, 4 }, { 1, 1 }, wxALIGN_CENTRE_VERTICAL);
	gb_sizer->Add(new wxStaticText(panel, -1, "Icons:"), { row, 0 }, { 1, 1 }, wxALIGN_CENTRE_VERTICAL);
	gb_sizer->Add(choice_iconset_entry_, { row, 1 }, { 1, 3 }, wxEXPAND);
	gb_sizer->Add(new wxStaticText(panel, -1, "*"), { row++, 4 }, { 1, 1 }, wxALIGN_CENTRE_VERTICAL);

	gb_sizer->AddGrowableCol(1, 1);
	sizer->Add(new wxStaticText(panel, -1, "* requires restart to take effect"), 0, wxALL | wxALIGN_RIGHT, ui::pad());

	return panel;
}
