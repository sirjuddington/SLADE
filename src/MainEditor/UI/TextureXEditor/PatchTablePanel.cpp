
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    PatchTablePanel.cpp
 * Description: The UI for viewing/editing a patch table (PNAMES)
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
#include "PatchTablePanel.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveManager.h"
#include "General/Misc.h"
#include "Graphics/SImage/SImage.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/MainWindow.h"
#include "TextureXEditor.h"
#include "UI/Canvas/GfxCanvas.h"
#include "UI/PaletteChooser.h"


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(String, dir_last)


/*******************************************************************
 * PATCHTABLELISTVIEW CLASS FUNCTIONS
 *******************************************************************/

/* PatchTableListView::PatchTableListView
 * PatchTableListView class constructor
 *******************************************************************/
PatchTableListView::PatchTableListView(wxWindow* parent, PatchTable* patch_table) : VirtualListView(parent)
{
	// Init Variables
	this->patch_table = patch_table;
	listenTo(patch_table);

	// Add columns
	InsertColumn(0, "#");
	InsertColumn(1, "Patch Name");
	InsertColumn(2, "Use Count");
	InsertColumn(3, "In Archive");

	// Update list
	updateList();

	// Listen to archive manager
	listenTo(theArchiveManager);
}

/* PatchTableListView::~PatchTableListView
 * PatchTableListView class constructor
 *******************************************************************/
PatchTableListView::~PatchTableListView()
{
}

/* PatchTableListView::getItemText
 * Returns the string for [item] at [column]
 *******************************************************************/
string PatchTableListView::getItemText(long item, long column, long index) const
{
	// Check patch table exists
	if (!patch_table)
		return "INVALID INDEX";

	// Check index is ok
	if (index < 0 || (unsigned)index > patch_table->nPatches())
		return "INVALID INDEX";

	// Get associated patch
	patch_t& patch = patch_table->patch(index);

	if (column == 0)						// Index column
		return S_FMT("%04d", index);
	else if (column == 1)					// Name column
		return patch.name;
	else if (column == 2)					// Usage count column
		return S_FMT("%lu", patch.used_in.size());
	else if (column == 3)  					// Archive column
	{
		// Get patch entry
		ArchiveEntry* entry = patch_table->patchEntry(index);

		// If patch entry can't be found return invalid
		if (entry)
			return entry->getParent()->getFilename(false);
		else
			return "(!) NOT FOUND";
	}
	else									// Invalid column
		return "INVALID COLUMN";
}

/* PatchTableListView::updateItemAttr
 * Updates the item attributes for [item] (red text if patch entry
 * not found, default otherwise)
 *******************************************************************/
void PatchTableListView::updateItemAttr(long item, long column, long index) const
{
	// Just set normal text colour
	item_attr->SetTextColour(wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT));
}

/* PatchTableListView::updateList
 * Updates + refreshes the patch list
 *******************************************************************/
void PatchTableListView::updateList(bool clear)
{
	if (clear)
		ClearAll();

	// Set list size
	items.clear();
	if (patch_table)
	{
		size_t count = patch_table->nPatches();
		SetItemCount(count);
		for (unsigned a = 0; a < count; a++)
			items.push_back(a);
	}
	else
		SetItemCount(0);

	sortItems();
	updateWidth();
	Refresh();
}

/* PatchTableListView::onAnnouncement
 * Handles announcements from the panel's PatchTable
 *******************************************************************/
void PatchTableListView::onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data)
{
	// Just refresh on any event from the patch table
	if (announcer == patch_table)
		updateList();

	if (announcer == theArchiveManager)
		updateList();
}

/* PatchTableListView::usageSort
 * Returns true if patch at index [left] is used less than [right]
 *******************************************************************/
bool PatchTableListView::usageSort(long left, long right)
{
	patch_t& p1 = ((PatchTableListView*)lv_current)->patchTable()->patch(left);
	patch_t& p2 = ((PatchTableListView*)lv_current)->patchTable()->patch(right);

	if (p1.used_in.size() == p2.used_in.size())
		return left < right;
	else
		return lv_current->sortDescend() ? p2.used_in.size() < p1.used_in.size() : p1.used_in.size() < p2.used_in.size();
}

