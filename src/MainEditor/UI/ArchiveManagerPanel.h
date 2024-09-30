#pragma once

#include "Archive/Formats/DirArchive.h"
#include "General/SAction.h"
#include "General/Sigslot.h"
#include "UI/Controls/DockPanel.h"
#include "UI/Lists/ListView.h"

wxDECLARE_EVENT(wxEVT_COMMAND_DIRARCHIVECHECK_COMPLETED, wxThreadEvent);

namespace slade
{
class ArchiveManagerPanel;
class ArchivePanel;
class Archive;
class STabCtrl;
class TextureXEditor;
class EntryPanel;

struct DirArchiveChangeList
{
	Archive*               archive;
	vector<DirEntryChange> changes;
};

class DirArchiveCheck : public wxThread
{
public:
	DirArchiveCheck(wxEvtHandler* handler, DirArchive* archive);
	~DirArchiveCheck() override = default;

	ExitCode Entry() override;

private:
	struct EntryInfo
	{
		wxString entry_path;
		wxString file_path;
		bool     is_dir;
		time_t   file_modified;

		EntryInfo(
			const wxString& entry_path    = "",
			const wxString& file_path     = "",
			bool            is_dir        = false,
			time_t          file_modified = 0) :
			entry_path{ entry_path },
			file_path{ file_path },
			is_dir{ is_dir },
			file_modified{ file_modified }
		{
		}
	};

	wxEvtHandler*        handler_;
	wxString             dir_path_;
	vector<EntryInfo>    entry_info_;
	vector<string>       removed_files_;
	DirArchiveChangeList change_list_;
	bool                 ignore_hidden_ = true;

	void addChange(DirEntryChange change);
};

class WMFileBrowser : public wxGenericDirCtrl
{
public:
	ArchiveManagerPanel* parent;

	WMFileBrowser(wxWindow* parent, ArchiveManagerPanel* wm, int id);
	~WMFileBrowser() override = default;

	void onItemActivated(wxTreeEvent& e);
};

class ArchiveManagerPanel : public DockPanel, SActionHandler
{
public:
	ArchiveManagerPanel(wxWindow* parent, STabCtrl* nb_archives);
	~ArchiveManagerPanel() override = default;

	wxMenu* recentFilesMenu() const { return menu_recent_; }
	wxMenu* bookmarksMenu() const { return menu_bookmarks_; }

	// DockPanel layout
	void createArchivesPanel();
	void createRecentPanel();
	void layoutNormal() override;
	void layoutHorizontal() override;

	void        disableArchiveListUpdate() const;
	void        refreshArchiveList() const;
	void        refreshRecentFileList() const;
	void        refreshBookmarkList() const;
	void        refreshAllTabs() const;
	void        updateOpenListItem(int index) const;
	void        updateRecentListItem(int index) const;
	void        updateBookmarkListItem(int index) const;
	void        updateArchiveTabTitle(int index) const;
	void        updateEntryTabTitle(ArchiveEntry* entry) const;
	bool        isArchiveTab(int tab_index) const;
	bool        isEntryTab(int tab_index) const;
	bool        isTextureEditorTab(int tab_index) const;
	Archive*    archiveForTab(int tab_index) const;
	int         currentTabIndex() const;
	Archive*    currentArchive() const;
	wxWindow*   currentPanel() const;
	EntryPanel* currentArea() const;
	bool        askedSaveUnchanged() const { return asked_save_unchanged_; }

	ArchiveEntry*         currentEntry() const;
	vector<ArchiveEntry*> currentEntrySelection() const;

	void            openTab(int archive_index) const;
	ArchivePanel*   tabForArchive(Archive* archive) const;
	void            openTab(Archive* archive) const;
	void            closeTab(int archive_index) const;
	void            openTextureTab(int archive_index, ArchiveEntry* entry = nullptr) const;
	TextureXEditor* textureTabForArchive(int archive_index) const;
	void            closeTextureTab(int archive_index) const;
	void            openEntryTab(ArchiveEntry* entry) const;
	void            closeEntryTab(ArchiveEntry* entry) const;
	void            closeEntryTabs(Archive* parent) const;
	void            openFile(const wxString& filename) const;
	void            openFiles(const wxArrayString& files) const;
	void            openDirAsArchive(const wxString& dir) const;
	bool            redirectToTab(ArchiveEntry* entry) const;
	bool            entryIsOpenInTab(ArchiveEntry* entry) const;
	void            closeCurrentTab();
	bool            saveCurrentTab() const;

	// General actions
	bool undo() const;
	bool redo() const;

	// Single archive actions
	bool saveEntryChanges(Archive* archive) const;
	bool saveArchive(Archive* archive) const;
	bool saveArchiveAs(Archive* archive) const;
	bool beforeCloseArchive(Archive* archive);
	bool closeArchive(Archive* archive);

	void createNewArchive(const wxString& format) const;
	bool closeAll();
	void saveAll() const;
	void checkDirArchives();

	// Selected archives in the lists
	void saveSelection() const;
	void saveSelectionAs() const;
	bool closeSelection();
	void openSelection() const;
	void removeSelection() const;

	// Bookmark functions
	void deleteSelectedBookmarks() const;
	void goToBookmark(long index = -1) const;

	// SAction handler
	bool handleAction(string_view id) override;

	vector<int> selectedArchives() const;
	vector<int> selectedBookmarks() const;
	vector<int> selectedFiles() const;

	// Event handlers
	void onListArchivesChanged(wxListEvent& e);
	void onListArchivesActivated(wxListEvent& e);
	void onListArchivesRightClick(wxListEvent& e);
	void onListRecentActivated(wxListEvent& e);
	void onListRecentRightClick(wxListEvent& e);
	void onListBookmarksRightClick(wxListEvent& e);
	void onArchiveTabChanged(wxAuiNotebookEvent& e);
	void onArchiveTabClose(wxAuiNotebookEvent& e);
	void onArchiveTabClosed(wxAuiNotebookEvent& e);
	void onDirArchiveCheckCompleted(wxThreadEvent& e);

private:
	STabCtrl*        stc_tabs_                    = nullptr;
	STabCtrl*        stc_archives_                = nullptr;
	wxPanel*         panel_am_                    = nullptr;
	wxPanel*         panel_archives_              = nullptr;
	wxPanel*         panel_rf_                    = nullptr;
	ListView*        list_archives_               = nullptr;
	ListView*        list_recent_                 = nullptr;
	ListView*        list_bookmarks_              = nullptr;
	WMFileBrowser*   file_browser_                = nullptr;
	wxMenu*          menu_recent_                 = nullptr;
	wxMenu*          menu_bookmarks_              = nullptr;
	Archive*         current_maps_                = nullptr;
	Archive*         pending_closed_archive_      = nullptr;
	bool             asked_save_unchanged_        = false;
	bool             checked_dir_archive_changes_ = false;
	vector<Archive*> checking_archives_;

	// Signal connections
	ScopedConnectionList signal_connections;

	void connectSignals();
	bool prepareCloseTab(int index);
};
} // namespace slade
