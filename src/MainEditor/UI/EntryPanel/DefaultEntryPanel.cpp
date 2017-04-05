
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    DefaultEntryPanel.cpp
 * Description: DefaultEntryPanel class. Used for entries that don't
 *              have their own specific editor, or entries of an
 *              unknown type. Has the option to open/edit the entry
 *              as text.
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
#include "DefaultEntryPanel.h"
#include "Archive/ArchiveManager.h"
#include "Dialogs/ModifyOffsetsDialog.h"
#include "General/Misc.h"
#include "MainEditor/EntryOperations.h"
#include "MainEditor/MainEditor.h"
#include "General/SAction.h"


/*******************************************************************
 * DEFAULTENTRYPANEL CLASS FUNCTIONS
 *******************************************************************/

/* DefaultEntryPanel::DefaultEntryPanel
 * DefaultEntryPanel class constructor
 *******************************************************************/
DefaultEntryPanel::DefaultEntryPanel(wxWindow* parent)
	: EntryPanel(parent, "default")
{
	sizer_main->AddStretchSpacer(1);

	// Add index label
	label_index = new wxStaticText(this, -1, "Index");
	sizer_main->Add(label_index, 0, wxALL|wxALIGN_CENTER, 4);

	// Add type label
	label_type = new wxStaticText(this, -1, "Type");
	sizer_main->Add(label_type, 0, wxALL|wxALIGN_CENTER, 4);

	// Add size label
	label_size = new wxStaticText(this, -1, "Size");
	sizer_main->Add(label_size, 0, wxALL|wxALIGN_CENTER, 4);

	// Add actions frame
	frame_actions = new wxStaticBox(this, -1, "Actions");
	wxStaticBoxSizer* framesizer = new wxStaticBoxSizer(frame_actions, wxVERTICAL);
	sizer_main->Add(framesizer, 0, wxALL|wxALIGN_CENTER, 4);

	// Add 'Convert Gfx' button
	btn_gfx_convert = new wxButton(this, -1, "Convert Gfx To...");
	framesizer->AddSpacer(4);
	framesizer->Add(btn_gfx_convert, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Add 'Modify Gfx Offsets' button
	btn_gfx_modify_offsets = new wxButton(this, -1, "Modify Gfx Offsets");
	framesizer->Add(btn_gfx_modify_offsets, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Add 'Edit Textures' button
	btn_texture_edit = new wxButton(this, -1, "Edit Textures");
	framesizer->Add(btn_texture_edit, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	sizer_main->AddStretchSpacer(1);

	// Bind events
	btn_gfx_convert->Bind(wxEVT_BUTTON, &DefaultEntryPanel::onBtnGfxConvert, this);
	btn_gfx_modify_offsets->Bind(wxEVT_BUTTON, &DefaultEntryPanel::onBtnGfxModifyOffsets, this);
	btn_texture_edit->Bind(wxEVT_BUTTON, &DefaultEntryPanel::onBtnTextureEdit, this);

	// Hide save/revert toolbar
	toolbar->deleteGroup("Entry");

	// Setup toolbar
	SToolBarGroup* group = new SToolBarGroup(toolbar, "View As");
	group->addActionButton("arch_view_text", "", true);
	group->addActionButton("arch_view_hex", "", true);
	toolbar->addGroup(group);

	Layout();
}

/* DefaultEntryPanel::~DefaultEntryPanel
 * DefaultEntryPanel class destructor
 *******************************************************************/
DefaultEntryPanel::~DefaultEntryPanel()
{
}

/* DefaultEntryPanel::loadEntry
 * Loads [entry] into the panel
 *******************************************************************/
bool DefaultEntryPanel::loadEntry(ArchiveEntry* entry)
{
	// Update labels
	label_index->SetLabel(S_FMT("Entry Index: %d", entry->getParentDir()->entryIndex(entry)));
	label_type->SetLabel(S_FMT("Entry Type: %s", entry->getTypeString()));
	label_size->SetLabel(S_FMT("Entry Size: %d bytes", entry->getSize()));

	// Setup actions frame
	btn_gfx_convert->Show(false);
	btn_gfx_modify_offsets->Show(false);
	btn_texture_edit->Show(false);
	frame_actions->Show(false);

	// Check for gfx entry
	if (entry->getType()->extraProps().propertyExists("image"))
	{
		frame_actions->Show(true);
		btn_gfx_convert->Show(true);
		btn_gfx_modify_offsets->Show(true);
	}

	// Check for TEXTUREx related entry
	if (entry->getType()->getId() == "texturex" || entry->getType()->getId() == "pnames")
	{
		frame_actions->Show(true);
		btn_texture_edit->Show(true);
	}

	// Update layout
	Layout();

	return true;
}

/* DefaultEntryPanel::loadEntries
 * Loads [entries] into the panel, for multiple selection handling
 *******************************************************************/
bool DefaultEntryPanel::loadEntries(vector<ArchiveEntry*>& entries)
{
	// Update labels
	label_type->SetLabel(S_FMT("%lu selected entries", entries.size()));
	unsigned size = 0;
	for (unsigned a = 0; a < entries.size(); a++)
		size += entries[a]->getSize();
	label_size->SetLabel(S_FMT("Total Size: %s", Misc::sizeAsString(size)));

	// Setup actions frame
	btn_gfx_convert->Show(false);
	btn_gfx_modify_offsets->Show(false);
	btn_texture_edit->Show(false);
	frame_actions->Show(false);

	bool gfx = false;
	bool texture = false;
	this->entries.clear();
	size_t max = 0, min = entries[0]->getParentDir()->entryIndex(entries[0]);
	for (unsigned a = 0; a < entries.size(); a++)
	{
		// Get index
		size_t index = entries[a]->getParentDir()->entryIndex(entries[a]);
		if (index < min)	min = index;
		if (index > max)	max = index;

		// Check for gfx entry
		if (entries[a]->getType()->extraProps().propertyExists("image"))
			gfx = true;

		// Check for TEXTUREx related entry
		if (entries[a]->getType()->getId() == "texturex" || entries[a]->getType()->getId() == "pnames")
			texture = true;

		this->entries.push_back(entries[a]);
	}
	label_index->SetLabel(S_FMT("Entry Indices: from %lu to %lu", (unsigned long) min, (unsigned long) max));
	if (gfx)
	{
		frame_actions->Show(true);
		btn_gfx_convert->Show(true);
		btn_gfx_modify_offsets->Show(true);
	}
	if (texture)
	{
		frame_actions->Show(true);
		btn_texture_edit->Show(true);
	}

	// Update layout
	Layout();

	return true;
}

/* DefaultEntryPanel::saveEntry
 * Saves any changes to the entry
 *******************************************************************/
bool DefaultEntryPanel::saveEntry()
{
	return true;
}


/*******************************************************************
 * DEFAULTENTRYPANEL CLASS EVENTS
 *******************************************************************/

/* DefaultEntryPanel::onBtnGfxConvert
 * Called when the 'Convert Gfx To' button is clicked
 *******************************************************************/
void DefaultEntryPanel::onBtnGfxConvert(wxCommandEvent& e)
{
	SActionHandler::doAction("arch_gfx_convert");
}

/* DefaultEntryPanel::onBtnGfxModifyOffsets
 * Called when the 'Modify Offsets' button is clicked
 *******************************************************************/
void DefaultEntryPanel::onBtnGfxModifyOffsets(wxCommandEvent& e)
{
	// Create and run modify offsets dialog
	ModifyOffsetsDialog mod;
	if (mod.ShowModal() == wxID_CANCEL)
		return;

	// Apply offsets to selected entries
	for (uint32_t a = 0; a < entries.size(); a++)
		EntryOperations::modifyGfxOffsets(entries[a], &mod);
	MainEditor::currentEntryPanel()->callRefresh();
}

/* DefaultEntryPanel::onBtnTextureEdit
 * Called when the 'Edit Textures' button is clicked
 *******************************************************************/
void DefaultEntryPanel::onBtnTextureEdit(wxCommandEvent& e)
{
	MainEditor::openTextureEditor(entry->getParent(), entry);
}
