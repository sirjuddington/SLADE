
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SDataViewCtrl.cpp
// Description: wxDataViewCtrl extension that handles some common extra
//              functionality such as typing to search (in Windows), improved
//              multiple selection (again in Windows), column visibility menus,
//              etc.
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
#include "SDataViewCtrl.h"
#include "UI/State.h"
#include "UI/WxUtils.h"
#include "Utility/StringUtils.h"

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
wxDEFINE_EVENT(EVT_SDVC_COLUMN_RESIZED, wxDataViewEvent);

vector<int> real_chars = {
	'.', ',', '_', '-', '+', '=', '`',  '~', '!', '@', '#', '$', '(',  ')',  '[',
	']', '{', '}', ':', ';', '/', '\\', '<', '>', '?', '^', '&', '\'', '\"',
};


// -----------------------------------------------------------------------------
//
// SDataViewCtrl Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// SDataViewCtrl class constructor
// -----------------------------------------------------------------------------
SDataViewCtrl::SDataViewCtrl(wxWindow* parent, long style) : wxDataViewCtrl(parent, -1, {}, {}, style)
{
	Bind(
		wxEVT_IDLE,
		[this](wxIdleEvent&)
		{
			auto max     = GetColumnPosition(lastVisibleColumn()) - 1; // Ignore last column - is stretched
			auto resized = false;
			for (int i = 0; i <= max; ++i)
			{
				auto col   = GetColumn(i);
				auto index = col->GetModelColumn();

				if (!col->IsHidden() && col->GetWidth() != column_widths_[index])
				{
					column_widths_[index] = col->GetWidth();
					onColumnResized(col);
					resized = true;
					wxDataViewEvent de{ EVT_SDVC_COLUMN_RESIZED, this, col };
					ProcessWindowEvent(de);
				}
			}

			if (resized)
				onAnyColumnResized();
		});

#ifdef __WXMSW__
	// Keypress event
	Bind(
		wxEVT_CHAR,
		[this](wxKeyEvent& e)
		{
			// Custom handling for shift+up/down
			if (e.ShiftDown())
			{
				int from_row = multi_select_base_index_;

				// Get row to select to
				// TODO: Handle PgUp/PgDn as well?
				int to_row;
				switch (e.GetKeyCode())
				{
				case WXK_DOWN: to_row = GetRowByItem(GetCurrentItem()) + 1; break;
				case WXK_UP:   to_row = GetRowByItem(GetCurrentItem()) - 1; break;
				default:
					// Not up or down arrow, do default handling
					e.Skip();
					return;
				}

				// Get new item to focus
				auto new_current_item = GetItemByRow(to_row);
				if (!new_current_item.IsOk())
				{
					e.Skip();
					return;
				}

				// Ensure valid range
				if (from_row > to_row)
					std::swap(from_row, to_row);

				// Get items to select
				wxDataViewItemArray items;
				for (int i = from_row; i <= to_row; ++i)
					items.Add(GetItemByRow(i));

				// Set new selection
				SetSelections(items);
				SetCurrentItem(new_current_item);

				// Trigger selection change event
				wxDataViewEvent de;
				de.SetEventType(wxEVT_DATAVIEW_SELECTION_CHANGED);
				ProcessWindowEvent(de);

				return;
			}

			// Search
			if (e.GetModifiers() == 0 && search_model_column_ >= 0)
			{
				if (searchChar(e.GetKeyCode()))
					return;
			}

			e.Skip();
		});

	Bind(
		wxEVT_DATAVIEW_SELECTION_CHANGED,
		[this](wxDataViewEvent& e)
		{
			if (GetSelectedItemsCount() == 1)
				multi_select_base_index_ = GetRowByItem(GetSelection());

			// Clear search string if selection change wasn't a result of searching
			if (e.GetString().Cmp("search"))
				search_.clear();

			e.Skip();
		});

	Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { search_.clear(); });
#endif
}

// -----------------------------------------------------------------------------
// Returns the last visible column
// -----------------------------------------------------------------------------
wxDataViewColumn* SDataViewCtrl::lastVisibleColumn() const
{
	for (int i = static_cast<int>(GetColumnCount()) - 1; i >= 0; --i)
		if (!GetColumn(i)->IsHidden())
			return GetColumn(i);

	return nullptr;
}

// -----------------------------------------------------------------------------
// Resets sorting to the default state (ie. no columns selected for sorting)
// -----------------------------------------------------------------------------
void SDataViewCtrl::resetSorting()
{
	for (unsigned c = 0; c < GetColumnCount(); ++c)
		if (GetColumn(c)->IsSortKey())
			GetColumn(c)->UnsetAsSortKey();
#ifdef __WXGTK__
	GetColumn(0)->SetSortOrder(true);
#else
	if (GetColumn(0)->IsSortKey())
		GetColumn(0)->UnsetAsSortKey();
#endif
	GetModel()->Resort();

	wxDataViewEvent de;
	de.SetEventType(wxEVT_DATAVIEW_COLUMN_SORTED);
	ProcessWindowEvent(de);
}

