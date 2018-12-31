
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    EntryPanel.cpp
// Description: EntryPanel class. Different UI panels for editing different
//              entry types extend from this class.
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
#include "EntryPanel.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/MainWindow.h"
#include "MainEditor/UI/ArchivePanel.h"
#include "UI/SToolBar/SToolBar.h"
#include "UI/SToolBar/SToolBarButton.h"
#include "General/UI.h"


// ----------------------------------------------------------------------------
//
// Variables
//
// ----------------------------------------------------------------------------
CVAR(Bool, confirm_entry_revert, true, CVAR_SAVE)


// ----------------------------------------------------------------------------
//
// EntryPanel Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// EntryPanel::EntryPanel
//
// EntryPanel class constructor
// ----------------------------------------------------------------------------
EntryPanel::EntryPanel(wxWindow* parent, string id) :
	wxPanel(parent, -1),
	id_{ id }
{
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Create & set sizer & border
	frame_ = new wxStaticBox(this, -1, "Entry Contents");
	wxStaticBoxSizer* framesizer = new wxStaticBoxSizer(frame_, wxVERTICAL);
	sizer->Add(framesizer, 1, wxEXPAND | wxALL, UI::pad());
	Show(false);

	// Add toolbar
	toolbar_ = new SToolBar(this);
	toolbar_->drawBorder(false);
	framesizer->Add(toolbar_, 0, wxEXPAND | wxLEFT | wxRIGHT, UI::pad());
	framesizer->AddSpacer(UI::px(UI::Size::PadMinimum));

	// Default entry toolbar group
	SToolBarGroup* tb_group = new SToolBarGroup(toolbar_, "Entry");
	stb_save_ = tb_group->addActionButton("save", "Save", "save", "Save any changes made to the entry", true);
	stb_revert_ = tb_group->addActionButton("revert", "Revert", "revert", "Revert any changes made to the entry", true);
	toolbar_->addGroup(tb_group);
	toolbar_->enableGroup("Entry", false);

	// Setup sizer positions
	sizer_bottom_ = new wxBoxSizer(wxHORIZONTAL);
	sizer_main_ = new wxBoxSizer(wxVERTICAL);
	framesizer->Add(sizer_main_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, UI::pad());
	framesizer->Add(sizer_bottom_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, UI::pad());

	// Bind button events
	Bind(wxEVT_STOOLBAR_BUTTON_CLICKED, &EntryPanel::onToolbarButton, this, toolbar_->GetId());
}

// ----------------------------------------------------------------------------
// EntryPanel::~EntryPanel
//
// EntryPanel class destructor
// ----------------------------------------------------------------------------
EntryPanel::~EntryPanel()
{
	removeCustomMenu();
}

// ----------------------------------------------------------------------------
// EntryPanel::setModified
//
// Sets the modified flag. If the entry is locked modified will always be false
// ----------------------------------------------------------------------------
void EntryPanel::setModified(bool c)
{
	if (!entry_)
	{
		modified_ = c;
		return;
	}
	else
	{
		if (entry_->isLocked())
			modified_ = false;
		else
			modified_ = c;
	}

	if (stb_save_ && stb_save_->IsEnabled() != modified_)
	{
		toolbar_->enableGroup("Entry", modified_);
		callRefresh();
	}
}

// ----------------------------------------------------------------------------
// EntryPanel::openEntry
//
// 'Opens' the given entry (sets the frame label then loads it)
// ----------------------------------------------------------------------------
bool EntryPanel::openEntry(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
	{
		entry_data_.clear();
		this->entry_ = nullptr;
		return false;
	}

	// Set unmodified
	setModified(false);

	// Copy current entry content
	entry_data_.clear();
	entry_data_.importMem(entry->getData(true), entry->getSize());

	// Load the entry
	if (loadEntry(entry))
	{
		this->entry_ = entry;
		updateStatus();
		toolbar_->updateLayout(true);
		Layout();
		return true;
	}
	else
	{
		theMainWindow->SetStatusText("", 1);
		theMainWindow->SetStatusText("", 2);
		return false;
	}
}

// ----------------------------------------------------------------------------
// EntryPanel::loadEntry
//
// Loads an entry into the entry panel (does nothing here, to be overridden by
// child classes)
// ----------------------------------------------------------------------------
bool EntryPanel::loadEntry(ArchiveEntry* entry)
{
	Global::error = "Cannot open an entry with the base EntryPanel class";
	return false;
}

// ----------------------------------------------------------------------------
// EntryPanel::saveEntry
//
// Saves the entrypanel content to the entry (does nothing here, to be
// overridden by child classes)
// ----------------------------------------------------------------------------
bool EntryPanel::saveEntry()
{
	Global::error = "Cannot save an entry with the base EntryPanel class";
	return false;
}

