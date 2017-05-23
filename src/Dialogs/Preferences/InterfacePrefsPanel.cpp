
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    InterfacePrefsPanel.cpp
 * Description: Panel containing interface preference controls
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
#include "InterfacePrefsPanel.h"
#include "Graphics/Icons.h"
#include "UI/STabCtrl.h"


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
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


/*******************************************************************
 * INTERFACEPREFSPANEL CLASS FUNCTIONS
 *******************************************************************/

/* InterfacePrefsPanel::InterfacePrefsPanel
 * InterfacePrefsPanel class constructor
 *******************************************************************/
InterfacePrefsPanel::InterfacePrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create sizer
	wxBoxSizer* psizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(psizer);

	// Create frame+sizer
	wxStaticBox* frame = new wxStaticBox(this, -1, "Interface Preferences");
	wxStaticBoxSizer* fsizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	psizer->Add(fsizer, 1, wxEXPAND|wxALL, 4);

	TabControl* stc_tabs = STabCtrl::createControl(this);
	fsizer->Add(stc_tabs, 1, wxEXPAND | wxALL, 4);

	// --- General ---
	wxPanel* panel = new wxPanel(stc_tabs, -1);
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);
	wxGridBagSizer*	gb_sizer = new wxGridBagSizer(8, 8);
	sizer->Add(gb_sizer, 1, wxALL | wxEXPAND, 8);
	stc_tabs->AddPage(panel, "General");

	// Show startpage
	int row = 0;
	cb_start_page = new wxCheckBox(panel, -1, "Show Start Page on Startup");
	gb_sizer->Add(cb_start_page, wxGBPosition(row++, 0), wxGBSpan(1, 2), wxEXPAND);

	// Show file browser
	cb_file_browser = new wxCheckBox(panel, -1, "Show File Browser tab in the Archive Manager panel *");
	gb_sizer->Add(cb_file_browser, wxGBPosition(row++, 0), wxGBSpan(1, 2), wxEXPAND);

	// Monospace list font
	cb_list_monospace = new wxCheckBox(panel, -1, "Use monospaced font for lists");
	gb_sizer->Add(cb_list_monospace, wxGBPosition(row++, 0), wxGBSpan(1, 2), wxEXPAND);

	// Condensed Tabs
	cb_condensed_tabs = new wxCheckBox(panel, -1, "Condensed tabs *");
	gb_sizer->Add(cb_condensed_tabs, wxGBPosition(row++, 0), wxGBSpan(1, 2), wxEXPAND);

	// Toolbar size
	string sizes[] = { "Small (16px)", "Medium (24px)", "Large (32px)" };
	choice_toolbar_size = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, 3, sizes);
	gb_sizer->Add(new wxStaticText(panel, -1, "Toolbar icon size:"), wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTRE_VERTICAL);
	gb_sizer->Add(choice_toolbar_size, wxGBPosition(row, 1), wxDefaultSpan, wxEXPAND);
	gb_sizer->Add(new wxStaticText(panel, -1, "*"), wxGBPosition(row++, 2), wxDefaultSpan, wxALIGN_CENTRE_VERTICAL);

	// Icon set
	vector<string> sets = Icons::getIconSets(Icons::GENERAL);
	choice_iconset_general = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, sets.size(), &sets[0]);
	gb_sizer->Add(new wxStaticText(panel, -1, "Icons:"), wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTRE_VERTICAL);
	gb_sizer->Add(choice_iconset_general, wxGBPosition(row, 1), wxDefaultSpan, wxEXPAND);
	gb_sizer->Add(new wxStaticText(panel, -1, "*"), wxGBPosition(row++, 2), wxDefaultSpan, wxALIGN_CENTRE_VERTICAL);

	gb_sizer->AddGrowableCol(1, 1);
	sizer->Add(new wxStaticText(panel, -1, "* requires restart to take effect"), 0, wxALL | wxALIGN_RIGHT, 8);


	// --- Entry List ---
	panel = new wxPanel(stc_tabs, -1);
	sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);
	gb_sizer = new wxGridBagSizer(8, 8);
	sizer->Add(gb_sizer, 1, wxALL | wxEXPAND, 8);
	stc_tabs->AddPage(panel, "Entry List");

	// Show entry size as string instead of a number
	row = 0;
	cb_size_as_string = new wxCheckBox(panel, -1, "Show entry size as a string with units");
	gb_sizer->Add(cb_size_as_string, wxGBPosition(row++, 0), wxGBSpan(1, 2), wxEXPAND);

	// Filter directories
	cb_filter_dirs = new wxCheckBox(panel, -1, "Ignore directories when filtering by name");
	gb_sizer->Add(cb_filter_dirs, wxGBPosition(row++, 0), wxGBSpan(1, 2), wxEXPAND);

	// Entry list background colour by type
	cb_elist_bgcol = new wxCheckBox(panel, -1, "Colour entry list item background by entry type");
	gb_sizer->Add(cb_elist_bgcol, wxGBPosition(row++, 0), wxGBSpan(1, 2), wxEXPAND);

	// Context menu submenus
	cb_context_submenus = new wxCheckBox(panel, -1, "Group related entry context menu items into submenus");
	gb_sizer->Add(cb_context_submenus, wxGBPosition(row++, 0), wxGBSpan(1, 2), wxEXPAND);

	// Icon set
	sets = Icons::getIconSets(Icons::ENTRY);
	choice_iconset_entry = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, sets.size(), &sets[0]);
	gb_sizer->Add(new wxStaticText(panel, -1, "Icons:"), wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTRE_VERTICAL);
	gb_sizer->Add(choice_iconset_entry, wxGBPosition(row, 1), wxDefaultSpan, wxEXPAND);
	gb_sizer->Add(new wxStaticText(panel, -1, "*"), wxGBPosition(row++, 2), wxDefaultSpan, wxALIGN_CENTRE_VERTICAL);

	gb_sizer->AddGrowableCol(1, 1);
	sizer->Add(new wxStaticText(panel, -1, "* requires restart to take effect"), 0, wxALL | wxALIGN_RIGHT, 8);

	

	//// Tab style
	//string styles[] ={ "Flat (Default)", "Glossy", "Rounded" };
	//choice_tab_style = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, 3, styles);
	//hbox = new wxBoxSizer(wxHORIZONTAL);
	//sizer->Add(hbox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);
	//hbox->Add(new wxStaticText(this, -1, "Tab style:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
	//hbox->Add(choice_tab_style, 0, wxEXPAND|wxRIGHT, 4);
	//sizer->Add(new wxStaticText(this, -1, "You need to quit and restart SLADE 3 for tab style changes to take effect."), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
}

