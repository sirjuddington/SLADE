
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    DefaultEntryPanel.cpp
// Description: DefaultEntryPanel class. Used for entries that don't have
//              their own specific editor, or entries of an unknown type.
//              Has the option to open/edit the entry as text.
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
#include "DefaultEntryPanel.h"
#include "Archive/ArchiveManager.h"
#include "Dialogs/ModifyOffsetsDialog.h"
#include "General/Misc.h"
#include "MainEditor/EntryOperations.h"
#include "MainEditor/MainEditor.h"
#include "General/SAction.h"
#include "General/UI.h"


// ----------------------------------------------------------------------------
//
// DefaultEntryPanel Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// DefaultEntryPanel::DefaultEntryPanel
//
// DefaultEntryPanel class constructor
// ----------------------------------------------------------------------------
DefaultEntryPanel::DefaultEntryPanel(wxWindow* parent)
	: EntryPanel(parent, "default")
{
	sizer_main_->AddStretchSpacer(1);

	// Add index label
	label_index_ = new wxStaticText(this, -1, "Index");
	sizer_main_->Add(label_index_, 0, wxALL|wxALIGN_CENTER, UI::pad());

	// Add type label
	label_type_ = new wxStaticText(this, -1, "Type");
	sizer_main_->Add(label_type_, 0, wxALL|wxALIGN_CENTER, UI::pad());

	// Add size label
	label_size_ = new wxStaticText(this, -1, "Size");
	sizer_main_->Add(label_size_, 0, wxALL|wxALIGN_CENTER, UI::pad());

	// Add actions frame
	frame_actions_ = new wxStaticBox(this, -1, "Actions");
	wxStaticBoxSizer* framesizer = new wxStaticBoxSizer(frame_actions_, wxVERTICAL);
	sizer_main_->Add(framesizer, 0, wxALL|wxALIGN_CENTER, UI::pad());

	// Add 'Convert Gfx' button
	btn_gfx_convert_ = new wxButton(this, -1, "Convert Gfx To...");
	framesizer->AddSpacer(4);
	framesizer->Add(btn_gfx_convert_, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, UI::pad());

	// Add 'Modify Gfx Offsets' button
	btn_gfx_modify_offsets_ = new wxButton(this, -1, "Modify Gfx Offsets");
	framesizer->Add(btn_gfx_modify_offsets_, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, UI::pad());

	// Add 'Edit Textures' button
	btn_texture_edit_ = new wxButton(this, -1, "Edit Textures");
	framesizer->Add(btn_texture_edit_, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, UI::pad());

	sizer_main_->AddStretchSpacer(1);

	// Bind events
	btn_gfx_convert_->Bind(wxEVT_BUTTON, &DefaultEntryPanel::onBtnGfxConvert, this);
	btn_gfx_modify_offsets_->Bind(wxEVT_BUTTON, &DefaultEntryPanel::onBtnGfxModifyOffsets, this);
	btn_texture_edit_->Bind(wxEVT_BUTTON, &DefaultEntryPanel::onBtnTextureEdit, this);

	// Hide save/revert toolbar
	toolbar_->deleteGroup("Entry");
	stb_save_ = nullptr;
	stb_revert_ = nullptr;

	// Setup toolbar
	SToolBarGroup* group = new SToolBarGroup(toolbar_, "View As");
	group->addActionButton("arch_view_text", "", true);
	group->addActionButton("arch_view_hex", "", true);
	toolbar_->addGroup(group);

	Layout();
}

