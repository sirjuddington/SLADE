
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ArchiveManagerPanel.cpp
// Description: ArchiveManagerPanel class. Basically the UI for the
//              ArchiveManager class, has a tree of all open archives and basic
//              file management controls.
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
#include "ArchiveManagerPanel.h"
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "Archive/Formats/DirArchive.h"
#include "ArchivePanel.h"
#include "EntryPanel/EntryPanel.h"
#include "General/UI.h"
#include "Graphics/Icons.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/MainWindow.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/UI/MapEditorWindow.h"
#include "TextureXEditor/TextureXEditor.h"
#include "UI/Controls/STabCtrl.h"
#include "UI/Dialogs/DirArchiveUpdateDialog.h"
#include "UI/Dialogs/NewArchiveDiaog.h"
#include "UI/WxUtils.h"
#include "Utility/FileUtils.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, close_archive_with_tab, true, CVar::Flag::Save)
CVAR(Int, am_current_tab, 0, CVar::Flag::Save)
CVAR(Bool, am_file_browser_tab, false, CVar::Flag::Save)
CVAR(Int, dir_archive_change_action, 2, CVar::Flag::Save) // 0=always ignore, 1=always apply, 2+=ask


// -----------------------------------------------------------------------------
//
// External Variables
//
// ----------------------------------------------------------------------------
EXTERN_CVAR(String, dir_last)
EXTERN_CVAR(Int, autosave_entry_changes)


// -----------------------------------------------------------------------------
//
// DirArchiveCheck Class Functions
//
// -----------------------------------------------------------------------------
wxDEFINE_EVENT(wxEVT_COMMAND_DIRARCHIVECHECK_COMPLETED, wxThreadEvent);

// -----------------------------------------------------------------------------
// DirArchiveCheck class constructor
// -----------------------------------------------------------------------------
DirArchiveCheck::DirArchiveCheck(wxEvtHandler* handler, DirArchive* archive) :
	handler_{ handler },
	dir_path_{ archive->filename() },
	removed_files_{ archive->removedFiles() },
	change_list_{ archive, {} },
	ignore_hidden_{ archive->hiddenFilesIgnored() }
{
	// Get flat entry list
	vector<ArchiveEntry*> entries;
	archive->putEntryTreeAsList(entries);

	// Build entry info list
	for (auto& entry : entries)
	{
		entry_info_.emplace_back(
			entry->path(true),
			entry->exProps().getOr<string>("filePath", ""),
			entry->type() == EntryType::folderType(),
			archive->fileModificationTime(entry));
	}
}

// -----------------------------------------------------------------------------
// Register a change to a file, as long as it hasn't been ignored
// -----------------------------------------------------------------------------
void DirArchiveCheck::addChange(DirEntryChange change)
{
	if (!dynamic_cast<DirArchive*>(change_list_.archive)->shouldIgnoreEntryChange(change))
		change_list_.changes.push_back(change);
}

// -----------------------------------------------------------------------------
// DirArchiveCheck thread entry function
// -----------------------------------------------------------------------------
wxThread::ExitCode DirArchiveCheck::Entry()
{
	// Get current directory structure
	vector<string>      files, dirs;
	DirArchiveTraverser traverser(files, dirs, ignore_hidden_);
	wxDir               dir(dir_path_);
	dir.Traverse(traverser, "", wxDIR_FILES | wxDIR_DIRS);

	// Check for deleted files
	for (auto& info : entry_info_)
	{
		auto path = info.file_path.ToStdString();

		// Ignore if not on disk
		if (path.empty())
			continue;

		if (info.is_dir)
		{
			if (!wxDirExists(path))
				addChange(DirEntryChange(DirEntryChange::Action::DeletedDir, path, info.entry_path.ToStdString()));
		}
		else
		{
			if (!wxFileExists(path))
				addChange(DirEntryChange(DirEntryChange::Action::DeletedFile, path, info.entry_path.ToStdString()));
		}
	}

	// Check for new/updated files
	for (const auto& file : files)
	{
		// Ignore files removed from archive since last save
		if (VECTOR_EXISTS(removed_files_, file))
			continue;

		// Find file in archive
		EntryInfo inf;
		bool      found = false;
		for (auto& info : entry_info_)
		{
			if (info.file_path == file)
			{
				inf   = info;
				found = true;
				break;
			}
		}

		time_t mod = wxFileModificationTime(file);

		// No match, added to archive
		if (!found)
			addChange(DirEntryChange(DirEntryChange::Action::AddedFile, file, "", mod));
		// Matched, check modification time
		else if (mod > inf.file_modified)
			addChange(DirEntryChange(DirEntryChange::Action::Updated, file, inf.entry_path.ToStdString(), mod));
	}

	// Check for new dirs
	for (const auto& subdir : dirs)
	{
		// Ignore dirs removed from archive since last save
		if (VECTOR_EXISTS(removed_files_, subdir))
			continue;

		bool found = false;
		for (auto& info : entry_info_)
		{
			if (info.file_path == subdir)
			{
				found = true;
				break;
			}
		}

		time_t mod = wxDateTime::Now().GetTicks();

		// No match, added to archive
		if (!found)
			addChange(DirEntryChange(DirEntryChange::Action::AddedDir, subdir, "", mod));
	}

	// Send changes via event
	auto event = new wxThreadEvent(wxEVT_COMMAND_DIRARCHIVECHECK_COMPLETED);
	event->SetPayload<DirArchiveChangeList>(change_list_);
	wxQueueEvent(handler_, event);

	return nullptr;
}


// -----------------------------------------------------------------------------
//
// WMFileBrowser Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// WMFileBrowser class constructor
// Note: wxWidgets 2.9.4 deprecated the wxDIRCTRL_SHOW_FILTERS flag; instead,
// the filters are always shown if any are defined.
// -----------------------------------------------------------------------------
WMFileBrowser::WMFileBrowser(wxWindow* parent, ArchiveManagerPanel* wm, int id) :
	wxGenericDirCtrl(parent, id, wxDirDialogDefaultFolderStr), parent{ wm }
{
	// Connect a custom event for when an item in the file tree is activated
	auto tree_ctrl = wxGenericDirCtrl::GetTreeCtrl();
	tree_ctrl->Connect(
		tree_ctrl->GetId(), wxEVT_TREE_ITEM_ACTIVATED, wxTreeEventHandler(WMFileBrowser::onItemActivated));
}

// -----------------------------------------------------------------------------
// Event called when an item in the tree is activated. Opens a file if one is
// selected.
// -----------------------------------------------------------------------------
void WMFileBrowser::onItemActivated(wxTreeEvent& e)
{
	// Get related objects
	auto tree    = dynamic_cast<wxTreeCtrl*>(e.GetEventObject());
	auto browser = dynamic_cast<WMFileBrowser*>(tree->GetParent());

	// If the selected item has no children (ie it's a file),
	// open it in the archive manager
	if (!tree->ItemHasChildren(e.GetItem()))
		browser->parent->openFile(browser->GetPath());

	e.Skip();
}


