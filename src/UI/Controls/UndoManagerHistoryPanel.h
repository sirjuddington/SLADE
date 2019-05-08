#pragma once

#include "General/ListenerAnnouncer.h"
#include "UI/Lists/VirtualListView.h"

class UndoManager;

class UndoListView : public VirtualListView, public Listener
{
public:
	UndoListView(wxWindow* parent, UndoManager* manager);
	~UndoListView() {}

	void setManager(UndoManager* manager);
	void onAnnouncement(Announcer* announcer, string_view event_name, MemChunk& event_data) override;

protected:
	// Virtual wxListCtrl overrides
	wxString itemText(long item, long column, long index) const override;
	int      itemIcon(long item, long column, long index) const override;
	void     updateItemAttr(long item, long column, long index) const override;

private:
	UndoManager* manager_ = nullptr;

	void updateFromManager();
};

class UndoManagerHistoryPanel : public wxPanel
{
public:
	UndoManagerHistoryPanel(wxWindow* parent, UndoManager* manager);
	~UndoManagerHistoryPanel() {}

	void setManager(UndoManager* manager);

private:
	UndoManager*  manager_     = nullptr;
	UndoListView* list_levels_ = nullptr;

	// Events
	void onItemRightClick(wxCommandEvent& e);
	void onMenu(wxCommandEvent& e);
};
