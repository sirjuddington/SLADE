
#include "Main.h"
#include "WxStuff.h"
#include "ActionSpecialTreeView.h"
#include "GameConfiguration.h"

ActionSpecialTreeView::ActionSpecialTreeView(wxWindow* parent) : wxDataViewTreeCtrl(parent, -1) {
	parent_dialog = NULL;

	// Create root item
	root = wxDataViewItem(0);

	// Add 'None'
	AppendItem(root, "0: None");

	// Populate tree
	vector<as_t> specials = theGameConfiguration->allActionSpecials();
	std::sort(specials.begin(), specials.end());
	for (unsigned a = 0; a < specials.size(); a++) {
		AppendItem(getGroup(specials[a].special->getGroup()),
					S_FMT("%d: %s", specials[a].number, CHR(specials[a].special->getName())), -1);
	}

	// Bind events
	Bind(wxEVT_COMMAND_DATAVIEW_ITEM_START_EDITING, &ActionSpecialTreeView::onItemEdit, this);
	Bind(wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, &ActionSpecialTreeView::onItemActivated, this);

	Expand(root);
}

ActionSpecialTreeView::~ActionSpecialTreeView() {
}

int ActionSpecialTreeView::specialNumber(wxDataViewItem item) {
	string num = GetItemText(item).BeforeFirst(':');
	long s;
	num.ToLong(&s);

	return s;
}

void ActionSpecialTreeView::showSpecial(int special) {
	// Go through item groups
	for (unsigned a = 0; a < groups.size(); a++) {
		// Go through group items
		for (int b = 0; b < GetChildCount(groups[a].item); b++) {
			wxDataViewItem item = GetNthChild(groups[a].item, b);

			// Select+show if match
			if (specialNumber(item) == special) {
				EnsureVisible(item);
				Select(item);
				SetFocus();
				return;
			}
		}
	}
}

int ActionSpecialTreeView::selectedSpecial() {
	wxDataViewItem item = GetSelection();
	if (item.IsOk())
		return specialNumber(item);
	else
		return -1;
}

wxDataViewItem ActionSpecialTreeView::getGroup(string group) {
	// Check if group was already made
	for (unsigned a = 0; a < groups.size(); a++) {
		if (group == groups[a].name)
			return groups[a].item;
	}

	// Split group into subgroups
	wxArrayString path = wxSplit(group, '/');

	// Create group needed
	wxDataViewItem current = root;
	string fullpath = "";
	for (unsigned p = 0; p < path.size(); p++) {
		if (p > 0) fullpath += "/";
		fullpath += path[p];

		bool found = false;
		for (unsigned a = 0; a < groups.size(); a++) {
			if (groups[a].name == fullpath) {
				current = groups[a].item;
				found = true;
				break;
			}
		}

		if (!found) {
			current = AppendContainer(current, path[p], -1, 1);
			groups.push_back(astv_group_t(current, fullpath));
		}
	}

	return current;
}


void ActionSpecialTreeView::onItemEdit(wxDataViewEvent& e) {
	e.Veto();
}

void ActionSpecialTreeView::onItemActivated(wxDataViewEvent& e) {
	if (parent_dialog)
		parent_dialog->EndModal(wxID_OK);
}





int ActionSpecialTreeView::showDialog(wxWindow* parent, int init) {
	wxDialog dlg(parent, -1, "Action Special", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	dlg.SetSizer(sizer);

	ActionSpecialTreeView* astv = new ActionSpecialTreeView(&dlg);
	astv->setParentDialog(&dlg);
	sizer->Add(astv, 1, wxEXPAND|wxALL, 4);
	sizer->Add(dlg.CreateButtonSizer(wxOK|wxCANCEL), 0, wxEXPAND|wxTOP|wxBOTTOM, 4);
	if (init >= 0) astv->showSpecial(init);

	dlg.SetSize(400, 500);
	dlg.CenterOnScreen();
	if (dlg.ShowModal() == wxID_OK)
		return astv->selectedSpecial();
	else
		return -1;
}
