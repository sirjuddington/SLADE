
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    VirtualListView.cpp
 * Description: A 'virtual' list control that makes use of the
 *              wxListCtrl::wxLC_VIRTUAL style. With this, the list
 *              works differently to the normal list view. Rather
 *              than containing specific items, the virtual list
 *              uses virtual functions to get item details from an
 *              external source.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "VirtualListView.h"
#include "UI/WxStuff.h"
#ifdef __WXMSW__
#include <CommCtrl.h>
#endif
#include "common.h"

/*******************************************************************
 * VARIABLES
 *******************************************************************/
wxDEFINE_EVENT(EVT_VLV_SELECTION_CHANGED, wxCommandEvent);
CVAR(Bool, list_font_monospace, false, CVAR_SAVE)
VirtualListView* VirtualListView::lv_current = NULL;
int vlv_chars[] =
{
	'.', ',', '_', '-', '+', '=', '`', '~',
	'!', '@', '#', '$', '(', ')', '[', ']',
	'{', '}', ':', ';', '/', '\\', '<', '>',
	'?', '^', '&', '\'', '\"',
};
int n_vlv_chars = 30;


/*******************************************************************
 * VIRTUALLISTVIEW CLASS FUNCTIONS
 *******************************************************************/

/* VirtualListView::VirtualListView
 * VirtualListView class constructor
 *******************************************************************/
VirtualListView::VirtualListView(wxWindow* parent)
#ifdef __WXMSW__
	: wxListCtrl(parent, -1, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_VIRTUAL|wxLC_EDIT_LABELS)
{
#else
	: wxListCtrl(parent, -1, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_VIRTUAL)
{
#endif
	item_attr = new wxListItemAttr();
	last_focus = 0;
	col_search = 0;
	filter_text = "";
	sort_column = -1;
	filter_column = -1;
	sort_descend = false;
	selection_updating = false;
	memset(cols_editable, 0, 100);

	// Set monospace font if configured
	font_normal = new wxFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
	font_monospace = new wxFont(getMonospaceFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT)));
	if (list_font_monospace)
		item_attr->SetFont(*font_monospace);

	// Bind events
#ifndef __WXMSW__
	Bind(wxEVT_KEY_DOWN, &VirtualListView::onKeyDown, this);
#endif
	Bind(wxEVT_LEFT_DOWN, &VirtualListView::onMouseLeftDown, this);
	Bind(wxEVT_LIST_COL_END_DRAG, &VirtualListView::onColumnResize, this);
	Bind(wxEVT_CHAR, &VirtualListView::onKeyChar, this);
	Bind(wxEVT_LIST_BEGIN_LABEL_EDIT, &VirtualListView::onLabelEditBegin, this);
	Bind(wxEVT_LIST_END_LABEL_EDIT, &VirtualListView::onLabelEditEnd, this);
	Bind(wxEVT_LIST_COL_CLICK, &VirtualListView::onColumnLeftClick, this);
#ifdef __WXGTK__
	Bind(wxEVT_LIST_ITEM_SELECTED, &VirtualListView::onItemSelected, this);
#endif
}

/* VirtualListView::~VirtualListView
 * VirtualListView class destructor
 *******************************************************************/
VirtualListView::~VirtualListView()
{
	delete item_attr;
	delete font_monospace;
	delete font_normal;
}

/* VirtualListView::sendSelectionChangedEvent
 * Creates and sends an EVT_VLV_SELECTION_CHANGED wxwidgets event
 *******************************************************************/
void VirtualListView::sendSelectionChangedEvent()
{
	wxCommandEvent evt(EVT_VLV_SELECTION_CHANGED, GetId());
	ProcessWindowEvent(evt);
	selection_updating = false;
}

/* VirtualListView::updateWidth
 * Updates the list's minimum requested width to allow the
 * widget to be shown with no horizontal scrollbar
 *******************************************************************/