// ----------------------------------------------------------------------------
// DefaultEntryPanel::loadEntry
//
// Loads [entry] into the panel
// ----------------------------------------------------------------------------
bool DefaultEntryPanel::loadEntry(ArchiveEntry* entry)
{
	// Update labels
	label_index_->SetLabel(S_FMT("Entry Index: %d", entry->getParentDir()->entryIndex(entry)));
	label_type_->SetLabel(S_FMT("Entry Type: %s", entry->getTypeString()));
	label_size_->SetLabel(S_FMT("Entry Size: %d bytes", entry->getSize()));

	// Setup actions frame
	btn_gfx_convert_->Show(false);
	btn_gfx_modify_offsets_->Show(false);
	btn_texture_edit_->Show(false);
	frame_actions_->Show(false);

	// Check for gfx entry
	if (entry->getType()->extraProps().propertyExists("image"))
	{
		frame_actions_->Show(true);
		btn_gfx_convert_->Show(true);
		btn_gfx_modify_offsets_->Show(true);
	}

	// Check for TEXTUREx related entry
	if (entry->getType()->id() == "texturex" || entry->getType()->id() == "pnames")
	{
		frame_actions_->Show(true);
		btn_texture_edit_->Show(true);
	}

	// Update layout
	Layout();

	return true;
}

// ----------------------------------------------------------------------------
// DefaultEntryPanel::loadEntries
//
// Loads [entries] into the panel, for multiple selection handling
// ----------------------------------------------------------------------------
bool DefaultEntryPanel::loadEntries(vector<ArchiveEntry*>& entries)
{
	// Update labels
	label_type_->SetLabel(S_FMT("%lu selected entries", entries.size()));
	unsigned size = 0;
	for (unsigned a = 0; a < entries.size(); a++)
		size += entries[a]->getSize();
	label_size_->SetLabel(S_FMT("Total Size: %s", Misc::sizeAsString(size)));

	// Setup actions frame
	btn_gfx_convert_->Show(false);
	btn_gfx_modify_offsets_->Show(false);
	btn_texture_edit_->Show(false);
	frame_actions_->Show(false);

	bool gfx = false;
	bool texture = false;
	this->entries_.clear();
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
		if (entries[a]->getType()->id() == "texturex" || entries[a]->getType()->id() == "pnames")
			texture = true;

		this->entries_.push_back(entries[a]);
	}
	label_index_->SetLabel(S_FMT("Entry Indices: from %lu to %lu", (unsigned long) min, (unsigned long) max));
	if (gfx)
	{
		frame_actions_->Show(true);
		btn_gfx_convert_->Show(true);
		btn_gfx_modify_offsets_->Show(true);
	}
	if (texture)
	{
		frame_actions_->Show(true);
		btn_texture_edit_->Show(true);
	}

	// Update layout
	Layout();

	return true;
}

// ----------------------------------------------------------------------------
// DefaultEntryPanel::saveEntry
//
// Saves any changes to the entry
// ----------------------------------------------------------------------------
bool DefaultEntryPanel::saveEntry()
{
	return true;
}


// ----------------------------------------------------------------------------
//
// DefaultEntryPanel Class Events
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// DefaultEntryPanel::onBtnGfxConvert
//
// Called when the 'Convert Gfx To' button is clicked
// ----------------------------------------------------------------------------
void DefaultEntryPanel::onBtnGfxConvert(wxCommandEvent& e)
{
	SActionHandler::doAction("arch_gfx_convert");
}

// ----------------------------------------------------------------------------
// DefaultEntryPanel::onBtnGfxModifyOffsets
//
// Called when the 'Modify Offsets' button is clicked
// ----------------------------------------------------------------------------
void DefaultEntryPanel::onBtnGfxModifyOffsets(wxCommandEvent& e)
{
	// Create and run modify offsets dialog
	ModifyOffsetsDialog mod;
	if (mod.ShowModal() == wxID_CANCEL)
		return;

	// Apply offsets to selected entries
	for (uint32_t a = 0; a < entries_.size(); a++)
		EntryOperations::modifyGfxOffsets(entries_[a], &mod);
	MainEditor::currentEntryPanel()->callRefresh();
}

// ----------------------------------------------------------------------------
// DefaultEntryPanel::onBtnTextureEdit
//
// Called when the 'Edit Textures' button is clicked
// ----------------------------------------------------------------------------
void DefaultEntryPanel::onBtnTextureEdit(wxCommandEvent& e)
{
	MainEditor::openTextureEditor(entry_->getParent(), entry_);
}
