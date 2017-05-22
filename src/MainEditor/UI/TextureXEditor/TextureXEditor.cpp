
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    TextureXEditor.cpp
 * Description: The UI for managing/viewing/editing all composite
 *              textures in an archive.
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
#include "App.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/MainWindow.h"
#include "TextureXEditor.h"
#include "Archive/ArchiveManager.h"
#include "Dialogs/ExtMessageDialog.h"
#include "General/ResourceManager.h"
#include "General/UndoRedo.h"
#include "UI/PaletteChooser.h"
#include "UI/SAuiTabArt.h"
#include "UI/UndoManagerHistoryPanel.h"


/*******************************************************************
 * CREATETEXTUREXDIALOG CLASS
 *******************************************************************
 A simple dialog that provides options to create new (empty) texture
 definitions or import them from the base resource archive. Also has
 options for the desired texture format (doom, strife or TEXTURES)
 */
class CreateTextureXDialog : public wxDialog
{
private:
	wxRadioButton*	rb_format_doom;
	wxRadioButton*	rb_format_strife;
	wxRadioButton*	rb_format_textures;

	wxRadioButton*	rb_new;
	wxRadioButton*	rb_import_bra;
	wxRadioButton*	rb_import_archive;

public:
	CreateTextureXDialog(wxWindow* parent) : wxDialog(parent, -1, "Create Texture Definitions")
	{
		// Setup layout
		wxBoxSizer* m_vbox = new wxBoxSizer(wxVERTICAL);
		SetSizer(m_vbox);

		// --- Format options ---
		wxStaticBox* frame = new wxStaticBox(this, -1, "Format");
		wxStaticBoxSizer* framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
		m_vbox->Add(framesizer, 0, wxEXPAND|wxALL, 4);

		// Doom format
		rb_format_doom = new wxRadioButton(this, -1, "Doom (TEXTURE1 + PNAMES)", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
		framesizer->Add(rb_format_doom, 0, wxEXPAND|wxALL, 4);

		// Strife format
		rb_format_strife = new wxRadioButton(this, -1, "Strife (TEXTURE1 + PNAMES)");
		framesizer->Add(rb_format_strife, 0, wxEXPAND|wxALL, 4);

		// ZDoom TEXTURES format
		rb_format_textures = new wxRadioButton(this, -1, "ZDoom (TEXTURES)");
		framesizer->Add(rb_format_textures, 0, wxEXPAND|wxALL, 4);


		// --- Source options ---
		frame = new wxStaticBox(this, -1, "Source");
		framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
		m_vbox->Add(framesizer, 0, wxEXPAND|wxALL, 4);

		// New list
		rb_new = new wxRadioButton(this, -1, "Create New (Empty)", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
		framesizer->Add(rb_new, 0, wxEXPAND|wxALL, 4);

		// Import from Base Resource Archive
		rb_import_bra = new wxRadioButton(this, -1, "Import from Base Resource Archive:");
		framesizer->Add(rb_import_bra, 0, wxEXPAND|wxALL, 4);

		// Add buttons
		m_vbox->Add(CreateButtonSizer(wxOK|wxCANCEL), 0, wxEXPAND|wxALL, 4);

		// Bind events
		rb_new->Bind(wxEVT_RADIOBUTTON, &CreateTextureXDialog::onRadioNewSelected, this);
		rb_import_bra->Bind(wxEVT_RADIOBUTTON, &CreateTextureXDialog::onRadioNewSelected, this);

		SetInitialSize(wxSize(-1, -1));
		Layout();
	}

	~CreateTextureXDialog() {}

	int	getSelectedFormat()
	{
		if (rb_format_doom->GetValue())
			return TXF_NORMAL;
		else if (rb_format_strife->GetValue())
			return TXF_STRIFE11;
		else if (rb_format_textures->GetValue())
			return TXF_TEXTURES;
		else
			return -1;
	}

	bool createNewSelected()
	{
		return rb_new->GetValue();
	}

	void onRadioNewSelected(wxCommandEvent& e)
	{
		// Enable/Disable format selection depending on rb_new state
		if (rb_new->GetValue())
		{
			rb_format_doom->Enable(true);
			rb_format_strife->Enable(true);
			rb_format_textures->Enable(true);
		}
		else
		{
			rb_format_doom->Enable(false);
			rb_format_strife->Enable(false);
			rb_format_textures->Enable(false);
		}
	}
};


/*******************************************************************
 * TEXTUREXEDITOR CLASS FUNCTIONS
 *******************************************************************/

/* TextureXEditor::TextureXEditor
 * TextureXEditor class constructor
 *******************************************************************/
TextureXEditor::TextureXEditor(wxWindow* parent) : wxPanel(parent, -1)
{
	// Init variables
	this->archive = NULL;
	this->pnames = NULL;
	this->pb_update = true;
	undo_manager = new UndoManager();
	SetName("texturex");

	// Create texture menu
	menu_texture = new wxMenu();
	SAction::fromId("txed_new")->addToMenu(menu_texture);
	SAction::fromId("txed_new_patch")->addToMenu(menu_texture);
	SAction::fromId("txed_new_file")->addToMenu(menu_texture);
	SAction::fromId("txed_delete")->addToMenu(menu_texture);
	menu_texture->AppendSeparator();
	SAction::fromId("txed_rename")->addToMenu(menu_texture);
	SAction::fromId("txed_rename_each")->addToMenu(menu_texture);
	wxMenu* menu_export = new wxMenu();
	SAction::fromId("txed_export")->addToMenu(menu_export, "Archive (as image)");
	SAction::fromId("txed_extract")->addToMenu(menu_export, "File");
	menu_texture->AppendSubMenu(menu_export, "&Export To");
	menu_texture->AppendSeparator();
	SAction::fromId("txed_copy")->addToMenu(menu_texture);
	SAction::fromId("txed_cut")->addToMenu(menu_texture);
	SAction::fromId("txed_paste")->addToMenu(menu_texture);
	menu_texture->AppendSeparator();
	SAction::fromId("txed_up")->addToMenu(menu_texture);
	SAction::fromId("txed_down")->addToMenu(menu_texture);
	SAction::fromId("txed_sort")->addToMenu(menu_texture);
	wxMenu* menu_patch = new wxMenu();
	SAction::fromId("txed_patch_add")->addToMenu(menu_patch);
	SAction::fromId("txed_patch_remove")->addToMenu(menu_patch);
	SAction::fromId("txed_patch_replace")->addToMenu(menu_patch);
	SAction::fromId("txed_patch_back")->addToMenu(menu_patch);
	SAction::fromId("txed_patch_forward")->addToMenu(menu_patch);
	SAction::fromId("txed_patch_duplicate")->addToMenu(menu_patch);
	menu_texture->AppendSubMenu(menu_patch, "&Patch");

	// Create patch browser
	patch_browser = new PatchBrowser(theMainWindow);
	patch_browser->CenterOnParent();
	patch_browser->Show(false);

	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Add tabs
	//tabs = new wxAuiNotebook(this, -1, wxDefaultPosition, wxDefaultSize, wxAUI_NB_TOP|wxAUI_NB_TAB_SPLIT|wxAUI_NB_TAB_MOVE|wxAUI_NB_SCROLL_BUTTONS|wxAUI_NB_WINDOWLIST_BUTTON|wxBORDER_NONE);
	tabs = STabCtrl::createControl(this);
	sizer->Add(tabs, 1, wxEXPAND|wxALL, 4);

	// Bind events
	//btn_save->Bind(wxEVT_BUTTON, &TextureXEditor::onSaveClicked, this);
	Bind(wxEVT_SHOW, &TextureXEditor::onShow, this);

	// Palette chooser
	listenTo(theMainWindow->getPaletteChooser());
	updateTexturePalette();

	// Listen to patch table
	listenTo(&patch_table);

	// Listen to resource manager
	listenTo(theResourceManager);

	// Update+ layout
	Layout();
	Show();
}

/* TextureXEditor::~TextureXEditor
 * TextureXEditor class destructor
 *******************************************************************/
TextureXEditor::~TextureXEditor()
{
	if (pnames)
		pnames->unlock();
	if (menu_texture)
		delete menu_texture;
	delete undo_manager;
}

/* TextureXEditor::openArchive
 * Opens an archive to manage. Opens all texture related entries in
 * the archive and sorts a bunch of stuff out. If no texture related
 * entries exist in the archive, give the user options to create or
 * import them
 *******************************************************************/
bool TextureXEditor::openArchive(Archive* archive)
{
	// Check any archive was given
	if (!archive)
		return false;

	// Setup archive texture entries
	if (!setupTextureEntries(archive))
		return false;

	// Search archive for any texture-related entries
	Archive::search_options_t options;
	options.match_type = EntryType::getType("texturex");
	vector<ArchiveEntry*> tx_entries = archive->findAll(options);	// Find all TEXTUREx entries
	options.match_type = EntryType::getType("pnames");
	ArchiveEntry* entry_pnames = archive->findLast(options);		// Find last PNAMES entry

	// If any TEXTURE1/2 entries were found, setup patch table stuff
	if (tx_entries.size() > 0)
	{
		// Todo: Jaguar textures don't use PNAMES, so skip following checks if all texture entries are in Jaguar mode
		// TODO: Probably a better idea here to get the user to select an archive to import the patch table from
		// If no PNAMES entry was found, search resource archives
		if (!entry_pnames)
		{
			Archive::search_options_t options;
			options.match_type = EntryType::getType("pnames");
			entry_pnames = theArchiveManager->findResourceEntry(options, archive);
		}
		else
			pnames = entry_pnames;	// If PNAMES was found in the archive, set the class variable so it is written to if modified

		// If no PNAMES entry is found at all, show an error and abort
		// TODO: ask user to select appropriate base resource archive
		if (!entry_pnames)
		{
			wxMessageBox("PNAMES entry not found!", wxMessageBoxCaptionStr, wxICON_ERROR);
			return false;
		}

		// Load patch table
		patch_table.loadPNAMES(entry_pnames, archive);
	}

	// Open texture editor tabs
	for (size_t a = 0; a < tx_entries.size(); a++)
	{
		TextureXPanel* tx_panel = new TextureXPanel(tabs, this);

		// Init texture panel
		tx_panel->Show(false);

		// Open TEXTUREX entry
		if (tx_panel->openTEXTUREX(tx_entries[a]))
		{
			// Set palette
			tx_panel->setPalette(theMainWindow->getPaletteChooser()->getSelectedPalette());
			// Lock entry
			tx_entries[a]->lock();

			// Add it to the list of editors, and a tab
			tx_panel->SetName("textures");
			texture_editors.push_back(tx_panel);
			tabs->AddPage(tx_panel, tx_entries[a]->getName());
		}

		tx_panel->Show(true);
	}

	// Open patch table tab if needed
	if (pnames)
	{
		PatchTablePanel* ptp = new PatchTablePanel(tabs, &patch_table);
		tabs->AddPage(ptp, "Patch Table (PNAMES)");
		ptp->SetName("pnames");
	}

	// Search archive for TEXTURES entries
	options.match_type = EntryType::getType("zdtextures");
	vector<ArchiveEntry*> ztx_entries = archive->findAll(options);

	// Open texture editor tabs
	for (unsigned a = 0; a < ztx_entries.size(); a++)
	{
		TextureXPanel* tx_panel = new TextureXPanel(tabs, this);

		// Init texture panel
		tx_panel->Show(false);

		// Open TEXTURES entry
		if (tx_panel->openTEXTUREX(ztx_entries[a]))
		{
			// Set palette
			tx_panel->setPalette(theMainWindow->getPaletteChooser()->getSelectedPalette());
			// Lock entry
			ztx_entries[a]->lock();

			// Add it to the list of editors, and a tab
			tx_panel->SetName("textures");
			texture_editors.push_back(tx_panel);
			tabs->AddPage(tx_panel, ztx_entries[a]->getName());
		}

		tx_panel->Show(true);
	}

	// Update layout
	Layout();
	tabs->Refresh();

	// Update variables
	this->archive = archive;
	pnames_modified = false;

	// Lock pnames entry if it exists
	if (pnames)
		pnames->lock();

	// Set global palette
	theMainWindow->getPaletteChooser()->setGlobalFromArchive(archive);

	// Setup patch browser
	if (patch_table.nPatches() > 0)
		patch_browser->openPatchTable(&patch_table);
	else
		patch_browser->openArchive(archive);

	return true;
}

/* TextureXEditor::updateTexturePalette
 * Sets the texture panels' palettes to what is selected in the
 * palette chooser
 *******************************************************************/
void TextureXEditor::updateTexturePalette()
{
	// Get palette
	Palette8bit* pal = theMainWindow->getPaletteChooser()->getSelectedPalette();

	// Send to whatever needs it
	for (size_t a = 0; a < texture_editors.size(); a++)
		texture_editors[a]->setPalette(pal);
	patch_browser->setPalette(pal);
}

/* TextureXEditor::saveChanges
 * Saves changes to all open TEXTUREx and PNAMES entries
 *******************************************************************/
void TextureXEditor::saveChanges()
{
	// Check for problems
	checkTextures();

	// Save TEXTUREx entries
	for (unsigned a = 0; a < texture_editors.size(); a++)
		if (texture_editors[a]->isModified())
			texture_editors[a]->saveTEXTUREX();

	// Save PNAMES if it exists
	if (patch_table.nPatches() > 0)
	{
		if (!pnames)
		{
			// If no PNAMES entry exists in the archive, create one
			int index = archive->entryIndex(texture_editors.back()->txEntry()) + 1;
			pnames = archive->addNewEntry("PNAMES", index);
			pnames->setType(EntryType::getType("pnames"));
			pnames->setExtensionByType();
		}

		pnames->unlock();	// Have to unlock it to write
		patch_table.writePNAMES(pnames);
		pnames->lock();

		pnames_modified = false;
	}
}

/* TextureXEditor::close
 * 'Closes' the texture editor, prompting the user to save any
 * unsaved changes. Returns false if the close operation should be
 * cancelled, true otherwise
 *******************************************************************/
bool TextureXEditor::close()
{
	// Check if any texture lists are modified
	bool modified = false;
	for (unsigned a = 0; a < texture_editors.size(); a++)
	{
		texture_editors[a]->applyChanges();
		if (texture_editors[a]->isModified())
			modified = true;
	}

	// Check if patch table was modified
	if (pnames_modified)
		modified = true;

	// Ask to save changes
	if (modified)
	{
		wxMessageDialog md(this, "Save changes to texture entries?", "Unsaved Changes", wxYES_NO|wxCANCEL);
		int result = md.ShowModal();
		if (result == wxID_YES)
			saveChanges();	// User selected to save
		else if (result == wxID_CANCEL)
			return false;	// User selected cancel, don't close the archive
	}

	return true;
}

/* TextureXEditor::showTextureMenu
 * Shows or hides the custom 'Texture' menu
 *******************************************************************/
void TextureXEditor::showTextureMenu(bool show)
{
	if (show)
		theMainWindow->addCustomMenu(menu_texture, "&Texture");
	else
		theMainWindow->removeCustomMenu(menu_texture);
}

/* TextureXEditor::removePatch
 * Removes the patch at [index] on the patch table from any textures
 * that contain it (and from the patch table itself)
 *******************************************************************/
bool TextureXEditor::removePatch(unsigned index, bool delete_entry)
{
	// Get patch we're removing
	patch_t& p = patch_table.patch(index);

	// Update TEXTUREx lists
	for (unsigned a = 0; a < texture_editors.size(); a++)
		texture_editors[a]->txList().removePatch(p.name);

	// Delete patch entry if it's part of this archive (and delete_entry is true)
	ArchiveEntry* entry = theResourceManager->getPatchEntry(p.name, "patches", archive);
	if (delete_entry && entry && entry->getParent() == archive)
		archive->removeEntry(entry);

	// Remove patch from patch table
	patch_table.removePatch(index);

	return true;
}

/* TextureXEditor::browsePatch
 * Opens the patch table in the patch browser. Returns the selected
 * patch index, or -1 if no patch was selected
 *******************************************************************/
int TextureXEditor::browsePatchTable(string first)
{
	// Select initial patch if specified
	if (!first.IsEmpty())
		patch_browser->selectPatch(first);

	if (patch_browser->ShowModal() == wxID_OK)
		return patch_browser->getSelectedPatch();
	else
		return -1;
}

/* TextureXEditor::browsePatchEntry
 * Opens resource patch entries in the patch browser. Returns the
 * selected patch name, or "" if no patch was selected
 *******************************************************************/
string TextureXEditor::browsePatchEntry(string first)
{
	// Update patch browser if necessary
	if (pb_update)
	{
		// Add archive textures (and resource textures)
		patch_browser->openArchive(archive);

		// Add each texture list from this archive
		for (unsigned a = 0; a < texture_editors.size(); a++)
			patch_browser->openTextureXList(&(texture_editors[a]->txList()), archive);

		pb_update = false;
	}

	// Select initial patch if specified
	if (!first.IsEmpty())
		patch_browser->selectPatch(first);

	if (patch_browser->ShowModal() == wxID_OK && patch_browser->getSelectedItem())
		return patch_browser->getSelectedItem()->getName();
	else
		return "";
}

/* TextureXEditor::checkTextures
 * Checks all texture definitions for problems and alerts the user
 * if any are found. Returns true if any problems were found, false
 * otherwise
 *******************************************************************/
bool TextureXEditor::checkTextures()
{
	string problems = wxEmptyString;

	// Go through all texturex lists
	for (unsigned a = 0; a < texture_editors.size(); a++)
	{
		// Go through all textures
		for (unsigned t = 0; t < texture_editors[a]->txList().nTextures(); t++)
		{
			// Get texture
			CTexture* tex = texture_editors[a]->txList().getTexture(t);

			// Check its patches are all valid
			if (tex->isExtended())
			{
				// Extended texture, check if each patch exists in any open archive (or as a composite texture)
				for (unsigned p = 0; p < tex->nPatches(); p++)
				{
					ArchiveEntry* pentry = theResourceManager->getPatchEntry(tex->getPatch(p)->getName());
					ArchiveEntry* fentry = theResourceManager->getFlatEntry(tex->getPatch(p)->getName());
					CTexture* ptex = theResourceManager->getTexture(tex->getPatch(p)->getName());
					if (!pentry && !fentry && !ptex)
						problems += S_FMT("Texture %s contains invalid/unknown patch %s\n", tex->getName(), tex->getPatch(p)->getName());
				}
			}
			else
			{
				// Regular texture, check the patch table
				for (unsigned p = 0; p < tex->nPatches(); p++)
				{
					if (patch_table.patchIndex(tex->getPatch(p)->getName()) == -1)
						problems += S_FMT("Texture %s contains invalid/unknown patch %s\n", tex->getName(), tex->getPatch(p)->getName());
				}
			}
		}
	}

	// Go through patch table
	for (unsigned a = 0; a < patch_table.nPatches(); a++)
	{
		// Check patch entry is valid
		patch_t& patch = patch_table.patch(a);
		ArchiveEntry* entry = theResourceManager->getPatchEntry(patch.name, "patches", archive);

		if (!entry)
		{
			problems += S_FMT("Patch %s cannot be found in any open archive\n", patch.name);
		}
		else
		{
			// Check patch entry type
			if (entry->getType() == EntryType::unknownType())
				EntryType::detectEntryType(entry);
			EntryType* type = entry->getType();

			if (!type->extraProps().propertyExists("patch"))
				problems += S_FMT("Patch %s is of type \"%s\", which is not a valid gfx format for patches. Convert it to either Doom Gfx or PNG\n", patch.name, type->getName());
		}
	}

	// Display a message box with any problems found
	if (!problems.IsEmpty())
	{
		ExtMessageDialog dlg(this, "Problems Found");
		dlg.setMessage("The following problems were found:");
		dlg.setExt(problems);
		dlg.ShowModal();

		return true;
	}
	else
		return false;
}

/* TextureXEditor::setSelection
 * Sets the active tab to be the one corresponding to the given
 * entry index or entry.
 *******************************************************************/
void TextureXEditor::setSelection(size_t index)
{
	if (index < tabs->GetPageCount() && index != tabs->GetSelection())
		tabs->SetSelection(index);
}
void TextureXEditor::setSelection(ArchiveEntry* entry)
{
	for (size_t a = 0; a < tabs->GetPageCount(); a++)
	{
		if (S_CMPNOCASE(tabs->GetPage(a)->GetName(), "pnames") && (entry == pnames))
		{
			tabs->SetSelection(a);
			return;
		}
		else if (S_CMPNOCASE(tabs->GetPage(a)->GetName(), "textures"))
		{
			TextureXPanel* txp = (TextureXPanel*)tabs->GetPage(a);
			if (txp->txEntry() == entry)
			{
				tabs->SetSelection(a);
				return;
			}
		}
	}
}

/* TextureXEditor::updateMenuStatus
 * Checks if the Texture menu needs to be displayed or not 
 *******************************************************************/
void TextureXEditor::updateMenuStatus()
{
	if (tabs->GetSelection() < 0)
		return;

	wxWindow* current = tabs->GetPage(tabs->GetSelection());

	// Check if the currently opened tab is a texturex list
	bool tex = false;
	for (unsigned a = 0; a < texture_editors.size(); a++)
	{
		if (texture_editors[a] == current)
		{
			tex = true;
			break;
		}
	}

	// Show/hide texture menu accordingly
	showTextureMenu(tex);
}

/* TextureXEditor::undo
 * Performs an undo operation
 *******************************************************************/
void TextureXEditor::undo()
{
	string action = undo_manager->undo();
	if (action != "")
	{
		for (unsigned a = 0; a < texture_editors.size(); a++)
			texture_editors[a]->onUndo(action);
	}
}

/* TextureXEditor::redo
 * Performs an redo operation
 *******************************************************************/
void TextureXEditor::redo()
{
	string action = undo_manager->redo();
	if (action != "")
	{
		for (unsigned a = 0; a < texture_editors.size(); a++)
			texture_editors[a]->onRedo(action);
	}
}

/* TextureXEditor::onAnnouncement
 * Handles any announcements from the current texture
 *******************************************************************/
void TextureXEditor::onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data)
{
	if (announcer == theMainWindow->getPaletteChooser() && event_name == "main_palette_changed")
	{
		updateTexturePalette();
	}

	if (announcer == &patch_table && event_name == "modified")
	{
		patch_browser->openPatchTable(&patch_table);
		pb_update = true;
	}

	if (announcer == theResourceManager && event_name == "resources_updated")
	{
		pb_update = true;
		updateTexturePalette();
	}
}


/*******************************************************************
 * TEXTUREXEDITOR EVENTS
 *******************************************************************/

/* TextureXEditor::onSaveClicked
 * Called when the 'Save Changes' button is clicked
 *******************************************************************/
void TextureXEditor::onSaveClicked(wxCommandEvent& e)
{
	saveChanges();
}

/* TextureXEditor::onShow
 * Called when the panel is shown or hidden
 *******************************************************************/
void TextureXEditor::onShow(wxShowEvent& e)
{
	if (!e.IsShown())
	{
		showTextureMenu(false);
		return;
	}
	else
		theMainWindow->getUndoHistoryPanel()->setManager(undo_manager);
	updateMenuStatus();
}


/*******************************************************************
 * TEXTUREXEDITOR STATIC FUNCTIONS
 *******************************************************************/

/* TextureXEditor::setupTextureEntries
 * Static function to check if an archive has sufficient texture
 * related entries, and if not, prompts the user to either create
 * or import them. Returns true if the entries exist, false otherwise
 *******************************************************************/
bool TextureXEditor::setupTextureEntries(Archive* archive)
{
	// Check any archive was given
	if (!archive)
		return false;

	// Search archive for any ZDoom TEXTURES entries
	Archive::search_options_t options;
	options.match_type = EntryType::getType("zdtextures");
	ArchiveEntry* entry_tx = archive->findFirst(options);	// Find any TEXTURES entry

	// If it's found, we're done
	if (entry_tx)
		return true;


	// Search archive for any texture-related entries
	options.match_type = EntryType::getType("texturex");
	entry_tx = archive->findFirst(options);						// Find any TEXTUREx entry
	options.match_type = EntryType::getType("pnames");
	ArchiveEntry* entry_pnames = archive->findFirst(options);	// Find any PNAMES entry

	// If both exist, we're done
	if (entry_tx && entry_pnames)
		return true;

	// Todo: accept entry_tx without pnames if the textures are in Jaguar mode

	// If no TEXTUREx entry exists
	if (!entry_tx)
	{
		// No TEXTUREx entries found, so ask if the user wishes to create one
		wxMessageDialog dlg(NULL, "The archive does not contain any texture definitions (TEXTURE1/2 or TEXTURES). Do you wish to create or import a texture definition list?", "No Texture Definitions Found", wxYES_NO);

		if (dlg.ShowModal() == wxID_YES)
		{
			CreateTextureXDialog ctxd(NULL);

			while (1)
			{
				// Check if cancelled
				if (ctxd.ShowModal() == wxID_CANCEL)
					return false;

				if (ctxd.createNewSelected())
				{
					// User selected to create a new TEXTUREx list
					ArchiveEntry* texturex = NULL;

					int format = ctxd.getSelectedFormat();
					if (ctxd.getSelectedFormat() == TXF_NORMAL || ctxd.getSelectedFormat() == TXF_STRIFE11)  	// Doom or Strife TEXTUREx
					{
						// Create texture list
						TextureXList txlist;
						txlist.setFormat(ctxd.getSelectedFormat());

						// Create patch table
						PatchTable ptt;

						// Create dummy patch
						ArchiveEntry* dpatch = theArchiveManager->programResourceArchive()->entryAtPath("s3dummy.lmp");
						dpatch = archive->addEntry(dpatch, "patches", true);
						ptt.addPatch("S3DUMMY");

						// Create dummy texture
						CTexture* dummytex = new CTexture();
						dummytex->setName("S3DUMMY");
						dummytex->addPatch("S3DUMMY", 0, 0);
						dummytex->setWidth(128);
						dummytex->setHeight(128);
						dummytex->setScale(0, 0);

						// Add dummy texture to list
						// (this serves two purposes - supplies the special 'invalid' texture by default,
						//   and allows the texturex format to be detected)
						txlist.addTexture(dummytex);

						// Add empty PNAMES entry to archive
						entry_pnames = archive->addNewEntry("PNAMES");
						ptt.writePNAMES(entry_pnames);
						entry_pnames->setType(EntryType::getType("pnames"));
						entry_pnames->setExtensionByType();

						// Add empty TEXTURE1 entry to archive
						texturex = archive->addNewEntry("TEXTURE1");
						txlist.writeTEXTUREXData(texturex, ptt);
						texturex->setType(EntryType::getType("texturex"));
						texturex->setExtensionByType();
					}
					else if (ctxd.getSelectedFormat() == TXF_TEXTURES)
					{
						// Create texture list
						TextureXList txlist;
						txlist.setFormat(TXF_TEXTURES);

						// Add empty TEXTURES entry to archive
						texturex = archive->addNewEntry("TEXTURES");
						texturex->setType(EntryType::getType("zdtextures"));
						texturex->setExtensionByType();

						return false;
					}

					if (!texturex)
						return false;
				}
				else
				{
					// User selected to import texture definitions from the base resource archive
					Archive* bra = theArchiveManager->baseResourceArchive();

					if (!bra)
					{
						wxMessageBox("No Base Resource Archive is opened, please select/open one", "Error", wxICON_ERROR);
						continue;
					}

					// Find all relevant entries in the base resource archive
					Archive::search_options_t options;
					options.match_type = EntryType::getType("texturex");
					vector<ArchiveEntry*> import_tx = bra->findAll(options);	// Find all TEXTUREx entries
					options.match_type = EntryType::getType("pnames");
					ArchiveEntry* import_pnames = bra->findLast(options);		// Find last PNAMES entry

					// Check enough entries exist
					if (import_tx.size() == 0 || !import_pnames)
					{
						wxMessageBox("The selected Base Resource Archive does not contain sufficient texture definition entries", "Error", wxICON_ERROR);
						continue;
					}

					// Copy TEXTUREx entries over to current archive
					for (unsigned a = 0; a < import_tx.size(); a++)
					{
						ArchiveEntry* texturex = archive->addEntry(import_tx[a], "global", true);
						texturex->setType(EntryType::getType("texturex"));
						texturex->setExtensionByType();
					}

					// Copy PNAMES entry over to current archive
					entry_pnames = archive->addEntry(import_pnames, "global", true);
					entry_pnames->setType(EntryType::getType("pnames"));
					entry_pnames->setExtensionByType();
				}

				break;
			}

			return true;
		}

		// 'No' clicked
		return false;
	}
	else  	// TEXTUREx entry exists
	{
		// TODO: Probably a better idea here to get the user to select an archive to import the patch table from
		// If no PNAMES entry was found, search resource archives
		if (!entry_pnames)
		{
			Archive::search_options_t options;
			options.match_type = EntryType::getType("pnames");
			entry_pnames = theArchiveManager->findResourceEntry(options, archive);
		}

		// If no PNAMES entry is found at all, show an error and abort
		// TODO: ask user to select appropriate base resource archive
		if (!entry_pnames)
		{
			wxMessageBox("PNAMES entry not found!", wxMessageBoxCaptionStr, wxICON_ERROR);
			return false;
		}

		return true;
	}

	return false;
}
