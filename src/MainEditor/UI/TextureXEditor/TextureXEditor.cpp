
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    TextureXEditor.cpp
// Description: The UI for managing/viewing/editing all composite textures in
//              an archive.
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
#include "TextureXEditor.h"
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "Dialogs/ExtMessageDialog.h"
#include "General/ResourceManager.h"
#include "General/UndoRedo.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/MainWindow.h"
#include "UI/Controls/PaletteChooser.h"
#include "UI/Controls/UndoManagerHistoryPanel.h"
#include "UI/WxUtils.h"


// -----------------------------------------------------------------------------
// CreateTextureXDialog Class
//
// A simple dialog that provides options to create new (empty) texture
// definitions or import them from the base resource archive.
// Also has options for the desired texture format (doom, strife or TEXTURES)
// -----------------------------------------------------------------------------
class CreateTextureXDialog : public wxDialog
{
public:
	CreateTextureXDialog(wxWindow* parent) : wxDialog(parent, -1, "Create Texture Definitions")
	{
		// Setup layout
		auto m_vbox = new wxBoxSizer(wxVERTICAL);
		SetSizer(m_vbox);

		// --- Format options ---
		auto frame      = new wxStaticBox(this, -1, "Format");
		auto framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
		m_vbox->Add(framesizer, 0, wxEXPAND | wxALL, UI::pad());

		// Doom format
		rb_format_doom_ = new wxRadioButton(
			this, -1, "Doom (TEXTURE1 + PNAMES)", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
		rb_format_strife_   = new wxRadioButton(this, -1, "Strife (TEXTURE1 + PNAMES)");
		rb_format_textures_ = new wxRadioButton(this, -1, "ZDoom (TEXTURES)");
		WxUtils::layoutVertically(
			framesizer,
			{ rb_format_doom_, rb_format_strife_, rb_format_textures_ },
			wxSizerFlags(1).Expand().Border(wxALL, UI::pad()));


		// --- Source options ---
		frame      = new wxStaticBox(this, -1, "Source");
		framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
		m_vbox->Add(framesizer, 0, wxEXPAND | wxALL, UI::pad());

		// New list
		rb_new_ = new wxRadioButton(this, -1, "Create New (Empty)", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
		framesizer->Add(rb_new_, 0, wxEXPAND | wxALL, UI::pad());

		// Import from Base Resource Archive
		rb_import_bra_ = new wxRadioButton(this, -1, "Import from Base Resource Archive:");
		framesizer->Add(rb_import_bra_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, UI::pad());

		// Add buttons
		m_vbox->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, UI::pad());

		// Bind events
		rb_new_->Bind(wxEVT_RADIOBUTTON, &CreateTextureXDialog::onRadioNewSelected, this);
		rb_import_bra_->Bind(wxEVT_RADIOBUTTON, &CreateTextureXDialog::onRadioNewSelected, this);

		SetInitialSize(wxSize(-1, -1));
		wxWindowBase::Layout();
	}

	~CreateTextureXDialog() = default;

	TextureXList::Format getSelectedFormat() const
	{
		if (rb_format_doom_->GetValue())
			return TextureXList::Format::Normal;
		else if (rb_format_strife_->GetValue())
			return TextureXList::Format::Strife11;
		else if (rb_format_textures_->GetValue())
			return TextureXList::Format::Textures;
		else
			return TextureXList::Format::Normal;
	}

	bool createNewSelected() const { return rb_new_->GetValue(); }

	void onRadioNewSelected(wxCommandEvent& e)
	{
		// Enable/Disable format selection depending on rb_new state
		if (rb_new_->GetValue())
		{
			rb_format_doom_->Enable(true);
			rb_format_strife_->Enable(true);
			rb_format_textures_->Enable(true);
		}
		else
		{
			rb_format_doom_->Enable(false);
			rb_format_strife_->Enable(false);
			rb_format_textures_->Enable(false);
		}
	}

private:
	wxRadioButton* rb_format_doom_;
	wxRadioButton* rb_format_strife_;
	wxRadioButton* rb_format_textures_;

	wxRadioButton* rb_new_;
	wxRadioButton* rb_import_bra_;
};


// -----------------------------------------------------------------------------
//
// TextureXEditor Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// TextureXEditor class constructor
// -----------------------------------------------------------------------------
TextureXEditor::TextureXEditor(wxWindow* parent) : wxPanel(parent, -1)
{
	// Init variables
	undo_manager_ = std::make_unique<UndoManager>();
	wxWindowBase::SetName("texturex");

	// Create texture menu
	menu_texture_ = new wxMenu();
	SAction::fromId("txed_new")->addToMenu(menu_texture_);
	SAction::fromId("txed_new_patch")->addToMenu(menu_texture_);
	SAction::fromId("txed_new_file")->addToMenu(menu_texture_);
	SAction::fromId("txed_delete")->addToMenu(menu_texture_);
	menu_texture_->AppendSeparator();
	SAction::fromId("txed_rename")->addToMenu(menu_texture_);
	SAction::fromId("txed_rename_each")->addToMenu(menu_texture_);
	auto menu_export = new wxMenu();
	SAction::fromId("txed_export")->addToMenu(menu_export, "Archive (as image)");
	SAction::fromId("txed_extract")->addToMenu(menu_export, "File");
	menu_texture_->AppendSubMenu(menu_export, "&Export To");
	menu_texture_->AppendSeparator();
	SAction::fromId("txed_copy")->addToMenu(menu_texture_);
	SAction::fromId("txed_cut")->addToMenu(menu_texture_);
	SAction::fromId("txed_paste")->addToMenu(menu_texture_);
	menu_texture_->AppendSeparator();
	SAction::fromId("txed_up")->addToMenu(menu_texture_);
	SAction::fromId("txed_down")->addToMenu(menu_texture_);
	SAction::fromId("txed_sort")->addToMenu(menu_texture_);
	auto menu_patch = new wxMenu();
	SAction::fromId("txed_patch_add")->addToMenu(menu_patch);
	SAction::fromId("txed_patch_remove")->addToMenu(menu_patch);
	SAction::fromId("txed_patch_replace")->addToMenu(menu_patch);
	SAction::fromId("txed_patch_back")->addToMenu(menu_patch);
	SAction::fromId("txed_patch_forward")->addToMenu(menu_patch);
	SAction::fromId("txed_patch_duplicate")->addToMenu(menu_patch);
	menu_texture_->AppendSubMenu(menu_patch, "&Patch");

	// Create patch browser
	patch_browser_ = new PatchBrowser(theMainWindow);
	patch_browser_->CenterOnParent();
	patch_browser_->Show(false);

	// Setup sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Add tabs
	tabs_ = STabCtrl::createControl(this);
	sizer->Add(tabs_, 1, wxEXPAND | wxALL, UI::pad());

	// Bind events
	Bind(wxEVT_SHOW, &TextureXEditor::onShow, this);

	// Update patch browser & palette when resources are updated or the patch table is modified
	sc_resources_updated_ = App::resources().signals().resources_updated.connect([this]() {
		pb_update_ = true;
		updateTexturePalette();
	});
	sc_ptable_modified_   = patch_table_.signals().modified.connect([this]() {
        pb_update_ = true;
        updateTexturePalette();
    });

	// Update the editor palette if the main palette is changed
	sc_palette_changed_ = theMainWindow->paletteChooser()->signals().palette_changed.connect(
		[this]() { updateTexturePalette(); });

	// Update + layout
	updateTexturePalette();
	wxWindowBase::Layout();
	wxWindow::Show();
}

// -----------------------------------------------------------------------------
// TextureXEditor class destructor
// -----------------------------------------------------------------------------
TextureXEditor::~TextureXEditor()
{
	if (pnames_)
		pnames_->unlock();

	delete menu_texture_;
}

// -----------------------------------------------------------------------------
// Opens an archive to manage. Opens all texture related entries in the archive
// and sorts a bunch of stuff out. If no texture related entries exist in the
// archive, give the user options to create or import them
// -----------------------------------------------------------------------------
bool TextureXEditor::openArchive(Archive* archive)
{
	// Check any archive was given
	if (!archive)
		return false;

	// Setup archive texture entries
	if (!setupTextureEntries(archive))
		return false;

	// Search archive for any texture-related entries
	Archive::SearchOptions options;
	options.match_type = EntryType::fromId("texturex");
	auto tx_entries    = archive->findAll(options); // Find all TEXTUREx entries
	options.match_type = EntryType::fromId("pnames");
	auto entry_pnames  = archive->findLast(options); // Find last PNAMES entry

	// If any TEXTURE1/2 entries were found, setup patch table stuff
	if (!tx_entries.empty())
	{
		// Todo: Jaguar textures don't use PNAMES, so skip following checks if all texture entries are in Jaguar mode
		// TODO: Probably a better idea here to get the user to select an archive to import the patch table from
		// If no PNAMES entry was found, search resource archives
		if (!entry_pnames)
		{
			Archive::SearchOptions opt;
			opt.match_type = EntryType::fromId("pnames");
			entry_pnames   = App::archiveManager().findResourceEntry(opt, archive);
		}
		else
			pnames_ = entry_pnames; // If PNAMES was found in the archive,
									// set the class variable so it is written to if modified

		// If no PNAMES entry is found at all, show an error and abort
		// TODO: ask user to select appropriate base resource archive
		if (!entry_pnames)
		{
			wxMessageBox("PNAMES entry not found!", wxMessageBoxCaptionStr, wxICON_ERROR);
			return false;
		}

		// Load patch table
		patch_table_.loadPNAMES(entry_pnames, archive);
	}

	// Open texture editor tabs
	for (auto& tx_entrie : tx_entries)
	{
		auto tx_panel = new TextureXPanel(tabs_, *this);

		// Init texture panel
		tx_panel->Show(false);

		// Open TEXTUREX entry
		if (tx_panel->openTEXTUREX(tx_entrie))
		{
			// Set palette
			tx_panel->setPalette(theMainWindow->paletteChooser()->selectedPalette());
			// Lock entry
			tx_entrie->lock();

			// Add it to the list of editors, and a tab
			tx_panel->SetName("textures");
			texture_editors_.push_back(tx_panel);
			tabs_->AddPage(tx_panel, tx_entrie->name());
		}

		tx_panel->Show(true);
	}

	// Open patch table tab if needed
	if (pnames_)
	{
		auto ptp = new PatchTablePanel(tabs_, &patch_table_, this);
		tabs_->AddPage(ptp, "Patch Table (PNAMES)");
		ptp->SetName("pnames");
	}

	// Search archive for TEXTURES entries
	options.match_type = EntryType::fromId("zdtextures");
	auto ztx_entries   = archive->findAll(options);

	// Open texture editor tabs
	for (auto& ztx_entrie : ztx_entries)
	{
		auto tx_panel = new TextureXPanel(tabs_, *this);

		// Init texture panel
		tx_panel->Show(false);

		// Open TEXTURES entry
		if (tx_panel->openTEXTUREX(ztx_entrie))
		{
			// Set palette
			tx_panel->setPalette(theMainWindow->paletteChooser()->selectedPalette());
			// Lock entry
			ztx_entrie->lock();

			// Add it to the list of editors, and a tab
			tx_panel->SetName("textures");
			texture_editors_.push_back(tx_panel);
			tabs_->AddPage(tx_panel, ztx_entrie->name());
		}

		tx_panel->Show(true);
	}

	// Update layout
	Layout();
	tabs_->Refresh();

	// Update variables
	archive_         = archive;
	pnames_modified_ = false;

	// Lock pnames entry if it exists
	if (pnames_)
		pnames_->lock();

	// Set global palette
	theMainWindow->paletteChooser()->setGlobalFromArchive(archive);

	// Setup patch browser
	if (patch_table_.nPatches() > 0)
		patch_browser_->openPatchTable(&patch_table_);
	else
		patch_browser_->openArchive(archive);

	return true;
}

// -----------------------------------------------------------------------------
// Sets the texture panels' palettes to what is selected in the palette chooser
// -----------------------------------------------------------------------------
void TextureXEditor::updateTexturePalette()
{
	// Get palette
	auto pal = theMainWindow->paletteChooser()->selectedPalette();

	// Send to whatever needs it
	for (auto& texture_editor : texture_editors_)
		texture_editor->setPalette(pal);
	patch_browser_->setPalette(pal);
}

// -----------------------------------------------------------------------------
// Saves changes to all open TEXTUREx and PNAMES entries
// -----------------------------------------------------------------------------
void TextureXEditor::saveChanges()
{
	// Check for problems
	checkTextures();

	// Save TEXTUREx entries
	for (auto& texture_editor : texture_editors_)
		if (texture_editor->isModified())
			texture_editor->saveTEXTUREX();

	// Save PNAMES if it exists
	if (patch_table_.nPatches() > 0)
	{
		if (!pnames_)
		{
			// If no PNAMES entry exists in the archive, create one
			int index = archive_->entryIndex(texture_editors_.back()->txEntry()) + 1;
			pnames_   = archive_->addNewEntry("PNAMES", index).get();
			pnames_->setType(EntryType::fromId("pnames"));
			pnames_->setExtensionByType();
		}

		pnames_->unlock(); // Have to unlock it to write
		patch_table_.writePNAMES(pnames_);
		pnames_->lock();

		pnames_modified_ = false;
	}
}

// -----------------------------------------------------------------------------
// 'Closes' the texture editor, prompting the user to save any unsaved changes.
// Returns false if the close operation should be cancelled, true otherwise
// -----------------------------------------------------------------------------
bool TextureXEditor::close()
{
	// Check if any texture lists are modified
	bool modified = false;
	for (auto& texture_editor : texture_editors_)
	{
		texture_editor->applyChanges();
		if (texture_editor->isModified())
			modified = true;
	}

	// Check if patch table was modified
	if (pnames_modified_)
		modified = true;

	// Ask to save changes
	if (modified)
	{
		wxMessageDialog md(this, "Save changes to texture entries?", "Unsaved Changes", wxYES_NO | wxCANCEL);
		int             result = md.ShowModal();
		if (result == wxID_YES)
			saveChanges(); // User selected to save
		else if (result == wxID_CANCEL)
			return false; // User selected cancel, don't close the archive
	}

	return true;
}

// -----------------------------------------------------------------------------
// Shows or hides the custom 'Texture' menu
// -----------------------------------------------------------------------------
void TextureXEditor::showTextureMenu(bool show) const
{
	if (show)
		theMainWindow->addCustomMenu(menu_texture_, "&Texture");
	else
		theMainWindow->removeCustomMenu(menu_texture_);
}

// -----------------------------------------------------------------------------
// Removes the patch at [index] on the patch table from any textures that
// contain it (and from the patch table itself)
// -----------------------------------------------------------------------------
bool TextureXEditor::removePatch(unsigned index, bool delete_entry)
{
	// Get patch we're removing
	auto& p    = patch_table_.patch(index);
	auto  name = p.name;

	// Update TEXTUREx lists
	for (auto& texture_editor : texture_editors_)
		texture_editor->txList().removePatch(name);

	// Delete patch entry if it's part of this archive (and delete_entry is true)
	auto entry = App::resources().getPatchEntry(name, "patches", archive_);
	if (delete_entry && entry && entry->parent() == archive_)
		archive_->removeEntry(entry);

	// Remove patch from patch table
	patch_table_.removePatch(index);

	return true;
}

// -----------------------------------------------------------------------------
// Opens the patch table in the patch browser.
// Returns the selected patch index, or -1 if no patch was selected
// -----------------------------------------------------------------------------
int TextureXEditor::browsePatchTable(const wxString& first) const
{
	// Select initial patch if specified
	if (!first.IsEmpty())
		patch_browser_->selectPatch(first);

	if (patch_browser_->ShowModal() == wxID_OK)
		return patch_browser_->selectedPatch();
	else
		return -1;
}

// -----------------------------------------------------------------------------
// Opens resource patch entries in the patch browser.
// Returns the selected patch name, or "" if no patch was selected
// -----------------------------------------------------------------------------
wxString TextureXEditor::browsePatchEntry(const wxString& first)
{
	// Update patch browser if necessary
	if (pb_update_)
	{
		// Add archive textures (and resource textures)
		patch_browser_->openArchive(archive_);

		// Add each texture list from this archive
		for (auto& texture_editor : texture_editors_)
			patch_browser_->openTextureXList(&(texture_editor->txList()), archive_);

		pb_update_ = false;
	}

	// Select initial patch if specified
	if (!first.IsEmpty())
		patch_browser_->selectPatch(first);

	if (patch_browser_->ShowModal() == wxID_OK && patch_browser_->selectedItem())
		return patch_browser_->selectedItem()->name();
	else
		return "";
}

// -----------------------------------------------------------------------------
// Checks all texture definitions for problems and alerts the user if any are
// found. Returns true if any problems were found, false otherwise
// -----------------------------------------------------------------------------
bool TextureXEditor::checkTextures()
{
	wxString problems = wxEmptyString;

	// Go through all texturex lists
	for (auto& texture_editor : texture_editors_)
	{
		// Go through all textures
		for (unsigned t = 0; t < texture_editor->txList().size(); t++)
		{
			// Get texture
			auto tex = texture_editor->txList().texture(t);

			// Check its patches are all valid
			if (tex->isExtended())
			{
				// Extended texture, check if each patch exists in any open archive (or as a composite texture)
				for (unsigned p = 0; p < tex->nPatches(); p++)
				{
					auto pentry = App::resources().getPatchEntry(tex->patch(p)->name());
					auto fentry = App::resources().getFlatEntry(tex->patch(p)->name());
					auto ptex   = App::resources().getTexture(tex->patch(p)->name());
					if (!pentry && !fentry && !ptex)
						problems += wxString::Format(
							"Texture %s contains invalid/unknown patch %s\n", tex->name(), tex->patch(p)->name());
				}
			}
			else
			{
				// Regular texture, check the patch table
				for (unsigned p = 0; p < tex->nPatches(); p++)
				{
					if (patch_table_.patchIndex(tex->patch(p)->name()) == -1)
						problems += wxString::Format(
							"Texture %s contains invalid/unknown patch %s\n", tex->name(), tex->patch(p)->name());
				}
			}
		}
	}

	// Go through patch table
	for (unsigned a = 0; a < patch_table_.nPatches(); a++)
	{
		// Check patch entry is valid
		auto& patch = patch_table_.patch(a);
		auto  entry = App::resources().getPatchEntry(patch.name, "patches", archive_);

		if (!entry)
		{
			problems += wxString::Format("Patch %s cannot be found in any open archive\n", patch.name);
		}
		else
		{
			// Check patch entry type
			if (entry->type() == EntryType::unknownType())
				EntryType::detectEntryType(*entry);
			auto type = entry->type();

			if (!type->extraProps().propertyExists("patch"))
				problems += wxString::Format(
					"Patch %s is of type \"%s\", which is not a valid gfx format for patches. "
					"Convert it to either Doom Gfx or PNG\n",
					patch.name,
					type->name());
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

// -----------------------------------------------------------------------------
// Sets the active tab to be the one corresponding to the given entry index or
// entry.
// -----------------------------------------------------------------------------
void TextureXEditor::setSelection(size_t index) const
{
	if (index < tabs_->GetPageCount() && index != tabs_->GetSelection())
		tabs_->SetSelection(index);
}
void TextureXEditor::setSelection(ArchiveEntry* entry) const
{
	for (size_t a = 0; a < tabs_->GetPageCount(); a++)
	{
		if (S_CMPNOCASE(tabs_->GetPage(a)->GetName(), "pnames") && (entry == pnames_))
		{
			tabs_->SetSelection(a);
			return;
		}
		else if (S_CMPNOCASE(tabs_->GetPage(a)->GetName(), "textures"))
		{
			auto txp = dynamic_cast<TextureXPanel*>(tabs_->GetPage(a));
			if (txp->txEntry() == entry)
			{
				tabs_->SetSelection(a);
				return;
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Checks if the Texture menu needs to be displayed or not
// -----------------------------------------------------------------------------
void TextureXEditor::updateMenuStatus()
{
	if (tabs_->GetSelection() < 0)
		return;

	auto current = tabs_->GetPage(tabs_->GetSelection());

	// Check if the currently opened tab is a texturex list
	bool tex = false;
	for (auto& texture_editor : texture_editors_)
	{
		if (texture_editor == current)
		{
			tex = true;
			break;
		}
	}

	// Show/hide texture menu accordingly
	showTextureMenu(tex);
}

// -----------------------------------------------------------------------------
// Performs an undo operation
// -----------------------------------------------------------------------------
void TextureXEditor::undo()
{
	wxString action = undo_manager_->undo();
	if (!action.empty())
	{
		for (auto& texture_editor : texture_editors_)
			texture_editor->onUndo(action);
	}
}

// -----------------------------------------------------------------------------
// Performs an redo operation
// -----------------------------------------------------------------------------
void TextureXEditor::redo()
{
	wxString action = undo_manager_->redo();
	if (!action.empty())
	{
		for (auto& texture_editor : texture_editors_)
			texture_editor->onRedo(action);
	}
}


// -----------------------------------------------------------------------------
//
// TextureXEditor Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the panel is shown or hidden
// -----------------------------------------------------------------------------
void TextureXEditor::onShow(wxShowEvent& e)
{
	if (!e.IsShown())
	{
		showTextureMenu(false);
		return;
	}
	else
		theMainWindow->undoHistoryPanel()->setManager(undo_manager_.get());
	updateMenuStatus();
}


// -----------------------------------------------------------------------------
//
// TextureXEditor Class Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Static function to check if an archive has sufficient texture related
// entries, and if not, prompts the user to either create or import them.
// Returns true if the entries exist, false otherwise
// -----------------------------------------------------------------------------
bool TextureXEditor::setupTextureEntries(Archive* archive)
{
	using Format = TextureXList::Format;

	// Check any archive was given
	if (!archive)
		return false;

	// Search archive for any ZDoom TEXTURES entries
	Archive::SearchOptions options;
	options.match_type = EntryType::fromId("zdtextures");
	auto entry_tx      = archive->findFirst(options); // Find any TEXTURES entry

	// If it's found, we're done
	if (entry_tx)
		return true;


	// Search archive for any texture-related entries
	options.match_type = EntryType::fromId("texturex");
	entry_tx           = archive->findFirst(options); // Find any TEXTUREx entry
	options.match_type = EntryType::fromId("pnames");
	auto entry_pnames  = archive->findFirst(options); // Find any PNAMES entry

	// If both exist, we're done
	if (entry_tx && entry_pnames)
		return true;

	// Todo: accept entry_tx without pnames if the textures are in Jaguar mode

	// If no TEXTUREx entry exists
	if (!entry_tx)
	{
		// No TEXTUREx entries found, so ask if the user wishes to create one
		wxMessageDialog dlg(
			nullptr,
			"The archive does not contain any texture definitions (TEXTURE1/2 or TEXTURES). "
			"Do you wish to create or import a texture definition list?",
			"No Texture Definitions Found",
			wxYES_NO);

		if (dlg.ShowModal() == wxID_YES)
		{
			CreateTextureXDialog ctxd(nullptr);

			while (true)
			{
				// Check if cancelled
				if (ctxd.ShowModal() == wxID_CANCEL)
					return false;

				if (ctxd.createNewSelected())
				{
					// User selected to create a new TEXTUREx list
					ArchiveEntry* texturex = nullptr;

					// Doom or Strife TEXTUREx
					if (ctxd.getSelectedFormat() == Format::Normal || ctxd.getSelectedFormat() == Format::Strife11)
					{
						// Create texture list
						TextureXList txlist;
						txlist.setFormat(ctxd.getSelectedFormat());

						// Create patch table
						PatchTable ptt;

						// Create dummy patch
						auto dpatch = App::archiveManager().programResourceArchive()->entryAtPath("s3dummy.lmp");
						archive->addEntry(std::make_shared<ArchiveEntry>(*dpatch), "patches");
						ptt.addPatch("S3DUMMY");

						// Create dummy texture
						auto dummytex = std::make_unique<CTexture>();
						dummytex->setName("S3DUMMY");
						dummytex->addPatch("S3DUMMY", 0, 0);
						dummytex->setWidth(128);
						dummytex->setHeight(128);
						dummytex->setScale({ 0., 0. });

						// Add dummy texture to list
						// (this serves two purposes - supplies the special 'invalid' texture by default,
						//   and allows the texturex format to be detected)
						txlist.addTexture(std::move(dummytex));

						// Add empty PNAMES entry to archive
						entry_pnames = archive->addNewEntry("PNAMES").get();
						ptt.writePNAMES(entry_pnames);
						entry_pnames->setType(EntryType::fromId("pnames"));
						entry_pnames->setExtensionByType();

						// Add empty TEXTURE1 entry to archive
						texturex = archive->addNewEntry("TEXTURE1").get();
						txlist.writeTEXTUREXData(texturex, ptt);
						texturex->setType(EntryType::fromId("texturex"));
						texturex->setExtensionByType();
					}
					else if (ctxd.getSelectedFormat() == Format::Textures)
					{
						// Create texture list
						TextureXList txlist;
						txlist.setFormat(Format::Textures);

						// Add empty TEXTURES entry to archive
						texturex = archive->addNewEntry("TEXTURES").get();
						texturex->setType(EntryType::fromId("zdtextures"));
						texturex->setExtensionByType();

						return false;
					}

					if (!texturex)
						return false;
				}
				else
				{
					// User selected to import texture definitions from the base resource archive
					auto bra = App::archiveManager().baseResourceArchive();

					if (!bra)
					{
						wxMessageBox(
							"No Base Resource Archive is opened, please select/open one", "Error", wxICON_ERROR);
						continue;
					}

					// Find all relevant entries in the base resource archive
					Archive::SearchOptions opt;
					opt.match_type     = EntryType::fromId("texturex");
					auto import_tx     = bra->findAll(opt); // Find all TEXTUREx entries
					opt.match_type     = EntryType::fromId("pnames");
					auto import_pnames = bra->findLast(opt); // Find last PNAMES entry

					// Check enough entries exist
					if (import_tx.empty() || !import_pnames)
					{
						wxMessageBox(
							"The selected Base Resource Archive does not contain "
							"sufficient texture definition entries",
							"Error",
							wxICON_ERROR);
						continue;
					}

					// Copy TEXTUREx entries over to current archive
					for (auto& entry : import_tx)
					{
						auto texturex = archive->addEntry(std::make_shared<ArchiveEntry>(*entry), "global");
						texturex->setType(EntryType::fromId("texturex"));
						texturex->setExtensionByType();
					}

					// Copy PNAMES entry over to current archive
					entry_pnames = archive->addEntry(std::make_shared<ArchiveEntry>(*import_pnames), "global").get();
					entry_pnames->setType(EntryType::fromId("pnames"));
					entry_pnames->setExtensionByType();
				}

				break;
			}

			return true;
		}

		// 'No' clicked
		return false;
	}
	else // TEXTUREx entry exists
	{
		// TODO: Probably a better idea here to get the user to select an archive to import the patch table from
		// If no PNAMES entry was found, search resource archives
		if (!entry_pnames)
		{
			Archive::SearchOptions opt;
			opt.match_type = EntryType::fromId("pnames");
			entry_pnames   = App::archiveManager().findResourceEntry(opt, archive);
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
