
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ListView.cpp
// Description: An extended version of the default wxListCtrl, in 'report' mode.
//              Provides various commonly used functionality that doesn't exist
//              in wxListCtrl by default: selection stuff, basic item addition,
//              swapping, and setting item colours to various program defaults
//              ('new', 'modified', etc).
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "ListView.h"
#include "General/ColourConfiguration.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Compare two integers
// -----------------------------------------------------------------------------
int cmp_int(int* a, int* b)
{
	if (*a > *b)
		return 1;
	else if (*a < *b)
		return -1;
	else
		return 0;
}
} // namespace


// -----------------------------------------------------------------------------
//
// ListView Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ListView class constructor
// -----------------------------------------------------------------------------
ListView::ListView(wxWindow* parent, int id, long style) :
	wxListCtrl(parent, id, wxDefaultPosition, wxDefaultSize, style)
{
}

// -----------------------------------------------------------------------------
// Adds an item to the list at [index] with [text]
// -----------------------------------------------------------------------------
bool ListView::addItem(int index, const string& text)
{
	// Check index
	if (index < 0)
		index = 0;
	if (index > GetItemCount())
		index = GetItemCount();

	// Add the item to the list
	wxListItem li;
	li.SetId(index);
	li.SetColumn(0);
	li.SetText(wxString::FromUTF8(text));
	InsertItem(li);

	// Update list size
	updateSize();

	return true;
}

// -----------------------------------------------------------------------------
// Adds an item to the list at [index], with [text] in the columns;
// text[0] goes in column 0, etc
// -----------------------------------------------------------------------------
bool ListView::addItem(int index, const vector<string>& text)
{
	// Check index
	if (index < 0)
		index = 0;
	if (index > GetItemCount())
		index = GetItemCount();

	// Add the item to the list
	wxListItem li;
	li.SetId(index);
	InsertItem(li);

	// Set item text for each column
	for (size_t a = 0; a < text.size(); a++)
		setItemText(index, a, text[a]);

	// Update list size
	updateSize();

	return true;
}

// -----------------------------------------------------------------------------
// Deletes all items at indices [items]
// -----------------------------------------------------------------------------
bool ListView::deleteItems(wxArrayInt items)
{
	// Sort items list
	items.Sort(cmp_int);

	// Go through list backwards and delete each item
	for (int a = items.size() - 1; a >= 0; a--)
		DeleteItem(items[a]);

	return true;
}

// -----------------------------------------------------------------------------
// Calculates the 'disabled' item colour based on the list text and bg colours
// -----------------------------------------------------------------------------
ColRGBA ListView::disabledColour() const
{
	auto fg = wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT);
	auto bg = wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOX);

	uint8_t red   = fg.Red() * 0.5 + bg.Red() * 0.5;
	uint8_t green = fg.Green() * 0.5 + bg.Green() * 0.5;
	uint8_t blue  = fg.Blue() * 0.5 + bg.Blue() * 0.5;
	return { red, green, blue };
}

// -----------------------------------------------------------------------------
// Sets the 'status' of [item], changing its text colour.
// Currently there are 6 statuses:
// Normal - default colour
// Modified - blue
// New - green
// Locked - orange
// Error - red
// Disabled - faded (depending on text & background colour)
// -----------------------------------------------------------------------------
bool ListView::setItemStatus(int item, ItemStatus status)
{
	// Check item id is in range
	if (item >= GetItemCount())
		return false;

	// If given a negative item id, set all items in the list to the given status
	if (item < 0)
	{
		for (int a = 0; a < GetItemCount(); a++)
			setItemStatus(a, status);

		return true;
	}

	// Set item text colour to given status colour
	switch (status)
	{
	case ItemStatus::Normal:   SetItemTextColour(item, wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT)); break;
	case ItemStatus::Modified: SetItemTextColour(item, colourconfig::colour("modified")); break;
	case ItemStatus::New:      SetItemTextColour(item, colourconfig::colour("new")); break;
	case ItemStatus::Locked:   SetItemTextColour(item, colourconfig::colour("locked")); break;
	case ItemStatus::Error:    SetItemTextColour(item, colourconfig::colour("error")); break;
	case ItemStatus::Disabled: SetItemTextColour(item, disabledColour());
	}

	return true;
}

// -----------------------------------------------------------------------------
// Sets the text of [item] at [column] to [text]
// -----------------------------------------------------------------------------
bool ListView::setItemText(int item, int column, const string& text)
{
	// Check if column is in range
	if (column < 0 || column >= GetColumnCount())
		return false;

	// Check item id is in range
	if (item >= GetItemCount())
		return false;

	// If negative item given, apply to all items
	if (item < 0)
	{
		for (int a = 0; a < GetItemCount(); a++)
			setItemText(a, column, text);

		return true;
	}

	// Set the item text
	wxListItem li;
	li.SetId(item);
	li.SetColumn(column);
	li.SetText(wxString::FromUTF8(text));
	SetItem(li);

	// Update widget size
	updateSize();

	return true;
}

