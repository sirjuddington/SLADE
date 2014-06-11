
#ifndef __VIRTUAL_LIST_VIEW_H__
#define __VIRTUAL_LIST_VIEW_H__

#include <wx/listctrl.h>

// An event to indicate when the selection has changed
wxDECLARE_EVENT(EVT_VLV_SELECTION_CHANGED, wxCommandEvent);

class VirtualListView : public wxListCtrl
{
private:
	long	last_focus;
	string	search;
	int		col_search;
	bool	cols_editable[100];	// Never really going to have more than 100 columns

	void	sendSelectionChangedEvent();

protected:
	wxListItemAttr*	item_attr;
	wxFont*			font_normal;
	wxFont*			font_monospace;

	// Item sorting/filtering
	vector<long>	items;
	int				sort_column;
	bool			sort_descend;
	int				filter_column;
	string			filter_text;

	static VirtualListView* lv_current;

	virtual string	getItemText(long item, long column, long index) const { return "UNDEFINED"; }
	virtual int		getItemIcon(long item, long column, long index) const { return -1; }
	virtual void	updateItemAttr(long item, long column, long index) const {}

	// Virtual wxListCtrl overrides
	virtual string			OnGetItemText(long item, long column) const { return getItemText(item, column, getItemIndex(item)); }
	virtual int				OnGetItemImage(long item) const { return getItemIcon(item, 0, getItemIndex(item)); }
	virtual int				OnGetItemColumnImage(long item, long column) const { return getItemIcon(item, column, getItemIndex(item)); }
	virtual wxListItemAttr*	OnGetItemAttr(long item) const { updateItemAttr(item, 0, getItemIndex(item)); return item_attr; }
	virtual wxListItemAttr*	OnGetItemColumnAttr(long item, long column) const { updateItemAttr(item, column, getItemIndex(item)); return item_attr; }

public:
	VirtualListView(wxWindow* parent);
	virtual ~VirtualListView();

	void	setSearchColumn(int col) { col_search = col; }
	void	setColumnEditable(int col, bool edit = true) { if (col >= 0 && col < 100) cols_editable[col] = edit; }

	// Selection
	void			selectItem(long item, bool select = true);
	void			selectItems(long start, long end, bool select = true);
	void			selectAll();
	void			clearSelection();
	vector<long>	getSelection();
	long			getFirstSelected();
	long			getLastSelected();

	// Focus
	void			focusItem(long item, bool focus = true);
	void			focusOnIndex(long index);
	long			getFocus();
	bool			lookForSearchEntryFrom(long focus);

	// Layout
	void			updateWidth();
	virtual void	updateList(bool clear = false);

	// Label editing
	virtual void	labelEdited(int col, int index, string new_label) {}

	// Filtering
	long			getItemIndex(long item) const;
	virtual void	applyFilter() {}

	// Sorting
	bool			sortDescend() { return sort_descend; }
	static bool		defaultSort(long left, long right);
	static bool		indexSort(long left, long right) { return lv_current->sort_descend ? right < left : left < right; }
	virtual void	sortItems();

	// Events
	void	onColumnResize(wxListEvent& e);
	void	onMouseLeftDown(wxMouseEvent& e);
	void	onKeyDown(wxKeyEvent& e);
	void	onKeyChar(wxKeyEvent& e);
	void	onLabelEditBegin(wxListEvent& e);
	void	onLabelEditEnd(wxListEvent& e);
	void	onColumnLeftClick(wxListEvent& e);
};

#endif//__VIRTUAL_LIST_VIEW_H__