void VirtualListView::updateWidth()
{
	// Get total column width
	int width = 8;
	for (int a = 0; a < GetColumnCount(); a++)
		width += GetColumnWidth(a);

	// Always leave room for the scrollbar (wxWidgets is silly)
	width += wxSystemSettings::GetMetric(wxSYS_VSCROLL_X, this);

	// Set widget size
	SetSizeHints(width, -1);
}

/* VirtualListView::selectItem
 * Selects (or deselects) [item], depending on [select]
 *******************************************************************/
void VirtualListView::selectItem(long item, bool select)
{
	// Check item id is in range
	if (item >= GetItemCount())
		return;

	// Select/deselect the item
	if (select)
		SetItemState(item, 0xFFFF, wxLIST_STATE_SELECTED);
	else
		SetItemState(item, 0x0000, wxLIST_STATE_SELECTED);
}

/* VirtualListView::selectItems
 * Selects/deselects all items within the range [start]->[end],
 * depending on [select]
 *******************************************************************/
void VirtualListView::selectItems(long start, long end, bool select)
{
	// Check/correct indices
	if (start > end)
	{
		long temp = start;
		start = end;
		end = temp;
	}
	if (start < 0) start = 0;
	if (end >= GetItemCount()) end = GetItemCount() - 1;

	// Go through range
	for (long a = start; a <= end; a++)
	{
		// Select/deselect the item
		if (select)
			SetItemState(a, 0xFFFF, wxLIST_STATE_SELECTED);
		else
			SetItemState(a, 0x0000, wxLIST_STATE_SELECTED);
	}
}

/* VirtualListView::selectAll
 * Selects all list items
 *******************************************************************/
void VirtualListView::selectAll()
{
	int itemcount = GetItemCount();
	for (int a = 0; a < itemcount; a++)
		SetItemState(a, 0xFFFF, wxLIST_STATE_SELECTED);

	sendSelectionChangedEvent();
}

/* VirtualListView::clearSelection
 * Deselects all list items
 *******************************************************************/
void VirtualListView::clearSelection()
{
	int itemcount = GetItemCount();
	for (int a = 0; a < itemcount; a++)
		SetItemState(a, 0x0000, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
}

/* VirtualListView::getSelection
 * Returns a list of all selected item indices. If [item_indices] is
 * true, the returned indices will have sorting and filtering applied
 *******************************************************************/
vector<long> VirtualListView::getSelection(bool item_indices)
{
	// Init return array
	vector<long> ret;

	// Go through all items
	long item = -1;
	while (true)
	{
		// Get the next item in the list that is selected
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);

		// If -1 then none were selected
		if (item == -1)
			break;

		// Otherwise add the selected index to the vector
		ret.push_back(item_indices ? getItemIndex(item) : item);
	}

	return ret;
}

/* VirtualListView::getFirstSelected
 * Returns the first selected item index
 *******************************************************************/
long VirtualListView::getFirstSelected()
{
	return GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
}

/* VirtualListView::getLastSelected
 * Returns the last selected item index
 *******************************************************************/
long VirtualListView::getLastSelected()
{
	// Go through all items
	long item = -1;
	while (true)
	{
		// Get the next item in the list that is selected
		long index = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);

		// Exit loop if there is no selected item after the current
		if (index == -1)
			break;

		// Otherwise set the item to this index and keep going
		item = index;
	}

	return item;
}

/* VirtualListView::focusItem
 * Sets the focus of [item]
 *******************************************************************/
void VirtualListView::focusItem(long item, bool focus)
{
	// Check item id is in range
	if (item >= GetItemCount())
		return;

	// Select/deselect the item
	if (focus)
	{
		SetItemState(item, 0xFFFF, wxLIST_STATE_FOCUSED);
		last_focus = item;
	}
	else
		SetItemState(item, 0x0000, wxLIST_STATE_FOCUSED);
}

/* VirtualListView::getFocus
 * Returns the index of the currently focused item
 *******************************************************************/
