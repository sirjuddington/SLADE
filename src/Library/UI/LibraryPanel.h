#pragma once

#include "General/SAction.h"
#include "General/Sigslot.h"
#include "Library/ArchiveFile.h"
#include "UI/Lists/SDataViewCtrl.h"

namespace slade
{
class SToolBar;

namespace ui
{
	class LibraryViewModel : public wxDataViewModel
	{
	public:
		LibraryViewModel();
		~LibraryViewModel() override = default;

		enum class Column
		{
			Name = 0,
			Path,
			Size,
			Type,
			LastOpened,
			FileModified,
			EntryCount,
			MapCount,

			_Count
		};

		struct LibraryListRow
		{
			int64_t  id = -1;
			string   path;
			unsigned size = 0;
			string   format_id;
			time_t   last_opened   = 0;
			time_t   last_modified = 0;
			unsigned entry_count   = 0;
			unsigned map_count     = 0;

			LibraryListRow() = default;
			LibraryListRow(database::Context& db, int64_t id);
		};

		wxDataViewItem  itemForArchiveId(int64_t id) const;
		LibraryListRow* rowForItem(const wxDataViewItem& item) const
		{
			return static_cast<LibraryListRow*>(item.GetID());
		}

	private:
		mutable vector<LibraryListRow> rows_;
		ScopedConnectionList           signal_connections_;

		// wxDataViewModel
		unsigned int   GetColumnCount() const override { return static_cast<unsigned>(Column::_Count); }
		wxString       GetColumnType(unsigned int col) const override;
		void           GetValue(wxVariant& variant, const wxDataViewItem& item, unsigned int col) const override;
		bool           GetAttr(const wxDataViewItem& item, unsigned int col, wxDataViewItemAttr& attr) const override;
		bool           SetValue(const wxVariant& variant, const wxDataViewItem& item, unsigned int col) override;
		wxDataViewItem GetParent(const wxDataViewItem& item) const override;
		bool           IsContainer(const wxDataViewItem& item) const override;
		unsigned int   GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const override;
		bool           IsListModel() const override { return true; }
		bool           HasDefaultCompare() const override { return true; }
		int Compare(const wxDataViewItem& item1, const wxDataViewItem& item2, unsigned int column, bool ascending)
			const override;

		void loadRows() const;
	};

	class LibraryPanel : public wxPanel, SActionHandler
	{
	public:
		LibraryPanel(wxWindow* parent);
		~LibraryPanel() override = default;

		void setup();

		// SAction handler
		bool handleAction(string_view id) override;

	private:
		SDataViewCtrl*    list_archives_ = nullptr;
		LibraryViewModel* model_library_ = nullptr;
		SToolBar*         toolbar_       = nullptr;

		void bindEvents();
		void setupListColumns() const;
		void updateColumnWidths();
		void appendTextColumn(LibraryViewModel::Column column, string_view title, string_view id) const;

		wxDataViewColumn* modelColumn(LibraryViewModel::Column column) const
		{
			return list_archives_->GetColumn(list_archives_->GetModelColumnIndex(static_cast<int>(column)));
		}
	};
} // namespace ui
} // namespace slade
