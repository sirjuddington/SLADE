
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ArchivePanel.cpp
// Description: ArchivePanel class. The base wxWidgets panel for archive
//              content editing. One of these is opened in a tab for each open
//              archive.
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
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "ArchiveManagerPanel.h"
#include "ArchivePanel.h"
#include "Dialogs/GfxConvDialog.h"
#include "Dialogs/MapEditorConfigDialog.h"
#include "Dialogs/MapReplaceDialog.h"
#include "Dialogs/ModifyOffsetsDialog.h"
#include "Dialogs/Preferences/PreferencesDialog.h"
#include "Dialogs/RunDialog.h"
#include "Dialogs/TranslationEditorDialog.h"
#include "EntryPanel/AnimatedEntryPanel.h"
#include "EntryPanel/ANSIEntryPanel.h"
#include "EntryPanel/AudioEntryPanel.h"
#include "EntryPanel/DataEntryPanel.h"
#include "EntryPanel/DefaultEntryPanel.h"
#include "EntryPanel/GfxEntryPanel.h"
#include "EntryPanel/HexEntryPanel.h"
#include "EntryPanel/MapEntryPanel.h"
#include "EntryPanel/PaletteEntryPanel.h"
#include "EntryPanel/SwitchesEntryPanel.h"
#include "EntryPanel/TextEntryPanel.h"
#include "Game/Configuration.h"
#include "General/Clipboard.h"
#include "General/Executables.h"
#include "General/KeyBind.h"
#include "General/Misc.h"
#include "General/UI.h"
#include "Graphics/Icons.h"
#include "Graphics/Palette/PaletteManager.h"
#include "MainEditor/ArchiveOperations.h"
#include "MainEditor/Conversions.h"
#include "MainEditor/EntryOperations.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/MainWindow.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/UI/MapEditorWindow.h"
#include "UI/Controls/PaletteChooser.h"
#include "Utility/SFileDialog.h"
#include "Archive/Formats/ZipArchive.h"
#include "Scripting/ScriptManager.h"
#include "UI/Controls/SIconButton.h"


// ----------------------------------------------------------------------------
//
// Variables
//
// ----------------------------------------------------------------------------
namespace
{
	wxMenu*	menu_archive	= nullptr;
	wxMenu*	menu_entry		= nullptr;

	const auto	ERROR_UNWRITABLE_IMAGE_FORMAT =
		"Error: Could not write image data to entry %s, unsupported format for writing";
}
CVAR(Int, autosave_entry_changes, 2, CVAR_SAVE)	// 0=no, 1=yes, 2=ask
CVAR(Bool, confirm_entry_delete, true, CVAR_SAVE)
CVAR(Bool, context_submenus, true, CVAR_SAVE)
CVAR(String, last_colour, "RGB(255, 0, 0)", CVAR_SAVE)
CVAR(String, last_tint_colour, "RGB(255, 0, 0)", CVAR_SAVE)
CVAR(Int, last_tint_amount, 50, CVAR_SAVE)
CVAR(Bool, auto_entry_replace, false, CVAR_SAVE)
CVAR(Bool, archive_build_skip_hidden, true, CVAR_SAVE)


// ----------------------------------------------------------------------------
//
// External Variables
//
// ----------------------------------------------------------------------------
EXTERN_CVAR(String, path_pngout)
EXTERN_CVAR(String, path_pngcrush)
EXTERN_CVAR(String, path_deflopt)
EXTERN_CVAR(Bool, confirm_entry_revert)


// ----------------------------------------------------------------------------
// APEntryListDropTarget Class
//
// Handles drag'n'drop of files on to the entry list
// ----------------------------------------------------------------------------
class APEntryListDropTarget : public wxFileDropTarget
{
public:
	APEntryListDropTarget(ArchivePanel* parent, ArchiveEntryList* list) : parent_{ parent }, list_{ list } {}
	virtual ~APEntryListDropTarget() {}

	bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames) override
	{
		// Determine what item the files were dragged onto
		int flags;
		long index = list_->HitTest(wxPoint(x, y), flags) - list_->entriesBegin();

		// Add to end if no item was hit
		if (index < 0)
			index = list_->GetItemCount() - list_->entriesBegin();

		bool yes_to_all = false;
		string caption = (filenames.size() > 1) ? "Overwrite entries" : "Overwrite entry";

		// Import all dragged files, inserting after the item they were dragged onto
		for (int a = filenames.size()-1; a >= 0; a--)
		{
			// Is this a directory?
			if (wxDirExists(filenames[a]))
			{
				// TODO: Handle folders with recursively importing all content
				// and converting to namespaces if dropping in a treeless archive.
			}
			else
			{
				wxFileName fn(filenames[a]);
				ArchiveEntry* entry = nullptr;

				// Find entry to replace if needed
				if (auto_entry_replace)
				{
					entry = parent_->archive()->entryAtPath(list_->getCurrentDir()->getPath() + fn.GetFullName());
					// An entry with that name is already present, so ask about replacing it
					if (entry && !yes_to_all)
					{
						// Since there is no standard "Yes/No to all" button or "Don't ask me again" checkbox,
						// we will instead hack the Cancel button into being a "Yes to all" button. This is
						// despite the existence of a wxID_YESTOALL return value...
						string message = S_FMT(
							"Overwrite existing entry %s%s",
							list_->getCurrentDir()->getPath(),
							fn.GetFullName()
						);
						wxMessageDialog dlg(parent_, message, caption, wxCANCEL|wxYES_NO|wxCENTRE);
						dlg.SetYesNoCancelLabels(_("Yes"), _("No"), _("Yes to all"));
						int result = dlg.ShowModal();

						// User doesn't want to replace the entry
						if (result == wxID_NO)			entry = nullptr;
						// User wants to replace all entries
						if (result == wxID_CANCEL)		yes_to_all = true;
					}
				}

				// Create new entry if needed
				if (entry == nullptr)
					entry = parent_->archive()->addNewEntry(fn.GetFullName(), index, list_->getCurrentDir());

				// Import the file to it
				entry->importFile(filenames[a]);
				EntryType::detectEntryType(entry);
			}
		}

		return true;
	}

private:
	ArchivePanel*		parent_;
	ArchiveEntryList*	list_;
};

