#pragma once

#include <array>

// An event to indicate when a column has been resized
wxDECLARE_EVENT(EVT_SDVC_COLUMN_RESIZED, wxDataViewEvent);

namespace slade::ui
{
class SDataViewCtrl : public wxDataViewCtrl
{
public:
	SDataViewCtrl(wxWindow* parent, long style);

	wxDataViewColumn* lastVisibleColumn() const;

	void setSearchColumn(int col_model) { search_model_column_ = col_model; }

	void resetSorting();
	void appendColumnToggleItem(wxMenu& menu, int col_model) const;
	void toggleColumnVisibility(int col_model, string_view state_prop) const;
	void setColumnWidth(wxDataViewColumn* column, int width) const;
	void setColumnWidth(int col_model, int width) const;
	int  modelColumnIndex(int model_column) const;

protected:
	mutable std::array<int, 50> column_widths_; // Doubt we'll ever need more than 50 columns

	virtual void onColumnResized(wxDataViewColumn* column) {}
	virtual void onAnyColumnResized() {}

private:
	int    multi_select_base_index_ = -1;
	string search_;
	int    search_model_column_ = -1;

#ifdef __WXMSW__
	bool lookForSearchItemFrom(int index_start);
	bool searchChar(int key_code);
#endif
};
} // namespace slade::ui
