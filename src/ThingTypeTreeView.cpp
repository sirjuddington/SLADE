
#include "Main.h"
#include "WxStuff.h"
#include "ThingTypeTreeView.h"
#include "GameConfiguration.h"

ThingTypeTreeView::ThingTypeTreeView(wxWindow* parent) : wxDataViewTreeCtrl(parent, -1)
{
	parent_dialog = NULL;

	// Create root item
	root = AppendContainer(wxDataViewItem(0), "Thing Types", -1, 1);

	// Populate tree
	vector<tt_t> specials = theGameConfiguration->allThingTypes();
	for (unsigned a = 0; a < specials.size(); a++)
	{
		AppendItem(getGroup(specials[a].type->getGroup()),
		           S_FMT("%d: %s", specials[a].number, specials[a].type->getName()), -1);
	}

	// Bind events
	Bind(wxEVT_DATAVIEW_ITEM_START_EDITING, &ThingTypeTreeView::onItemEdit, this);
	Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &ThingTypeTreeView::onItemActivated, this);

	Expand(root);
}

ThingTypeTreeView::~ThingTypeTreeView()
{
}

int ThingTypeTreeView::typeNumber(wxDataViewItem item)
{
	string num = GetItemText(item).BeforeFirst(':');
	long s;
	num.ToLong(&s);

	return s;
}

void ThingTypeTreeView::showType(int type)
{
	// Go through item groups
	for (unsigned a = 0; a < groups.size(); a++)
	{
		// Go through group items
		for (int b = 0; b < GetChildCount(groups[a].item); b++)
		{
			wxDataViewItem item = GetNthChild(groups[a].item, b);

			// Select+show if match
			if (typeNumber(item) == type)
			{
				Select(item);
				EnsureVisible(item);
				return;
			}
		}
	}
}

int ThingTypeTreeView::selectedType()
{
	wxDataViewItem item = GetSelection();
	if (item.IsOk())
		return typeNumber(item);
	else
		return -1;
}

wxDataViewItem ThingTypeTreeView::getGroup(string group)
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
			groups.push_back(group_t(current, fullpath));
		}
	}

	return current;
}


void ThingTypeTreeView::onItemEdit(wxDataViewEvent& e)
{
	e.Veto();
}

void ThingTypeTreeView::onItemActivated(wxDataViewEvent& e)
{
	if (parent_dialog)
		parent_dialog->EndModal(wxID_OK);
}





int ThingTypeTreeView::showDialog(wxWindow* parent, int init)
{
	wxDialog dlg(parent, -1, "Thing Type", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	dlg.SetSizer(sizer);

	ThingTypeTreeView* tttv = new ThingTypeTreeView(&dlg);
	if (init >= 0) tttv->showType(init);
	tttv->setParentDialog(&dlg);
	sizer->Add(tttv, 1, wxEXPAND|wxALL, 4);
	sizer->Add(dlg.CreateButtonSizer(wxOK|wxCANCEL), 0, wxEXPAND|wxTOP|wxBOTTOM, 4);

	dlg.SetSize(400, 500);
	dlg.CenterOnScreen();
	if (dlg.ShowModal() == wxID_OK)
		return tttv->selectedType();
	else
		return -1;
}
