
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
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
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "DefaultEntryPanel.h"
#include "General/Misc.h"
#include "General/SAction.h"
#include "General/UI.h"
#include "MainEditor/EntryOperations.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/ArchivePanel.h"
#include "UI/Dialogs/ModifyOffsetsDialog.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// DefaultEntryPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// DefaultEntryPanel class constructor
// -----------------------------------------------------------------------------
DefaultEntryPanel::DefaultEntryPanel(wxWindow* parent) : EntryPanel(parent, "default")
{
	sizer_main_->AddStretchSpacer(1);

	// Add index label
	label_index_ = new wxStaticText(this, -1, wxS("Index"));
	sizer_main_->Add(label_index_, 0, wxALL | wxALIGN_CENTER, ui::pad());

	// Add type label
	label_type_ = new wxStaticText(this, -1, wxS("Type"));
	sizer_main_->Add(label_type_, 0, wxALL | wxALIGN_CENTER, ui::pad());

	// Add size label
	label_size_ = new wxStaticText(this, -1, wxS("Size"));
	sizer_main_->Add(label_size_, 0, wxALL | wxALIGN_CENTER, ui::pad());

	// Add actions frame
	frame_actions_  = new wxStaticBox(this, -1, wxS("Actions"));
	auto framesizer = new wxStaticBoxSizer(frame_actions_, wxVERTICAL);
	sizer_main_->Add(framesizer, 0, wxALL | wxALIGN_CENTER, ui::pad());

	// Add 'Convert Gfx' button
	btn_gfx_convert_ = new wxButton(frame_actions_, -1, wxS("Convert Gfx To..."));
	framesizer->AddSpacer(4);
	framesizer->Add(btn_gfx_convert_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, ui::pad());

	// Add 'Modify Gfx Offsets' button
	btn_gfx_modify_offsets_ = new wxButton(frame_actions_, -1, wxS("Modify Gfx Offsets"));
	framesizer->Add(btn_gfx_modify_offsets_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, ui::pad());

	// Add 'Edit Textures' button
	btn_texture_edit_ = new wxButton(frame_actions_, -1, wxS("Edit Textures"));
	framesizer->Add(btn_texture_edit_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, ui::pad());

	sizer_main_->AddStretchSpacer(1);

	// Bind events
	btn_gfx_convert_->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { SActionHandler::doAction("arch_gfx_convert"); });
	btn_gfx_modify_offsets_->Bind(wxEVT_BUTTON, &DefaultEntryPanel::onBtnGfxModifyOffsets, this);
	btn_texture_edit_->Bind(
		wxEVT_BUTTON,
		[&](wxCommandEvent&)
		{
			if (auto entry = entry_.lock())
				maineditor::openTextureEditor(entry->parent(), entry.get());
		});

	// Hide save/revert toolbar
	toolbar_->deleteGroup("Entry");
	stb_save_   = nullptr;
	stb_revert_ = nullptr;

	// Setup toolbar
	auto group = new SToolBarGroup(toolbar_, "View As");
	group->addActionButton("arch_view_text", "", true);
	group->addActionButton("arch_view_hex", "", true);
	toolbar_->addGroup(group);

	wxWindowBase::Layout();
}

// -----------------------------------------------------------------------------
// Loads [entry] into the panel
// -----------------------------------------------------------------------------
bool DefaultEntryPanel::loadEntry(ArchiveEntry* entry)
{
	// Update labels
	label_index_->SetLabel(WX_FMT("Entry Index: {}", entry->index()));
	label_type_->SetLabel(WX_FMT("Entry Type: {}", entry->typeString()));
	label_size_->SetLabel(WX_FMT("Entry Size: {} bytes", entry->size()));

	// Setup actions frame
	btn_gfx_convert_->Show(false);
	btn_gfx_modify_offsets_->Show(false);
	btn_texture_edit_->Show(false);
	frame_actions_->Show(false);

	// Check for gfx entry
	if (entry->type()->extraProps().contains("image"))
	{
		frame_actions_->Show(true);
		btn_gfx_convert_->Show(true);
		btn_gfx_modify_offsets_->Show(true);
	}

	// Check for TEXTUREx related entry
	if (entry->type()->id() == "texturex" || entry->type()->id() == "pnames")
	{
		frame_actions_->Show(true);
		btn_texture_edit_->Show(true);
	}

	// Update layout
	Layout();

	return true;
}

// -----------------------------------------------------------------------------
// Loads [entries] into the panel, for multiple selection handling
// -----------------------------------------------------------------------------
bool DefaultEntryPanel::loadEntries(vector<ArchiveEntry*>& entries)
{
	// Update labels
	label_type_->SetLabel(WX_FMT("{} selected entries", static_cast<unsigned long>(entries.size())));
	unsigned size = 0;
	for (auto& entry : entries)
		size += entry->size();
	label_size_->SetLabel(WX_FMT("Total Size: {}", misc::sizeAsString(size)));

	// Setup actions frame
	btn_gfx_convert_->Show(false);
	btn_gfx_modify_offsets_->Show(false);
	btn_texture_edit_->Show(false);
	frame_actions_->Show(false);

	bool gfx     = false;
	bool texture = false;
	entries_.clear();
	size_t max = 0, min = entries[0]->index();
	for (auto& entry : entries)
	{
		// Get index
		size_t index = entry->index();
		if (index < min)
			min = index;
		if (index > max)
			max = index;

		// Check for gfx entry
		if (entry->type()->extraProps().contains("image"))
			gfx = true;

		// Check for TEXTUREx related entry
		if (entry->type()->id() == "texturex" || entry->type()->id() == "pnames")
			texture = true;

		entries_.push_back(entry);
	}
	label_index_->SetLabel(
		WX_FMT("Entry Indices: from {} to {}", static_cast<unsigned long>(min), static_cast<unsigned long>(max)));
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


// -----------------------------------------------------------------------------
//
// DefaultEntryPanel Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the 'Modify Offsets' button is clicked
// -----------------------------------------------------------------------------
void DefaultEntryPanel::onBtnGfxModifyOffsets(wxCommandEvent& e)
{
	// Create and run modify offsets dialog
	ModifyOffsetsDialog mod;
	if (mod.ShowModal() == wxID_CANCEL)
		return;

	// Begin recording undo level
	undo_manager_->beginRecord("Gfx Modify Offsets");

	// Go through selected entries
	for (auto& entry : entries_)
	{
		undo_manager_->recordUndoStep(std::make_unique<EntryDataUS>(entry));
		mod.apply(*entry);
	}
	maineditor::currentEntryPanel()->callRefresh();

	// Finish recording undo level
	undo_manager_->endRecord(true);
}
