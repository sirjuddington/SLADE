#pragma once

#include "VirtualListView.h"
#include "General/ListenerAnnouncer.h"
#include "General/SAction.h"
#include "Archive/Archive.h"

wxDECLARE_EVENT(EVT_AEL_DIR_CHANGED, wxCommandEvent);

class UndoManager;
class ArchiveEntryList : public VirtualListView, public Listener, public SActionHandler
{
public:
	ArchiveEntryList(wxWindow* parent);
	~ArchiveEntryList();

	ArchiveTreeNode*	getCurrentDir() const { return current_dir; }

	bool	showDirBack() const { return show_dir_back; }
	void	showDirBack(bool db) { show_dir_back = db; }

	void	setArchive(Archive* archive);
	void	setUndoManager(UndoManager* manager) { undo_manager = manager; }

	void	setupColumns();
	int		columnType(int column) const;
	void	updateList(bool clear = false) override;
	int		entriesBegin();

	void	filterList(string filter = "", string category = "");
	void	applyFilter() override;
	bool	goUpDir();
	bool	setDir(ArchiveTreeNode* dir);

	void	setEntriesAutoUpdate(bool update) { entries_update = update; }

	// Sorting
	void	sortItems() override;

	ArchiveEntry*				getEntry(int index, bool filtered = true) const;
	int							getEntryIndex(int index, bool filtered = true);
	ArchiveEntry*				getFocusedEntry();
	vector<ArchiveEntry*>		getSelectedEntries();
	ArchiveEntry*				getLastSelectedEntry();
	vector<ArchiveTreeNode*>	getSelectedDirectories();

	// Label editing
	void	labelEdited(int col, int index, string new_label) override;

	void	onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data) override;

	// SAction handler
	bool	handleAction(string id) override;

	// Events
	void	onColumnHeaderRightClick(wxListEvent& e);
	void	onColumnResize(wxListEvent& e);
	void	onListItemActivated(wxListEvent& e);

protected:
	// Virtual wxListCtrl overrides
	string	getItemText(long item, long column, long index) const override;
	int		getItemIcon(long item, long column, long index) const override;
	void	updateItemAttr(long item, long column, long index) const override;

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
	bool				entries_update;

	int	entrySize(long index);
};
