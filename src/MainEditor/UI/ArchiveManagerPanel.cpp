
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
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
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveManager.h"
#include "Archive/EntryType/EntryType.h"
#include "Archive/Formats/DirArchiveHandler.h"
#include "ArchivePanel.h"
#include "Database/Database.h"
#include "Database/Tables/ArchiveFile.h"
#include "EntryPanel/EntryPanel.h"
#include "General/SAction.h"
#include "Graphics/Icons.h"
#include "MainEditor/ArchiveOperations.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/MainWindow.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/UI/MapEditorWindow.h"
#include "TextureXEditor/TextureXEditor.h"
#include "UI/Controls/STabCtrl.h"
#include "UI/Dialogs/DirArchiveUpdateDialog.h"
#include "UI/Dialogs/NewArchiveDiaog.h"
#include "UI/Layout.h"
#include "UI/Lists/ArchiveListView.h"
#include "UI/Lists/ListView.h"
#include "UI/UI.h"
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
// -----------------------------------------------------------------------------
EXTERN_CVAR(String, dir_last)
EXTERN_CVAR(Int, autosave_entry_changes)


// -----------------------------------------------------------------------------
//
// DirArchiveCheck Class
//
// -----------------------------------------------------------------------------
wxDEFINE_EVENT(wxEVT_COMMAND_DIRARCHIVECHECK_COMPLETED, wxThreadEvent);
class DirArchiveCheck : public wxThread
{
public:
	struct ChangeList
	{
		Archive*               archive;
		vector<DirEntryChange> changes;
	};

	DirArchiveCheck(wxEvtHandler* handler, Archive* archive);
	~DirArchiveCheck() override = default;

	bool isValid() const { return valid_; }

	ExitCode Entry() override;

private:
	struct EntryInfo
	{
		string entry_path;
		string file_path;
		bool   is_dir;
		time_t file_modified;

		EntryInfo(
			const string& entry_path    = "",
			const string& file_path     = "",
			bool          is_dir        = false,
			time_t        file_modified = 0) :
			entry_path{ entry_path },
			file_path{ file_path },
			is_dir{ is_dir },
			file_modified{ file_modified }
		{
		}
	};

	wxEvtHandler*     handler_;
	string            dir_path_;
	vector<EntryInfo> entry_info_;
	vector<string>    removed_files_;
	ChangeList        change_list_;
	bool              ignore_hidden_ = true;
	bool              valid_         = false;

	void addChange(const DirEntryChange& change);
};

// -----------------------------------------------------------------------------
// DirArchiveCheck class constructor
// -----------------------------------------------------------------------------
DirArchiveCheck::DirArchiveCheck(wxEvtHandler* handler, Archive* archive) :
	handler_{ handler },
	dir_path_{ archive->filename() },
	change_list_{ archive, {} }
{
	if (archive->format() != ArchiveFormat::Dir)
	{
		log::error("DirArchiveCheck requires a directory archive!");
		return;
	}

	// Get Directory archive specific info
	auto format_handler = dynamic_cast<DirArchiveHandler&>(archive->formatHandler());
	removed_files_      = format_handler.removedFiles();
	ignore_hidden_      = format_handler.hiddenFilesIgnored();
	valid_              = true;

	// Get flat entry list
	vector<ArchiveEntry*> entries;
	archive->putEntryTreeAsList(entries);

	// Build entry info list
	for (auto& entry : entries)
	{
		entry_info_.emplace_back(
			entry->path(true),
			entry->exProps().getOr<string>("filePath", ""),
			entry->isFolderType(),
			format_handler.fileModificationTime(entry));
	}
}