// -----------------------------------------------------------------------------
//
// ArchiveManagerPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ArchiveManagerPanel class constructor
// -----------------------------------------------------------------------------
ArchiveManagerPanel::ArchiveManagerPanel(wxWindow* parent, STabCtrl* nb_archives) :
	DockPanel(parent), stc_archives_{ nb_archives }
{
	// Create main sizer
	auto vbox = new wxBoxSizer(wxVERTICAL);
	SetSizer(vbox);

	// Create/setup tabs
	stc_tabs_ = new STabCtrl(this, false);
	stc_tabs_->SetInitialSize(wxSize(ui::scalePx(224), -1));
	vbox->Add(stc_tabs_, 1, wxEXPAND | wxALL, ui::pad());

	// Open archives tab
	panel_am_ = new wxPanel(stc_tabs_);
	stc_tabs_->AddPage(panel_am_, "Archives", true);

	// Create/setup archive list
	createArchivesPanel();
	refreshArchiveList();

	// Create/setup recent files list and menu
	menu_recent_ = new wxMenu();
	createRecentPanel();
	refreshRecentFileList();

	// Create/setup bookmarks tab
	auto panel_bm = new wxPanel(stc_tabs_);
	panel_bm->SetSizer(new wxBoxSizer(wxVERTICAL));
	list_bookmarks_ = new ListView(panel_bm, -1);
	menu_bookmarks_ = new wxMenu();
	panel_bm->GetSizer()->Add(list_bookmarks_, 1, wxEXPAND | wxALL, ui::pad());
	refreshBookmarkList();
	stc_tabs_->AddPage(panel_bm, "Bookmarks", true);

	// Create/setup file browser tab
	// Used to cause problems, however seems to work much better now than what I can remember,
	// adding a cvar to disable in case it still has issues
	if (am_file_browser_tab)
	{
		auto panel = new wxPanel(stc_tabs_);
		panel->SetSizer(new wxBoxSizer(wxVERTICAL));
		file_browser_ = new WMFileBrowser(panel, this, -1);
		panel->GetSizer()->Add(file_browser_, 1, wxEXPAND | wxALL, ui::pad());
		stc_tabs_->AddPage(panel, "File Browser");
	}

	// Set current tab
	stc_tabs_->SetSelection(am_current_tab);

	// Bind events
	list_archives_->Bind(wxEVT_LIST_ITEM_SELECTED, &ArchiveManagerPanel::onListArchivesChanged, this);
	list_archives_->Bind(wxEVT_LIST_ITEM_ACTIVATED, &ArchiveManagerPanel::onListArchivesActivated, this);
	list_archives_->Bind(wxEVT_LIST_ITEM_RIGHT_CLICK, &ArchiveManagerPanel::onListArchivesRightClick, this);
	list_recent_->Bind(wxEVT_LIST_ITEM_ACTIVATED, &ArchiveManagerPanel::onListRecentActivated, this);
	list_recent_->Bind(wxEVT_LIST_ITEM_RIGHT_CLICK, &ArchiveManagerPanel::onListRecentRightClick, this);
	list_bookmarks_->Bind(wxEVT_LIST_ITEM_ACTIVATED, [&](wxListEvent&) { am_current_tab = stc_tabs_->GetSelection(); });
	list_bookmarks_->Bind(wxEVT_LIST_ITEM_RIGHT_CLICK, &ArchiveManagerPanel::onListBookmarksRightClick, this);
	stc_archives_->Bind(wxEVT_AUINOTEBOOK_PAGE_CHANGING, [&](wxAuiNotebookEvent& e) { e.Skip(); });
	stc_archives_->Bind(wxEVT_AUINOTEBOOK_PAGE_CHANGED, &ArchiveManagerPanel::onArchiveTabChanged, this);
	stc_archives_->Bind(wxEVT_AUINOTEBOOK_PAGE_CLOSE, &ArchiveManagerPanel::onArchiveTabClose, this);
	stc_archives_->Bind(wxEVT_AUINOTEBOOK_PAGE_CLOSED, &ArchiveManagerPanel::onArchiveTabClosed, this);
	stc_tabs_->Bind(
		wxEVT_AUINOTEBOOK_PAGE_CHANGED, [&](wxAuiNotebookEvent&) { am_current_tab = stc_tabs_->GetSelection(); });
	Bind(wxEVT_COMMAND_DIRARCHIVECHECK_COMPLETED, &ArchiveManagerPanel::onDirArchiveCheckCompleted, this);

	connectSignals();

	// Init layout
	wxWindowBase::Layout();
	SetInitialSize(wxSize(ui::scalePx(256), -1));
}

// -----------------------------------------------------------------------------
// Creates the 'Open Archives' panel
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::createArchivesPanel()
{
	panel_archives_ = new wxPanel(panel_am_, -1);
	auto vbox       = new wxBoxSizer(wxVERTICAL);
	panel_archives_->SetSizer(vbox);
	vbox->Add(new wxStaticText(panel_archives_, -1, "Open Archives:"), 0, wxEXPAND);
	list_archives_ = new ListView(panel_archives_, -1);
	vbox->Add(list_archives_, 1, wxEXPAND | wxTOP, ui::px(ui::Size::PadMinimum));
}

// -----------------------------------------------------------------------------
// Creates the 'Recent Files' panel
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::createRecentPanel()
{
	panel_rf_ = new wxPanel(panel_am_, -1);
	auto vbox = new wxBoxSizer(wxVERTICAL);
	panel_rf_->SetSizer(vbox);
	vbox->Add(new wxStaticText(panel_rf_, -1, "Recent Files:"), 0, wxEXPAND);
	list_recent_ = new ListView(panel_rf_, -1);
	vbox->Add(list_recent_, 1, wxEXPAND | wxTOP, ui::px(ui::Size::PadMinimum));

	// Setup image list
	auto list = wxutil::createSmallImageList();
	list->Add(icons::getIcon(icons::Entry, "archive"));
	list->Add(icons::getIcon(icons::Entry, "wad"));
	list->Add(icons::getIcon(icons::Entry, "zip"));
	list->Add(icons::getIcon(icons::Entry, "folder"));
	list_recent_->SetImageList(list, wxIMAGE_LIST_SMALL);
}

// -----------------------------------------------------------------------------
// Layout the panel normally
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::layoutNormal()
{
	// Layout archives tab vertically
	auto vbox = new wxBoxSizer(wxVERTICAL);
	panel_am_->SetSizer(vbox);

	vbox->Add(panel_archives_, 1, wxEXPAND | wxALL, ui::pad());
	vbox->Add(panel_rf_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, ui::pad());
}

// -----------------------------------------------------------------------------
// Layout the panel horizontally
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::layoutHorizontal()
{
	// Layout archives tab horizontally
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	panel_am_->SetSizer(hbox);

	hbox->Add(panel_archives_, 1, wxEXPAND | wxALL, ui::pad());
	hbox->Add(panel_rf_, 1, wxEXPAND | wxTOP | wxRIGHT | wxBOTTOM, ui::pad());
}

// -----------------------------------------------------------------------------
// Clears and rebuilds the recent file list in the menu and the tab
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::refreshRecentFileList() const
{
	// Clear the list
	list_recent_->ClearAll();

	// Get first recent file menu id
	auto a_recent        = SAction::fromId("aman_recent");
	int  id_recent_start = a_recent->wxId();

	// Clear menu; needs to do with a count down rather than up
	// otherwise the following elements are not properly removed
	for (unsigned a = menu_recent_->GetMenuItemCount(); a > 0; a--)
		menu_recent_->Destroy(id_recent_start + a - 1);

	// Add columns
	list_recent_->InsertColumn(0, "Filename");
	list_recent_->InsertColumn(1, "Path");

	// Add each recent archive (same logic as the recent files submenu)
	list_recent_->enableSizeUpdate(false);
	for (unsigned a = 0; a < app::archiveManager().numRecentFiles(); a++)
	{
		list_recent_->addItem(a, wxEmptyString);
		updateRecentListItem(a);

		if (a < 8)
		{
			// Get path and determine icon
			auto   fn   = app::archiveManager().recentFile(a);
			string icon = "archive";
			if (strutil::endsWith(fn, ".wad"))
				icon = "wad";
			else if (strutil::endsWith(fn, ".zip") || strutil::endsWith(fn, ".pk3") || strutil::endsWith(fn, ".pke"))
				icon = "zip";
			else if (wxDirExists(fn))
				icon = "folder";

			// Create and add menu item
			a_recent->addToMenu(menu_recent_, fn, icon, a);
			// wxMenuItem* mi = new wxMenuItem(menu_recent, id_recent_start + a, fn);
			// mi->SetBitmap(Icons::getIcon(Icons::ENTRY, icon));
			// menu_recent->Append(mi);
		}
	}

	// Update size
	list_recent_->enableSizeUpdate(true);
	list_recent_->updateSize();
}

// -----------------------------------------------------------------------------
// Disables updating the archive list
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::disableArchiveListUpdate() const
{
	list_archives_->enableSizeUpdate(false);
}

// -----------------------------------------------------------------------------
// Clears and rebuilds the open archives list
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::refreshArchiveList() const
{
	// Clear the list
	list_archives_->ClearAll();

	// Add columns
	list_archives_->InsertColumn(0, "Filename");
	list_archives_->InsertColumn(1, "Path");

	// Add each archive that is opened in the ArchiveManager
	list_archives_->enableSizeUpdate(false);
	for (int a = 0; a < app::archiveManager().numArchives(); a++)
	{
		list_archives_->addItem(a, wxEmptyString);
		updateOpenListItem(a);
	}

	// Update size
	list_archives_->enableSizeUpdate(true);
	list_archives_->updateSize();
}

