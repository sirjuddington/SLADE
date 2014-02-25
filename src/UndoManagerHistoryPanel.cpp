
#include "Main.h"
#include "WxStuff.h"
#include "UndoManagerHistoryPanel.h"
#include "UndoRedo.h"


UndoListView::UndoListView(wxWindow* parent, UndoManager* manager) : VirtualListView(parent)
{
	this->manager = manager;

	if (manager)
	{
		SetItemCount(manager->nUndoLevels());
		listenTo(manager);
	}
}

UndoListView::~UndoListView()
{
}

string UndoListView::getItemText(long item, long column) const
{
	if (!manager)
		return "";

	int max = manager->nUndoLevels();
	if (item < max)
	{
		int index = max - item - 1;
		if (column == 0)
		{
			string name = manager->undoLevel(index)->getName();
			return S_FMT("%d. %s", index + 1, name);
		}
		else
		{
			return manager->undoLevel(index)->getTimeStamp(false, true);
		}
	}
	else
		return "Invalid Index";
}

int UndoListView::getItemIcon(long item) const
{
	return -1;
}

void UndoListView::updateItemAttr(long item) const
{
	if (!manager)
		return;

	item_attr->SetTextColour(wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT));

	int index = manager->nUndoLevels() - item - 1;
	if (index == manager->getCurrentIndex())
		item_attr->SetTextColour(WXCOL(rgba_t(0, 170, 0)));
	else if (index > manager->getCurrentIndex())
		item_attr->SetTextColour(WXCOL(rgba_t(150, 150, 150)));
}

void UndoListView::setManager(UndoManager* manager)
{
	if (this->manager)
		stopListening(this->manager);

	this->manager = manager;
	listenTo(manager);

	SetItemCount(manager->nUndoLevels());
	Refresh();
}

void UndoListView::onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data)
{
	if (announcer != manager)
		return;

	SetItemCount(manager->nUndoLevels());
	Refresh();
}



UndoManagerHistoryPanel::UndoManagerHistoryPanel(wxWindow* parent, UndoManager* manager) : wxPanel(parent, -1)
{
	// Init variables
	this->manager = manager;

	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Add undo levels list
	list_levels = new UndoListView(this, manager);
	sizer->Add(list_levels, 1, wxEXPAND|wxALL, 4);

	list_levels->AppendColumn("Action", wxLIST_FORMAT_LEFT, 160);
	list_levels->AppendColumn("Time", wxLIST_FORMAT_RIGHT);
	list_levels->Bind(wxEVT_LIST_ITEM_RIGHT_CLICK, &UndoManagerHistoryPanel::onItemRightClick, this);
}

UndoManagerHistoryPanel::~UndoManagerHistoryPanel()
{
}

void UndoManagerHistoryPanel::setManager(UndoManager* manager)
{
	this->manager = manager;
	list_levels->setManager(manager);
}

/*
void UndoManagerHistoryPanel::populateList() {
	vector<string> levels;
	manager->getAllLevels(levels);
	int index = manager->getCurrentIndex();

	list_levels->ClearAll();
	list_levels->AppendColumn("Action");
	for (unsigned a = 0; a < levels.size(); a++) {
	//for (int a = levels.size() - 1; a >= 0; a--) {
		list_levels->addItem(0, levels[a]);
		if (a > (unsigned)index)
			list_levels->setItemStatus(a, LV_STATUS_DISABLED);
		else if (a == index)
			list_levels->setItemStatus(a, LV_STATUS_NEW);
	}
}

void UndoManagerHistoryPanel::updateList() {
	int index = manager->getCurrentIndex();
	for (int a = 0; a < list_levels->GetItemCount(); a++) {
		if (a < index)
			list_levels->setItemStatus(a, LV_STATUS_DISABLED);
		else if (a == index)
			list_levels->setItemStatus(a, LV_STATUS_NEW);
		else
			list_levels->setItemStatus(a, LV_STATUS_NORMAL);
	}
}
*/

void UndoManagerHistoryPanel::onItemRightClick(wxCommandEvent& e)
{
	int index = e.GetInt();
	wxMessageBox(S_FMT("Item %d", e.GetInt()));

	if (index == manager->getCurrentIndex())
	{
	}
	else if (index < manager->getCurrentIndex())
	{
	}
	else
	{
	}
}
