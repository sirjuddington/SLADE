
#ifndef __UNDO_MANAGER_HISTORY_PANEL_H__
#define __UNDO_MANAGER_HISTORY_PANEL_H__

#include "common.h"
#include "UI/Lists/VirtualListView.h"
#include "General/ListenerAnnouncer.h"

class UndoManager;
class UndoListView : public VirtualListView, public Listener
{
private:
	UndoManager*	manager;

	void	updateFromManager();

protected:
	// Virtual wxListCtrl overrides
	string	getItemText(long item, long column, long index) const;
	int		getItemIcon(long item, long column, long index) const;
	void	updateItemAttr(long item, long column, long index) const;

public:
	UndoListView(wxWindow* parent, UndoManager* manager);
	~UndoListView();

	void	setManager(UndoManager* manager);
	void	onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data);
};

class UndoManagerHistoryPanel : public wxPanel
{
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
	void	onMenu(wxCommandEvent& e);
};

#endif//__UNDO_MANAGER_HISTORY_PANEL_H__
