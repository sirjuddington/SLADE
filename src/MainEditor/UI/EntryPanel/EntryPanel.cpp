
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/EntryType/EntryType.h"
#include "General/SAction.h"
#include "General/UI.h"
#include "General/UndoSteps/EntryDataUS.h"
#include "MainEditor/EntryOperations.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/ArchiveManagerPanel.h"
#include "MainEditor/UI/MainWindow.h"
#include "UI/SToolBar/SToolBar.h"
#include "UI/SToolBar/SToolBarButton.h"
#include "UI/WxUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
wxMenu* menu_entry = nullptr;
}
CVAR(Bool, confirm_entry_revert, true, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// EntryPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// EntryPanel class constructor
// -----------------------------------------------------------------------------
EntryPanel::EntryPanel(wxWindow* parent, const wxString& id, bool left_toolbar) : wxPanel(parent, -1), id_{ id }
{
	namespace wx = wxutil;

	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	wxWindow::Show(false);

	// Add toolbar
	toolbar_ = new SToolBar(this);
	sizer->Add(toolbar_, wx::sfWithMinBorder(0, wxLEFT).Expand());
	sizer->AddSpacer(ui::padMin());

	// Default entry toolbar group
	auto tb_group = new SToolBarGroup(toolbar_, "Entry");
	stb_save_     = tb_group->addActionButton("arch_entry_save", "save", true);
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
		hbox->Add(toolbar_left_, wx::sfWithMinBorder(0, wxRIGHT).Expand());
		hbox->Add(sizer_main_, wxSizerFlags(1).Expand());
		sizer->Add(hbox, wx::sfWithMinBorder(1, wxLEFT).Expand());
	}
	else
		sizer->Add(sizer_main_, wx::sfWithMinBorder(1, wxLEFT).Expand());
	sizer->Add(sizer_bottom_, wx::sfWithBorder(0, wxTOP | wxLEFT).Expand());

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
		maineditor::window()->archiveManagerPanel()->updateEntryTabTitle(entry.get());
	}
}

// -----------------------------------------------------------------------------
// Adds extra border padding to the EntryPanel
// -----------------------------------------------------------------------------
void EntryPanel::addBorderPadding()
{
	Freeze();
	auto* sizer = GetSizer();
	SetSizer(new wxBoxSizer(wxHORIZONTAL), false);
	GetSizer()->AddSpacer(ui::padMin());
	GetSizer()->Add(sizer, wxutil::sfWithBorder(1, wxTOP | wxRIGHT | wxBOTTOM).Expand());
	Layout();
	Thaw();
}

// -----------------------------------------------------------------------------
// 'Opens' the given entry (sets the frame label then loads it)
// -----------------------------------------------------------------------------
bool EntryPanel::openEntry(const ArchiveEntry* entry)
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
	entry_data_.importMem(entry->rawData(), entry->size());

	// Load the entry
	if (loadEntry(entry.get()))
	{
		entry_ = entry;
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
// Saves any changes to the entry
// -----------------------------------------------------------------------------
bool EntryPanel::saveEntry()
{
	if (!modified_)
		return false;

	auto* entry = entry_.lock().get();
	if (!entry)
		return false;

	if (undo_manager_)
	{
		undo_manager_->beginRecord("Save Entry Modifications");
		undo_manager_->recordUndoStep(std::make_unique<EntryDataUS>(entry));
	}

	bool ok = writeEntry(*entry);

	if (undo_manager_)
		undo_manager_->endRecord(ok);

	if (ok)
		setModified(false);

	return ok;
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
			if (undo_manager_)
			{
				undo_manager_->beginRecord("Revert Entry Modifications");
				undo_manager_->recordUndoStep(std::make_unique<EntryDataUS>(entry.get()));
			}

			auto state = entry->state();
			entry->importMemChunk(entry_data_);
			entry->setState(state);
			EntryType::detectEntryType(*entry);
			loadEntry(entry.get());

			if (undo_manager_)
				undo_manager_->endRecord(true);

			// Update archive modified status
			if (auto* archive = entry->parent())
				archive->findModifiedEntries();
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
// Adds this EntryPanel's custom menu to the main window menubar (if it exists).
// If [add_entry_menu] is true, adds an 'Entry' menu to the menubar with some
// basic actions for a standalone entry tab
// -----------------------------------------------------------------------------
void EntryPanel::addCustomMenu(bool add_entry_menu) const
{
	if (add_entry_menu)
	{
		if (!menu_entry)
		{
			menu_entry = new wxMenu();
			SAction::fromId("arch_entry_save")->addToMenu(menu_entry);
			SAction::fromId("arch_entry_rename")->addToMenu(menu_entry);
			SAction::fromId("arch_entry_export")->addToMenu(menu_entry);
		}

		theMainWindow->addCustomMenu(menu_entry, "&Entry");
	}

	if (menu_custom_)
		theMainWindow->addCustomMenu(menu_custom_, custom_menu_name_);
}

// -----------------------------------------------------------------------------
// Removes this EntryPanel's custom menu from the main window menubar
// -----------------------------------------------------------------------------
void EntryPanel::removeCustomMenu() const
{
	if (menu_entry)
		theMainWindow->removeCustomMenu(menu_entry);

	theMainWindow->removeCustomMenu(menu_custom_);
}

// -----------------------------------------------------------------------------
// Returns true if the entry panel is the Archive Manager Panel's current area.
// This is needed because the wx function IsShown() is not enough, it will
// return true if the panel is shown on any tab, even if it is not on the one
// that is selected...
// -----------------------------------------------------------------------------
bool EntryPanel::isActivePanel() const
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
// Handles an action from the 'standalone' Entry menu (when this EntryPanel is
// in its own tab)
// -----------------------------------------------------------------------------
bool EntryPanel::handleStandaloneAction(const string_view id)
{
	// Save
	if (id == "arch_entry_save")
		saveEntry();

	// Rename
	else if (id == "arch_entry_rename")
	{
		// Get entry and parent archive
		auto* entry = entry_.lock().get();
		if (!entry)
			return true;
		auto* archive = entry->parent();
		if (!archive)
			return true;

		// Do rename
		entryoperations::rename({ entry }, archive, true);

		// Update tab title
		maineditor::window()->archiveManagerPanel()->updateEntryTabTitle(entry);
	}

	// Export
	else if (id == "arch_entry_export")
	{
		// Get entry
		auto* entry = entry_.lock().get();
		if (!entry)
			return true;

		// Do export
		entryoperations::exportEntry(entry);
	}

	// Not handled here
	else
		return false;

	return true;
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

	// Revert
	if (button == "revert")
		revertEntry();

	else
		toolbarButtonClick(button);
}
