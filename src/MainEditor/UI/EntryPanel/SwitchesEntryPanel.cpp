
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    SwitchesEntryPanel.cpp
 * Description: SwitchesEntryPanel class. The UI for editing Boom
 *              SWITCHES lumps.
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
#include "SwitchesEntryPanel.h"
#include "Archive/Archive.h"


/*******************************************************************
 * TEXTENTRYPANEL CLASS FUNCTIONS
 *******************************************************************/

/* SwitchesEntryPanel::SwitchesEntryPanel
 * SwitchesEntryPanel class constructor
 *******************************************************************/
SwitchesEntryPanel::SwitchesEntryPanel(wxWindow* parent)
	: EntryPanel(parent, "switches")
{
	se_current = NULL;
	se_modified = false;

	// Setup toolbar
	SToolBarGroup* group = new SToolBarGroup(toolbar, "Switches");
	group->addActionButton("new_switch", "New Switch", "switch_new", "Create a new switch definition", true);
	toolbar->addGroup(group);

	// Setup panel sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	sizer_main->Add(sizer, 1, wxEXPAND, 0);

	// Add entry list
	wxStaticBox* frame = new wxStaticBox(this, -1, "Switches");
	wxStaticBoxSizer* framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	list_entries = new ListView(this, -1);
	list_entries->showIcons(false);
	framesizer->Add(list_entries, 1, wxEXPAND|wxALL, 4);
	sizer->Add(framesizer, 0, wxEXPAND|wxALL, 4);

	// Add editing controls
	frame = new wxStaticBox(this, -1, "Selection");
	framesizer = new wxStaticBoxSizer(frame, wxHORIZONTAL);

	wxStaticBox* textframe;
	wxStaticBoxSizer* textframesizer;

	textframe = new wxStaticBox(this, -1, "Off frame");
	textframesizer = new wxStaticBoxSizer(textframe, wxVERTICAL);
	text_offname = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxSize(80, -1));
	textframesizer->Add(text_offname, 1, wxTILE, 4);
	framesizer->Add(textframesizer, 1, wxTILE, 4);

	textframe = new wxStaticBox(this, -1, "On frame");
	textframesizer = new wxStaticBoxSizer(textframe, wxVERTICAL);
	text_onname = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxSize(80, -1));
	textframesizer->Add(text_onname, 1, wxTILE, 4);
	framesizer->Add(textframesizer, 1, wxTILE, 4);

	textframe = new wxStaticBox(this, -1, "Range");
	textframesizer = new wxStaticBoxSizer(textframe, wxVERTICAL);
	rbtn_shareware = new wxRadioButton(this, 101, "Shareware", wxDefaultPosition, wxDefaultSize, 0);
	textframesizer->Add(rbtn_shareware, 1, wxTILE, 4);
	rbtn_registered = new wxRadioButton(this, 102, "Registered", wxDefaultPosition, wxDefaultSize, 0);
	textframesizer->Add(rbtn_registered, 1, wxTILE, 4);
	rbtn_commercial = new wxRadioButton(this, 103, "Commercial", wxDefaultPosition, wxDefaultSize, 0);
	textframesizer->Add(rbtn_commercial, 1, wxTILE, 4);
	framesizer->Add(textframesizer, 1, wxTILE, 4);

	// Finish layout
	sizer->Add(framesizer, 0, wxEXPAND|wxALL, 4);

	Layout();

	// Bind events
	list_entries->Bind(wxEVT_LIST_ITEM_SELECTED, &SwitchesEntryPanel::onListSelect, this);
	list_entries->Bind(wxEVT_LIST_ITEM_RIGHT_CLICK, &SwitchesEntryPanel::onListRightClick, this);
	rbtn_shareware->Bind(wxEVT_RADIOBUTTON, &SwitchesEntryPanel::onTypeChanged, this);
	rbtn_registered->Bind(wxEVT_RADIOBUTTON, &SwitchesEntryPanel::onTypeChanged, this);
	rbtn_commercial->Bind(wxEVT_RADIOBUTTON, &SwitchesEntryPanel::onTypeChanged, this);
	text_offname->Bind(wxEVT_TEXT, &SwitchesEntryPanel::onOffNameChanged, this);
	text_onname->Bind(wxEVT_TEXT, &SwitchesEntryPanel::onOnNameChanged, this);
}

/* SwitchesEntryPanel::~SwitchesEntryPanel
 * SwitchesEntryPanel class destructor
 *******************************************************************/
SwitchesEntryPanel::~SwitchesEntryPanel()
{
}

/* SwitchesEntryPanel::handleAction
 * Handles the action [id]. Returns true if the action was handled,
 * false otherwise
 *******************************************************************/
