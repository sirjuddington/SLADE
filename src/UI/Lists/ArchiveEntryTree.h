#pragma once

#include "General/Sigslot.h"
#include <wx/dataview.h>

namespace slade
{
class Archive;
class ArchiveEntry;
class ArchiveDir;
class UndoManager;

namespace ui
{
	class ArchiveViewModel : public wxDataViewModel
	{
	public:
		ArchiveViewModel() = default;

		void openArchive(shared_ptr<Archive> archive, UndoManager* undo_manager);
		void setFilter(string_view name, string_view category);

	private:
		weak_ptr<Archive>    archive_;
		ScopedConnectionList connections_;
		vector<string>       filter_name_;
		string               filter_category_;
		UndoManager*         undo_manager_ = nullptr;
		bool                 sort_enabled_ = true;

		// wxDataViewModel
		unsigned int   GetColumnCount() const override { return 4; }
		wxString       GetColumnType(unsigned int col) const override;
		void           GetValue(wxVariant& variant, const wxDataViewItem& item, unsigned int col) const override;
		bool           GetAttr(const wxDataViewItem& item, unsigned int col, wxDataViewItemAttr& attr) const override;
		bool           SetValue(const wxVariant& variant, const wxDataViewItem& item, unsigned int col) override;
		wxDataViewItem GetParent(const wxDataViewItem& item) const override;
		bool           IsContainer(const wxDataViewItem& item) const override;
		unsigned int   GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const override;
		bool           IsListModel() const override;
		bool           HasDefaultCompare() const override { return sort_enabled_; }
		int Compare(const wxDataViewItem& item1, const wxDataViewItem& item2, unsigned int column, bool ascending)
			const override;

		wxDataViewItem createItemForDirectory(const ArchiveDir& dir) const;
		bool           matchesFilter(const ArchiveEntry& entry) const;
		void           getDirChildItems(wxDataViewItemArray& items, const ArchiveDir& dir, bool filter = true) const;
	};

	class ArchiveEntryTree : public wxDataViewCtrl
	{
	public:
		ArchiveEntryTree(wxWindow* parent, shared_ptr<Archive> archive, UndoManager* undo_manager);

		ArchiveEntry* entryForItem(const wxDataViewItem& item) const
		{
			return static_cast<ArchiveEntry*>(item.GetID());
		}
		ArchiveDir* dirForDirItem(const wxDataViewItem& item) const;

		bool isSortedByName() const { return GetSortingColumn() == col_name_; }
		bool isSortedBySize() const { return GetSortingColumn() == col_size_; }
		bool isSortedByType() const { return GetSortingColumn() == col_type_; }
		bool isDefaultSorted() const;

		vector<ArchiveEntry*> selectedEntries(bool include_dirs = false) const;
		ArchiveEntry*         firstSelectedEntry(bool include_dirs = false) const;
		ArchiveEntry*         lastSelectedEntry(bool include_dirs = false) const;
		vector<ArchiveDir*>   selectedDirectories() const;
		ArchiveDir*           firstSelectedDirectory() const;
		ArchiveDir*           lastSelectedDirectory() const;
		wxDataViewItem        firstSelectedItem() const;
		wxDataViewItem        lastSelectedItem() const;
		ArchiveDir*           currentSelectedDir() const;
		ArchiveDir*           selectedEntriesDir() const;
		vector<ArchiveDir*>   expandedDirs() const;

		void setFilter(string_view name, string_view category);
		void collapseAll(const ArchiveDir& dir_start);

	private:
		weak_ptr<Archive> archive_;
		ArchiveViewModel* model_     = nullptr;
		wxDataViewColumn* col_name_  = nullptr;
		wxDataViewColumn* col_size_  = nullptr;
		wxDataViewColumn* col_type_  = nullptr;
		wxDataViewColumn* col_index_ = nullptr;

		void setupColumns();
		void saveColumnWidths() const;
		void updateColumnWidths();
	};

} // namespace ui
} // namespace slade
