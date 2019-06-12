
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    PatchTablePanel.cpp
// Description: The UI for viewing/editing a patch table (PNAMES)
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
#include "PatchTablePanel.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveManager.h"
#include "General/Misc.h"
#include "Graphics/Icons.h"
#include "Graphics/SImage/SImage.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/MainWindow.h"
#include "TextureXEditor.h"
#include "UI/Canvas/GfxCanvas.h"
#include "UI/Controls/PaletteChooser.h"
#include "UI/Controls/SIconButton.h"
#include "UI/Controls/SZoomSlider.h"
#include "UI/WxUtils.h"


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(String, dir_last)


// -----------------------------------------------------------------------------
//
// PatchTableListView Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// PatchTableListView class constructor
// -----------------------------------------------------------------------------
PatchTableListView::PatchTableListView(wxWindow* parent, PatchTable* patch_table) :
	VirtualListView(parent),
	patch_table_{ patch_table }
{
	listenTo(patch_table);

	// Add columns
	InsertColumn(0, "#");
	InsertColumn(1, "Patch Name");
	InsertColumn(2, "Use Count");
	InsertColumn(3, "In Archive");

	// Update list
	PatchTableListView::updateList();

	// Listen to archive manager
	listenTo(&App::archiveManager());
}

// -----------------------------------------------------------------------------
// Returns the string for [item] at [column]
// -----------------------------------------------------------------------------
wxString PatchTableListView::itemText(long item, long column, long index) const
{
	// Check patch table exists
	if (!patch_table_)
		return "INVALID INDEX";

	// Check index is ok
	if (index < 0 || (unsigned)index > patch_table_->nPatches())
		return "INVALID INDEX";

	// Get associated patch
	auto& patch = patch_table_->patch(index);

	if (column == 0) // Index column
		return wxString::Format("%04ld", index);
	else if (column == 1) // Name column
		return patch.name;
	else if (column == 2) // Usage count column
		return wxString::Format("%lu", patch.used_in.size());
	else if (column == 3) // Archive column
	{
		// Get patch entry
		auto entry = patch_table_->patchEntry(index);

		// If patch entry can't be found return invalid
		if (entry)
			return entry->parent()->filename(false);
		else
			return "(!) NOT FOUND";
	}
	else // Invalid column
		return "INVALID COLUMN";
}

// -----------------------------------------------------------------------------
// Updates the item attributes for [item] (red text if patch entry not found,
// default otherwise)
// -----------------------------------------------------------------------------
void PatchTableListView::updateItemAttr(long item, long column, long index) const
{
	// Just set normal text colour
	item_attr_->SetTextColour(wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT));
}

// -----------------------------------------------------------------------------
// Updates + refreshes the patch list
// -----------------------------------------------------------------------------
void PatchTableListView::updateList(bool clear)
{
	if (clear)
		ClearAll();

	// Set list size
	items_.clear();
	if (patch_table_)
	{
		size_t count = patch_table_->nPatches();
		SetItemCount(count);
		for (unsigned a = 0; a < count; a++)
			items_.push_back(a);
	}
	else
		SetItemCount(0);

	sortItems();
	updateWidth();
	Refresh();
}

// -----------------------------------------------------------------------------
// Handles announcements from the panel's PatchTable
// -----------------------------------------------------------------------------
void PatchTableListView::onAnnouncement(Announcer* announcer, string_view event_name, MemChunk& event_data)
{
	// Just refresh on any event from the patch table
	if (announcer == patch_table_)
		updateList();

	if (announcer == &App::archiveManager())
		updateList();
}

// -----------------------------------------------------------------------------
// Returns true if patch at index [left] is used less than [right]
// -----------------------------------------------------------------------------
bool PatchTableListView::usageSort(long left, long right)
{
	auto& p1 = dynamic_cast<PatchTableListView*>(lv_current_)->patchTable()->patch(left);
	auto& p2 = dynamic_cast<PatchTableListView*>(lv_current_)->patchTable()->patch(right);

	if (p1.used_in.size() == p2.used_in.size())
		return left < right;
	else
		return lv_current_->sortDescend() ? p2.used_in.size() < p1.used_in.size() :
											p1.used_in.size() < p2.used_in.size();
}