long VirtualListView::getFocus()
{
	return GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED);
}

/* VirtualListView::defaultSort
 * Default sorting calculation, sorts by index if there is no sorted
 * column, otherwise sorts by the column item text > index
 *******************************************************************/
bool VirtualListView::defaultSort(long left, long right)
{
	// No sort column, just sort by index
	if (lv_current->sort_column < 0)
		return lv_current->sort_descend ? right < left : left < right;

	// Sort by column text > index
	else
	{
		int result = lv_current->getItemText(left, lv_current->sort_column, left).Lower().compare(lv_current->getItemText(right, lv_current->sort_column, right).Lower());
		if (result == 0)
			return left < right;
		else
			return lv_current->sort_descend ? result > 0 : result < 0;
	}
}

/* VirtualListView::getItemIndex
 * Returns the filtered index of the list item at [item]
 *******************************************************************/
long VirtualListView::getItemIndex(long item) const
{
	if (item < 0 || item >= (long)items.size())
		return item;
	else
		return items[item];
}

/* VirtualListView::updateList
 * Updates the list item count and refreshes it
 *******************************************************************/
void VirtualListView::updateList(bool clear)
{
	// Update list
	if (!items.empty())
		SetItemCount(items.size());

	Refresh();
}

/* VirtualListView::sortItems
 * Sorts the list items depending on the current sorting column
 *******************************************************************/
void VirtualListView::sortItems()
{
	lv_current = this;
	std::sort(items.begin(), items.end(), &VirtualListView::defaultSort);
}

/* VirtualListView::setColumnHeaderArrow
 * Sets the sorting arrow indicator on [column], [arrow] can be 0
 * (none), 1 (up) or 2 (down)
 *******************************************************************/
void VirtualListView::setColumnHeaderArrow(long column, int arrow)
{
	// Win32 implementation
#ifdef __WXMSW__
	HWND hwnd = ListView_GetHeader((HWND)GetHandle());
	HDITEM header ={ 0 };
	if (hwnd)
	{
		header.mask = HDI_FORMAT;

		if (Header_GetItem(hwnd, column, &header))
		{
			if (arrow == 2)
				header.fmt = (header.fmt & ~HDF_SORTUP)|HDF_SORTDOWN;
			else if (arrow == 1)
				header.fmt = (header.fmt & ~HDF_SORTDOWN)|HDF_SORTUP;
			else
				header.fmt = header.fmt & ~(HDF_SORTDOWN|HDF_SORTUP);

			Header_SetItem(hwnd, column, &header);
		}
	}
#endif
}

/* VirtualListView::focusOnIndex
 * Selects an entry by its given index and makes sure it is visible
 *******************************************************************/
void VirtualListView::focusOnIndex(long index)
{
	if (index < GetItemCount())
	{
		clearSelection();
		selectItem(index);
		focusItem(index);
		EnsureVisible(index);
		sendSelectionChangedEvent();
	}
}

/* VirtualListView::lookForSearchEntryFrom
 * Used by VirtualListView::onKeyChar, returns true if an entry
 * matching search is found, false otherwise
 *******************************************************************/
bool VirtualListView::lookForSearchEntryFrom(long focus)
{
	long index = focus;
	bool looped = false;
	bool gotmatch = false;
	while ((!looped && index < GetItemCount()) || (looped && index < focus))
	{
		string name = getItemText(index, col_search, items[index]);
		if (name.Upper().StartsWith(search))
		{
			// Matches, update selection+focus
			focusOnIndex(index);
			return true;
		}

		// No match, next item; look in the above entries
		// if no matches were found below.
		if (++index == GetItemCount() && !looped)
		{
			looped = true;
			index = 0;
		}
	}
	// Didn't get any match
	return false;
}


/*******************************************************************
 * VIRTUALLISTVIEW CLASS EVENTS
 *******************************************************************/

/* VirtualListView::onColumnResize
 * Called when a column is resized
 *******************************************************************/
