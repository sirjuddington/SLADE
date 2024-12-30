
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "ArchivePanel.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveFormat.h"
#include "Archive/ArchiveManager.h"
#include "Archive/EntryType/EntryType.h"
#include "Archive/MapDesc.h"
#include "ArchiveManagerPanel.h"
#include "EntryPanel/ANSIEntryPanel.h"
#include "EntryPanel/AudioEntryPanel.h"
#include "EntryPanel/DataEntryPanel.h"
#include "EntryPanel/DefaultEntryPanel.h"
#include "EntryPanel/GfxEntryPanel.h"
#include "EntryPanel/HexEntryPanel.h"
#include "EntryPanel/MapEntryPanel.h"
#include "EntryPanel/PaletteEntryPanel.h"
#include "EntryPanel/TextEntryPanel.h"
#include "Game/Configuration.h"
#include "General/Clipboard.h"
#include "General/Executables.h"
#include "General/KeyBind.h"
#include "General/SAction.h"
#include "General/UndoRedo.h"
#include "General/UndoSteps/EntryDataUS.h"
#include "Graphics/Palette/PaletteManager.h"
#include "MainEditor/ArchiveOperations.h"
#include "MainEditor/Conversions.h"
#include "MainEditor/EntryOperations.h"
#include "MainEditor/ExternalEditManager.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/MainWindow.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/UI/MapEditorWindow.h"
#include "Scripting/ScriptManager.h"
#include "UI/Controls/PaletteChooser.h"
#include "UI/Controls/SIconButton.h"
#include "UI/Controls/Splitter.h"
#include "UI/Dialogs/GfxConvDialog.h"
#include "UI/Dialogs/MapEditorConfigDialog.h"
#include "UI/Dialogs/MapReplaceDialog.h"
#include "UI/Dialogs/NewEntryDialog.h"
#include "UI/Dialogs/RunDialog.h"
#include "UI/Dialogs/SettingsDialog.h"
#include "UI/Layout.h"
#include "UI/Lists/ArchiveEntryTree.h"
#include "UI/SToolBar/SToolBar.h"
#include "UI/SToolBar/SToolBarButton.h"
#include "UI/UI.h"
#include "UI/WxUtils.h"
#include "Utility/SFileDialog.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
wxMenu* menu_archive = nullptr;
wxMenu* menu_entry   = nullptr;
} // namespace
CVAR(Int, autosave_entry_changes, 2, CVar::Flag::Save) // 0=no, 1=yes, 2=ask
CVAR(Bool, confirm_entry_delete, true, CVar::Flag::Save)
CVAR(Bool, context_submenus, true, CVar::Flag::Save)
CVAR(Bool, auto_entry_replace, false, CVar::Flag::Save)
CVAR(Bool, elist_show_filter, false, CVar::Flag::Save)
CVAR(Int, ap_splitter_position_tree, 300, CVar::Flag::Save)
CVAR(Int, ap_splitter_position_list, 300, CVar::Flag::Save)
CVAR(Bool, elist_no_tree, false, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, confirm_entry_revert)
EXTERN_CVAR(Bool, archive_dir_ignore_hidden)


// -----------------------------------------------------------------------------
// APEntryListDropTarget Class
//
// Handles drag'n'drop of files on to the entry list
// -----------------------------------------------------------------------------
class APEntryListDropTarget : public wxFileDropTarget
{
public:
	APEntryListDropTarget(ArchivePanel* parent, ui::ArchiveEntryTree* list) : parent_{ parent }, list_{ list } {}
	~APEntryListDropTarget() override = default;

	bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames) override
	{
		auto* archive = parent_->archive();
		if (!archive)
			return false;

		// Determine what item the files were dragged onto
		wxDataViewItem    hit_item;
		wxDataViewColumn* hit_column = nullptr;
		list_->HitTest(wxPoint(x, y), hit_item, hit_column);

		// Determine directory and index to import to
		auto* hit_entry = static_cast<ArchiveEntry*>(hit_item.GetID());
		auto* dir       = list_->currentRootDir();
		int   index     = -1;
		if (hit_entry)
		{
			if (hit_entry->type() == EntryType::folderType())
				dir = ArchiveDir::findDirByDirEntry(archive->rootDir(), *hit_entry).get();
			else
			{
				dir   = hit_entry->parentDir();
				index = hit_entry->index();
			}
		}

		bool     yes_to_all = false;
		wxString caption    = (filenames.size() > 1) ? "Overwrite entries" : "Overwrite entry";

		// Import all dragged files, inserting after the item they were dragged onto
		list_->Freeze();
		for (int a = filenames.size() - 1; a >= 0; a--)
		{
			// Is this a directory?
			if (wxDirExists(filenames[a]))
			{
				// If the archive supports directories, create the directory and import its contents
				if (archive->formatInfo().supports_dirs)
				{
					strutil::Path fn(filenames[a].ToStdString());
					auto          ndir = archive->createDir(fn.fileName(false), ArchiveDir::getShared(dir));
					archive->importDir(fn.fullPath(), true, ndir);
				}

				// TODO: Do we want to support flat list archives here? If so might want special handling for
				// namespaces etc.
			}
			else
			{
				strutil::Path fn(filenames[a].ToStdString());
				ArchiveEntry* entry = nullptr;

				// Find entry to replace if needed
				if (auto_entry_replace)
				{
					entry = archive->entryAtPath(dir->path().append(fn.fileName()));
					// An entry with that name is already present, so ask about replacing it
					if (entry && !yes_to_all)
					{
						// Since there is no standard "Yes/No to all" button or "Don't ask me again" checkbox,
						// we will instead hack the Cancel button into being a "Yes to all" button. This is
						// despite the existence of a wxID_YESTOALL return value...
						auto message = fmt::format("Overwrite existing entry {}{}", dir->path(), fn.fileName());
						wxMessageDialog dlg(parent_, message, caption, wxCANCEL | wxYES_NO | wxCENTRE);
						dlg.SetYesNoCancelLabels(_("Yes"), _("No"), _("Yes to all"));
						int result = dlg.ShowModal();

						// User doesn't want to replace the entry
						if (result == wxID_NO)
							entry = nullptr;
						// User wants to replace all entries
						if (result == wxID_CANCEL)
							yes_to_all = true;
					}
				}

				// Create new entry if needed
				if (entry == nullptr)
					entry = archive->addNewEntry(fn.fileName(), index, dir).get();

				// Import the file to it
				entry->importFile(filenames[a].ToStdString());
				EntryType::detectEntryType(*entry);
			}
		}
		list_->Thaw();

		return true;
	}

private:
	ArchivePanel*         parent_ = nullptr;
	ui::ArchiveEntryTree* list_   = nullptr;
};


// -----------------------------------------------------------------------------
// ChoosePaletteDialog Class
//
// A simple dialog for the 'Choose Palette' function when creating a new
// palette entry.
// -----------------------------------------------------------------------------
class ChoosePaletteDialog : public wxDialog
{
public:
	ChoosePaletteDialog(wxWindow* parent) :
		wxDialog{ parent,
				  -1,
				  "Choose Base Palette",
				  wxDefaultPosition,
				  wxDefaultSize,
				  wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER }
	{
		auto lh = ui::LayoutHelper(this);

		// Set dialog icon
		wxutil::setWindowIcon(this, "palette");

		// Setup main sizer
		auto sizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(sizer);

		// Add choose
		pal_chooser_ = new PaletteChooser(this, -1);
		sizer->Add(pal_chooser_, lh.sfWithBorder().Expand());

		sizer->Add(CreateButtonSizer(wxOK | wxCANCEL), lh.sfWithBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Expand());

		// Init layout
		wxDialog::Layout();

		// Setup dialog size
		SetInitialSize(wxSize(-1, -1));
		wxDialog::SetMinSize(GetSize());
	}

	int getChoice() const { return pal_chooser_->GetSelection(); }

private:
	PaletteChooser* pal_chooser_ = nullptr;
};


// -----------------------------------------------------------------------------
// EntryTreeClipboardItem Class
//
// A clipboard item for copy+paste of archive entries and folders
// -----------------------------------------------------------------------------
class EntryTreeClipboardItem : public ClipboardItem
{
public:
	EntryTreeClipboardItem(const vector<ArchiveEntry*>& entries, const vector<ArchiveDir*>& dirs) :
		ClipboardItem(Type::EntryTree),
		tree_{ new ArchiveDir("") }
	{
		// Copy entries
		for (auto& entry : entries)
			tree_->addEntry(std::make_shared<ArchiveEntry>(*entry));

		// Copy entries to system clipboard
		// (exports them as temp files and adds the paths to the clipboard)
		if (wxTheClipboard->Open())
		{
			wxTheClipboard->Clear();
			auto file          = new wxFileDataObject();
			auto tmp_directory = app::path("", app::Dir::Temp); // cache temp directory
			for (auto& entry : entries)
			{
				// Export to file
				auto filename = fmt::format("{}{}.{}", tmp_directory, entry->nameNoExt(), entry->type()->extension());
				entry->exportFile(filename);

				// Add to clipboard
				file->AddFile(filename);
			}
			wxTheClipboard->AddData(file);
			wxTheClipboard->Close();
		}

		// Copy dirs
		for (auto& dir : dirs)
			tree_->addSubdir(dir->clone(tree_));
	}

	~EntryTreeClipboardItem() override = default;

	ArchiveDir* tree() const { return tree_.get(); }

private:
	shared_ptr<ArchiveDir> tree_;
};


// -----------------------------------------------------------------------------
//
// ArchivePanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ArchivePanel class constructor
// -----------------------------------------------------------------------------
ArchivePanel::ArchivePanel(wxWindow* parent, const shared_ptr<Archive>& archive) :
	wxPanel(parent, -1),
	archive_{ archive },
	undo_manager_{ new UndoManager() },
	ee_manager_{ new ExternalEditManager }
{
	setup(archive.get());
	bindEvents(archive.get());
}

// -----------------------------------------------------------------------------
// Setup the panel controls and layout
// -----------------------------------------------------------------------------
void ArchivePanel::setup(const Archive* archive)
{
	// Create controls
	splitter_         = new ui::Splitter(this, -1, wxSP_3DSASH | wxSP_LIVE_UPDATE);
	entry_area_       = new EntryPanel(splitter_, "nil");
	default_area_     = new DefaultEntryPanel(splitter_);
	auto* elist_panel = createEntryListPanel(splitter_);

	// Create sizer
	auto m_hbox = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(m_hbox);

	// Set default entry panel
	cur_area_ = entry_area_;
	cur_area_->Show(true);
	cur_area_->setUndoManager(undo_manager_.get());

	// Setup splitter
	splitter_->SetMinimumPaneSize(FromDIP(300));
	m_hbox->Add(splitter_, ui::LayoutHelper(this).sfWithBorder(1).Expand());
	int split_pos = FromDIP(ap_splitter_position_list);
	if (archive && archive->formatInfo().supports_dirs)
		split_pos = FromDIP(ap_splitter_position_tree);
	splitter_->SplitVertically(elist_panel, cur_area_, split_pos);

	// Update size+layout
	Layout();
	elist_panel->Layout();
	elist_panel->Update();
	elist_panel->Refresh();
}

// -----------------------------------------------------------------------------
// Bind widget events and [archive] signals
// -----------------------------------------------------------------------------
void ArchivePanel::bindEvents(Archive* archive)
{
	entry_tree_->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &ArchivePanel::onEntryListSelectionChange, this);
	entry_tree_->GetMainWindow()->Bind(wxEVT_KEY_DOWN, &ArchivePanel::onEntryListKeyDown, this);
	entry_tree_->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &ArchivePanel::onEntryListRightClick, this);
	entry_tree_->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &ArchivePanel::onEntryListActivated, this);
	text_filter_->Bind(wxEVT_TEXT, &ArchivePanel::onTextFilterChanged, this);
	choice_category_->Bind(wxEVT_CHOICE, &ArchivePanel::onChoiceCategoryChanged, this);
	btn_clear_filter_->Bind(wxEVT_BUTTON, &ArchivePanel::onBtnClearFilter, this);

	// Update splitter position cvar when moved
	splitter_->Bind(
		wxEVT_SPLITTER_SASH_POS_CHANGED,
		[this](wxSplitterEvent& e)
		{
			if (auto archive = archive_.lock().get())
			{
				if (archive->formatInfo().supports_dirs)
					ap_splitter_position_tree = ToDIP(e.GetSashPosition());
				else
					ap_splitter_position_list = ToDIP(e.GetSashPosition());
			}
		});

	// Update entry moving toolbar if sorting changed
	entry_tree_->Bind(
		wxEVT_DATAVIEW_COLUMN_SORTED,
		[this](wxDataViewEvent& e) { toolbar_elist_->enableGroup("_Moving", canMoveEntries()); });

	// Update this tab's name in the parent notebook when the archive is saved
	sc_archive_saved_ = archive->signals().saved.connect(
		[this](Archive& a)
		{
			auto parent = dynamic_cast<wxAuiNotebook*>(GetParent());
			parent->SetPageText(parent->GetPageIndex(this), a.filename(false));
		});

	// Close current entry panel if it's entry was removed
	sc_entry_removed_ = archive->signals().entry_removed.connect(
		[this](Archive&, ArchiveDir&, ArchiveEntry& entry)
		{
			if (currentArea()->entry() == &entry)
			{
				currentArea()->closeEntry();
				currentArea()->openEntry(nullptr);
				currentArea()->Show(false);
			}
		});
}

