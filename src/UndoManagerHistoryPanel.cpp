
#include "Main.h"
#include "WxStuff.h"
#include "UndoManagerHistoryPanel.h"
#include "UndoRedo.h"

UndoManagerHistoryPanel::UndoManagerHistoryPanel(wxWindow* parent, UndoManager* manager) : wxPanel(parent, -1) {
	// Init variables
	this->manager = manager;

	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Add undo levels list
	list_levels = new ListView(this, -1);
	sizer->Add(list_levels, 1, wxEXPAND|wxALL, 4);

	list_levels->AppendColumn("Action", 0, 160);

	list_levels->Bind(wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK, &UndoManagerHistoryPanel::onItemRightClick, this); 
}

UndoManagerHistoryPanel::~UndoManagerHistoryPanel() {
}

void UndoManagerHistoryPanel::setManager(UndoManager* manager) {
	if (this->manager)
		stopListening(this->manager);

	this->manager = manager;
	listenTo(manager);
}

void UndoManagerHistoryPanel::populateList() {
	vector<string> levels;
	manager->getAllLevels(levels);
	int index = manager->getCurrentIndex();

	list_levels->ClearAll();
	list_levels->AppendColumn("Action");
	for (unsigned a = 0; a < levels.size(); a++) {
	//for (int a = levels.size() - 1; a >= 0; a--) {
		list_levels->addItem(0, levels[a]);
		if (a > index)
			list_levels->setItemStatus(a, LV_STATUS_DISABLED);
		else if (a == index)
			list_levels->setItemStatus(a, LV_STATUS_NEW);
	}
}

void UndoManagerHistoryPanel::updateList() {
	int index = manager->getCurrentIndex();
	for (unsigned a = 0; a < list_levels->GetItemCount(); a++) {
		if ((int)a < index)
			list_levels->setItemStatus(a, LV_STATUS_DISABLED);
		else if ((int)a == index)
			list_levels->setItemStatus(a, LV_STATUS_NEW);
		else
			list_levels->setItemStatus(a, LV_STATUS_NORMAL);
	}
}

void UndoManagerHistoryPanel::onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data) {
	if (announcer != manager)
		return;

	if (event_name == "level_recorded")
		populateList();
	else if (event_name == "undo" || event_name == "redo")
		updateList();
}

void UndoManagerHistoryPanel::onItemRightClick(wxCommandEvent& e) {
	int index = e.GetInt();
	wxMessageBox(S_FMT("Item %d", e.GetInt()));

	if (index == manager->getCurrentIndex()) {
	}
	else if (index < manager->getCurrentIndex()) {
	}
	else {
	}
}