/* TextureXListView::sortItems
 * Sorts the list items depending on the current sorting column
 *******************************************************************/
void PatchTableListView::sortItems()
{
	lv_current = this;
	if (sort_column == 2)
		std::sort(items.begin(), items.end(), &PatchTableListView::usageSort);
	else
		std::sort(items.begin(), items.end(), &VirtualListView::defaultSort);
}


/*******************************************************************
 * PATCHTABLEPANEL CLASS FUNCTIONS
 *******************************************************************/

/* PatchTablePanel::PatchTablePanel
 * PatchTablePanel class constructor
 *******************************************************************/
PatchTablePanel::PatchTablePanel(wxWindow* parent, PatchTable* patch_table) : wxPanel(parent, -1)
{
	// Init variables
	this->patch_table = patch_table;
	this->parent = (TextureXEditor*)parent;

	// Setup sizers
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(sizer);
	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);

	// Add PNAMES list
	wxStaticBox* frame = new wxStaticBox(this, -1, "Patches (PNAMES)");
	wxStaticBoxSizer* framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, 0, wxEXPAND|wxALL, 4);
	list_patches = new PatchTableListView(this, patch_table);
	list_patches->setSearchColumn(1);	// Want to search by patch name not index
	framesizer->Add(list_patches, 1, wxEXPAND|wxALL, 4);

	// Add editing controls
	sizer->Add(vbox, 1, wxEXPAND|wxALL, 4);
	vbox->Add(hbox, 0, wxEXPAND, 4);
	frame = new wxStaticBox(this, -1, "Actions");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	hbox->Add(framesizer, 0, wxEXPAND|wxALL, 4);

	// Add patch button
	btn_add_patch = new wxButton(this, -1, "New Patch");
	framesizer->Add(btn_add_patch, 0, wxEXPAND|wxALL, 4);

	// New patch from file button
	btn_patch_from_file = new wxButton(this, -1, "New Patch from File");
	framesizer->Add(btn_patch_from_file, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Remove patch button
	btn_remove_patch = new wxButton(this, -1, "Remove Patch");
	framesizer->Add(btn_remove_patch, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Change patch button
	btn_change_patch = new wxButton(this, -1, "Change Patch");
	framesizer->Add(btn_change_patch, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Add information display
	frame = new wxStaticBox(this, -1, "Info");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	hbox->Add(framesizer, 1, wxEXPAND|wxALL, 4);
	label_dimensions = new wxStaticText(this, -1, "Size: N/A");
	framesizer->Add(label_dimensions, 0, wxEXPAND|wxALL, 4);
	label_textures = new wxStaticText(this, -1, "In Textures: -", wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);
	framesizer->Add(label_textures, 1, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Add patch canvas
	frame = new wxStaticBox(this, -1, "Display");
	framesizer = new wxStaticBoxSizer(frame, wxHORIZONTAL);
	vbox->Add(framesizer, 1, wxEXPAND|wxALL, 4);
	wxBoxSizer* c_box = new wxBoxSizer(wxVERTICAL);
	framesizer->Add(c_box, 1, wxEXPAND|wxALL, 4);
	patch_canvas = new GfxCanvas(this, -1);
	c_box->Add(patch_canvas->toPanel(this), 1, wxEXPAND|wxALL, 4);
	patch_canvas->setViewType(GFXVIEW_DEFAULT);
	patch_canvas->allowDrag(true);
	patch_canvas->allowScroll(true);

	// Bind events
	btn_add_patch->Bind(wxEVT_BUTTON, &PatchTablePanel::onBtnAddPatch, this);
	btn_patch_from_file->Bind(wxEVT_BUTTON, &PatchTablePanel::onBtnPatchFromFile, this);
	btn_remove_patch->Bind(wxEVT_BUTTON, &PatchTablePanel::onBtnRemovePatch, this);
	btn_change_patch->Bind(wxEVT_BUTTON, &PatchTablePanel::onBtnChangePatch, this);
	list_patches->Bind(wxEVT_LIST_ITEM_SELECTED, &PatchTablePanel::onDisplayChanged, this);

	// Palette chooser
	listenTo(theMainWindow->getPaletteChooser());
}

/* PatchTablePanel::~PatchTablePanel
 * PatchTablePanel class destructor
 *******************************************************************/
PatchTablePanel::~PatchTablePanel()
{
}


/*******************************************************************
 * PATCHTABLEPANEL CLASS EVENTS
 *******************************************************************/

/* PatchTablePanel::onBtnAddPatch
 * Called when the 'New Patch' button is clicked
 *******************************************************************/
void PatchTablePanel::onBtnAddPatch(wxCommandEvent& e)
{
	// Prompt for new patch name
	string patch = wxGetTextFromUser("Enter patch entry name:", "Add Patch", wxEmptyString, this);

	// Check something was entered
	if (patch.IsEmpty())
		return;

	// Add to patch table
	patch_table->addPatch(patch);

	// Update list
	list_patches->updateList();
	parent->pnamesModified(true);
}

/* PatchTablePanel::onBtnPatchFromFile
 * Called when the 'New Patch from File' button is clicked
 *******************************************************************/
void PatchTablePanel::onBtnPatchFromFile(wxCommandEvent& e)
{
	// Get all entry types
	vector<EntryType*> etypes = EntryType::allTypes();

	// Go through types
	string ext_filter = "All files (*.*)|*.*|";
	for (unsigned a = 0; a < etypes.size(); a++)
	{
		// If the type is a valid image type, add its extension filter
		if (etypes[a]->extraProps().propertyExists("image"))
		{
			ext_filter += etypes[a]->getFileFilterString();
			ext_filter += "|";
		}
	}

	// Create open file dialog
	wxFileDialog dialog_open(this, "Choose file(s) to open", dir_last, wxEmptyString,
	                         ext_filter, wxFD_OPEN|wxFD_MULTIPLE|wxFD_FILE_MUST_EXIST, wxDefaultPosition);

	// Run the dialog & check that the user didn't cancel
	if (dialog_open.ShowModal() == wxID_OK)
	{
		// Get file selection
		wxArrayString files;
		dialog_open.GetPaths(files);

		// Save 'dir_last'
		dir_last = dialog_open.GetDirectory();

		// Go through file selection
		for (unsigned a = 0; a < files.size(); a++)
		{
			// Load the file into a temporary ArchiveEntry
			ArchiveEntry* entry = new ArchiveEntry();
			entry->importFile(files[a]);

			// Determine type
			EntryType::detectEntryType(entry);

			// If it's not a valid image type, ignore this file
			if (!entry->getType()->extraProps().propertyExists("image"))
			{
				LOG_MESSAGE(1, "%s is not a valid image file", files[a]);
				continue;
			}

			// Ask for name for patch
			wxFileName fn(files[a]);
			string name = fn.GetName().Upper().Truncate(8);
			name = wxGetTextFromUser(S_FMT("Enter a patch name for %s:", fn.GetFullName()), "New Patch", name);
			name = name.Truncate(8);

			// Add patch to archive
			entry->setName(name);
			entry->setExtensionByType();
			parent->getArchive()->addEntry(entry, "patches");

			// Add patch to patch table
			patch_table->addPatch(name);
		}

		// Refresh patch list
		list_patches->updateList();
		parent->pnamesModified(true);
	}
}

/* PatchTablePanel::onBtnRemovePatch
 * Called when the 'Remove Patch' button is clicked
 *******************************************************************/
void PatchTablePanel::onBtnRemovePatch(wxCommandEvent& e)
{
	// Check anything is selected
	vector<long> selection = list_patches->getSelection(true);
	if (selection.size() == 0)
		return;

	// TODO: Yes(to All) + No(to All) messagebox asking to delete entries along with patches

	// Go through patch list selection
	for (int a = selection.size() - 1; a >= 0; a--)
	{
		// Check if patch is currently in use
		patch_t& patch = patch_table->patch(selection[a]);
		if (patch.used_in.size() > 0)
		{
			// In use, ask if it's ok to remove the patch
			int answer = wxMessageBox(S_FMT("The patch \"%s\" is currently used by %lu texture(s), are you sure you wish to remove it?", patch.name, patch.used_in.size()), "Confirm Remove Patch", wxYES_NO|wxCANCEL|wxICON_QUESTION, this);
			if (answer == wxYES)
			{
				// Answered yes, remove the patch
				parent->removePatch(selection[a]);

				// Deselect it
				list_patches->selectItem(selection[a], false);
			}
		}
		else
		{
			// Not in use, just delete it
			parent->removePatch(selection[a]);

			// Deselect it
			list_patches->selectItem(selection[a], false);
		}
	}

	// Update list
	list_patches->updateList();
	parent->pnamesModified(true);
}

/* PatchTablePanel::onBtnChangePatch
 * Called when the 'Change Patch' button is clicked
 *******************************************************************/
void PatchTablePanel::onBtnChangePatch(wxCommandEvent& e)
{
	// Check anything is selected
	vector<long> selection = list_patches->getSelection(true);
	if (selection.size() == 0)
		return;

	// Go through patch list selection
	for (unsigned a = 0; a < selection.size(); a++)
	{
		patch_t& patch = patch_table->patch(selection[a]);

		// Prompt for new patch name
		string newname = wxGetTextFromUser("Enter new patch entry name:", "Change Patch", patch.name, this);

		// Update the patch if it's not the Cancel button that was clicked
		if (newname.Length() > 0)
			patch_table->replacePatch(selection[a], newname);

		// Update the list
		list_patches->updateList();
		parent->pnamesModified(true);
	}
}

/* PatchTablePanel::updateDisplay
 * Called when a different patch or palette is selected
 * TODO: Separate palette changed and patch changed without breaking
 * default palette display; optimize label_textures display
 *******************************************************************/
void PatchTablePanel::updateDisplay()
{
	// Get selected patch
	int index = list_patches->getItemIndex(list_patches->getLastSelected());
	patch_t& patch = patch_table->patch(index);

	// Load the image
	ArchiveEntry* entry = patch_table->patchEntry(index);
	if (Misc::loadImageFromEntry(patch_canvas->getImage(), entry))
	{
		theMainWindow->getPaletteChooser()->setGlobalFromArchive(entry->getParent());
		patch_canvas->setPalette(theMainWindow->getPaletteChooser()->getSelectedPalette());
		label_dimensions->SetLabel(S_FMT("Size: %d x %d", patch_canvas->getImage()->getWidth(), patch_canvas->getImage()->getHeight()));
	}
	else
	{
		patch_canvas->getImage()->clear();
		label_dimensions->SetLabel("Size: ? x ?");
	}
	patch_canvas->Refresh();

	// List which textures use this patch
	if (patch.used_in.size() > 0)
	{
		string alltextures = "";
		int count = 0;
		string previous = "";
		for (size_t a = 0; a < patch.used_in.size(); ++a)
		{
			string current = patch.used_in[a];

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
					alltextures += S_FMT(" (%i)", count + 1);
					count = 0;
				}

				// Add a separator if appropriate
				if (a > 0)
					alltextures += ';';

				// Then print the new texture's name
				alltextures += S_FMT(" %s", patch.used_in[a].mb_str());

				// And set it for comparison with the next one
				previous = current;
			}
		}
		// If count is still non-zero, it's because the patch was repeated in the last texture
		if (count)
			alltextures += S_FMT(" (%i)", count + 1);

		// Finally display the listing
		label_textures->SetLabel(S_FMT("In Textures:%s", alltextures.mb_str()));
	}
	else
		label_textures->SetLabel("In Textures: -");

	// Wrap the text label
	label_textures->Wrap(label_textures->GetSize().GetWidth());

	// Update layout
	Layout();
}

/* PatchTablePanel::onDisplayChanged
 * Called when a different patch or palette is selected
 * TODO: Separate palette changed and patch changed without breaking
 * default palette display; optimize label_textures display
 *******************************************************************/
void PatchTablePanel::onDisplayChanged(wxCommandEvent& e)
{
	updateDisplay();
}

/* PatchTablePanel::onAnnouncement
 * Handles any announcements
 *******************************************************************/
void PatchTablePanel::onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data)
{
	if (announcer != theMainWindow->getPaletteChooser())
		return;

	if (event_name == "main_palette_changed")
	{
		updateDisplay();
	}
}
