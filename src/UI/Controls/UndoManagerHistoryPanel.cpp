
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         https://slade.mancubus.net
// Filename:    UndoManagerHistoryPanel.cpp
// Description: Panel with a list of all undo levels from an UndoManager.
//              The next level to be undone is shown in green while levels that
//              have been undone are shown in grey
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
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#include "UndoManagerHistoryPanel.h"
#include "General/UndoRedo.h"
#include "UI/WxUtils.h"


// ----------------------------------------------------------------------------
//
// UndoListView Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// UndoListView::UndoListView
//
// UndoListView class constructor
// ----------------------------------------------------------------------------
UndoListView::UndoListView(wxWindow* parent, UndoManager* manager) :
	VirtualListView{ parent },
	manager_{ manager }
{
	if (manager)
	{
		SetItemCount(manager->nUndoLevels());
		listenTo(manager);
	}
}

// ----------------------------------------------------------------------------
// UndoListView::getItemText
//
// Returns the list text for [item] at [column]
// ----------------------------------------------------------------------------
string UndoListView::getItemText(long item, long column, long index) const
{
	if (!manager_)
		return "";

	int max = manager_->nUndoLevels();
	if (item < max)
	{
		if (column == 0)
		{
			string name = manager_->undoLevel((unsigned) item)->getName();
			return S_FMT("%lu. %s", item + 1, name);
		}
		else
		{
			return manager_->undoLevel((unsigned) item)->getTimeStamp(false, true);
		}
	}
	else
		return "Invalid Index";
}

// ----------------------------------------------------------------------------
// UndoListView::getItemIcon
//
// Returns the icon index for [item]
// ----------------------------------------------------------------------------
int UndoListView::getItemIcon(long item, long column, long index) const
{
	return -1;
}

// ----------------------------------------------------------------------------
// UndoListView::updateItemAttr
//
// Updates display attributes for [item]
// ----------------------------------------------------------------------------
void UndoListView::updateItemAttr(long item, long column, long index) const
{
	if (!manager_)
		return;

	item_attr->SetTextColour(wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT));

	if (item == manager_->getCurrentIndex())
		item_attr->SetTextColour(WXCOL(rgba_t(0, 170, 0)));
	else if (item > manager_->getCurrentIndex())
		item_attr->SetTextColour(WXCOL(rgba_t(150, 150, 150)));
}

// ----------------------------------------------------------------------------
// UndoListView::setManager
//
// Sets the undo [manager] to show in the list
// ----------------------------------------------------------------------------
void UndoListView::setManager(UndoManager* manager)
{
	if (this->manager_)
		stopListening(this->manager_);

	this->manager_ = manager;
	listenTo(manager);

	updateFromManager();
}

// ----------------------------------------------------------------------------
// UndoListView::onAnnouncement
//
// Called when an announcement is received from the undo manager
// ----------------------------------------------------------------------------
void UndoListView::onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data)
{
	if (announcer != manager_)
		return;

	updateFromManager();
}

// ----------------------------------------------------------------------------
// UndoListView::updateFromManager
//
// Updates visual representation
// ----------------------------------------------------------------------------
void UndoListView::updateFromManager()
{
	SetItemCount(manager_->nUndoLevels());
	Refresh();

	int current_index = manager_->getCurrentIndex();
	if (current_index >= 0)
		EnsureVisible(current_index);
}


// ----------------------------------------------------------------------------
//
// UndoManagerHistoryPanel Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// UndoManagerHistoryPanel::UndoManagerHistoryPanel
//
// UndoManagerHistoryPanel class constructor
// ----------------------------------------------------------------------------
UndoManagerHistoryPanel::UndoManagerHistoryPanel(wxWindow* parent, UndoManager* manager) :
	wxPanel{ parent, -1 },
	manager_{ manager }
{
	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Add undo levels list
	list_levels_ = new UndoListView(this, manager);
	sizer->Add(list_levels_, 1, wxEXPAND|wxALL, UI::pad());

	list_levels_->AppendColumn("Action", wxLIST_FORMAT_LEFT, UI::scalePx(160));
	list_levels_->AppendColumn("Time", wxLIST_FORMAT_RIGHT);
	list_levels_->Bind(wxEVT_LIST_ITEM_RIGHT_CLICK, &UndoManagerHistoryPanel::onItemRightClick, this);
	Bind(wxEVT_MENU, &UndoManagerHistoryPanel::onMenu, this);
}

// ----------------------------------------------------------------------------
// UndoManagerHistoryPanel::setManager
//
// Sets the undo [manager] to display
// ----------------------------------------------------------------------------
void UndoManagerHistoryPanel::setManager(UndoManager* manager)
{
	this->manager_ = manager;
	list_levels_->setManager(manager);
}


// ----------------------------------------------------------------------------
//
// UndoManagerHistoryPanel Class Events
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// UndoManagerHistoryPanel::onItemRightClick
//
// Called when a list item is right clicked
// ----------------------------------------------------------------------------
void UndoManagerHistoryPanel::onItemRightClick(wxCommandEvent& e)
{
	long index = list_levels_->getFocus();
	//wxMessageBox(S_FMT("Item %d", index));

	wxMenu context;
	if (index == manager_->getCurrentIndex())
		context.Append(0, "Undo");
	else if (index < manager_->getCurrentIndex())
		context.Append(1, "Undo To Here");
	else if (index == manager_->getCurrentIndex() + 1)
		context.Append(2, "Redo");
	else
		context.Append(3, "Redo To Here");
	PopupMenu(&context);
}

// ----------------------------------------------------------------------------
// UndoManagerHistoryPanel::onMenu
//
// Called when a context menu item is selected
// ----------------------------------------------------------------------------
void UndoManagerHistoryPanel::onMenu(wxCommandEvent& e)
{
	long index = list_levels_->getFocus();

	if (index <= manager_->getCurrentIndex())
	{
		while (index <= manager_->getCurrentIndex())
			manager_->undo();
	}
	else
	{
		while (manager_->getCurrentIndex() < index)
			manager_->redo();
	}
}