// -----------------------------------------------------------------------------
// Adds a check item to [menu] for showing/hiding a column [col_model]
// -----------------------------------------------------------------------------
void SDataViewCtrl::appendColumnToggleItem(wxMenu& menu, int col_model) const
{
	for (unsigned i = 0; i < GetColumnCount(); ++i)
	{
		auto* col = GetColumn(i);
		if (col->GetModelColumn() == col_model)
		{
			menu.AppendCheckItem(col_model, col->GetTitle(), wxString::Format("Show the %s column", col->GetTitle()))
				->Check(col->IsShown());

			return;
		}
	}
}

// -----------------------------------------------------------------------------
// Toggles visibility of column [col_model], saving the result to UI state
// property [state_prop]
// -----------------------------------------------------------------------------
void SDataViewCtrl::toggleColumnVisibility(int col_model, string_view state_prop) const
{
	auto* column = GetColumn(GetModelColumnIndex(col_model));

	column->SetHidden(!column->IsHidden());

	if (!state_prop.empty())
		saveStateBool(state_prop, column->IsShown());
}

// -----------------------------------------------------------------------------
// Sets [column]'s [width]
// -----------------------------------------------------------------------------
void SDataViewCtrl::setColumnWidth(wxDataViewColumn* column, int width) const
{
	if (!column || column->IsHidden())
		return;

	column->SetWidth(column == lastVisibleColumn() ? 0 : width);
}

#ifdef __WXMSW__
// -----------------------------------------------------------------------------
// Beginning from [index_start], finds and selects the first item which matches
// the internal search_ string on search_model_column_.
// Returns true if a match was found
// -----------------------------------------------------------------------------
bool SDataViewCtrl::lookForSearchItemFrom(int index_start)
{
	long               index = index_start;
	wxVariant          value;
	wxString           val_string;
	wxDataViewIconText val_dvit;

	while (true)
	{
		auto item = GetItemByRow(index);
		if (!item.IsOk())
			break;

		// Get column text
		GetModel()->GetValue(value, item, search_model_column_);
		if (value.GetType() == "string")
			val_string = value.GetString();
		else if (value.GetType() == "wxDataViewIconText")
		{
			val_dvit << value;
			val_string = val_dvit.GetText();
		}

		if (strutil::startsWithCI(wxutil::strToView(val_string), search_))
		{
			// Matches, update selection+focus
			wxDataViewItemArray items;
			items.Add(item);
			SetSelections(items);
			SetCurrentItem(item);
			EnsureVisible(item);
			return true;
		}

		++index;
	}

	// Didn't get any match
	return false;
}

// -----------------------------------------------------------------------------
// Adds [key_code] to the current internal search string (if valid) and performs
// quick search.
// Returns false if the key was not a 'real' character usable for searching
// -----------------------------------------------------------------------------
bool SDataViewCtrl::searchChar(int key_code)
{
	// Check the key pressed is actually a character (a-z, 0-9 etc)
	bool real_char = false;
	if (key_code >= 'a' && key_code <= 'z') // Lowercase
		real_char = true;
	else if (key_code >= 'A' && key_code <= 'Z') // Uppercase
		real_char = true;
	else if (key_code >= '0' && key_code <= '9') // Number
		real_char = true;
	else
	{
		for (int elist_char : real_chars)
		{
			if (key_code == elist_char)
			{
				real_char = true;
				break;
			}
		}
	}

	if (!real_char)
	{
		search_.clear();
		return false;
	}

	// Get currently focused item (or first if nothing is focused)
	auto index = GetRowByItem(GetCurrentItem());
	if (index < 0)
		index = 0;

	// Build search string
	search_ += static_cast<char>(key_code);

	// Find matching item, beginning from current item
	// If no match found, try again from the top
	auto found = true;
	if (!lookForSearchItemFrom(index))
		found = lookForSearchItemFrom(0);

	// No match, continue from next item with fresh search string
	if (!found)
	{
		search_.clear();
		search_ += static_cast<char>(key_code);
		found = lookForSearchItemFrom(index + 1);
		if (!found)
			found = lookForSearchItemFrom(0);
	}

	if (found)
	{
		// Trigger selection change event
		wxDataViewEvent de;
		de.SetEventType(wxEVT_DATAVIEW_SELECTION_CHANGED);
		de.SetString("search");
		ProcessWindowEvent(de);
	}

	return true;
}
#endif
