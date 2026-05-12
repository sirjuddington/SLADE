#pragma once

#include "General/SActionHandler.h"
#include "General/Sigslot.h"
#include "UI/Controls/DockPanel.h"
#include "UI/Controls/STabCtrl.h"

namespace slade
{
namespace ui
{
	class ArchiveListView;
	struct LayoutHelper;
} // namespace ui

class ArchiveManagerPanel;
class ArchivePanel;
class EntryPanel;
class ListView;
class TextureXEditor;
class WMFileBrowser;
struct DirEntryChange;

class ArchiveManagerPanel : public DockPanel, SActionHandler
{
public:
	ArchiveManagerPanel(wxWindow* parent, STabCtrl* nb_archives);
	~ArchiveManagerPanel() override = default;

	wxMenu* recentFilesMenu() const { return menu_recent_; }
	wxMenu* bookmarksMenu() const { return menu_bookmarks_; }

	// DockPanel layout
	void createArchivesPanel(const ui::LayoutHelper& lh);
	void createRecentPanel(const ui::LayoutHelper& lh);
	void layoutNormal() override;
	void layoutHorizontal() override;

	void        disableArchiveListUpdate() const;
	void        refreshArchiveList() const;
	void        refreshRecentFileList();
	void        refreshBookmarkList() const;
	void        refreshAllTabs() const;
	void        updateOpenListItem(int index) const;
	void        updateRecentListItem(int index) const;
	void        updateBookmarkListItem(int index) const;
	void        updateArchiveTabTitle(int index) const;
	void        updateEntryTabTitle(const ArchiveEntry* entry) const;
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
	ArchivePanel*   tabForArchive(const Archive* archive) const;
	void            openTab(const Archive* archive) const;
	void            closeTab(int archive_index) const;
	void            openTextureTab(int archive_index, ArchiveEntry* entry = nullptr) const;
	TextureXEditor* textureTabForArchive(int archive_index) const;
	void            closeTextureTab(int archive_index) const;
	void            openEntryTab(ArchiveEntry* entry) const;
	void            closeEntryTab(const ArchiveEntry* entry) const;
	void            closeEntryTabs(const Archive* parent) const;
	void            openFile(const string& filename) const;
	void            openFiles(const wxArrayString& files) const;
	void            openDirAsArchive(string_view dir) const;
	bool            redirectToTab(const ArchiveEntry* entry) const;
	bool            entryIsOpenInTab(const ArchiveEntry* entry) const;
	void            closeCurrentTab();
	bool            saveCurrentTab() const;

	// General actions
	bool undo() const;
	bool redo() const;

	// Single archive actions
	bool saveEntryChanges(const Archive* archive) const;
	bool saveArchive(Archive* archive) const;
	bool saveArchiveAs(Archive* archive) const;
	bool beforeCloseArchive(Archive* archive);
	bool closeArchive(Archive* archive);

	void createNewArchive(const string& format) const;
	bool closeAll();
	void saveAll() const;
	void checkDirArchives();

	// Selected archives in the lists
	void saveSelection() const;
	void saveSelectionAs() const;
	bool closeSelection();
	void openSelection() const;
	void removeSelection();

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
	TabControl*          stc_tabs_                    = nullptr;
	STabCtrl*            stc_archives_                = nullptr;
	wxPanel*             panel_am_                    = nullptr;
	wxPanel*             panel_archives_              = nullptr;
	wxPanel*             panel_rf_                    = nullptr;
	ui::ArchiveListView* list_archives_               = nullptr;
	ui::ArchiveListView* list_recent_                 = nullptr;
	ListView*            list_bookmarks_              = nullptr;
	WMFileBrowser*       file_browser_                = nullptr;
	wxMenu*              menu_recent_                 = nullptr;
	wxMenu*              menu_bookmarks_              = nullptr;
	Archive*             current_maps_                = nullptr;
	Archive*             pending_closed_archive_      = nullptr;
	bool                 asked_save_unchanged_        = false;
	bool                 checked_dir_archive_changes_ = false;
	vector<Archive*>     checking_archives_;
	vector<string>       recent_files_;

	// Signal connections
	ScopedConnectionList signal_connections;

	void connectSignals();
	bool prepareCloseTab(int index);
};
} // namespace slade
