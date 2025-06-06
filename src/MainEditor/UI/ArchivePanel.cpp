
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
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
#include "Archive/ArchiveManager.h"
#include "Archive/Formats/ZipArchive.h"
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
#include "General/Misc.h"
#include "General/UI.h"
#include "Graphics/Palette/PaletteManager.h"
#include "MainEditor/ArchiveOperations.h"
#include "MainEditor/Conversions.h"
#include "MainEditor/EntryOperations.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/MainWindow.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/UI/MapEditorWindow.h"
#include "Scripting/ScriptManager.h"
#include "UI/Controls/PaletteChooser.h"
#include "UI/Controls/SIconButton.h"
#include "UI/Controls/Splitter.h"
#include "UI/Dialogs/GfxColouriseDialog.h"
#include "UI/Dialogs/GfxConvDialog.h"
#include "UI/Dialogs/GfxTintDialog.h"
#include "UI/Dialogs/MapEditorConfigDialog.h"
#include "UI/Dialogs/MapReplaceDialog.h"
#include "UI/Dialogs/ModifyOffsetsDialog.h"
#include "UI/Dialogs/NewEntryDialog.h"
#include "UI/Dialogs/Preferences/PreferencesDialog.h"
#include "UI/Dialogs/RunDialog.h"
#include "UI/Dialogs/TranslationEditorDialog.h"
#include "UI/Lists/ArchiveEntryTree.h"
#include "UI/WxUtils.h"
#include "Utility/FileUtils.h"
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

