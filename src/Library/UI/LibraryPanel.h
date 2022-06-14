#pragma once

#include "General/SAction.h"
#include "General/Sigslot.h"
#include "Library/ArchiveFile.h"

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

			_Count
		};

		wxDataViewItem itemForArchiveId(int64_t id) const;

	private:
		mutable vector<library::ArchiveFileRow> rows_;
		ScopedConnectionList                    signal_connections_;

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
		wxDataViewCtrl*   list_archives_ = nullptr;
		LibraryViewModel* model_library_ = nullptr;
		SToolBar*         toolbar_       = nullptr;

		void bindEvents() const;
	};
} // namespace ui
} // namespace slade