/* InterfacePrefsPanel::~InterfacePrefsPanel
 * InterfacePrefsPanel class destructor
 *******************************************************************/
InterfacePrefsPanel::~InterfacePrefsPanel()
{
}

/* InterfacePrefsPanel::init
 * Initialises panel controls
 *******************************************************************/
void InterfacePrefsPanel::init()
{
	cb_size_as_string->SetValue(size_as_string);
	cb_filter_dirs->SetValue(!elist_filter_dirs);
	cb_list_monospace->SetValue(list_font_monospace);
	cb_start_page->SetValue(show_start_page);
	cb_context_submenus->SetValue(context_submenus);
	cb_elist_bgcol->SetValue(elist_type_bgcol);
	cb_file_browser->SetValue(am_file_browser_tab);
	cb_condensed_tabs->SetValue(tabs_condensed);

	if (toolbar_size <= 16)
		choice_toolbar_size->Select(0);
	else if (toolbar_size <= 24)
		choice_toolbar_size->Select(1);
	else
		choice_toolbar_size->Select(2);

	choice_iconset_general->SetSelection(0);
	for (unsigned a = 0; a < choice_iconset_general->GetCount(); a++)
		if (choice_iconset_general->GetString(a) == iconset_general)
		{
			choice_iconset_general->SetSelection(a);
			break;
		}

	choice_iconset_entry->SetSelection(0);
	for (unsigned a = 0; a < choice_iconset_entry->GetCount(); a++)
		if (choice_iconset_entry->GetString(a) == iconset_entry_list)
		{
			choice_iconset_entry->SetSelection(a);
			break;
		}

	//if (tab_style < 0)
	//	tab_style = 0;
	//else if (tab_style > 2)
	//	tab_style = 2;
}

/* InterfacePrefsPanel::applyPreferences
 * Applies preference values from the controls to CVARs
 *******************************************************************/
void InterfacePrefsPanel::applyPreferences()
{
	size_as_string = cb_size_as_string->GetValue();
	elist_filter_dirs = !cb_filter_dirs->GetValue();
	list_font_monospace = cb_list_monospace->GetValue();
	show_start_page = cb_start_page->GetValue();
	context_submenus = cb_context_submenus->GetValue();
	elist_type_bgcol = cb_elist_bgcol->GetValue();
	am_file_browser_tab = cb_file_browser->GetValue();
	tabs_condensed = cb_condensed_tabs->GetValue();

	if (choice_toolbar_size->GetSelection() == 0)
		toolbar_size = 16;
	else if (choice_toolbar_size->GetSelection() == 1)
		toolbar_size = 24;
	else
		toolbar_size = 32;

	iconset_general = choice_iconset_general->GetString(choice_iconset_general->GetSelection());
	iconset_entry_list = choice_iconset_entry->GetString(choice_iconset_entry->GetSelection());

	//tab_style = choice_tab_style->GetSelection();
}
