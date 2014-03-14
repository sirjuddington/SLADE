
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ActionSpecialDialog.cpp
 * Description: A dialog that allows selection of an action special
 *              (and other related classes)
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
#include "WxStuff.h"
#include "ActionSpecialDialog.h"
#include "GameConfiguration.h"
#include "GenLineSpecialPanel.h"


/*******************************************************************
 * ACTIONSPECIALTREEVIEW CLASS FUNCTIONS
 *******************************************************************/

/* ActionSpecialTreeView::ActionSpecialTreeView
 * ActionSpecialTreeView class constructor
 *******************************************************************/
ActionSpecialTreeView::ActionSpecialTreeView(wxWindow* parent) : wxDataViewTreeCtrl(parent, -1)
{
	parent_dialog = NULL;

	// Create root item
	root = wxDataViewItem(0);

	// Add 'None'
	AppendItem(root, "0: None");

	// Populate tree
	vector<as_t> specials = theGameConfiguration->allActionSpecials();
	std::sort(specials.begin(), specials.end());
	for (unsigned a = 0; a < specials.size(); a++)
	{
		AppendItem(getGroup(specials[a].special->getGroup()),
		           S_FMT("%d: %s", specials[a].number, specials[a].special->getName()), -1);
	}

	// Bind events
	Bind(wxEVT_DATAVIEW_ITEM_START_EDITING, &ActionSpecialTreeView::onItemEdit, this);
	Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &ActionSpecialTreeView::onItemActivated, this);

	Expand(root);
}

/* ActionSpecialTreeView::~ActionSpecialTreeView
 * ActionSpecialTreeView class destructor
 *******************************************************************/
ActionSpecialTreeView::~ActionSpecialTreeView()
{
}

/* ActionSpecialTreeView::specialNumber
 * Returns the action special value for [item]
 *******************************************************************/
int ActionSpecialTreeView::specialNumber(wxDataViewItem item)
{
	string num = GetItemText(item).BeforeFirst(':');
	long s;
	num.ToLong(&s);

	return s;
}

/* ActionSpecialTreeView::showSpecial
 * Finds the item for [special], selects it and ensures it is shown
 *******************************************************************/
void ActionSpecialTreeView::showSpecial(int special)
{
	// Go through item groups
	for (unsigned a = 0; a < groups.size(); a++)
	{
		// Go through group items
		for (int b = 0; b < GetChildCount(groups[a].item); b++)
		{
			wxDataViewItem item = GetNthChild(groups[a].item, b);

			// Select+show if match
			if (specialNumber(item) == special)
			{
				EnsureVisible(item);
				Select(item);
				SetFocus();
				return;
			}
		}
	}
}

/* ActionSpecialTreeView::selectedSpecial
 * Returns the currently selected action special value
 *******************************************************************/
int ActionSpecialTreeView::selectedSpecial()
{
	wxDataViewItem item = GetSelection();
	if (item.IsOk())
		return specialNumber(item);
	else
		return -1;
}

/* ActionSpecialTreeView::getGroup
 * Returns the parent wxDataViewItem representing action special
 * group [group]
 *******************************************************************/
wxDataViewItem ActionSpecialTreeView::getGroup(string group)
{
	// Check if group was already made
	for (unsigned a = 0; a < groups.size(); a++)
	{
		if (group == groups[a].name)
			return groups[a].item;
	}

	// Split group into subgroups
	wxArrayString path = wxSplit(group, '/');

	// Create group needed
	wxDataViewItem current = root;
	string fullpath = "";
	for (unsigned p = 0; p < path.size(); p++)
	{
		if (p > 0) fullpath += "/";
		fullpath += path[p];

		bool found = false;
		for (unsigned a = 0; a < groups.size(); a++)
		{
			if (groups[a].name == fullpath)
			{
				current = groups[a].item;
				found = true;
				break;
			}
		}

		if (!found)
		{
			current = AppendContainer(current, path[p], -1, 1);
			groups.push_back(astv_group_t(current, fullpath));
		}
	}

	return current;
}


/*******************************************************************
 * ACTIONSPECIALTREEVIEW CLASS EVENTS
 *******************************************************************/

/* ActionSpecialTreeView::onItemEdit
 * Called when a tree item label is edited
 *******************************************************************/
void ActionSpecialTreeView::onItemEdit(wxDataViewEvent& e)
{
	e.Veto();
}

/* ActionSpecialTreeView::onItemActivated
 * Called when a tree item is activated
 *******************************************************************/
void ActionSpecialTreeView::onItemActivated(wxDataViewEvent& e)
{
	if (parent_dialog)
		parent_dialog->EndModal(wxID_OK);
}


/*******************************************************************
 * ACTIONSPECIALDIALOG CLASS FUNCTIONS
 *******************************************************************/

/* ActionSpecialDialog::ActionSpecialDialog
 * ActionSpecialDialog class constructor
 *******************************************************************/
ActionSpecialDialog::ActionSpecialDialog(wxWindow* parent)
	: SDialog(parent, "Select Action Special", "actionspecial", 400, 500)
{
	// Setup layout
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Boom (tabbed w/generalised tab)
	if (theGameConfiguration->isBoom())
	{
		// Notebook (tabs)
		nb_tabs = new wxNotebook(this, -1);
		sizer->Add(nb_tabs, 1, wxEXPAND|wxALL, 4);

		// Action special tree page
		tree_specials = new ActionSpecialTreeView(nb_tabs);
		nb_tabs->AddPage(tree_specials, "Action Special", true);

		// Boom generalised line special page
		panel_gen_specials = new GenLineSpecialPanel(nb_tabs);
		nb_tabs->AddPage(panel_gen_specials, "Generalised Special");
	}

	// Non-Boom (no generalised tab)
	else
	{
		tree_specials = new ActionSpecialTreeView(this);
		sizer->Add(tree_specials, 1, wxEXPAND|wxALL, 4);
	}

	// Add buttons
	sizer->Add(CreateButtonSizer(wxOK|wxCANCEL), 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Init
	SetMinClientSize(sizer->GetMinSize());
	CenterOnParent();
}

/* ActionSpecialDialog::~ActionSpecialDialog
 * ActionSpecialDialog class destructor
 *******************************************************************/
ActionSpecialDialog::~ActionSpecialDialog()
{
}

/* ActionSpecialDialog::setSpecial
 * Selects the item for special [special] in the specials tree
 *******************************************************************/
void ActionSpecialDialog::setSpecial(int special)
{
	// Check for boom generalised special
	if (theGameConfiguration->isBoom())
	{
		if (panel_gen_specials->loadSpecial(special))
		{
			nb_tabs->SetSelection(1);
			panel_gen_specials->SetFocus();
			return;
		}
	}

	// Regular action special
	tree_specials->showSpecial(special);
	tree_specials->SetFocus();
	tree_specials->SetFocusFromKbd();
}

/* ActionSpecialDialog::selectedSpecial
 * Returns the currently selected action special
 *******************************************************************/
int ActionSpecialDialog::selectedSpecial()
{
	if (theGameConfiguration->isBoom())
	{
		if (nb_tabs->GetSelection() == 0)
			return tree_specials->selectedSpecial();
		else
			return panel_gen_specials->getSpecial();
	}
	else
		return tree_specials->selectedSpecial();
}
