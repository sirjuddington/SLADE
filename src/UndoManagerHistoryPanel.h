
#ifndef __UNDO_MANAGER_HISTORY_PANEL_H__
#define __UNDO_MANAGER_HISTORY_PANEL_H__

#include "ListView.h"
#include "ListenerAnnouncer.h"

class UndoManager;
class UndoManagerHistoryPanel : public wxPanel, public Listener {
private:
	UndoManager*	manager;
	ListView*		list_levels;

public:
	UndoManagerHistoryPanel(wxWindow* parent, UndoManager* manager);
	~UndoManagerHistoryPanel();

	void	setManager(UndoManager* manager);
	void	populateList();
	void	updateList();
	void	onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data);

	// Events
	void	onItemRightClick(wxCommandEvent& e);
};

#endif//__UNDO_MANAGER_HISTORY_PANEL_H__