// -----------------------------------------------------------------------------
// Register a change to a file, as long as it hasn't been ignored
// -----------------------------------------------------------------------------
void DirArchiveCheck::addChange(const DirEntryChange& change)
{
	if (!dynamic_cast<DirArchiveHandler&>(change_list_.archive->formatHandler()).shouldIgnoreEntryChange(change))
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
	wxDir               dir(wxString::FromUTF8(dir_path_));
	dir.Traverse(traverser, wxEmptyString, wxDIR_FILES | wxDIR_DIRS);

	// Check for deleted files
	for (auto& info : entry_info_)
	{
		auto path = info.file_path;

		// Ignore if not on disk
		if (path.empty())
			continue;

		if (info.is_dir)
		{
			if (!fileutil::dirExists(path))
				addChange(DirEntryChange(DirEntryChange::Action::DeletedDir, path, info.entry_path));
		}
		else
		{
			if (!fileutil::fileExists(path))
				addChange(DirEntryChange(DirEntryChange::Action::DeletedFile, path, info.entry_path));
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

		time_t mod = fileutil::fileModifiedTime(file);

		// No match, added to archive
		if (!found)
			addChange(DirEntryChange(DirEntryChange::Action::AddedFile, file, "", mod));
		// Matched, check modification time
		else if (mod > inf.file_modified)
			addChange(DirEntryChange(DirEntryChange::Action::Updated, file, inf.entry_path, mod));
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
	event->SetPayload<ChangeList>(change_list_);
	wxQueueEvent(handler_, event);

	return nullptr;
}


// -----------------------------------------------------------------------------
//
// WMFileBrowser Class
//
// -----------------------------------------------------------------------------
namespace slade
{
class WMFileBrowser : public wxGenericDirCtrl
{
public:
	ArchiveManagerPanel* parent;

	// Note: wxWidgets 2.9.4 deprecated the wxDIRCTRL_SHOW_FILTERS flag; instead,
	// the filters are always shown if any are defined.
	WMFileBrowser(wxWindow* parent, ArchiveManagerPanel* wm, int id) :
		wxGenericDirCtrl(parent, id, wxString::FromUTF8(wxDirDialogDefaultFolderStr)),
		parent{ wm }
	{
		// Connect a custom event for when an item in the file tree is activated
		auto tree_ctrl = wxGenericDirCtrl::GetTreeCtrl();
		tree_ctrl->Connect(
			tree_ctrl->GetId(), wxEVT_TREE_ITEM_ACTIVATED, wxTreeEventHandler(WMFileBrowser::onItemActivated));
	}

	~WMFileBrowser() override = default;

	// Event called when an item in the tree is activated. Opens a file if one
	// is selected
	void onItemActivated(wxTreeEvent& e)
	{
		// Get related objects
		auto tree    = dynamic_cast<wxTreeCtrl*>(e.GetEventObject());
		auto browser = dynamic_cast<WMFileBrowser*>(tree->GetParent());

		// If the selected item has no children (ie it's a file),
		// open it in the archive manager
		if (!tree->ItemHasChildren(e.GetItem()))
			browser->parent->openFile(browser->GetPath().utf8_string());

		e.Skip();
	}
};
} // namespace slade


// -----------------------------------------------------------------------------
//
// ArchiveManagerPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ArchiveManagerPanel class constructor
// -----------------------------------------------------------------------------
ArchiveManagerPanel::ArchiveManagerPanel(wxWindow* parent, STabCtrl* nb_archives) :
	DockPanel(parent),
	stc_archives_{ nb_archives }
{
	auto lh = ui::LayoutHelper(this);

	// Create main sizer
	auto vbox = new wxBoxSizer(wxVERTICAL);
	SetSizer(vbox);

	// Create/setup tabs
	stc_tabs_ = new TabControl(this, false);
	stc_tabs_->SetInitialSize(wxSize(FromDIP(224), -1));
	vbox->Add(stc_tabs_, lh.sfWithBorder(1).Expand());

	// Open archives tab
	panel_am_ = new wxPanel(stc_tabs_);
	stc_tabs_->AddPage(panel_am_, wxS("Archives"), true);

	// Create/setup archive list
	createArchivesPanel(lh);
	refreshArchiveList();

	// Create/setup recent files list and menu
	menu_recent_ = new wxMenu();
	createRecentPanel(lh);
	refreshRecentFileList();

	// Create/setup bookmarks tab
	auto panel_bm = new wxPanel(stc_tabs_);
	panel_bm->SetSizer(new wxBoxSizer(wxVERTICAL));
	list_bookmarks_     = new ListView(panel_bm, -1);
	auto* bm_image_list = wxutil::createSmallImageList();
	auto  et_icon_list  = EntryType::iconList();
	for (const auto& name : et_icon_list)
	{
		if (icons::iconExists(icons::Entry, name))
			wxutil::addImageListIcon(bm_image_list, icons::Entry, name);
		else
			wxutil::addImageListIcon(bm_image_list, icons::Entry, "default");
	}
	list_bookmarks_->SetImageList(bm_image_list, wxIMAGE_LIST_SMALL);
	menu_bookmarks_ = new wxMenu();
	panel_bm->GetSizer()->Add(list_bookmarks_, lh.sfWithBorder(1).Expand());
	refreshBookmarkList();
	stc_tabs_->AddPage(panel_bm, wxS("Bookmarks"), true);

	// Create/setup file browser tab
	// Used to cause problems, however seems to work much better now than what I can remember,
	// adding a cvar to disable in case it still has issues
	if (am_file_browser_tab)
	{
		auto panel = new wxPanel(stc_tabs_);
		panel->SetSizer(new wxBoxSizer(wxVERTICAL));
		file_browser_ = new WMFileBrowser(panel, this, -1);
		panel->GetSizer()->Add(file_browser_, lh.sfWithBorder(1).Expand());
		stc_tabs_->AddPage(panel, wxS("File Browser"));
	}

	// Set current tab
	stc_tabs_->SetSelection(am_current_tab);

	// Bind events
	list_archives_->Bind(wxEVT_LIST_ITEM_SELECTED, &ArchiveManagerPanel::onListArchivesChanged, this);
	list_archives_->Bind(wxEVT_LIST_ITEM_ACTIVATED, &ArchiveManagerPanel::onListArchivesActivated, this);
	list_archives_->Bind(wxEVT_LIST_ITEM_RIGHT_CLICK, &ArchiveManagerPanel::onListArchivesRightClick, this);
	list_recent_->Bind(wxEVT_LIST_ITEM_ACTIVATED, &ArchiveManagerPanel::onListRecentActivated, this);
	list_recent_->Bind(wxEVT_LIST_ITEM_RIGHT_CLICK, &ArchiveManagerPanel::onListRecentRightClick, this);
	list_bookmarks_->Bind(wxEVT_LIST_ITEM_ACTIVATED, [&](wxListEvent& e) { goToBookmark(e.GetIndex()); });
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
	SetInitialSize(lh.size(256, -1));
}

// -----------------------------------------------------------------------------
// Creates the 'Open Archives' panel
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::createArchivesPanel(const ui::LayoutHelper& lh)
{
	panel_archives_ = new wxPanel(panel_am_, -1);
	auto vbox       = new wxBoxSizer(wxVERTICAL);
	panel_archives_->SetSizer(vbox);
	vbox->Add(new wxStaticText(panel_archives_, -1, wxS("Open Archives:")), wxSizerFlags().Expand());
	list_archives_ = new ui::ArchiveListView(panel_archives_);
	vbox->Add(list_archives_, lh.sfWithSmallBorder(1, wxTOP).Expand());
}

// -----------------------------------------------------------------------------
// Creates the 'Recent Files' panel
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::createRecentPanel(const ui::LayoutHelper& lh)
{
	panel_rf_ = new wxPanel(panel_am_, -1);
	auto vbox = new wxBoxSizer(wxVERTICAL);
	panel_rf_->SetSizer(vbox);
	vbox->Add(new wxStaticText(panel_rf_, -1, wxS("Recent Files:")), wxSizerFlags().Expand());
	list_recent_ = new ui::ArchiveListView(panel_rf_);
	vbox->Add(list_recent_, lh.sfWithSmallBorder(1, wxTOP).Expand());

	// Setup image list
	auto list = wxutil::createSmallImageList();
	wxutil::addImageListIcon(list, icons::Entry, "archive");
	wxutil::addImageListIcon(list, icons::Entry, "wad");
	wxutil::addImageListIcon(list, icons::Entry, "zip");
	wxutil::addImageListIcon(list, icons::Entry, "folder");
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

	auto lh = ui::LayoutHelper(this);
	vbox->Add(panel_archives_, lh.sfWithBorder(1).Expand());
	vbox->Add(panel_rf_, lh.sfWithBorder(1, wxLEFT | wxRIGHT | wxBOTTOM).Expand());
}

// -----------------------------------------------------------------------------
// Layout the panel horizontally
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::layoutHorizontal()
{
	// Layout archives tab horizontally
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	panel_am_->SetSizer(hbox);

	auto lh = ui::LayoutHelper(this);
	hbox->Add(panel_archives_, lh.sfWithBorder(1).Expand());
	hbox->Add(panel_rf_, lh.sfWithBorder(1, wxTOP | wxRIGHT | wxBOTTOM).Expand());
}

// -----------------------------------------------------------------------------
// Clears and rebuilds the recent file list in the menu and the tab
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::refreshRecentFileList()
{
	// Clear the list
	list_recent_->DeleteAllItems();

	// Get first recent file menu id
	auto a_recent        = SAction::fromId("aman_recent");
	int  id_recent_start = a_recent->wxId();

	// Clear menu; needs to do with a count down rather than up
	// otherwise the following elements are not properly removed
	for (unsigned a = menu_recent_->GetMenuItemCount(); a > 0; a--)
		menu_recent_->Destroy(id_recent_start + a - 1);

	// Add each recent archive (same logic as the recent files submenu)
	list_recent_->enableSizeUpdate(false);
	recent_files_ = database::recentFiles();
	for (unsigned a = 0; a < recent_files_.size(); a++)
	{
		list_recent_->append(recent_files_[a]);

		if (a < 8)
		{
			// Get path and determine icon
			auto   fn   = recent_files_[a];
			string icon = "archive";
			if (strutil::endsWith(fn, ".wad"))
				icon = "wad";
			else if (strutil::endsWith(fn, ".zip") || strutil::endsWith(fn, ".pk3") || strutil::endsWith(fn, ".pke"))
				icon = "zip";
			else if (fileutil::dirExists(fn))
				icon = "folder";

			// Create and add menu item
			a_recent->addToMenu(menu_recent_, 0, fn, icon, a);
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
	list_archives_->DeleteAllItems();

	// Add each archive that is opened in the ArchiveManager
	list_archives_->enableSizeUpdate(false);
	for (int a = 0; a < app::archiveManager().numArchives(); a++)
		list_archives_->append(app::archiveManager().getArchive(a).get());

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
		if (isArchiveTab(a))
			dynamic_cast<ArchivePanel*>(tab)->refreshPanel();
	}
}

// -----------------------------------------------------------------------------
// Updates the archive list item at <index>
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::updateOpenListItem(int index) const
{
	list_archives_->setItem(index, app::archiveManager().getArchive(index).get());
}

// -----------------------------------------------------------------------------
// Updates the recent file list item at <index>
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::updateRecentListItem(int index) const
{
	// Get path as wxFileName for processing
	auto          path = recent_files_[index];
	strutil::Path fn(path);

	// Set item name
	list_recent_->setItemText(index, 0, string{ fn.fileName() });
	list_recent_->setItemText(index, 1, string{ fn.path() });

	// Set item icon
	int icon = 0;
	if (strutil::equalCI(fn.extension(), "wad"))
		icon = 1;
	else if (
		strutil::equalCI(fn.extension(), "zip") || strutil::equalCI(fn.extension(), "pk3")
		|| strutil::equalCI(fn.extension(), "pke"))
		icon = 2;
	else if (fileutil::dirExists(path))
		icon = 3;
	list_recent_->SetItemImage(index, icon);
}

// -----------------------------------------------------------------------------
// Updates the title of the tab for the archive at [index]
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
		if (stc_archives_->GetPage(a)->GetName().CmpNoCase(wxS("archive")))
			continue;

		// Check for archive match
		auto ap = dynamic_cast<ArchivePanel*>(stc_archives_->GetPage(a));
		if (ap->archive() == archive.get())
		{
			auto title = archive->filename(false);
			if (archive->isModified())
				title += " *";
			stc_archives_->SetPageText(a, wxString::FromUTF8(title));
			return;
		}
	}
}

// -----------------------------------------------------------------------------
// Updates the title of the tab for [entry]
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::updateEntryTabTitle(const ArchiveEntry* entry) const
{
	for (unsigned i = 0; i < stc_archives_->GetPageCount(); ++i)
	{
		auto* page = stc_archives_->GetPage(i);
		if (!page->GetName().CmpNoCase(wxS("entry")))
		{
			auto* ep = dynamic_cast<EntryPanel*>(page);
			if (ep->entry() == entry)
			{
				auto tab_name = fmt::format("{}/{}", entry->parent()->filename(false), entry->name());
				if (ep->isModified())
					tab_name += " *";

				stc_archives_->SetPageText(i, wxString::FromUTF8(tab_name));
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Checks if the tab at [tab_index] is an ArchivePanel.
// Returns true if it is, false if not
// -----------------------------------------------------------------------------
bool ArchiveManagerPanel::isArchiveTab(int tab_index) const
{
	// Check that tab index is in range
	if (static_cast<unsigned>(tab_index) >= stc_archives_->GetPageCount())
		return false;

	// Check the page's name
	return !stc_archives_->GetPage(tab_index)->GetName().CmpNoCase(wxS("archive")) ? true : false;
}

// -----------------------------------------------------------------------------
// Checks if the tab at [tab_index] is an EntryPanel.
// Returns true if it is, false if not
// -----------------------------------------------------------------------------
bool ArchiveManagerPanel::isEntryTab(int tab_index) const
{
	// Check that tab index is in range
	if (static_cast<unsigned>(tab_index) >= stc_archives_->GetPageCount())
		return false;

	// Check the page's name
	return !stc_archives_->GetPage(tab_index)->GetName().CmpNoCase(wxS("entry")) ? true : false;
}

// -----------------------------------------------------------------------------
// Checks if the tab at [tab_index] is a Texture Editor.
// Returns true if it is, false if not
// -----------------------------------------------------------------------------
bool ArchiveManagerPanel::isTextureEditorTab(int tab_index) const
{
	// Check that tab index is in range
	if (static_cast<unsigned>(tab_index) >= stc_archives_->GetPageCount())
		return false;

	// Check the page's name
	return !stc_archives_->GetPage(tab_index)->GetName().CmpNoCase(wxS("texture")) ? true : false;
}

// -----------------------------------------------------------------------------
// Returns the archive associated with the archive tab at [tab_index] or null
// if the index is invalid or the tab isn't an archive panel
// -----------------------------------------------------------------------------
Archive* ArchiveManagerPanel::archiveForTab(int tab_index) const
{
	// Check the index is valid
	if (tab_index < 0 || static_cast<unsigned>(tab_index) >= stc_archives_->GetPageCount())
		return nullptr;

	// Check the specified tab is actually an archive tab
	if (!isArchiveTab(tab_index))
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
	if (page->GetName() == wxS("archive"))
	{
		auto ap = dynamic_cast<ArchivePanel*>(page);
		return ap->archive();
	}

	// EntryPanel
	else if (page->GetName() == wxS("entry"))
	{
		auto ep = dynamic_cast<EntryPanel*>(page);
		return ep->entry() ? ep->entry()->parent() : nullptr;
	}

	// TextureXEditor
	else if (page->GetName() == wxS("texture"))
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
	if (isEntryTab(selected))
		return dynamic_cast<EntryPanel*>(stc_archives_->GetPage(selected));

	// Archive tab
	if (isArchiveTab(selected))
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
	if (!isArchiveTab(selected))
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
	if (!isArchiveTab(selected))
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
ArchivePanel* ArchiveManagerPanel::tabForArchive(const Archive* archive) const
{
	// Check archive was given
	if (!archive)
		return nullptr;

	// Go through all tabs
	for (size_t a = 0; a < stc_archives_->GetPageCount(); a++)
	{
		// Check page type is "archive"
		if (stc_archives_->GetPage(a)->GetName().CmpNoCase(wxS("archive")))
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
void ArchiveManagerPanel::openTab(const Archive* archive) const
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
		maineditor::window()->Freeze();
		wp = new ArchivePanel(stc_archives_, sp_archive);

		// Determine icon
		string icon = "archive";
		if (archive->format() == ArchiveFormat::Wad)
			icon = "wad";
		else if (archive->format() == ArchiveFormat::Zip)
			icon = "zip";
		else if (archive->format() == ArchiveFormat::Dir)
			icon = "folder";

		wp->SetName(wxS("archive"));
		stc_archives_->AddPage(wp, wxString::FromUTF8(archive->filename(false)), false);
		stc_archives_->SetSelection(stc_archives_->GetPageCount() - 1);
		stc_archives_->SetPageBitmap(stc_archives_->GetPageCount() - 1, icons::getIcon(icons::Entry, icon));
		wp->addMenus();
		wp->Show(true);
		maineditor::window()->Thaw();
		wp->refreshPanel();
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
			if (stc_archives_->GetPage(a)->GetName().CmpNoCase(wxS("texture")))
				continue;

			// Check for archive match
			auto txed = dynamic_cast<TextureXEditor*>(stc_archives_->GetPage(a));
			if (txed->archive() == archive.get())
			{
				// Selected archive already has its texture editor open, so show that tab
				stc_archives_->SetSelection(a);
				if (entry && !txed->setSelection(entry))
				{
					// Texture entry isn't open in the editor, open it
					txed->openEntry(entry);
					txed->setSelection(entry);
				}
				return;
			}
		}

		// If tab isn't already open, open a new one
		maineditor::window()->Freeze();
		auto txed = new TextureXEditor(stc_archives_);
		txed->Show(false);
		bool ok = false;
		if (entry)
			ok = txed->openEntry(entry);
		else
			ok = txed->openArchive(archive.get());
		if (!ok)
		{
			delete txed;
			maineditor::window()->Thaw();
			return;
		}

		stc_archives_->AddPage(txed, WX_FMT("TEXTUREx Editor ({})", archive->filename(false)), true);
		stc_archives_->SetPageBitmap(stc_archives_->GetPageCount() - 1, icons::getIcon(icons::Entry, "texturex"));
		txed->setSelection(entry);
		txed->Show(true);
		// Select the new tab
		for (size_t a = 0; a < stc_archives_->GetPageCount(); a++)
		{
			if (stc_archives_->GetPage(a) == txed)
			{
				stc_archives_->SetSelection(a);
				maineditor::window()->Thaw();
				return;
			}
		}

		maineditor::window()->Thaw(); // Shouldn't get to this line but putting this here just in case
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
			if (stc_archives_->GetPage(a)->GetName().CmpNoCase(wxS("texture")))
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
bool ArchiveManagerPanel::redirectToTab(const ArchiveEntry* entry) const
{
	for (unsigned a = 0; a < stc_archives_->GetPageCount(); a++)
	{
		// Check page type is "entry"
		if (stc_archives_->GetPage(a)->GetName() != wxS("entry"))
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
bool ArchiveManagerPanel::entryIsOpenInTab(const ArchiveEntry* entry) const
{
	for (unsigned a = 0; a < stc_archives_->GetPageCount(); a++)
	{
		// Check page type is "entry"
		if (stc_archives_->GetPage(a)->GetName() != wxS("entry"))
			continue;

		// Check for entry match
		auto ep = dynamic_cast<EntryPanel*>(stc_archives_->GetPage(a));
		if (ep->entry() == entry)
			return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Closes the currently selected tab
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::closeCurrentTab()
{
	auto index = stc_archives_->GetSelection();
	if (prepareCloseTab(index))
	{
		stc_archives_->DeletePage(index);

		if (pending_closed_archive_)
		{
			app::archiveManager().closeArchive(pending_closed_archive_);
			pending_closed_archive_ = nullptr;
		}
	}
}

// -----------------------------------------------------------------------------
// Saves any changes in the currently selected tab
// -----------------------------------------------------------------------------
bool ArchiveManagerPanel::saveCurrentTab() const
{
	auto  index = stc_archives_->GetSelection();
	auto* tab   = stc_archives_->GetPage(index);

	// Archive Tab
	if (isArchiveTab(index))
	{
		auto* archive_panel = dynamic_cast<ArchivePanel*>(tab);
		return saveArchive(archive_panel->archive());
	}

	// Entry Tab
	if (isEntryTab(index))
	{
		auto* entry_panel = dynamic_cast<EntryPanel*>(tab);
		return entry_panel->saveEntry();
	}

	// Texture Editor Tab
	if (isTextureEditorTab(index))
	{
		auto* texture_editor = dynamic_cast<TextureXEditor*>(tab);
		texture_editor->saveChanges();
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
	maineditor::window()->Freeze();
	auto ep = ArchivePanel::createPanelForEntry(entry, stc_archives_);
	ep->addBorderPadding();
	ep->openEntry(entry);

	// Don't bother with the default entry panel (it's absolutely useless to open in a tab)
	if (ep->name() == "default")
	{
		delete ep;
		maineditor::window()->Thaw();
		return;
	}

	// Create new tab for the EntryPanel
	stc_archives_->AddPage(ep, WX_FMT("{}/{}", entry->parent()->filename(false), entry->name()), true);
	stc_archives_->SetPageBitmap(
		stc_archives_->GetPageCount() - 1, icons::getIcon(icons::Entry, entry->type()->icon()));
	ep->SetName(wxS("entry"));
	ep->Show(true);
	ep->addCustomMenu(true);
	maineditor::window()->Thaw();

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
void ArchiveManagerPanel::closeEntryTab(const ArchiveEntry* entry) const
{
	// Go through tabs
	for (unsigned a = 0; a < stc_archives_->GetPageCount(); a++)
	{
		// Check page type is "entry"
		if (stc_archives_->GetPage(a)->GetName() != wxS("entry"))
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
void ArchiveManagerPanel::closeEntryTabs(const Archive* parent) const
{
	// Check archive was given
	if (!parent)
		return;

	// Go through tabs
	for (unsigned a = 0; a < stc_archives_->GetPageCount(); a++)
	{
		// Check page type is "entry"
		if (stc_archives_->GetPage(a)->GetName() != wxS("entry"))
			continue;

		// Check for entry parent archive match
		auto ep = dynamic_cast<EntryPanel*>(stc_archives_->GetPage(a));
		if (ep->entry() && ep->entry()->parent() == parent)
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
void ArchiveManagerPanel::openFile(const string& filename) const
{
	// Show splash screen
	ui::showSplash("Opening Archive...", true, maineditor::windowWx());

	// test
	wxStopWatch sw;
	sw.Start();

	// Open the file in the archive manager
	auto new_archive = app::archiveManager().openArchive(filename);

	sw.Pause();
	log::info("Opening took {} ms", (int)sw.Time());

	// Hide splash screen
	ui::hideSplash();

	// Check that the archive opened ok
	if (!new_archive)
	{
		// If archive didn't open ok, show error message
		wxMessageBox(WX_FMT("Error opening {}:\n{}", filename, global::error), wxS("Error"), wxICON_ERROR);
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
		openFile(file.utf8_string());
	}
}

// -----------------------------------------------------------------------------
// Opens a directory as an archive and initialises the UI for it
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::openDirAsArchive(string_view dir) const
{
	// Show splash screen
	ui::showSplash("Opening Directory...", true, maineditor::windowWx());

	// test
	wxStopWatch sw;
	sw.Start();

	// Open the file in the archive manager
	auto new_archive = app::archiveManager().openDirArchive(dir);

	sw.Pause();
	log::info("Opening took {} ms", (int)sw.Time());

	// Hide splash screen
	ui::hideSplash();

	// Check that the archive opened ok
	if (!new_archive)
	{
		// If archive didn't open ok, show error message
		wxMessageBox(WX_FMT("Error opening directory {}:\n{}", dir, global::error), wxS("Error"), wxICON_ERROR);
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
	if (S_CMPNOCASE(page_current->GetName(), wxS("archive")))
	{
		dynamic_cast<ArchivePanel*>(page_current)->undo();
		return true;
	}

	// TEXTUREx panel
	else if (S_CMPNOCASE(page_current->GetName(), wxS("texture")))
	{
		dynamic_cast<TextureXEditor*>(page_current)->undo();
		return true;
	}

	// Entry panel
	else if (S_CMPNOCASE(page_current->GetName(), wxS("entry")))
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
	if (S_CMPNOCASE(page_current->GetName(), wxS("archive")))
	{
		dynamic_cast<ArchivePanel*>(page_current)->redo();
		return true;
	}

	// TEXTUREx panel
	else if (S_CMPNOCASE(page_current->GetName(), wxS("texture")))
	{
		dynamic_cast<TextureXEditor*>(page_current)->redo();
		return true;
	}

	// Entry panel
	else if (S_CMPNOCASE(page_current->GetName(), wxS("entry")))
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
				wxMessageBox(WX_FMT("Error: {}", global::error), wxS("Error"), wxICON_ERROR);
			}
		}
		else
		{
			// If the archive is newly created, do Save As instead

			// Popup file save dialog
			wxString formats  = wxString::FromUTF8(archive->fileExtensionString());
			wxString filename = wxFileSelector(
				wxString::FromUTF8("Save Archive " + archive->filename(false) + " As"),
				dir_last,
				wxEmptyString,
				wxEmptyString,
				formats,
				wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

			// Check a filename was selected
			if (!filename.empty())
			{
				// Save the archive
				if (!archive->save(filename.utf8_string()))
				{
					// If there was an error pop up a message box
					wxMessageBox(WX_FMT("Error: {}", global::error), wxS("Error"), wxICON_ERROR);
				}

				// Save 'dir_last'
				wxFileName fn(filename);
				dir_last = fn.GetPath(true).utf8_string();
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
		if (archive->format() != ArchiveFormat::Dir)
			continue;

		if (VECTOR_EXISTS(checking_archives_, archive.get()))
			continue;

		log::info(2, "Checking {} for external changes...", archive->filename());
		checking_archives_.push_back(archive.get());
		auto check = new DirArchiveCheck(this, archive.get());
		if (check->isValid())
		{
			check->Create();
			check->Run();
		}
		else
			delete check;
	}
}

// -----------------------------------------------------------------------------
// Creates a new archive of the given type and opens it in a tab
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::createNewArchive(const string& format) const
{
	if (!format.empty())
	{
		Archive* archive = app::archiveManager().newArchive(format).get();

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
bool ArchiveManagerPanel::saveEntryChanges(const Archive* archive) const
{
	bool changes = false;

	// Go through tabs
	for (size_t a = 0; a < stc_archives_->GetPageCount(); a++)
	{
		// Check page type is "archive"
		if (!stc_archives_->GetPage(a)->GetName().CmpNoCase(wxS("archive")))
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
		if (!stc_archives_->GetPage(a)->GetName().CmpNoCase(wxS("entry")))
		{
			// Check for entry parent archive match
			auto ep = dynamic_cast<EntryPanel*>(stc_archives_->GetPage(a));
			if (ep->entry() && ep->entry()->parent() == archive)
			{
				if (ep->isModified() && autosave_entry_changes > 0)
				{
					// Ask if needed
					if (autosave_entry_changes > 1)
					{
						int result = wxMessageBox(
							WX_FMT("Save changes to entry \"{}\"?", ep->entry()->name()),
							wxS("Unsaved Changes"),
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

	// Save if we can
	if (archive->canSave())
		return archiveoperations::save(*archive);
	else
		return saveArchiveAs(archive); // If the archive is newly created, do Save As instead
}

// -----------------------------------------------------------------------------
// Saves [archive] to disk under a different filename, opens a file dialog to
// select the new name/path.
// Returns false on error or if the dialog was cancelled, true otherwise
// -----------------------------------------------------------------------------
bool ArchiveManagerPanel::saveArchiveAs(Archive* archive) const
{
	// Check for null pointer
	if (!archive)
		return false;

	// Check archive type
	if (archive->format() == ArchiveFormat::Dir)
		return true; // Can't do save as for folder

	// Check for unsaved entry changes
	saveEntryChanges(archive);

	// Do Save As
	return archiveoperations::saveAs(*archive);
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
			WX_FMT("Save changes to archive {}?", archive->filename(false)),
			wxS("Unsaved Changes"),
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
	for (size_t a = selected_archives.size() - 1; static_cast<signed>(a) >= 0; --a)
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
	vector<string> selected_archives;
	for (int index : selection)
		selected_archives.emplace_back(recent_files_[index]);

	// Open all selected archives
	for (const auto& selected_archive : selected_archives)
		app::archiveManager().openArchive(selected_archive);
}

// -----------------------------------------------------------------------------
// Remove the currently selected archive(s) from the recent file list
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::removeSelection()
{
	// Get the list of selected list items
	auto selection = selectedFiles();

	// Don't continue if there are no selected items
	if (selection.empty())
		return;

	// Don't want to trigger this signal multiple times
	database::signals().archive_file_updated.block();

	// Reset last opened times for each selected recent file
	for (unsigned a = 0; a < selection.size(); ++a)
	{
		auto path  = recent_files_[selection[a]];
		auto db_id = database::archiveFileId(path);
		database::setArchiveFileLastOpened(db_id, 0);
	}

	// Signal change to refresh the list
	database::signals().archive_file_updated.unblock();
	database::signals().archive_file_updated();
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
		auto extensions = app::archiveManager().getArchiveExtensionsString();
		extensions += "|All Files (*.*)|*";

		// Open a file browser dialog that allows multiple selection
		// and filters by wad, zip and pk3 file extensions
		wxFileDialog dialog_open(
			this,
			wxS("Choose file(s) to open"),
			dir_last,
			wxEmptyString,
			wxString::FromUTF8(extensions),
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
			dir_last = dialog_open.GetDirectory().utf8_string();
		}
	}

	// File->Open Directory
	else if (id == "aman_opendir")
	{
		// Open a directory browser dialog
		wxDirDialog dialog_open(
			this, wxS("Select a Directory to open"), dir_last, wxDD_DIR_MUST_EXIST | wxDD_NEW_DIR_BUTTON);

		// Run the dialog & check the user didn't cancel
		if (dialog_open.ShowModal() == wxID_OK)
		{
			wxBeginBusyCursor();

			// Open directory
			openDirAsArchive(dialog_open.GetPath().utf8_string());

			wxEndBusyCursor();

			// Save 'dir_last'
			dir_last = dialog_open.GetPath().utf8_string();
		}
	}

	// File->Recent
	else if (id == "aman_recent")
	{
		// Get recent file index
		unsigned index = wxIdOffset();

		// Open it
		openFile(recent_files_[index]);
	}

	// File->Save
	else if (id == "aman_save")
		saveCurrentTab();

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
		closeCurrentTab();

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
		goToBookmark(wxIdOffset());
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
	if (index < 0 || static_cast<unsigned>(index) >= app::archiveManager().numBookmarks())
		return;

	auto entry = app::archiveManager().getBookmark(index);
	if (!entry)
		return;

	// Set item name
	list_bookmarks_->SetItemImage(index, entry->type()->index());
	list_bookmarks_->setItemText(index, 0, entry->name());
	list_bookmarks_->setItemText(index, 1, entry->parent()->filename(false));

	// Set item status colour
	using ItemStatus = ListView::ItemStatus;
	if (entry->isLocked())
		list_bookmarks_->setItemStatus(index, ItemStatus::Locked);
	else
		switch (entry->state())
		{
		case EntryState::Unmodified: list_bookmarks_->setItemStatus(index, ItemStatus::Normal); break;
		case EntryState::Modified:   list_bookmarks_->setItemStatus(index, ItemStatus::Modified); break;
		case EntryState::New:        list_bookmarks_->setItemStatus(index, ItemStatus::New); break;
		default:                     list_bookmarks_->setItemStatus(index, ItemStatus::Error); break;
		}
}

// -----------------------------------------------------------------------------
// Clears and rebuilds the bookmark list and menu
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::refreshBookmarkList() const
{
	// Get first bookmark menu id
	auto a_bookmark = SAction::fromId("aman_bookmark_menu");

	// Clear the list
	list_bookmarks_->ClearAll();

	// Clear menu
	while (!menu_bookmarks_->GetMenuItems().empty())
		menu_bookmarks_->Delete(menu_bookmarks_->GetMenuItems()[0]);

	// Add columns
	list_bookmarks_->InsertColumn(0, wxS("Entry"));
	list_bookmarks_->InsertColumn(1, wxS("Archive"));

	// Add each bookmark
	list_bookmarks_->enableSizeUpdate(false);
	for (unsigned a = 0; a < app::archiveManager().numBookmarks(); a++)
	{
		list_bookmarks_->addItem(a, "");
		updateBookmarkListItem(a);

		// Add to menu
		if (a < 20)
		{
			// Get path and determine icon
			auto* entry      = app::archiveManager().getBookmark(a);
			auto  entry_path = fmt::format("{}/{}", entry->parent()->filename(false), entry->path(true).substr(1));

			// Create and add menu item
			a_bookmark->addToMenu(menu_bookmarks_, 0, entry_path, entry->type()->icon(), a);
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
	if (!tab || !(S_CMP(tab->GetName(), wxS("archive"))))
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

// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppParameterMayBeConstPtrOrRef

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
	openFile(recent_files_[e.GetIndex()]);
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
	theMainWindow->Freeze();
	theMainWindow->removeAllCustomMenus();
	theMainWindow->enableToolBarGroup("Archive", false);

	// ArchivePanel
	if (isArchiveTab(selection))
	{
		auto ap = dynamic_cast<ArchivePanel*>(stc_archives_->GetPage(selection));
		ap->currentArea()->updateStatus();
		ap->addMenus();
		ap->refreshPanel();
	}

	// EntryPanel
	if (stc_archives_->GetPage(selection)->GetName() == wxS("entry"))
	{
		auto ep = dynamic_cast<EntryPanel*>(stc_archives_->GetPage(selection));
		ep->addCustomMenu(true);
	}

	// TextureXEditor
	if (isTextureEditorTab(selection))
	{
		auto te = dynamic_cast<TextureXEditor*>(stc_archives_->GetPage(selection));
		te->updateMenuStatus();
	}

	theMainWindow->Thaw();

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the user clicks the close button on a tab
// -----------------------------------------------------------------------------
void ArchiveManagerPanel::onArchiveTabClose(wxAuiNotebookEvent& e)
{
	// Veto the event if the tab shouldn't be closed
	if (prepareCloseTab(e.GetSelection()))
		e.Skip();
	else
		e.Veto();
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
	auto change_list = e.GetPayload<DirArchiveCheck::ChangeList>();

	// Check the archive is still open
	if (app::archiveManager().archiveIndex(change_list.archive) >= 0)
	{
		log::info(2, "Finished checking {} for external changes", change_list.archive->filename());

		if (!change_list.changes.empty())
		{
			checked_dir_archive_changes_ = true;

			auto format_handler = dynamic_cast<DirArchiveHandler&>(change_list.archive->formatHandler());

			// Auto apply if option set
			if (dir_archive_change_action == 1)
				format_handler.updateChangedEntries(*change_list.archive, change_list.changes);

			// Otherwise show change/update dialog
			else
			{
				// Show on top of the focused top-level window
				// (There's a wxGetActiveWindow, but it doesn't work on Mac.)
				wxWindow* focused = wxWindow::FindFocus();
				if (focused)
					focused = wxGetTopLevelParent(focused);
				if (!focused)
					focused = maineditor::windowWx();
				DirArchiveUpdateDialog dlg(focused, change_list.archive, change_list.changes);
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
		[this](unsigned index) { list_archives_->insert(index, app::archiveManager().getArchive(index).get()); });
	signal_connections += signals.archive_closed.connect([this](unsigned index) { list_archives_->DeleteItem(index); });
	signal_connections += signals.archive_saved.connect(
		[this](unsigned index)
		{
			updateOpenListItem(index);
			updateArchiveTabTitle(index);
		});
	signal_connections += signals.archive_modified.connect(
		[this](unsigned index, bool)
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
	signal_connections += database::signals().archive_file_updated.connect([this] { refreshRecentFileList(); });

	// Refresh bookmarks list when changed
	signal_connections += signals.bookmark_added.connect([this](ArchiveEntry*) { refreshBookmarkList(); });
	signal_connections += signals.bookmarks_removed.connect([this](const vector<ArchiveEntry*>&)
															{ refreshBookmarkList(); });
}

// -----------------------------------------------------------------------------
// Checks if the tab at [index] can be safely closed, handling anything that
// needs to be handled (unsaved changes etc.)
// Returns true if the tab can be closed, false otherwise
// -----------------------------------------------------------------------------
bool ArchiveManagerPanel::prepareCloseTab(int index)
{
	auto page = stc_archives_->GetPage(index);
	if (!page)
		return false;

	// Close the tab's archive if needed
	if (close_archive_with_tab && isArchiveTab(index))
	{
		auto ap      = dynamic_cast<ArchivePanel*>(page);
		auto archive = ap->archive();

		// Close dependant archives first (if any)
		auto deps = app::archiveManager().getDependentArchives(archive);

		// Iterate in reverse order so the deepest-nested is closed first
		for (unsigned a = deps.size(); a > 0; a--)
		{
			if (!beforeCloseArchive(deps[a - 1].get()))
				return false;
		}

		// Close archive
		if (!beforeCloseArchive(archive))
			return false;

		pending_closed_archive_ = archive;

		return true;
	}

	if (isEntryTab(index))
	{
		auto ep = dynamic_cast<EntryPanel*>(page);
		if (ep->isModified() && autosave_entry_changes > 0)
		{
			// Ask if needed
			if (autosave_entry_changes > 1)
			{
				int result = wxMessageBox(
					WX_FMT("Save changes to entry \"{}\"?", ep->entry()->name()),
					wxS("Unsaved Changes"),
					wxYES_NO | wxCANCEL | wxICON_QUESTION);

				// Stop if user clicked cancel
				if (result == wxCANCEL)
					return false;

				// Don't save if user clicked no
				if (result == wxNO)
					return true;
			}

			// Save entry changes
			ep->saveEntry();
		}
	}

	// Check for texture editor
	else if (page->GetName() == wxS("texture"))
	{
		auto txed = dynamic_cast<TextureXEditor*>(page);
		if (!txed->close())
			return false;
	}

	return true;
}