// -----------------------------------------------------------------------------
// Deselects all list items
// -----------------------------------------------------------------------------
void ListView::clearSelection()
{
	for (int a = 0; a < GetItemCount(); a++)
		SetItemState(a, 0x0000, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
}

// -----------------------------------------------------------------------------
// Selects [item]. Sets the focus to [item] if [focus] is true
// -----------------------------------------------------------------------------
bool ListView::selectItem(int item, bool focus)
{
	// Check item id is in range
	if (item >= GetItemCount())
		return false;

	// If negative item given, select all items
	if (item < 0)
	{
		for (int a = 0; a < GetItemCount(); a++)
			SetItemState(a, 0xFFFF, wxLIST_STATE_SELECTED);

		return true;
	}

	// Select the item (and focus if needed)
	if (focus)
		SetItemState(item, 0xFFFF, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
	else
		SetItemState(item, 0xFFFF, wxLIST_STATE_SELECTED);

	return true;
}

// -----------------------------------------------------------------------------
// Deselects [item]
// -----------------------------------------------------------------------------
bool ListView::deSelectItem(int item)
{
	// Check item id is in range
	if (item >= GetItemCount())
		return false;

	// If negative item given, deselect all items
	if (item < 0)
	{
		clearSelection();
		return true;
	}

	// DeSelect the item
	SetItemState(item, 0x0000, wxLIST_STATE_SELECTED);

	return true;
}

// -----------------------------------------------------------------------------
// Returns a list with the indices of all selected items
// -----------------------------------------------------------------------------
wxArrayInt ListView::selectedItems() const
{
	// Init return array
	wxArrayInt ret;

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
		ret.Add(item);
	}

	return ret;
}

// -----------------------------------------------------------------------------
// Ensures [item] can be seen. If not, the list is scrolled so that it is.
// (probably not really needed)
// -----------------------------------------------------------------------------
bool ListView::showItem(int item)
{
	// Check item id is in range
	if (item < 0 || item >= GetItemCount())
		return false;

	EnsureVisible(item);

	return true;
}

// -----------------------------------------------------------------------------
// Swaps [item1] with [item2]
// -----------------------------------------------------------------------------
bool ListView::swapItems(int item1, int item2)
{
	// Check item id's are in range
	if (item1 < 0 || item1 >= GetItemCount() || item2 < 0 || item2 >= GetItemCount())
		return false;

	// Get item 1 info
	wxListItem i1;
	i1.SetId(item1);
	i1.SetMask(0xFFFF);
	GetItem(i1);
	long state1 = GetItemState(item1, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);

	// Get item 2 info
	wxListItem i2;
	i2.SetId(item2);
	i2.SetMask(0xFFFF);
	GetItem(i2);
	long state2 = GetItemState(item2, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);

	// Swap the items
	i1.SetId(item2);
	i2.SetId(item1);
	SetItem(i1);
	SetItem(i2);
	SetItemState(item1, state2, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
	SetItemState(item2, state1, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);

	return true;
}

// -----------------------------------------------------------------------------
// As the default wxListCtrl does a terrible job at autosizing itself, this is
// here and should be called whenever a change is made to the list that could
// change its size
// -----------------------------------------------------------------------------
bool ListView::updateSize()
{
	// Update column widths if enabled
	if (update_width_)
	{
		for (int a = 0; a < GetColumnCount(); a++)
		{
			SetColumnWidth(a, (a == GetColumnCount() - 1 ? wxLIST_AUTOSIZE : wxLIST_AUTOSIZE_USEHEADER));
			int maxsize = GetColumnWidth(a);

			// Autosize column
			SetColumnWidth(a, wxLIST_AUTOSIZE);

			// If header width is larger, use that
			if (maxsize > GetColumnWidth(a))
				SetColumnWidth(a, maxsize);

			// Minimum size of 24
			if (GetColumnWidth(a) < 24)
				SetColumnWidth(a, 24);

			// Add spacing if any
			if (column_spacing_ > 0)
				SetColumnWidth(a, GetColumnWidth(a) + column_spacing_);
		}

		// Add extra width to the first column in linux as wxLIST_AUTOSIZE seems to ignore listitem images on wxGTK
		if (icons_)
		{
#ifdef __WXGTK__
			SetColumnWidth(0, GetColumnWidth(0) + 20);
#endif
		}
	}

	// Get total column width
	int width = 8;
	for (int a = 0; a < GetColumnCount(); a++)
		width += GetColumnWidth(a);

	// Always leave room for the scrollbar (wxWidgets is silly)
	width += wxSystemSettings::GetMetric(wxSYS_VSCROLL_X, this);

	// Set widget size
	SetSizeHints(width, -1);

	return true;
}
