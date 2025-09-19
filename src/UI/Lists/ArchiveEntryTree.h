#pragma once

#include "General/Sigslot.h"
#include "SDataViewCtrl.h"
#include "UI/SAuiToolBar.h"

namespace slade
{
class UndoManager;
class SToolButton;

namespace ui
{
	class ArchivePathPanel : public SAuiToolBar
	{
	public:
		ArchivePathPanel(wxWindow* parent);
		~ArchivePathPanel() override;

		void setCurrentPath(const ArchiveDir* dir);

	private:
		wxStaticText* text_path_ = nullptr;
	};

	class ArchiveViewModel : public wxDataViewModel
	{
	public:
		ArchiveViewModel() = default;

		enum class ViewType
		{
			Tree,
			List
		};

		ViewType    viewType() const { return view_type_; }
		ArchiveDir* rootDir() const { return root_dir_.lock().get(); }

		void setFilter(string_view name, string_view category);
		void showModifiedIndicators(bool show) { modified_indicator_ = show; }
		void setRootDir(const shared_ptr<ArchiveDir>& dir);
		void setRootDir(const wxDataViewItem& item);
		void setPathPanel(ArchivePathPanel* path_panel);

		void openArchive(const shared_ptr<Archive>& archive, UndoManager* undo_manager, bool force_list = false);

		ArchiveEntry* entryForItem(const wxDataViewItem& item) const
		{
			return static_cast<ArchiveEntry*>(item.GetID());
		}
		ArchiveDir*    dirForDirItem(const wxDataViewItem& item) const;
		wxDataViewItem createItemForDirectory(const ArchiveDir& dir) const;

		void reload(ViewType view_type);

	private:
		weak_ptr<Archive>    archive_;
		weak_ptr<ArchiveDir> root_dir_;
		ScopedConnectionList connections_;
		vector<string>       filter_name_;
		string               filter_category_;
		UndoManager*         undo_manager_       = nullptr;
		bool                 sort_enabled_       = true;
		bool                 modified_indicator_ = true;
		ViewType             view_type_          = ViewType::Tree;
		ArchivePathPanel*    path_panel_         = nullptr;

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

		bool matchesFilter(const ArchiveEntry& entry) const;
		void getDirChildItems(wxDataViewItemArray& items, const ArchiveDir& dir, bool filter = true) const;
		bool entryIsInList(const ArchiveEntry& entry) const;
		bool dirIsInList(const ArchiveDir& dir) const;
	};

	class ArchiveEntryTree : public SDataViewCtrl
	{
	public:
		ArchiveEntryTree(
			wxWindow*                  parent,
			const shared_ptr<Archive>& archive,
			UndoManager*               undo_manager,
			bool                       force_list = false);

		ArchiveEntry* entryForItem(const wxDataViewItem& item) const
		{
			return model_ ? model_->entryForItem(item) : nullptr;
		}

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
		ArchiveDir*           currentRootDir() const;

		void setPathPanel(ArchivePathPanel* path_panel) const
		{
			if (model_)
				model_->setPathPanel(path_panel);
		}
		void setFilter(string_view name, string_view category);
		void collapseAll(const ArchiveDir& dir_start);
		void upDir();
		void homeDir();
		void goToDir(const shared_ptr<ArchiveDir>& dir, bool expand = false);
		void reloadModel(bool tree = true);

		// Overrides
		void EnsureVisible(const wxDataViewItem& item, const wxDataViewColumn* column = nullptr) override;

	private:
		weak_ptr<Archive> archive_;
		ArchiveViewModel* model_                   = nullptr;
		wxDataViewColumn* col_name_                = nullptr;
		wxDataViewColumn* col_size_                = nullptr;
		wxDataViewColumn* col_type_                = nullptr;
		wxDataViewColumn* col_index_               = nullptr;
		int               multi_select_base_index_ = -1;
		string            search_;

		void setupColumns();
		void updateColumnWidths();
		void saveColumnConfig();
		void onAnyColumnResized() override;
	};

} // namespace ui
} // namespace slade
