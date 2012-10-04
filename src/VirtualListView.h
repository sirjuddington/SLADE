
#ifndef __VIRTUAL_LIST_VIEW_H__
#define __VIRTUAL_LIST_VIEW_H__

#include <wx/listctrl.h>

// An event to indicate when the selection has changed
wxDECLARE_EVENT(EVT_VLV_SELECTION_CHANGED, wxCommandEvent);

class VirtualListView : public wxListCtrl {
private:
	long	last_focus;
	string	search;
	int		col_search;
	bool	cols_editable[100];	// Never really going to have more than 100 columns

	void	sendSelectionChangedEvent();

protected:
	wxListItemAttr*	item_attr;

	virtual string	getItemText(long item, long column) const { return "UNDEFINED"; }
	virtual int		getItemIcon(long item) const { return -1; }
	virtual void	updateItemAttr(long item) const {}

	// Virtual wxListCtrl overrides
	string			OnGetItemText(long item, long column) const { return getItemText(item, column); }
	int				OnGetItemImage(long item) const { return getItemIcon(item); }
	wxListItemAttr*	OnGetItemAttr(long item) const { updateItemAttr(item); return item_attr; }

public:
	VirtualListView(wxWindow* parent);
	~VirtualListView();

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
	virtual void	updateList(bool clear = false) { SetItemCount(0); }

	// Label editing
	virtual void	labelEdited(int col, int index, string new_label) {}

	// Events
	void	onColumnResize(wxListEvent& e);
	void	onMouseLeftDown(wxMouseEvent& e);
	void	onKeyDown(wxKeyEvent& e);
	void	onKeyChar(wxKeyEvent& e);
	void	onLabelEditBegin(wxListEvent& e);
	void	onLabelEditEnd(wxListEvent& e);
};

#endif//__VIRTUAL_LIST_VIEW_H__
