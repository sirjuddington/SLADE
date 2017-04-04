
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    AnimatedEntryPanel.cpp
 * Description: AnimatedEntryPanel class. The UI for editing Boom
 *              ANIMATED lumps.
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
#include "AnimatedEntryPanel.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveManager.h"
#include "General/Misc.h"
#include "UI/Lists/ListView.h"
#include "UI/SToolBar/SToolBar.h"


/*******************************************************************
 * ANIMATEDENTRYPANEL CLASS FUNCTIONS
 *******************************************************************/

/* AnimatedEntryPanel::AnimatedEntryPanel
 * AnimatedEntryPanel class constructor
 *******************************************************************/
AnimatedEntryPanel::AnimatedEntryPanel(wxWindow* parent)
	: EntryPanel(parent, "animated")
{
	ae_current = NULL;
	ae_modified = false;

	// Setup toolbar
	SToolBarGroup* group = new SToolBarGroup(toolbar, "Animated");
	group->addActionButton("new_anim", "New Animation", "animation_new", "Create a new animation definition", true);
	toolbar->addGroup(group);

	// Setup panel sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	sizer_main->Add(sizer, 1, wxEXPAND, 0);

	// Add entry list
	wxStaticBox* frame = new wxStaticBox(this, -1, "Animations");
	wxStaticBoxSizer* framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	list_entries = new ListView(this, -1);
	list_entries->showIcons(false);
	framesizer->Add(list_entries, 1, wxEXPAND|wxALL, 4);
	sizer->Add(framesizer, 0, wxEXPAND|wxALL, 4);

	// Add editing controls
	frame = new wxStaticBox(this, -1, "Selection");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	wxBoxSizer* ctrlsizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* rowsizer = new wxBoxSizer(wxHORIZONTAL);

	wxStaticBox* textframe;
	wxStaticBoxSizer* textframesizer;

	textframe = new wxStaticBox(this, -1, "First frame");
	textframesizer = new wxStaticBoxSizer(textframe, wxHORIZONTAL);
	text_firstname = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxSize(80, -1));
	textframesizer->Add(text_firstname, 1, wxTILE, 4);
	rowsizer->Add(textframesizer, 1, wxTILE, 4);

	textframe = new wxStaticBox(this, -1, "Last frame");
	textframesizer = new wxStaticBoxSizer(textframe, wxHORIZONTAL);
	text_lastname = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxSize(80, -1));
	textframesizer->Add(text_lastname, 1, wxTILE, 4);
	rowsizer->Add(textframesizer, 1, wxTILE, 4);

	textframe = new wxStaticBox(this, -1, "Speed");
	textframesizer = new wxStaticBoxSizer(textframe, wxHORIZONTAL);
	text_speed = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxSize(80, -1));
	textframesizer->Add(text_speed, 1, wxTILE, 4);
	rowsizer->Add(textframesizer, 1, wxTILE, 4);

	ctrlsizer->Add(rowsizer, 1, wxTILE, 4);

	rowsizer = new wxBoxSizer(wxHORIZONTAL);
	textframe = new wxStaticBox(this, -1, "Type");
	textframesizer = new wxStaticBoxSizer(textframe, wxHORIZONTAL);
	rbtn_flat = new wxRadioButton(this, 101, "Flat", wxDefaultPosition, wxDefaultSize, 0);
	rbtn_texture = new wxRadioButton(this, 102, "Texture", wxDefaultPosition, wxDefaultSize, 0);
	cbox_decals = new wxCheckBox(this, -1, "Decals?");
	cbox_swirl = new wxCheckBox(this, -1, "Swirl?");
	textframesizer->Add(rbtn_flat, 0, wxEXPAND|wxALL, 4);
	textframesizer->Add(rbtn_texture, 0, wxEXPAND|wxALL, 4);
	textframesizer->Add(cbox_decals, 0, wxEXPAND|wxALL, 4);
	textframesizer->Add(cbox_swirl, 0, wxEXPAND|wxALL, 4);
	rowsizer->Add(textframesizer, 1, wxTILE, 4);

	ctrlsizer->Add(rowsizer, 1, wxTILE, 4);

	framesizer->Add(ctrlsizer, 1, wxTILE, 4);
	sizer->Add(framesizer, 0, wxEXPAND|wxALL, 4);

	Layout();

	// Bind events
	list_entries->Bind(wxEVT_LIST_ITEM_SELECTED, &AnimatedEntryPanel::onListSelect, this);
	list_entries->Bind(wxEVT_LIST_ITEM_RIGHT_CLICK, &AnimatedEntryPanel::onListRightClick, this);
	rbtn_flat->Bind(wxEVT_RADIOBUTTON, &AnimatedEntryPanel::onTypeChanged, this);
	rbtn_texture->Bind(wxEVT_RADIOBUTTON, &AnimatedEntryPanel::onTypeChanged, this);
	cbox_decals->Bind(wxEVT_CHECKBOX, &AnimatedEntryPanel::onDecalsChanged, this);
	cbox_swirl->Bind(wxEVT_CHECKBOX, &AnimatedEntryPanel::onSwirlChanged, this);
	text_firstname->Bind(wxEVT_TEXT, &AnimatedEntryPanel::onFirstNameChanged, this);
	text_lastname->Bind(wxEVT_TEXT, &AnimatedEntryPanel::onLastNameChanged, this);
	text_speed->Bind(wxEVT_TEXT, &AnimatedEntryPanel::onSpeedChanged, this);
}