// -----------------------------------------------------------------------------
// Refreshes all open archive tabs
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::refreshAllTabs() const
{
	// Go through tabs
	for (unsigned a = 0; a < stc_archives_->GetPageCount(); a++)
	{
		auto tab = stc_archives_->GetPage(a);

		// Refresh if it's an archive panel
		if (isArchivePanel(a))
			dynamic_cast<ArchivePanel*>(tab)->refreshPanel();
	}
}

// -----------------------------------------------------------------------------
// Updates the archive list item at <index>
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::updateOpenListItem(int index) const
{
	auto archive = app::archiveManager().getArchive(index);

	if (!archive)
		return;

	// Get path as wxFileName for processing
	wxFileName fn(archive->filename());

	// Set item name

	list_archives_->setItemText(index, 0, fn.GetFullName());
	list_archives_->setItemText(index, 1, fn.GetPath());

	// Set item status colour
	using ItemStatus = ListView::ItemStatus;
	if (archive->canSave())
	{
		if (archive->isModified())
			list_archives_->setItemStatus(index, ItemStatus::Modified);
		else
			list_archives_->setItemStatus(index, ItemStatus::Normal);
	}
	else
		list_archives_->setItemStatus(index, ItemStatus::New);
}

// -----------------------------------------------------------------------------
// Updates the recent file list item at <index>
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::updateRecentListItem(int index) const
{
	// Get path as wxFileName for processing
	wxString   path = app::archiveManager().recentFile(index);
	wxFileName fn(path);

	// Set item name
	list_recent_->setItemText(index, 0, fn.GetFullName());
	list_recent_->setItemText(index, 1, fn.GetPath());

	// Set item icon
	int icon = 0;
	if (path.EndsWith(".wad"))
		icon = 1;
	else if (path.EndsWith(".zip") || path.EndsWith(".pk3") || path.EndsWith(".pke"))
		icon = 2;
	else if (wxDirExists(path))
		icon = 3;
	list_recent_->SetItemImage(index, icon);
}

// -----------------------------------------------------------------------------
// Updates the title of the tab for the archive at <index>
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::updateArchiveTabTitle(int index) const
{
	auto archive = app::archiveManager().getArchive(index);

	if (!archive)
		return;

	// Go through all tabs
	for (size_t a = 0; a < stc_archives_->GetPageCount(); a++)
	{
		// Check page type is "archive"
		if (stc_archives_->GetPage(a)->GetName().CmpNoCase("archive"))
			continue;

		// Check for archive match
		auto ap = dynamic_cast<ArchivePanel*>(stc_archives_->GetPage(a));
		if (ap->archive() == archive.get())
		{
			wxString title;
			if (archive->isModified())
				title = wxString::Format("%s *", archive->filename(false));
			else
				title = archive->filename(false);
			stc_archives_->SetPageText(a, title);
			return;
		}
	}
}

// -----------------------------------------------------------------------------
// Checks if the tab at [tab_index] is an ArchivePanel.
// Returns true if it is, false if not
// -----------------------------------------------------------------------------
bool ArchiveManagerPanel::isArchivePanel(int tab_index) const
{
	// Check that tab index is in range
	if ((unsigned)tab_index >= stc_archives_->GetPageCount())
		return false;

	// Check the page's name
	return !stc_archives_->GetPage(tab_index)->GetName().CmpNoCase("archive") ? true : false;
}

// -----------------------------------------------------------------------------
// Checks if the tab at [tab_index] is an EntryPanel.
// Returns true if it is, false if not
// -----------------------------------------------------------------------------
bool ArchiveManagerPanel::isEntryPanel(int tab_index) const
{
	// Check that tab index is in range
	if ((unsigned)tab_index >= stc_archives_->GetPageCount())
		return false;

	// Check the page's name
	return !stc_archives_->GetPage(tab_index)->GetName().CmpNoCase("entry") ? true : false;
}

// -----------------------------------------------------------------------------
// Returns the archive associated with the archive tab at [tab_index] or null
// if the index is invalid or the tab isn't an archive panel
// -----------------------------------------------------------------------------
Archive* ArchiveManagerPanel::archiveForTab(int tab_index) const
{
	// Check the index is valid
	if (tab_index < 0 || (unsigned)tab_index >= stc_archives_->GetPageCount())
		return nullptr;

	// Check the specified tab is actually an archive tab
	if (!isArchivePanel(tab_index))
		return nullptr;

	// Get the archive associated with the tab
	auto ap = dynamic_cast<ArchivePanel*>(stc_archives_->GetPage(tab_index));
	return ap->archive();
}

// -----------------------------------------------------------------------------
// Returns the index of the currently open tab
// -----------------------------------------------------------------------------
int ArchiveManagerPanel::currentTabIndex() const
{
	return stc_archives_->GetSelection();
}