// ----------------------------------------------------------------------------
// ChoosePaletteDialog Class
//
// A simple dialog for the 'Choose Palette' function when creating a new
// palette entry.
// ----------------------------------------------------------------------------
class ChoosePaletteDialog : public wxDialog
{
public:
	ChoosePaletteDialog(wxWindow* parent) :
		wxDialog{
			parent,
			-1,
			"Choose Base Palette",
			wxDefaultPosition,
			wxDefaultSize,
			wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER}
	{

		// Set dialog icon
		wxIcon icon;
		icon.CopyFromBitmap(Icons::getIcon(Icons::ENTRY, "palette"));
		SetIcon(icon);

		// Setup main sizer
		wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(sizer);

		// Add choose
		pal_chooser_ = new PaletteChooser(this, -1);
		sizer->Add(pal_chooser_, 0, wxEXPAND|wxALL, 4);

		sizer->Add(CreateButtonSizer(wxOK|wxCANCEL), 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

		// Init layout
		wxDialog::Layout();

		// Setup dialog size
		SetInitialSize(wxSize(-1, -1));
		wxDialog::SetMinSize(GetSize());
	}

	int getChoice()
	{
		return pal_chooser_->GetSelection();
	}

private:
	PaletteChooser*	pal_chooser_;
};


// ----------------------------------------------------------------------------
//
// Functions
//
// ----------------------------------------------------------------------------
namespace
{

// ----------------------------------------------------------------------------
// initNamespaceVector
//
// Creates a vector of namespaces in a predefined order
// ----------------------------------------------------------------------------
void initNamespaceVector(vector<string> &ns, bool flathack)
{
	ns.clear();
	if (flathack)
		ns.emplace_back("flats");
	ns.emplace_back("global");
	ns.emplace_back("colormaps");
	ns.emplace_back("acs");
	ns.emplace_back("maps");
	ns.emplace_back("sounds");
	ns.emplace_back("music");
	ns.emplace_back("voices");
	ns.emplace_back("voxels");
	ns.emplace_back("graphics");
	ns.emplace_back("sprites");
	ns.emplace_back("patches");
	ns.emplace_back("textures");
	ns.emplace_back("hires");
	if (!flathack)
		ns.emplace_back("flats");
}

// ----------------------------------------------------------------------------
// isInMap
//
// Checks through a mapdesc_t vector and returns which one, if any, the entry
// index is in, -1 otherwise
// ----------------------------------------------------------------------------
int isInMap(size_t index, vector<Archive::MapDesc> &maps)
{
	for (size_t m = 0; m < maps.size(); ++m)
	{
		size_t head_index = maps[m].head->getParentDir()->entryIndex(maps[m].head);
		size_t end_index = maps[m].head->getParentDir()->entryIndex(maps[m].end, head_index);
		if (index >= head_index && index <= end_index)
			return m;
	}
	return -1;
}

// ----------------------------------------------------------------------------
// getNamespaceNumber
//
// Returns the position of the given entry's detected namespace in the
// namespace vector. Also hacks around a bit to put less entries in the global
// namespace and allow sorting a bit by categories.
// ----------------------------------------------------------------------------
size_t getNamespaceNumber(ArchiveEntry * entry, size_t index, vector<string> &ns, vector<Archive::MapDesc> &maps)
{
	string ens = entry->getParent()->detectNamespace(index);
	if (S_CMPNOCASE(ens, "global"))
	{
		if (!maps.empty() && isInMap(index, maps) >= 0)
			ens = "maps";
		else if (S_CMPNOCASE(entry->getType()->category(), "Graphics"))
			ens = "graphics";
		else if (S_CMPNOCASE(entry->getType()->category(), "Audio"))
		{
			if (S_CMPNOCASE(entry->getType()->icon(), "music"))
				ens = "music";
			else ens = "sounds";
		}
	}
	for (size_t n = 0; n < ns.size(); ++n)
		if (S_CMPNOCASE(ns[n], ens))
			return n;

	ns.push_back(ens);
	return ns.size();
}

} // namespace (anonymous)


// ----------------------------------------------------------------------------
//
// ArchivePanel Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// ArchivePanel::ArchivePanel
//
// ArchivePanel class constructor
// ----------------------------------------------------------------------------
ArchivePanel::ArchivePanel(wxWindow* parent, Archive* archive)
	: wxPanel(parent, -1)
{
	// Init variables
	undo_manager_ = std::make_unique<UndoManager>();
	ignore_focus_change_ = false;
	ee_manager_ = std::make_unique<ExternalEditManager>();

	// Set archive
	this->archive_ = archive;
	listenTo(archive);

	// Create entry panels
	entry_area_ = new EntryPanel(this, "nil");
	default_area_ = new DefaultEntryPanel(this);
	text_area_ = new TextEntryPanel(this);
	gfx_area_ = new GfxEntryPanel(this);
	pal_area_ = new PaletteEntryPanel(this);
	animated_area_ = new AnimatedEntryPanel(this);
	switches_area_ = new SwitchesEntryPanel(this);
	hex_area_ = new HexEntryPanel(this);
	ansi_area_ = new ANSIEntryPanel(this);
	map_area_ = new MapEntryPanel(this);
	audio_area_ = new AudioEntryPanel(this);
	data_area_ = new DataEntryPanel(this);


	// --- Setup Layout ---

	// Create sizer
	wxBoxSizer* m_hbox = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(m_hbox);

	// Entry list panel

	// Create & set sizer & border
	wxStaticBox* frame = new wxStaticBox(this, -1, "Entries");
	wxStaticBoxSizer* framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	m_hbox->Add(framesizer, 0, wxEXPAND | wxLEFT | wxTOP | wxBOTTOM, UI::pad());


	// Create path display
	sizer_path_controls_ = new wxBoxSizer(wxHORIZONTAL);
	framesizer->Add(sizer_path_controls_, 0, wxEXPAND|wxLEFT|wxRIGHT, UI::pad());
	framesizer->AddSpacer(UI::px(UI::Size::PadMinimum));

	// Label
	label_path_ = new wxStaticText(this, -1, "Path:", wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_START);
	sizer_path_controls_->Add(label_path_, 1, wxRIGHT|wxALIGN_CENTER_VERTICAL, UI::pad());

	// 'Up' button
	btn_updir_ = new SIconButton(this, Icons::ENTRY, "upfolder");
	btn_updir_->Enable(false);
	sizer_path_controls_->Add(btn_updir_, 0, wxEXPAND);


	// Create entry list panel
	entry_list_ = new ArchiveEntryList(this);
	entry_list_->setArchive(archive);
	entry_list_->SetDropTarget(new APEntryListDropTarget(this, entry_list_));
	entry_list_->setUndoManager(undo_manager_.get());
	framesizer->Add(entry_list_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, UI::pad());


	wxGridBagSizer* gb_sizer = new wxGridBagSizer(UI::pad(), UI::pad());
	framesizer->Add(gb_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, UI::pad());

	// Create category selector
	choice_category_ = new wxChoice(this, -1);
	choice_category_->Append("All");
	vector<string> cats = EntryType::allCategories();
	for (unsigned a = 0; a < cats.size(); a++)
		choice_category_->Append(cats[a]);
	choice_category_->SetSelection(0);
	gb_sizer->Add(new wxStaticText(this, -1, "Show:"), wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(choice_category_, wxGBPosition(0, 1), wxGBSpan(1, 2), wxEXPAND);
	gb_sizer->AddGrowableCol(1, 1);

	// Create filter
	text_filter_ = new wxTextCtrl(this, -1);
	gb_sizer->Add(new wxStaticText(this, -1, "Filter:"), wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(text_filter_, wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND);
	btn_clear_filter_ = new SIconButton(this, "close");
	btn_clear_filter_->SetToolTip("Clear Filter");
	gb_sizer->Add(btn_clear_filter_, wxGBPosition(1, 2), wxDefaultSpan, wxEXPAND);


	// Add default entry panel
	cur_area_ = entry_area_;
	m_hbox->Add(cur_area_, 1, wxEXPAND);
	cur_area_->Show(true);
	cur_area_->setUndoManager(undo_manager_.get());

	// Bind events
	entry_list_->Bind(EVT_VLV_SELECTION_CHANGED, &ArchivePanel::onEntryListSelectionChange, this);
#ifndef __WXGTK__
	entry_list_->Bind(wxEVT_LIST_ITEM_FOCUSED, &ArchivePanel::onEntryListFocusChange, this);
#endif
	entry_list_->Bind(wxEVT_KEY_DOWN, &ArchivePanel::onEntryListKeyDown, this);
	entry_list_->Bind(wxEVT_LIST_ITEM_RIGHT_CLICK, &ArchivePanel::onEntryListRightClick, this);
	entry_list_->Bind(wxEVT_LIST_ITEM_ACTIVATED, &ArchivePanel::onEntryListActivated, this);
	text_filter_->Bind(wxEVT_TEXT, &ArchivePanel::onTextFilterChanged, this);
	choice_category_->Bind(wxEVT_CHOICE, &ArchivePanel::onChoiceCategoryChanged, this);
	Bind(EVT_AEL_DIR_CHANGED, &ArchivePanel::onDirChanged, this);
	btn_updir_->Bind(wxEVT_BUTTON, &ArchivePanel::onBtnUpDir, this);
	btn_clear_filter_->Bind(wxEVT_BUTTON, &ArchivePanel::onBtnClearFilter, this);

	// Do a quick check to see if we need the path display
	if (archive->rootDir()->nChildren() == 0)
		sizer_path_controls_->Show(false);

	// Update size+layout
	entry_list_->updateWidth();
	wxPanel::Layout();
}

// ----------------------------------------------------------------------------
// ArchivePanel::saveEntryChanges
//
// Saves any changes made to the currently open entry
// ----------------------------------------------------------------------------
bool ArchivePanel::saveEntryChanges()
{
	// Ignore if no changes have been made (or no entry is open)
	if (!cur_area_->isModified() || !cur_area_->entry())
		return true;

	// Don't save if autosave is off
	if (autosave_entry_changes == 0)
		return false;

	// Ask if needed
	if (autosave_entry_changes > 1)
	{
		int result = wxMessageBox(S_FMT("Save changes to entry \"%s\"?", cur_area_->entry()->getName()),
		                          "Unsaved Changes", wxYES_NO|wxICON_QUESTION);

		// Stop if user clicked no
		if (result == wxNO)
			return false;
	}

	// Save entry changes
	return cur_area_->saveEntry();
}

// ----------------------------------------------------------------------------
// ArchivePanel::addMenus
//
// Adds the 'Archive' and 'Entry' menus to the main window menubar
// ----------------------------------------------------------------------------
void ArchivePanel::addMenus()
{
	// Create menus if needed
	if (!menu_archive)
	{
		// Archive menu
		wxMenu* menu_new = new wxMenu("");
		SAction::fromId("arch_newentry")->addToMenu(menu_new, "&Entry");
		SAction::fromId("arch_newdir")->addToMenu(menu_new, "&Directory");
		SAction::fromId("arch_newpalette")->addToMenu(menu_new, "&PLAYPAL");
		SAction::fromId("arch_newanimated")->addToMenu(menu_new, "&ANIMATED");
		SAction::fromId("arch_newswitches")->addToMenu(menu_new, "&SWITCHES");
		menu_archive = new wxMenu();
		menu_archive->AppendSubMenu(menu_new, "&New");
		SAction::fromId("arch_importfiles")->addToMenu(menu_archive);
		SAction::fromId("arch_buildarchive")->addToMenu(menu_archive);
		menu_archive->AppendSeparator();
		SAction::fromId("arch_texeditor")->addToMenu(menu_archive);
		SAction::fromId("arch_mapeditor")->addToMenu(menu_archive);
		wxMenu* menu_clean = new wxMenu("");
		SAction::fromId("arch_clean_patches")->addToMenu(menu_clean);
		SAction::fromId("arch_clean_textures")->addToMenu(menu_clean);
		SAction::fromId("arch_clean_flats")->addToMenu(menu_clean);
		SAction::fromId("arch_clean_iwaddupes")->addToMenu(menu_clean);
		SAction::fromId("arch_check_duplicates")->addToMenu(menu_clean);
		SAction::fromId("arch_check_duplicates2")->addToMenu(menu_clean);
		SAction::fromId("arch_replace_maps")->addToMenu(menu_clean);
		menu_archive->AppendSubMenu(menu_clean, "&Maintenance");
		auto menu_scripts = new wxMenu();
		ScriptManager::populateEditorScriptMenu(menu_scripts, ScriptManager::ScriptType::Archive, "arch_script");
		menu_archive->AppendSubMenu(menu_scripts, "&Run Script");
	}
	if (!menu_entry)
	{
		// Entry menu
		menu_entry = new wxMenu();
		SAction::fromId("arch_entry_rename")->addToMenu(menu_entry);
		SAction::fromId("arch_entry_delete")->addToMenu(menu_entry);
		SAction::fromId("arch_entry_revert")->addToMenu(menu_entry);
		menu_entry->AppendSeparator();
		SAction::fromId("arch_entry_cut")->addToMenu(menu_entry);
		SAction::fromId("arch_entry_copy")->addToMenu(menu_entry);
		SAction::fromId("arch_entry_paste")->addToMenu(menu_entry);
		menu_entry->AppendSeparator();
		SAction::fromId("arch_entry_moveup")->addToMenu(menu_entry);
		SAction::fromId("arch_entry_movedown")->addToMenu(menu_entry);
		SAction::fromId("arch_entry_sort")->addToMenu(menu_entry);
		menu_entry->AppendSeparator();
		SAction::fromId("arch_entry_import")->addToMenu(menu_entry);
		SAction::fromId("arch_entry_export")->addToMenu(menu_entry);
		menu_entry->AppendSeparator();
		SAction::fromId("arch_entry_bookmark")->addToMenu(menu_entry);
		auto menu_scripts = new wxMenu();
		ScriptManager::populateEditorScriptMenu(menu_scripts, ScriptManager::ScriptType::Entry, "arch_entry_script");
		menu_entry->AppendSubMenu(menu_scripts, "&Run Script");
	}

	// Add them to the main window menubar
	MainEditor::window()->addCustomMenu(menu_archive, "&Archive");
	MainEditor::window()->addCustomMenu(menu_entry, "&Entry");
	cur_area_->addCustomMenu();

	// Also enable the related toolbars
	MainEditor::window()->enableToolBar("_archive");
	MainEditor::window()->enableToolBar("_entry");
}

// ----------------------------------------------------------------------------
// ArchivePanel::removeMenus
//
// Removes the 'Archive' and 'Entry' menus from the main window menubar
// ----------------------------------------------------------------------------
void ArchivePanel::removeMenus()
{
	// Remove ArchivePanel menus from the main window menubar
	MainEditor::window()->removeCustomMenu(menu_archive);
	MainEditor::window()->removeCustomMenu(menu_entry);
	cur_area_->removeCustomMenu();

	// Also disable the related toolbars
	MainEditor::window()->enableToolBar("_archive", false);
	MainEditor::window()->enableToolBar("_entry", false);
}

// ----------------------------------------------------------------------------
// ArchivePanel::undo
//
// Performs an undo operation
// ----------------------------------------------------------------------------
void ArchivePanel::undo()
{
	if (!(cur_area_ && cur_area_->undo()))
	{
		// Undo
		entry_list_->setEntriesAutoUpdate(false);
		undo_manager_->undo();
		entry_list_->setEntriesAutoUpdate(true);

		// setEntriesAutoUpdate blocks previous announce
		archive_->announce("entries_changed");
	}
}

// ----------------------------------------------------------------------------
// ArchivePanel::redo
//
// Performs a redo operation
// ----------------------------------------------------------------------------
void ArchivePanel::redo()
{
	if (!(cur_area_ && cur_area_->redo()))
	{
		// Redo
		entry_list_->setEntriesAutoUpdate(false);
		undo_manager_->redo();
		entry_list_->setEntriesAutoUpdate(true);

		// setEntriesAutoUpdate blocks previous announce
		archive_->announce("entries_changed");
	}
}

// ----------------------------------------------------------------------------
// ArchivePanel::save
//
// Saves the archive
// ----------------------------------------------------------------------------
bool ArchivePanel::save()
{
	// Check the archive exists
	if (!archive_)
		return false;

	// Save any changes in the current entry panel
	saveEntryChanges();

	// Check the archive has been previously saved
	if (!archive_->canSave())
		return saveAs();

	// Save the archive
	if (!archive_->save())
	{
		// If there was an error pop up a message box
		wxMessageBox(S_FMT("Error:\n%s", Global::error), "Error", wxICON_ERROR);
		return false;
	}

	// Refresh entry list
	entry_list_->updateList();

	return true;
}

// ----------------------------------------------------------------------------
// ArchivePanel::saveAs
//
// Saves the archive to a new file
// ----------------------------------------------------------------------------
bool ArchivePanel::saveAs()
{
	// Check the archive exists
	if (!archive_)
		return false;

	// Do save dialog
	SFileDialog::fd_info_t info;
	if (SFileDialog::saveFile(
		info,
		"Save Archive " + archive_->filename(false) + " As",
		archive_->fileExtensionString(),
		this))
	{
		// Save the archive
		if (!archive_->save(info.filenames[0]))
		{
			// If there was an error pop up a message box
			wxMessageBox(S_FMT("Error:\n%s", Global::error), "Error", wxICON_ERROR);
			return false;
		}
	}

	// Refresh entry list
	entry_list_->updateList();

	// Add as recent file
	App::archiveManager().addRecentFile(info.filenames[0]);

	return true;
}

// ----------------------------------------------------------------------------
// ArchivePanel::newEntry
//
// Adds a new entry to the archive after the last selected entry in the list.
// If nothing is selected it is added at the end of the list. Asks the user for
// a name for the new entry, and doesn't add one if no name is entered.
// Returns true if the entry was created, false otherwise.
// ----------------------------------------------------------------------------
bool ArchivePanel::newEntry(int type)
{
	// Prompt for new entry name if needed
	string name;
	switch (type)
	{
	default:
	case ENTRY_EMPTY:
		name = wxGetTextFromUser("Enter new entry name:", "New Entry");
		break;
	case ENTRY_PALETTE:
		name = "playpal.lmp";
		break;
	case ENTRY_ANIMATED:
		name = "animated.lmp";
		break;
	case ENTRY_SWITCHES:
		name = "switches.lmp";
		break;
	}

	// Check if any name was entered
	if (name.empty())
		return false;

	// Check for \ character (e.g., from Arch-Viles graphics). They have to be kept.
	if (archive_->formatId() == "wad" && name.length() <= 8
	        && (name.find('\\') != wxNOT_FOUND || name.find('/') != wxNOT_FOUND))
	{
	} // Don't process as a file name

	// Remove any path from the name, if any (for now)
	else
	{
		wxFileName fn(name);
		name = fn.GetFullName();
	}

	// Get the entry index of the last selected list item
	int index = archive_->entryIndex(entry_list_->getLastSelectedEntry(), entry_list_->getCurrentDir());

	// If something was selected, add 1 to the index so we add the new entry after the last selected
	if (index >= 0)
		index++;
	else
		index = -1;	// If not add to the end of the list

	// Add the entry to the archive
	undo_manager_->beginRecord("Add Entry");
	ArchiveEntry* new_entry = archive_->addNewEntry(name, index, entry_list_->getCurrentDir());
	undo_manager_->endRecord(true);

	// Deal with specific entry type that we may want created
	if (type && new_entry)
	{
		ArchiveEntry* e_import;
		MemChunk mc;
		ChoosePaletteDialog cp(this);
		switch (type)
		{
			// Import a palette from the available ones
		case ENTRY_PALETTE:
			if (cp.ShowModal() == wxID_OK)
			{
				Palette* pal;
				int choice = cp.getChoice();
				if (choice)
					pal = App::paletteManager()->getPalette(choice - 1);
				else pal = App::paletteManager()->globalPalette();
				pal->saveMem(mc);
			}
			else
			{
				mc.reSize(256 * 3);
			}
			new_entry->importMemChunk(mc);
			break;
			// Import the ZDoom definitions as a baseline
		case ENTRY_ANIMATED:
			e_import = App::archiveManager().programResourceArchive()->entryAtPath("animated.lmp");
			if (e_import)
				new_entry->importEntry(e_import);
			break;
			// Import the Boom definitions as a baseline
		case ENTRY_SWITCHES:
			e_import = App::archiveManager().programResourceArchive()->entryAtPath("switches.lmp");
			if (e_import)
				new_entry->importEntry(e_import);
			break;
			// This is just to silence compilers that insist on default cases being handled
		default:
			break;
		}
	}

	// Return whether the entry was created ok
	return !!new_entry;
}

// ----------------------------------------------------------------------------
// ArchivePanel::newDirectory
//
// Adds a new subdirectory to the current directory, but only if the archive
// supports them
// ----------------------------------------------------------------------------
bool ArchivePanel::newDirectory()
{
	// Check archive supports directories
	if (!archive_->formatDesc().supports_dirs)
	{
		wxMessageBox("This Archive format does not support directories", "Can't create new directory", wxICON_ERROR);
		return false;
	}

	// Prompt for new directory name
	string name = wxGetTextFromUser("Enter new directory name:", "New Directory");

	// Check if any name was entered
	if (name.empty())
		return false;

	// Remove any path from the name, if any (for now)
	wxFileName fn(name);
	name = fn.GetFullName();

	// Add the directory to the archive
	undo_manager_->beginRecord("Create Directory");
	ArchiveTreeNode* dir = archive_->createDir(name, entry_list_->getCurrentDir());
	undo_manager_->endRecord(!!dir);

	// Return whether the directory was created ok
	return !!dir;
}

// ----------------------------------------------------------------------------
// ArchivePanel::importFiles
//
// Opens a file selection dialog and imports any selected files to the current
// directory, using the filenames as entry names
// ----------------------------------------------------------------------------
bool ArchivePanel::importFiles()
{
	// Run open files dialog
	SFileDialog::fd_info_t info;
	if (SFileDialog::openFiles(info, "Choose files to import", "Any File (*.*)|*.*", this))
	{
		// Get the entry index of the last selected list item
		int index = archive_->entryIndex(entry_list_->getLastSelectedEntry(), entry_list_->getCurrentDir());

		// If something was selected, add 1 to the index so we add the new entry after the last selected
		if (index >= 0)
			index++;
		else
			index = -1;	// If not add to the end of the list

		// Begin recording undo level
		undo_manager_->beginRecord("Import Files");

		// Go through the list of files
		bool ok = false;
		entry_list_->Show(false);
		UI::showSplash("Importing Files...", true);
		entry_list_->setEntriesAutoUpdate(false);
		for (size_t a = 0; a < info.filenames.size(); a++)
		{
			if (a == info.filenames.size()-1)
				entry_list_->setEntriesAutoUpdate(true);

			// Get filename
			string name = wxFileName(info.filenames[a]).GetFullName();

			// Update splash window
			UI::setSplashProgress(float(a) / float(info.filenames.size()));
			UI::setSplashProgressMessage(name);

			// Add the entry to the archive
			ArchiveEntry* new_entry = archive_->addNewEntry(name, index, entry_list_->getCurrentDir());

			// If the entry was created ok, load the file into it
			if (new_entry)
			{
				new_entry->importFile(info.filenames[a]);		// Import file to entry
				EntryType::detectEntryType(new_entry);	// Detect entry type
				ok = true;
			}

			if (index > 0) index++;
		}
		UI::hideSplash();
		entry_list_->Show(true);

		// End recording undo level
		undo_manager_->endRecord(true);

		entry_list_->setEntriesAutoUpdate(true);
		return ok;
	}
	else	// User cancelled, return false
		return false;
}

// ----------------------------------------------------------------------------
// ArchivePanel::convertArchiveTo
//
// Not implemented
// ----------------------------------------------------------------------------
bool ArchivePanel::convertArchiveTo()
{
	wxMessageBox("Not Implemented");
	return false;
}

// ----------------------------------------------------------------------------
// ArchivePanel::cleanupArchive
//
// Not implemented
// ----------------------------------------------------------------------------
bool ArchivePanel::cleanupArchive()
{
	wxMessageBox("Not Implemented");
	return false;
}

// ----------------------------------------------------------------------------
// ArchivePanel::buildArchive
//
// Build pk3/zip archive from the current directory
// ----------------------------------------------------------------------------
bool ArchivePanel::buildArchive()
{
	if (archive_->formatId() != "folder")
	{
		wxMessageBox("This function is only supported with directories", "Can't build archive", wxICON_ERROR);
		return false;
	}

	// Create temporary archive
	ZipArchive zip;

	// Create dialog
	SFileDialog::fd_info_t info;
	if (SFileDialog::saveFile(info, "Build archive", zip.fileExtensionString(), this))
	{
		UI::showSplash(string("Building ") + info.filenames[0], true);
		UI::setSplashProgress(0.0f);

		// prevent for "archive in archive" when saving in the current directory
		if(wxFileExists(info.filenames[0]))
			wxRemoveFile(info.filenames[0]);

		// Log
		UI::setSplashMessage("Importing files... (Esc to cancel)");

		// import all files into new archive
		// Get a list of all files in the directory
		wxArrayString files;
		wxDir::GetAllFiles(archive_->filename(), &files);

		// Go through files
		for (unsigned a = 0; a < files.size(); a++)
		{
			// Cancel event
			if (wxGetKeyState(WXK_ESCAPE))
			{
				UI::hideSplash();
				return true;
			}
			
			string name = files[a];
			name.Replace(archive_->filename(), "", false);	// Remove directory from entry name
	
			// Split filename into dir+name
			wxFileName fn(name);
			string ename = fn.GetFullName();
			string edir = fn.GetPath();

			// Remove beginning \ or / from dir
			if (edir.StartsWith("\\") || edir.StartsWith("/"))
				edir.Remove(0, 1);

			// Skip hidden files
			if (archive_build_skip_hidden && (edir[0] == '.' || ename[0] == '.'))
				continue;

			// Add the entry
			ArchiveTreeNode* dir = zip.createDir(edir);
			ArchiveEntry* entry = zip.addNewEntry(ename, dir->numEntries()+1, dir);

			// Log
			UI::setSplashProgressMessage(ename);
			UI::setSplashProgress((float)a/files.size());

			// Load data
			entry->importFile(files[a]);
	
			// Set unmodified
			entry->setState(0);
			dir->dirEntry()->setState(0);
		}

		UI::setSplashProgress(1.0f);
		UI::setSplashMessage("Saving archive...");
		UI::setSplashProgressMessage("");
		
		// Save the archive
		if (!zip.save(info.filenames[0]))
		{
			UI::hideSplash();

			// If there was an error pop up a message box
			wxMessageBox(S_FMT("Error:\n%s", Global::error), "Error", wxICON_ERROR);
			return false;
		}
	}

	UI::hideSplash();

	// Refresh entry list
	entry_list_->updateList();
	return true;
}

// ----------------------------------------------------------------------------
// ArchivePanel::renameEntry
//
// Opens a dialog to rename the selected entries. If multiple entries are
// selected, a mass-rename is performed
// ----------------------------------------------------------------------------
bool ArchivePanel::renameEntry(bool each)
{
	Global::error = "Rename failed";

	// Get a list of selected entries
	vector<ArchiveEntry*> selection = entry_list_->getSelectedEntries();

	// Begin recording undo level
	undo_manager_->beginRecord("Rename Entry");

	/* Define alphabet */
	const wxString alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	// Check any are selected
	if (each || selection.size() == 1)
	{
		// If only one entry is selected, or "rename each" mode is desired, just do basic rename
		entry_list_->setEntriesAutoUpdate(false);
		for (unsigned a = 0; a < selection.size(); a++)
		{
			if (a == selection.size()-1)
				entry_list_->setEntriesAutoUpdate(true);

			// Prompt for a new name
			string new_name = wxGetTextFromUser("Enter new entry name:", "Rename", selection[a]->getName());

			// Rename entry (if needed)
			if (!new_name.IsEmpty() && selection[a]->getName() != new_name)
			{
				if (!archive_->renameEntry(selection[a], new_name))
					wxMessageBox(
						S_FMT("Unable to rename entry %s: %s", CHR(selection[a]->getName()), CHR(Global::error)),
						"Rename Entry",
						wxICON_EXCLAMATION|wxOK
					);
			}
		}
	}
	else if (selection.size() > 1)
	{
		// Get a list of entry names
		wxArrayString names;
		for (auto& entry : selection)
			names.push_back(entry->getName(true));

		// Get filter string
		string filter = Misc::massRenameFilter(names);

		// Prompt for a new name
		string new_name = wxGetTextFromUser(
			"Enter new entry name: (* = unchanged, ^ = alphabet letter, ^^ = lower case\n% = alphabet repeat number, "
				"& = entry number, %% or && = n-1)",
			"Rename",
			filter
		);

		// Apply mass rename to list of names
		if (!new_name.IsEmpty())
		{
			Misc::doMassRename(names, new_name);

			// Go through the list
			entry_list_->setEntriesAutoUpdate(false);
			for (size_t a = 0; a < selection.size(); a++)
			{
				if (a == selection.size()-1)
					entry_list_->setEntriesAutoUpdate(true);

				ArchiveEntry* entry = selection[a];

				// If the entry is a folder then skip it
				if (entry->getType() == EntryType::folderType())
					continue;

				// Get current name as wxFileName for processing
				wxFileName fn(entry->getName());

				// Rename the entry (if needed)
				if (fn.GetName() != names[a])
				{
					wxString filename = names[a];
					/* file renaming syntax */
					int num = a / alphabet.size();
					int cn = a-(num*alphabet.size());
					filename.Replace("^^", alphabet.Lower()[cn]);
					filename.Replace("^", alphabet[cn]);
					filename.Replace("%%", wxString::FromDouble(num, 0));
					filename.Replace("%", wxString::FromDouble(num+1, 0));
					filename.Replace("&&", wxString::FromDouble(a, 0));
					filename.Replace("&", wxString::FromDouble(a+1, 0));
					fn.SetName(filename);							// Change name

					// Rename in archive
					if (!archive_->renameEntry(entry, fn.GetFullName()))
						wxMessageBox(
							S_FMT("Unable to rename entry %s: %s", CHR(selection[a]->getName()), CHR(Global::error)),
							"Rename Entry",
							wxICON_EXCLAMATION | wxOK
						);
				}
			}
		}
	}


	// Get a list of selected directories
	vector<ArchiveTreeNode*> selected_dirs = entry_list_->getSelectedDirectories();

	// Go through the list
	entry_list_->setEntriesAutoUpdate(false);
	for (size_t a = 0; a < selected_dirs.size(); a++)
	{
		if (a == selected_dirs.size()-1)
			entry_list_->setEntriesAutoUpdate(true);

		// Get the current directory's name
		string old_name = selected_dirs[a]->getName();

		// Prompt for a new name
		string new_name = wxGetTextFromUser(
			"Enter new directory name:",
			S_FMT("Rename Directory %s", old_name),
			old_name
		);

		// Do nothing if no name was entered
		if (new_name.IsEmpty())
			continue;

		// Discard any given path (for now)
		wxFileName fn(new_name);
		new_name = fn.GetFullName();

		// Rename the directory if the new entered name is different from the original
		if (new_name != old_name)
			archive_->renameDir(selected_dirs[a], new_name);
	}

	// Finish recording undo level
	undo_manager_->endRecord(true);

	entry_list_->setEntriesAutoUpdate(true);
	return true;
}

// ----------------------------------------------------------------------------
// ArchivePanel::deleteEntry
//
// Deletes any selected entries from the archive
// ----------------------------------------------------------------------------
bool ArchivePanel::deleteEntry(bool confirm)
{
	// Get a list of selected entries
	vector<ArchiveEntry*> selected_entries = entry_list_->getSelectedEntries();

	// Get a list of selected directories
	vector<ArchiveTreeNode*> selected_dirs = entry_list_->getSelectedDirectories();

	// Confirmation dialog
	if (confirm_entry_delete && confirm)
	{
		string item;
		int num = selected_entries.size() + selected_dirs.size();
		if (num == 1)
		{
			if (selected_entries.size() == 1)
				item = selected_entries[0]->getName();
			else
				item = selected_dirs[0]->getName();
		}
		else if (num > 0)
			item = S_FMT("these %d items", num);

		if (wxMessageBox(
				S_FMT("Are you sure you want to delete %s?", item),
				"Delete Confirmation",
				wxYES_NO|wxICON_QUESTION
			) != wxYES)
			return false;
	}

	// Clear the selection
	entry_list_->clearSelection();

	// Begin recording undo level
	undo_manager_->beginRecord("Delete Entry");

	// Go through the selected entries
	entry_list_->setEntriesAutoUpdate(false);
	for (int a = selected_entries.size() - 1; a >= 0; a--)
	{
		if (a==0)
			entry_list_->setEntriesAutoUpdate(true);

		// Remove from bookmarks
		App::archiveManager().deleteBookmark(selected_entries[a]);

		// Close entry tab if open
		MainEditor::window()->getArchiveManagerPanel()->closeEntryTab(selected_entries[a]);

		// Remove the current selected entry if it isn't a directory
		if (selected_entries[a]->getType() != EntryType::folderType())
			archive_->removeEntry(selected_entries[a]);
	}

	// Go through the selected directories
	entry_list_->setEntriesAutoUpdate(false);
	for (int a = selected_dirs.size() - 1; a >= 0; a--)
	{
		if (a==0)
			entry_list_->setEntriesAutoUpdate(true);

		// Remove from bookmarks
		App::archiveManager().deleteBookmarksInDir(selected_dirs[a]);

		// Close entry tabs
		for (const auto& entry : selected_dirs[a]->entries())
			MainEditor::window()->getArchiveManagerPanel()->closeEntryTab(entry.get());

		// Remove the selected directory from the archive
		archive_->removeDir(selected_dirs[a]->getName(), entry_list_->getCurrentDir());
	}
	entry_list_->setEntriesAutoUpdate(true);

	// Finish recording undo level
	undo_manager_->endRecord(true);

	// Switch to blank entry panel
	wxSizer* sizer = GetSizer();
	cur_area_->Show(false);
	cur_area_->nullEntry();
	sizer->Replace(cur_area_, entry_area_);
	cur_area_ = entry_area_;
	cur_area_->Show(true);
	Layout();

	return true;
}

// ----------------------------------------------------------------------------
// ArchivePanel::revertEntry
//
// Reverts any selected entries
// ----------------------------------------------------------------------------
bool ArchivePanel::revertEntry()
{
	// Prompt to revert if configured to
	if (confirm_entry_revert)
		if (wxMessageBox(
			"Are you sure you want to revert changes made to the entry?",
			"Revert Changes",
			wxICON_QUESTION | wxYES_NO) == wxNO)
			return false;

	// Get selected entries
	vector<ArchiveEntry*> selected_entries = entry_list_->getSelectedEntries();

	// Begin recording undo level
	undo_manager_->beginRecord("Revert Entry");

	// Go through selection
	for (auto& entry : selected_entries)
	{
		undo_manager_->recordUndoStep(new EntryDataUS(entry));
		archive_->revertEntry(entry);
	}

	// Finish recording undo level
	undo_manager_->endRecord(true);

	// Reload entry if currently open
	if (selected_entries.size() == 1 &&
		MainEditor::currentEntryPanel() &&
		MainEditor::currentEntryPanel()->entry() == selected_entries[0])
		MainEditor::currentEntryPanel()->openEntry(selected_entries[0]);

	// If the entries reverted were the only modified entries in the
	// archive, the archive is no longer modified.
	archive_->findModifiedEntries();

	return true;
}

// ----------------------------------------------------------------------------
// ArchivePanel::moveUp
//
// Moves any selected entries up in the list
// ----------------------------------------------------------------------------
bool ArchivePanel::moveUp()
{
	// Get selection
	vector<long> selection = entry_list_->getSelection();
	long focus = entry_list_->getFocus();

	// If nothing is selected, do nothing
	if (selection.empty())
		return false;

	// If the first selected item is at the top of the list
	// or before entries start then don't move anything up
	if (selection[0] <= entry_list_->entriesBegin())
		return false;

	// Move each one up by swapping it with the entry above it
	undo_manager_->beginRecord("Move Up");
	for (long index : selection)
		archive_->swapEntries(
			entry_list_->getEntryIndex(index),
			entry_list_->getEntryIndex(index -1),
			entry_list_->getCurrentDir()
		);
	undo_manager_->endRecord(true);

	// Update selection
	entry_list_->clearSelection();
	for (long index : selection)
		entry_list_->selectItem(index - 1);
	ignore_focus_change_ = true;
	entry_list_->focusItem(focus - 1);

	// Ensure top-most entry is visible
	entry_list_->EnsureVisible(entry_list_->getEntryIndex(selection[0]));

	// Return success
	return true;
}

// ----------------------------------------------------------------------------
// ArchivePanel::moveDown
//
// Moves any selected entries down in the list
// ----------------------------------------------------------------------------
bool ArchivePanel::moveDown()
{
	// Get selection
	vector<long> selection = entry_list_->getSelection();
	long focus = entry_list_->getFocus();

	// If nothing is selected, do nothing
	if (selection.empty())
		return false;

	// If the last selected item is at the end of the list
	// then don't move anything down
	if (selection.back() == entry_list_->GetItemCount()-1 || selection.back() < entry_list_->entriesBegin())
		return false;

	// Move each one down by swapping it with the entry below it
	undo_manager_->beginRecord("Move Down");
	for (int a = selection.size()-1; a >= 0; a--)
		archive_->swapEntries(
			entry_list_->getEntryIndex(selection[a]),
			entry_list_->getEntryIndex(selection[a]+1),
			entry_list_->getCurrentDir()
		);
	undo_manager_->endRecord(true);

	// Update selection
	entry_list_->clearSelection();
	for (long index : selection)
		entry_list_->selectItem(index + 1);
	ignore_focus_change_ = true;
	entry_list_->focusItem(focus + 1);

	// Ensure bottom-most entry is visible
	entry_list_->EnsureVisible(entry_list_->getEntryIndex(selection[selection.size() - 1]));

	// Return success
	return true;
}

// ----------------------------------------------------------------------------
// ArchivePanel::sort
//
// Sorts all selected entries. If the selection is empty or only contains one
// single entry, sort the entire archive instead.
// Note that a simple sort is not desired for three reasons:
// 1. Map lumps have to remain in sequence
// 2. Namespaces should be respected
// 3. Marker lumps used as separators should also be respected
// The way we're doing that is more than a bit hacky, sorry.
// The basic idea is to assign to each entry a sortkey (thanks to ExProps for
// that) which is prefixed with namespace information. Also, the name of map
// lumps is replaced by the map name so that they stay together. Finally, the
// original index is appended so that duplicate names are disambiguated.
// ----------------------------------------------------------------------------
bool ArchivePanel::sort()
{
	// Get selected entries
	vector<long> selection = entry_list_->getSelection();
	ArchiveTreeNode* dir = entry_list_->getCurrentDir();

	size_t start, stop;

	// Without selection of multiple entries, sort everything instead
	if (selection.size() < 2)
	{
		start = 0;
		stop = dir->numEntries();
	}
	// We need sorting to be contiguous, otherwise it'll destroy maps
	else
	{
		start = selection[0];
		stop = selection[selection.size() - 1] + 1;
	}

	// Make sure everything in the range is selected
	selection.clear();
	selection.resize(stop - start);
	for (size_t i = start; i < stop; ++i)
		selection[i - start] = i;

	// No sorting needed even after adding everything
	if (selection.size() < 2)
		return false;

	vector<string> nspaces;
	initNamespaceVector(nspaces, dir->archive()->hasFlatHack());
	vector<Archive::MapDesc> maps = dir->archive()->detectMaps();

	string ns = dir->archive()->detectNamespace(entry_list_->getEntry(selection[0]));
	size_t nsn = 0, lnsn = 0;

	// Fill a map with <entry name, entry index> pairs
	std::map<string, size_t> emap; emap.clear();
	for (size_t i = 0; i < selection.size(); ++i)
	{
		bool ns_changed = false;
		int mapindex = isInMap(selection[i], maps);
		string mapname;
		ArchiveEntry* entry = entry_list_->getEntry(selection[i]);
		if (!entry)
			continue;

		// Ignore subdirectories
		if (entry->getType() == EntryType::folderType())
			continue;

		// If this is a map entry, deal with it
		if (!maps.empty() && mapindex > -1)
		{
			// Keep track of the name
			mapname = maps[mapindex].name;

			// If part of a map is selected, make sure the rest is selected as well
			size_t head_index = maps[mapindex].head->getParentDir()->entryIndex(maps[mapindex].head);
			size_t end_index = maps[mapindex].head->getParentDir()->entryIndex(maps[mapindex].end, head_index);
			// Good thing we can rely on selection being contiguous
			for (size_t a = head_index; a <= end_index; ++a)
			{
				bool selected = (a >= start && a < stop);
				if (!selected) selection.push_back(a);
			}
			if (head_index < start) start = head_index;
			if (end_index+1 > stop) stop = end_index+1;
		}
		else if (dir->archive()->detectNamespace(selection[i]) != ns)
		{
			ns = dir->archive()->detectNamespace(selection[i]);
			nsn = getNamespaceNumber(entry, selection[i], nspaces, maps) * 1000;
			ns_changed = true;
		}
		else if (mapindex < 0 && (entry->getSize() == 0))
		{
			nsn++;
			ns_changed = true;
		}

		// Local namespace number is not necessarily computed namespace number.
		// This is because the global namespace in wads is bloated and we want more
		// categories than it actually has to offer.
		lnsn = (nsn == 0 ? getNamespaceNumber(entry, selection[i], nspaces, maps)*1000 : nsn);
		string name, ename = entry->getName().Upper();
		// Want to get another hack in this stuff? Yeah, of course you do!
		// This here hack will sort Doom II songs by their associated map.
		if (ename.StartsWith("D_") && S_CMPNOCASE(entry->getType()->icon(), "music"))
		{
			if		(ename == "D_RUNNIN")	ename = "D_MAP01";
			else if (ename == "D_STALKS")	ename = "D_MAP02";
			else if (ename == "D_COUNTD")	ename = "D_MAP03";
			else if (ename == "D_BETWEE")	ename = "D_MAP04";
			else if (ename == "D_DOOM"  )	ename = "D_MAP05";
			else if (ename == "D_THE_DA")	ename = "D_MAP06";
			else if (ename == "D_SHAWN" )	ename = "D_MAP07";
			else if (ename == "D_DDTBLU")	ename = "D_MAP08";
			else if (ename == "D_IN_CIT")	ename = "D_MAP09";
			else if (ename == "D_DEAD"  )	ename = "D_MAP10";
			else if (ename == "D_STLKS2")	ename = "D_MAP11";
			else if (ename == "D_THEDA2")	ename = "D_MAP12";
			else if (ename == "D_DOOM2" )	ename = "D_MAP13";
			else if (ename == "D_DDTBL2")	ename = "D_MAP14";
			else if (ename == "D_RUNNI2")	ename = "D_MAP15";
			else if (ename == "D_DEAD2" )	ename = "D_MAP16";
			else if (ename == "D_STLKS3")	ename = "D_MAP17";
			else if (ename == "D_ROMERO")	ename = "D_MAP18";
			else if (ename == "D_SHAWN2")	ename = "D_MAP19";
			else if (ename == "D_MESSAG")	ename = "D_MAP20";
			else if (ename == "D_COUNT2")	ename = "D_MAP21";
			else if (ename == "D_DDTBL3")	ename = "D_MAP22";
			else if (ename == "D_AMPIE" )	ename = "D_MAP23";
			else if (ename == "D_THEDA3")	ename = "D_MAP24";
			else if (ename == "D_ADRIAN")	ename = "D_MAP25";
			else if (ename == "D_MESSG2")	ename = "D_MAP26";
			else if (ename == "D_ROMER2")	ename = "D_MAP27";
			else if (ename == "D_TENSE" )	ename = "D_MAP28";
			else if (ename == "D_SHAWN3")	ename = "D_MAP29";
			else if (ename == "D_OPENIN")	ename = "D_MAP30";
			else if (ename == "D_EVIL"  )	ename = "D_MAP31";
			else if (ename == "D_ULTIMA")	ename = "D_MAP32";
			else if (ename == "D_READ_M")	ename = "D_MAP33";
			else if (ename == "D_DM2TTL")	ename = "D_MAP34";
			else if (ename == "D_DM2INT")	ename = "D_MAP35";
		}
		// All map lumps have the same sortkey name so they stay grouped
		if (mapindex > -1)
		{
			name = S_FMT("%08d%-64s%8d", lnsn, mapname, selection[i]);
		}
		// Yet another hack! Make sure namespace start markers are first
		else if (ns_changed)
		{
			name = S_FMT("%08d%-64s%8d", lnsn, wxEmptyString, selection[i]);
		}
		// Generic case: actually use the entry name to sort
		else
		{
			name = S_FMT("%08d%-64s%8d", lnsn, ename, selection[i]);
		}
		// Let the entry remember how it was sorted this time
		entry->exProp("sortkey") = name;
		// Insert sortkey into entry map so it'll be sorted
		emap[name] = selection[i];
	}

	// And now, sort the entries based on the map
	undo_manager_->beginRecord("Sort Entries");
	std::map<string, size_t>::iterator itr = emap.begin();
	for (size_t i = start; i < stop; ++i, ++itr)
	{
		ArchiveEntry * entry = entry_list_->getEntry(i);

		// Ignore subdirectories
		if (entry->getType() == EntryType::folderType())
			continue;

		// If the entry isn't in its sorted place already
		if (i != (size_t)itr->second)
		{
			// Swap the texture in the spot with the sorted one
			dir->swapEntries(i, itr->second);

			// Update the position of the displaced texture in the emap
			string name = entry->exProp("sortkey");
			emap[name] = itr->second;
		}
	}
	undo_manager_->endRecord(true);

	// Refresh
	entry_list_->updateList();
	archive_->setModified(true);

	return true;
}

// ----------------------------------------------------------------------------
// ArchivePanel::bookmark
//
// Adds the currently focused archive entry to the list of bookmarks
// ----------------------------------------------------------------------------
bool ArchivePanel::bookmark()
{
	ArchiveEntry* entry = entry_list_->getFocusedEntry();

	if (entry)
	{
		App::archiveManager().addBookmark(entry_list_->getFocusedEntry());
		return true;
	}
	else
		return false;
}

// ----------------------------------------------------------------------------
// ArchivePanel::openTab
//
// Opens currently selected entries in separate tabs
// ----------------------------------------------------------------------------
bool ArchivePanel::openTab()
{
	// Get selected entries
	vector<ArchiveEntry*> selection = entry_list_->getSelectedEntries();

	// Open each in its own tab
	for (auto& entry : selection)
		MainEditor::openEntry(entry);

	return true;
}

// ----------------------------------------------------------------------------
// ArchivePanel::crc32
//
// Computes the CRC-32 checksums of the selected entries
// ----------------------------------------------------------------------------
bool ArchivePanel::crc32()
{
	// Get selected entries
	vector<ArchiveEntry*> selection = entry_list_->getSelectedEntries();

	// Compute CRC-32 checksums for each
	string checksums = "\nCRC-32:\n";
	for (auto& entry : selection)
	{
		uint32_t crc = entry->getMCData().crc();
		checksums += S_FMT("%s:\t%x\n", entry->getName(), crc);
	}
	LOG_MESSAGE(1, checksums);
	wxMessageBox(checksums);

	return true;
}

// ----------------------------------------------------------------------------
// ArchivePanel::convertEntryTo
//
// Not implemented
// ----------------------------------------------------------------------------
bool ArchivePanel::convertEntryTo()
{
	wxMessageBox("Not Implemented");
	return false;
}

// ----------------------------------------------------------------------------
// ArchivePanel::importEntry
//
// For each selected entry, opens a file selection dialog. The selected file
// will be imported into the entry
// ----------------------------------------------------------------------------
bool ArchivePanel::importEntry()
{
	// Get a list of selected entries
	vector<ArchiveEntry*> selection = entry_list_->getSelectedEntries();

	// Begin recording undo level
	undo_manager_->beginRecord("Import Entry");

	// Go through the list
	entry_list_->setEntriesAutoUpdate(false);
	for (auto& entry : selection)
	{
		// Run open file dialog
		SFileDialog::fd_info_t info;
		if (SFileDialog::openFile(info, "Import Entry \"" + entry->getName() + "\"", "Any File (*.*)|*.*", this))
		{

			// Preserve gfx offset if needed
			point2_t offset;
			if (!entry->getType()->editor().Cmp("gfx"))
			{
				// We have an image
				SImage si;
				si.open(entry->getMCData());
				offset = si.offset();
			}

			// Create undo step
			undo_manager_->recordUndoStep(new EntryDataUS(entry));

			// If a file was selected, import it
			entry->importFile(info.filenames[0]);

			// Re-detect entry type
			EntryType::detectEntryType(entry);

			// Restore offsets if needed
			if (!entry->getType()->editor().Cmp("gfx"))
			{
				SImage si;
				si.open(entry->getMCData());

				point2_t noffset = si.offset();
				bool ok = true;
				// Don't bother if the same offsets are reimported
				if (offset == noffset) ok = false;
				// Ask for confirmation if there actually are offsets but they are different
				else if (noffset.x | noffset.y)
				{
					wxMessageDialog md(this,
					                   S_FMT("Image %s had offset [%d, %d], imported file has offset [%d, %d]. "
					                         "Do you want to keep the old offset and override the new?",
							entry->getName(), offset.x, offset.y, noffset.x, noffset.y),
					                   "Conflicting Offsets", wxYES_NO);
					int result = md.ShowModal();
					if (result != wxID_YES)
						ok = false;
				}
				// Warn if the offsets couldn't be written
				if (ok && si.getFormat() && !si.getFormat()->writeOffset(si, entry, offset))
					LOG_MESSAGE(1, "Old offset information [%d, %d] couldn't be "
					             "preserved in the new image format for image %s.",
					             offset.x, offset.y, entry->getName());
			}

			// Set extension by type
			entry->setExtensionByType();

			// If the entry is currently open, refresh the entry panel
			if (cur_area_->entry() == entry)
				openEntry(entry, true);
		}
	}
	entry_list_->setEntriesAutoUpdate(true);

	// Finish recording undo level
	undo_manager_->endRecord(true);

	return true;
}

// ----------------------------------------------------------------------------
// ArchivePanel::exportEntry
//
// Exports any selected entries to files. If multiple entries are selected, a
// directory selection dialog is shown, and any selected entries will be
// exported to that directory
// ----------------------------------------------------------------------------
bool ArchivePanel::exportEntry()
{
	// Get a list of selected entries & dirs
	vector<ArchiveEntry*> selection = entry_list_->getSelectedEntries();
	vector<ArchiveTreeNode*> selected_dirs = entry_list_->getSelectedDirectories();

	// If we're just exporting 1 entry
	if (selection.size() == 1 && selected_dirs.empty())
	{
		string name = Misc::lumpNameToFileName(selection[0]->getName());
		wxFileName fn(name);

		// Add appropriate extension if needed
		if (fn.GetExt().Len() == 0) fn.SetExt(selection[0]->getType()->extension());

		// Run save file dialog
		SFileDialog::fd_info_t info;
		if (SFileDialog::saveFile(
			info,
			"Export Entry \"" + selection[0]->getName() + "\"", "Any File (*.*)|*.*",
			this,
			fn.GetFullName()))
			selection[0]->exportFile(info.filenames[0]);	// Export entry if ok was clicked
	}
	else
	{
		// Run save files dialog
		SFileDialog::fd_info_t info;
		if (SFileDialog::saveFiles(info, "Export Multiple Entries (Filename is ignored)", "Any File (*.*)|*.*", this))
		{
			// Go through the selected entries
			for (auto& entry : selection)
			{
				// Setup entry filename
				wxFileName fn(entry->getName());
				fn.SetPath(info.path);

				// Add file extension if it doesn't exist
				if (!fn.HasExt())
					fn.SetExt(entry->getType()->extension());

				// Do export
				entry->exportFile(fn.GetFullPath());
			}

			// Go through selected dirs
			for (auto& dir : selected_dirs)
				dir->exportTo(info.path + "/" + dir->getName());
		}
	}

	return true;
}

// ----------------------------------------------------------------------------
// ArchivePanel::exportEntryAs
//
// Not implemented
// ----------------------------------------------------------------------------
bool ArchivePanel::exportEntryAs()
{
	wxMessageBox("Not Implemented");
	return false;
}

// ----------------------------------------------------------------------------
// ArchivePanel::copyEntry
//
// Copies selected entries+directories to the clipboard
// ----------------------------------------------------------------------------
bool ArchivePanel::copyEntry()
{
	// Get a list of selected entries
	vector<ArchiveEntry*> entries = entry_list_->getSelectedEntries();

	// Get a list of selected directories
	vector<ArchiveTreeNode*> dirs = entry_list_->getSelectedDirectories();

	// Do nothing if nothing selected
	if (entries.empty() && dirs.empty())
		return false;

	// Create clipboard item from selection
	theClipboard->addItem(new EntryTreeClipboardItem(entries, dirs));

	return true;
}

// ----------------------------------------------------------------------------
// ArchivePanel::cutEntry
//
// Copies selected entries+diretories to the clipboard, and deletes them from
// the archive
// ----------------------------------------------------------------------------
bool ArchivePanel::cutEntry()
{
	if (copyEntry())
		return deleteEntry(false);
	else
		return false;
}

// ----------------------------------------------------------------------------
// ArchivePanel::pasteEntry
//
// Pastes any entries and directories on the clipboard into the current
// directory. Entries will be pasted after the last selected entry, whereas
// directories will be pasted after any subdirectories. Pasting a directory
// will also paste any entries and subdirectories within it.
// ----------------------------------------------------------------------------
bool ArchivePanel::pasteEntry()
{
	// Do nothing if there is nothing in the clipboard
	if (theClipboard->nItems() == 0)
		return false;

	// Get the entry index of the last selected list item
	int index = archive_->entryIndex(entry_list_->getLastSelectedEntry(), entry_list_->getCurrentDir());

	// If something was selected, add 1 to the index so we add the new entry after the last selected
	if (index >= 0)
		index++;
	else
		index = -1;	// If not add to the end of the list
	
	// Go through all clipboard items
	ArchiveManagerPanel *panel = theMainWindow->getArchiveManagerPanel();
	panel->disableArchiveListUpdate();
	bool pasted = false;
	undo_manager_->beginRecord("Paste Entry");
	entry_list_->setEntriesAutoUpdate(false);
	for (unsigned a = 0; a < theClipboard->nItems(); a++)
	{
		// Check item type
		if (theClipboard->getItem(a)->getType() != CLIPBOARD_ENTRY_TREE)
			continue;

		// Get clipboard item
		EntryTreeClipboardItem* clip = (EntryTreeClipboardItem*)theClipboard->getItem(a);

		// Merge it in
		if (archive_->paste(clip->getTree(), index, entry_list_->getCurrentDir()))
			pasted = true;
	}
	undo_manager_->endRecord(true);
	entry_list_->setEntriesAutoUpdate(true);
	panel->refreshArchiveList();
	if (pasted)
	{
		// Update archive
		archive_->setModified(true);

		return true;
	}
	else
		return false;
}

// ----------------------------------------------------------------------------
// ArchivePanel::openEntryExternal
//
// Opens selected entries in their configured external editors
// ----------------------------------------------------------------------------
bool ArchivePanel::openEntryExternal()
{
	vector<ArchiveEntry*> selection = entry_list_->getSelectedEntries();
	for (auto& entry : selection)
	{
		// Open entry in selected external editor
		bool ok = ee_manager_->openEntryExternal(
			entry,
			current_external_exes_[wx_id_offset],
			current_external_exe_category_
		);

		// Show error message if failed
		if (!ok)
			wxMessageBox(
				S_FMT("Failed opening %s in external editor: %s", entry->getName(), Global::error),
				"External Edit Failed",
				wxOK | wxICON_ERROR
			);
	}

	return true;
}

// ----------------------------------------------------------------------------
// ArchivePanel::gfxConvert
//
// Opens the Gfx Conversion dialog and sends selected entries to it
// ----------------------------------------------------------------------------
bool ArchivePanel::gfxConvert()
{
	// Create gfx conversion dialog
	GfxConvDialog gcd(theMainWindow);

	// Send selection to the gcd
	vector<ArchiveEntry*> selection = entry_list_->getSelectedEntries();
	gcd.openEntries(selection);

	// Run the gcd
	gcd.ShowModal();

	// Show splash window
	UI::showSplash("Writing converted image data...", true);

	// Begin recording undo level
	undo_manager_->beginRecord("Gfx Format Conversion");

	// Write any changes
	entry_list_->setEntriesAutoUpdate(false);
	for (unsigned a = 0; a < selection.size(); a++)
	{
		if (a == selection.size()-1)
			entry_list_->setEntriesAutoUpdate(true);

		// Update splash window
		UI::setSplashProgressMessage(selection[a]->getName());
		UI::setSplashProgress((float)a / (float)selection.size());

		// Skip if the image wasn't converted
		if (!gcd.itemModified(a))
			continue;

		// Get image and conversion info
		SImage* image = gcd.getItemImage(a);
		SIFormat* format = gcd.getItemFormat(a);

		// Write converted image back to entry
		MemChunk mc;
		format->saveImage(*image, mc, gcd.getItemPalette(a));
		selection[a]->importMemChunk(mc);
		EntryType::detectEntryType(selection[a]);
		selection[a]->setExtensionByType();
	}
	entry_list_->setEntriesAutoUpdate(true);

	// Finish recording undo level
	undo_manager_->endRecord(true);

	// Hide splash window
	UI::hideSplash();
	MainEditor::currentEntryPanel()->callRefresh();

	return true;
}

// ----------------------------------------------------------------------------
// ArchivePanel::gfxRemap
//
// Opens the Translation editor dialog to remap colours on selected gfx entries
// ----------------------------------------------------------------------------
bool ArchivePanel::gfxRemap()
{
	// Get selected entries
	vector<ArchiveEntry*> selection = entry_list_->getSelectedEntries();

	// Create preview image (just use first selected entry)
	SImage image(PALMASK);
	Misc::loadImageFromEntry(&image, selection[0]);

	// Create translation editor dialog
	Palette* pal = theMainWindow->getPaletteChooser()->getSelectedPalette();
	TranslationEditorDialog ted(this, pal, "Colour Remap", &image);
	ted.openTranslation(((GfxEntryPanel*)gfx_area_)->prevTranslation());

	// Run dialog
	if (ted.ShowModal() == wxID_OK)
	{
		// Begin recording undo level
		undo_manager_->beginRecord("Gfx Colour Remap");

		// Apply translation to all entry images
		SImage temp;
		MemChunk mc;

		entry_list_->setEntriesAutoUpdate(false);
		for (unsigned a = 0; a < selection.size(); a++)
		{
			if (a == selection.size()-1)
				entry_list_->setEntriesAutoUpdate(true);

			ArchiveEntry* entry = selection[a];
			if (Misc::loadImageFromEntry(&temp, entry))
			{
				// Apply translation
				temp.applyTranslation(&ted.getTranslation(), pal);

				// Create undo step
				undo_manager_->recordUndoStep(new EntryDataUS(entry));

				// Write modified image data
				if (!temp.getFormat()->saveImage(temp, mc, pal))
					Log::error(1, S_FMT(ERROR_UNWRITABLE_IMAGE_FORMAT, entry->getName()));
				else
					entry->importMemChunk(mc);
			}
		}
		entry_list_->setEntriesAutoUpdate(true);

		// Update variables
		((GfxEntryPanel*)gfx_area_)->prevTranslation().copy(ted.getTranslation());

		// Finish recording undo level
		undo_manager_->endRecord(true);
	}
	MainEditor::currentEntryPanel()->callRefresh();

	return true;
}

// ----------------------------------------------------------------------------
// ArchivePanel::gfxColourise
//
// Opens the Colourise dialog to batch-colour selected gfx entries
// ----------------------------------------------------------------------------
bool ArchivePanel::gfxColourise()
{
	// Get selected entries
	vector<ArchiveEntry*> selection = entry_list_->getSelectedEntries();

	// Create colourise dialog
	Palette* pal = theMainWindow->getPaletteChooser()->getSelectedPalette();
	GfxColouriseDialog gcd(this, selection[0], pal);
	gcd.setColour(last_colour);

	// Run dialog
	if (gcd.ShowModal() == wxID_OK)
	{
		// Begin recording undo level
		undo_manager_->beginRecord("Gfx Colourise");

		// Apply translation to all entry images
		SImage temp;
		MemChunk mc;
		entry_list_->setEntriesAutoUpdate(false);
		for (unsigned a = 0; a < selection.size(); a++)
		{
			if (a == selection.size()-1)
				entry_list_->setEntriesAutoUpdate(true);

			ArchiveEntry* entry = selection[a];
			if (Misc::loadImageFromEntry(&temp, entry))
			{
				// Apply translation
				temp.colourise(gcd.getColour(), pal);

				// Create undo step
				undo_manager_->recordUndoStep(new EntryDataUS(entry));

				// Write modified image data
				if (!temp.getFormat()->saveImage(temp, mc, pal))
					LOG_MESSAGE(1, ERROR_UNWRITABLE_IMAGE_FORMAT, entry->getName());
				else
					entry->importMemChunk(mc);
			}
		}
		entry_list_->setEntriesAutoUpdate(true);
		// Finish recording undo level
		undo_manager_->endRecord(true);
	}
	rgba_t gcdcol = gcd.getColour();
	last_colour = S_FMT("RGB(%d, %d, %d)", gcdcol.r, gcdcol.g, gcdcol.b);
	MainEditor::currentEntryPanel()->callRefresh();

	return true;
}

// ----------------------------------------------------------------------------
// ArchivePanel::gfxTint
//
// Opens the Tint dialog to batch-colour selected gfx entries
// ----------------------------------------------------------------------------
bool ArchivePanel::gfxTint()
{
	// Get selected entries
	vector<ArchiveEntry*> selection = entry_list_->getSelectedEntries();

	// Create colourise dialog
	Palette* pal = theMainWindow->getPaletteChooser()->getSelectedPalette();
	GfxTintDialog gtd(this, selection[0], pal);
	gtd.setValues(last_tint_colour, last_tint_amount);

	// Run dialog
	if (gtd.ShowModal() == wxID_OK)
	{
		// Begin recording undo level
		undo_manager_->beginRecord("Gfx Tint");

		// Apply translation to all entry images
		SImage temp;
		MemChunk mc;
		entry_list_->setEntriesAutoUpdate(false);
		for (unsigned a = 0; a < selection.size(); a++)
		{
			if (a == selection.size()-1)
				entry_list_->setEntriesAutoUpdate(true);

			ArchiveEntry* entry = selection[a];
			if (Misc::loadImageFromEntry(&temp, entry))
			{
				// Apply translation
				temp.tint(gtd.getColour(), gtd.getAmount(), pal);

				// Create undo step
				undo_manager_->recordUndoStep(new EntryDataUS(entry));

				// Write modified image data
				if (!temp.getFormat()->saveImage(temp, mc, pal))
					LOG_MESSAGE(1, ERROR_UNWRITABLE_IMAGE_FORMAT, entry->getName());
				else
					entry->importMemChunk(mc);
			}
		}
		entry_list_->setEntriesAutoUpdate(true);

		// Finish recording undo level
		undo_manager_->endRecord(true);
	}
	rgba_t gtdcol = gtd.getColour();
	last_tint_colour = S_FMT("RGB(%d, %d, %d)", gtdcol.r, gtdcol.g, gtdcol.b);
	last_tint_amount = (int)(gtd.getAmount() * 100.0f);
	MainEditor::currentEntryPanel()->callRefresh();

	return true;
}

// ----------------------------------------------------------------------------
// ArchivePanel::gfxModifyOffsets
//
// Opens the Modify Offsets dialog to mass-modify offsets of any selected,
// offset-compatible gfx entries
// ----------------------------------------------------------------------------
bool ArchivePanel::gfxModifyOffsets()
{
	// Create modify offsets dialog
	ModifyOffsetsDialog mod;

	// Run the dialog
	if (mod.ShowModal() == wxID_CANCEL)
		return false;

	// Begin recording undo level
	undo_manager_->beginRecord("Gfx Modify Offsets");

	// Go through selected entries
	entry_list_->setEntriesAutoUpdate(false);
	vector<ArchiveEntry*> selection = entry_list_->getSelectedEntries();
	for (auto& entry : selection)
	{
		undo_manager_->recordUndoStep(new EntryDataUS(entry));
		EntryOperations::modifyGfxOffsets(entry, &mod);
	}
	MainEditor::currentEntryPanel()->callRefresh();
	entry_list_->setEntriesAutoUpdate(true);

	// Finish recording undo level
	undo_manager_->endRecord(true);

	return true;
}

// ----------------------------------------------------------------------------
// ArchivePanel::gfxExportPNG
//
// Exports any selected gfx entries as png format images
// ----------------------------------------------------------------------------
bool ArchivePanel::gfxExportPNG()
{
	// Get a list of selected entries
	vector<ArchiveEntry*> selection = entry_list_->getSelectedEntries();

	// If we're just exporting 1 entry
	if (selection.size() == 1)
	{
		string name = Misc::lumpNameToFileName(selection[0]->getName());
		wxFileName fn(name);

		// Set extension
		fn.SetExt("png");

		// Run save file dialog
		SFileDialog::fd_info_t info;
		if (SFileDialog::saveFile(
			info,
			"Export Entry \"" + selection[0]->getName() + "\" as PNG",
			"PNG Files (*.png)|*.png",
			this,
			fn.GetFullName()))
		{
			// If a filename was selected, export it
			if (!EntryOperations::exportAsPNG(selection[0], info.filenames[0]))
			{
				wxMessageBox(S_FMT("Error: %s", Global::error), "Error", wxOK|wxICON_ERROR);
				return false;
			}
		}

		return true;
	}
	else
	{
		// Run save files dialog
		SFileDialog::fd_info_t info;
		if (SFileDialog::saveFiles(
			info,
			"Export Entries as PNG (Filename will be ignored)",
			"PNG Files (*.png)|*.png",
			this))
		{
			// Go through the selection
			for (auto& entry : selection)
			{
				// Setup entry filename
				wxFileName fn(entry->getName());
				fn.SetPath(info.path);
				fn.SetExt("png");

				// Do export
				EntryOperations::exportAsPNG(entry, fn.GetFullPath());
			}
		}
	}

	return true;
}

// ----------------------------------------------------------------------------
// ArchivePanel::currentEntry
//
// Returns the entry currently open for editing
// ----------------------------------------------------------------------------
ArchiveEntry* ArchivePanel::currentEntry()
{
	if (entry_list_->GetSelectedItemCount() == 1)
		return cur_area_->entry();
	else
		return nullptr;
}

// ----------------------------------------------------------------------------
// ArchivePanel::currentEntries
//
// Returns a vector of all selected entries
// ----------------------------------------------------------------------------
vector<ArchiveEntry*> ArchivePanel::currentEntries()
{
	vector<ArchiveEntry*> selection;
	if (entry_list_)
		selection = entry_list_->getSelectedEntries();
	return selection;
}

// ----------------------------------------------------------------------------
// ArchivePanel::currentDir
//
// Returns a vector of all selected entries
// ----------------------------------------------------------------------------
ArchiveTreeNode* ArchivePanel::currentDir()
{
	if (entry_list_)
		return entry_list_->getCurrentDir();
	return nullptr;
}

// ----------------------------------------------------------------------------
// ArchivePanel::swanConvert
//
// Converts any selected SWANTBLS entries to SWITCHES and ANIMATED
// ----------------------------------------------------------------------------
bool ArchivePanel::swanConvert()
{
	// Get the entry index of the last selected list item
	int index = archive_->entryIndex(currentEntry(), entry_list_->getCurrentDir());

	// If something was selected, add 1 to the index so we add the new entry after the last selected
	if (index >= 0)
		index++;

	// MemChunks for ANIMATED and SWITCHES
	MemChunk mca, mcs;

	// Get a list of selected entries
	vector<ArchiveEntry*> selection = entry_list_->getSelectedEntries();

	bool error = false;

	// Check each selected entry for possible conversion
	for (auto& entry : selection)
	{
		if (entry->getType()->id() == "swantbls")
		{
			error |= !AnimatedList::convertSwanTbls(entry, &mca);
			error |= !SwitchesList::convertSwanTbls(entry, &mcs);
		}
	}

	// Close off ANIMATED lump if needed
	if (mca.getSize())
	{
		uint8_t buffer = 255;
		error |= !mca.reSize(mca.getSize() + 1, true);
		error |= !mca.write(&buffer, 1);
	}
	// Close off SWITCHES lump if needed
	if (mcs.getSize())
	{
		uint8_t buffer[20];
		memset(buffer, 0, 20);
		error |= !mcs.reSize(mcs.getSize() + 20, true);
		error |= !mcs.write(buffer, 20);
	}

	// Create entries
	MemChunk* mc[2] = {&mca, &mcs};
	string wadnames[2] = { "ANIMATED", "SWITCHES" };
	string zipnames[2] = { "animated.lmp", "switches.lmp" };
	string etypeids[2] = { "animated", "switches" };
	for (int e = 0; e < 2; ++e)
	{
		if (mc[e]->getSize())
		{
			// Begin recording undo level
			undo_manager_->beginRecord(S_FMT("Creating %s", wadnames[e]));

			ArchiveEntry * output = archive_->addNewEntry(
				(archive_->formatId() == "wad" ?  wadnames[e] : zipnames[e]), 
				index, entry_list_->getCurrentDir());
			if (output)
			{
				error |= !output->importMemChunk(*mc[e]);
				EntryType::detectEntryType(output);
				if (output->getType() == EntryType::unknownType())
					output->setType(EntryType::fromId(etypeids[e]));
				if (index >= 0) index++;
			}
			else error = true;
			// Finish recording undo level
			undo_manager_->endRecord(true);
		}
	}
	return !error;
}

// ----------------------------------------------------------------------------
// ArchivePanel::basConvert
//
// Converts any selected SWITCHES or ANIMATED entries to a newly created
// ANIMDEFS or SWANTBLS entry
// ----------------------------------------------------------------------------
bool ArchivePanel::basConvert(bool animdefs)
{
	// Get the entry index of the last selected list item
	int index = archive_->entryIndex(currentEntry(), entry_list_->getCurrentDir());

	// If something was selected, add 1 to the index so we add the new entry after the last selected
	if (index >= 0)
		index++;

	// Get a list of selected entries
	vector<ArchiveEntry*> selection = entry_list_->getSelectedEntries();

	// Begin recording undo level
	undo_manager_->beginRecord("Convert to ANIMDEFS");

	// Create new entry
	ArchiveEntry* output = archive_->addNewEntry(
		(animdefs
		? (archive_->formatId() == "wad" ? "ANIMDEFS" : "animdefs.txt")
		: (archive_->formatId() == "wad" ? "SWANTBLS" : "swantbls.dat")
		), index, entry_list_->getCurrentDir());

	// Finish recording undo level
	undo_manager_->endRecord(true);

	// Convert to ZDoom-compatible ANIMDEFS
	if (output)
	{
		// Create the memory buffer
		string gentext; 
		if (animdefs)
		{
			gentext = "// ANIMDEFS lump generated by SLADE3\n// on " + wxNow() + "\n\n";
		}
		else
		{
			gentext = "# SWANTBLS data generated by SLADE 3\n# on " + wxNow() + "\n#\n"
				"# This file is input for SWANTBLS.EXE, it specifies the switchnames\n"
				"# and animated textures and flats usable with BOOM. The output of\n"
				"# SWANTBLS is two lumps, SWITCHES.LMP and ANIMATED.LMP that should\n"
				"# be inserted in the PWAD as lumps.\n#\n";
		}

		MemChunk animdata(gentext.length());
		animdata.seek(0, SEEK_SET);
		animdata.write(CHR(gentext), gentext.length());

		// Check each selected entry for possible conversion
		for (auto& entry : selection)
		{
			if (entry->getType()->formatId() == "animated")
				AnimatedList::convertAnimated(entry, &animdata, animdefs);
			else if (entry->getType()->formatId() == "switches")
				SwitchesList::convertSwitches(entry, &animdata, animdefs);
		}
		output->importMemChunk(animdata);

		// Identify the new lump as what it is
		EntryType::detectEntryType(output);
		// Failsafe is detectEntryType doesn't accept to work, grumble
		if (output->getType() == EntryType::unknownType())
			output->setType(EntryType::fromId("animdefs"));
	}

	// Force entrylist width update
	Layout();

	return (output != nullptr);
}

// ----------------------------------------------------------------------------
// ArchivePanel::palConvert
//
// Unused (converts 6-bit palette to 8-bit)
// ----------------------------------------------------------------------------
bool ArchivePanel::palConvert()
{
	// Get the entry index of the last selected list item
	ArchiveEntry* pal6bit = currentEntry();
	const uint8_t* source = pal6bit->getData(true);
	uint8_t* dest = new uint8_t[pal6bit->getSize()];
	memcpy(dest, source, pal6bit->getSize());
	for (size_t i = 0; i < pal6bit->getSize(); ++i)
	{
		dest[i] = ((dest[i] << 2) | (dest[i] >> 4));
	}
	pal6bit->importMem(dest, pal6bit->getSize());
	MainEditor::currentEntryPanel()->callRefresh();
	delete[] dest;
	return true;
}

// ----------------------------------------------------------------------------
// ArchivePanel::wavDSndConvert
//
// Converts selected wav format entries to doom sound format
// ----------------------------------------------------------------------------
bool ArchivePanel::wavDSndConvert()
{
	// Get selected entries
	vector<ArchiveEntry*> selection = entry_list_->getSelectedEntries();

	// Begin recording undo level
	undo_manager_->beginRecord("Convert Wav -> Doom Sound");

	// Go through selection
	bool errors = false;
	entry_list_->setEntriesAutoUpdate(false);
	for (unsigned a = 0; a < selection.size(); a++)
	{
		if (a == selection.size()-1)
			entry_list_->setEntriesAutoUpdate(true);

		// Convert WAV -> Doom Sound if the entry is WAV format
		if (selection[a]->getType()->formatId() == "snd_wav")
		{
			MemChunk dsnd;
			// Attempt conversion
			if (!Conversions::wavToDoomSnd(selection[a]->getMCData(), dsnd))
			{
				LOG_MESSAGE(1, "Error: Unable to convert entry %s: %s", selection[a]->getName(), Global::error);
				errors = true;
				continue;
			}
			undo_manager_->recordUndoStep(new EntryDataUS(selection[a]));	// Create undo step
			selection[a]->importMemChunk(dsnd);								// Load doom sound data
			EntryType::detectEntryType(selection[a]);						// Update entry type
			selection[a]->setExtensionByType();								// Update extension if necessary
		}
	}
	entry_list_->setEntriesAutoUpdate(true);

	// Finish recording undo level
	undo_manager_->endRecord(true);

	// Show message if errors occurred
	if (errors)
		wxMessageBox("Some entries could not be converted, see console log for details", "SLADE", wxICON_INFORMATION);

	return true;
}

// ----------------------------------------------------------------------------
// ArchivePanel::dSndWavConvert
//
// Converts selected doom sound format entries to wav format
// ----------------------------------------------------------------------------
bool ArchivePanel::dSndWavConvert()
{
	// Get selected entries
	vector<ArchiveEntry*> selection = entry_list_->getSelectedEntries();

	// Begin recording undo level
	undo_manager_->beginRecord("Convert Doom Sound -> Wav");

	// Go through selection
	bool errors = false;
	entry_list_->setEntriesAutoUpdate(false);
	for (unsigned a = 0; a < selection.size(); a++)
	{
		if (a == selection.size()-1)
			entry_list_->setEntriesAutoUpdate(true);

		bool worked = false;
		MemChunk wav;
		// Convert Doom Sound -> WAV if the entry is Doom Sound format
		if (selection[a]->getType()->formatId() == "snd_doom" ||
		        selection[a]->getType()->formatId() == "snd_doom_mac")
			worked = Conversions::doomSndToWav(selection[a]->getMCData(), wav);
		// Or Doom Speaker sound format
		else if (selection[a]->getType()->formatId() == "snd_speaker")
			worked = Conversions::spkSndToWav(selection[a]->getMCData(), wav);
		// Or Jaguar Doom sound format
		else if (selection[a]->getType()->formatId() == "snd_jaguar")
			worked = Conversions::jagSndToWav(selection[a]->getMCData(), wav);
		// Or Wolfenstein 3D sound format
		else if (selection[a]->getType()->formatId() == "snd_wolf")
			worked = Conversions::wolfSndToWav(selection[a]->getMCData(), wav);
		// Or Creative Voice File format
		else if (selection[a]->getType()->formatId() == "snd_voc")
			worked = Conversions::vocToWav(selection[a]->getMCData(), wav);
		// Or Blood SFX format (this one needs to be given the entry, not just the mem chunk)
		else if (selection[a]->getType()->formatId() == "snd_bloodsfx")
			worked = Conversions::bloodToWav(selection[a], wav);
		// If successfully converted, update the entry
		if (worked)
		{
			undo_manager_->recordUndoStep(new EntryDataUS(selection[a]));	// Create undo step
			selection[a]->importMemChunk(wav);								// Load wav data
			EntryType::detectEntryType(selection[a]);						// Update entry type
			selection[a]->setExtensionByType();								// Update extension if necessary
		}
		else
		{
			LOG_MESSAGE(1, "Error: Unable to convert entry %s: %s", selection[a]->getName(), Global::error);
			errors = true;
			continue;
		}
	}
	entry_list_->setEntriesAutoUpdate(true);

	// Finish recording undo level
	undo_manager_->endRecord(true);

	// Show message if errors occurred
	if (errors)
		wxMessageBox("Some entries could not be converted, see console log for details", "SLADE", wxICON_INFORMATION);

	return true;
}

// ----------------------------------------------------------------------------
// ArchivePanel::musMidiConvert
//
// Converts selected mus format entries to midi format
// ----------------------------------------------------------------------------
bool ArchivePanel::musMidiConvert()
{
	// Get selected entries
	vector<ArchiveEntry*> selection = entry_list_->getSelectedEntries();

	// Begin recording undo level
	undo_manager_->beginRecord("Convert Mus -> Midi");

	// Go through selection
	entry_list_->setEntriesAutoUpdate(false);
	for (unsigned a = 0; a < selection.size(); a++)
	{
		if (a == selection.size()-1)
			entry_list_->setEntriesAutoUpdate(true);

		// Convert MUS -> MIDI if the entry is a MIDI-like format
		if (selection[a]->getType()->formatId().StartsWith("midi_") &&
			selection[a]->getType()->formatId() != "midi_smf")
		{
			MemChunk midi;
			undo_manager_->recordUndoStep(new EntryDataUS(selection[a]));	// Create undo step
			if (selection[a]->getType()->formatId() == "midi_mus")
				Conversions::musToMidi(selection[a]->getMCData(), midi);	// Convert
			else if (selection[a]->getType()->formatId() == "midi_gmid")
				Conversions::gmidToMidi(selection[a]->getMCData(), midi);	// Convert
			else
				Conversions::zmusToMidi(selection[a]->getMCData(), midi);	// Convert
			selection[a]->importMemChunk(midi);								// Load midi data
			EntryType::detectEntryType(selection[a]);						// Update entry type
			selection[a]->setExtensionByType();								// Update extension if necessary
		}

	}
	entry_list_->setEntriesAutoUpdate(true);

	// Finish recording undo level
	undo_manager_->endRecord(true);

	return true;
}

// ----------------------------------------------------------------------------
// ArchivePanel::compileACS
//
// Compiles any selected text entries as ACS scripts
// ----------------------------------------------------------------------------
bool ArchivePanel::compileACS(bool hexen)
{
	// Get selected entries
	vector<ArchiveEntry*> selection = entry_list_->getSelectedEntries();

	// Go through selection
	entry_list_->setEntriesAutoUpdate(false);
	for (unsigned a = 0; a < selection.size(); a++)
	{
		if (a == selection.size()-1)
			entry_list_->setEntriesAutoUpdate(true);
		// Compile ACS script
		EntryOperations::compileACS(selection[a], hexen, nullptr, theMainWindow);
	}
	entry_list_->setEntriesAutoUpdate(true);

	return true;
}

// ----------------------------------------------------------------------------
// ArchivePanel::optimizePNG
//
// Compiles any selected text entries as ACS scripts
// ----------------------------------------------------------------------------
bool ArchivePanel::optimizePNG()
{
	// Check if the PNG tools path are set up, at least one of them should be
	string pngpathc = path_pngcrush;
	string pngpatho = path_pngout;
	string pngpathd = path_deflopt;
	if ((pngpathc.IsEmpty() || !wxFileExists(pngpathc)) &&
	        (pngpatho.IsEmpty() || !wxFileExists(pngpatho)) &&
	        (pngpathd.IsEmpty() || !wxFileExists(pngpathd)))
	{
		wxMessageBox(
			"Error: PNG tool paths not defined or invalid, please configure in SLADE preferences",
			"Error",
			wxOK|wxCENTRE|wxICON_ERROR
		);
		return false;
	}

	// Get selected entries
	vector<ArchiveEntry*> selection = entry_list_->getSelectedEntries();

	UI::showSplash("Running external programs, please wait...", true);

	// Begin recording undo level
	undo_manager_->beginRecord("Optimize PNG");

	// Go through selection
	entry_list_->setEntriesAutoUpdate(false);
	for (unsigned a = 0; a < selection.size(); a++)
	{
		if (a == selection.size()-1)
			entry_list_->setEntriesAutoUpdate(true);

		UI::setSplashProgressMessage(selection[a]->getName(true));
		UI::setSplashProgress(float(a) / float(selection.size()));
		if (selection[a]->getType()->formatId() == "img_png")
		{
			undo_manager_->recordUndoStep(new EntryDataUS(selection[a]));
			EntryOperations::optimizePNG(selection[a]);
		}
	}
	entry_list_->setEntriesAutoUpdate(true);
	UI::hideSplash();

	// Finish recording undo level
	undo_manager_->endRecord(true);

	return true;
}

// ----------------------------------------------------------------------------
// ArchivePanel::convertTextures
//
// Converts any selected TEXTUREx entries to a ZDoom TEXTURES entry
// ----------------------------------------------------------------------------
bool ArchivePanel::convertTextures()
{
	// Get selected entries
	long index = entry_list_->getSelection()[0];
	vector<ArchiveEntry*> selection = entry_list_->getSelectedEntries();

	// Begin recording undo level
	undo_manager_->beginRecord("Convert TEXTUREx -> TEXTURES");

	// Do conversion
	if (EntryOperations::convertTextures(selection))
	{
		// Select new TEXTURES entry
		entry_list_->clearSelection();
		entry_list_->selectItem(index);

		// Finish recording undo level
		undo_manager_->endRecord(true);

		return true;
	}

	// Finish recording undo level
	undo_manager_->endRecord(false);

	return false;
}

// ----------------------------------------------------------------------------
// ArchivePanel::findTextureErrors
//
// Detect errors in a TEXTUREx entry
// ----------------------------------------------------------------------------
bool ArchivePanel::findTextureErrors()
{
	return EntryOperations::findTextureErrors(entry_list_->getSelectedEntries());
}

// ----------------------------------------------------------------------------
// ArchivePanel::mapOpenDb2
//
// Opens the currently selected entry in Doom Builder 2 if it is a valid map
// entry (either a map header or archive in maps/)
// ----------------------------------------------------------------------------
bool ArchivePanel::mapOpenDb2()
{
	// Get first selected entry
	ArchiveEntry* entry = entry_list_->getEntry(entry_list_->getFirstSelected());

	// Do open in db2
	return EntryOperations::openMapDB2(entry);
}

// ----------------------------------------------------------------------------
// ArchivePanel::openDir
//
// Opens the given directory.
// ----------------------------------------------------------------------------
bool ArchivePanel::openDir(ArchiveTreeNode* dir)
{
	return entry_list_->setDir(dir);
}

// ----------------------------------------------------------------------------
// ArchivePanel::closeCurrentEntry
//
// Closes the current entry in archive tab.
// ----------------------------------------------------------------------------
void ArchivePanel::closeCurrentEntry()
{
	// Close the current entry
	showEntryPanel(nullptr, false);
}

// ----------------------------------------------------------------------------
// ArchivePanel::openEntry
//
// Shows the appropriate entry area and sends the given entry to it.
// If [force] is true, the entry is opened even if it is already open
// ----------------------------------------------------------------------------
bool ArchivePanel::openEntry(ArchiveEntry* entry, bool force)
{
	// Null entry, do nothing
	if (!entry)
	{
		LOG_MESSAGE(1, "Warning: NULL entry focused in the list");
		return false;
	}

	// First check if the entry is already open in its own tab
	ArchiveManagerPanel* am_panel = theMainWindow->getArchiveManagerPanel();
	/*if (panel->redirectToTab(entry))
	{
		closeCurrentEntry();
		return true;
	}*/

	// Do nothing if the entry is already open
	if (cur_area_->entry() == entry && !force)
		return false;

	// Detect entry type if it hasn't been already
	if (entry->getType() == EntryType::unknownType())
		EntryType::detectEntryType(entry);

	// Are we trying to open a directory? This can happen from bookmarks.
	if (entry->getType() == EntryType::folderType())
	{
		// Removes starting / from path
		string name = entry->getPath(true);
		if (name.StartsWith("/"))
			name.Remove(0, 1);

		// Get directory to open
		auto dir = archive_->getDir(name, nullptr);

		// Check it exists (really should)
		if (!dir)
		{
			LOG_MESSAGE(1, "Error: Trying to open nonexistant directory %s", name);
			return false;
		}
		entry_list_->setDir(dir);
	}
	else
	{
		// Save changes if needed
		saveEntryChanges();

		// Close the current entry
		cur_area_->closeEntry();

		// Get the appropriate entry panel for the entry's type
		EntryPanel* new_area = default_area_;
		if (am_panel->entryIsOpenInTab(entry))
			new_area = default_area_;
		else if (entry->getType() == EntryType::mapMarkerType())
			new_area = map_area_;
		else if (!entry->getType()->editor().Cmp("gfx"))
			new_area = gfx_area_;
		else if (!entry->getType()->editor().Cmp("palette"))
			new_area = pal_area_;
		else if (!entry->getType()->editor().Cmp("ansi"))
			new_area = ansi_area_;
		else if (!entry->getType()->editor().Cmp("text"))
			new_area = text_area_;
		else if (!entry->getType()->editor().Cmp("animated"))
			new_area = animated_area_;
		else if (!entry->getType()->editor().Cmp("switches"))
			new_area = switches_area_;
		else if (!entry->getType()->editor().Cmp("audio"))
			new_area = audio_area_;
		else if (!entry->getType()->editor().Cmp("data"))
			new_area = data_area_;
		else if (!entry->getType()->editor().Cmp("default"))
			new_area = default_area_;
		else
			LOG_MESSAGE(1, "Entry editor %s does not exist, using default editor", entry->getType()->editor());

		// Load the entry into the panel
		if (!new_area->openEntry(entry))
			wxMessageBox(S_FMT("Error loading entry:\n%s", Global::error), "Error", wxOK|wxICON_ERROR);

		// Show the new entry panel
		bool changed = (cur_area_ != new_area);
		if (!showEntryPanel(new_area, false))
			return false;
		else if (changed)
			new_area->updateToolbar();
	}
	return true;
}

// ----------------------------------------------------------------------------
// ArchivePanel::openEntryAsText
//
// Opens [entry] in the text editor panel
// ----------------------------------------------------------------------------
bool ArchivePanel::openEntryAsText(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// First check if the entry is already open in its own tab
	ArchiveManagerPanel *panel = theMainWindow->getArchiveManagerPanel();
	if (panel->redirectToTab(entry))
	{
		//closeCurrentEntry();
		return true;
	}

	// Load the current entry into the panel
	if (!text_area_->openEntry(entry))
	{
		wxMessageBox(S_FMT("Error loading entry:\n%s", Global::error), "Error", wxOK|wxICON_ERROR);
	}

	// Show the text entry panel
	return showEntryPanel(text_area_);
}

// ----------------------------------------------------------------------------
// ArchivePanel::openEntryAsHex
//
// Opens [entry] in the hex editor panel
// ----------------------------------------------------------------------------
bool ArchivePanel::openEntryAsHex(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// First check if the entry is already open in its own tab
	ArchiveManagerPanel *panel = theMainWindow->getArchiveManagerPanel();
	if (panel->redirectToTab(entry))
	{
		//closeCurrentEntry();
		return true;
	}

	// Load the current entry into the panel
	if (!hex_area_->openEntry(entry))
	{
		wxMessageBox(S_FMT("Error loading entry:\n%s", Global::error), "Error", wxOK|wxICON_ERROR);
	}

	// Show the text entry panel
	return showEntryPanel(hex_area_);
}

// ----------------------------------------------------------------------------
// ArchivePanel::reloadCurrentPanel
//
// If only one entry is selected, force its reload
// ----------------------------------------------------------------------------
bool ArchivePanel::reloadCurrentPanel()
{
	// Do nothing if there are multiple entries
	if (cur_area_ == default_area_)
		return false;

	return openEntry(cur_area_->entry(), true);
}

// ----------------------------------------------------------------------------
// ArchivePanel::focusOnEntry
//
// Makes sure the list view display shows the given entry
// ----------------------------------------------------------------------------
void ArchivePanel::focusOnEntry(ArchiveEntry* entry)
{
	if (entry)
	{
		// Do we need to change directory?
		if (entry->getParentDir() != entry_list_->getCurrentDir())
			entry_list_->setDir(entry->getParentDir());

		// Now focus on the entry if it is listed
		for (long index = 0; index < entry_list_->GetItemCount(); ++index)
		{
			if (entry == entry_list_->getEntry(index))
			{
				entry_list_->focusOnIndex(index);
				return;
			}
		}
	}
}

// ----------------------------------------------------------------------------
// ArchivePanel::showEntryPanel
//
// Show an entry panel appropriate to the current entry
// ----------------------------------------------------------------------------
bool ArchivePanel::showEntryPanel(EntryPanel* new_area, bool ask_save)
{
	// Save any changes if needed
	if (ask_save)
		saveEntryChanges();

	// Get the panel sizer
	wxSizer* sizer = GetSizer();

	// If the new panel is different than the current, swap them
	if (new_area != cur_area_)
	{
		cur_area_->Show(false);				// Hide current
		cur_area_->removeCustomMenu();		// Remove current custom menu (if any)
		if (new_area != nullptr)
		{
			sizer->Replace(cur_area_, new_area);	// Swap the panels
			cur_area_ = new_area;				// Set the new panel to current
			cur_area_->Show(true);				// Show current

			// Add the current panel's custom menu and toolbar if needed
			cur_area_->addCustomMenu();
		}

		// Set panel undo manager
		cur_area_->setUndoManager(undo_manager_.get());

		// Update panel layout
		Layout();
		theMainWindow->Update();
		theMainWindow->Refresh();
		theMainWindow->Update();
	}
	else if (!cur_area_->IsShown())
	{
		// Show current
		cur_area_->Show();
	}

	return true;
}

// ----------------------------------------------------------------------------
// ArchivePanel::refreshPanel
//
// Refreshes everything on the panel
// ----------------------------------------------------------------------------
void ArchivePanel::refreshPanel()
{
	// Refresh entry list
	entry_list_->applyFilter();

	// Refresh current entry panel
	cur_area_->refreshPanel();

	// Refresh entire panel
	Update();
	Refresh();
}

// ----------------------------------------------------------------------------
// ArchivePanel::createEntryOpenMenu
//
// Creates and returns the 'Open In' submenu for the entry context menu, adding
// any external editors for entries of [category] 
// ----------------------------------------------------------------------------
wxMenu* ArchivePanel::createEntryOpenMenu(string category)
{
	current_external_exe_category_ = category;
	current_external_exes_.clear();
	wxMenu* menu_open = new wxMenu();

	// New Tab
	SAction::fromId("arch_entry_opentab")->addToMenu(menu_open, true);
	menu_open->AppendSeparator();

	// External editors
	vector<Executables::external_exe_t> external = Executables::getExternalExes(category);
	SAction* a_open_ext = SAction::fromId("arch_entry_openext");
	unsigned num = MIN(external.size(), 20);
	for (unsigned a = 0; a < num; a++)
	{
		a_open_ext->addToMenu(menu_open, "With " + external[a].name, "NO", a);
		current_external_exes_.push_back(external[a].name);
	}

	// Add separator if any external editors were added
	if (menu_open->GetMenuItemCount() > 2)
		menu_open->AppendSeparator();

	// Setup external editors
	SAction::fromId("arch_entry_setup_external")->addToMenu(menu_open);

	return menu_open;
}

// ----------------------------------------------------------------------------
// ArchivePanel::handleAction
//
// Handles the action [id].
// Returns true if the action was handled, false otherwise
// ----------------------------------------------------------------------------
bool ArchivePanel::handleAction(string id)
{
	// Don't handle actions if hidden
	if (!IsShown())
		return false;

	// We're only interested in "arch_" actions (and some others)
	if (!id.StartsWith("arch_") && !id.StartsWith("pmap_"))
		return false;


	// ------------------------------------------------------------------------
	// ARCHIVE MENU
	// ------------------------------------------------------------------------

	// Archive->New->Entry
	else if (id == "arch_newentry")
		newEntry();

	// Archive->New->Entry variants
	else if (id == "arch_newpalette")	newEntry(ENTRY_PALETTE);
	else if (id == "arch_newanimated")	newEntry(ENTRY_ANIMATED);
	else if (id == "arch_newswitches")	newEntry(ENTRY_SWITCHES);

	// Archive->New->Directory
	else if (id == "arch_newdir")
		newDirectory();

	// Archive->Import Files
	else if (id == "arch_importfiles")
		importFiles();

	// Archive->Build Archive
	else if (id == "arch_buildarchive")
		buildArchive();

	// Archive->Texture Editor
	else if (id == "arch_texeditor")
		MainEditor::openTextureEditor(archive_);

	else if (id == "arch_mapeditor")
		MainEditor::openMapEditor(archive_);

	// Archive->Convert To...
	else if (id == "arch_convert")
		convertArchiveTo();

	// Archive->Maintenance->Remove Unused Patches
	else if (id == "arch_clean_patches")
		ArchiveOperations::removeUnusedPatches(archive_);

	// Archive->Maintenance->Remove Unused Textures
	else if (id == "arch_clean_textures")
		ArchiveOperations::removeUnusedTextures(archive_);

	// Archive->Maintenance->Remove Unused Flats
	else if (id == "arch_clean_flats")
		ArchiveOperations::removeUnusedFlats(archive_);

	// Archive->Maintenance->Check Duplicate Entry Names
	else if (id == "arch_check_duplicates")
		ArchiveOperations::checkDuplicateEntryNames(archive_);

	// Archive->Maintenance->Check Duplicate Entry Names
	else if (id == "arch_check_duplicates2")
		ArchiveOperations::checkDuplicateEntryContent(archive_);

	// Archive->Maintenance->Check Duplicate Entry Names
	else if (id == "arch_clean_iwaddupes")
		ArchiveOperations::removeEntriesUnchangedFromIWAD(archive_);

	// Archive->Maintenance->Replace in Maps
	else if (id == "arch_replace_maps")
	{
		MapReplaceDialog dlg(this, archive_);
		dlg.ShowModal();
	}

	// Archive->Scripts->...
	else if (id == "arch_script")
		ScriptManager::runArchiveScript(archive_, wx_id_offset);


	// ------------------------------------------------------------------------
	// ENTRY MENU
	// ------------------------------------------------------------------------

	// Entry->Rename
	else if (id == "arch_entry_rename")
		renameEntry();

	// Entry->Rename Each
	else if (id == "arch_entry_rename_each")
		renameEntry(true);

	// Entry->Delete
	else if (id == "arch_entry_delete")
		deleteEntry();

	else if (id == "arch_entry_revert")
		revertEntry();

	// Entry->Cut
	else if (id == "arch_entry_cut")
		cutEntry();

	// Entry->Copy
	else if (id == "arch_entry_copy")
		copyEntry();

	// Entry->Paste
	else if (id == "arch_entry_paste")
		pasteEntry();

	// Entry->Move Up
	else if (id == "arch_entry_moveup")
		moveUp();

	// Entry->Move Down
	else if (id == "arch_entry_movedown")
		moveDown();

	// Entry->Sort
	else if (id == "arch_entry_sort")
		sort();

	// Entry->Bookmark
	else if (id == "arch_entry_bookmark")
		bookmark();

	// Open in Tab
	else if (id == "arch_entry_opentab")
		openTab();

	// CRC-32
	else if (id == "arch_entry_crc32")
		crc32();

	// Entry->Convert To...
	else if (id == "arch_entry_convert")
		convertEntryTo();

	// Entry->Import
	else if (id == "arch_entry_import")
		importEntry();

	// Entry->Export
	else if (id == "arch_entry_export")
		exportEntry();

	// Entry->Export As...
	else if (id == "arch_entry_exportas")
		exportEntryAs();

	// Entry->Open in External Editor
	else if (id == "arch_entry_openext")
		openEntryExternal();

	// Entry->Run Script
	else if (id == "arch_entry_script")
		ScriptManager::runEntryScript(entry_list_->getSelectedEntries(), wx_id_offset, MainEditor::windowWx());


	// Context menu actions
	else if (id == "arch_bas_convertb")
		basConvert(false);
	else if (id == "arch_bas_convertz")
		basConvert(true);
	else if (id == "arch_swan_convert")
		swanConvert();
	else if (id == "arch_gfx_convert")
		gfxConvert();
	else if (id == "arch_gfx_translate")
		gfxRemap();
	else if (id == "arch_gfx_colourise")
		gfxColourise();
	else if (id == "arch_gfx_tint")
		gfxTint();
	else if (id == "arch_gfx_offsets")
		gfxModifyOffsets();
	else if (id == "arch_gfx_addptable")
		EntryOperations::addToPatchTable(entry_list_->getSelectedEntries());
	else if (id == "arch_gfx_addtexturex")
		EntryOperations::createTexture(entry_list_->getSelectedEntries());
	else if (id == "arch_gfx_exportpng")
		gfxExportPNG();
	else if (id == "arch_gfx_pngopt")
		optimizePNG();
	else if (id == "arch_view_text")
		openEntryAsText(entry_list_->getFocusedEntry());
	else if (id == "arch_view_hex")
		openEntryAsHex(entry_list_->getFocusedEntry());
	else if (id == "arch_audio_convertdw")
		dSndWavConvert();
	else if (id == "arch_audio_convertwd")
		wavDSndConvert();
	else if (id == "arch_audio_convertmus")
		musMidiConvert();
	else if (id == "arch_scripts_compileacs")
		compileACS();
	else if (id == "arch_scripts_compilehacs")
		compileACS(true);
	else if (id == "arch_texturex_convertzd")
		convertTextures();
	else if (id == "arch_texturex_finderrors")
		findTextureErrors();
	else if (id == "arch_map_opendb2")
		mapOpenDb2();
	else if (id == "arch_entry_setup_external")
		PreferencesDialog::openPreferences(theMainWindow, "Editing", "external");

	// Map Editor Panel
	else if (id == "pmap_open_text")
	{
		// Edit Level Script

		// Get entry to edit
		ArchiveEntry* entry = map_area_->entry();

		// Open in text editor
		openEntryAsText(entry);
	}

	// Run archive
	else if (id == "arch_run")
	{
		RunDialog dlg(this, archive_);
		if (dlg.ShowModal() == wxID_OK)
		{
			string command = dlg.getSelectedCommandLine(archive_, "");
			if (!command.IsEmpty())
			{
				// Set working directory
				string wd = wxGetCwd();
				wxSetWorkingDirectory(dlg.getSelectedExeDir());

				// Run
				wxExecute(command, wxEXEC_ASYNC);

				// Restore working directory
				wxSetWorkingDirectory(wd);
			}
		}

		return true;
	}

	// Unknown action
	else
		return false;

	// Action handled, return true
	return true;
}

// ----------------------------------------------------------------------------
// ArchivePanel::onAnnouncement
//
// Called when an announcement is recieved from the archive that this
// ArchivePanel is managing
// ----------------------------------------------------------------------------
void ArchivePanel::onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data)
{
	// Reset event data for reading
	event_data.seek(0, SEEK_SET);

	// If the archive was saved
	if (announcer == archive_ && event_name == "saved")
	{
		// Update this tab's name in the parent notebook (if filename was changed)
		wxAuiNotebook* parent = (wxAuiNotebook*)GetParent();
		parent->SetPageText(parent->GetPageIndex(this), archive_->filename(false));
	}

	// If a directory was added
	if (announcer == archive_ && event_name == "directory_added")
	{
		// Show path controls (if they aren't already)
		wxSizer* sizer = GetSizer();
		wxSizerItem* item = sizer->GetItem(sizer_path_controls_, true);
		if (!item->IsShown())
		{
			item->Show(true);
			sizer->Layout();
		}
	}

	// If an entry was removed
	if (announcer == archive_ && event_name == "entry_removing")
	{
		// Get entry pointer
		wxUIntPtr ptr;
		event_data.seek(sizeof(int), 0);
		event_data.read(&ptr, sizeof(wxUIntPtr));
		ArchiveEntry* entry = (ArchiveEntry*)wxUIntToPtr(ptr);

		// Close current entry panel if it's entry was removed
		if (currentArea()->entry() == entry)
		{
			currentArea()->closeEntry();
			currentArea()->openEntry(nullptr);
			currentArea()->Show(false);
		}
	}
}


// ----------------------------------------------------------------------------
//
// ArchivePanel Static Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// ArchivePanel::createPanelForEntry
//
// Creates the appropriate EntryPanel for [entry] and returns it
// ----------------------------------------------------------------------------
EntryPanel* ArchivePanel::createPanelForEntry(ArchiveEntry* entry, wxWindow* parent)
{
	EntryPanel* entry_panel;

	if (entry->getType() == EntryType::mapMarkerType())
		entry_panel = new MapEntryPanel(parent);
	else if (!entry->getType()->editor().Cmp("gfx"))
		entry_panel = new GfxEntryPanel(parent);
	else if (!entry->getType()->editor().Cmp("palette"))
		entry_panel = new PaletteEntryPanel(parent);
	else if (!entry->getType()->editor().Cmp("ansi"))
		entry_panel = new ANSIEntryPanel(parent);
	else if (!entry->getType()->editor().Cmp("text"))
		entry_panel = new TextEntryPanel(parent);
	else if (!entry->getType()->editor().Cmp("animated"))
		entry_panel = new AnimatedEntryPanel(parent);
	else if (!entry->getType()->editor().Cmp("switches"))
		entry_panel = new SwitchesEntryPanel(parent);
	else if (!entry->getType()->editor().Cmp("audio"))
		entry_panel = new AudioEntryPanel(parent);
	else
		entry_panel = new DefaultEntryPanel(parent);

	return entry_panel;
}


// ----------------------------------------------------------------------------
//
// ArchivePanel Class Events
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// ArchivePanel::onEntryListSelectionChange
//
// Called when the selection on the entry list is changed
// ----------------------------------------------------------------------------
void ArchivePanel::onEntryListSelectionChange(wxCommandEvent& e)
{
	// Do nothing if not shown
	if (!IsShown())
		return;

	// Get selected entries
	vector<ArchiveEntry*> selection = entry_list_->getSelectedEntries();

	if (selection.empty())
		return;	// If no entries are selected do nothing
	else if (selection.size() == 1)
	{
		// If one entry is selected, open it in the entry area
		openEntry(selection[0]);
	}
	else
	{
		// If multiple entries are selected, show/update the multi entry area
		showEntryPanel(default_area_);
		((DefaultEntryPanel*)default_area_)->loadEntries(selection);
	}
}

// ----------------------------------------------------------------------------
// ArchivePanel::onEntryListFocusChange
//
// Called when the focused item on the entry list is changed
// ----------------------------------------------------------------------------
void ArchivePanel::onEntryListFocusChange(wxListEvent& e)
{
	// Do nothing if not shown
	if (!IsShown())
		return;

	// Ignore if needed (once)
	if (ignore_focus_change_)
	{
		ignore_focus_change_ = false;
		return;
	}

	// Get selected entries
	vector<ArchiveEntry*> selection = entry_list_->getSelectedEntries();

	if (selection.empty())
		return;	// If no entries are selected do nothing
	else if (selection.size() == 1)
	{
		// If one entry is selected, open it in the entry area
		openEntry(selection[0]);
		e.Skip();
	}
	else
	{
		// If multiple entries are selected, show/update the multi entry area
		showEntryPanel(default_area_);
		((DefaultEntryPanel*)default_area_)->loadEntries(selection);
	}
}

// ----------------------------------------------------------------------------
// ArchivePanel::onEntryListRightClick
//
// Called when the entry list is right clicked
// ----------------------------------------------------------------------------
void ArchivePanel::onEntryListRightClick(wxListEvent& e)
{
	// Get selected entries
	vector<ArchiveEntry*> selection = entry_list_->getSelectedEntries();

	// Check what types exist in the selection
	// TODO: This stuff is absolutely terrible, nicer system needed
	bool gfx_selected = false;
	bool png_selected = false;
	bool bas_selected = false;
	bool wav_selected = false;
	bool dsnd_selected = false;
	bool mus_selected = false;
	bool text_selected = false;
	bool unknown_selected = false;
	bool texturex_selected = false;
	bool modified_selected = false;
	bool map_selected = false;
	bool swan_selected = false;
//	bool rle_selected = false;
	string category = "";
	for (auto& entry : selection)
	{
		// Check for gfx entry
		if (!gfx_selected)
		{
			if (entry->getType()->extraProps().propertyExists("image"))
				gfx_selected = true;
		}
		if (!png_selected)
		{
			if (entry->getType()->formatId() == "img_png")
				png_selected = true;
		}
		if (!bas_selected)
		{
			if (entry->getType()->formatId() == "animated" ||
				entry->getType()->formatId() == "switches")
				bas_selected = true;
		}
		if (!swan_selected)
		{
			if (entry->getType()->id() == "swantbls")
				swan_selected = true;
		}
		if (!wav_selected)
		{
			if (entry->getType()->formatId() == "snd_wav")
				wav_selected = true;
		}
		if (!dsnd_selected)
		{
			if (entry->getType()->formatId() == "snd_doom" ||
				entry->getType()->formatId() == "snd_speaker" ||
				entry->getType()->formatId() == "snd_wolf" ||
				entry->getType()->formatId() == "snd_doom_mac" ||
				entry->getType()->formatId() == "snd_jaguar" ||
				entry->getType()->formatId() == "snd_bloodsfx" ||
				entry->getType()->formatId() == "snd_voc")
				dsnd_selected = true;
		}
		if (!mus_selected)
		{
			if (entry->getType()->formatId().StartsWith("midi_") &&
				entry->getType()->formatId() != "midi_smf")
				mus_selected = true;
		}
		if (!text_selected)
		{
			if (entry->getType()->formatId() == "text")
				text_selected = true;
		}
		if (!unknown_selected)
		{
			if (entry->getType() == EntryType::unknownType())
				unknown_selected = true;
		}
		if (!texturex_selected)
		{
			if (entry->getType()->formatId() == "texturex")
				texturex_selected = true;
		}
		if (!modified_selected)
		{
			if (entry->getState() == 1)
				modified_selected = true;
		}
		if (!map_selected)
		{
			if (entry->getType() == EntryType::mapMarkerType())
				map_selected = true;
			else if (entry->getParentDir()->getName() == "maps")
				map_selected = true;
		}
#if 0
		if (!rle_selected)
		{
			if (selection[a]->getType()->getFormat() == "misc_rle0")
				rle_selected = true;
		}
#endif
		if (category != "diff")
		{
			if (category.empty())
				category = entry->getType()->category();
			else
			{
				string ed = entry->getType()->category();
				if (category != ed)
					category = "diff";
			}
		}
	}

	// Generate context menu
	wxMenu context;
	SAction::fromId("arch_entry_rename")->addToMenu(&context, true);
	if (selection.size() > 1) SAction::fromId("arch_entry_rename_each")->addToMenu(&context, true);
	SAction::fromId("arch_entry_delete")->addToMenu(&context, true);
	if (modified_selected) SAction::fromId("arch_entry_revert")->addToMenu(&context, true);
	context.AppendSeparator();
	SAction::fromId("arch_entry_cut")->addToMenu(&context, true);
	SAction::fromId("arch_entry_copy")->addToMenu(&context, true);
	SAction::fromId("arch_entry_paste")->addToMenu(&context, true);
	context.AppendSeparator();
	SAction::fromId("arch_entry_import")->addToMenu(&context, true);
	SAction::fromId("arch_entry_export")->addToMenu(&context, true);
	context.AppendSeparator();
	SAction::fromId("arch_entry_moveup")->addToMenu(&context, true);
	SAction::fromId("arch_entry_movedown")->addToMenu(&context, true);
	SAction::fromId("arch_entry_sort")->addToMenu(&context, true);
	context.AppendSeparator();
	SAction::fromId("arch_entry_bookmark")->addToMenu(&context, true);
	//SAction::fromId("arch_entry_opentab")->addToMenu(&context, true);
	//SAction::fromId("arch_entry_crc32")->addToMenu(&context, true);

	// Add 'Open In' menu
	context.AppendSubMenu(createEntryOpenMenu(category), "Open")->SetBitmap(Icons::getIcon(Icons::GENERAL, "open"));

	// Add custom menu items
	wxMenu* custom;
	if (context_submenus)
	{
		custom = new wxMenu();
	}
	else custom = &context;
	bool ok = cur_area_->fillCustomMenu(custom);
	if (context_submenus)
	{
		if (ok) context.AppendSubMenu(custom, cur_area_->getCustomMenuName());
		else delete custom;
	}

	// Add Boom Animations/Switches related menu items if they are selected
	if (bas_selected)
	{
		SAction::fromId("arch_bas_convertb")->addToMenu(&context, true);
		SAction::fromId("arch_bas_convertz")->addToMenu(&context, true);
	}
	if (swan_selected)
	{
		SAction::fromId("arch_swan_convert")->addToMenu(&context, true);
	}

	// Add texturex related menu items if needed
	if (texturex_selected)
	{
		SAction::fromId("arch_texturex_convertzd")->addToMenu(&context, true);
		SAction::fromId("arch_texturex_finderrors")->addToMenu(&context, true);
	}

	// 'View As' menu
	if (context_submenus)
	{
		wxMenu* viewas = new wxMenu();
		context.AppendSubMenu(viewas, "View As");
		SAction::fromId("arch_view_text")->addToMenu(viewas, "Text");
		SAction::fromId("arch_view_hex")->addToMenu(viewas, "Hex");
	}
	else
	{
		context.AppendSeparator();
		SAction::fromId("arch_view_text")->addToMenu(&context, true);
		SAction::fromId("arch_view_hex")->addToMenu(&context, true);
	}

	// Add gfx-related menu items if gfx are selected (multi-select only)
	if (gfx_selected && selection.size() > 1)
	{
		wxMenu* gfx;
		if (context_submenus)
		{
			gfx = new wxMenu();
			context.AppendSubMenu(gfx, "Graphics");
		}
		else
		{
			context.AppendSeparator();
			gfx = &context;
		}
		SAction::fromId("arch_gfx_convert")->addToMenu(gfx, true);
		SAction::fromId("arch_gfx_translate")->addToMenu(gfx, true);
		SAction::fromId("arch_gfx_colourise")->addToMenu(gfx, true);
		SAction::fromId("arch_gfx_tint")->addToMenu(gfx, true);
		SAction::fromId("arch_gfx_offsets")->addToMenu(gfx, true);
		SAction::fromId("arch_gfx_addptable")->addToMenu(gfx, true);
		SAction::fromId("arch_gfx_addtexturex")->addToMenu(gfx, true);
		SAction::fromId("arch_gfx_exportpng")->addToMenu(gfx, true);
		if (png_selected)
			SAction::fromId("arch_gfx_pngopt")->addToMenu(gfx, true);
	}

	// Add Audio related menu items if needed
	if (wav_selected || dsnd_selected || mus_selected)
	{
		wxMenu* audio;
		if (context_submenus)
		{
			audio = new wxMenu();
			context.AppendSubMenu(audio, "Audio");
		}
		else
		{
			context.AppendSeparator();
			audio = &context;
		}
		if (wav_selected)
			SAction::fromId("arch_audio_convertwd")->addToMenu(audio, true);
		if (dsnd_selected)
			SAction::fromId("arch_audio_convertdw")->addToMenu(audio, true);
		if (mus_selected)
			SAction::fromId("arch_audio_convertmus")->addToMenu(audio, true);
	}

	// Add script related menu items if needed
	if (text_selected || unknown_selected)
	{
		wxMenu* scripts;
		if (context_submenus)
		{
			scripts = new wxMenu();
			context.AppendSubMenu(scripts, "Script");
		}
		else
		{
			context.AppendSeparator();
			scripts = &context;
		}
		SAction::fromId("arch_scripts_compileacs")->addToMenu(scripts, true);
		SAction::fromId("arch_scripts_compilehacs")->addToMenu(scripts, true);
	}

	// Add map related menu items if needed
	if (map_selected)
	{
		// 'Open in Doom Builder 2' (windows-only)
#ifdef __WXMSW__
		SAction::fromId("arch_map_opendb2")->addToMenu(&context, true);
#endif
	}

	// Entry scripts
	if (!ScriptManager::editorScripts(ScriptManager::ScriptType::Entry).empty())
	{
		auto menu_scripts = new wxMenu();
		ScriptManager::populateEditorScriptMenu(menu_scripts, ScriptManager::ScriptType::Entry, "arch_entry_script");
		context.AppendSeparator();
		context.AppendSubMenu(menu_scripts, "Run &Script");
	}

	// Popup the context menu
	PopupMenu(&context);
}

// ----------------------------------------------------------------------------
// ArchivePanel::onEntryListKeyDown
//
// Called when a key is pressed on the entry list
// ----------------------------------------------------------------------------
void ArchivePanel::onEntryListKeyDown(wxKeyEvent& e)
{
	// Check if keypress matches any keybinds
	wxArrayString binds = KeyBind::getBinds(KeyBind::asKeyPress(e.GetKeyCode(), e.GetModifiers()));

	// Go through matching binds
	for (auto bind : binds)
	{
		// --- General ---

		// Copy
		if (bind == "copy")
		{
			copyEntry();
			return;
		}

		// Cut
		else if (bind == "cut")
		{
			cutEntry();
			return;
		}

		// Paste
		else if (bind == "paste")
		{
			pasteEntry();
			return;
		}

		// Select All
		else if (bind == "select_all")
		{
			entry_list_->selectAll();
			return;
		}


		// --- Entry list specific ---

		// New Entry
		else if (bind == "el_new")
		{
			newEntry();
			return;
		}

		// Rename Entry
		else if (bind == "el_rename")
		{
			renameEntry();
			return;
		}

		// Delete Entry
		else if (bind == "el_delete")
		{
			deleteEntry();
			return;
		}

		// Move Entry up
		else if (bind == "el_move_up")
		{
			moveUp();
			return;
		}

		// Move Entry down
		else if (bind == "el_move_down")
		{
			moveDown();
			return;
		}

		// Import to Entry
		else if (bind == "el_import")
		{
			importEntry();
			return;
		}

		// Import Files
		else if (bind == "el_import_files")
		{
			importFiles();
			return;
		}

		// Export Entry
		else if (bind == "el_export")
		{
			exportEntry();
			return;
		}

		// Up directory
		else if (bind == "el_up_dir")
		{
			entry_list_->goUpDir();
			return;
		}
	}

	// Not handled, send to parent
	e.Skip();
}

// ----------------------------------------------------------------------------
// ArchivePanel::onEntryListActivated
//
// Called when an item on the entry list is 'activated'
// (via double-click or enter)
// ----------------------------------------------------------------------------
void ArchivePanel::onEntryListActivated(wxListEvent& e)
{
	ArchiveEntry* entry = entry_list_->getFocusedEntry();

	if (!entry)
		return;

	// Archive
	if (entry->getType()->formatId().substr(0, 8) == "archive_")
		App::archiveManager().openArchive(entry);

	// Texture list
	else if (entry->getType()->formatId() == "texturex" ||
		     entry->getType() == EntryType::fromId("pnames") ||
	         entry->getType() == EntryType::fromId("zdtextures"))
		MainEditor::openTextureEditor(archive_, entry);

	// Map
	// TODO: Needs to filter the game/port lists in the dialog by the map format
	else if (entry->getType() == EntryType::mapMarkerType())
	{
		// Open map editor config dialog
		MapEditorConfigDialog dlg(this, archive_, false);
		if (dlg.ShowModal() == wxID_OK)
		{
			Archive::MapDesc info = archive_->getMapInfo(entry);

			// Check selected game configuration is ok
			if (!dlg.configMatchesMap(info))
				wxMessageBox("Selected Game Configuration does not match the map format", "Error", wxICON_ERROR);
			else
			{
				// Load game configuration
				Game::configuration().openConfig(dlg.selectedGame(), dlg.selectedPort(), info.format);

				// Attempt to open map
				if (MapEditor::window()->openMap(info))
					MapEditor::window()->Show();
				else
				{
					MapEditor::window()->Hide();
					wxMessageBox(S_FMT("Unable to open map %s: %s", entry->getName(), Global::error), "Invalid map error", wxICON_ERROR);
				}
			}
		}
	}

	// Other entry
	else if (entry->getType() != EntryType::folderType())
		MainEditor::openEntry(entry);

	e.Skip();
}

// ----------------------------------------------------------------------------
// ArchivePanel::onMEPEditAsText
//
// Called when the 'Edit Level Script' button is clicked on the map entry panel
// (opens the entry in the text editor panel)
// ----------------------------------------------------------------------------
void ArchivePanel::onMEPEditAsText(wxCommandEvent& e)
{
	// Get entry to edit
	ArchiveEntry* entry = map_area_->entry();

	// Open in text editor
	openEntryAsText(entry);
}

// ----------------------------------------------------------------------------
// ArchivePanel::onDEPEditAsText
//
// Called when the 'Edit as Text' button is clicked on the default entry panel
// (opens the entry in the text editor panel)
// ----------------------------------------------------------------------------
void ArchivePanel::onDEPEditAsText(wxCommandEvent& e)
{
	// Get entry to edit
	ArchiveEntry* entry = default_area_->entry();

	// Open in text editor
	openEntryAsText(entry);
}

// ----------------------------------------------------------------------------
// ArchivePanel::onDEPEViewAsHex
//
// Called when the 'View as Hex' button is clicked on the default entry panel
// (opens the entry in the hex editor panel)
// ----------------------------------------------------------------------------
void ArchivePanel::onDEPViewAsHex(wxCommandEvent& e)
{
	// Get entry to view
	ArchiveEntry* entry = default_area_->entry();

	// Open in hex editor
	openEntryAsHex(entry);
}

// ----------------------------------------------------------------------------
// ArchivePanel::onTextFilterChanged
//
// Called when the filter text is changed
// ----------------------------------------------------------------------------
void ArchivePanel::onTextFilterChanged(wxCommandEvent& e)
{
	// Get category string to filter by
	string category = "";
	if (choice_category_->GetSelection() > 0)
		category = choice_category_->GetStringSelection();

	// Filter the entry list
	entry_list_->filterList(text_filter_->GetValue(), category);

	e.Skip();
}

// ----------------------------------------------------------------------------
// ArchivePanel::onChoiceCategoryChanged
//
// Called when the 'Category' choice is changed
// ----------------------------------------------------------------------------
void ArchivePanel::onChoiceCategoryChanged(wxCommandEvent& e)
{
	// Get category string to filter by
	string category = "";
	if (choice_category_->GetSelection() > 0)
		category = choice_category_->GetStringSelection();

	// Filter the entry list
	entry_list_->filterList(text_filter_->GetValue(), category);

	e.Skip();
}

// ----------------------------------------------------------------------------
// ArchivePanel::onDirChanged
//
// Called when the entry list directory is changed
// ----------------------------------------------------------------------------
void ArchivePanel::onDirChanged(wxCommandEvent& e)
{
	// Get directory
	ArchiveTreeNode* dir = entry_list_->getCurrentDir();

	if (!dir->getParent())
	{
		// Root dir
		label_path_->SetLabel("Path:");
		btn_updir_->Enable(false);
	}
	else
	{
		// Setup path string
		string path = dir->getPath();
		path.Remove(0, 1);
		path.Prepend("Path: ");

		label_path_->SetLabel(path);
		btn_updir_->Enable(true);
	}
}

// ----------------------------------------------------------------------------
// ArchivePanel::onBtnUpDir
//
// Called when the 'Up Directory' button is clicked
// ----------------------------------------------------------------------------
void ArchivePanel::onBtnUpDir(wxCommandEvent& e)
{
	// Go up a directory in the entry list
	entry_list_->goUpDir();
}

// ----------------------------------------------------------------------------
// ArchivePanel::onBtnClearFilter
//
// Called when the 'Clear Filter' button is clicked
// ----------------------------------------------------------------------------
void ArchivePanel::onBtnClearFilter(wxCommandEvent& e)
{
	text_filter_->SetValue("");
}


// ----------------------------------------------------------------------------
//
// EntryDataUS Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// EntryDataUS::swapData
//
// Swaps data between the entry and the undo step
// ----------------------------------------------------------------------------
bool EntryDataUS::swapData()
{
	//LOG_MESSAGE(1, "Entry data swap...");

	// Get parent dir
	ArchiveTreeNode* dir = archive_->getDir(path_);
	if (dir)
	{
		// Get entry
		ArchiveEntry* entry = dir->entryAt(index_);
		if (!entry)
			return false;

		// Backup data
		MemChunk temp_data;
		temp_data.importMem(entry->getData(), entry->getSize());
		//LOG_MESSAGE(1, "Backup current data, size %d", entry->getSize());

		// Restore entry data
		if (data_.getSize() == 0)
		{
			entry->clearData();
			//LOG_MESSAGE(1, "Clear entry data");
		}
		else
		{
			entry->importMemChunk(data_);
			//LOG_MESSAGE(1, "Restored entry data, size %d", data.getSize());
		}

		// Store previous entry data
		if (temp_data.getSize() > 0)
			data_.importMem(temp_data.getData(), temp_data.getSize());
		else
			data_.clear();

		return true;
	}

	return false;
}


// ----------------------------------------------------------------------------
//
// Console Commands
//
// I'd love to put them in their own file, but attempting to do so results in
// a circular include nightmare and nothing works anymore.
// TODO: Look at doing something else with these
// ----------------------------------------------------------------------------

#include "General/Console/ConsoleHelpers.h"
#include "General/Console/Console.h"
#include "App.h"
Archive* CH::getCurrentArchive()
{
	if (MainEditor::window())
	{
		if (MainEditor::window()->getArchiveManagerPanel())
		{
			return MainEditor::window()->getArchiveManagerPanel()->currentArchive();
		}
	}
	return nullptr;
}

ArchivePanel* CH::getCurrentArchivePanel()
{
	ArchiveManagerPanel* archie = MainEditor::window()->getArchiveManagerPanel();
	if (archie)
	{
		if (archie->isArchivePanel(archie->currentTabIndex()))
			return (ArchivePanel*)(archie->currentPanel());
	}
	return nullptr;
}

CONSOLE_COMMAND(palconv, 0, false)
{
	ArchivePanel* meep = CH::getCurrentArchivePanel();
	if (meep)
	{
		meep->palConvert();
		meep->reloadCurrentPanel();
	}
}

CONSOLE_COMMAND(palconv64, 0, false)
{
	ArchivePanel* meep = CH::getCurrentArchivePanel();
	if (meep)
	{
		// Get the entry index of the last selected list item
		ArchiveEntry* pal = meep->currentEntry();
		const uint8_t* source = pal->getData(true);
		uint8_t* dest = new uint8_t[(pal->getSize() / 2) * 3];
		for (size_t i = 0; i < pal->getSize() / 2; ++i)
		{
			uint8_t r, g, b;
			uint16_t col = READ_B16(source, 2*i);
			r = (col & 0xF800) >> 8;
			g = (col & 0x07C0) >> 3;
			b = (col & 0x003E) << 2;
			dest[(3*i)+0] = r;
			dest[(3*i)+1] = g;
			dest[(3*i)+2] = b;
		}
		pal->importMem(dest, (pal->getSize()/2)*3);
		MainEditor::currentEntryPanel()->callRefresh();
		delete[] dest;
	}
}

CONSOLE_COMMAND(palconvpsx, 0, false)
{
	ArchivePanel* meep = CH::getCurrentArchivePanel();
	if (meep)
	{
		// Get the entry index of the last selected list item
		ArchiveEntry* pal = meep->currentEntry();
		const uint8_t* source = pal->getData(true);
		uint8_t* dest = new uint8_t[(pal->getSize() / 2) * 3];
		for (size_t i = 0; i < pal->getSize() / 2; ++i)
		{
			// A1 B5 G5 R5, LE
			uint8_t a, r, g, b;
			uint16_t col = READ_L16(source, 2*i);
			a = (col & 0x8000) >> 15;
			b = (col & 0x7C00) >> 10;
			g = (col & 0x03E0) >>  5;
			r = (col & 0x001F);
			r = (r << 3) | (r >> 2);
			g = (g << 3) | (g >> 2);
			b = (b << 3) | (b >> 2);
			dest[(3*i)+0] = r;
			dest[(3*i)+1] = g;
			dest[(3*i)+2] = b;
		}
		pal->importMem(dest, (pal->getSize()/2)*3);
		MainEditor::currentEntryPanel()->callRefresh();
		delete[] dest;
	}
}

CONSOLE_COMMAND(vertex32x, 0, false)
{
	ArchivePanel* meep = CH::getCurrentArchivePanel();
	if (meep)
	{
		// Get the entry index of the last selected list item
		ArchiveEntry* v32x = meep->currentEntry();
		const uint8_t* source = v32x->getData(true);
		uint8_t* dest = new uint8_t[v32x->getSize() / 2];
		for (size_t i = 0; i < v32x->getSize() / 4; ++i)
		{
			dest[2*i+0] = source[4*i+1];
			dest[2*i+1] = source[4*i+0];
		}
		v32x->importMem(dest, v32x->getSize()/2);
		MainEditor::currentEntryPanel()->callRefresh();
		delete[] dest;
	}
}

CONSOLE_COMMAND(vertexpsx, 0, false)
{
	ArchivePanel* meep = CH::getCurrentArchivePanel();
	if (meep)
	{
		// Get the entry index of the last selected list item
		ArchiveEntry* vpsx = meep->currentEntry();
		const uint8_t* source = vpsx->getData(true);
		uint8_t* dest = new uint8_t[vpsx->getSize() / 2];
		for (size_t i = 0; i < vpsx->getSize() / 4; ++i)
		{
			dest[2*i+0] = source[4*i+2];
			dest[2*i+1] = source[4*i+3];
		}
		vpsx->importMem(dest, vpsx->getSize()/2);
		MainEditor::currentEntryPanel()->callRefresh();
		delete[] dest;
	}
}

CONSOLE_COMMAND(lightspsxtopalette, 0, false)
{
	ArchivePanel* meep = CH::getCurrentArchivePanel();
	if (meep)
	{
		// Get the entry index of the last selected list item
		ArchiveEntry* lights = meep->currentEntry();
		const uint8_t* source = lights->getData(true);
		size_t entries = lights->getSize() / 4;
		uint8_t* dest = new uint8_t[entries * 3];
		for (size_t i = 0; i < entries; ++i)
		{
			dest[3*i+0] = source[4*i+0];
			dest[3*i+1] = source[4*i+1];
			dest[3*i+2] = source[4*i+2];
		}
		lights->importMem(dest, entries * 3);
		MainEditor::currentEntryPanel()->callRefresh();
		delete[] dest;
	}
}


vector<ArchiveEntry*> Console_SearchEntries(string name)
{
	vector<ArchiveEntry*> entries;
	Archive* archive = MainEditor::currentArchive();
	ArchivePanel* panel = CH::getCurrentArchivePanel();

	if (archive)
	{
		Archive::SearchOptions options;
		options.search_subdirs = true;
		if (panel)
		{
			options.dir = panel->currentDir();
		}
		options.match_name = name;
		entries = archive->findAll(options);
	}
	return entries;
}

CONSOLE_COMMAND(find, 1, true)
{
	vector<ArchiveEntry*> entries = Console_SearchEntries(args[0]);

	string message;
	size_t count = entries.size();
	if (count > 0)
	{
		for (size_t i = 0; i < count; ++i)
		{
			message += entries[i]->getPath(true) + "\n";
		}
	}
	Log::info(S_FMT("Found %i entr%s", count, count==1?"y":"ies\n") + message);
}

CONSOLE_COMMAND(ren, 2, true)
{
	Archive* archive = MainEditor::currentArchive();
	vector<ArchiveEntry*> entries = Console_SearchEntries(args[0]);
	if (!entries.empty())
	{
		size_t count = 0;
		for (size_t i = 0; i < entries.size(); ++i)
		{
			// Rename filter logic
			string newname = entries[i]->getName();
			for (unsigned c = 0; c < args[1].size(); c++)
			{
				// Check character
				if (args[1][c] == '*')
					continue;					// Skip if *
				else
				{
					// First check that we aren't past the end of the name
					if (c >= newname.size())
					{
						// If we are, pad it with spaces
						while (newname.size() <= c)
							newname += " ";
					}

					// Replace character
					newname[c] = args[1][c];
				}
			}

			if (archive->renameEntry(entries[i], newname))
				++count;
		}
		LOG_MESSAGE(1, "Renamed %i entr%s", count, count==1?"y":"ies");
	}
}

CONSOLE_COMMAND(cd, 1, true)
{
	Archive* current = MainEditor::currentArchive();
	ArchivePanel* panel = CH::getCurrentArchivePanel();

	if (current && panel)
	{
		ArchiveTreeNode* dir = panel->currentDir();
		ArchiveTreeNode* newdir = current->getDir(args[0], dir);
		if (newdir == nullptr)
		{
			if (args[0].Matches(".."))
				newdir = (ArchiveTreeNode*) dir->getParent();
			else if (args[0].Matches("/") || args[0].Matches("\\"))
				newdir = current->rootDir();
		}

		if (newdir)
		{
			panel->openDir(newdir);
		}
		else
		{
			LOG_MESSAGE(1, "Error: Trying to open nonexistant directory %s", args[0]);
		}

	}
}

/*
#include "Utility/Tokenizer.h"

CONSOLE_COMMAND(run, 1, true)
{
	MemChunk mc;
	// Try to run a batch command file
	if (wxFile::Exists(args[0]))
	{
		if (!mc.importFile(args[0]))
			return;
	}
	else
	{
		// Try to run a batch command lump
		vector<ArchiveEntry*> entries = Console_SearchEntries(args[0]);
		if (entries.size() > 0)
		{
			if (!mc.importMem(entries[0]->getData(), entries[0]->getSize()))
				return;
		}
	}

	Tokenizer t;
	if (!t.openMem(mc, args[0]))
		return;

	string cmdline, s;
	uint32_t line = t.lineNo();

	// There's probably a better way to slice the chunk into lines...
	while (1 + t.tokenEnd() < mc.getSize())
	{
		s = t.getToken() + " ";
		if (t.lineNo() != line || 1 + t.tokenEnd() >= mc.getSize())
		{
			App::console()->execute(cmdline);
			cmdline.Empty();
			line = t.lineNo();
		};
		cmdline += s;
	};
}
*/