bool SwitchesEntryPanel::handleAction(string id)
{
	// Don't handle actions if hidden
	if (!isActivePanel())
		return false;

	// We're only interested in "anim_" actions
	if (!id.StartsWith("swch_"))
		return false;

	if (id == "swch_new")
	{
		add();
	}

	else if (id == "swch_delete")
	{
		remove();
	}

	else if (id == "swch_up")
	{
		moveUp();
	}

	else if (id == "swch_down")
	{
		moveDown();
	}

	// Unknown action
	else
		return false;

	// Action handled
	return true;

}

/* SwitchesEntryPanel::loadEntry
 * Loads an entry into the SWITCHES entry panel
 *******************************************************************/
bool SwitchesEntryPanel::loadEntry(ArchiveEntry* entry)
{
	// Do nothing if entry is already open
	if (this->entry == entry && !isModified())
		return true;

	// Read SWITCHES entry into list
	switches.clear();
	switches.readSWITCHESData(entry);

	// Update variables
	this->entry = entry;
	setModified(false);

	// Refresh controls
	populateEntryList();
	Layout();
	Refresh();

	return true;
}

/* SwitchesEntryPanel::saveEntry
 * Saves any changes made to the entry
 *******************************************************************/
bool SwitchesEntryPanel::saveEntry()
{
	MemChunk mc;
	mc.seek(0, SEEK_SET);
	switches_t swch;
	for (uint32_t a = 0; a < switches.nEntries(); a++)
	{
		SwitchesEntry* ent = switches.getEntry(a);
		for (size_t i = 0; i < 9; ++i)
		{
			if (ent->getOff().length() > i)
				swch.off[i] = ent->getOff()[i];
			else swch.off[i] = 0;
			if (ent->getOn().length() > i)
				swch.on[i] = ent->getOn()[i];
			else swch.on[i] = 0;
		}
		swch.type = ent->getType();
		mc.write(&swch, 20);
	}
	memset(&swch, 0, 20);
	mc.write(&swch, 20);
	bool success = entry->importMemChunk(mc);
	if (success)
	{
		for (uint32_t a = 0; a < switches.nEntries(); a++)
			list_entries->setItemStatus(a, LV_STATUS_NORMAL);
	}
	return success;
}

/* SwitchesEntryPanel::revertEntry
 * Undoes any changes made to the entry
 *******************************************************************/
bool SwitchesEntryPanel::revertEntry()
{
	ArchiveEntry* reload = entry;
	entry = NULL;
	return loadEntry(reload);
}

/* SwitchesEntryPanel::insertListItem
 * Adds an entry to the list
 *******************************************************************/
void SwitchesEntryPanel::insertListItem(SwitchesEntry* ent, uint32_t pos)
{
	if (ent == NULL) return;
	string cols[] = { ent->getOff(), ent->getOn(),
	                  ent->getType() == SWCH_COMM ? "Commercial"
	                  : ent->getType() == SWCH_FULL ? "Registered"
	                  : ent->getType() == SWCH_DEMO ? "Shareware"
	                  : "BugBugBug"
	                };
	list_entries->addItem(pos, wxArrayString(3, cols));
	list_entries->setItemStatus(pos, ent->getStatus());
}

/* SwitchesEntryPanel::updateListItem
 * Updates an entry to the list
 *******************************************************************/
void SwitchesEntryPanel::updateListItem(SwitchesEntry* ent, uint32_t pos)
{
	if (ent == NULL) return;
	string cols[] = { ent->getOff(), ent->getOn(),
	                  ent->getType() == SWCH_COMM ? "Commercial"
	                  : ent->getType() == SWCH_FULL ? "Registered"
	                  : ent->getType() == SWCH_DEMO ? "Shareware"
	                  : "BugBugBug"
	                };
	for (size_t a = 0; a < 3; ++a)
	{
		list_entries->setItemText(pos, a, cols[a]);
	}
	list_entries->setItemStatus(pos, ent->getStatus());
}

/* SwitchesEntryPanel::populateEntryList
 * Clears and adds all entries to the entry list
 *******************************************************************/
void SwitchesEntryPanel::populateEntryList()
{
	// Clear current list
	list_entries->ClearAll();

	// Add columns
	list_entries->InsertColumn(0, "Off Texture");
	list_entries->InsertColumn(1, "On Texture");
	list_entries->InsertColumn(2, "Range");

	// Add each switch to the list
	list_entries->enableSizeUpdate(false);
	for (uint32_t a = 0; a < switches.nEntries(); a++)
	{
		insertListItem(switches.getEntry(a), a);
	}

	// Update list width
	list_entries->enableSizeUpdate(true);
	list_entries->updateSize();
}

/* SwitchesEntryPanel::applyChanges
 * Saves changes to list
 *******************************************************************/
