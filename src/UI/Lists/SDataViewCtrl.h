#pragma once

#include <array>

// An event to indicate when a column has been resized
wxDECLARE_EVENT(EVT_SDVC_COLUMN_RESIZED, wxDataViewEvent);

namespace slade::ui
{
class SDataViewCtrl : public wxDataViewCtrl
{
public:
	enum class ColumnVisibility
	{
		AlwaysVisible, // Column is always visible, don't save visibility state
		Visible,
		Hidden,
	};

	enum class ColumnType
	{
		Text,
		IconAndText
	};

	SDataViewCtrl(wxWindow* parent, long style);
	~SDataViewCtrl() override;

	wxDataViewColumn* lastVisibleColumn() const;

	void setSearchColumn(int col_model) { search_model_column_ = col_model; }

	wxDataViewColumn* addColumn(
		ColumnType       type,
		int              model_column,
		string_view      title,
		int              width,
		string_view      state_id   = {},
		ColumnVisibility visibility = ColumnVisibility::Visible,
		bool             editable   = false);

	void resetSorting();
	void enableHeaderContextMenu();
	void appendColumnToggleItem(wxMenu& menu, int col_model) const;
	void toggleColumnVisibility(int col_model) const;
	void setColumnWidth(wxDataViewColumn* column, int width) const;
	void setColumnWidth(int col_model, int width) const;
	int  modelColumnIndex(int model_column) const;

	// Column state persistence
	void loadColumnState(const Archive* archive = nullptr);
	void restoreColumnWidths(const Archive* archive = nullptr);
	void loadSortState(const Archive* archive = nullptr);
	void saveSortState(const Archive* archive = nullptr) const;

protected:
	mutable std::array<int, 50> column_widths_; // For detecting column width changes on Linux/Mac

	// Column state persistence
	struct ColumnDef
	{
		wxDataViewColumn* column = nullptr;
		string            id;                     // Unique ID for this column (used to persist state)
		bool              always_visible = false; // Don't save visibility state for this column, always show it
	};
	vector<ColumnDef> columns_state_;
	string            prop_sort_column_;
	string            prop_sort_descending_;

	// The associated Archive to use for column state persistence
	virtual const Archive* stateArchive() const { return nullptr; }

	virtual void onColumnResized(wxDataViewColumn* column) {}
	virtual void onAnyColumnResized();

private:
	int     multi_select_base_index_ = -1;
	string  search_;
	int     search_model_column_ = -1;
	wxTimer column_resize_check_timer_;

	// Menu id for the header context menu's 'Reset Sorting' item (won't clash with a column model index)
	static constexpr int id_reset_sorting_ = wxID_HIGHEST + 1;

#ifdef __WXMSW__
	bool lookForSearchItemFrom(int index_start);
	bool searchChar(int key_code);
#endif
};
} // namespace slade::ui