const auto ERROR_UNWRITABLE_IMAGE_FORMAT = "Could not write image data to entry {}, unsupported format for writing";
} // namespace
CVAR(Int, autosave_entry_changes, 2, CVar::Flag::Save) // 0=no, 1=yes, 2=ask
CVAR(Bool, confirm_entry_delete, true, CVar::Flag::Save)
CVAR(Bool, context_submenus, true, CVar::Flag::Save)
CVAR(String, last_colour, "RGB(255, 0, 0)", CVar::Flag::Save)
CVAR(String, last_tint_colour, "RGB(255, 0, 0)", CVar::Flag::Save)
CVAR(Int, last_tint_amount, 50, CVar::Flag::Save)
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
EXTERN_CVAR(String, path_pngout)
EXTERN_CVAR(String, path_pngcrush)
EXTERN_CVAR(String, path_deflopt)
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
		wxString caption    = (filenames.size() > 1) ? wxS("Overwrite entries") : wxS("Overwrite entry");

		// Import all dragged files, inserting after the item they were dragged onto
		list_->Freeze();
		for (int a = filenames.size() - 1; a >= 0; a--)
		{
			// Is this a directory?
			if (wxDirExists(filenames[a]))
			{
				// If the archive supports directories, create the directory and import its contents
				if (archive->formatDesc().supports_dirs)
				{
					strutil::Path fn(filenames[a].utf8_string());
					auto          ndir = archive->createDir(fn.fileName(false), ArchiveDir::getShared(dir));
					archive->importDir(fn.fullPath(), true, ndir);
				}

				// TODO: Do we want to support flat list archives here? If so might want special handling for
				// namespaces etc.
			}
			else
			{
				strutil::Path fn(filenames[a].utf8_string());
				ArchiveEntry* entry = nullptr;

				// Find entry to replace if needed
				if (auto_entry_replace)
				{
					entry = archive->entryAtPath(dir->path().append(fn.fileName()));
					// An entry with that name is already present, so ask about replacing it
					if (entry && !yes_to_all)
					{
						// Since there is no standard "Yes/No to a" button or "Don't ask me again" checkbox,
						// we will instead hack the Cancel button into being a "Yes to a" button. This is
						// despite the existence of a wxID_YESTOALL return value...
						auto message = fmt::format("Overwrite existing entry {}{}", dir->path(), fn.fileName());
						wxMessageDialog dlg(
							parent_, wxString::FromUTF8(message), caption, wxCANCEL | wxYES_NO | wxCENTRE);
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
				entry->importFile(filenames[a].utf8_string());
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
				  wxS("Choose Base Palette"),
				  wxDefaultPosition,
				  wxDefaultSize,
				  wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER }
	{
		// Set dialog icon
		wxutil::setWindowIcon(this, "palette");

		// Setup main sizer
		auto sizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(sizer);

		// Add choose
		pal_chooser_ = new PaletteChooser(this, -1);
		sizer->Add(pal_chooser_, 0, wxEXPAND | wxALL, 4);

		sizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4);

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
				file->AddFile(wxString::FromUTF8(filename));
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
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Creates a vector of namespaces in a predefined order
// -----------------------------------------------------------------------------
void initNamespaceVector(vector<string>& ns, bool flathack)
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

// -----------------------------------------------------------------------------
// Checks through a MapDesc vector and returns which one, if any, the entry
// index is in, -1 otherwise
// -----------------------------------------------------------------------------
int isInMap(size_t index, const vector<Archive::MapDesc>& maps)
{
	for (size_t m = 0; m < maps.size(); ++m)
	{
		// Get map header and ending entries
		auto m_head = maps[m].head.lock();
		auto m_end  = maps[m].end.lock();
		if (!m_head || !m_end)
			continue;

		// Check indices
		size_t head_index = m_head->index();
		size_t end_index  = m_head->parentDir()->entryIndex(m_end.get(), head_index);
		if (index >= head_index && index <= end_index)
			return m;
	}
	return -1;
}

// -----------------------------------------------------------------------------
// Returns the position of the given entry's detected namespace in the
// namespace vector. Also hacks around a bit to put less entries in the global
// namespace and allow sorting a bit by categories.
// -----------------------------------------------------------------------------
size_t getNamespaceNumber(
	const ArchiveEntry*             entry,
	size_t                          index,
	vector<string>&                 ns,
	const vector<Archive::MapDesc>& maps)
{
	auto ens = entry->parent()->detectNamespace(index);
	if (strutil::equalCI(ens, "global"))
	{
		if (!maps.empty() && isInMap(index, maps) >= 0)
			ens = "maps";
		else if (strutil::equalCI(entry->type()->category(), "Graphics"))
			ens = "graphics";
		else if (strutil::equalCI(entry->type()->category(), "Audio"))
		{
			if (strutil::equalCI(entry->type()->icon(), "music"))
				ens = "music";
			else
				ens = "sounds";
		}
	}
	for (size_t n = 0; n < ns.size(); ++n)
		if (strutil::equalCI(ns[n], ens))
			return n;

	ns.emplace_back(ens);
	return ns.size();
}

} // namespace


// -----------------------------------------------------------------------------
//
// ArchivePanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ArchivePanel class constructor
// -----------------------------------------------------------------------------
ArchivePanel::ArchivePanel(wxWindow* parent, shared_ptr<Archive>& archive) :
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
void ArchivePanel::setup(Archive* archive)
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
	splitter_->SetMinimumPaneSize(ui::scalePx(300));
	m_hbox->Add(splitter_, wxSizerFlags(1).Expand().Border(wxALL, ui::pad()));
	int split_pos = ap_splitter_position_list;
	if (archive && archive->formatDesc().supports_dirs)
		split_pos = ap_splitter_position_tree;
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
				if (archive->formatDesc().supports_dirs)
					ap_splitter_position_tree = e.GetSashPosition();
				else
					ap_splitter_position_list = e.GetSashPosition();
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
			parent->SetPageText(parent->GetPageIndex(this), wxString::FromUTF8(a.filename(false)));
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
	bool  has_dirs = archive->formatDesc().supports_dirs;
	auto  min_pad  = ui::px(ui::Size::PadMinimum);

	// Create & set sizer & border
	auto* hbox = new wxBoxSizer(wxHORIZONTAL);
	panel->SetSizer(hbox);

	// Create entry list
	entry_tree_ = new ui::ArchiveEntryTree(panel, archive, undo_manager_.get(), elist_no_tree);
	entry_tree_->SetInitialSize({ 400, -1 });
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
	if (archive->formatId() != "folder")
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
	auto* gbsizer = new wxGridBagSizer(ui::pad(), ui::pad());
	panel_filter_->SetSizer(gbsizer);

	// Create category selector
	choice_category_ = new wxChoice(panel_filter_, -1);
	choice_category_->Append(wxS("All"));
	for (const auto& cat : EntryType::allCategories())
		choice_category_->Append(wxString::FromUTF8(cat));
	choice_category_->SetSelection(0);
	gbsizer->Add(
		new wxStaticText(panel_filter_, -1, wxS("Show:")), wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gbsizer->Add(choice_category_, wxGBPosition(0, 1), wxGBSpan(1, 2), wxEXPAND);
	gbsizer->AddGrowableCol(1, 1);

	// Create filter
	text_filter_ = new wxTextCtrl(panel_filter_, -1);
	gbsizer->Add(
		new wxStaticText(panel_filter_, -1, wxS("Filter:")),
		wxGBPosition(1, 0),
		wxDefaultSpan,
		wxALIGN_CENTER_VERTICAL);
	gbsizer->Add(text_filter_, wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND);
	btn_clear_filter_ = new SIconButton(panel_filter_, "close");
	btn_clear_filter_->SetToolTip(wxS("Clear Filter"));
	gbsizer->Add(btn_clear_filter_, wxGBPosition(1, 2), wxDefaultSpan, wxEXPAND);

	// Show/hide filter controls depending on cvar
	panel_filter_->Show(elist_show_filter);

	// Layout entry list
	hbox->Add(toolbar_elist_, 0, wxEXPAND);
	hbox->AddSpacer(min_pad);
	auto* vbox = new wxBoxSizer(wxVERTICAL);
	hbox->Add(vbox, 1, wxEXPAND | wxRIGHT, min_pad);
	if (etree_path_)
	{
		vbox->Add(etree_path_, 0, wxEXPAND);
		vbox->AddSpacer(min_pad);
		vbox->Add(entry_tree_, 1, wxEXPAND);
	}
	else
		vbox->Add(entry_tree_, 1, wxEXPAND);
	vbox->Add(panel_filter_, 0, wxEXPAND | wxTOP, ui::pad());

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
			WX_FMT("Save changes to entry \"{}\"?", cur_area_->entry()->name()),
			wxS("Unsaved Changes"),
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

		auto maintenance_item = new wxMenuItem(menu_archive, wxID_ANY, wxS("&Maintenance"));
		maintenance_item->SetSubMenu(menu_clean);
		maintenance_item->SetBitmap(icons::getIcon(icons::Type::Any, "wrench"));
		menu_archive->Append(maintenance_item);

		auto menu_scripts = new wxMenu();
#ifndef NO_LUA
		scriptmanager::populateEditorScriptMenu(menu_scripts, scriptmanager::ScriptType::Archive, "arch_script");
		menu_archive->AppendSubMenu(menu_scripts, wxS("&Run Script"));
#endif

		menu_archive->AppendSeparator();
		SAction::fromId("arch_run")->addToMenu(menu_archive, true, "Run");
		SAction::fromId("arch_quick_run")->addToMenu(menu_archive, true, "Quick Run");
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
		menu_entry->AppendSubMenu(menu_scripts, wxS("&Run Script"));
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
	if (archive->formatDesc().supports_dirs)
	{
		auto dir_path = dlg->parentDirPath();
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
	auto new_entry = std::make_shared<ArchiveEntry>(dlg->entryName());

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
	if (archive->formatDesc().supports_dirs)
	{
		auto dir_path = dlg->parentDirPath();
		strutil::replaceIP(dir_path, "\\", "/");

		parent_dir = archive->dirAtPath(dir_path);
		if (!parent_dir)
			parent_dir = archive->createDir(dir_path).get();
	}

	// Add the directory to the archive
	undo_manager_->beginRecord("Create Directory");
	auto dir = archive->createDir(dlg->entryName(), ArchiveDir::getShared(parent_dir));
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
			auto name = strutil::Path::fileNameOf(info.filenames[a]);

			// Update splash window
			ui::setSplashProgress(static_cast<float>(a) / static_cast<float>(info.filenames.size()));
			ui::setSplashProgressMessage(name);

			// Create new entry
			auto new_entry = std::make_shared<ArchiveEntry>(name);

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
	wxMessageBox(wxS("Not Implemented"));
	return false;
}

// -----------------------------------------------------------------------------
// Not implemented
// -----------------------------------------------------------------------------
bool ArchivePanel::cleanupArchive() const
{
	wxMessageBox(wxS("Not Implemented"));
	return false;
}

// -----------------------------------------------------------------------------
// Build pk3/zip archive from the current directory
// -----------------------------------------------------------------------------
bool ArchivePanel::buildArchive()
{
	// Check the archive is still open
	auto archive = archive_.lock();
	if (!archive)
		return false;

	if (archive->formatId() != "folder")
	{
		wxMessageBox(wxS("This function is only supported with directories"), wxS("Can't build archive"), wxICON_ERROR);
		return false;
	}

	// Create temporary archive
	ZipArchive zip;

	// Create dialog
	filedialog::FDInfo info;
	if (filedialog::saveFile(info, "Build Archive", zip.fileExtensionString(), this))
	{
		ui::showSplash("Building " + info.filenames[0], true, maineditor::windowWx());
		ui::setSplashProgress(0.0f);

		// prevent for "archive in archive" when saving in the current directory
		if (fileutil::fileExists(info.filenames[0]))
			fileutil::removeFile(info.filenames[0]);

		// Log
		ui::setSplashProgressMessage("Importing files...");
		ui::setSplashProgress(-1.0f);

		// Import all files into new archive
		zip.importDir(archive->filename(), archive_dir_ignore_hidden);
		ui::setSplashProgress(1.0f);
		ui::setSplashMessage("Saving archive...");
		ui::setSplashProgressMessage("");

		// Save the archive
		if (!zip.save(info.filenames[0]))
		{
			ui::hideSplash();

			// If there was an error pop up a message box
			wxMessageBox(WX_FMT("Error:\n{}", global::error), wxS("Error"), wxICON_ERROR);
			return false;
		}
	}

	ui::hideSplash();

	return true;
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
		string item;
		int    num = selected_entries.size() + selected_dirs.size();
		if (num == 1)
		{
			if (selected_entries.size() == 1)
				item = selected_entries[0]->name();
			else
				item = selected_dirs[0]->name();
		}
		else if (num > 0)
			item = fmt::format("these {} items", num);

		if (wxMessageBox(
				WX_FMT("Are you sure you want to delete {}?", item),
				wxS("Delete Confirmation"),
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
				wxS("Are you sure you want to revert changes made to the entry?"),
				wxS("Revert Changes"),
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
// Note that a simple sort is not desired for three reasons:
// 1. Map lumps have to remain in sequence
// 2. Namespaces should be respected
// 3. Marker lumps used as separators should also be respected
// The way we're doing that is more than a bit hacky, sorry.
// The basic idea is to assign to each entry a sortkey (thanks to ExProps for
// that) which is prefixed with namespace information. Also, the name of map
// lumps is replaced by the map name so that they stay together. Finally, the
// original index is appended so that duplicate names are disambiguated.
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

	// Get vector of selected entry indices
	vector<unsigned> selection;
	for (const auto& entry : sel_entries)
		selection.push_back(entry->index());

	size_t start, stop;

	// Without selection of multiple entries, sort everything instead
	if (selection.size() < 2)
	{
		start = 0;
		stop  = dir->numEntries();
	}
	// We need sorting to be contiguous, otherwise it'll destroy maps
	else
	{
		start = selection[0];
		stop  = selection[selection.size() - 1] + 1;
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
	auto maps = dir->archive()->detectMaps();

	auto   ns  = dir->archive()->detectNamespace(dir->entryAt(selection[0]));
	size_t nsn = 0, lnsn = 0;

	// Fill a map with <entry name, entry index> pairs
	std::map<string, size_t> emap;
	emap.clear();
	for (size_t i = 0; i < selection.size(); ++i)
	{
		auto entry = dir->entryAt(selection[i]);
		if (!entry)
			continue;

		// Ignore subdirectories
		if (entry->type() == EntryType::folderType())
			continue;

		// If not a WAD, do basic alphabetical sorting
		if (archive->formatId() != "wad" && archive->formatId() != "wadj")
		{
			auto sortkey             = fmt::format("{:<64}{:8d}", entry->upperName(), selection[i]);
			emap[sortkey]            = selection[i];
			entry->exProp("sortkey") = sortkey;

			continue;
		}

		bool   ns_changed = false;
		int    mapindex   = isInMap(selection[i], maps);
		string mapname;

		// If this is a map entry, deal with it
		if (!maps.empty() && mapindex > -1)
		{
			auto head = maps[mapindex].head.lock();
			if (!head)
				return false;

			// Keep track of the name
			mapname = maps[mapindex].name;

			// If part of a map is selected, make sure the rest is selected as well
			size_t head_index = head->index();
			size_t end_index  = head->parentDir()->entryIndex(maps[mapindex].end.lock().get(), head_index);
			// Good thing we can rely on selection being contiguous
			for (size_t a = head_index; a <= end_index; ++a)
			{
				bool selected = (a >= start && a < stop);
				if (!selected)
					selection.push_back(a);
			}
			if (head_index < start)
				start = head_index;
			if (end_index + 1 > stop)
				stop = end_index + 1;
		}
		else if (dir->archive()->detectNamespace(selection[i]) != ns)
		{
			ns         = dir->archive()->detectNamespace(selection[i]);
			nsn        = getNamespaceNumber(entry, selection[i], nspaces, maps) * 1000;
			ns_changed = true;
		}
		else if (mapindex < 0 && (entry->size() == 0))
		{
			nsn++;
			ns_changed = true;
		}

		// Local namespace number is not necessarily computed namespace number.
		// This is because the global namespace in wads is bloated and we want more
		// categories than it actually has to offer.
		lnsn = (nsn == 0 ? getNamespaceNumber(entry, selection[i], nspaces, maps) * 1000 : nsn);
		string name, ename = entry->upperName();
		// Want to get another hack in this stuff? Yeah, of course you do!
		// This here hack will sort Doom II songs by their associated map.
		if (strutil::startsWith(ename, "D_") && strutil::equalCI(entry->type()->icon(), "music"))
		{
			if (ename == "D_RUNNIN")
				ename = "D_MAP01";
			else if (ename == "D_STALKS")
				ename = "D_MAP02";
			else if (ename == "D_COUNTD")
				ename = "D_MAP03";
			else if (ename == "D_BETWEE")
				ename = "D_MAP04";
			else if (ename == "D_DOOM")
				ename = "D_MAP05";
			else if (ename == "D_THE_DA")
				ename = "D_MAP06";
			else if (ename == "D_SHAWN")
				ename = "D_MAP07";
			else if (ename == "D_DDTBLU")
				ename = "D_MAP08";
			else if (ename == "D_IN_CIT")
				ename = "D_MAP09";
			else if (ename == "D_DEAD")
				ename = "D_MAP10";
			else if (ename == "D_STLKS2")
				ename = "D_MAP11";
			else if (ename == "D_THEDA2")
				ename = "D_MAP12";
			else if (ename == "D_DOOM2")
				ename = "D_MAP13";
			else if (ename == "D_DDTBL2")
				ename = "D_MAP14";
			else if (ename == "D_RUNNI2")
				ename = "D_MAP15";
			else if (ename == "D_DEAD2")
				ename = "D_MAP16";
			else if (ename == "D_STLKS3")
				ename = "D_MAP17";
			else if (ename == "D_ROMERO")
				ename = "D_MAP18";
			else if (ename == "D_SHAWN2")
				ename = "D_MAP19";
			else if (ename == "D_MESSAG")
				ename = "D_MAP20";
			else if (ename == "D_COUNT2")
				ename = "D_MAP21";
			else if (ename == "D_DDTBL3")
				ename = "D_MAP22";
			else if (ename == "D_AMPIE")
				ename = "D_MAP23";
			else if (ename == "D_THEDA3")
				ename = "D_MAP24";
			else if (ename == "D_ADRIAN")
				ename = "D_MAP25";
			else if (ename == "D_MESSG2")
				ename = "D_MAP26";
			else if (ename == "D_ROMER2")
				ename = "D_MAP27";
			else if (ename == "D_TENSE")
				ename = "D_MAP28";
			else if (ename == "D_SHAWN3")
				ename = "D_MAP29";
			else if (ename == "D_OPENIN")
				ename = "D_MAP30";
			else if (ename == "D_EVIL")
				ename = "D_MAP31";
			else if (ename == "D_ULTIMA")
				ename = "D_MAP32";
			else if (ename == "D_READ_M")
				ename = "D_MAP33";
			else if (ename == "D_DM2TTL")
				ename = "D_MAP34";
			else if (ename == "D_DM2INT")
				ename = "D_MAP35";
		}
		// All map lumps have the same sortkey name so they stay grouped
		if (mapindex > -1)
		{
			name = fmt::format("{:08d}{:<64}{:8d}", lnsn, mapname, selection[i]);
		}
		// Yet another hack! Make sure namespace start markers are first
		else if (ns_changed)
		{
			name = fmt::format("{:08d}{:<64}{:8d}", lnsn, "", selection[i]);
		}
		// Generic case: actually use the entry name to sort
		else
		{
			name = fmt::format("{:08d}{:<64}{:8d}", lnsn, ename, selection[i]);
		}
		// Let the entry remember how it was sorted this time
		entry->exProp("sortkey") = name;
		// Insert sortkey into entry map so it'll be sorted
		emap[name] = selection[i];
	}

	// And now, sort the entries based on the map
	undo_manager_->beginRecord("Sort Entries");
	entry_tree_->Freeze();
	auto itr = emap.begin();
	for (size_t i = start; i < stop; ++i, ++itr)
	{
		if (itr == emap.end())
			break;

		auto entry = dir->entryAt(i);

		// Ignore subdirectories
		if (entry->type() == EntryType::folderType())
			continue;

		// If the entry isn't in its sorted place already
		if (i != (size_t)itr->second)
		{
			// Swap the entry in the spot with the sorted one
			archive->swapEntries(i, itr->second, dir);

			// Update the position of the displaced entry in the emap
			auto name  = entry->exProp<string>("sortkey");
			emap[name] = itr->second;
		}
	}
	undo_manager_->endRecord(true);

	// Refresh
	entry_tree_->Thaw();
	archive->setModified(true);

	return true;
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
	string checksums = "\nCRC-32:\n";
	for (auto& entry : selection)
	{
		uint32_t crc = entry->data().crc();
		checksums += fmt::format("{}:\t{:x}\n", entry->name(), crc);
	}
	log::info(1, checksums);
	wxMessageBox(wxString::FromUTF8(checksums));

	return true;
}

// -----------------------------------------------------------------------------
// Not implemented
// -----------------------------------------------------------------------------
bool ArchivePanel::convertEntryTo() const
{
	wxMessageBox(wxS("Not Implemented"));
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
					WX_FMT("Failed to import file: {}", global::error), wxS("Import Failed"), wxICON_ERROR, this);
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
						WX_FMT(
							"Image {} had offset [{}, {}], imported file has offset [{}, {}]. "
							"Do you want to keep the old offset and override the new?",
							entry->name(),
							offset.x,
							offset.y,
							noffset.x,
							noffset.y),
						wxS("Conflicting Offsets"),
						wxYES_NO);
					int result = md.ShowModal();
					if (result != wxID_YES)
						ok = false;
				}
				// Warn if the offsets couldn't be written
				if (ok && si.format() && !si.format()->writeOffset(si, entry, offset))
					log::warning(
						"Old offset information [{}, {}] couldn't be "
						"preserved in the new image format for image %s.",
						offset.x,
						offset.y,
						entry->name());
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
	if (entry_type_changed && archive_.lock()->formatDesc().names_extensions)
	{
		auto multi_select = selection.size() > 1;

		if (wxMessageBox(
				multi_select ? wxS("One or more entry types were changed. Update entry extensions?")
							 : wxS("The entry type has changed. Update it's extension?"),
				wxS("Update Entry Extension(s)"),
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
	wxMessageBox(wxS("Not Implemented"));
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
				WX_FMT("Failed opening {} in external editor: {}", entry->name(), global::error),
				wxS("External Edit Failed"),
				wxOK | wxICON_ERROR);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Opens the Gfx Conversion dialog and sends selected entries to it
// -----------------------------------------------------------------------------
bool ArchivePanel::gfxConvert() const
{
	// Create gfx conversion dialog
	GfxConvDialog gcd(theMainWindow);

	// Send selection to the gcd
	auto selection = entry_tree_->selectedEntries();
	gcd.openEntries(selection);

	// Run the gcd
	gcd.ShowModal();

	// Show splash window
	ui::showSplash("Writing converted image data...", true, maineditor::windowWx());

	// Begin recording undo level
	undo_manager_->beginRecord("Gfx Format Conversion");

	// Write any changes
	for (unsigned a = 0; a < selection.size(); a++)
	{
		// Update splash window
		ui::setSplashProgressMessage(selection[a]->name());
		ui::setSplashProgress((float)a / (float)selection.size());

		// Skip if the image wasn't converted
		if (!gcd.itemModified(a))
			continue;

		// Get image and conversion info
		auto image  = gcd.itemImage(a);
		auto format = gcd.itemFormat(a);

		// Write converted image back to entry
		MemChunk mc;
		format->saveImage(*image, mc, gcd.itemPalette(a));
		selection[a]->importMemChunk(mc);
		EntryType::detectEntryType(*selection[a]);
		selection[a]->setExtensionByType();
	}

	// Finish recording undo level
	undo_manager_->endRecord(true);

	// Hide splash window
	ui::hideSplash();
	maineditor::currentEntryPanel()->callRefresh();

	return true;
}

// -----------------------------------------------------------------------------
// Opens the Translation editor dialog to remap colours on selected gfx entries
// -----------------------------------------------------------------------------
bool ArchivePanel::gfxRemap()
{
	// Get selected entries
	auto selection = entry_tree_->selectedEntries();

	// Create preview image (just use first selected entry)
	SImage image(SImage::Type::PalMask);
	misc::loadImageFromEntry(&image, selection[0]);

	// Create translation editor dialog
	auto                    pal = theMainWindow->paletteChooser()->selectedPalette();
	TranslationEditorDialog ted(this, *pal, "Colour Remap", &image);
	ted.openTranslation(dynamic_cast<GfxEntryPanel*>(gfxArea())->prevTranslation());

	// Run dialog
	if (ted.ShowModal() == wxID_OK)
	{
		// Begin recording undo level
		undo_manager_->beginRecord("Gfx Colour Remap");

		// Apply translation to all entry images
		SImage   temp;
		MemChunk mc;

		for (auto entry : selection)
		{
			if (misc::loadImageFromEntry(&temp, entry))
			{
				// Apply translation
				temp.applyTranslation(&ted.getTranslation(), pal);

				// Create undo step
				undo_manager_->recordUndoStep(std::make_unique<EntryDataUS>(entry));

				// Write modified image data
				if (!temp.format()->saveImage(temp, mc, pal))
					log::error(1, ERROR_UNWRITABLE_IMAGE_FORMAT, entry->name());
				else
					entry->importMemChunk(mc);
			}
		}

		// Update variables
		dynamic_cast<GfxEntryPanel*>(gfxArea())->prevTranslation().copy(ted.getTranslation());

		// Finish recording undo level
		undo_manager_->endRecord(true);
	}
	maineditor::currentEntryPanel()->callRefresh();

	return true;
}

// -----------------------------------------------------------------------------
// Opens the Colourise dialog to batch-colour selected gfx entries
// -----------------------------------------------------------------------------
bool ArchivePanel::gfxColourise()
{
	// Get selected entries
	auto selection = entry_tree_->selectedEntries();

	// Create colourise dialog
	auto               pal = theMainWindow->paletteChooser()->selectedPalette();
	GfxColouriseDialog gcd(this, selection[0], *pal);
	gcd.setColour(last_colour);

	// Run dialog
	if (gcd.ShowModal() == wxID_OK)
	{
		// Begin recording undo level
		undo_manager_->beginRecord("Gfx Colourise");

		// Apply translation to all entry images
		SImage   temp;
		MemChunk mc;
		for (auto entry : selection)
		{
			if (misc::loadImageFromEntry(&temp, entry))
			{
				// Apply translation
				temp.colourise(gcd.colour(), pal);

				// Create undo step
				undo_manager_->recordUndoStep(std::make_unique<EntryDataUS>(entry));

				// Write modified image data
				if (!temp.format()->saveImage(temp, mc, pal))
					log::error(ERROR_UNWRITABLE_IMAGE_FORMAT, entry->name());
				else
					entry->importMemChunk(mc);
			}
		}
		// Finish recording undo level
		undo_manager_->endRecord(true);
	}
	last_colour = gcd.colour().toString(ColRGBA::StringFormat::RGB);
	maineditor::currentEntryPanel()->callRefresh();

	return true;
}

// -----------------------------------------------------------------------------
// Opens the Tint dialog to batch-colour selected gfx entries
// -----------------------------------------------------------------------------
bool ArchivePanel::gfxTint()
{
	// Get selected entries
	auto selection = entry_tree_->selectedEntries();

	// Create colourise dialog
	auto          pal = theMainWindow->paletteChooser()->selectedPalette();
	GfxTintDialog gtd(this, selection[0], *pal);
	gtd.setValues(last_tint_colour, last_tint_amount);

	// Run dialog
	if (gtd.ShowModal() == wxID_OK)
	{
		// Begin recording undo level
		undo_manager_->beginRecord("Gfx Tint");

		// Apply translation to all entry images
		SImage   temp;
		MemChunk mc;
		for (auto entry : selection)
		{
			if (misc::loadImageFromEntry(&temp, entry))
			{
				// Apply translation
				temp.tint(gtd.colour(), gtd.amount(), pal);

				// Create undo step
				undo_manager_->recordUndoStep(std::make_unique<EntryDataUS>(entry));

				// Write modified image data
				if (!temp.format()->saveImage(temp, mc, pal))
					log::info(ERROR_UNWRITABLE_IMAGE_FORMAT, entry->name());
				else
					entry->importMemChunk(mc);
			}
		}

		// Finish recording undo level
		undo_manager_->endRecord(true);
	}
	last_tint_colour = gtd.colour().toString(ColRGBA::StringFormat::RGB);
	last_tint_amount = (int)(gtd.amount() * 100.0f);
	maineditor::currentEntryPanel()->callRefresh();

	return true;
}

// -----------------------------------------------------------------------------
// Opens the Modify Offsets dialog to mass-modify offsets of any selected,
// offset-compatible gfx entries
// -----------------------------------------------------------------------------
bool ArchivePanel::gfxModifyOffsets() const
{
	// Create modify offsets dialog
	ModifyOffsetsDialog mod;

	// Run the dialog
	if (mod.ShowModal() == wxID_CANCEL)
		return false;

	// Begin recording undo level
	undo_manager_->beginRecord("Gfx Modify Offsets");

	// Go through selected entries
	auto selection = entry_tree_->selectedEntries();
	for (auto& entry : selection)
	{
		undo_manager_->recordUndoStep(std::make_unique<EntryDataUS>(entry));
		mod.apply(*entry);
	}
	maineditor::currentEntryPanel()->callRefresh();

	// Finish recording undo level
	undo_manager_->endRecord(true);

	return true;
}

// -----------------------------------------------------------------------------
// Exports any selected gfx entries as png format images
// -----------------------------------------------------------------------------
bool ArchivePanel::gfxExportPNG()
{
	// Get a list of selected entries
	auto selection = entry_tree_->selectedEntries();

	// If we're just exporting 1 entry
	if (selection.size() == 1)
	{
		auto          name = misc::lumpNameToFileName(selection[0]->name());
		strutil::Path fn(name);

		// Set extension
		fn.setExtension("png");

		// Run save file dialog
		filedialog::FDInfo info;
		if (filedialog::saveFile(
				info,
				"Export Entry \"" + selection[0]->name() + "\" as PNG",
				"PNG Files (*.png)|*.png",
				this,
				fn.fileName()))
		{
			// If a filename was selected, export it
			if (!entryoperations::exportAsPNG(selection[0], info.filenames[0]))
			{
				wxMessageBox(WX_FMT("Error: {}", global::error), wxS("Error"), wxOK | wxICON_ERROR);
				return false;
			}
		}

		return true;
	}
	else
	{
		// Run save files dialog
		filedialog::FDInfo info;
		if (filedialog::saveFiles(
				info, "Export Entries as PNG (Filename will be ignored)", "PNG Files (*.png)|*.png", this))
		{
			// Go through the selection
			for (auto& entry : selection)
			{
				// Setup entry filename
				strutil::Path fn(entry->name());
				fn.setPath(info.path);
				fn.setExtension("png");

				// Do export
				entryoperations::exportAsPNG(entry, fn.fullPath());
			}
		}
	}

	return true;
}

bool ArchivePanel::voxelConvert() const
{
	// Get selected entries
	auto selection = entry_tree_->selectedEntries();

	// Begin recording undo level
	undo_manager_->beginRecord("Convert .vox -> .kvx");

	// Go through selection
	bool errors = false;
	for (auto* entry : selection)
	{
		if (entry->type()->formatId() == "voxel_vox")
		{
			MemChunk kvx;
			// Attempt conversion
			if (!conversion::voxToKvx(entry->data(), kvx))
			{
				log::error("Unable to convert entry {}: {}", entry->name(), global::error);
				errors = true;
				continue;
			}
			undo_manager_->recordUndoStep(std::make_unique<EntryDataUS>(entry)); // Create undo step
			entry->importMemChunk(kvx);                                          // Load doom sound data
			EntryType::detectEntryType(*entry);                                  // Update entry type
			entry->setExtensionByType();                                         // Update extension if necessary
		}
	}

	// Finish recording undo level
	undo_manager_->endRecord(true);

	// Show message if errors occurred
	if (errors)
		wxMessageBox(
			wxS("Some entries could not be converted, see console log for details"), wxS("SLADE"), wxICON_INFORMATION);

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
				(archive->formatId() == "wad" ? wadnames[e] : zipnames[e]), index, lastEntry->parentDir());
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
		(animdefs ? (archive->formatId() == "wad" ? "ANIMDEFS" : "animdefs.txt")
				  : (archive->formatId() == "wad" ? "SWANTBLS" : "swantbls.dat")),
		index,
		lastEntry->parentDir());

	// Finish recording undo level
	undo_manager_->endRecord(true);

	// Convert to ZDoom-compatible ANIMDEFS
	if (output)
	{
		// Create the memory buffer
		string gentext;
		if (animdefs)
		{
			gentext = "// ANIMDEFS lump generated by SLADE3\n// on " + wxNow().utf8_string() + "\n\n";
		}
		else
		{
			gentext = "# SWANTBLS data generated by SLADE 3\n# on " + wxNow().utf8_string() + "\n#\n"
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
	auto            source  = pal6bit->rawData(true);
	vector<uint8_t> dest(pal6bit->size());
	memcpy(dest.data(), source, pal6bit->size());
	for (size_t i = 0; i < pal6bit->size(); ++i)
		dest[i] = ((dest[i] << 2) | (dest[i] >> 4));
	pal6bit->importMem(dest.data(), pal6bit->size());
	maineditor::currentEntryPanel()->callRefresh();
	return true;
}

// -----------------------------------------------------------------------------
// Converts selected wav format entries to doom sound format
// -----------------------------------------------------------------------------
bool ArchivePanel::wavDSndConvert() const
{
	// Get selected entries
	auto selection = entry_tree_->selectedEntries();

	// Begin recording undo level
	undo_manager_->beginRecord("Convert Wav -> Doom Sound");

	// Go through selection
	bool errors = false;
	for (auto& entry : selection)
	{
		// Convert WAV -> Doom Sound if the entry is WAV format
		if (entry->type()->formatId() == "snd_wav")
		{
			MemChunk dsnd;
			// Attempt conversion
			if (!conversion::wavToDoomSnd(entry->data(), dsnd))
			{
				log::error("Unable to convert entry {}: {}", entry->name(), global::error);
				errors = true;
				continue;
			}
			undo_manager_->recordUndoStep(std::make_unique<EntryDataUS>(entry)); // Create undo step
			entry->importMemChunk(dsnd);                                         // Load doom sound data
			EntryType::detectEntryType(*entry);                                  // Update entry type
			entry->setExtensionByType();                                         // Update extension if necessary
		}
	}

	// Finish recording undo level
	undo_manager_->endRecord(true);

	// Show message if errors occurred
	if (errors)
		wxMessageBox(
			wxS("Some entries could not be converted, see console log for details"), wxS("SLADE"), wxICON_INFORMATION);

	return true;
}

// -----------------------------------------------------------------------------
// Converts selected doom sound format entries to wav format
// -----------------------------------------------------------------------------
bool ArchivePanel::dSndWavConvert() const
{
	// Get selected entries
	auto selection = entry_tree_->selectedEntries();

	// Begin recording undo level
	undo_manager_->beginRecord("Convert Doom Sound -> Wav");

	// Go through selection
	bool errors = false;
	for (auto& entry : selection)
	{
		bool     worked = false;
		MemChunk wav;
		// Convert Doom Sound -> WAV if the entry is Doom Sound format
		if (entry->type()->formatId() == "snd_doom" || entry->type()->formatId() == "snd_doom_mac")
			worked = conversion::doomSndToWav(entry->data(), wav);
		// Or Doom Speaker sound format
		else if (entry->type()->formatId() == "snd_speaker")
			worked = conversion::spkSndToWav(entry->data(), wav);
		// Or Jaguar Doom sound format
		else if (entry->type()->formatId() == "snd_jaguar")
			worked = conversion::jagSndToWav(entry->data(), wav);
		// Or Wolfenstein 3D sound format
		else if (entry->type()->formatId() == "snd_wolf")
			worked = conversion::wolfSndToWav(entry->data(), wav);
		// Or Creative Voice File format
		else if (entry->type()->formatId() == "snd_voc")
			worked = conversion::vocToWav(entry->data(), wav);
		// Or Blood SFX format (this one needs to be given the entry, not just the mem chunk)
		else if (entry->type()->formatId() == "snd_bloodsfx")
			worked = conversion::bloodToWav(entry, wav);
		// If successfully converted, update the entry
		if (worked)
		{
			undo_manager_->recordUndoStep(std::make_unique<EntryDataUS>(entry)); // Create undo step
			entry->importMemChunk(wav);                                          // Load wav data
			EntryType::detectEntryType(*entry);                                  // Update entry type
			entry->setExtensionByType();                                         // Update extension if necessary
		}
		else
		{
			log::error("Unable to convert entry {}: {}", entry->name(), global::error);
			errors = true;
		}
	}

	// Finish recording undo level
	undo_manager_->endRecord(true);

	// Show message if errors occurred
	if (errors)
		wxMessageBox(
			wxS("Some entries could not be converted, see console log for details"), wxS("SLADE"), wxICON_INFORMATION);

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
// Compiles any selected text entries as ACS scripts
// -----------------------------------------------------------------------------
bool ArchivePanel::optimizePNG() const
{
	// Check if the PNG tools path are set up, at least one of them should be
	if ((path_pngcrush.empty() || !fileutil::fileExists(path_pngcrush))
		&& (path_pngout.empty() || !fileutil::fileExists(path_pngout))
		&& (path_deflopt.empty() || !fileutil::fileExists(path_deflopt)))
	{
		wxMessageBox(
			wxS("Error: PNG tool paths not defined or invalid, please configure in SLADE preferences"),
			wxS("Error"),
			wxOK | wxCENTRE | wxICON_ERROR);
		return false;
	}

	// Get selected entries
	auto selection = entry_tree_->selectedEntries();

	ui::showSplash("Running external programs, please wait...", true, maineditor::windowWx());

	// Begin recording undo level
	undo_manager_->beginRecord("Optimize PNG");

	// Go through selection
	for (unsigned a = 0; a < selection.size(); a++)
	{
		ui::setSplashProgressMessage(selection[a]->nameNoExt());
		ui::setSplashProgress(static_cast<float>(a) / static_cast<float>(selection.size()));
		if (selection[a]->type()->formatId() == "img_png")
		{
			undo_manager_->recordUndoStep(std::make_unique<EntryDataUS>(selection[a]));
			entryoperations::optimizePNG(selection[a]);
		}
	}
	ui::hideSplash();

	// Finish recording undo level
	undo_manager_->endRecord(true);

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
		// Select new TEXTURES entry
		// entry_list_->clearSelection();
		// entry_list_->selectItem(index);

		// Finish recording undo level
		undo_manager_->endRecord(true);

		return true;
	}

	// Finish recording undo level
	undo_manager_->endRecord(false);

	return false;
}

// -----------------------------------------------------------------------------
// Detect errors in a TEXTUREx entry
// -----------------------------------------------------------------------------
bool ArchivePanel::findTextureErrors() const
{
	return entryoperations::findTextureErrors(entry_tree_->selectedEntries());
}

// -----------------------------------------------------------------------------
// Clean texture entries that are duplicates of entries in the iwad
// -----------------------------------------------------------------------------
bool ArchivePanel::cleanTextureIwadDupes() const
{
	return entryoperations::cleanTextureIwadDupes(entry_tree_->selectedEntries());
}

// -----------------------------------------------------------------------------
// Clean ZDTEXTURES entries that are just a single patch
// -----------------------------------------------------------------------------
bool ArchivePanel::cleanZdTextureSinglePatch() const
{
	return entryoperations::cleanZdTextureSinglePatch(entry_tree_->selectedEntries());
}

// -----------------------------------------------------------------------------
// Opens the currently selected entry in Doom Builder 2 if it is a valid map
// entry (either a map header or archive in maps/)
// -----------------------------------------------------------------------------
bool ArchivePanel::mapOpenDb2() const
{
	return entryoperations::openMapDB2(entry_tree_->firstSelectedEntry());
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
			log::error("Trying to open nonexistant directory {}", entry->path(true));
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
			log::warning("Entry editor {} does not exist, using default editor", entry->type()->editor());

		// Load the entry into the panel
		if (!new_area->openEntry(entry))
			wxMessageBox(WX_FMT("Error loading entry:\n{}", global::error), wxS("Error"), wxOK | wxICON_ERROR);

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
bool ArchivePanel::openEntryAsText(ArchiveEntry* entry)
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
		wxMessageBox(WX_FMT("Error loading entry:\n{}", global::error), wxS("Error"), wxOK | wxICON_ERROR);

	// Show the text entry panel
	return showEntryPanel(textArea());
}

// -----------------------------------------------------------------------------
// Opens [entry] in the hex editor panel
// -----------------------------------------------------------------------------
bool ArchivePanel::openEntryAsHex(ArchiveEntry* entry)
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
		wxMessageBox(WX_FMT("Error loading entry:\n{}", global::error), wxS("Error"), wxOK | wxICON_ERROR);

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
wxMenu* ArchivePanel::createEntryOpenMenu(const string& category)
{
	current_external_exe_category_ = category;
	current_external_exes_.clear();
	auto menu_open = new wxMenu();

	// New Tab
	SAction::fromId("arch_entry_opentab")->addToMenu(menu_open, true);
	menu_open->AppendSeparator();

	// External editors
	auto     external   = executables::externalExes(category);
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
		entryoperations::addToPatchTable(entry_tree_->selectedEntries());
	else if (id == "arch_gfx_addtexturex")
		entryoperations::createTexture(entry_tree_->selectedEntries());
	else if (id == "arch_gfx_exportpng")
		gfxExportPNG();
	else if (id == "arch_gfx_pngopt")
		optimizePNG();
	else if (id == "arch_gfx_offsetpaste")
		entryoperations::pasteGfxOffsets(entry_tree_->selectedEntries());
	else if (id == "arch_view_text")
		openEntryAsText(entry_tree_->firstSelectedEntry());
	else if (id == "arch_view_hex")
		openEntryAsHex(entry_tree_->firstSelectedEntry());
	else if (id == "arch_audio_convertdw")
		dSndWavConvert();
	else if (id == "arch_audio_convertwd")
		wavDSndConvert();
	else if (id == "arch_audio_convertmus")
		musMidiConvert();
	else if (id == "arch_voxel_convertvox")
		voxelConvert();
	else if (id == "arch_scripts_compileDECOHack")
		compileDECOHack();
	else if (id == "arch_scripts_compileacs")
		compileACS();
	else if (id == "arch_scripts_compilehacs")
		compileACS(true);
	else if (id == "arch_texturex_convertzd")
		convertTextures();
	else if (id == "arch_texturex_finderrors")
		findTextureErrors();
	else if (id == "arch_texture_clean_iwaddupes")
		cleanTextureIwadDupes();
	else if (id == "arch_zdtextures_clean_singlepatch")
		cleanZdTextureSinglePatch();
	else if (id == "arch_map_opendb2")
		mapOpenDb2();
	else if (id == "arch_entry_setup_external")
		PreferencesDialog::openPreferences(theMainWindow, "Editing", "external");

	// Map Editor Panel
	else if (id == "pmap_open_text")
	{
		// Edit Level Script

		// Get entry to edit
		ArchiveEntry* entry = mapArea()->entry();

		// Open in text editor
		openEntryAsText(entry);
	}

	// Run archive
	else if (id == "arch_run" || id == "arch_quick_run")
	{
		RunDialog dlg(this, archive.get());
		if (id == "arch_quick_run" || dlg.ShowModal() == wxID_OK)
		{
			auto command = dlg.selectedCommandLine(archive.get(), "");
			if (!command.empty())
			{
				// Set working directory
				auto wd = wxGetCwd();
				wxSetWorkingDirectory(wxString::FromUTF8(dlg.selectedExeDir()));

				// Run
				wxExecute(wxString::FromUTF8(command), wxEXEC_ASYNC);

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
		if (archive->formatId() == "folder")
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
		entry_tree_->setFilter(
			text_filter_->GetValue().utf8_string(),
			choice_category_->GetSelection() > 0 ? choice_category_->GetStringSelection().utf8_string() : "");
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
	bool has_dirs = archive->formatDesc().supports_dirs;

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
	string category;
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
			if (entry->state() == ArchiveEntry::State::Modified)
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
				if (category != entry->type()->category())
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
		auto open_item = new wxMenuItem(&context, wxID_ANY, wxS("Open"));
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
			context.AppendSubMenu(custom, wxString::FromUTF8(cur_area_->customMenuName()));
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
			context.AppendSubMenu(viewas, wxS("View As"));
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
			context.AppendSubMenu(gfx, wxS("Graphics"));
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
		if (app::clipboard().firstItem(ClipboardItem::Type::GfxOffsets) != nullptr)
			SAction::fromId("arch_gfx_offsetpaste")->addToMenu(gfx, true);
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
			context.AppendSubMenu(audio, wxS("Audio"));
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
			context.AppendSubMenu(scripts, wxS("Script"));
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
			context.AppendSubMenu(voxels, wxS("Voxels"));
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
		context.AppendSubMenu(menu_scripts, wxS("Run &Script"));
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
		context.AppendSubMenu(menu_new, wxS("New"));
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

	// Archive
	if (entry->type()->formatId().substr(0, 8) == "archive_")
		app::archiveManager().openArchive(entry);

	// Texture list
	else if (
		entry->type()->formatId() == "texturex" || entry->type() == EntryType::fromId("pnames")
		|| entry->type() == EntryType::fromId("zdtextures"))
		maineditor::openTextureEditor(archive.get(), entry);

	// Map
	// TODO: Needs to filter the game/port lists in the dialog by the map format
	else if (entry->type() == EntryType::mapMarkerType())
	{
		// Open map editor config dialog
		MapEditorConfigDialog dlg(this, archive.get(), false);
		if (dlg.ShowModal() == wxID_OK)
		{
			auto info = archive->mapDesc(entry);

			// Check selected game configuration is ok
			if (!dlg.configMatchesMap(info))
				wxMessageBox(
					wxS("Selected Game Configuration does not match the map format"), wxS("Error"), wxICON_ERROR);
			else
			{
				// Load game configuration
				game::configuration().openConfig(dlg.selectedGame(), dlg.selectedPort(), info.format);

				// Attempt to open map
				if (mapeditor::window()->openMap(info))
					mapeditor::window()->Show();
				else
				{
					mapeditor::window()->Hide();
					wxMessageBox(
						WX_FMT("Unable to open map {}: {}", entry->name(), global::error),
						wxS("Invalid map error"),
						wxICON_ERROR);
				}
			}
		}
	}

	// Other entry
	else if (entry->type() != EntryType::folderType())
		maineditor::openEntry(entry);

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
	text_filter_->SetValue(wxEmptyString);
}


// -----------------------------------------------------------------------------
//
// EntryDataUS Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Swaps data between the entry and the undo step
// -----------------------------------------------------------------------------
bool EntryDataUS::swapData()
{
	// log::info(1, "Entry data swap...");

	// Get parent dir
	auto dir = archive_->dirAtPath(path_);
	if (dir)
	{
		// Get entry
		auto entry = dir->entryAt(index_);
		if (!entry)
			return false;

		// Backup data
		MemChunk temp_data;
		temp_data.importMem(entry->rawData(), entry->size());
		// log::info(1, "Backup current data, size %d", entry->getSize());

		// Restore entry data
		if (data_.size() == 0)
		{
			entry->clearData();
			// log::info(1, "Clear entry data");
		}
		else
		{
			entry->importMemChunk(data_);
			// log::info(1, "Restored entry data, size %d", data.getSize());
		}

		// Store previous entry data
		if (temp_data.size() > 0)
			data_.importMem(temp_data.data(), temp_data.size());
		else
			data_.clear();

		return true;
	}

	return false;
}


// -----------------------------------------------------------------------------
// Console Commands
//
// I'd love to put them in their own file, but attempting to do so results in
// a circular include nightmare and nothing works anymore.
// TODO: Look at doing something else with these
// -----------------------------------------------------------------------------

#include "App.h"
#include "General/Console.h"

CONSOLE_COMMAND(palconv, 0, false)
{
	ArchivePanel* meep = maineditor::currentArchivePanel();
	if (meep)
	{
		meep->palConvert();
		meep->reloadCurrentPanel();
	}
}

CONSOLE_COMMAND(palconv64, 0, false)
{
	ArchivePanel* meep = maineditor::currentArchivePanel();
	if (meep)
	{
		// Get the entry index of the last selected list item
		ArchiveEntry* pal    = meep->currentEntry();
		auto&         source = pal->data(true);
		uint8_t*      dest   = new uint8_t[(pal->size() / 2) * 3];
		for (size_t i = 0; i < pal->size() / 2; ++i)
		{
			uint8_t  r, g, b;
			uint16_t col      = source.readB16(2 * i);
			r                 = (col & 0xF800) >> 8;
			g                 = (col & 0x07C0) >> 3;
			b                 = (col & 0x003E) << 2;
			dest[(3 * i) + 0] = r;
			dest[(3 * i) + 1] = g;
			dest[(3 * i) + 2] = b;
		}
		pal->importMem(dest, (pal->size() / 2) * 3);
		maineditor::currentEntryPanel()->callRefresh();
		delete[] dest;
	}
}

CONSOLE_COMMAND(palconvpsx, 0, false)
{
	ArchivePanel* meep = maineditor::currentArchivePanel();
	if (meep)
	{
		// Get the entry index of the last selected list item
		ArchiveEntry* pal    = meep->currentEntry();
		auto&         source = pal->data(true);
		uint8_t*      dest   = new uint8_t[(pal->size() / 2) * 3];
		for (size_t i = 0; i < pal->size() / 2; ++i)
		{
			// A1 B5 G5 R5, LE
			uint8_t  a, r, g, b;
			uint16_t col      = source.readL16(2 * i);
			a                 = (col & 0x8000) >> 15;
			b                 = (col & 0x7C00) >> 10;
			g                 = (col & 0x03E0) >> 5;
			r                 = (col & 0x001F);
			r                 = (r << 3) | (r >> 2);
			g                 = (g << 3) | (g >> 2);
			b                 = (b << 3) | (b >> 2);
			dest[(3 * i) + 0] = r;
			dest[(3 * i) + 1] = g;
			dest[(3 * i) + 2] = b;
		}
		pal->importMem(dest, (pal->size() / 2) * 3);
		maineditor::currentEntryPanel()->callRefresh();
		delete[] dest;
	}
}

CONSOLE_COMMAND(vertex32x, 0, false)
{
	ArchivePanel* meep = maineditor::currentArchivePanel();
	if (meep)
	{
		// Get the entry index of the last selected list item
		ArchiveEntry*  v32x   = meep->currentEntry();
		const uint8_t* source = v32x->rawData(true);
		uint8_t*       dest   = new uint8_t[v32x->size() / 2];
		for (size_t i = 0; i < v32x->size() / 4; ++i)
		{
			dest[2 * i + 0] = source[4 * i + 1];
			dest[2 * i + 1] = source[4 * i + 0];
		}
		v32x->importMem(dest, v32x->size() / 2);
		maineditor::currentEntryPanel()->callRefresh();
		delete[] dest;
	}
}

CONSOLE_COMMAND(vertexpsx, 0, false)
{
	ArchivePanel* meep = maineditor::currentArchivePanel();
	if (meep)
	{
		// Get the entry index of the last selected list item
		ArchiveEntry*  vpsx   = meep->currentEntry();
		const uint8_t* source = vpsx->rawData(true);
		uint8_t*       dest   = new uint8_t[vpsx->size() / 2];
		for (size_t i = 0; i < vpsx->size() / 4; ++i)
		{
			dest[2 * i + 0] = source[4 * i + 2];
			dest[2 * i + 1] = source[4 * i + 3];
		}
		vpsx->importMem(dest, vpsx->size() / 2);
		maineditor::currentEntryPanel()->callRefresh();
		delete[] dest;
	}
}

CONSOLE_COMMAND(lightspsxtopalette, 0, false)
{
	ArchivePanel* meep = maineditor::currentArchivePanel();
	if (meep)
	{
		// Get the entry index of the last selected list item
		ArchiveEntry*  lights  = meep->currentEntry();
		const uint8_t* source  = lights->rawData(true);
		size_t         entries = lights->size() / 4;
		uint8_t*       dest    = new uint8_t[entries * 3];
		for (size_t i = 0; i < entries; ++i)
		{
			dest[3 * i + 0] = source[4 * i + 0];
			dest[3 * i + 1] = source[4 * i + 1];
			dest[3 * i + 2] = source[4 * i + 2];
		}
		lights->importMem(dest, entries * 3);
		maineditor::currentEntryPanel()->callRefresh();
		delete[] dest;
	}
}


vector<ArchiveEntry*> Console_SearchEntries(string name)
{
	vector<ArchiveEntry*> entries;
	Archive*              archive = maineditor::currentArchive();
	ArchivePanel*         panel   = maineditor::currentArchivePanel();

	if (archive)
	{
		Archive::SearchOptions options;
		options.search_subdirs = true;
		if (panel)
		{
			options.dir = panel->currentDir();
		}
		options.match_name = name;
		entries            = archive->findAll(options);
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
			message += entries[i]->path(true) + "\n";
		}
	}
	log::info("Found %i entr%s", count, count == 1 ? "y" : "ies\n" + message);
}

CONSOLE_COMMAND(ren, 2, true)
{
	Archive*              archive = maineditor::currentArchive();
	vector<ArchiveEntry*> entries = Console_SearchEntries(args[0]);
	if (!entries.empty())
	{
		size_t count = 0;
		for (size_t i = 0; i < entries.size(); ++i)
		{
			// Rename filter logic
			auto newname = entries[i]->name();
			for (unsigned c = 0; c < args[1].size(); c++)
			{
				// Check character
				if (args[1][c] == '*')
					continue; // Skip if *
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
		log::info("Renamed {} entr{}", count, count == 1 ? "y" : "ies");
	}
}

CONSOLE_COMMAND(cd, 1, true)
{
	Archive*      current = maineditor::currentArchive();
	ArchivePanel* panel   = maineditor::currentArchivePanel();

	if (current && panel)
	{
		ArchiveDir* dir    = panel->currentDir();
		ArchiveDir* newdir = current->dirAtPath(args[0], dir);
		if (newdir == nullptr)
		{
			if (args[0] == "..")
				newdir = dir->parent().get();
			else if (args[0] == "/" || args[0] == "\\")
				newdir = current->rootDir().get();
		}

		if (newdir)
		{
			panel->openDir(ArchiveDir::getShared(newdir));
		}
		else
		{
			log::error("Error: Trying to open nonexistant directory {}", args[0]);
		}
	}
}