void SwitchesEntryPanel::applyChanges()
{
	if (se_current == NULL)
		return;

	list_entries->enableSizeUpdate(false);

	se_current->setOff(text_offname->GetValue());
	se_current->setOn(text_onname->GetValue());
	if (rbtn_shareware->GetValue())
		se_current->setType(SWCH_DEMO);
	else if (rbtn_registered->GetValue())
		se_current->setType(SWCH_FULL);
	else if (rbtn_commercial->GetValue())
		se_current->setType(SWCH_COMM);
	else se_current->setType(SWCH_STOP);

	// Find entry
	for (uint32_t a = 0; a < switches.nEntries(); a++)
	{
		if (switches.getEntry(a) == se_current)
		{
			if (se_current->getStatus() == LV_STATUS_NORMAL)
			{
				se_current->setStatus(LV_STATUS_MODIFIED);
			}
			updateListItem(se_current, a);
			break;
		}
	}
	setModified(true);
	list_entries->enableSizeUpdate(true);
}

/* SwitchesEntryPanel::updateControls
 * Update the content of the control fields
 *******************************************************************/
void SwitchesEntryPanel::updateControls()
{
	if (se_current == NULL)
	{
		text_offname->Clear();
		text_onname->Clear();
		rbtn_shareware->SetValue(false);
		rbtn_registered->SetValue(false);
		rbtn_commercial->SetValue(false);
	}
	else
	{
		text_offname->ChangeValue(se_current->getOff());
		text_onname->ChangeValue(se_current->getOn());
		rbtn_shareware->SetValue(se_current->getType() == SWCH_DEMO);
		rbtn_registered->SetValue(se_current->getType() == SWCH_FULL);
		rbtn_commercial->SetValue(se_current->getType() == SWCH_COMM);
	}
}

/* SwitchesEntryPanel::add
 * Insert new switch after selected switches
 *******************************************************************/
void SwitchesEntryPanel::add()
{
	// Get selected switch
	wxArrayInt selection = list_entries->selectedItems();
	uint32_t index = list_entries->GetItemCount();
	if (selection.size() > 0)
		index = selection[selection.size() - 1] + 1;

	// Create new switch
	switches_t swch = { "????????", "????????", SWCH_DEMO };
	SwitchesEntry* se = new SwitchesEntry(swch);
	se->setStatus(LV_STATUS_NEW);

	// Insert it in list
	list_entries->enableSizeUpdate(false);
	switches.addEntry(se, index);
	insertListItem(se, index);
	list_entries->enableSizeUpdate(true);
	list_entries->EnsureVisible(index);

	// Update variables
	setModified(true);
}

/* SwitchesEntryPanel::remove
 * Removes any selected animations
 *******************************************************************/
void SwitchesEntryPanel::remove()
{
	// Get selected animations
	wxArrayInt selection = list_entries->selectedItems();

	// Nothing to do on an empty selection
	if (selection.size() == 0)
		return;

	list_entries->enableSizeUpdate(false);

	// Go through selection backwards
	for (int a = selection.size() - 1; a >= 0; a--)
	{
		// Remove animation from list
		switches.removeEntry(selection[a]);
		list_entries->DeleteItem(selection[a]);
	}

	// Clear selection & refresh
	list_entries->clearSelection();
	list_entries->enableSizeUpdate(true);

	// Update variables
	setModified(true);
}

/* SwitchesEntryPanel::moveUp
 * Moves all selected animations up
 *******************************************************************/
void SwitchesEntryPanel::moveUp()
{
	// Get selected animations
	wxArrayInt selection = list_entries->selectedItems();

	// Do nothing if nothing is selected or if the
	// first selected item is at the top of the list
	if (selection.size() == 0 || selection[0] == 0)
		return;

	list_entries->enableSizeUpdate(false);

	// Go through selection
	for (unsigned a = 0; a < selection.size(); a++)
	{
		// Swap selected animation with the one above it
		switches.swapEntries(selection[a], selection[a] - 1);
		updateListItem(switches.getEntry(selection[a]), selection[a]);
		updateListItem(switches.getEntry(selection[a]-1), selection[a]-1);
	}

	// Update selection
	list_entries->clearSelection();
	for (unsigned a = 0; a < selection.size(); a++)
		list_entries->selectItem(selection[a] - 1);

	// Refresh
	list_entries->enableSizeUpdate(true);
	list_entries->EnsureVisible(selection[0] - 4);

	// Update variables
	setModified(true);
}

/* SwitchesEntryPanel::moveDown
 * Moves all selected animations down
 *******************************************************************/