/* AnimatedEntryPanel::~AnimatedEntryPanel
 * AnimatedEntryPanel class destructor
 *******************************************************************/
AnimatedEntryPanel::~AnimatedEntryPanel()
{
}

/* AnimatedEntryPanel::handleAction
 * Handles the action [id]. Returns true if the action was handled,
 * false otherwise
 *******************************************************************/
bool AnimatedEntryPanel::handleAction(string id)
{
	// Don't handle actions if hidden
	if (!isActivePanel())
		return false;

	// We're only interested in "anim_" actions
	if (!id.StartsWith("anim_"))
		return false;

	if (id == "anim_new")
	{
		add();
	}

	else if (id == "anim_delete")
	{
		remove();
	}

	else if (id == "anim_up")
	{
		moveUp();
	}

	else if (id == "anim_down")
	{
		moveDown();
	}

	// Unknown action
	else
		return false;

	// Action handled
	return true;

}

/* AnimatedEntryPanel::loadEntry
 * Loads an entry into the ANIMATED entry panel
 *******************************************************************/
bool AnimatedEntryPanel::loadEntry(ArchiveEntry* entry)
{
	// Do nothing if entry is already open
	if (this->entry == entry && !isModified())
		return true;

	// Read ANIMATED entry into texturexlist
	animated.clear();
	animated.readANIMATEDData(entry);

	// Update variables
	this->entry = entry;
	setModified(false);

	// Refresh controls
	populateEntryList();
	Layout();
	Refresh();

	return true;
}

/* AnimatedEntryPanel::saveEntry
 * Saves any changes made to the entry
 *******************************************************************/
bool AnimatedEntryPanel::saveEntry()
{
	MemChunk mc;
	mc.seek(0, SEEK_SET);
	animated_t anim;
	for (uint32_t a = 0; a < animated.nEntries(); a++)
	{
		AnimatedEntry* ent = animated.getEntry(a);
		for (size_t i = 0; i < 9; ++i)
		{
			if (ent->getFirst().length() > i)
				anim.first[i] = ent->getFirst()[i];
			else anim.first[i] = 0;
			if (ent->getLast().length() > i)
				anim.last[i] = ent->getLast()[i];
			else anim.last[i] = 0;
		}
		anim.speed = ent->getSpeed();
		anim.type = ent->getType();
		if (ent->getDecals()) anim.type |= ANIM_DECALS;
		mc.write(&anim, 23);
	}
	anim.type = 255;
	mc.write(&anim, 1);
	bool success = entry->importMemChunk(mc);
	if (success)
	{
		for (uint32_t a = 0; a < animated.nEntries(); a++)
			list_entries->setItemStatus(a, LV_STATUS_NORMAL);
	}
	return success;
}

/* AnimatedEntryPanel::revertEntry
 * Undoes any changes made to the entry
 *******************************************************************/
