
#ifndef __ARCHIVE_ENTRY_LIST_H__
#define __ARCHIVE_ENTRY_LIST_H__

#include "VirtualListView.h"
#include "ListenerAnnouncer.h"
#include "Archive.h"
#include "MainApp.h"
#include <wx/listctrl.h>
#include <wx/textctrl.h>

wxDECLARE_EVENT(EVT_AEL_DIR_CHANGED, wxCommandEvent);

class UndoManager;
class ArchiveEntryList : public VirtualListView, public Listener, public SActionHandler
{
private:
	Archive*			archive;
	string				filter_category;
	ArchiveTreeNode*	current_dir;
	ArchiveEntry*		entry_dir_back;
	bool				show_dir_back;
	UndoManager*		undo_manager;
	int					col_index;
	int					col_name;
	int					col_size;
	int					col_type;

protected:
	// Virtual wxListCtrl overrides
	string	getItemText(long item, long column, long index) const;
	int		getItemIcon(long item, long column, long index) const;
	void	updateItemAttr(long item, long column, long index) const;

public:
	ArchiveEntryList(wxWindow* parent);
	~ArchiveEntryList();

	ArchiveTreeNode*	getCurrentDir() { return current_dir; }

	bool	showDirBack() { return show_dir_back; }
	void	showDirBack(bool db) { show_dir_back = db; }

	void	setArchive(Archive* archive);
	void	setUndoManager(UndoManager* manager) { undo_manager = manager; }

	void	setupColumns();
	int		columnType(int column) const;
	void	updateList(bool clear = false);
	int		entriesBegin();

	void	filterList(string filter = "", string category = "");
	void	applyFilter();
	bool	goUpDir();
	bool	setDir(ArchiveTreeNode* dir);

	// Sorting
	bool	sortSize(long left, long right);
	void	sortItems();

	ArchiveEntry*				getEntry(int index, bool filtered = true) const;
	int							getEntryIndex(int index, bool filtered = true);
	ArchiveEntry*				getFocusedEntry();
	vector<ArchiveEntry*>		getSelectedEntries();
	ArchiveEntry*				getLastSelectedEntry();
	vector<ArchiveTreeNode*>	getSelectedDirectories();

	// Label editing
	void	labelEdited(int col, int index, string new_label);

	void	onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data);

	// SAction handler
	bool	handleAction(string id);

	// Events
	void	onColumnHeaderRightClick(wxListEvent& e);
	void	onColumnResize(wxListEvent& e);
	void	onListItemActivated(wxListEvent& e);
};

#endif//__ARCHIVE_ENTRY_LIST_H__