void SwitchesEntryPanel::moveDown()
{
	// Get selected animations
	wxArrayInt selection = list_entries->selectedItems();

	// Do nothing if nothing is selected or if the
	// last selected item is at the end of the list
	if (selection.size() == 0 || selection.back() == list_entries->GetItemCount()-1)
		return;

	list_entries->enableSizeUpdate(false);

	// Go through selection backwards
	for (int a = selection.size()-1; a >= 0; a--)
	{
		// Swap selected animation with the one below it
		switches.swapEntries(selection[a], selection[a] + 1);
		updateListItem(switches.getEntry(selection[a]), selection[a]);
		updateListItem(switches.getEntry(selection[a]+1), selection[a]+1);
	}

	// Update selection
	list_entries->clearSelection();
	for (unsigned a = 0; a < selection.size(); a++)
		list_entries->selectItem(selection[a] + 1);

	// Refresh
	list_entries->enableSizeUpdate(true);
	list_entries->EnsureVisible(selection[selection.size() - 1] + 3);

	// Update variables
	setModified(true);
}

/* PaletteEntryPanel::toolbarButtonClick
* Called when a (EntryPanel) toolbar button is clicked
*******************************************************************/
void SwitchesEntryPanel::toolbarButtonClick(string action_id)
{
	// New switch
	if (action_id == "new_switch")
	{
		add();
	}
}


/*******************************************************************
 * SWITCHESENTRYPANEL EVENTS
 *******************************************************************/

/* SwitchesEntryPanel::onListSelect
 * Called when an item on the animation list is selected
 *******************************************************************/
void SwitchesEntryPanel::onListSelect(wxListEvent& e)
{
	// Do nothing if multiple animations are selected
	if (list_entries->GetSelectedItemCount() > 1)
	{
		se_current = NULL;
	}
	else
	{
		// Get selected texture
		SwitchesEntry* se = switches.getEntry(e.GetIndex());

		// Save any changes to previous texture
		if (se_modified) applyChanges();

		// Set current texture
		se_current = se;
		se_modified = false;
	}
	// Show relevant information in controls
	updateControls();
}

/* SwitchesEntryPanel::onListRightClick
 * Called when an item on the animation list is right clicked
 *******************************************************************/
void SwitchesEntryPanel::onListRightClick(wxListEvent& e)
{
	// Create context menu
	wxMenu context;
	SAction::fromId("swch_delete")->addToMenu(&context, true);
	SAction::fromId("swch_new")->addToMenu(&context, true);
	context.AppendSeparator();
	SAction::fromId("swch_up")->addToMenu(&context, true);
	SAction::fromId("swch_down")->addToMenu(&context, true);

	// Pop it up
	PopupMenu(&context);
}

/* SwitchesEntryPanel::onTypeChanged
 * Called when the 'flat' and 'texture' radio buttons are toggled
 *******************************************************************/
void SwitchesEntryPanel::onTypeChanged(wxCommandEvent& e)
{
	if ((rbtn_shareware->GetValue()  && se_current->getType() != SWCH_DEMO) ||
	        (rbtn_registered->GetValue() && se_current->getType() != SWCH_FULL) ||
	        (rbtn_commercial->GetValue() && se_current->getType() != SWCH_COMM))
	{
		se_modified = true;
		setModified(true);
	}
}

/* SwitchesEntryPanel::onOffNameChanged
 * Called when the first name entry box is changed
 *******************************************************************/
void SwitchesEntryPanel::onOffNameChanged(wxCommandEvent& e)
{
	// Change texture name
	if (se_current)
	{
		string tmpstr = text_offname->GetValue();
		tmpstr.MakeUpper();
		tmpstr.Truncate(8);
		size_t ip = text_offname->GetInsertionPoint();
		text_offname->ChangeValue(tmpstr);
		text_offname->SetInsertionPoint(ip);
		if (tmpstr.CmpNoCase(se_current->getOff()))
		{
			se_modified = true;
			setModified(true);
		}
	}
}

/* SwitchesEntryPanel::onOnNameChanged
 * Called when the last name entry box is changed
 *******************************************************************/
void SwitchesEntryPanel::onOnNameChanged(wxCommandEvent& e)
{
	// Change texture name
	if (se_current)
	{
		string tmpstr = text_onname->GetValue();
		tmpstr.MakeUpper();
		tmpstr.Truncate(8);
		size_t ip = text_onname->GetInsertionPoint();
		text_onname->ChangeValue(tmpstr);
		text_onname->SetInsertionPoint(ip);
		if (tmpstr.CmpNoCase(se_current->getOn()))
		{
			se_modified = true;
			setModified(true);
		}
	}
}

/* TODO, MAYBE:
 * Allow in-place editing in the list?
 * Add a preview window displaying the texture toggling back and forth?
 * Add a "Convert to ANIMDEFS" button?
 */
