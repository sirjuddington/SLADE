
#ifndef __UNDO_MANAGER_HISTORY_PANEL_H__
#define __UNDO_MANAGER_HISTORY_PANEL_H__

#include "VirtualListView.h"
#include "ListenerAnnouncer.h"

class UndoManager;
class UndoListView : public VirtualListView, public Listener {
private:
	UndoManager*	manager;

protected:
	// Virtual wxListCtrl overrides
	string	getItemText(long item, long column) const;
	int		getItemIcon(long item) const;
	void	updateItemAttr(long item) const;

public:
	UndoListView(wxWindow* parent, UndoManager* manager);
	~UndoListView();

	void	setManager(UndoManager* manager);
	void	onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data);
};

class UndoManagerHistoryPanel : public wxPanel {
private:
	UndoManager*	manager;
	UndoListView*	list_levels;

public:
	UndoManagerHistoryPanel(wxWindow* parent, UndoManager* manager);
	~UndoManagerHistoryPanel();

	void	setManager(UndoManager* manager);
	void	populateList();
	void	updateList();
	
	// Events
	void	onItemRightClick(wxCommandEvent& e);
};

#endif//__UNDO_MANAGER_HISTORY_PANEL_H__