// -----------------------------------------------------------------------------
// Sorts the list items depending on the current sorting column
// -----------------------------------------------------------------------------
void PatchTableListView::sortItems()
{
	lv_current_ = this;
	if (sort_column_ == 2)
		std::sort(items_.begin(), items_.end(), &PatchTableListView::usageSort);
	else
		std::sort(items_.begin(), items_.end(), &VirtualListView::defaultSort);
}


// -----------------------------------------------------------------------------
//
// PatchTablePanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// PatchTablePanel class constructor
// -----------------------------------------------------------------------------
PatchTablePanel::PatchTablePanel(wxWindow* parent, PatchTable* patch_table, TextureXEditor* tx_editor) :
	wxPanel(parent, -1),
	patch_table_{ patch_table },
	parent_{ tx_editor }
{
	// Create controls
	list_patches_ = new PatchTableListView(this, patch_table);
	list_patches_->setSearchColumn(1); // Want to search by patch name not index
	btn_add_patch_       = new SIconButton(this, "patch_add", "Add Patch");
	btn_patch_from_file_ = new SIconButton(this, "patch_add", "Add Patch from File"); // TODO: Icon
	btn_remove_patch_    = new SIconButton(this, "patch_remove", "Remove Patch");
	btn_change_patch_    = new SIconButton(this, "patch_replace", "Change Patch");
	label_dimensions_    = new wxStaticText(this, -1, "Size: N/A");
	label_textures_      = new wxStaticText(
        this, -1, "In Textures: -", wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);
	patch_canvas_ = new GfxCanvas(this, -1);
	patch_canvas_->setViewType(GfxCanvas::View::Centered);
	patch_canvas_->allowDrag(true);
	patch_canvas_->allowScroll(true);
	slider_zoom_ = new SZoomSlider(this, patch_canvas_);

	setupLayout();

	// Bind events
	btn_add_patch_->Bind(wxEVT_BUTTON, &PatchTablePanel::onBtnAddPatch, this);
	btn_patch_from_file_->Bind(wxEVT_BUTTON, &PatchTablePanel::onBtnPatchFromFile, this);
	btn_remove_patch_->Bind(wxEVT_BUTTON, &PatchTablePanel::onBtnRemovePatch, this);
	btn_change_patch_->Bind(wxEVT_BUTTON, &PatchTablePanel::onBtnChangePatch, this);
	list_patches_->Bind(wxEVT_LIST_ITEM_SELECTED, &PatchTablePanel::onDisplayChanged, this);

	// Palette chooser
	listenTo(theMainWindow->paletteChooser());
}

// -----------------------------------------------------------------------------
// Lays out controls on the panel
// -----------------------------------------------------------------------------
void PatchTablePanel::setupLayout()
{
	auto sizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(sizer);

	// Patches List + actions
	auto frame      = new wxStaticBox(this, -1, "Patch List (PNAMES)");
	auto framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, 0, wxEXPAND | wxALL, UI::pad());
	framesizer->Add(list_patches_, 1, wxEXPAND | wxALL, UI::pad());
	WxUtils::layoutHorizontally(
		framesizer,
		vector<wxObject*>{ btn_add_patch_, btn_patch_from_file_, btn_remove_patch_, btn_change_patch_ },
		wxSizerFlags(0).Border(wxLEFT | wxRIGHT | wxBOTTOM, UI::pad()));

	// Patch preview & info
	frame      = new wxStaticBox(this, -1, "Patch Preview && Info");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, 1, wxEXPAND | wxTOP | wxRIGHT | wxBOTTOM, UI::pad());
	framesizer->Add(slider_zoom_, 0, wxALL, UI::pad());
	framesizer->Add(patch_canvas_->toPanel(this), 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, UI::pad());
	framesizer->Add(label_dimensions_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, UI::pad());
	framesizer->Add(label_textures_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, UI::pad());
}