void VirtualListView::onColumnResize(wxListEvent& e)
{
	// Update width etc
	updateWidth();
	if (GetParent())
		GetParent()->Layout();
}

/* VirtualListView::onMouseLeftDown
 * Called when the list is left clicked
 *******************************************************************/
void VirtualListView::onMouseLeftDown(wxMouseEvent& e)
{
#ifndef __WXMSW__
	// Default handler for double-click
	if (e.ButtonDClick())
	{
		e.Skip();
		return;
	}

	// Get item at click position
	int flags = 0;
	long item = this->HitTest(wxPoint(e.GetX(), e.GetY()), flags);
	if (flags & wxLIST_HITTEST_ONITEM)
	{
		if (e.GetModifiers() == wxMOD_SHIFT)
		{
			// Shift+left click: Add all items between the focused item and the item that was clicked to the selection
			long focus = getFocus();
			if (focus < 0) focus = last_focus;		// If no current focus, go with last focused item
			selectItems(item, focus);
			focusItem(item);
			sendSelectionChangedEvent();
		}
		else if (e.GetModifiers() == wxMOD_CMD)
		{
			// Ctrl+left click: Toggle the selection status of the clicked item
			bool selected = !!(GetItemState(item, wxLIST_STATE_SELECTED) & wxLIST_STATE_SELECTED);
			selectItem(item, !selected);
			focusItem(item, !selected);
			sendSelectionChangedEvent();
		}
		else if (e.GetModifiers() == wxMOD_NONE)
		{
			// Just a left click, select+focus the item
			clearSelection();
			selectItem(item);
			focusItem(item);
			sendSelectionChangedEvent();
			e.Skip();
		}

		search = "";
	}
#else
	search = "";
	e.Skip();
#endif
}

/* VirtualListView::onKeyDown
 * Called when a key is pressed within the list
 *******************************************************************/
void VirtualListView::onKeyDown(wxKeyEvent& e)
{
	if (e.GetKeyCode() == WXK_UP)
	{
		if (e.GetModifiers() == wxMOD_SHIFT)
		{
			long focus = getFocus();
			if (focus < 0) focus = last_focus;		// If no current focus, go with last focused item
			if (focus > 0)
			{
				focusItem(focus, false);
				selectItem(focus-1);
				focusItem(focus-1);
				EnsureVisible(focus-1);
				sendSelectionChangedEvent();
			}
		}
		else if (e.GetModifiers() == wxMOD_NONE)
		{
			long focus = getFocus();
			if (focus < 0) focus = last_focus;		// If no current focus, go with last focused item
			if (focus > 0)
			{
				clearSelection();
				focusItem(focus, false);
				selectItem(focus-1);
				focusItem(focus-1);
				EnsureVisible(focus-1);
				sendSelectionChangedEvent();
			}
		}
		search = "";
	}
	else if (e.GetKeyCode() == WXK_DOWN)
	{
		if (e.GetModifiers() == wxMOD_SHIFT)
		{
			long focus = getFocus();
			if (focus < 0) focus = last_focus;		// If no current focus, go with last focused item
			if (focus < GetItemCount() - 1)
			{
				focusItem(focus, false);
				selectItem(focus+1);
				focusItem(focus+1);
				EnsureVisible(focus+1);
				sendSelectionChangedEvent();
			}
		}
		else if (e.GetModifiers() == wxMOD_NONE)
		{
			long focus = getFocus();
			if (focus < 0) focus = last_focus;		// If no current focus, go with last focused item
			if (focus < GetItemCount() - 1)
			{
				clearSelection();
				focusItem(focus, false);
				selectItem(focus+1);
				focusItem(focus+1);
				EnsureVisible(focus+1);
				sendSelectionChangedEvent();
			}
		}
		search = "";
	}
	else
		e.Skip();
}

/* VirtualListView::onKeyChar
 * Called when a 'character' key is pressed within the list
 *******************************************************************/