// ----------------------------------------------------------------------------
// EntryPanel::revertEntry
//
// Reverts any changes made to the entry since it was loaded into the editor.
// Returns false if no changes have been made or if the entry data wasn't saved
// ----------------------------------------------------------------------------
bool EntryPanel::revertEntry(bool confirm)
{
	if (modified_ && entry_data_.hasData())
	{
		bool ok = true;

		// Prompt to revert if configured to
		if (confirm_entry_revert && confirm)
			if (wxMessageBox("Are you sure you want to revert changes made to the entry?", "Revert Changes", wxICON_QUESTION | wxYES_NO) == wxNO)
				ok = false;

		if (ok)
		{
			uint8_t state = entry_->getState();
			entry_->importMemChunk(entry_data_);
			entry_->setState(state);
			EntryType::detectEntryType(entry_);
			loadEntry(entry_);
		}

		return true;
	}

	return false;
}

// ----------------------------------------------------------------------------
// EntryPanel::refreshPanel
//
// Redraws the panel
// ----------------------------------------------------------------------------
void EntryPanel::refreshPanel()
{
	Update();
	Refresh();
}

// ----------------------------------------------------------------------------
// EntryPanel::closeEntry
//
// 'Closes' the current entry - clean up, save extra info, etc
// ----------------------------------------------------------------------------
void EntryPanel::closeEntry()
{
	entry_data_.clear();
	this->entry_ = nullptr;
}

// ----------------------------------------------------------------------------
// EntryPanel::updateStatus
//
// Updates the main window status bar with info about the current entry
// ----------------------------------------------------------------------------
void EntryPanel::updateStatus()
{
	// Basic info
	if (entry_)
	{
		string name = entry_->getName();
		string type = entry_->getTypeString();
		string text = S_FMT(
			"%d: %s, %d bytes, %s",
			entry_->getParentDir()->entryIndex(entry_),
			name,
			entry_->getSize(),
			type
		);

		theMainWindow->CallAfter(&MainWindow::SetStatusText, text, 1);

		// Extended info
		theMainWindow->CallAfter(&MainWindow::SetStatusText, statusString(), 2);
	}
	else
	{
		// Clear status
		theMainWindow->CallAfter(&MainWindow::SetStatusText, "", 1);
		theMainWindow->CallAfter(&MainWindow::SetStatusText, "", 2);
	}
}

// ----------------------------------------------------------------------------
// EntryPanel::addCustomMenu
//
// Adds this EntryPanel's custom menu to the main window menubar (if it exists)
// ----------------------------------------------------------------------------
void EntryPanel::addCustomMenu()
{
	if (menu_custom_)
		theMainWindow->addCustomMenu(menu_custom_, custom_menu_name_);
}

// ----------------------------------------------------------------------------
// EntryPanel::removeCustomMenu
//
// Removes this EntryPanel's custom menu from the main window menubar
// ----------------------------------------------------------------------------
void EntryPanel::removeCustomMenu()
{
	theMainWindow->removeCustomMenu(menu_custom_);
}

// ----------------------------------------------------------------------------
// EntryPanel::isActivePanel
//
// Returns true if the entry panel is the Archive Manager Panel's current area.
// This is needed because the wx function IsShown() is not enough, it will
// return true if the panel is shown on any tab, even if it is not on the one
// that is selected...
// ----------------------------------------------------------------------------
bool EntryPanel::isActivePanel()
{
	return (IsShown() && MainEditor::currentEntryPanel() == this);
}

// ----------------------------------------------------------------------------
// EntryPanel::updateToolbar
//
// Updates the toolbar layout
// ----------------------------------------------------------------------------
void EntryPanel::updateToolbar()
{
	toolbar_->updateLayout(true);
	Layout();
}


// ----------------------------------------------------------------------------
//
// EntryPanel Class Events
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// EntryPanel::onToolbarButton
//
// Called when a button on the toolbar is clicked
// ----------------------------------------------------------------------------
void EntryPanel::onToolbarButton(wxCommandEvent& e)
{
	string button = e.GetString();

	// Save
	if (button == "save")
	{
		if (modified_)
		{
			if (undo_manager_)
			{
				undo_manager_->beginRecord("Save Entry Modifications");
				undo_manager_->recordUndoStep(new EntryDataUS(entry_));
			}

			if (saveEntry())
			{
				modified_ = false;
				if (undo_manager_)
					undo_manager_->endRecord(true);
			}
			else if (undo_manager_)
				undo_manager_->endRecord(false);
		}
	}

	// Revert
	else if (button == "revert")
		revertEntry();

	else
		toolbarButtonClick(button);
}
