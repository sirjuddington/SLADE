
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
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
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "EntryPanel.h"
#include "General/UI.h"
#include "Graphics/Icons.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/ArchivePanel.h"
#include "MainEditor/UI/MainWindow.h"
#include "UI/SToolBar/SToolBar.h"
#include "UI/SToolBar/SToolBarButton.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, confirm_entry_revert, true, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// EntryPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// EntryPanel class constructor
// -----------------------------------------------------------------------------
EntryPanel::EntryPanel(wxWindow* parent, const wxString& id, bool left_toolbar) :
	wxPanel(parent, -1), id_{ id }
{
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	wxWindow::Show(false);

	// Add toolbar
	toolbar_     = new SToolBar(this);
	auto pad_min = ui::px(ui::Size::PadMinimum);
	sizer->AddSpacer(pad_min);
	sizer->Add(toolbar_, 0, wxEXPAND | wxLEFT | wxRIGHT, pad_min);
	sizer->AddSpacer(pad_min);

	// Default entry toolbar group
	auto tb_group = new SToolBarGroup(toolbar_, "Entry");
	stb_save_     = tb_group->addActionButton("save", "Save", "save", "Save any changes made to the entry", true);
	stb_revert_ = tb_group->addActionButton("revert", "Revert", "revert", "Revert any changes made to the entry", true);
	toolbar_->addGroup(tb_group);
	toolbar_->enableGroup("Entry", false);

	// Create left toolbar
	if (left_toolbar)
		toolbar_left_ = new SToolBar(this, false, wxVERTICAL);

	// Setup sizer positions
	sizer_bottom_ = new wxBoxSizer(wxHORIZONTAL);
	sizer_main_   = new wxBoxSizer(wxVERTICAL);
	if (left_toolbar)
	{
		auto* hbox = new wxBoxSizer(wxHORIZONTAL);
		hbox->AddSpacer(pad_min);
		hbox->Add(toolbar_left_, 0, wxEXPAND | wxRIGHT, pad_min);
		hbox->Add(sizer_main_, 1, wxEXPAND | wxRIGHT | wxBOTTOM, ui::pad());
		sizer->Add(hbox, 1, wxEXPAND);
	}
	else
		sizer->Add(sizer_main_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, ui::pad());
	sizer->Add(sizer_bottom_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, ui::pad());

	// Bind button events
	Bind(wxEVT_STOOLBAR_BUTTON_CLICKED, &EntryPanel::onToolbarButton, this, toolbar_->GetId());
}

// -----------------------------------------------------------------------------
// EntryPanel class destructor
// -----------------------------------------------------------------------------
EntryPanel::~EntryPanel()
{
	removeCustomMenu();
}

// -----------------------------------------------------------------------------
// Sets the modified flag. If the entry is locked modified will always be false
// -----------------------------------------------------------------------------
void EntryPanel::setModified(bool c)
{
	auto entry = entry_.lock();
	if (!entry)
	{
		modified_ = c;
		return;
	}

	modified_ = entry->isLocked() ? false : c;

	if (stb_save_ && stb_save_->IsEnabled() != modified_)
	{
		toolbar_->enableGroup("Entry", modified_);
		callRefresh();
	}
}

// -----------------------------------------------------------------------------
// 'Opens' the given entry (sets the frame label then loads it)
// -----------------------------------------------------------------------------
bool EntryPanel::openEntry(ArchiveEntry* entry)
{
	return openEntry(entry ? entry->getShared() : nullptr);
}
bool EntryPanel::openEntry(shared_ptr<ArchiveEntry> entry)
{
	entry_data_.clear();
	entry_.reset();

	// Check entry was given
	if (!entry)
		return false;

	// Set unmodified
	setModified(false);

	// Copy current entry content
	entry_data_.clear();
	entry_data_.importMem(entry->rawData(true), entry->size());

	// Load the entry
	Freeze();
	if (loadEntry(entry.get()))
	{
		entry_ = entry;
		updateStatus();
		toolbar_->updateLayout(true);
		Layout();
		Thaw();
		return true;
	}
	else
	{
		theMainWindow->SetStatusText("", 1);
		theMainWindow->SetStatusText("", 2);
		Thaw();
		return false;
	}
}