// -----------------------------------------------------------------------------
//
// PatchTablePanel Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the 'New Patch' button is clicked
// -----------------------------------------------------------------------------
void PatchTablePanel::onBtnAddPatch(wxCommandEvent& e)
{
	// Prompt for new patch name
	auto patch = wxGetTextFromUser("Enter patch entry name:", "Add Patch", wxEmptyString, this);

	// Check something was entered
	if (patch.IsEmpty())
		return;

	// Add to patch table
	patch_table_->addPatch(WxUtils::strToView(patch));

	// Update list
	list_patches_->updateList();
	parent_->pnamesModified(true);
}

// -----------------------------------------------------------------------------
// Called when the 'New Patch from File' button is clicked
// -----------------------------------------------------------------------------
void PatchTablePanel::onBtnPatchFromFile(wxCommandEvent& e)
{
	// Get all entry types
	auto etypes = EntryType::allTypes();

	// Go through types
	wxString ext_filter = "All files (*.*)|*.*|";
	for (auto& etype : etypes)
	{
		// If the type is a valid image type, add its extension filter
		if (etype->extraProps().propertyExists("image"))
		{
			ext_filter += etype->fileFilterString();
			ext_filter += "|";
		}
	}

	// Create open file dialog
	wxFileDialog dialog_open(
		this,
		"Choose file(s) to open",
		dir_last,
		wxEmptyString,
		ext_filter,
		wxFD_OPEN | wxFD_MULTIPLE | wxFD_FILE_MUST_EXIST,
		wxDefaultPosition);

	// Run the dialog & check that the user didn't cancel
	if (dialog_open.ShowModal() == wxID_OK)
	{
		// Get file selection
		wxArrayString files;
		dialog_open.GetPaths(files);

		// Save 'dir_last'
		dir_last = WxUtils::strToView(dialog_open.GetDirectory());

		// Go through file selection
		for (const auto& file : files)
		{
			// Load the file into a temporary ArchiveEntry
			auto entry = std::make_shared<ArchiveEntry>();
			entry->importFile(file.ToStdString());

			// Determine type
			EntryType::detectEntryType(entry.get());

			// If it's not a valid image type, ignore this file
			if (!entry->type()->extraProps().propertyExists("image"))
			{
				Log::warning(wxString::Format("%s is not a valid image file", file));
				continue;
			}

			// Ask for name for patch
			wxFileName fn(file);
			wxString   name = fn.GetName().Upper().Truncate(8);
			name            = wxGetTextFromUser(
                wxString::Format("Enter a patch name for %s:", fn.GetFullName()), "New Patch", name);
			name = name.Truncate(8);

			// Add patch to archive
			entry->setName(name.ToStdString());
			entry->setExtensionByType();
			parent_->archive()->addEntry(entry, "patches");

			// Add patch to patch table
			patch_table_->addPatch(WxUtils::strToView(name));
		}

		// Refresh patch list
		list_patches_->updateList();
		parent_->pnamesModified(true);
	}
}

// -----------------------------------------------------------------------------
// Called when the 'Remove Patch' button is clicked
// -----------------------------------------------------------------------------
void PatchTablePanel::onBtnRemovePatch(wxCommandEvent& e)
{
	// Check anything is selected
	auto selection = list_patches_->selection(true);
	if (selection.empty())
		return;

	// TODO: Yes(to All) + No(to All) messagebox asking to delete entries along with patches

	// Go through patch list selection
	for (int a = selection.size() - 1; a >= 0; a--)
	{
		// Check if patch is currently in use
		auto& patch = patch_table_->patch(selection[a]);
		if (!patch.used_in.empty())
		{
			// In use, ask if it's ok to remove the patch
			int answer = wxMessageBox(
				wxString::Format(
					"The patch \"%s\" is currently used by %lu texture(s), are you sure you wish to remove it?",
					patch.name,
					patch.used_in.size()),
				"Confirm Remove Patch",
				wxYES_NO | wxCANCEL | wxICON_QUESTION,
				this);
			if (answer == wxYES)
			{
				// Answered yes, remove the patch
				parent_->removePatch(selection[a]);

				// Deselect it
				list_patches_->selectItem(selection[a], false);
			}
		}
		else
		{
			// Not in use, just delete it
			parent_->removePatch(selection[a]);

			// Deselect it
			list_patches_->selectItem(selection[a], false);
		}
	}

	// Update list
	list_patches_->updateList();
	parent_->pnamesModified(true);
}