bool AnimatedEntryPanel::revertEntry()
{
	ArchiveEntry* reload = entry;
	entry = NULL;
	return loadEntry(reload);
}

/* AnimatedEntryPanel::insertListItem
 * Adds an entry to the list
 *******************************************************************/
void AnimatedEntryPanel::insertListItem(AnimatedEntry* ent, uint32_t pos)
{
	if (ent == NULL) return;
	string cols[] = { ent->getType() ? "Texture" : "Flat",
	                  ent->getFirst(), ent->getLast(),
	                  ent->getSpeed() < 65535 ? S_FMT("%d tics", ent->getSpeed()) : "Swirl",
	                  ent->getDecals()? "Allowed" : " "
	                };
	list_entries->addItem(pos, wxArrayString(5, cols));
	list_entries->setItemStatus(pos, ent->getStatus());
}

/* AnimatedEntryPanel::updateListItem
 * Updates an entry to the list
 *******************************************************************/
void AnimatedEntryPanel::updateListItem(AnimatedEntry* ent, uint32_t pos)
{
	if (ent == NULL) return;
	string cols[] = { ent->getType() ? "Texture" : "Flat",
	                  ent->getFirst(), ent->getLast(),
	                  ent->getSpeed() < 65535 ? S_FMT("%d tics", ent->getSpeed()) : "Swirl",
	                  ent->getDecals()? "Allowed" : " "
	                };
	for (size_t a = 0; a < 5; ++a)
	{
		list_entries->setItemText(pos, a, cols[a]);
	}
	list_entries->setItemStatus(pos, ent->getStatus());
}

/* AnimatedEntryPanel::populateEntryList
 * Clears and adds all entries to the entry list
 *******************************************************************/
void AnimatedEntryPanel::populateEntryList()
{
	// Clear current list
	list_entries->ClearAll();

	// Add columns
	list_entries->InsertColumn(0, "Type");
	list_entries->InsertColumn(1, "First frame");
	list_entries->InsertColumn(2, "Last frame");
	list_entries->InsertColumn(3, "Speed");
	list_entries->InsertColumn(4, "Decals");

	// Add each animation to the list
	list_entries->enableSizeUpdate(false);
	for (uint32_t a = 0; a < animated.nEntries(); a++)
		insertListItem(animated.getEntry(a), a);

	// Update list width
	list_entries->enableSizeUpdate(true);
	list_entries->updateSize();
}

/* AnimatedEntryPanel::applyChanges
 * Saves changes to list
 *******************************************************************/
void AnimatedEntryPanel::applyChanges()
{
	if (ae_current == NULL)
		return;

	list_entries->enableSizeUpdate(false);

	ae_current->setFirst(text_firstname->GetValue());
	ae_current->setLast(text_lastname->GetValue());
	long val;
	if (text_speed->GetValue().ToLong(&val))
		ae_current->setSpeed((uint32_t) val);
	if (rbtn_texture->GetValue())
		ae_current->setType(1);
	else ae_current->setType(0);
	ae_current->setDecals(cbox_decals->GetValue());
	if (cbox_swirl->GetValue())
		ae_current->setSpeed(65536);

	// Find entry
	for (uint32_t a = 0; a < animated.nEntries(); a++)
	{
		if (animated.getEntry(a) == ae_current)
		{
			if (ae_current->getStatus() == LV_STATUS_NORMAL)
			{
				ae_current->setStatus(LV_STATUS_MODIFIED);
			}
			updateListItem(ae_current, a);
			break;
		}
	}
	list_entries->enableSizeUpdate(true);
}

/* AnimatedEntryPanel::updateControls
 * Update the content of the control fields
 *******************************************************************/