// -----------------------------------------------------------------------------
// Returns the currently 'open' archive - the archive associated with the
// current ArchivePanel tab.
// Returns null if the current tab isn't an ArchivePanel
// -----------------------------------------------------------------------------
Archive* ArchiveManagerPanel::currentArchive() const
{
	// Get current tab
	auto page = stc_archives_->GetPage(stc_archives_->GetSelection());

	// Return if no tabs exist
	if (!page)
		return nullptr;

	// ArchivePanel
	if (page->GetName() == "archive")
	{
		auto ap = dynamic_cast<ArchivePanel*>(page);
		return ap->archive();
	}

	// EntryPanel
	else if (page->GetName() == "entry")
	{
		auto ep = dynamic_cast<EntryPanel*>(page);
		return ep->entry()->parent();
	}

	// TextureXEditor
	else if (page->GetName() == "texture")
	{
		auto tx = dynamic_cast<TextureXEditor*>(page);
		return tx->archive();
	}

	// Not an archive-related tab
	else
		return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the current panel tab
// -----------------------------------------------------------------------------
wxWindow* ArchiveManagerPanel::currentPanel() const
{
	return stc_archives_->GetPage(stc_archives_->GetSelection());
}

// -----------------------------------------------------------------------------
// Returns the current panel
// -----------------------------------------------------------------------------
EntryPanel* ArchiveManagerPanel::currentArea() const
{
	// Get current tab index
	int selected = stc_archives_->GetSelection();

	// Entry tab
	if (isEntryPanel(selected))
		return dynamic_cast<EntryPanel*>(stc_archives_->GetPage(selected));

	// Archive tab
	if (isArchivePanel(selected))
	{
		auto ap = dynamic_cast<ArchivePanel*>(stc_archives_->GetPage(selected));
		return ap->currentArea();
	}

	// No currently active entry panel
	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the currently open entry in the current archive panel (if any)
// -----------------------------------------------------------------------------
ArchiveEntry* ArchiveManagerPanel::currentEntry() const
{
	// Get current tab index
	int selected = stc_archives_->GetSelection();

	// Check it's an archive tab
	if (!isArchivePanel(selected))
		return nullptr;

	// Get the archive panel
	auto ap = dynamic_cast<ArchivePanel*>(stc_archives_->GetPage(selected));
	return ap->currentEntry();
}

// -----------------------------------------------------------------------------
// Returns a list of all selected entries in the current archive panel
// -----------------------------------------------------------------------------
vector<ArchiveEntry*> ArchiveManagerPanel::currentEntrySelection() const
{
	// Get current tab index
	int selected = stc_archives_->GetSelection();

	// Check it's an archive tab
	if (!isArchivePanel(selected))
		return {};

	// Get the archive panel
	auto ap = dynamic_cast<ArchivePanel*>(stc_archives_->GetPage(selected));
	return ap->currentEntries();
}

// -----------------------------------------------------------------------------
// Opens a new tab for the archive at <archive_index> in the archive manager
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::openTab(int archive_index) const
{
	auto archive = app::archiveManager().getArchive(archive_index);
	if (archive)
		openTab(archive.get());
}

// -----------------------------------------------------------------------------
// Returns the ArchivePanel for [archive], or null if none is open
// -----------------------------------------------------------------------------
ArchivePanel* ArchiveManagerPanel::tabForArchive(Archive* archive) const
{
	// Check archive was given
	if (!archive)
		return nullptr;

	// Go through all tabs
	for (size_t a = 0; a < stc_archives_->GetPageCount(); a++)
	{
		// Check page type is "archive"
		if (stc_archives_->GetPage(a)->GetName().CmpNoCase("archive"))
			continue;

		// Check for archive match
		auto ap = dynamic_cast<ArchivePanel*>(stc_archives_->GetPage(a));
		if (ap->archive() == archive)
			return ap;
	}

	// No tab currently open for archive
	return nullptr;
}

// -----------------------------------------------------------------------------
// Opens a new tab for the archive
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::openTab(Archive* archive) const
{
	auto sp_archive = app::archiveManager().shareArchive(archive);
	if (sp_archive)
	{
		// Check if the archive is already open in a tab
		auto wp = tabForArchive(archive);
		if (wp)
		{
			// Switch to tab
			stc_archives_->SetSelection(stc_archives_->GetPageIndex(wp));
			wp->focusEntryList();

			return;
		}

		// If tab isn't already open, open a new one
		wp = new ArchivePanel(stc_archives_, sp_archive);

		// Determine icon
		string icon = "archive";
		if (archive->formatId() == "wad")
			icon = "wad";
		else if (archive->formatId() == "zip")
			icon = "zip";
		else if (archive->formatId() == "folder")
			icon = "folder";

		wp->SetName("archive");
		stc_archives_->AddPage(wp, archive->filename(false), false);
		stc_archives_->SetSelection(stc_archives_->GetPageCount() - 1);
		stc_archives_->SetPageBitmap(stc_archives_->GetPageCount() - 1, icons::getIcon(icons::Entry, icon));
		wp->addMenus();
		wp->Show(true);
		wp->SetFocus();
		wp->focusEntryList();
	}
}

// -----------------------------------------------------------------------------
// Closes the archive editor tab for the archive at [archive_index] in the
// archive manager
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::closeTab(int archive_index) const
{
	auto archive = app::archiveManager().getArchive(archive_index);
	auto ap      = tabForArchive(archive.get());

	if (ap)
		stc_archives_->DeletePage(stc_archives_->GetPageIndex(ap));
}

// -----------------------------------------------------------------------------
// Opens a new texture editor tab for the archive at [archive_index] in the
// archive manager
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::openTextureTab(int archive_index, ArchiveEntry* entry) const
{
	auto archive = app::archiveManager().getArchive(archive_index);

	if (archive)
	{
		// Go through all tabs
		for (size_t a = 0; a < stc_archives_->GetPageCount(); a++)
		{
			// Check page type is "texture"
			if (stc_archives_->GetPage(a)->GetName().CmpNoCase("texture"))
				continue;

			// Check for archive match
			auto txed = dynamic_cast<TextureXEditor*>(stc_archives_->GetPage(a));
			if (txed->archive() == archive.get())
			{
				// Selected archive already has its texture editor open, so show that tab
				stc_archives_->SetSelection(a);
				txed->setSelection(entry);
				return;
			}
		}

		// If tab isn't already open, open a new one
		auto txed = new TextureXEditor(stc_archives_);
		txed->Show(false);
		if (!txed->openArchive(archive.get()))
		{
			delete txed;
			return;
		}

		stc_archives_->AddPage(txed, wxString::Format("TEXTUREx Editor (%s)", archive->filename(false)), true);
		stc_archives_->SetPageBitmap(stc_archives_->GetPageCount() - 1, icons::getIcon(icons::Entry, "texturex"));
		txed->SetName("texture");
		txed->setSelection(entry);
		txed->Show(true);
		// Select the new tab
		for (size_t a = 0; a < stc_archives_->GetPageCount(); a++)
		{
			if (stc_archives_->GetPage(a) == txed)
			{
				stc_archives_->SetSelection(a);
				return;
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Returns the TextureXEditor for the archive at [archive_index], or null if
// none is open for that archive
// -----------------------------------------------------------------------------
TextureXEditor* ArchiveManagerPanel::textureTabForArchive(int archive_index) const
{
	auto archive = app::archiveManager().getArchive(archive_index);

	if (archive)
	{
		// Go through all tabs
		for (size_t a = 0; a < stc_archives_->GetPageCount(); a++)
		{
			// Check page type is "texture"
			if (stc_archives_->GetPage(a)->GetName().CmpNoCase("texture"))
				continue;

			// Check for archive match
			auto txed = dynamic_cast<TextureXEditor*>(stc_archives_->GetPage(a));
			if (txed->archive() == archive.get())
				return txed;
		}
	}

	// No texture editor open for that archive
	return nullptr;
}

// -----------------------------------------------------------------------------
// Closes the texture editor tab for the archive at [archive_index] in the
// archive manager
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::closeTextureTab(int archive_index) const
{
	auto txed = textureTabForArchive(archive_index);
	if (txed)
		stc_archives_->DeletePage(stc_archives_->GetPageIndex(txed));
}

// -----------------------------------------------------------------------------
// Redirects to the separated tab with given entry if exists
// -----------------------------------------------------------------------------
bool ArchiveManagerPanel::redirectToTab(ArchiveEntry* entry) const
{
	for (unsigned a = 0; a < stc_archives_->GetPageCount(); a++)
	{
		// Check page type is "entry"
		if (stc_archives_->GetPage(a)->GetName() != "entry")
			continue;

		// Check for entry match
		auto ep = dynamic_cast<EntryPanel*>(stc_archives_->GetPage(a));
		if (ep->entry() == entry)
		{
			// Already open, switch to tab
			stc_archives_->SetSelection(a);
			return true;
		}
	}
	return false;
}

// -----------------------------------------------------------------------------
// Returns true if [entry] is open in a tab
// -----------------------------------------------------------------------------
bool ArchiveManagerPanel::entryIsOpenInTab(ArchiveEntry* entry) const
{
	for (unsigned a = 0; a < stc_archives_->GetPageCount(); a++)
	{
		// Check page type is "entry"
		if (stc_archives_->GetPage(a)->GetName() != "entry")
			continue;

		// Check for entry match
		auto ep = dynamic_cast<EntryPanel*>(stc_archives_->GetPage(a));
		if (ep->entry() == entry)
			return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Opens the appropriate EntryPanel for [entry] in a new tab
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::openEntryTab(ArchiveEntry* entry) const
{
	// First check if the entry is already open in a tab
	if (redirectToTab(entry))
		return;

	// If the entry is an archive, open it
	if (entry->type()->formatId().substr(0, 8) == "archive_")
	{
		app::archiveManager().openArchive(entry);
		return;
	}

	// Switch to the default entry panel in the archive tab
	auto panel = tabForArchive(entry->parent());
	if (!panel->switchToDefaultEntryPanel())
		return;

	// Create an EntryPanel for the entry
	auto ep = ArchivePanel::createPanelForEntry(entry, stc_archives_);
	ep->openEntry(entry);

	// Don't bother with the default entry panel (it's absolutely useless to open in a tab)
	if (ep->name() == "default")
	{
		delete ep;
		return;
	}

	// Create new tab for the EntryPanel
	stc_archives_->AddPage(ep, wxString::Format("%s/%s", entry->parent()->filename(false), entry->name()), true);
	stc_archives_->SetPageBitmap(
		stc_archives_->GetPageCount() - 1, icons::getIcon(icons::Entry, entry->type()->icon()));
	ep->SetName("entry");
	ep->Show(true);
	ep->addCustomMenu();
	ep->updateToolbar();

	// Select the new tab
	for (size_t a = 0; a < stc_archives_->GetPageCount(); a++)
	{
		if (stc_archives_->GetPage(a) == ep)
		{
			stc_archives_->SetSelection(a);
			return;
		}
	}
}

// -----------------------------------------------------------------------------
// Closes the EntryPanel tab for [entry]
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::closeEntryTab(ArchiveEntry* entry) const
{
	// Go through tabs
	for (unsigned a = 0; a < stc_archives_->GetPageCount(); a++)
	{
		// Check page type is "entry"
		if (stc_archives_->GetPage(a)->GetName() != "entry")
			continue;

		// Check for entry parent archive match
		auto ep = dynamic_cast<EntryPanel*>(stc_archives_->GetPage(a));
		if (ep->entry() == entry)
		{
			// Close tab
			ep->removeCustomMenu();
			stc_archives_->DeletePage(a);
			a--;
		}
	}
}

// -----------------------------------------------------------------------------
// Closes any EntryPanel tabs for entries in [parent]
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::closeEntryTabs(Archive* parent) const
{
	// Check archive was given
	if (!parent)
		return;

	// Go through tabs
	for (unsigned a = 0; a < stc_archives_->GetPageCount(); a++)
	{
		// Check page type is "entry"
		if (stc_archives_->GetPage(a)->GetName() != "entry")
			continue;

		// Check for entry parent archive match
		auto ep = dynamic_cast<EntryPanel*>(stc_archives_->GetPage(a));
		if (ep->entry()->parent() == parent)
		{
			// Close tab
			ep->removeCustomMenu();
			stc_archives_->DeletePage(a);
			a--;
		}
	}
}

// -----------------------------------------------------------------------------
// Opens an archive and initialises the UI for it
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::openFile(const wxString& filename) const
{
	// Show splash screen
	ui::showSplash("Opening Archive...", true);

	// test
	wxStopWatch sw;
	sw.Start();

	// Open the file in the archive manager
	auto new_archive = app::archiveManager().openArchive(filename.ToStdString());

	sw.Pause();
	log::info(wxString::Format("Opening took %d ms", (int)sw.Time()));

	// Hide splash screen
	ui::hideSplash();

	// Check that the archive opened ok
	if (!new_archive)
	{
		// If archive didn't open ok, show error message
		wxMessageBox(wxString::Format("Error opening %s:\n%s", filename, global::error), "Error", wxICON_ERROR);
	}
}

// -----------------------------------------------------------------------------
// Opens each file in a supplied array of filenames
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::openFiles(const wxArrayString& files) const
{
	// Go through each filename in the array
	for (const auto& file : files)
	{
		// Open the archive
		openFile(file);
	}
}

// -----------------------------------------------------------------------------
// Opens a directory as an archive and initialises the UI for it
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::openDirAsArchive(const wxString& dir) const
{
	// Show splash screen
	ui::showSplash("Opening Directory...", true);

	// test
	wxStopWatch sw;
	sw.Start();

	// Open the file in the archive manager
	auto new_archive = app::archiveManager().openDirArchive(dir.ToStdString());

	sw.Pause();
	log::info(wxString::Format("Opening took %d ms", (int)sw.Time()));

	// Hide splash screen
	ui::hideSplash();

	// Check that the archive opened ok
	if (!new_archive)
	{
		// If archive didn't open ok, show error message
		wxMessageBox(wxString::Format("Error opening directory %s:\n%s", dir, global::error), "Error", wxICON_ERROR);
	}
}

// -----------------------------------------------------------------------------
// Performs an undo operation on the currently selected tab.
// Returns true if the tab supports undo, false otherwise
// -----------------------------------------------------------------------------
bool ArchiveManagerPanel::undo() const
{
	// Get current tab page
	auto page_current = currentPanel();

	// Archive panel
	if (S_CMPNOCASE(page_current->GetName(), "archive"))
	{
		dynamic_cast<ArchivePanel*>(page_current)->undo();
		return true;
	}

	// TEXTUREx panel
	else if (S_CMPNOCASE(page_current->GetName(), "texture"))
	{
		dynamic_cast<TextureXEditor*>(page_current)->undo();
		return true;
	}

	// Entry panel
	else if (S_CMPNOCASE(page_current->GetName(), "entry"))
	{
		dynamic_cast<EntryPanel*>(page_current)->undo();
		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Performs an redo operation on the currently selected tab.
// Returns true if the tab supports redo, false otherwise
// -----------------------------------------------------------------------------
bool ArchiveManagerPanel::redo() const
{
	// Get current tab page
	auto page_current = currentPanel();

	// Archive panel
	if (S_CMPNOCASE(page_current->GetName(), "archive"))
	{
		dynamic_cast<ArchivePanel*>(page_current)->redo();
		return true;
	}

	// TEXTUREx panel
	else if (S_CMPNOCASE(page_current->GetName(), "texture"))
	{
		dynamic_cast<TextureXEditor*>(page_current)->redo();
		return true;
	}

	// Entry panel
	else if (S_CMPNOCASE(page_current->GetName(), "entry"))
	{
		dynamic_cast<EntryPanel*>(page_current)->redo();
		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Closes all currently open archives
// -----------------------------------------------------------------------------
bool ArchiveManagerPanel::closeAll()
{
	asked_save_unchanged_ = false;

	while (app::archiveManager().numArchives() > 0)
	{
		if (!closeArchive(app::archiveManager().getArchive(0).get()))
			return false;
	}

	return true;
}

// -----------------------------------------------------------------------------
// Saves all currently open archives
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::saveAll() const
{
	// Go through all open archives
	for (int a = 0; a < app::archiveManager().numArchives(); a++)
	{
		// Get the archive to be saved
		auto archive = app::archiveManager().getArchive(a);

		if (archive->canSave())
		{
			// Save the archive if possible
			if (!archive->save())
			{
				// If there was an error pop up a message box
				wxMessageBox(wxString::Format("Error: %s", global::error), "Error", wxICON_ERROR);
			}
		}
		else
		{
			// If the archive is newly created, do Save As instead

			// Popup file save dialog
			wxString formats  = archive->fileExtensionString();
			wxString filename = wxFileSelector(
				"Save Archive " + archive->filename(false) + " As",
				dir_last,
				"",
				wxEmptyString,
				formats,
				wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

			// Check a filename was selected
			if (!filename.empty())
			{
				// Save the archive
				if (!archive->save(filename.ToStdString()))
				{
					// If there was an error pop up a message box
					wxMessageBox(wxString::Format("Error: %s", global::error), "Error", wxICON_ERROR);
				}

				// Save 'dir_last'
				wxFileName fn(filename);
				dir_last = wxutil::strToView(fn.GetPath(true));
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Checks all open directory archives for changes on the file system in
// background threads
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::checkDirArchives()
{
	if (checked_dir_archive_changes_ || dir_archive_change_action == 0)
		return;

	for (int a = 0; a < app::archiveManager().numArchives(); a++)
	{
		auto archive = app::archiveManager().getArchive(a);
		if (archive->formatId() != "folder")
			continue;

		if (VECTOR_EXISTS(checking_archives_, archive.get()))
			continue;

		log::info(2, "Checking {} for external changes...", archive->filename());
		checking_archives_.push_back(archive.get());
		auto check = new DirArchiveCheck(this, dynamic_cast<DirArchive*>(archive.get()));
		check->Create();
		check->Run();
	}
}

// -----------------------------------------------------------------------------
// Creates a new archive of the given type and opens it in a tab
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::createNewArchive(const wxString& format) const
{
	if (!format.empty())
	{
		Archive* archive = app::archiveManager().newArchive(format.ToStdString()).get();

		if (archive)
			openTab(app::archiveManager().archiveIndex(archive));
	}
	else
	{
		auto* na_dlg = new ui::NewArchiveDialog(maineditor::windowWx());
		if (na_dlg->ShowModal() == wxID_OK)
		{
			if (na_dlg->createdArchive())
				openTab(app::archiveManager().archiveIndex(na_dlg->createdArchive()));
		}
	}
}

// -----------------------------------------------------------------------------
// If there are any unsaved entry changes in [archive]'s ArchivePanel tab,
// saves the changes (or not, depending on user settings)
// -----------------------------------------------------------------------------
bool ArchiveManagerPanel::saveEntryChanges(Archive* archive) const
{
	bool changes = false;

	// Go through tabs
	for (size_t a = 0; a < stc_archives_->GetPageCount(); a++)
	{
		// Check page type is "archive"
		if (!stc_archives_->GetPage(a)->GetName().CmpNoCase("archive"))
		{
			// Check for archive match
			auto ap = dynamic_cast<ArchivePanel*>(stc_archives_->GetPage(a));
			if (ap->archive() == archive)
			{
				// Save entry changes
				if (ap->saveEntryChanges())
					changes = true;
			}
		}

		// Check page type is "entry"
		if (!stc_archives_->GetPage(a)->GetName().CmpNoCase("entry"))
		{
			// Check for entry parent archive match
			auto ep = dynamic_cast<EntryPanel*>(stc_archives_->GetPage(a));
			if (ep->entry()->parent() == archive)
			{
				if (ep->isModified() && autosave_entry_changes > 0)
				{
					// Ask if needed
					if (autosave_entry_changes > 1)
					{
						int result = wxMessageBox(
							wxString::Format("Save changes to entry \"%s\"?", ep->entry()->name()),
							"Unsaved Changes",
							wxYES_NO | wxICON_QUESTION);

						// Don't save if user clicked no
						if (result == wxNO)
							continue;
					}

					// Save entry changes
					if (ep->saveEntry())
						changes = true;
				}
			}
		}
	}

	return changes;
}

// -----------------------------------------------------------------------------
// Saves [archive] to disk, opens a file dialog if necessary
// -----------------------------------------------------------------------------
bool ArchiveManagerPanel::saveArchive(Archive* archive) const
{
	// Check for null pointer
	if (!archive)
		return false;

	// Check for unsaved entry changes
	saveEntryChanges(archive);

	if (archive->canSave())
	{
		// Check if the file has been modified on disk
		if (fileutil::fileModifiedTime(archive->filename()) > archive->fileModifiedTime())
		{
			if (wxMessageBox(
					wxString::Format(
						"The file %s has been modified on disk since the archive was last saved, are you sure you want "
						"to continue with saving?",
						archive->filename(false)),
					"File Modified",
					wxICON_WARNING | wxYES_NO)
				== wxNO)
				return false;
		}

		// Save the archive if possible
		if (!archive->save())
		{
			// If there was an error pop up a message box
			wxMessageBox(wxString::Format("Error: %s", global::error), "Error", wxICON_ERROR);
			return false;
		}

		return true;
	}
	else
		return saveArchiveAs(archive); // If the archive is newly created, do Save As instead
}

// -----------------------------------------------------------------------------
// Saves [archive] to disk under a different filename, opens a file dialog to
// select the new name/path
// -----------------------------------------------------------------------------
bool ArchiveManagerPanel::saveArchiveAs(Archive* archive) const
{
	// Check for null pointer
	if (!archive)
		return false;

	// Check archive type
	if (archive->formatId() == "folder")
		return true; // Can't do save as for folder

	// Check for unsaved entry changes
	saveEntryChanges(archive);

	// Popup file save dialog
	wxString formats  = archive->fileExtensionString();
	wxString filename = wxFileSelector(
		"Save Archive " + archive->filename(false) + " As",
		dir_last,
		"",
		wxEmptyString,
		formats,
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	// Check a filename was selected
	if (!filename.empty())
	{
		// Save the archive
		if (!archive->save(filename.ToStdString()))
		{
			// If there was an error pop up a message box
			wxMessageBox(wxString::Format("Error: %s", global::error), "Error", wxICON_ERROR);
			return false;
		}

		// Save 'dir_last'
		wxFileName fn(filename);
		dir_last = wxutil::strToView(fn.GetPath(true));

		// Add recent file
		app::archiveManager().addRecentFile(filename.ToStdString());
	}
	else
		return false;

	return true;
}

// -----------------------------------------------------------------------------
// Checks for any unsaved changes and prompts the user to save if needed, but
// doesn't actually close the archive
// -----------------------------------------------------------------------------
bool ArchiveManagerPanel::beforeCloseArchive(Archive* archive)
{
	// Check for NULL pointers -- this can happen, for example,
	// with onArchiveTabClose() when closing a texture editor tab.
	if (!archive)
		return false;

	// Check if a map from the archive is currently open
	if (mapeditor::window()->IsShown() && mapeditor::window()->hasMapOpen(archive))
	{
		if (!mapeditor::window()->Close())
			return false;
	}

	// Check for unsaved entry changes
	saveEntryChanges(archive);

	// Check for unsaved texture editor changes
	int  archive_index = app::archiveManager().archiveIndex(archive);
	auto txed          = textureTabForArchive(archive_index);
	if (txed)
	{
		openTextureTab(archive_index);
		if (!txed->close())
			return false; // User cancelled saving texturex changes, don't close
	}

	// If the archive has unsaved changes, prompt to save
	if (archive->isModified() && archive->isWritable())
	{
		asked_save_unchanged_ = true;
		wxMessageDialog md(
			this,
			wxString::Format("Save changes to archive %s?", archive->filename(false)),
			"Unsaved Changes",
			wxYES_NO | wxCANCEL);
		int result = md.ShowModal();
		if (result == wxID_YES)
		{
			// User selected to save
			if (!saveArchive(archive))
				return false; // Unsuccessful save, don't close
		}
		else if (result == wxID_CANCEL)
			return false; // User selected cancel, don't close the archive
	}

	return true;
}

// -----------------------------------------------------------------------------
// Checks for any unsaved changes and prompts the user to save if needed before
// closing [archive]
// -----------------------------------------------------------------------------
bool ArchiveManagerPanel::closeArchive(Archive* archive)
{
	if (!archive)
		return false;

	checked_dir_archive_changes_ = true;
	bool ok                      = beforeCloseArchive(archive) && app::archiveManager().closeArchive(archive);
	checked_dir_archive_changes_ = false;

	return ok;
}

// -----------------------------------------------------------------------------
// Gets a list of indices of all selected archive list items
// -----------------------------------------------------------------------------
vector<int> ArchiveManagerPanel::selectedArchives() const
{
	vector<int> ret;

	// Go through all wad list items
	long item = -1;
	while (true)
	{
		// Get the next item in the list that is selected
		item = list_archives_->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);

		// If -1 then none were selected
		if (item == -1)
			break;

		// Otherwise add the selected item to the vector
		ret.push_back(item);
	}

	return ret;
}

// -----------------------------------------------------------------------------
// Gets a list of indices of all selected recent file list items
// -----------------------------------------------------------------------------
vector<int> ArchiveManagerPanel::selectedFiles() const
{
	vector<int> ret;

	// Go through all wad list items
	long item = -1;
	while (true)
	{
		// Get the next item in the list that is selected
		item = list_recent_->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);

		// If -1 then none were selected
		if (item == -1)
			break;

		// Otherwise add the selected item to the vector
		ret.push_back(item);
	}

	return ret;
}

// -----------------------------------------------------------------------------
// Gets a list of indices of all selected bookmark list items
// -----------------------------------------------------------------------------
vector<int> ArchiveManagerPanel::selectedBookmarks() const
{
	vector<int> ret;

	// Go through all wad list items
	long item = -1;
	while (true)
	{
		// Get the next item in the list that is selected
		item = list_bookmarks_->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);

		// If -1 then none were selected
		if (item == -1)
			break;

		// Otherwise add the selected item to the vector
		ret.push_back(item);
	}

	return ret;
}

// -----------------------------------------------------------------------------
// Saves the currently selected archive(s) in the list
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::saveSelection() const
{
	// Get the list of selected archives
	auto selection = selectedArchives();

	// Don't continue if there are no selected items
	if (selection.empty())
		return;

	// Go through the selection and save
	for (int index : selection)
		saveArchive(app::archiveManager().getArchive(index).get());
}

// -----------------------------------------------------------------------------
// Saves the currently selected archive(s) in the list as new file(s)
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::saveSelectionAs() const
{
	// Get the list of selected archives
	auto selection = selectedArchives();

	// Don't continue if there are no selected items
	if (selection.empty())
		return;

	// Go through the selection and save as
	for (int index : selection)
		saveArchiveAs(app::archiveManager().getArchive(index).get());

	refreshArchiveList();
}

// -----------------------------------------------------------------------------
// Closes the currently selected archive(s) in the list
// -----------------------------------------------------------------------------
bool ArchiveManagerPanel::closeSelection()
{
	// Get the list of selected list items
	auto selection = selectedArchives();

	// Don't continue if there are no selected items
	if (selection.empty())
		return true;

	// Get the list of selected archives
	vector<Archive*> selected_archives;
	for (int index : selection)
		selected_archives.push_back(app::archiveManager().getArchive(index).get());

	// Close all selected archives, starting from the last
	bool all_closed = true;
	for (size_t a = selected_archives.size() - 1; (signed)a >= 0; --a)
	{
		if (!closeArchive(selected_archives[a]))
			all_closed = false;
	}

	return all_closed;
}

// -----------------------------------------------------------------------------
// Open the currently selected archive(s) in the recent file list
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::openSelection() const
{
	// Get the list of selected list items
	auto selection = selectedFiles();

	// Don't continue if there are no selected items
	if (selection.empty())
		return;

	// Get the list of selected archives
	vector<wxString> selected_archives;
	for (int index : selection)
		selected_archives.emplace_back(app::archiveManager().recentFile(index));

	// Open all selected archives
	for (const auto& selected_archive : selected_archives)
		app::archiveManager().openArchive(selected_archive.ToStdString());
}

// -----------------------------------------------------------------------------
// Remove the currently selected archive(s) from the recent file list
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::removeSelection() const
{
	// Get the list of selected list items
	auto selection = selectedFiles();

	// Don't continue if there are no selected items
	if (selection.empty())
		return;

	// Remove selected recent files (starting from the last and going backward,
	// because the list reorders itself whenever an item is removed)
	for (unsigned a = selection.size(); a > 0; --a)
		app::archiveManager().removeRecentFile(app::archiveManager().recentFile(selection[a - 1]));
}

// -----------------------------------------------------------------------------
// Handles the action [id].
// Returns true if the action was handled, false otherwise
// -----------------------------------------------------------------------------
bool ArchiveManagerPanel::handleAction(string_view id)
{
	// We're only interested in "aman_" actions
	if (!strutil::startsWith(id, "aman_"))
		return false;

	// File->New Archive
	if (id == "aman_newarchive")
		createNewArchive("");
	// Start Page: Create Wad Archive (File->New Archive->wad)
	else if (id == "aman_newwad")
		createNewArchive("wad");
	// Start Page: Create Zip Archive (File->New Archive->zip)
	else if (id == "aman_newzip")
		createNewArchive("zip");
	// File->New Map
	else if (id == "aman_newmap")
	{
		if (!mapeditor::chooseMap())
			mapeditor::window()->Show(false);
	}

	// File->Open
	else if (id == "aman_open")
	{
		// Create extensions string
		wxString extensions = app::archiveManager().getArchiveExtensionsString();
		extensions += "|All Files (*.*)|*.*";

		// Open a file browser dialog that allows multiple selection
		// and filters by wad, zip and pk3 file extensions
		wxFileDialog dialog_open(
			this,
			"Choose file(s) to open",
			dir_last,
			wxEmptyString,
			extensions,
			wxFD_OPEN | wxFD_MULTIPLE | wxFD_FILE_MUST_EXIST,
			wxDefaultPosition);

		// Run the dialog & check that the user didn't cancel
		if (dialog_open.ShowModal() == wxID_OK)
		{
			wxBeginBusyCursor();

			// Get an array of selected filenames
			wxArrayString files;
			dialog_open.GetPaths(files);

			// Open them
			openFiles(files);

			wxEndBusyCursor();

			// Save 'dir_last'
			dir_last = wxutil::strToView(dialog_open.GetDirectory());
		}
	}

	// File->Open Directory
	else if (id == "aman_opendir")
	{
		// Open a directory browser dialog
		wxDirDialog dialog_open(
			this, "Select a Directory to open", dir_last, wxDD_DIR_MUST_EXIST | wxDD_NEW_DIR_BUTTON);

		// Run the dialog & check the user didn't cancel
		if (dialog_open.ShowModal() == wxID_OK)
		{
			wxBeginBusyCursor();

			// Open directory
			openDirAsArchive(dialog_open.GetPath());

			wxEndBusyCursor();

			// Save 'dir_last'
			dir_last = wxutil::strToView(dialog_open.GetPath());
		}
	}

	// File->Recent
	else if (id == "aman_recent")
	{
		// Get recent file index
		unsigned index = wx_id_offset_;

		// Open it
		openFile(app::archiveManager().recentFile(index));
	}

	// File->Save
	else if (id == "aman_save")
		saveArchive(currentArchive());

	// File->Save As
	else if (id == "aman_saveas")
		saveArchiveAs(currentArchive());

	// File->Save All
	else if (id == "aman_saveall")
		saveAll();

	// File->Close All
	else if (id == "aman_closeall")
		closeAll();

	// File->Close
	else if (id == "aman_close")
		closeArchive(currentArchive());

	// Archives context menu cannot needs its own functions!
	else if (id == "aman_save_a")
		saveSelection();
	else if (id == "aman_saveas_a")
		saveSelectionAs();
	else if (id == "aman_close_a")
		closeSelection();

	// Recent files context menu
	else if (id == "aman_recent_open")
		openSelection();
	else if (id == "aman_recent_remove")
		removeSelection();

	// Bookmarks context menu
	else if (id == "aman_bookmark_go")
		goToBookmark();
	else if (id == "aman_bookmark_remove")
		deleteSelectedBookmarks();

	// Bookmarks dropdown menu
	else if (id == "aman_bookmark_menu")
		goToBookmark(wx_id_offset_);
	else if (id == "aman_bookmark_removeall")
		app::archiveManager().deleteAllBookmarks();


	// Unknown action
	else
		return false;

	// Action performed, return true
	return true;
}

// -----------------------------------------------------------------------------
// Updates the bookmark list item at <index>
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::updateBookmarkListItem(int index) const
{
	// Only valid indices
	if (index < 0 || (unsigned)index >= app::archiveManager().numBookmarks())
		return;

	auto entry = app::archiveManager().getBookmark(index);
	if (!entry)
		return;

	// Set item name
	list_bookmarks_->setItemText(index, 0, entry->name());
	list_bookmarks_->setItemText(index, 1, entry->parent()->filename());

	// Set item status colour
	using ItemStatus = ListView::ItemStatus;
	if (entry->isLocked())
		list_bookmarks_->setItemStatus(index, ItemStatus::Locked);
	else
		switch (entry->state())
		{
		case ArchiveEntry::State::Unmodified: list_bookmarks_->setItemStatus(index, ItemStatus::Normal); break;
		case ArchiveEntry::State::Modified: list_bookmarks_->setItemStatus(index, ItemStatus::Modified); break;
		case ArchiveEntry::State::New: list_bookmarks_->setItemStatus(index, ItemStatus::New); break;
		default: list_bookmarks_->setItemStatus(index, ItemStatus::Error); break;
		}
}

// -----------------------------------------------------------------------------
// Clears and rebuilds the bookmark list and menu
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::refreshBookmarkList() const
{
	// Get first bookmark menu id
	auto a_bookmark  = SAction::fromId("aman_bookmark_menu");
	int  id_bm_start = a_bookmark->wxId();

	// Clear the list
	list_bookmarks_->ClearAll();

	// Clear menu
	while (!menu_bookmarks_->GetMenuItems().empty())
		menu_bookmarks_->Delete(menu_bookmarks_->GetMenuItems()[0]);

	// Add columns
	list_bookmarks_->InsertColumn(0, "Entry");
	list_bookmarks_->InsertColumn(1, "Archive");

	// Add each bookmark
	list_bookmarks_->enableSizeUpdate(false);
	for (unsigned a = 0; a < app::archiveManager().numBookmarks(); a++)
	{
		list_bookmarks_->addItem(a, wxEmptyString);
		updateBookmarkListItem(a);

		// Add to menu
		if (a < 20)
		{
			// Get path and determine icon
			auto* entry      = app::archiveManager().getBookmark(a);
			auto  entry_path = fmt::format("{}/{}", entry->parent()->filename(false), entry->path(true).substr(1));

			// Create and add menu item
			a_bookmark->addToMenu(menu_bookmarks_, entry_path, entry->type()->icon(), a);
		}
	}

	// Add 'remove all bookmarks' to bookmarks menu
	if (menu_bookmarks_->GetMenuItemCount() > 0)
		menu_bookmarks_->AppendSeparator();
	SAction::fromId("aman_bookmark_removeall")->addToMenu(menu_bookmarks_);

	// Update size
	list_bookmarks_->enableSizeUpdate(true);
	list_bookmarks_->updateSize();
}

// -----------------------------------------------------------------------------
// Deletes selected bookmarks from the list
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::deleteSelectedBookmarks() const
{
	auto selection = selectedBookmarks();

	// Don't continue if there are no selected items
	if (selection.empty())
		return;

	// Clear selection
	list_bookmarks_->clearSelection();

	// Remove bookmarks
	for (int a = selection.size() - 1; a >= 0; a--)
		app::archiveManager().deleteBookmark(selection[a]);
}

// -----------------------------------------------------------------------------
// Open the bookmark in the entry panel
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::goToBookmark(long index) const
{
	// Get first selected bookmark if no index given
	if (index < 0)
	{
		auto selected = list_bookmarks_->selectedItems();
		if (selected.empty())
			return;

		index = selected[0];
	}

	// Get the selected bookmark entry
	auto bookmark = app::archiveManager().getBookmark(index);

	// Check it's valid
	if (!bookmark)
		return;

	// Open its parent archive in a tab
	openTab(bookmark->parent());

	// Get the opened tab (should be an ArchivePanel unless something went wrong)
	auto tab = stc_archives_->GetPage(stc_archives_->GetSelection());

	// Check it's an archive panel
	if (!tab || !(S_CMP(tab->GetName(), "archive")))
		return;

	// Finally, open the entry
	auto ap = dynamic_cast<ArchivePanel*>(tab);
	ap->openEntry(bookmark, true);
	if (bookmark->type() != EntryType::folderType())
		ap->focusOnEntry(bookmark);
}


// -----------------------------------------------------------------------------
//
// ArchiveManagerPanel Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the user selects an archive in the open files list.
// Updates the maps list with any maps found within the selected archive
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::onListArchivesChanged(wxListEvent& e)
{
	// Get the selected archive
	auto selected_archive = app::archiveManager().getArchive(e.GetIndex());

	// Return if selection doesn't exist
	if (!selected_archive)
		return;

	current_maps_ = selected_archive.get();
}

// -----------------------------------------------------------------------------
// Called when the user activates an archive in the list.
// Opens the archive in a new tab, if it isn't already open.
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::onListArchivesActivated(wxListEvent& e)
{
	// Open the archive tab, or create a new tab if it isn't already
	openTab(e.GetIndex());
}

// -----------------------------------------------------------------------------
// Called when the user right clicks an item on the archive list, pops up a
// context menu
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::onListArchivesRightClick(wxListEvent& e)
{
	// Generate context menu
	wxMenu context;
	SAction::fromId("aman_save_a")->addToMenu(&context, true);
	SAction::fromId("aman_saveas_a")->addToMenu(&context, true);
	SAction::fromId("aman_close_a")->addToMenu(&context, true);

	// Pop it up
	PopupMenu(&context);
}

// -----------------------------------------------------------------------------
// Called when the user activates an archive in the list.
// Opens the archive in a new tab, if it isn't already open.
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::onListRecentActivated(wxListEvent& e)
{
	// Open the archive
	openFile(app::archiveManager().recentFile(e.GetIndex()));
	// Refresh the list
	refreshRecentFileList();
}

// -----------------------------------------------------------------------------
// Called when the user right clicks an item on the archive list, pops up a
// context menu
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::onListRecentRightClick(wxListEvent& e)
{
	// Generate context menu
	wxMenu context;
	SAction::fromId("aman_recent_open")->addToMenu(&context, true);
	SAction::fromId("aman_recent_remove")->addToMenu(&context, true);

	// Pop it up
	PopupMenu(&context);
}

// -----------------------------------------------------------------------------
// Called when the user right clicks an item on the bookmark list, pops up a
// context menu
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::onListBookmarksRightClick(wxListEvent& e)
{
	// Generate context menu
	wxMenu context;
	SAction::fromId("aman_bookmark_go")->addToMenu(&context, true);
	SAction::fromId("aman_bookmark_remove")->addToMenu(&context, true);

	// Pop it up
	PopupMenu(&context);
}

// -----------------------------------------------------------------------------
// Called when the current archive tab has changed
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::onArchiveTabChanged(wxAuiNotebookEvent& e)
{
	// Page has changed, update custom menus and toolbars
	int selection = stc_archives_->GetSelection();

	// Remove any current custom menus/toolbars
	theMainWindow->removeAllCustomMenus();
	theMainWindow->removeAllCustomToolBars();
	theMainWindow->enableToolBar("_archive", false);
	theMainWindow->enableToolBar("_entry", false);

	// ArchivePanel
	if (isArchivePanel(selection))
	{
		auto ap = dynamic_cast<ArchivePanel*>(stc_archives_->GetPage(selection));
		ap->currentArea()->updateStatus();
		ap->addMenus();
		ap->refreshPanel();
	}

	// EntryPanel
	if (stc_archives_->GetPage(selection)->GetName() == "entry")
	{
		auto ep = dynamic_cast<EntryPanel*>(stc_archives_->GetPage(selection));
		ep->addCustomMenu();
	}

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the user clicks the close button on an archive tab
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::onArchiveTabClose(wxAuiNotebookEvent& e)
{
	// Get tab that is closing
	int  tabindex = e.GetSelection();
	auto page     = stc_archives_->GetPage(tabindex);

	if (tabindex < 0)
		return;

	// Close the tab's archive if needed
	if (close_archive_with_tab && isArchivePanel(tabindex))
	{
		auto ap      = dynamic_cast<ArchivePanel*>(page);
		auto archive = ap->archive();

		// Close dependant archives first (if any)
		auto deps = app::archiveManager().getDependentArchives(archive);

		// Iterate in reverse order so the deepest-nested is closed first
		for (unsigned a = deps.size(); a > 0; a--)
		{
			if (!beforeCloseArchive(deps[a - 1].get()))
			{
				e.Veto();
				return;
			}
		}

		// Close archive
		if (!beforeCloseArchive(archive))
		{
			e.Veto();
			return;
		}

		pending_closed_archive_ = archive;

		e.Skip();
		return;
	}

	if (isEntryPanel(tabindex))
	{
		auto ep = dynamic_cast<EntryPanel*>(page);
		if (ep->isModified() && autosave_entry_changes > 0)
		{
			// Ask if needed
			if (autosave_entry_changes > 1)
			{
				int result = wxMessageBox(
					wxString::Format("Save changes to entry \"%s\"?", ep->entry()->name()),
					"Unsaved Changes",
					wxYES_NO | wxCANCEL | wxICON_QUESTION);

				// Stop if user clicked cancel
				if (result == wxCANCEL)
				{
					e.Veto();
					return;
				}

				// Don't save if user clicked no
				if (result == wxNO)
				{
					e.Skip();
					return;
				}
			}

			// Save entry changes
			ep->saveEntry();
		}
	}

	// Check for texture editor
	else if (page->GetName() == "texture")
	{
		auto txed = dynamic_cast<TextureXEditor*>(page);
		if (!txed->close())
			e.Veto();
	}

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called after a tab has been closed by clicking on a close button
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::onArchiveTabClosed(wxAuiNotebookEvent& e)
{
	// Actually close the archive the CLOSE event decided to close
	if (pending_closed_archive_)
	{
		app::archiveManager().closeArchive(pending_closed_archive_);
		pending_closed_archive_ = nullptr;
	}

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when a directory archive check thread finishes work.
// Pops up a dialog to apply any changes found (if any)
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::onDirArchiveCheckCompleted(wxThreadEvent& e)
{
	auto change_list = e.GetPayload<DirArchiveChangeList>();

	// Check the archive is still open
	if (app::archiveManager().archiveIndex(change_list.archive) >= 0)
	{
		log::info(2, wxString::Format("Finished checking %s for external changes", change_list.archive->filename()));

		if (!change_list.changes.empty())
		{
			checked_dir_archive_changes_ = true;

			auto archive = dynamic_cast<DirArchive*>(change_list.archive);

			// Auto apply if option set
			if (dir_archive_change_action == 1)
				archive->updateChangedEntries(change_list.changes);

			// Otherwise show change/update dialog
			else
			{
				DirArchiveUpdateDialog dlg(maineditor::windowWx(), archive, change_list.changes);
				dlg.ShowModal();
			}

			checked_dir_archive_changes_ = false;
		}
		else
			log::info(2, "No changes");
	}

	VECTOR_REMOVE(checking_archives_, change_list.archive);
}

void ArchiveManagerPanel::connectSignals()
{
	auto& signals = app::archiveManager().signals();

	// Update the archives list if an archive is added/closed/modified
	signal_connections += signals.archive_added.connect(
		[this](unsigned index)
		{
			list_archives_->addItem(index, wxEmptyString);
			updateOpenListItem(index);
		});
	signal_connections += signals.archive_closed.connect([this](unsigned index) { list_archives_->DeleteItem(index); });
	signal_connections += signals.archive_saved.connect(
		[this](unsigned index)
		{
			updateOpenListItem(index);
			updateArchiveTabTitle(index);
		});
	signal_connections += signals.archive_modified.connect(
		[this](unsigned index)
		{
			updateOpenListItem(index);
			updateArchiveTabTitle(index);
		});

	// When an archive is being closed, close any related tabs
	signal_connections += signals.archive_closing.connect(
		[this](unsigned index)
		{
			closeTextureTab(index);
			closeEntryTabs(app::archiveManager().getArchive(index).get());
			closeTab(index);
		});

	// When an archive is opened, open its tab
	signal_connections += signals.archive_opened.connect([this](int index) { openTab(index); });

	// Refresh recent files list when changed
	signal_connections += signals.recent_files_changed.connect([this]() { refreshRecentFileList(); });

	// Refresh bookmarks list when changed
	signal_connections += signals.bookmark_added.connect([this](ArchiveEntry*) { refreshBookmarkList(); });
	signal_connections += signals.bookmarks_removed.connect([this](const vector<ArchiveEntry*>&)
															{ refreshBookmarkList(); });
}