// -----------------------------------------------------------------------------
// Loads an entry into the entry panel (does nothing here, to be overridden by
// child classes)
// -----------------------------------------------------------------------------
bool EntryPanel::loadEntry(ArchiveEntry* entry)
{
	global::error = "Cannot open an entry with the base EntryPanel class";
	return false;
}

// -----------------------------------------------------------------------------
// Saves the entrypanel content to the entry (does nothing here, to be
// overridden by child classes)
// -----------------------------------------------------------------------------
bool EntryPanel::saveEntry()
{
	global::error = "Cannot save an entry with the base EntryPanel class";
	return false;
}

// -----------------------------------------------------------------------------
// Reverts any changes made to the entry since it was loaded into the editor.
// Returns false if no changes have been made or if the entry data wasn't saved
// -----------------------------------------------------------------------------
bool EntryPanel::revertEntry(bool confirm)
{
	if (modified_ && entry_data_.hasData())
	{
		auto entry = entry_.lock();
		if (!entry)
			return false;

		bool ok = true;

		// Prompt to revert if configured to
		if (confirm_entry_revert && confirm)
			if (wxMessageBox(
					"Are you sure you want to revert changes made to the entry?",
					"Revert Changes",
					wxICON_QUESTION | wxYES_NO)
				== wxNO)
				ok = false;

		if (ok)
		{
			auto state = entry->state();
			entry->importMemChunk(entry_data_);
			entry->setState(state);
			EntryType::detectEntryType(*entry);
			loadEntry(entry.get());
		}

		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Redraws the panel
// -----------------------------------------------------------------------------
void EntryPanel::refreshPanel()
{
	Update();
	Refresh();
}

// -----------------------------------------------------------------------------
// 'Closes' the current entry - clean up, save extra info, etc
// -----------------------------------------------------------------------------
void EntryPanel::closeEntry()
{
	entry_data_.clear();
	entry_.reset();
}

// -----------------------------------------------------------------------------
// Updates the main window status bar with info about the current entry
// -----------------------------------------------------------------------------
void EntryPanel::updateStatus()
{
	// Basic info
	if (auto entry = entry_.lock())
	{
		wxString name = entry->name();
		wxString type = entry->typeString();
		wxString text = wxString::Format("%d: %s, %d bytes, %s", entry->index(), name, entry->size(), type);

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

// -----------------------------------------------------------------------------
// Adds this EntryPanel's custom menu to the main window menubar (if it exists)
// -----------------------------------------------------------------------------
void EntryPanel::addCustomMenu()
{
	if (menu_custom_)
		theMainWindow->addCustomMenu(menu_custom_, custom_menu_name_);
}

// -----------------------------------------------------------------------------
// Removes this EntryPanel's custom menu from the main window menubar
// -----------------------------------------------------------------------------
void EntryPanel::removeCustomMenu() const
{
	theMainWindow->removeCustomMenu(menu_custom_);
}

// -----------------------------------------------------------------------------
// Returns true if the entry panel is the Archive Manager Panel's current area.
// This is needed because the wx function IsShown() is not enough, it will
// return true if the panel is shown on any tab, even if it is not on the one
// that is selected...
// -----------------------------------------------------------------------------
bool EntryPanel::isActivePanel()
{
	return (IsShown() && maineditor::currentEntryPanel() == this);
}

// -----------------------------------------------------------------------------
// Updates the toolbar layout
// -----------------------------------------------------------------------------
void EntryPanel::updateToolbar()
{
	toolbar_->updateLayout(true);
	Layout();
}


// -----------------------------------------------------------------------------
//
// EntryPanel Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when a button on the toolbar is clicked
// -----------------------------------------------------------------------------
void EntryPanel::onToolbarButton(wxCommandEvent& e)
{
	wxString button = e.GetString();

	// Save
	if (button == "save")
	{
		auto entry = entry_.lock();

		if (modified_ && entry)
		{
			if (undo_manager_)
			{
				undo_manager_->beginRecord("Save Entry Modifications");
				undo_manager_->recordUndoStep(std::make_unique<EntryDataUS>(entry.get()));
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
