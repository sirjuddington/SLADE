#pragma once

#include "Archive/Archive.h"
#include "General/SAction.h"
#include "VirtualListView.h"

wxDECLARE_EVENT(EVT_AEL_DIR_CHANGED, wxCommandEvent);

class UndoManager;
class ArchiveEntryList : public VirtualListView, public SActionHandler
{
public:
	ArchiveEntryList(wxWindow* parent);
	~ArchiveEntryList() = default;

	const weak_ptr<ArchiveDir>& currentDir() const { return current_dir_; }

	bool showDirBack() const { return show_dir_back_; }
	void showDirBack(bool db) { show_dir_back_ = db; }

	void setArchive(const shared_ptr<Archive>& archive);
	void setUndoManager(UndoManager* manager) { undo_manager_ = manager; }

	void setupColumns();
	int  columnType(int column) const;
	void updateList(bool clear = false) override;
	int  entriesBegin() const;

	void filterList(const wxString& filter = "", const wxString& category = "");
	void applyFilter() override;
	bool goUpDir();
	bool setDir(const shared_ptr<ArchiveDir>& dir);

	void setEntriesAutoUpdate(bool update) { entries_update_ = update; }

	// Sorting
	void sortItems() override;

	ArchiveEntry*         entryAt(int index, bool filtered = true) const;
	int                   entryIndexAt(int index, bool filtered = true);
	ArchiveEntry*         focusedEntry() const;
	vector<ArchiveEntry*> selectedEntries() const;
	ArchiveEntry*         lastSelectedEntry() const;
	vector<ArchiveDir*>   selectedDirectories() const;

	// Label editing
	void labelEdited(int col, int index, const wxString& new_label) override;

	// SAction handler
	bool handleAction(string_view id) override;

	// Events
	void onColumnHeaderRightClick(wxListEvent& e);
	void onColumnResize(wxListEvent& e);
	void onListItemActivated(wxListEvent& e);

protected:
	// Virtual wxListCtrl overrides
	wxString itemText(long item, long column, long index) const override;
	int      itemIcon(long item, long column, long index) const override;
	void     updateItemAttr(long item, long column, long index) const override;

private:
	weak_ptr<Archive>        archive_;
	wxString                 filter_category_;
	weak_ptr<ArchiveDir>     current_dir_;
	unique_ptr<ArchiveEntry> entry_dir_back_;
	bool                     show_dir_back_  = false;
	UndoManager*             undo_manager_   = nullptr;
	int                      col_index_      = 0;
	int                      col_name_       = 0;
	int                      col_size_       = 0;
	int                      col_type_       = 0;
	bool                     entries_update_ = true;

	// Signal connections
	sigslot::scoped_connection sc_archive_modified_;

	int entrySize(long index) const;
};
