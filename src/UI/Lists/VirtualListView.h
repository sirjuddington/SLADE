#pragma once

// An event to indicate when the selection has changed
wxDECLARE_EVENT(EVT_VLV_SELECTION_CHANGED, wxCommandEvent);

namespace slade
{
class VirtualListView : public wxListCtrl
{
public:
	VirtualListView(wxWindow* parent);
	~VirtualListView() override = default;

	void setSearchColumn(int col) { col_search_ = col; }
	void setColumnEditable(int col, bool edit = true)
	{
		if (col >= 0 && col < 100)
			cols_editable_[col] = edit;
	}

	// Selection
	void         selectItem(long item, bool select = true);
	void         selectItems(long start, long end, bool select = true);
	void         selectAll();
	void         clearSelection();
	vector<long> selection(bool item_indices = false) const;
	long         firstSelected() const;
	long         lastSelected() const;

	// Focus
	void focusItem(long item, bool focus = true);
	void focusOnIndex(long index);
	long focusedIndex() const;
	bool lookForSearchEntryFrom(long focus);

	// Layout
	void         updateWidth();
	virtual void updateList(bool clear = false);

	// Label editing
	virtual void labelEdited(int col, int index, const string& new_label) {}

	// Filtering
	long         itemIndex(long item) const;
	virtual void applyFilter() {}

	// Sorting
	int          sortColumn() const { return sort_column_; }
	bool         sortDescend() const { return sort_descend_; }
	static bool  defaultSort(long left, long right);
	static bool  indexSort(long left, long right) { return lv_current_->sort_descend_ ? right < left : left < right; }
	virtual void sortItems();
	void         setColumnHeaderArrow(long column, int arrow) const;

protected:
	unique_ptr<wxListItemAttr> item_attr_;
	wxFont                     font_normal_;
	wxFont                     font_monospace_;

	// Item sorting/filtering
	vector<long> items_;
	int          sort_column_   = -1;
	bool         sort_descend_  = false;
	int          filter_column_ = -1;
	string       filter_text_;

	static VirtualListView* lv_current_;

	virtual string itemText(long item, long column, long index) const { return "UNDEFINED"; }
	virtual int    itemIcon(long item, long column, long index) const { return -1; }
	virtual void   updateItemAttr(long item, long column, long index) const {}

	// Virtual wxListCtrl overrides
	wxString OnGetItemText(long item, long column) const override
	{
		return wxString::FromUTF8(itemText(item, column, itemIndex(item)));
	}
	int OnGetItemImage(long item) const override { return itemIcon(item, 0, itemIndex(item)); }
	int OnGetItemColumnImage(long item, long column) const override { return itemIcon(item, column, itemIndex(item)); }

	wxListItemAttr* OnGetItemAttr(long item) const override
	{
		updateItemAttr(item, 0, itemIndex(item));
		return item_attr_.get();
	}

#ifdef __WXMSW__
	wxListItemAttr* OnGetItemColumnAttr(long item, long column) const override
	{
		updateItemAttr(item, column, itemIndex(item));
		return item_attr_.get();
	}
#endif // __WXMSW__

	// Events
	void onColumnResize(wxListEvent& e);
	void onMouseLeftDown(wxMouseEvent& e);
	void onKeyDown(wxKeyEvent& e);
	void onKeyChar(wxKeyEvent& e);
	void onLabelEditBegin(wxListEvent& e);
	void onLabelEditEnd(wxListEvent& e);
	void onColumnLeftClick(wxListEvent& e);
	void onItemSelected(wxListEvent& e);
	void onIdle(wxIdleEvent& e);

private:
	string search_;
	long   last_focus_         = 0;
	int    col_search_         = 0;
	bool   cols_editable_[100] = {}; // Never really going to have more than 100 columns
	bool   selection_updating_ = false;
	int    prev_idle_selcount_ = 0;
	long   prev_idle_index_    = -1;

	void sendSelectionChangedEvent();
};
} // namespace slade