// -----------------------------------------------------------------------------
// Called when the 'Change Patch' button is clicked
// -----------------------------------------------------------------------------
void PatchTablePanel::onBtnChangePatch(wxCommandEvent& e)
{
	// Check anything is selected
	auto selection = list_patches_->selection(true);
	if (selection.empty())
		return;

	// Go through patch list selection
	for (auto index : selection)
	{
		auto& patch = patch_table_->patch(index);

		// Prompt for new patch name
		wxString newname = wxGetTextFromUser("Enter new patch entry name:", "Change Patch", patch.name, this);

		// Update the patch if it's not the Cancel button that was clicked
		if (newname.Length() > 0)
			patch_table_->replacePatch(index, WxUtils::strToView(newname));

		// Update the list
		list_patches_->updateList();
		parent_->pnamesModified(true);
	}
}

// -----------------------------------------------------------------------------
// Called when a different patch or palette is selected
// -----------------------------------------------------------------------------
void PatchTablePanel::updateDisplay()
{
	// TODO: Separate palette changed and patch changed without breaking default
	// palette display; optimize label_textures display

	// Get selected patch
	int   index = list_patches_->itemIndex(list_patches_->lastSelected());
	auto& patch = patch_table_->patch(index);

	// Load the image
	auto entry = patch_table_->patchEntry(index);
	if (Misc::loadImageFromEntry(&patch_canvas_->image(), entry))
	{
		theMainWindow->paletteChooser()->setGlobalFromArchive(entry->parent());
		patch_canvas_->setPalette(theMainWindow->paletteChooser()->selectedPalette());
		label_dimensions_->SetLabel(
			wxString::Format("Size: %d x %d", patch_canvas_->image().width(), patch_canvas_->image().height()));
	}
	else
	{
		patch_canvas_->image().clear();
		label_dimensions_->SetLabel("Size: ? x ?");
	}
	patch_canvas_->Refresh();

	// List which textures use this patch
	if (!patch.used_in.empty())
	{
		wxString alltextures = "";
		int      count       = 0;
		wxString previous    = "";
		for (size_t a = 0; a < patch.used_in.size(); ++a)
		{
			wxString current = patch.used_in[a];

			// Is the use repeated for the same texture?
			if (!current.CmpNoCase(previous))
			{
				count++;
				// Else it's a new texture
			}
			else
			{
				// First add the count to the previous texture if needed
				if (count)
				{
					alltextures += wxString::Format(" (%i)", count + 1);
					count = 0;
				}

				// Add a separator if appropriate
				if (a > 0)
					alltextures += ';';

				// Then print the new texture's name
				alltextures += wxString::Format(" %s", patch.used_in[a]);

				// And set it for comparison with the next one
				previous = current;
			}
		}
		// If count is still non-zero, it's because the patch was repeated in the last texture
		if (count)
			alltextures += wxString::Format(" (%i)", count + 1);

		// Finally display the listing
		label_textures_->SetLabel(wxString::Format("In Textures:%s", alltextures));
	}
	else
		label_textures_->SetLabel("In Textures: -");

	// Wrap the text label
	label_textures_->Wrap(label_textures_->GetSize().GetWidth());

	// Update layout
	Layout();
}

// -----------------------------------------------------------------------------
// Called when a different patch or palette is selected
// -----------------------------------------------------------------------------
void PatchTablePanel::onDisplayChanged(wxCommandEvent& e)
{
	// TODO: Separate palette changed and patch changed without breaking
	// default palette display; optimize label_textures display
	updateDisplay();
}

// -----------------------------------------------------------------------------
// Handles any announcements
// -----------------------------------------------------------------------------
void PatchTablePanel::onAnnouncement(Announcer* announcer, string_view event_name, MemChunk& event_data)
{
	if (announcer != theMainWindow->paletteChooser())
		return;

	if (event_name == "main_palette_changed")
		updateDisplay();
}
