#pragma once

// An event to indicate when the selection has changed
wxDECLARE_EVENT(EVT_VLV_SELECTION_CHANGED, wxCommandEvent);

class VirtualListView : public wxListCtrl
{
public:
	VirtualListView(wxWindow* parent);
	virtual ~VirtualListView();

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
	vector<long> getSelection(bool item_indices = false);
	long         getFirstSelected();
	long         getLastSelected();

	// Focus
	void focusItem(long item, bool focus = true);
	void focusOnIndex(long index);
	long getFocus();
	bool lookForSearchEntryFrom(long focus);

	// Layout
	void         updateWidth();
	virtual void updateList(bool clear = false);

	// Label editing
	virtual void labelEdited(int col, int index, string new_label) {}

	// Filtering
	long         getItemIndex(long item) const;
	virtual void applyFilter() {}

	// Sorting
	int          sortColumn() { return sort_column_; }
	bool         sortDescend() { return sort_descend_; }
	static bool  defaultSort(long left, long right);
	static bool  indexSort(long left, long right) { return lv_current->sort_descend_ ? right < left : left < right; }
	virtual void sortItems();
	void         setColumnHeaderArrow(long column, int arrow);

protected:
	wxListItemAttr* item_attr_;
	wxFont*         font_normal_;
	wxFont*         font_monospace_;

	// Item sorting/filtering
	vector<long> items_;
	int          sort_column_;
	bool         sort_descend_;
	int          filter_column_;
	string       filter_text_;

	static VirtualListView* lv_current;

	virtual string getItemText(long item, long column, long index) const { return "UNDEFINED"; }
	virtual int    getItemIcon(long item, long column, long index) const { return -1; }
	virtual void   updateItemAttr(long item, long column, long index) const {}

	// Virtual wxListCtrl overrides
	virtual string OnGetItemText(long item, long column) const { return getItemText(item, column, getItemIndex(item)); }
	virtual int    OnGetItemImage(long item) const { return getItemIcon(item, 0, getItemIndex(item)); }
	virtual int    OnGetItemColumnImage(long item, long column) const
	{
		return getItemIcon(item, column, getItemIndex(item));
	}
	virtual wxListItemAttr* OnGetItemAttr(long item) const
	{
		updateItemAttr(item, 0, getItemIndex(item));
		return item_attr_;
	}
	virtual wxListItemAttr* OnGetItemColumnAttr(long item, long column) const
	{
		updateItemAttr(item, column, getItemIndex(item));
		return item_attr_;
	}

	// Events
	void onColumnResize(wxListEvent& e);
	void onMouseLeftDown(wxMouseEvent& e);
	void onKeyDown(wxKeyEvent& e);
	void onKeyChar(wxKeyEvent& e);
	void onLabelEditBegin(wxListEvent& e);
	void onLabelEditEnd(wxListEvent& e);
	void onColumnLeftClick(wxListEvent& e);
	void onItemSelected(wxListEvent& e);

private:
	long   last_focus_;
	string search_;
	int    col_search_;
	bool   cols_editable_[100]; // Never really going to have more than 100 columns
	bool   selection_updating_;

	void sendSelectionChangedEvent();
};
