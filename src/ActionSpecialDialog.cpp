
#include "Main.h"
#include "WxStuff.h"
#include "ActionSpecialDialog.h"
#include "GameConfiguration.h"
#include "GenLineSpecialPanel.h"

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

ActionSpecialTreeView::~ActionSpecialTreeView()
{
}

int ActionSpecialTreeView::specialNumber(wxDataViewItem item)
{
	string num = GetItemText(item).BeforeFirst(':');
	long s;
	num.ToLong(&s);

	return s;
}

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

int ActionSpecialTreeView::selectedSpecial()
{
	wxDataViewItem item = GetSelection();
	if (item.IsOk())
		return specialNumber(item);
	else
		return -1;
}

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


void ActionSpecialTreeView::onItemEdit(wxDataViewEvent& e)
{
	e.Veto();
}

void ActionSpecialTreeView::onItemActivated(wxDataViewEvent& e)
{
	if (parent_dialog)
		parent_dialog->EndModal(wxID_OK);
}



ActionSpecialDialog::ActionSpecialDialog(wxWindow* parent)
	: wxDialog(parent, -1, "Select Action Special", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
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
	SetInitialSize(wxSize(400, 500));
	CenterOnParent();
}

ActionSpecialDialog::~ActionSpecialDialog()
{
}

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