void AnimatedEntryPanel::updateControls()
{
	if (ae_current == NULL)
	{
		text_firstname->Clear();
		text_lastname->Clear();
		text_speed->Clear();
		rbtn_flat->SetValue(false);
		rbtn_texture->SetValue(false);
		cbox_decals->SetValue(false);
		cbox_swirl->SetValue(false);
	}
	else
	{
		text_firstname->ChangeValue(ae_current->getFirst());
		text_lastname->ChangeValue(ae_current->getLast());
		text_speed->ChangeValue(S_FMT("%d", ae_current->getSpeed()));
		rbtn_flat->SetValue(!ae_current->getType());
		rbtn_texture->SetValue(!!ae_current->getType());
		cbox_decals->SetValue(ae_current->getDecals());
		cbox_swirl->SetValue(ae_current->getSpeed() > 65535);
		if (rbtn_flat->GetValue())
		{
			cbox_decals->Disable();
			cbox_decals->SetValue(false);
		}
		else
		{
			cbox_decals->Enable();
		}
	}
}

/* AnimatedEntryPanel::add
 * Insert new animation after selected animations
 *******************************************************************/
void AnimatedEntryPanel::add()
{
	// Get selected animations
	wxArrayInt selection = list_entries->selectedItems();
	uint32_t index = list_entries->GetItemCount();
	if (selection.size() > 0)
		index = selection[selection.size() - 1] + 1;

	// Create new animation
	animated_t anim = { 0, "????????", "????????", 8 };
	AnimatedEntry* ae = new AnimatedEntry(anim);
	ae->setStatus(LV_STATUS_NEW);

	// Insert it in list
	list_entries->enableSizeUpdate(false);
	animated.addEntry(ae, index);
	insertListItem(ae, index);
	list_entries->enableSizeUpdate(true);
	list_entries->EnsureVisible(index);

	// Update variables
	setModified(true);
}

/* AnimatedEntryPanel::remove
 * Removes any selected animations
 *******************************************************************/
void AnimatedEntryPanel::remove()
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
		animated.removeEntry(selection[a]);
		list_entries->DeleteItem(selection[a]);
	}

	// Clear selection & refresh
	list_entries->clearSelection();
	list_entries->enableSizeUpdate(true);

	// Update variables
	setModified(true);
}

/* AnimatedEntryPanel::moveUp
 * Moves all selected animations up
 *******************************************************************/
void AnimatedEntryPanel::moveUp()
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
		animated.swapEntries(selection[a], selection[a] - 1);
		updateListItem(animated.getEntry(selection[a]), selection[a]);
		updateListItem(animated.getEntry(selection[a]-1), selection[a]-1);
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

/* AnimatedEntryPanel::moveDown
 * Moves all selected animations down
 *******************************************************************/
void AnimatedEntryPanel::moveDown()
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
		animated.swapEntries(selection[a], selection[a] + 1);
		updateListItem(animated.getEntry(selection[a]), selection[a]);
		updateListItem(animated.getEntry(selection[a]+1), selection[a]+1);
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
void AnimatedEntryPanel::toolbarButtonClick(string action_id)
{
	// New animation
	if (action_id == "new_anim")
	{
		add();
	}
}


/*******************************************************************
 * ANIMATEDENTRYPANEL EVENTS
 *******************************************************************/

/* AnimatedEntryPanel::onListSelect
 * Called when an item on the animation list is selected
 *******************************************************************/
void AnimatedEntryPanel::onListSelect(wxListEvent& e)
{
	// Do nothing if multiple animations are selected
	if (list_entries->GetSelectedItemCount() > 1)
	{
		ae_current = NULL;
	}
	else
	{
		// Get selected texture
		AnimatedEntry* ae = animated.getEntry(e.GetIndex());

		// Save any changes to previous texture
		if (ae_modified) applyChanges();

		// Set current texture
		ae_current = ae;
		ae_modified = false;
	}
	// Show relevant information in controls
	updateControls();
}

/* AnimatedEntryPanel::onListRightClick
 * Called when an item on the animation list is right clicked
 *******************************************************************/
void AnimatedEntryPanel::onListRightClick(wxListEvent& e)
{
	// Create context menu
	wxMenu context;
	SAction::fromId("anim_delete")->addToMenu(&context, true);
	SAction::fromId("anim_new")->addToMenu(&context, true);
	context.AppendSeparator();
	SAction::fromId("anim_up")->addToMenu(&context, true);
	SAction::fromId("anim_down")->addToMenu(&context, true);

	// Pop it up
	PopupMenu(&context);
}