// -----------------------------------------------------------------------------
// Creates the entry list panel
// -----------------------------------------------------------------------------
wxPanel* ArchivePanel::createEntryListPanel(wxWindow* parent)
{
	auto* panel    = new wxPanel(parent);
	auto  archive  = archive_.lock();
	bool  has_dirs = archive->formatInfo().supports_dirs;
	auto  lh       = ui::LayoutHelper(panel);

	// Create & set sizer & border
	auto* hbox = new wxBoxSizer(wxHORIZONTAL);
	panel->SetSizer(hbox);

	// Create entry list
	entry_tree_ = new ui::ArchiveEntryTree(panel, archive, undo_manager_.get(), elist_no_tree);
	entry_tree_->SetInitialSize(lh.size(400, -1));
	entry_tree_->SetDropTarget(new APEntryListDropTarget(this, entry_tree_));

	// Create path controls if needed
	if (has_dirs && elist_no_tree)
	{
		etree_path_ = new ui::ArchivePathPanel(panel);
		entry_tree_->setPathPanel(etree_path_);
	}

	// Entry list toolbar
	toolbar_elist_ = new SToolBar(panel, false, wxVERTICAL);
	if (has_dirs && !elist_no_tree)
	{
		auto* tbg_folder = new SToolBarGroup(toolbar_elist_, "_Folder");
		tbg_folder->addActionButton("arch_elist_collapseall");
		toolbar_elist_->addGroup(tbg_folder);
	}
	auto* tbg_create = new SToolBarGroup(toolbar_elist_, "_Create");
	tbg_create->addActionButton("arch_newentry");
	if (has_dirs)
		tbg_create->addActionButton("arch_newdir");
	tbg_create->addActionButton("arch_importfiles");
	tbg_create->addActionButton("arch_importdir");
	toolbar_elist_->addGroup(tbg_create);
	auto* tbg_entry = new SToolBarGroup(toolbar_elist_, "_Entry");
	tbg_entry->addActionButton("arch_entry_rename");
	tbg_entry->addActionButton("arch_entry_rename_each");
	tbg_entry->addActionButton("arch_entry_delete");
	tbg_entry->addSeparator();
	tbg_entry->addActionButton("arch_entry_import");
	tbg_entry->addActionButton("arch_entry_export");
	tbg_entry->setAllButtonsEnabled(false);
	toolbar_elist_->addGroup(tbg_entry);
	if (archive->format() != ArchiveFormat::Dir)
	{
		auto* tbg_moving = new SToolBarGroup(toolbar_elist_, "_Moving");
		tbg_moving->addActionButton("arch_entry_moveup");
		tbg_moving->addActionButton("arch_entry_movedown");
		tbg_moving->addActionButton("arch_entry_sort");
		tbg_moving->Enable(false);
		toolbar_elist_->addGroup(tbg_moving);
	}
	auto* tbg_bookmark = new SToolBarGroup(toolbar_elist_, "_Bookmark");
	tbg_bookmark->addActionButton("arch_entry_bookmark");
	tbg_bookmark->setAllButtonsEnabled(false);
	toolbar_elist_->addGroup(tbg_bookmark);
	auto* tbg_filter = new SToolBarGroup(toolbar_elist_, "_Filter");
	tbg_filter->addActionButton("arch_elist_togglefilter")->action()->setChecked(elist_show_filter);
	toolbar_elist_->addGroup(tbg_filter, true);

	// Entry List filter controls
	panel_filter_ = new wxPanel(panel, -1);
	auto* gbsizer = new wxGridBagSizer(lh.pad(), lh.pad());
	panel_filter_->SetSizer(gbsizer);

	// Create category selector
	choice_category_ = new wxChoice(panel_filter_, -1);
	choice_category_->Append("All");
	for (const auto& cat : EntryType::allCategories())
		choice_category_->Append(cat);
	choice_category_->SetSelection(0);
	gbsizer->Add(
		new wxStaticText(panel_filter_, -1, "Show:"), wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gbsizer->Add(choice_category_, wxGBPosition(0, 1), wxGBSpan(1, 2), wxEXPAND);
	gbsizer->AddGrowableCol(1, 1);

	// Create filter
	text_filter_ = new wxTextCtrl(panel_filter_, -1);
	gbsizer->Add(
		new wxStaticText(panel_filter_, -1, "Filter:"), wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gbsizer->Add(text_filter_, wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND);
	btn_clear_filter_ = new SIconButton(panel_filter_, "close");
	btn_clear_filter_->SetToolTip("Clear Filter");
	gbsizer->Add(btn_clear_filter_, wxGBPosition(1, 2), wxDefaultSpan, wxEXPAND);

	// Show/hide filter controls depending on cvar
	panel_filter_->Show(elist_show_filter);

	// Layout entry list
	hbox->Add(toolbar_elist_, wxSizerFlags().Expand());
	hbox->AddSpacer(lh.padSmall());
	auto* vbox = new wxBoxSizer(wxVERTICAL);
	hbox->Add(vbox, lh.sfWithSmallBorder(1, wxRIGHT).Expand());
	if (etree_path_)
	{
		vbox->Add(etree_path_, wxSizerFlags().Expand());
		vbox->AddSpacer(lh.padSmall());
		vbox->Add(entry_tree_, wxSizerFlags(1).Expand());
	}
	else
		vbox->Add(entry_tree_, wxSizerFlags(1).Expand());
	vbox->Add(panel_filter_, lh.sfWithBorder(0, wxTOP).Expand());

	return panel;
}

// -----------------------------------------------------------------------------
// Saves any changes made to the currently open entry.
// If [force] is true the autosave_entry_changes cvar is ignored and the entry
// will always be saved (if there are any changes)
// -----------------------------------------------------------------------------
bool ArchivePanel::saveEntryChanges(bool force) const
{
	// Ignore if no changes have been made (or no entry is open)
	if (!cur_area_->isModified() || !cur_area_->entry())
		return true;

	// Don't save if autosave is off
	if (!force && autosave_entry_changes == 0)
		return false;

	// Ask if needed
	if (!force && autosave_entry_changes > 1)
	{
		int result = wxMessageBox(
			wxString::Format("Save changes to entry \"%s\"?", cur_area_->entry()->name()),
			"Unsaved Changes",
			wxYES_NO | wxICON_QUESTION);

		// Stop if user clicked no
		if (result == wxNO)
			return false;
	}

	// Save entry changes
	return cur_area_->saveEntry();
}

// -----------------------------------------------------------------------------
// Adds the 'Archive' and 'Entry' menus to the main window menubar
// -----------------------------------------------------------------------------
void ArchivePanel::addMenus() const
{
	// Create menus if needed
	if (!menu_archive)
	{
		// Archive menu
		menu_archive = new wxMenu();
		SAction::fromId("arch_newentry")->addToMenu(menu_archive);
		SAction::fromId("arch_newdir")->addToMenu(menu_archive);
		menu_archive->AppendSeparator();
		SAction::fromId("arch_importfiles")->addToMenu(menu_archive);
		SAction::fromId("arch_importdir")->addToMenu(menu_archive);
		SAction::fromId("arch_buildarchive")->addToMenu(menu_archive);
		menu_archive->AppendSeparator();
		SAction::fromId("arch_texeditor")->addToMenu(menu_archive);
		SAction::fromId("arch_mapeditor")->addToMenu(menu_archive);
		auto menu_clean = createMaintenanceMenu();

		auto maintenance_item = new wxMenuItem(menu_archive, wxID_ANY, "&Maintenance");
		maintenance_item->SetSubMenu(menu_clean);
		maintenance_item->SetBitmap(icons::getIcon(icons::Type::Any, "wrench"));
		menu_archive->Append(maintenance_item);

		auto menu_scripts = new wxMenu();
#ifndef NO_LUA
		scriptmanager::populateEditorScriptMenu(menu_scripts, scriptmanager::ScriptType::Archive, "arch_script");
		menu_archive->AppendSubMenu(menu_scripts, "&Run Script");
#endif

		menu_archive->AppendSeparator();
		SAction::fromId("arch_run")->addToMenu(menu_archive, true, "Run");
	}
	if (!menu_entry)
	{
		// Entry menu
		menu_entry = new wxMenu();
		SAction::fromId("arch_entry_save")->addToMenu(menu_entry);
		SAction::fromId("arch_entry_rename")->addToMenu(menu_entry);
		SAction::fromId("arch_entry_delete")->addToMenu(menu_entry);
		SAction::fromId("arch_entry_revert")->addToMenu(menu_entry);
		menu_entry->AppendSeparator();
		SAction::fromId("arch_entry_cut")->addToMenu(menu_entry);
		SAction::fromId("arch_entry_copy")->addToMenu(menu_entry);
		SAction::fromId("arch_entry_paste")->addToMenu(menu_entry);
		menu_entry->AppendSeparator();
		SAction::fromId("arch_entry_moveup")->addToMenu(menu_entry, false);
		SAction::fromId("arch_entry_movedown")->addToMenu(menu_entry, false);
		SAction::fromId("arch_entry_sort")->addToMenu(menu_entry);
		menu_entry->AppendSeparator();
		SAction::fromId("arch_entry_import")->addToMenu(menu_entry);
		SAction::fromId("arch_entry_export")->addToMenu(menu_entry);
#ifndef NO_LUA
		menu_entry->AppendSeparator();
		auto menu_scripts = new wxMenu();
		scriptmanager::populateEditorScriptMenu(menu_scripts, scriptmanager::ScriptType::Entry, "arch_entry_script");
		menu_entry->AppendSubMenu(menu_scripts, "&Run Script");
#endif
	}

	// Add them to the main window menubar
	maineditor::window()->addCustomMenu(menu_archive, "&Archive");
	maineditor::window()->addCustomMenu(menu_entry, "&Entry");
	cur_area_->addCustomMenu();

	// Also enable the related toolbars
	maineditor::window()->enableToolBar("_archive");
	maineditor::window()->enableToolBar("_entry");
}

// -----------------------------------------------------------------------------
// Removes the 'Archive' and 'Entry' menus from the main window menubar
// -----------------------------------------------------------------------------
void ArchivePanel::removeMenus() const
{
	// Remove ArchivePanel menus from the main window menubar
	maineditor::window()->removeCustomMenu(menu_archive);
	maineditor::window()->removeCustomMenu(menu_entry);
	cur_area_->removeCustomMenu();

	// Also disable the related toolbars
	maineditor::window()->enableToolBar("_archive", false);
	maineditor::window()->enableToolBar("_entry", false);
}

// -----------------------------------------------------------------------------
// Performs an undo operation
// -----------------------------------------------------------------------------
void ArchivePanel::undo() const
{
	if (!(cur_area_ && cur_area_->undo()))
	{
		entry_tree_->Freeze();
		undo_manager_->undo();
		entry_tree_->Thaw();
	}
}

// -----------------------------------------------------------------------------
// Performs a redo operation
// -----------------------------------------------------------------------------
void ArchivePanel::redo() const
{
	if (!(cur_area_ && cur_area_->redo()))
	{
		entry_tree_->Freeze();
		undo_manager_->redo();
		entry_tree_->Thaw();
	}
}

// -----------------------------------------------------------------------------
// Adds a new entry to the archive after the last selected entry in the list.
// If nothing is selected it is added at the end of the list.
// Pops up a NewEntryDialog to get the name/type/(directory) for the new entry.
// Returns true if the entry was created, false otherwise.
// -----------------------------------------------------------------------------
bool ArchivePanel::newEntry()
{
	// Check the archive is still open
	auto archive = archive_.lock();
	if (!archive)
		return false;

	// Show new entry dialog
	auto* last_entry = entry_tree_->lastSelectedEntry(true);
	auto* dlg        = new ui::NewEntryDialog(this, *archive, entry_tree_->currentSelectedDir());
	if (dlg->ShowModal() != wxID_OK)
		return false;

	// Determine the directory to add the new entry to
	auto* dir = archive->rootDir().get();
	if (archive->formatInfo().supports_dirs)
	{
		auto dir_path = dlg->parentDirPath().ToStdString();
		strutil::replaceIP(dir_path, "\\", "/");

		dir = archive->dirAtPath(dir_path);
		if (!dir)
			dir = archive->createDir(dir_path).get();
	}

	// Get the entry index of the last selected list item
	auto index = last_entry ? dir->entryIndex(last_entry) : -1;

	// If something was selected, add 1 to the index so we add the new entry after the last selected
	if (index >= 0)
		index++;

	// Create the new entry
	undo_manager_->beginRecord("Add Entry");
	auto new_entry = std::make_shared<ArchiveEntry>(dlg->entryName().ToStdString());

	// Deal with specific entry type that we may want created
	using NewEntry = maineditor::NewEntryType;
	auto type      = static_cast<NewEntry>(dlg->entryType());
	if (type != NewEntry::Empty && new_entry)
	{
		ArchiveEntry*       e_import;
		MemChunk            mc;
		ChoosePaletteDialog cp(this);
		switch (type)
		{
		case NewEntry::Text:
			new_entry->setType(EntryType::fromId("txt"));
			new_entry->setExtensionByType();
			break;

		case NewEntry::Palette:
			// Import a palette from the available ones
			cp.CenterOnParent();
			if (cp.ShowModal() == wxID_OK)
			{
				Palette* pal;
				int      choice = cp.getChoice();
				if (choice)
					pal = app::paletteManager()->palette(choice - 1);
				else
					pal = app::paletteManager()->globalPalette();
				pal->saveMem(mc);
			}
			else
			{
				mc.reSize(256 * 3);
			}
			new_entry->importMemChunk(mc);
			new_entry->setType(EntryType::fromId("palette"));
			new_entry->setExtensionByType();
			break;

		case NewEntry::Animated:
			// Import the ZDoom definitions as a baseline
			e_import = app::archiveManager().programResourceArchive()->entryAtPath("animated.lmp");
			if (e_import)
				new_entry->importEntry(e_import);
			EntryType::detectEntryType(*new_entry);
			new_entry->setExtensionByType();
			break;

		case NewEntry::Switches:
			// Import the Boom definitions as a baseline
			e_import = app::archiveManager().programResourceArchive()->entryAtPath("switches.lmp");
			if (e_import)
				new_entry->importEntry(e_import);
			EntryType::detectEntryType(*new_entry);
			new_entry->setExtensionByType();
			break;

		default: break;
		}
	}

	// Add the entry to the archive
	archive->addEntry(new_entry, index, dir);
	focusOnEntry(new_entry.get());
	selectionChanged();

	undo_manager_->endRecord(!!new_entry);

	// Return whether the entry was created ok
	return !!new_entry;
}

// -----------------------------------------------------------------------------
// Adds a new subdirectory to the current directory, but only if the archive
// supports them
// -----------------------------------------------------------------------------
bool ArchivePanel::newDirectory()
{
	// Check the archive is still open
	auto archive = archive_.lock();
	if (!archive)
		return false;

	// Show new directory dialog
	auto* dlg = new ui::NewEntryDialog(this, *archive, entry_tree_->currentSelectedDir(), true);
	if (dlg->ShowModal() != wxID_OK)
		return false;

	// Determine the parent directory
	auto* parent_dir = archive->rootDir().get();
	if (archive->formatInfo().supports_dirs)
	{
		auto dir_path = dlg->parentDirPath().ToStdString();
		strutil::replaceIP(dir_path, "\\", "/");

		parent_dir = archive->dirAtPath(dir_path);
		if (!parent_dir)
			parent_dir = archive->createDir(dir_path).get();
	}

	// Add the directory to the archive
	undo_manager_->beginRecord("Create Directory");
	auto dir = archive->createDir(dlg->entryName().ToStdString(), ArchiveDir::getShared(parent_dir));
	undo_manager_->endRecord(!!dir);

	if (dir)
	{
		focusOnEntry(dir->dirEntry());
		selectionChanged();
	}

	// Return whether the directory was created ok
	return !!dir;
}

// -----------------------------------------------------------------------------
// Opens a file selection dialog and imports any selected files to the current
// directory, using the filenames as entry names
// -----------------------------------------------------------------------------
bool ArchivePanel::importFiles()
{
	// Check the archive is still open
	auto archive = archive_.lock();
	if (!archive)
		return false;

	// Run open files dialog
	filedialog::FDInfo info;
	if (filedialog::openFiles(info, "Choose files to import", "Any File (*.*)|*", this))
	{
		// Get the entry index of the last selected list item
		auto dir   = entry_tree_->currentSelectedDir();
		int  index = archive->entryIndex(entry_tree_->lastSelectedEntry(), dir);

		// If something was selected, add 1 to the index so we add the new entry after the last selected
		if (index >= 0)
			index++;
		else
			index = -1; // If not add to the end of the list

		// Begin recording undo level
		undo_manager_->beginRecord("Import Files");

		// Go through the list of files
		bool ok = true;
		entry_tree_->Freeze();
		ui::showSplash("Importing Files...", true, maineditor::windowWx());
		for (size_t a = 0; a < info.filenames.size(); a++)
		{
			// Get filename
			wxString name = wxFileName(info.filenames[a]).GetFullName();

			// Update splash window
			ui::setSplashProgress(a, info.filenames.size());
			ui::setSplashProgressMessage(name.ToStdString());

			// Create new entry
			auto new_entry = std::make_shared<ArchiveEntry>(name.ToStdString());

			// Import file to entry
			if (new_entry->importFile(info.filenames[a]))
			{
				archive->addEntry(new_entry, index, dir); // Add the entry to the archive
				EntryType::detectEntryType(*new_entry);   // Detect entry type
			}
			else
			{
				log::error(global::error);
				ok = false;
			}

			if (index > 0)
				index++;
		}
		ui::hideSplash();
		entry_tree_->Thaw();

		// End recording undo level
		undo_manager_->endRecord(true);

		return ok;
	}
	else // User cancelled
		return false;
}

// -----------------------------------------------------------------------------
// Opens a dir selection dialog and imports any files within the selected
// directory into the current directory, using the filenames as entry names
// -----------------------------------------------------------------------------
bool ArchivePanel::importDir()
{
	// Check the archive is still open
	auto archive = archive_.lock();
	if (!archive)
		return false;

	// Run open dir dialog
	auto path = filedialog::openDirectory("Choose directory to import", this);
	if (!path.empty())
	{
		auto dir = ArchiveDir::getShared(entry_tree_->currentSelectedDir());

		// Begin recording undo level
		undo_manager_->beginRecord("Import Directory");

		entry_tree_->Freeze();
		ui::showSplash("Importing Files...", true, maineditor::windowWx());

		// Import the directory
		auto ok = archive->importDir(path, archive_dir_ignore_hidden, dir);

		ui::hideSplash();
		entry_tree_->Thaw();

		// End recording undo level
		undo_manager_->endRecord(true);

		return ok;
	}
	else // User cancelled
		return false;
}

// -----------------------------------------------------------------------------
// Not implemented
// -----------------------------------------------------------------------------
bool ArchivePanel::convertArchiveTo() const
{
	wxMessageBox("Not Implemented");
	return false;
}

// -----------------------------------------------------------------------------
// Not implemented
// -----------------------------------------------------------------------------
bool ArchivePanel::cleanupArchive() const
{
	wxMessageBox("Not Implemented");
	return false;
}

// -----------------------------------------------------------------------------
// Build pk3/zip archive from the current directory
// -----------------------------------------------------------------------------
bool ArchivePanel::buildArchive() const
{
	// Check the archive is still open
	auto archive = archive_.lock();
	if (!archive)
		return false;

	if (archive->format() != ArchiveFormat::Dir)
	{
		wxMessageBox("This function is only supported with directories", "Can't build archive", wxICON_ERROR);
		return false;
	}

	return archiveoperations::buildArchive(archive->filename());
}

// -----------------------------------------------------------------------------
// Opens a dialog to rename the selected entries.
// If multiple entries are selected, a mass-rename is performed
// -----------------------------------------------------------------------------
bool ArchivePanel::renameEntry(bool each) const
{
	// Check the archive is still open
	auto archive = archive_.lock();
	if (!archive)
		return false;

	// Get selected entries & directories
	auto selection     = entry_tree_->selectedEntries();
	auto selected_dirs = entry_tree_->selectedDirectories();

	// Do rename (with undo level)
	undo_manager_->beginRecord("Rename Entry");
	entryoperations::rename(selection, archive.get(), each);
	entryoperations::renameDir(selected_dirs, archive.get());
	undo_manager_->endRecord(true);

	// Update UI
	for (auto* entry : selection)
		maineditor::window()->archiveManagerPanel()->updateEntryTabTitle(entry);

	return true;
}

// -----------------------------------------------------------------------------
// Deletes any selected entries from the archive
// -----------------------------------------------------------------------------
bool ArchivePanel::deleteEntry(bool confirm)
{
	// Check the archive is still open
	auto archive = archive_.lock();
	if (!archive)
		return false;

	// Get a list of selected entries
	auto selected_entries = entry_tree_->selectedEntries();

	// Get a list of selected directories
	auto selected_dirs = entry_tree_->selectedDirectories();

	if (selected_entries.empty() && selected_dirs.empty())
		return false;

	// Confirmation dialog
	if (confirm_entry_delete && confirm)
	{
		wxString item;
		int      num = selected_entries.size() + selected_dirs.size();
		if (num == 1)
		{
			if (selected_entries.size() == 1)
				item = selected_entries[0]->name();
			else
				item = selected_dirs[0]->name();
		}
		else if (num > 0)
			item = wxString::Format("these %d items", num);

		if (wxMessageBox(
				wxString::Format("Are you sure you want to delete %s?", item),
				"Delete Confirmation",
				wxYES_NO | wxICON_QUESTION)
			!= wxYES)
			return false;
	}

	// Clear the selection
	entry_tree_->UnselectAll();

	// Begin recording undo level
	undo_manager_->beginRecord("Delete Entry");

	// Go through the selected entries
	entry_tree_->Freeze();
	for (int a = selected_entries.size() - 1; a >= 0; a--)
	{
		// Remove from bookmarks
		app::archiveManager().deleteBookmark(selected_entries[a]);

		// Close entry tab if open
		maineditor::window()->archiveManagerPanel()->closeEntryTab(selected_entries[a]);

		// Remove the current selected entry if it isn't a directory
		if (selected_entries[a]->type() != EntryType::folderType())
			archive->removeEntry(selected_entries[a]);
	}

	// Go through the selected directories
	for (int a = selected_dirs.size() - 1; a >= 0; a--)
	{
		// Remove from bookmarks
		app::archiveManager().deleteBookmarksInDir(selected_dirs[a]);

		// Close entry tabs
		for (const auto& entry : selected_dirs[a]->entries())
			maineditor::window()->archiveManagerPanel()->closeEntryTab(entry.get());

		// Remove the selected directory from the archive
		archive->removeDir(selected_dirs[a]->path());
	}
	entry_tree_->Thaw();

	// Finish recording undo level
	undo_manager_->endRecord(true);

	// Switch to blank entry panel
	cur_area_->Show(false);
	cur_area_->nullEntry();
	splitter_->ReplaceWindow(cur_area_, entry_area_);
	cur_area_ = entry_area_;
	cur_area_->Show(true);
	Layout();

	return true;
}

// -----------------------------------------------------------------------------
// Reverts any selected entries
// -----------------------------------------------------------------------------
bool ArchivePanel::revertEntry() const
{
	// Check the archive is still open
	auto archive = archive_.lock();
	if (!archive)
		return false;

	// Prompt to revert if configured to
	if (confirm_entry_revert)
		if (wxMessageBox(
				"Are you sure you want to revert changes made to the entry?",
				"Revert Changes",
				wxICON_QUESTION | wxYES_NO)
			== wxNO)
			return false;

	// Get selected entries
	auto selected_entries = entry_tree_->selectedEntries();

	// Begin recording undo level
	undo_manager_->beginRecord("Revert Entry");

	// Go through selection
	for (auto& entry : selected_entries)
	{
		undo_manager_->recordUndoStep(std::make_unique<EntryDataUS>(entry));
		archive->revertEntry(entry);
	}

	// Finish recording undo level
	undo_manager_->endRecord(true);

	// Reload entry if currently open
	if (selected_entries.size() == 1 && maineditor::currentEntryPanel()
		&& maineditor::currentEntryPanel()->entry() == selected_entries[0])
		maineditor::currentEntryPanel()->openEntry(selected_entries[0]);

	// If the entries reverted were the only modified entries in the
	// archive, the archive is no longer modified.
	archive->findModifiedEntries();

	return true;
}

// -----------------------------------------------------------------------------
// Moves any selected entries up in the list
// -----------------------------------------------------------------------------
bool ArchivePanel::moveUp() const
{
	auto archive = archive_.lock();

	// Check we can move
	if (!archive || entry_tree_->GetSelectedItemsCount() == 0 || !canMoveEntries())
		return false;

	// Check selection is valid for move action (all entries must be in same dir)
	auto* dir = entry_tree_->selectedEntriesDir();
	if (!dir)
	{
		log::warning("Can't move selected entries - all selected entries must be in the same directory");
		return false;
	}

	// Get selection
	wxDataViewItemArray sel_items;
	entry_tree_->GetSelections(sel_items);
	auto sel_entries = entry_tree_->selectedEntries();
	auto focus       = entry_tree_->GetCurrentItem();
	auto first       = entry_tree_->firstSelectedItem();
	if (sel_entries.empty())
		return false;

	// If the first selected item is the first entry in the directory, don't move
	if (sel_entries[0]->index() == 0)
		return false;

	// Move each one up by swapping it with the entry above it
	entry_tree_->Freeze();
	undo_manager_->beginRecord("Move Up");
	for (auto* entry : sel_entries)
		archive->swapEntries(entry->index(), entry->index() - 1, dir);
	undo_manager_->endRecord(true);

	// Update selection
	entry_tree_->SetSelections(sel_items);
	entry_tree_->SetCurrentItem(focus);

	// Ensure top-most entry is visible
	entry_tree_->EnsureVisible(first);
	entry_tree_->GetModel()->Resort();
	entry_tree_->Thaw();

	// Return success
	return true;
}

// -----------------------------------------------------------------------------
// Moves any selected entries down in the list
// -----------------------------------------------------------------------------
bool ArchivePanel::moveDown() const
{
	auto archive = archive_.lock();

	// Check we can move
	if (!archive || entry_tree_->GetSelectedItemsCount() == 0 || !canMoveEntries())
		return false;

	// Check selection is valid for move action (all entries must be in same dir)
	auto* dir = entry_tree_->selectedEntriesDir();
	if (!dir)
	{
		log::warning("Can't move selected entries - all selected entries must be in the same directory");
		return false;
	}

	// Get selection
	wxDataViewItemArray sel_items;
	entry_tree_->GetSelections(sel_items);
	auto sel_entries = entry_tree_->selectedEntries();
	auto focus       = entry_tree_->GetCurrentItem();
	auto last        = entry_tree_->lastSelectedItem();
	if (sel_entries.empty())
		return false;

	// If the last selected item the last entry in the directory, don't move
	if (sel_entries.back()->index() == dir->numEntries() - 1)
		return false;

	// Move each one down by swapping it with the entry below it
	entry_tree_->Freeze();
	undo_manager_->beginRecord("Move Down");
	for (int a = sel_entries.size() - 1; a >= 0; a--)
		archive->swapEntries(sel_entries[a]->index(), sel_entries[a]->index() + 1, dir);
	undo_manager_->endRecord(true);

	// Update selection
	entry_tree_->SetSelections(sel_items);
	entry_tree_->SetCurrentItem(focus);

	// Ensure bottom-most entry is visible
	entry_tree_->EnsureVisible(last);
	entry_tree_->GetModel()->Resort();
	entry_tree_->Thaw();

	// Return success
	return true;
}

// -----------------------------------------------------------------------------
// Sorts all selected entries. If the selection is empty or only contains one
// single entry, sort the entire archive instead.
// -----------------------------------------------------------------------------
bool ArchivePanel::sort() const
{
	// Check the archive is still open
	auto archive = archive_.lock();
	if (!archive)
		return false;

	// Get selected entries and their parent dir
	auto sel_entries = entry_tree_->selectedEntries();
	auto dir         = entry_tree_->selectedEntriesDir();
	if (!dir)
		return false; // Must all be in the same dir

	// Perform sort
	entry_tree_->Freeze();
	auto success = entryoperations::sortEntries(*archive, sel_entries, *dir, undo_manager_.get());
	entry_tree_->Thaw();

	return success;
}

// -----------------------------------------------------------------------------
// Toggles a bookmark on the currently focused entry
// -----------------------------------------------------------------------------
bool ArchivePanel::bookmark() const
{
	// Entry
	if (auto entry = entry_tree_->firstSelectedEntry())
	{
		if (app::archiveManager().isBookmarked(entry))
			app::archiveManager().deleteBookmark(entry);
		else
			app::archiveManager().addBookmark(entry->getShared());

		return true;
	}

	// Directory
	if (auto dir = entry_tree_->firstSelectedDirectory())
	{
		auto dir_entry = dir->dirEntryShared();

		if (app::archiveManager().isBookmarked(dir_entry.get()))
			app::archiveManager().deleteBookmark(dir_entry.get());
		else
			app::archiveManager().addBookmark(dir_entry);

		return true;
	}

	// Invalid selection
	return false;
}

// -----------------------------------------------------------------------------
// Opens currently selected entries in separate tabs
// -----------------------------------------------------------------------------
bool ArchivePanel::openTab() const
{
	// Get selected entries
	auto selection = entry_tree_->selectedEntries();

	// Open each in its own tab
	for (auto& entry : selection)
	{
		if (strutil::startsWith(entry->typeString(), "archive_"))
			app::archiveManager().openArchive(entry);
		else
			maineditor::openEntry(entry);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Computes the CRC-32 checksums of the selected entries
// -----------------------------------------------------------------------------
bool ArchivePanel::crc32() const
{
	// Get selected entries
	auto selection = entry_tree_->selectedEntries();

	// Compute CRC-32 checksums for each
	wxString checksums = "\nCRC-32:\n";
	for (auto& entry : selection)
	{
		uint32_t crc = entry->data().crc();
		checksums += wxString::Format("%s:\t%x\n", entry->name(), crc);
	}
	log::info(1, checksums);
	wxMessageBox(checksums);

	return true;
}

// -----------------------------------------------------------------------------
// Not implemented
// -----------------------------------------------------------------------------
bool ArchivePanel::convertEntryTo() const
{
	wxMessageBox("Not Implemented");
	return false;
}

// -----------------------------------------------------------------------------
// For each selected entry, opens a file selection dialog. The selected file
// will be imported into the entry
// -----------------------------------------------------------------------------
bool ArchivePanel::importEntry()
{
	// Get a list of selected entries
	auto selection = entry_tree_->selectedEntries();

	// Begin recording undo level
	undo_manager_->beginRecord("Import Entry");

	// Go through the list
	bool entry_type_changed = false;
	for (auto& entry : selection)
	{
		// Run open file dialog
		filedialog::FDInfo info;
		if (filedialog::openFile(info, "Import Entry \"" + entry->name() + "\"", "Any File (*.*)|*", this))
		{
			// Preserve gfx offset if needed
			Vec2i offset;
			if (entry->type()->editor() == "gfx")
			{
				// We have an image
				SImage si;
				si.open(entry->data());
				offset = si.offset();
			}

			// Get current entry type
			auto cur_type = entry->type();

			// Create undo step
			undo_manager_->recordUndoStep(std::make_unique<EntryDataUS>(entry));

			// If a file was selected, import it
			if (!entry->importFile(info.filenames[0]))
			{
				wxMessageBox(
					wxString::Format("Failed to import file: %s", global::error), "Import Failed", wxICON_ERROR, this);
				return false;
			}

			// Re-detect entry type
			EntryType::detectEntryType(*entry);

			// Restore offsets if needed
			if (entry->type()->editor() == "gfx")
			{
				SImage si;
				si.open(entry->data());

				Vec2i noffset = si.offset();
				bool  ok      = true;
				// Don't bother if the same offsets are reimported
				if (offset == noffset)
					ok = false;
				// Ask for confirmation if there actually are offsets but they are different
				else if (noffset.x | noffset.y)
				{
					wxMessageDialog md(
						this,
						wxString::Format(
							"Image %s had offset [%d, %d], imported file has offset [%d, %d]. "
							"Do you want to keep the old offset and override the new?",
							entry->name(),
							offset.x,
							offset.y,
							noffset.x,
							noffset.y),
						"Conflicting Offsets",
						wxYES_NO);
					int result = md.ShowModal();
					if (result != wxID_YES)
						ok = false;
				}
				// Warn if the offsets couldn't be written
				if (ok && si.format() && !si.format()->writeOffset(si, entry, offset))
					log::warning(wxString::Format(
						"Old offset information [%d, %d] couldn't be "
						"preserved in the new image format for image %s.",
						offset.x,
						offset.y,
						entry->name()));
			}

			// Check if the entry type changed
			if (entry->type() != cur_type)
				entry_type_changed = true;

			// If the entry is currently open, refresh the entry panel
			if (cur_area_->entry() == entry)
				openEntry(entry, true);
		}
	}

	// Prompt to update entry extensions
	if (entry_type_changed && archive_.lock()->formatInfo().names_extensions)
	{
		auto multi_select = selection.size() > 1;

		if (wxMessageBox(
				multi_select ? "One or more entry types were changed. Update entry extensions?"
							 : "The entry type has changed. Update it's extension?",
				"Update Entry Extension(s)",
				wxYES_NO | wxICON_QUESTION)
			== wxYES)
		{
			for (auto& entry : selection)
				entry->setExtensionByType();
		}
	}

	// Finish recording undo level
	undo_manager_->endRecord(true);

	return true;
}

// -----------------------------------------------------------------------------
// Exports any selected entries to files. If multiple entries are selected, a
// directory selection dialog is shown, and any selected entries will be
// exported to that directory
// -----------------------------------------------------------------------------
bool ArchivePanel::exportEntry() const
{
	// Get a list of selected entries & dirs
	auto selection     = entry_tree_->selectedEntries();
	auto selected_dirs = entry_tree_->selectedDirectories();

	// If we're just exporting 1 entry
	if (selection.size() == 1 && selected_dirs.empty())
		return entryoperations::exportEntry(selection[0]);

	// Multiple entries/dirs
	return entryoperations::exportEntries(selection, selected_dirs);
}

// -----------------------------------------------------------------------------
// Not implemented
// -----------------------------------------------------------------------------
bool ArchivePanel::exportEntryAs() const
{
	wxMessageBox("Not Implemented");
	return false;
}

// -----------------------------------------------------------------------------
// Copies selected entries+directories to the clipboard
// -----------------------------------------------------------------------------
bool ArchivePanel::copyEntry() const
{
	// Get a list of selected entries
	auto entries = entry_tree_->selectedEntries();

	// Get a list of selected directories
	auto dirs = entry_tree_->selectedDirectories();

	// Do nothing if nothing selected
	if (entries.empty() && dirs.empty())
		return false;

	// Create clipboard item from selection
	app::clipboard().clear();
	app::clipboard().add(std::make_unique<EntryTreeClipboardItem>(entries, dirs));

	return true;
}

// -----------------------------------------------------------------------------
// Copies selected entries+diretories to the clipboard, and deletes them from
// the archive
// -----------------------------------------------------------------------------
bool ArchivePanel::cutEntry()
{
	if (copyEntry())
		return deleteEntry(false);
	else
		return false;
}

// -----------------------------------------------------------------------------
// Pastes any entries and directories on the clipboard into the current
// directory. Entries will be pasted after the last selected entry, whereas
// directories will be pasted after any subdirectories. Pasting a directory
// will also paste any entries and subdirectories within it.
// -----------------------------------------------------------------------------
bool ArchivePanel::pasteEntry() const
{
	// Do nothing if there is nothing in the clipboard
	if (app::clipboard().empty())
		return false;

	// Check the archive is still open
	auto archive = archive_.lock();
	auto dir     = ArchiveDir::getShared(entry_tree_->currentSelectedDir());
	if (!archive || !dir)
		return false;

	// Get the entry index of the last selected list item
	int index = archive->entryIndex(entry_tree_->lastSelectedEntry(), dir.get());

	// If something was selected, add 1 to the index so we add the new entry after the last selected
	if (index >= 0)
		index++;
	else
		index = -1; // If not add to the end of the list

	// Go through all clipboard items
	auto panel = theMainWindow->archiveManagerPanel();
	panel->disableArchiveListUpdate();
	bool pasted = false;
	undo_manager_->beginRecord("Paste Entry");
	entry_tree_->Freeze();
	for (unsigned a = 0; a < app::clipboard().size(); a++)
	{
		// Check item type
		if (app::clipboard().item(a)->type() != ClipboardItem::Type::EntryTree)
			continue;

		// Get clipboard item
		auto clip = dynamic_cast<EntryTreeClipboardItem*>(app::clipboard().item(a));

		// Merge it in
		if (archive->paste(clip->tree(), index, dir))
			pasted = true;
	}
	undo_manager_->endRecord(true);
	entry_tree_->Thaw();
	panel->refreshArchiveList();
	if (pasted)
	{
		// Update archive
		archive->setModified(true);

		return true;
	}
	else
		return false;
}

// -----------------------------------------------------------------------------
// Opens selected entries in their configured external editors
// -----------------------------------------------------------------------------
bool ArchivePanel::openEntryExternal() const
{
	auto selection = entry_tree_->selectedEntries();
	for (auto& entry : selection)
	{
		// Open entry in selected external editor
		bool ok = ee_manager_->openEntryExternal(
			*entry, current_external_exes_[wx_id_offset_], current_external_exe_category_);

		// Show error message if failed
		if (!ok)
			wxMessageBox(
				wxString::Format("Failed opening %s in external editor: %s", entry->name(), global::error),
				"External Edit Failed",
				wxOK | wxICON_ERROR);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Returns the entry currently open for editing
// -----------------------------------------------------------------------------
ArchiveEntry* ArchivePanel::currentEntry() const
{
	if (entry_tree_->GetSelectedItemsCount() == 1)
		return cur_area_->entry();
	else
		return nullptr;
}

// -----------------------------------------------------------------------------
// Returns a vector of all selected entries
// -----------------------------------------------------------------------------
vector<ArchiveEntry*> ArchivePanel::currentEntries() const
{
	vector<ArchiveEntry*> selection;
	if (entry_tree_)
		selection = entry_tree_->selectedEntries();
	return selection;
}

// -----------------------------------------------------------------------------
// Returns the currently open dir
// -----------------------------------------------------------------------------
ArchiveDir* ArchivePanel::currentDir() const
{
	// if (entry_list_)
	//	return entry_list_->currentDir().lock().get();
	return nullptr;
}

// -----------------------------------------------------------------------------
// Converts any selected SWANTBLS entries to SWITCHES and ANIMATED
// -----------------------------------------------------------------------------
bool ArchivePanel::swanConvert() const
{
	// Check the archive is still open
	auto archive = archive_.lock();
	if (!archive)
		return false;

	// Get the entry index of the last selected list item
	auto lastEntry = entry_tree_->lastSelectedEntry();
	if (!lastEntry)
		return false;
	int index = lastEntry->index();

	// If something was selected, add 1 to the index so we add the new entry after the last selected
	if (index >= 0)
		index++;

	// MemChunks for ANIMATED and SWITCHES
	MemChunk mca, mcs;

	// Get a list of selected entries
	auto selection = entry_tree_->selectedEntries();
	bool error     = false;

	// Check each selected entry for possible conversion
	for (auto& entry : selection)
	{
		if (entry->type()->id() == "swantbls")
		{
			error |= !entryoperations::convertSwanTbls(entry, &mca, false);
			error |= !entryoperations::convertSwanTbls(entry, &mcs, true);
		}
	}

	// Close off ANIMATED lump if needed
	if (mca.size())
	{
		uint8_t buffer = 255;
		error |= !mca.reSize(mca.size() + 1, true);
		error |= !mca.write(&buffer, 1);
	}
	// Close off SWITCHES lump if needed
	if (mcs.size())
	{
		uint8_t buffer[20] = {};
		error |= !mcs.reSize(mcs.size() + 20, true);
		error |= !mcs.write(buffer, 20);
	}

	// Create entries
	MemChunk* mc[2]       = { &mca, &mcs };
	string    wadnames[2] = { "ANIMATED", "SWITCHES" };
	string    zipnames[2] = { "animated.lmp", "switches.lmp" };
	string    etypeids[2] = { "animated", "switches" };
	for (int e = 0; e < 2; ++e)
	{
		if (mc[e]->size())
		{
			// Begin recording undo level
			undo_manager_->beginRecord(fmt::format("Create {}", wadnames[e]));

			auto output = archive->addNewEntry(
				(archive->format() == ArchiveFormat::Wad ? wadnames[e] : zipnames[e]), index, lastEntry->parentDir());
			if (output)
			{
				error |= !output->importMemChunk(*mc[e]);
				EntryType::detectEntryType(*output);
				if (output->type() == EntryType::unknownType())
					output->setType(EntryType::fromId(etypeids[e]));
				if (index >= 0)
					index++;
			}
			else
				error = true;
			// Finish recording undo level
			undo_manager_->endRecord(true);
		}
	}
	return !error;
}

// -----------------------------------------------------------------------------
// Converts any selected SWITCHES or ANIMATED entries to a newly created
// ANIMDEFS or SWANTBLS entry
// -----------------------------------------------------------------------------
bool ArchivePanel::basConvert(bool animdefs)
{
	// Check the archive is still open
	auto archive = archive_.lock();
	if (!archive)
		return false;

	// Get the entry index of the last selected list item
	auto lastEntry = entry_tree_->lastSelectedEntry();
	if (!lastEntry)
		return false;
	int index = lastEntry->index();

	// If something was selected, add 1 to the index so we add the new entry after the last selected
	if (index >= 0)
		index++;

	// Get a list of selected entries
	auto selection = entry_tree_->selectedEntries();

	// Begin recording undo level
	undo_manager_->beginRecord("Convert to ANIMDEFS");

	// Create new entry
	auto output = archive->addNewEntry(
		(animdefs ? (archive->format() == ArchiveFormat::Wad ? "ANIMDEFS" : "animdefs.txt")
				  : (archive->format() == ArchiveFormat::Wad ? "SWANTBLS" : "swantbls.dat")),
		index,
		lastEntry->parentDir());

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

		MemChunk animdata(gentext.size());
		animdata.seek(0, SEEK_SET);
		animdata.write(gentext.data(), gentext.size());

		// Check each selected entry for possible conversion
		for (auto& entry : selection)
		{
			if (entry->type()->formatId() == "animated")
				entryoperations::convertAnimated(entry, &animdata, animdefs);
			else if (entry->type()->formatId() == "switches")
				entryoperations::convertSwitches(entry, &animdata, animdefs);
		}
		output->importMemChunk(animdata);

		// Identify the new lump as what it is
		EntryType::detectEntryType(*output);
		// Failsafe is detectEntryType doesn't accept to work, grumble
		if (output->type() == EntryType::unknownType())
			output->setType(EntryType::fromId("animdefs"));
	}

	// Finish recording undo level
	undo_manager_->endRecord(true);

	// Force entrylist width update
	Layout();

	return (output != nullptr);
}

// -----------------------------------------------------------------------------
// Unused (converts 6-bit palette to 8-bit)
// -----------------------------------------------------------------------------
bool ArchivePanel::palConvert() const
{
	// Get the entry index of the last selected list item
	auto            pal6bit = currentEntry();
	auto            source  = pal6bit->rawData();
	vector<uint8_t> dest(pal6bit->size());
	memcpy(dest.data(), source, pal6bit->size());
	for (size_t i = 0; i < pal6bit->size(); ++i)
		dest[i] = ((dest[i] << 2) | (dest[i] >> 4));
	pal6bit->importMem(dest.data(), pal6bit->size());
	maineditor::currentEntryPanel()->callRefresh();
	return true;
}

// -----------------------------------------------------------------------------
// Converts selected mus format entries to midi format
// -----------------------------------------------------------------------------
bool ArchivePanel::musMidiConvert() const
{
	// Get selected entries
	auto selection = entry_tree_->selectedEntries();

	// Begin recording undo level
	undo_manager_->beginRecord("Convert Mus -> Midi");

	// Go through selection
	for (auto& entry : selection)
	{
		// Convert MUS -> MIDI if the entry is a MIDI-like format
		const auto& format_id = entry->type()->formatId();
		if (strutil::startsWith(format_id, "midi_") && format_id != "midi_smf")
		{
			MemChunk midi;
			undo_manager_->recordUndoStep(std::make_unique<EntryDataUS>(entry)); // Create undo step
			if (format_id == "midi_mus")
				conversion::musToMidi(entry->data(), midi); // Convert
			else if (format_id == "midi_gmid")
				conversion::gmidToMidi(entry->data(), midi); // Convert
			else
				conversion::zmusToMidi(entry->data(), midi); // Convert
			entry->importMemChunk(midi);                     // Load midi data
			EntryType::detectEntryType(*entry);              // Update entry type
			entry->setExtensionByType();                     // Update extension if necessary
		}
	}

	// Finish recording undo level
	undo_manager_->endRecord(true);

	return true;
}

// -----------------------------------------------------------------------------
// Compiles any selected text entries as ACS scripts
// -----------------------------------------------------------------------------
bool ArchivePanel::compileACS(bool hexen) const
{
	// Get selected entries
	auto selection = entry_tree_->selectedEntries();

	// Go through selection
	for (auto& entry : selection)
	{
		// Compile ACS script
		entryoperations::compileACS(entry, hexen, nullptr, theMainWindow);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Compiles any selected text entries as DECOHack code
// -----------------------------------------------------------------------------
bool ArchivePanel::compileDECOHack() const
{
	// Get selected entries
	auto selection = entry_tree_->selectedEntries();

	// Go through selection
	for (auto& entry : selection)
	{
		// Compile
		entryoperations::compileDECOHack(entry, nullptr, theMainWindow);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Converts any selected TEXTUREx entries to a ZDoom TEXTURES entry
// -----------------------------------------------------------------------------
bool ArchivePanel::convertTextures() const
{
	// Get selected entries
	auto selection = entry_tree_->selectedEntries();

	// Begin recording undo level
	undo_manager_->beginRecord("Convert TEXTUREx -> TEXTURES");

	// Do conversion
	if (entryoperations::convertTextures(selection))
	{
		// Finish recording undo level
		undo_manager_->endRecord(true);

		return true;
	}

	// Finish recording undo level
	undo_manager_->endRecord(false);

	return false;
}

// -----------------------------------------------------------------------------
// Opens the given directory.
// -----------------------------------------------------------------------------
bool ArchivePanel::openDir(const shared_ptr<ArchiveDir>& dir) const
{
	return false; // entry_list_->setDir(dir);
}

// -----------------------------------------------------------------------------
// Closes the current entry in archive tab.
// -----------------------------------------------------------------------------
void ArchivePanel::closeCurrentEntry()
{
	// Close the current entry
	showEntryPanel(nullptr, false);
}

// -----------------------------------------------------------------------------
// Shows the appropriate entry area and sends the given entry to it.
// If [force] is true, the entry is opened even if it is already open
// -----------------------------------------------------------------------------
bool ArchivePanel::openEntry(ArchiveEntry* entry, bool force)
{
	// Null entry, do nothing
	if (!entry)
	{
		log::warning("NULL entry focused in the list");
		return false;
	}

	// Check the archive is still open
	auto archive = archive_.lock();
	if (!archive)
		return false;

	// First check if the entry is already open in its own tab
	auto am_panel = theMainWindow->archiveManagerPanel();

	// Do nothing if the entry is already open
	if (cur_area_->entry() == entry && !force)
		return false;

	// Detect entry type if it hasn't been already
	if (entry->type() == EntryType::unknownType())
		EntryType::detectEntryType(*entry);

	// Are we trying to open a directory? This can happen from bookmarks.
	if (entry->type() == EntryType::folderType())
	{
		// Get directory to open
		auto dir = ArchiveDir::findDirByDirEntry(archive->rootDir(), *entry);

		// Check it exists (really should)
		if (!dir)
		{
			log::error(wxString::Format("Trying to open nonexistant directory %s", entry->path(true)));
			return false;
		}

		entry_tree_->goToDir(dir, true);
	}
	else
	{
		// Save changes if needed
		saveEntryChanges();

		// Close the current entry
		cur_area_->closeEntry();

		// Get the appropriate entry panel for the entry's type
		auto new_area = default_area_;
		if (am_panel->entryIsOpenInTab(entry))
			new_area = default_area_;
		else if (entry->type() == EntryType::mapMarkerType())
			new_area = mapArea();
		else if (entry->isArchive())
		{
			// Check for archive that is a map (eg. maps/ in a zip)
			if (archive->mapDesc(entry).head.lock().get() == entry)
				new_area = mapArea();
		}
		else if (entry->type()->editor() == "gfx")
			new_area = gfxArea();
		else if (entry->type()->editor() == "palette")
			new_area = palArea();
		else if (entry->type()->editor() == "ansi")
			new_area = ansiArea();
		else if (entry->type()->editor() == "text")
			new_area = textArea();
		else if (entry->type()->editor() == "audio")
			new_area = audioArea();
		else if (entry->type()->editor() == "data")
			new_area = dataArea();
		else if (entry->type()->editor() == "default")
			new_area = default_area_;
		else
			log::warning(
				wxString::Format("Entry editor %s does not exist, using default editor", entry->type()->editor()));

		// Load the entry into the panel
		if (!new_area->openEntry(entry))
			wxMessageBox(wxString::Format("Error loading entry:\n%s", global::error), "Error", wxOK | wxICON_ERROR);

		// Show the new entry panel
		bool changed = (cur_area_ != new_area);
		if (!showEntryPanel(new_area, false))
			return false;
		else if (changed)
			new_area->updateToolbar();
	}
	return true;
}

// -----------------------------------------------------------------------------
// Opens [entry] in the text editor panel
// -----------------------------------------------------------------------------
bool ArchivePanel::openEntryAsText(const ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// First check if the entry is already open in its own tab
	auto panel = theMainWindow->archiveManagerPanel();
	if (panel->redirectToTab(entry))
		return true;

	// Load the current entry into the panel
	if (!textArea()->openEntry(entry))
		wxMessageBox(wxString::Format("Error loading entry:\n%s", global::error), "Error", wxOK | wxICON_ERROR);

	// Show the text entry panel
	return showEntryPanel(textArea());
}

// -----------------------------------------------------------------------------
// Opens [entry] in the hex editor panel
// -----------------------------------------------------------------------------
bool ArchivePanel::openEntryAsHex(const ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// First check if the entry is already open in its own tab
	auto panel = theMainWindow->archiveManagerPanel();
	if (panel->redirectToTab(entry))
		return true;

	// Load the current entry into the panel
	if (!hexArea()->openEntry(entry))
		wxMessageBox(wxString::Format("Error loading entry:\n%s", global::error), "Error", wxOK | wxICON_ERROR);

	// Show the text entry panel
	return showEntryPanel(hexArea());
}

// -----------------------------------------------------------------------------
// If only one entry is selected, force its reload
// -----------------------------------------------------------------------------
bool ArchivePanel::reloadCurrentPanel()
{
	// Do nothing if there are multiple entries
	if (cur_area_ == default_area_)
		return false;

	return openEntry(cur_area_->entry(), true);
}

// -----------------------------------------------------------------------------
// Makes sure the list view display shows the given entry
// -----------------------------------------------------------------------------
void ArchivePanel::focusOnEntry(ArchiveEntry* entry) const
{
	if (!entry)
		return;

	wxDataViewItem item{ entry };
	entry_tree_->EnsureVisible(item);
	entry_tree_->SetSelections(wxDataViewItemArray(1, item));
}

// -----------------------------------------------------------------------------
// Sets focus to the entry list
// -----------------------------------------------------------------------------
void ArchivePanel::focusEntryList() const
{
	entry_tree_->SetFocus();
}

// -----------------------------------------------------------------------------
// Show an entry panel appropriate to the current entry
// -----------------------------------------------------------------------------
bool ArchivePanel::showEntryPanel(EntryPanel* new_area, bool ask_save)
{
	// Save any changes if needed
	if (ask_save)
		saveEntryChanges();

	// If the new panel is different than the current, swap them
	if (new_area != cur_area_)
	{
		Freeze();
		cur_area_->Show(false);        // Hide current
		cur_area_->removeCustomMenu(); // Remove current custom menu (if any)
		if (new_area != nullptr)
		{
			splitter_->ReplaceWindow(cur_area_, new_area); // Swap the panels
			cur_area_ = new_area;                          // Set the new panel to current
			cur_area_->Show(true);                         // Show current

			// Add the current panel's custom menu and toolbar if needed
			cur_area_->addCustomMenu();
		}

		// Set panel undo manager
		cur_area_->setUndoManager(undo_manager_.get());

		// Update panel layout
		Layout();
		Thaw();
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

// -----------------------------------------------------------------------------
// Refreshes everything on the panel
// -----------------------------------------------------------------------------
void ArchivePanel::refreshPanel()
{
	// Refresh entry list
	updateFilter();

	// Refresh current entry panel
	cur_area_->refreshPanel();

	// Update filter control visibility
	panel_filter_->Show(elist_show_filter);

	// Refresh entire panel
	Freeze();
	toolbar_elist_->updateLayout();
	splitter_->GetWindow1()->Layout();
	splitter_->GetWindow1()->Update();
	splitter_->GetWindow1()->Refresh();
	splitter_->GetWindow2()->Layout();
	splitter_->GetWindow2()->Update();
	splitter_->GetWindow2()->Refresh();
	Layout();
	Update();
	Refresh();
	Thaw();
}

// -----------------------------------------------------------------------------
// Creates and returns the 'Open In' submenu for the entry context menu, adding
// any external editors for entries of [category]
// -----------------------------------------------------------------------------
wxMenu* ArchivePanel::createEntryOpenMenu(const wxString& category)
{
	current_external_exe_category_ = category;
	current_external_exes_.clear();
	auto menu_open = new wxMenu();

	// New Tab
	SAction::fromId("arch_entry_opentab")->addToMenu(menu_open, true);
	menu_open->AppendSeparator();

	// External editors
	auto     external   = executables::externalExes(category.ToStdString());
	auto     a_open_ext = SAction::fromId("arch_entry_openext");
	unsigned num        = std::min<unsigned>(external.size(), 20);
	for (unsigned a = 0; a < num; a++)
	{
		a_open_ext->addToMenu(menu_open, 0, "With " + external[a].name, "NO", a);
		current_external_exes_.push_back(external[a].name);
	}

	// Add separator if any external editors were added
	if (menu_open->GetMenuItemCount() > 2)
		menu_open->AppendSeparator();

	// Setup external editors
	SAction::fromId("arch_entry_setup_external")->addToMenu(menu_open);

	return menu_open;
}

// -----------------------------------------------------------------------------
// Switches to the default entry panel.
// Returns false if the current entry couldn't be opened in the default entry
// panel, or if the current panel has unsaved changes and the user cancelled
// -----------------------------------------------------------------------------
bool ArchivePanel::switchToDefaultEntryPanel()
{
	if (cur_area_ == default_area_)
		return true;

	if (default_area_->openEntry(cur_area_->entry()))
		return showEntryPanel(default_area_);

	return false;
}

// -----------------------------------------------------------------------------
// Handles the action [id].
// Returns true if the action was handled, false otherwise
// -----------------------------------------------------------------------------
bool ArchivePanel::handleAction(string_view id)
{
	// Don't handle actions if hidden
	if (!IsShown())
		return false;

	// We're only interested in "arch_" actions (and some others)
	if (!strutil::startsWith(id, "arch_") && !strutil::startsWith(id, "pmap_"))
		return false;

	// Check the archive is still open
	auto archive = archive_.lock();
	if (!archive)
		return false;


	// ------------------------------------------------------------------------
	// ENTRY LIST
	// ------------------------------------------------------------------------

	// 'Toggle Filter Controls' button
	if (id == "arch_elist_togglefilter")
	{
		updateFilter();
		panel_filter_->Show(elist_show_filter);
		splitter_->GetWindow1()->Layout();
		splitter_->GetWindow1()->Update();
		splitter_->GetWindow1()->Refresh();
		Layout();
		Refresh();
	}

	// 'Collapse All' button
	else if (id == "arch_elist_collapseall")
	{
		entry_tree_->Freeze();
		entry_tree_->collapseAll(*archive->rootDir());
		entry_tree_->Thaw();
	}

	// 'Up Directory' button
	else if (id == "arch_elist_updir")
		entry_tree_->upDir();

	// 'Home Directory' button
	else if (id == "arch_elist_homedir")
		entry_tree_->homeDir();


	// ------------------------------------------------------------------------
	// ARCHIVE MENU
	// ------------------------------------------------------------------------

	// Archive->New->Entry
	else if (id == "arch_newentry")
		newEntry();

	// Archive->New->Directory
	else if (id == "arch_newdir")
		newDirectory();

	// Archive->Import Files
	else if (id == "arch_importfiles")
		importFiles();

	// Archive->Import Directory
	else if (id == "arch_importdir")
		importDir();

	// Archive->Build Archive
	else if (id == "arch_buildarchive")
		buildArchive();

	// Archive->Texture Editor
	else if (id == "arch_texeditor")
		maineditor::openTextureEditor(archive.get());

	else if (id == "arch_mapeditor")
		maineditor::openMapEditor(archive.get());

	// Archive->Convert To...
	else if (id == "arch_convert")
		convertArchiveTo();

	// Archive->Maintenance->Remove Unused Patches
	else if (id == "arch_clean_patches")
		archiveoperations::removeUnusedPatches(archive.get());

	// Archive->Maintenance->Remove Unused Textures
	else if (id == "arch_clean_textures")
		archiveoperations::removeUnusedTextures(archive.get());

	// Archive->Maintenance->Remove Unused Flats
	else if (id == "arch_clean_flats")
		archiveoperations::removeUnusedFlats(archive.get());

	else if (id == "arch_clean_zdoom_textures")
		archiveoperations::removeUnusedZDoomTextures(archive.get());

	// Archive->Maintenance->Check Duplicate Entry Names
	else if (id == "arch_check_duplicates")
		archiveoperations::checkDuplicateEntryNames(archive.get());

	// Archive->Maintenance->Check Duplicate Entry Names
	else if (id == "arch_check_duplicates2")
		archiveoperations::checkDuplicateEntryContent(archive.get());

	else if (id == "arch_check_zdoom_texture_duplicates")
		archiveoperations::checkDuplicateZDoomTextures(archive.get());

	else if (id == "arch_check_zdoom_patch_duplicates")
		archiveoperations::checkDuplicateZDoomPatches(archive.get());

	// Archive->Maintenance->Check Duplicate Entry Names
	else if (id == "arch_clean_iwaddupes")
		archiveoperations::removeEntriesUnchangedFromIWAD(archive.get());

	else if (id == "arch_check_iwadoverrides")
		archiveoperations::checkOverriddenEntriesInIWAD(archive.get());

	else if (id == "arch_check_zdoomiwadtexoverrides")
		archiveoperations::checkZDoomOverriddenEntriesInIWAD(archive.get());

	// Archive->Maintenance->Replace in Maps
	else if (id == "arch_replace_maps")
	{
		MapReplaceDialog dlg(this, archive.get());
		dlg.ShowModal();
	}

#ifndef NO_LUA
	// Archive->Scripts->...
	else if (id == "arch_script")
		scriptmanager::runArchiveScript(archive.get(), wx_id_offset_);
#endif


	// ------------------------------------------------------------------------
	// ENTRY MENU
	// ------------------------------------------------------------------------

	// Entry->Save
	else if (id == "arch_entry_save")
		saveEntryChanges(true);

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

#ifndef NO_LUA
	// Entry->Run Script
	else if (id == "arch_entry_script")
		scriptmanager::runEntryScript(entry_tree_->selectedEntries(), wx_id_offset_, maineditor::windowWx());
#endif


	// Context menu actions
	else if (id == "arch_bas_convertb")
		basConvert(false);
	else if (id == "arch_bas_convertz")
		basConvert(true);
	else if (id == "arch_swan_convert")
		swanConvert();
	else if (id == "arch_gfx_convert")
		entryoperations::convertGfxEntries(entry_tree_->selectedEntries(), undo_manager_.get());
	else if (id == "arch_gfx_translate")
	{
		entryoperations::remapGfxEntries(
			entry_tree_->selectedEntries(),
			dynamic_cast<GfxEntryPanel*>(gfxArea())->prevTranslation(),
			undo_manager_.get());
	}
	else if (id == "arch_gfx_colourise")
		entryoperations::colourizeGfxEntries(entry_tree_->selectedEntries(), undo_manager_.get());
	else if (id == "arch_gfx_tint")
		entryoperations::tintGfxEntries(entry_tree_->selectedEntries(), undo_manager_.get());
	else if (id == "arch_gfx_offsets")
		entryoperations::modifyOffsets(entry_tree_->selectedEntries(), undo_manager_.get());
	else if (id == "arch_gfx_addptable")
		entryoperations::addToPatchTable(entry_tree_->selectedEntries());
	else if (id == "arch_gfx_addtexturex")
		entryoperations::createTexture(entry_tree_->selectedEntries());
	else if (id == "arch_gfx_exportpng")
		entryoperations::exportEntriesAsPNG(entry_tree_->selectedEntries());
	else if (id == "arch_gfx_pngopt")
		entryoperations::optimizePNGEntries(entry_tree_->selectedEntries(), undo_manager_.get());
	else if (id == "arch_view_text")
		openEntryAsText(entry_tree_->firstSelectedEntry());
	else if (id == "arch_view_hex")
		openEntryAsHex(entry_tree_->firstSelectedEntry());
	else if (id == "arch_audio_convertdw")
		entryoperations::convertSoundToWav(entry_tree_->selectedEntries(), undo_manager_.get());
	else if (id == "arch_audio_convertwd")
		entryoperations::convertWavToDSound(entry_tree_->selectedEntries(), undo_manager_.get());
	else if (id == "arch_audio_convertmus")
		musMidiConvert();
	else if (id == "arch_voxel_convertvox")
		entryoperations::convertVoxelEntries(entry_tree_->selectedEntries(), undo_manager_.get());
	else if (id == "arch_scripts_compileDECOHack")
		compileDECOHack();
	else if (id == "arch_scripts_compileacs")
		compileACS();
	else if (id == "arch_scripts_compilehacs")
		compileACS(true);
	else if (id == "arch_texturex_convertzd")
		convertTextures();
	else if (id == "arch_texturex_finderrors")
		entryoperations::findTextureErrors(entry_tree_->selectedEntries());
	else if (id == "arch_texture_clean_iwaddupes")
		entryoperations::cleanTextureIwadDupes(entry_tree_->selectedEntries());
	else if (id == "arch_zdtextures_clean_singlepatch")
		entryoperations::cleanZdTextureSinglePatch(entry_tree_->selectedEntries());
	else if (id == "arch_map_opendb2")
		entryoperations::openMapDB2(entry_tree_->firstSelectedEntry());
	else if (id == "arch_entry_setup_external")
		ui::SettingsDialog::popupSettingsPage(theMainWindow, ui::SettingsPage::ExternalEditors);

	// Map Editor Panel
	else if (id == "pmap_open_text")
	{
		// Edit Level Script

		// Get entry to edit
		ArchiveEntry* entry = mapArea()->entry();

		// Open in text editor
		openEntryAsText(entry);
	}
	else if (id == "pmap_open_archive")
	{
		// Open map archive (eg. wad in zip)
		ArchiveEntry* entry = mapArea()->entry();
		app::archiveManager().openArchive(entry);
	}

	// Run archive
	else if (id == "arch_run")
	{
		RunDialog dlg(this, archive.get());
		if (dlg.ShowModal() == wxID_OK)
		{
			wxString command = dlg.selectedCommandLine(archive.get(), "");
			if (!command.IsEmpty())
			{
				// Set working directory
				wxString wd = wxGetCwd();
				wxSetWorkingDirectory(dlg.selectedExeDir());

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


// -----------------------------------------------------------------------------
//
// ArchivePanel Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Creates the appropriate EntryPanel for [entry] and returns it
// -----------------------------------------------------------------------------
EntryPanel* ArchivePanel::createPanelForEntry(const ArchiveEntry* entry, wxWindow* parent)
{
	EntryPanel* entry_panel;

	if (entry->type() == EntryType::mapMarkerType())
		entry_panel = new MapEntryPanel(parent);
	else if (entry->type()->editor() == "gfx")
		entry_panel = new GfxEntryPanel(parent);
	else if (entry->type()->editor() == "palette")
		entry_panel = new PaletteEntryPanel(parent);
	else if (entry->type()->editor() == "ansi")
		entry_panel = new ANSIEntryPanel(parent);
	else if (entry->type()->editor() == "text")
		entry_panel = new TextEntryPanel(parent);
	else if (entry->type()->editor() == "audio")
		entry_panel = new AudioEntryPanel(parent);
	else if (entry->type()->editor() == "data")
		entry_panel = new DataEntryPanel(parent);
	else
		entry_panel = new DefaultEntryPanel(parent);

	return entry_panel;
}

// -----------------------------------------------------------------------------
// Creates and returns the 'Maintenance' menu for archives
// -----------------------------------------------------------------------------
wxMenu* ArchivePanel::createMaintenanceMenu()
{
	auto menu_clean = new wxMenu();
	SAction::fromId("arch_clean_patches")->addToMenu(menu_clean);
	SAction::fromId("arch_clean_textures")->addToMenu(menu_clean);
	SAction::fromId("arch_clean_flats")->addToMenu(menu_clean);
	SAction::fromId("arch_clean_zdoom_textures")->addToMenu(menu_clean);
	SAction::fromId("arch_clean_iwaddupes")->addToMenu(menu_clean);
	SAction::fromId("arch_check_iwadoverrides")->addToMenu(menu_clean);
	SAction::fromId("arch_check_zdoomiwadtexoverrides")->addToMenu(menu_clean);
	SAction::fromId("arch_check_duplicates")->addToMenu(menu_clean);
	SAction::fromId("arch_check_duplicates2")->addToMenu(menu_clean);
	SAction::fromId("arch_check_zdoom_texture_duplicates")->addToMenu(menu_clean);
	SAction::fromId("arch_check_zdoom_patch_duplicates")->addToMenu(menu_clean);
	SAction::fromId("arch_replace_maps")->addToMenu(menu_clean);
	return menu_clean;
}

// -----------------------------------------------------------------------------
// Returns true if entries can be moved up/down in the list
// -----------------------------------------------------------------------------
bool ArchivePanel::canMoveEntries() const
{
	// Can't move if entry tree is sorted
	if (!entry_tree_->isDefaultSorted())
		return false;

	// Can't move in directory archives
	if (auto archive = archive_.lock())
		if (archive->format() == ArchiveFormat::Dir)
			return false;

	// Can't move if no entries selected (ie. only dirs)
	if (entry_tree_->selectedEntries().empty())
		return false;

	return true;
}

// -----------------------------------------------------------------------------
// Update the UI when the selection on the entry list is changed
// -----------------------------------------------------------------------------
void ArchivePanel::selectionChanged()
{
	// Get selected entries
	auto selection = entry_tree_->selectedEntries();

	if (selection.empty())
	{
		toolbar_elist_->group("_Entry")->setAllButtonsEnabled(false);
		toolbar_elist_->enableGroup("_Moving", false);
	}
	else if (selection.size() == 1)
	{
		// If one entry is selected, open it in the entry area
		toolbar_elist_->group("_Entry")->setAllButtonsEnabled(true);
		toolbar_elist_->findActionButton("arch_entry_rename_each")->Enable(false);
		toolbar_elist_->findActionButton("arch_entry_bookmark")->Enable(true);
		toolbar_elist_->enableGroup("_Moving", canMoveEntries());
		openEntry(selection[0]);
	}
	else
	{
		// If multiple entries are selected, show/update the multi entry area
		toolbar_elist_->group("_Entry")->setAllButtonsEnabled(true);
		toolbar_elist_->findActionButton("arch_entry_rename_each")->Enable(true);
		toolbar_elist_->findActionButton("arch_entry_bookmark")->Enable(false);
		toolbar_elist_->enableGroup("_Moving", canMoveEntries());
		showEntryPanel(default_area_);
		dynamic_cast<DefaultEntryPanel*>(default_area_)->loadEntries(selection);
	}

	// Get selected directories
	auto sel_dirs = entry_tree_->selectedDirectories();

	if (selection.empty() && !sel_dirs.empty())
	{
		toolbar_elist_->findActionButton("arch_entry_rename")->Enable(true);
		toolbar_elist_->findActionButton("arch_entry_delete")->Enable(true);
		toolbar_elist_->findActionButton("arch_entry_export")->Enable(true);

		if (sel_dirs.size() == 1)
			toolbar_elist_->findActionButton("arch_entry_bookmark")->Enable(true);
	}

	toolbar_elist_->Refresh();
}

// -----------------------------------------------------------------------------
// Updates the filtering on the entry tree
// -----------------------------------------------------------------------------
void ArchivePanel::updateFilter() const
{
	if (elist_show_filter)
	{
		// Get category string to filter by
		wxString category = "";
		if (choice_category_->GetSelection() > 0)
			category = choice_category_->GetStringSelection();

		// Filter the entry list
		entry_tree_->setFilter(wxutil::strToView(text_filter_->GetValue()), wxutil::strToView(category));
	}
	else
		entry_tree_->setFilter({}, {});
}

// -----------------------------------------------------------------------------
// Returns the TextEntryPanel for this ArchivePanel, creating it if needed
// -----------------------------------------------------------------------------
EntryPanel* ArchivePanel::textArea()
{
	if (text_area_ == nullptr)
		text_area_ = new TextEntryPanel(splitter_);

	return text_area_;
}

// -----------------------------------------------------------------------------
// Returns the ANSIEntryPanel for this ArchivePanel, creating it if needed
// -----------------------------------------------------------------------------
EntryPanel* ArchivePanel::ansiArea()
{
	if (ansi_area_ == nullptr)
		ansi_area_ = new ANSIEntryPanel(splitter_);

	return ansi_area_;
}

// -----------------------------------------------------------------------------
// Returns the GfxEntryPanel for this ArchivePanel, creating it if needed
// -----------------------------------------------------------------------------
EntryPanel* ArchivePanel::gfxArea()
{
	if (gfx_area_ == nullptr)
		gfx_area_ = new GfxEntryPanel(splitter_);

	return gfx_area_;
}

// -----------------------------------------------------------------------------
// Returns the PaletteEntryPanel for this ArchivePanel, creating it if needed
// -----------------------------------------------------------------------------
EntryPanel* ArchivePanel::palArea()
{
	if (pal_area_ == nullptr)
		pal_area_ = new PaletteEntryPanel(splitter_);

	return pal_area_;
}

// -----------------------------------------------------------------------------
// Returns the HexEntryPanel for this ArchivePanel, creating it if needed
// -----------------------------------------------------------------------------
EntryPanel* ArchivePanel::hexArea()
{
	if (hex_area_ == nullptr)
		hex_area_ = new HexEntryPanel(splitter_);

	return hex_area_;
}

// -----------------------------------------------------------------------------
// Returns the MapEntryPanel for this ArchivePanel, creating it if needed
// -----------------------------------------------------------------------------
EntryPanel* ArchivePanel::mapArea()
{
	if (map_area_ == nullptr)
		map_area_ = new MapEntryPanel(splitter_);

	return map_area_;
}

// -----------------------------------------------------------------------------
// Returns the AudioEntryPanel for this ArchivePanel, creating it if needed
// -----------------------------------------------------------------------------
EntryPanel* ArchivePanel::audioArea()
{
	if (audio_area_ == nullptr)
		audio_area_ = new AudioEntryPanel(splitter_);

	return audio_area_;
}

// -----------------------------------------------------------------------------
// Returns the DataEntryPanel for this ArchivePanel, creating it if needed
// -----------------------------------------------------------------------------
EntryPanel* ArchivePanel::dataArea()
{
	if (data_area_ == nullptr)
		data_area_ = new DataEntryPanel(splitter_);

	return data_area_;
}


// -----------------------------------------------------------------------------
//
// ArchivePanel Class Events
//
// -----------------------------------------------------------------------------

// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppParameterMayBeConstPtrOrRef

// -----------------------------------------------------------------------------
// Called when the selection on the entry list is changed
// -----------------------------------------------------------------------------
void ArchivePanel::onEntryListSelectionChange(wxDataViewEvent& e)
{
	// Do nothing if not shown or frozen (refreshing)
	if (!IsShown() || entry_tree_->IsFrozen())
		return;

	selectionChanged();

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the entry list is right clicked
// -----------------------------------------------------------------------------
void ArchivePanel::onEntryListRightClick(wxDataViewEvent& e)
{
	auto archive  = archive_.lock();
	bool has_dirs = archive->formatInfo().supports_dirs;

	// Get selected entries
	auto selection = entry_tree_->selectedEntries();
	auto sel_dirs  = entry_tree_->selectedDirectories();

	// Check what types exist in the selection
	// TODO: This stuff is absolutely terrible, nicer system needed
	bool gfx_selected        = false;
	bool png_selected        = false;
	bool bas_selected        = false;
	bool wav_selected        = false;
	bool dsnd_selected       = false;
	bool mus_selected        = false;
	bool text_selected       = false;
	bool unknown_selected    = false;
	bool texturex_selected   = false;
	bool zdtextures_selected = false;
	bool modified_selected   = false;
	bool map_selected        = false;
	bool swan_selected       = false;
	bool voxel_selected      = false;
	//	bool rle_selected = false;
	wxString category = "";
	for (auto& entry : selection)
	{
		// Check for gfx entry
		if (!gfx_selected)
		{
			if (entry->type()->extraProps().contains("image"))
				gfx_selected = true;
		}
		if (!voxel_selected)
		{
			if (entry->type()->formatId() == "voxel_vox")
				voxel_selected = true;
		}
		if (!png_selected)
		{
			if (entry->type()->formatId() == "img_png")
				png_selected = true;
		}
		if (!bas_selected)
		{
			if (entry->type()->formatId() == "animated" || entry->type()->formatId() == "switches")
				bas_selected = true;
		}
		if (!swan_selected)
		{
			if (entry->type()->id() == "swantbls")
				swan_selected = true;
		}
		if (!wav_selected)
		{
			if (entry->type()->formatId() == "snd_wav")
				wav_selected = true;
		}
		if (!dsnd_selected)
		{
			if (entry->type()->formatId() == "snd_doom" || entry->type()->formatId() == "snd_speaker"
				|| entry->type()->formatId() == "snd_wolf" || entry->type()->formatId() == "snd_doom_mac"
				|| entry->type()->formatId() == "snd_jaguar" || entry->type()->formatId() == "snd_bloodsfx"
				|| entry->type()->formatId() == "snd_voc")
				dsnd_selected = true;
		}
		if (!mus_selected)
		{
			if (strutil::startsWith(entry->type()->formatId(), "midi_") && entry->type()->formatId() != "midi_smf")
				mus_selected = true;
		}
		if (!text_selected)
		{
			if (entry->type()->formatId() == "text")
				text_selected = true;
		}
		if (!unknown_selected)
		{
			if (entry->type() == EntryType::unknownType())
				unknown_selected = true;
		}
		if (!texturex_selected)
		{
			if (entry->type()->formatId() == "texturex")
				texturex_selected = true;
		}
		if (!zdtextures_selected)
		{
			if (entry->type()->id() == "zdtextures")
				zdtextures_selected = true;
		}
		if (!modified_selected)
		{
			if (entry->state() == EntryState::Modified)
				modified_selected = true;
		}
		if (!map_selected)
		{
			if (entry->type() == EntryType::mapMarkerType())
				map_selected = true;
			else if (entry->parentDir()->name() == "maps")
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
				category = entry->type()->category();
			else
			{
				wxString ed = entry->type()->category();
				if (category != ed)
					category = "diff";
			}
		}
	}

	// Generate context menu
	wxMenu context;
	if (!selection.empty())
	{
		SAction::fromId("arch_entry_rename")->addToMenu(&context, true);
		if (selection.size() > 1)
			SAction::fromId("arch_entry_rename_each")->addToMenu(&context, true);
		SAction::fromId("arch_entry_delete")->addToMenu(&context, true);
		if (modified_selected)
			SAction::fromId("arch_entry_revert")->addToMenu(&context, true);
		context.AppendSeparator();
		SAction::fromId("arch_entry_cut")->addToMenu(&context, true);
		SAction::fromId("arch_entry_copy")->addToMenu(&context, true);
		SAction::fromId("arch_entry_paste")->addToMenu(&context, true);
		context.AppendSeparator();
		SAction::fromId("arch_entry_import")->addToMenu(&context, true);
		SAction::fromId("arch_entry_export")->addToMenu(&context, true);
		context.AppendSeparator();
		if (canMoveEntries())
		{
			SAction::fromId("arch_entry_moveup")->addToMenu(&context, true);
			SAction::fromId("arch_entry_movedown")->addToMenu(&context, true);
			SAction::fromId("arch_entry_sort")->addToMenu(&context, true);
			context.AppendSeparator();
		}
		if (selection.size() == 1)
			SAction::fromId("arch_entry_bookmark")->addToMenu(&context, true);
	}
	else if (!sel_dirs.empty())
	{
		SAction::fromId("arch_entry_rename")->addToMenu(&context, true);
		if (sel_dirs.size() > 1)
			SAction::fromId("arch_entry_rename_each")->addToMenu(&context, true);
		SAction::fromId("arch_entry_delete")->addToMenu(&context, true);
		context.AppendSeparator();
		SAction::fromId("arch_entry_cut")->addToMenu(&context, true);
		SAction::fromId("arch_entry_copy")->addToMenu(&context, true);
		SAction::fromId("arch_entry_paste")->addToMenu(&context, true);
		context.AppendSeparator();
		SAction::fromId("arch_entry_export")->addToMenu(&context, true);
		if (sel_dirs.size() == 1)
		{
			context.AppendSeparator();
			SAction::fromId("arch_entry_bookmark")->addToMenu(&context, true);
		}
	}
	else
	{
		// No entry selected
		SAction::fromId("arch_entry_paste")->addToMenu(&context, true);
		context.AppendSeparator();
		SAction::fromId("arch_importfiles")->addToMenu(&context, true);
		SAction::fromId("arch_importdir")->addToMenu(&context, true);
	}

	// Add 'Open In' menu
	if (!selection.empty())
	{
		auto open_item = new wxMenuItem(&context, wxID_ANY, "Open");
		open_item->SetSubMenu(createEntryOpenMenu(category));
		open_item->SetBitmap(icons::getIcon(icons::General, "open"));
		context.Append(open_item);
	}

	// Add custom menu items
	wxMenu* custom;
	if (context_submenus)
	{
		custom = new wxMenu();
	}
	else
		custom = &context;
	bool ok = cur_area_->fillCustomMenu(custom);
	if (context_submenus)
	{
		if (ok)
			context.AppendSubMenu(custom, cur_area_->customMenuName());
		else
			delete custom;
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

	// Add texturex/zdtextures related menu items if needed
	if (texturex_selected || zdtextures_selected)
	{
		SAction::fromId("arch_texture_clean_iwaddupes")->addToMenu(&context, true);
	}

	// Add zdtextures related menu items if needed
	if (zdtextures_selected)
	{
		SAction::fromId("arch_zdtextures_clean_singlepatch")->addToMenu(&context, true);
	}

	// 'View As' menu
	if (!selection.empty())
	{
		if (context_submenus)
		{
			auto viewas = new wxMenu();
			context.AppendSubMenu(viewas, "View As");
			SAction::fromId("arch_view_text")->addToMenu(viewas, 2, "Text");
			SAction::fromId("arch_view_hex")->addToMenu(viewas, 2, "Hex");
		}
		else
		{
			context.AppendSeparator();
			SAction::fromId("arch_view_text")->addToMenu(&context, true);
			SAction::fromId("arch_view_hex")->addToMenu(&context, true);
		}
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
		SAction::fromId("arch_scripts_compiledecohack")->addToMenu(scripts, true);
	}

	if (voxel_selected)
	{
		wxMenu* voxels;
		if (context_submenus)
		{
			voxels = new wxMenu();
			context.AppendSubMenu(voxels, "Voxels");
		}
		else
		{
			context.AppendSeparator();
			voxels = &context;
		}
		SAction::fromId("arch_voxel_convertvox")->addToMenu(voxels, true);
	}

	// Add map related menu items if needed
	if (map_selected)
	{
		// 'Open in Doom Builder 2' (windows-only)
#ifdef __WXMSW__
		SAction::fromId("arch_map_opendb2")->addToMenu(&context, true);
#endif
	}

#ifndef NO_LUA
	// Entry scripts
	if (!scriptmanager::editorScripts(scriptmanager::ScriptType::Entry).empty())
	{
		auto menu_scripts = new wxMenu();
		scriptmanager::populateEditorScriptMenu(menu_scripts, scriptmanager::ScriptType::Entry, "arch_entry_script");
		context.AppendSeparator();
		context.AppendSubMenu(menu_scripts, "Run &Script");
	}
#endif

	// New entry/dir options
	// Would be nice to eventually have each different type of entry that can be
	// created in a submenu to skip the new entry/dir dialog but this will do
	// for now
	context.AppendSeparator();
	if (has_dirs)
	{
		// 'New' menu for archives supporting directories
		auto menu_new = new wxMenu();
		SAction::fromId("arch_newentry")->addToMenu(menu_new, true, "Entry");
		SAction::fromId("arch_newdir")->addToMenu(menu_new, true, "Directory");
		context.AppendSubMenu(menu_new, "New");
	}
	else
		SAction::fromId("arch_newentry")->addToMenu(&context, true);

	// Popup the context menu
	PopupMenu(&context);
}

// -----------------------------------------------------------------------------
// Called when a key is pressed on the entry list
// -----------------------------------------------------------------------------
void ArchivePanel::onEntryListKeyDown(wxKeyEvent& e)
{
	// Check if keypress matches any keybinds
	auto binds = KeyBind::bindsForKey(KeyBind::asKeyPress(e.GetKeyCode(), e.GetModifiers()));

	// Go through matching binds
	for (const auto& bind : binds)
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
			entry_tree_->SelectAll();

			// Trigger selection change event (since SelectAll doesn't trigger it)
			wxDataViewEvent de;
			de.SetEventType(wxEVT_DATAVIEW_SELECTION_CHANGED);
			entry_tree_->ProcessWindowEvent(de);

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

		// Import Directory
		else if (bind == "el_import_dir")
		{
			importDir();
			return;
		}

		// Export Entry
		else if (bind == "el_export")
		{
			exportEntry();
			return;
		}

		// Toggle bookmark
		else if (bind == "el_bookmark")
		{
			bookmark();
			return;
		}

		else if (bind == "el_up_dir")
		{
			entry_tree_->upDir();
			return;
		}
	}

	// Not handled, send to parent
	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when an item on the entry list is 'activated'
// (via double-click or enter)
// -----------------------------------------------------------------------------
void ArchivePanel::onEntryListActivated(wxDataViewEvent& e)
{
	// Check the archive is still open
	auto archive = archive_.lock();
	if (!archive)
		return;

	auto entry = entry_tree_->entryForItem(e.GetItem());

	if (!entry)
		return;

	bool is_map = false;

	// Archive
	if (entry->isArchive())
	{
		if (archive->mapDesc(entry).head.lock().get() == entry)
			is_map = true;
		else
			app::archiveManager().openArchive(entry);
	}

	// Texture list
	else if (
		entry->type()->formatId() == "texturex" || entry->type() == EntryType::fromId("pnames")
		|| entry->type() == EntryType::fromId("zdtextures"))
		maineditor::openTextureEditor(archive.get(), entry);

	// Map
	else if (entry->type() == EntryType::mapMarkerType())
		is_map = true;

	// Other entry
	else if (entry->type() != EntryType::folderType())
		maineditor::openEntry(entry);

	// Launch map editor if map
	if (is_map)
	{
		// TODO: Needs to filter the game/port lists in the dialog by the map format

		// Open map editor config dialog
		MapEditorConfigDialog dlg(this, archive.get(), false);
		if (dlg.ShowModal() == wxID_OK)
		{
			// Get map info
			auto info = archive->mapDesc(entry);
			if (entry->isArchive() && info.format == MapFormat::Unknown)
			{
				auto tmp_archive = std::make_unique<Archive>(ArchiveFormat::Wad);
				tmp_archive->open(entry);
				auto maps = tmp_archive->detectMaps();
				if (!maps.empty())
					info.format = maps[0].format;
			}

			// Check selected game configuration is ok
			if (!dlg.configMatchesMap(info))
				wxMessageBox("Selected Game Configuration does not match the map format", "Error", wxICON_ERROR);
			else
			{
				// Load game configuration
				game::configuration().openConfig(
					dlg.selectedGame().ToStdString(), dlg.selectedPort().ToStdString(), info.format);

				// Attempt to open map
				if (mapeditor::window()->openMap(info))
					mapeditor::window()->Show();
				else
				{
					mapeditor::window()->Hide();
					wxMessageBox(
						wxString::Format("Unable to open map %s: %s", entry->name(), global::error),
						"Invalid map error",
						wxICON_ERROR);
				}
			}
		}
	}

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the 'Edit Level Script' button is clicked on the map entry panel
// (opens the entry in the text editor panel)
// -----------------------------------------------------------------------------
void ArchivePanel::onMEPEditAsText(wxCommandEvent& e)
{
	// Get entry to edit
	auto entry = mapArea()->entry();

	// Open in text editor
	openEntryAsText(entry);
}

// -----------------------------------------------------------------------------
// Called when the 'Edit as Text' button is clicked on the default entry panel
// (opens the entry in the text editor panel)
// -----------------------------------------------------------------------------
void ArchivePanel::onDEPEditAsText(wxCommandEvent& e)
{
	// Get entry to edit
	auto entry = default_area_->entry();

	// Open in text editor
	openEntryAsText(entry);
}

// -----------------------------------------------------------------------------
// Called when the 'View as Hex' button is clicked on the default entry panel
// (opens the entry in the hex editor panel)
// -----------------------------------------------------------------------------
void ArchivePanel::onDEPViewAsHex(wxCommandEvent& e)
{
	// Get entry to view
	auto entry = default_area_->entry();

	// Open in hex editor
	openEntryAsHex(entry);
}

// -----------------------------------------------------------------------------
// Called when the filter text is changed
// -----------------------------------------------------------------------------
void ArchivePanel::onTextFilterChanged(wxCommandEvent& e)
{
	updateFilter();
	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the 'Category' choice is changed
// -----------------------------------------------------------------------------
void ArchivePanel::onChoiceCategoryChanged(wxCommandEvent& e)
{
	updateFilter();
	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the 'Clear Filter' button is clicked
// -----------------------------------------------------------------------------
void ArchivePanel::onBtnClearFilter(wxCommandEvent& e)
{
	text_filter_->SetValue("");
}
