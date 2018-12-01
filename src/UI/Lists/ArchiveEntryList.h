#pragma once

#include "Archive/Archive.h"
#include "General/ListenerAnnouncer.h"
#include "General/SAction.h"
#include "VirtualListView.h"

wxDECLARE_EVENT(EVT_AEL_DIR_CHANGED, wxCommandEvent);

class UndoManager;
class ArchiveEntryList : public VirtualListView, public Listener, public SActionHandler
{
public:
	ArchiveEntryList(wxWindow* parent);
	~ArchiveEntryList();

	ArchiveTreeNode* currentDir() const { return current_dir_; }

	bool showDirBack() const { return show_dir_back_; }
	void showDirBack(bool db) { show_dir_back_ = db; }

	void setArchive(Archive* archive);
	void setUndoManager(UndoManager* manager) { undo_manager_ = manager; }

	void setupColumns();
	int  columnType(int column) const;
	void updateList(bool clear = false) override;
	int  entriesBegin();

	void filterList(string filter = "", string category = "");
	void applyFilter() override;
	bool goUpDir();
	bool setDir(ArchiveTreeNode* dir);

	void setEntriesAutoUpdate(bool update) { entries_update_ = update; }

	// Sorting
	void sortItems() override;

	ArchiveEntry*            entryAt(int index, bool filtered = true) const;
	int                      entryIndexAt(int index, bool filtered = true);
	ArchiveEntry*            focusedEntry();
	vector<ArchiveEntry*>    selectedEntries();
	ArchiveEntry*            lastSelectedEntry();
	vector<ArchiveTreeNode*> selectedDirectories();

	// Label editing
	void labelEdited(int col, int index, string new_label) override;

	void onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data) override;

	// SAction handler
	bool handleAction(string id) override;

	// Events
	void onColumnHeaderRightClick(wxListEvent& e);
	void onColumnResize(wxListEvent& e);
	void onListItemActivated(wxListEvent& e);

protected:
	// Virtual wxListCtrl overrides
	string itemText(long item, long column, long index) const override;
	int    itemIcon(long item, long column, long index) const override;
	void   updateItemAttr(long item, long column, long index) const override;

private:
	Archive*         archive_;
	string           filter_category_;
	ArchiveTreeNode* current_dir_;
	ArchiveEntry*    entry_dir_back_;
	bool             show_dir_back_;
	UndoManager*     undo_manager_;
	int              col_index_;
	int              col_name_;
	int              col_size_;
	int              col_type_;
	bool             entries_update_;

	int entrySize(long index);
};