void VirtualListView::onKeyChar(wxKeyEvent& e)
{
	// Check the key pressed is actually a character (a-z, 0-9 etc)
	bool isRealChar = false;
	if (e.GetKeyCode() >= 'a' && e.GetKeyCode() <= 'z')			// Lowercase
		isRealChar = true;
	else if (e.GetKeyCode() >= 'A' && e.GetKeyCode() <= 'Z')	// Uppercase
		isRealChar = true;
	else if (e.GetKeyCode() >= '0' && e.GetKeyCode() <= '9')	// Number
		isRealChar = true;
	else
	{
		for (int a = 0; a < n_vlv_chars; a++)
		{
			if (e.GetKeyCode() == vlv_chars[a])
			{
				isRealChar = true;
				break;
			}
		}
	}

	if (isRealChar && e.GetModifiers() == 0)
	{
		// Get currently focused item (or first if nothing is focused)
		long focus = getFocus();
		if (focus < 0) focus = 0;

		// Build search string
		search += e.GetKeyCode();
		search.MakeUpper();

		// Search for match from the current focus, and if failed
		// start a new search from after the current focus.
		if (!lookForSearchEntryFrom(focus))
		{
			search = S_FMT("%c", e.GetKeyCode());
			search.MakeUpper();
			lookForSearchEntryFrom(focus+1);
		}
	}
	else
	{
		search = "";
#ifdef __WXGTK__
		e.Skip();
#else
		// Only want to do default action on navigation key
		if (e.GetKeyCode() == WXK_UP || e.GetKeyCode() == WXK_DOWN ||
		        e.GetKeyCode() == WXK_PAGEUP || e.GetKeyCode() == WXK_PAGEDOWN ||
		        e.GetKeyCode() == WXK_HOME || e.GetKeyCode() == WXK_END ||
		        e.GetKeyCode() == WXK_TAB)
			e.Skip();
#endif
	}
}

/* VirtualListView::onLabelEditBegin
 * Called when an item label is clicked twice to edit it
 *******************************************************************/
void VirtualListView::onLabelEditBegin(wxListEvent& e)
{
	// For now we'll enable it if editing column 0 is allowed
	if (!cols_editable[0])
		e.Veto();
	else
		e.Skip();
}

/* VirtualListView::onLabelEditEnd
 * Called when an item label edit event finishes
 *******************************************************************/
void VirtualListView::onLabelEditEnd(wxListEvent& e)
{
	if (!e.IsEditCancelled())
		labelEdited(e.GetColumn(), e.GetIndex(), e.GetLabel());
}

/* VirtualListView::onColumnLeftClick
 * Called when a column header is clicked
 *******************************************************************/
void VirtualListView::onColumnLeftClick(wxListEvent& e)
{
	// Clear current sorting arrow
	setColumnHeaderArrow(sort_column, 0);

	// Current sorting column clicked
	if (sort_column == e.GetColumn())
	{
		if (sort_descend)
		{
			sort_column = -1;
			sort_descend = false;
		}
		else
			sort_descend = true;
	}

	// Different sorting column clicked
	else
	{
		sort_column = e.GetColumn();
		sort_descend = false;
	}

	// Set new sorting arrow
	if (sort_column >= 0)
		setColumnHeaderArrow(sort_column, sort_descend ? 2 : 1);

	if (sort_column >= 0)
	{
		LOG_MESSAGE(2, "Sort column %d (%s)", sort_column, sort_descend ? "descending" : "ascending");
	}
	else
	{
		LOG_MESSAGE(2, "No sorting");
	}

	updateList();
}

/* VirtualListView::onItemSelected
 * Called when an item in the list is selected
 *******************************************************************/
void VirtualListView::onItemSelected(wxListEvent &e)
{
	if (!selection_updating)
	{
		selection_updating = true;
		CallAfter(&VirtualListView::sendSelectionChangedEvent);
	}
}