/* AnimatedEntryPanel::onTypeChanged
 * Called when the 'flat' and 'texture' radio buttons are toggled
 *******************************************************************/
void AnimatedEntryPanel::onTypeChanged(wxCommandEvent& e)
{
	if (rbtn_texture->GetValue() != !!ae_current->getType())
	{
		ae_modified = true;
		setModified(true);
	}

	// Enable the decals checkbox for textures
	if (rbtn_texture->GetValue())
	{
		cbox_decals->Enable();
		cbox_decals->SetValue(true);
	}
	else
	{
		cbox_decals->Disable();
		cbox_decals->SetValue(false);
	}
}

/* AnimatedEntryPanel::onSwirlChanged
 * Called when the 'swirl' checkbox is toggled
 *******************************************************************/
void AnimatedEntryPanel::onSwirlChanged(wxCommandEvent& e)
{
	if (cbox_swirl->GetValue() != ae_current->getSpeed() > 65535)
	{
		ae_modified = true;
		setModified(true);

		if (cbox_swirl->GetValue())
			ae_current->setSpeed(65536);
		else
			ae_current->setSpeed(8);

		text_speed->ChangeValue(S_FMT("%d", ae_current->getSpeed()));
	}
}

/* AnimatedEntryPanel::onDecalsChanged
 * Called when the 'decals' checkbox is toggled
 *******************************************************************/
void AnimatedEntryPanel::onDecalsChanged(wxCommandEvent& e)
{
	// Only textures can have decals
	if (rbtn_flat->GetValue())
	{
		cbox_decals->SetValue(false);
	}
	if (cbox_decals->GetValue() != ae_current->getDecals())
	{
		ae_modified = true;
		setModified(true);
		ae_current->setDecals(cbox_decals->GetValue());
	}
}

/* AnimatedEntryPanel::onFirstNameChanged
 * Called when the first name entry box is changed
 *******************************************************************/
void AnimatedEntryPanel::onFirstNameChanged(wxCommandEvent& e)
{
	// Change texture name
	if (ae_current)
	{
		string tmpstr = text_firstname->GetValue();
		tmpstr.MakeUpper();
		tmpstr.Truncate(8);
		size_t ip = text_firstname->GetInsertionPoint();
		text_firstname->ChangeValue(tmpstr);
		text_firstname->SetInsertionPoint(ip);
		if (tmpstr.CmpNoCase(ae_current->getFirst()))
		{
			ae_modified = true;
			setModified(true);
		}
	}
}

/* AnimatedEntryPanel::onLastNameChanged
 * Called when the last name entry box is changed
 *******************************************************************/
void AnimatedEntryPanel::onLastNameChanged(wxCommandEvent& e)
{
	// Change texture name
	if (ae_current)
	{
		string tmpstr = text_lastname->GetValue();
		tmpstr.MakeUpper();
		tmpstr.Truncate(8);
		size_t ip = text_lastname->GetInsertionPoint();
		text_lastname->ChangeValue(tmpstr);
		text_lastname->SetInsertionPoint(ip);
		if (tmpstr.CmpNoCase(ae_current->getLast()))
		{
			ae_modified = true;
			setModified(true);
		}
	}
}

/* AnimatedEntryPanel::onSpeedChanged
 * Called when the speed entry box is changed
 *******************************************************************/
void AnimatedEntryPanel::onSpeedChanged(wxCommandEvent& e)
{
	// Change texture name
	if (ae_current)
	{
		string tmpstr = text_speed->GetValue();
		long tmpval;
		if (tmpstr.ToLong(&tmpval) && ae_current->getSpeed() != tmpval)
		{
			//ae_modified = true;
			setModified(true);
			ae_current->setSpeed(tmpval);
		}
		else
		{
			size_t ip = text_speed->GetInsertionPoint();
			text_speed->ChangeValue(S_FMT("%d", ae_current->getSpeed()));
			text_speed->SetInsertionPoint(ip);
		}
	}
}

/* TODO, MAYBE:
 * Allow in-place editing in the list?
 * Make preview window to display the texture or flat cycling at the proper speed?
 * Add a "Convert to ANIMDEFS" button?
 * panel_archivemanager->newEntry();
 */
