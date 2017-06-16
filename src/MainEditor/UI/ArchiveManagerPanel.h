
#ifndef __ARCHIVEMANAGERPANEL_H__
#define __ARCHIVEMANAGERPANEL_H__

#include "Archive/Formats/DirArchive.h"
#include "General/ListenerAnnouncer.h"
#include "General/SAction.h"
#include "UI/DockPanel.h"
#include "UI/Lists/ListView.h"

class ArchiveManagerPanel;
class ArchivePanel;
class Archive;
class STabCtrl;
class TextureXEditor;
class EntryPanel;

wxDECLARE_EVENT(wxEVT_COMMAND_DIRARCHIVECHECK_COMPLETED, wxThreadEvent);

struct DirArchiveChangeList
{
	Archive*				archive;
	vector<DirEntryChange>	changes;
};

class DirArchiveCheck : public wxThread
{
public:
	DirArchiveCheck(wxEvtHandler* handler, DirArchive* archive);
	virtual ~DirArchiveCheck();

	ExitCode Entry() override;

private:
	struct EntryInfo
	{
		string	entry_path;
		string	file_path;
		bool	is_dir;
		time_t	file_modified;
	};

	wxEvtHandler*			handler_;
	string					dir_path_;
	vector<EntryInfo>		entry_info_;
	vector<string>			removed_files_;
	DirArchiveChangeList	change_list_;

	void addChange(DirEntryChange change);
};

class WMFileBrowser : public wxGenericDirCtrl
{
public:
	ArchiveManagerPanel*	parent;

	WMFileBrowser(wxWindow* parent, ArchiveManagerPanel* wm, int id);
	~WMFileBrowser();

	void onItemActivated(wxTreeEvent& e);
};

class ArchiveManagerPanel : public DockPanel, Listener, SActionHandler
{
public:
	ArchiveManagerPanel(wxWindow* parent, STabCtrl* nb_archives);
	~ArchiveManagerPanel();

	wxMenu*	getRecentMenu() const { return menu_recent_; }

	// DockPanel layout
	void	createArchivesPanel();
	void	createRecentPanel();
	void	layoutNormal() override;
	void	layoutHorizontal() override;

	void			disableArchiveListUpdate() const;
	void			refreshArchiveList() const;
	void			refreshRecentFileList() const;
	void			refreshBookmarkList() const;
	void			refreshAllTabs() const;
	void			updateOpenListItem(int index) const;
	void			updateRecentListItem(int index) const;
	void			updateBookmarkListItem(int index) const;
	void			updateArchiveTabTitle(int index) const;
	bool			isArchivePanel(int tab_index) const;
	bool			isEntryPanel(int tab_index) const;
	Archive*		getArchive(int tab_index) const;
	int				currentTabIndex() const;
	Archive*		currentArchive() const;
	wxWindow*		currentPanel() const;
	EntryPanel*		currentArea() const;
	bool			askedSaveUnchanged() const { return asked_save_unchanged_; }

	ArchiveEntry*			currentEntry() const;
	vector<ArchiveEntry*>	currentEntrySelection() const;

	void			openTab(int archive_index) const;
	ArchivePanel*	getArchiveTab(Archive* archive) const;
	void			openTab(Archive* archive) const;
	void			closeTab(int archive_index) const;
	void			openTextureTab(int archive_index, ArchiveEntry* entry = nullptr) const;
	TextureXEditor*	getTextureTab(int archive_index) const;
	void			closeTextureTab(int archive_index) const;
	void			openEntryTab(ArchiveEntry* entry) const;
	void			closeEntryTabs(Archive* parent) const;
	void			openFile(string filename) const;
	void			openFiles(wxArrayString& files) const;
	void			openDirAsArchive(string dir) const;
	bool			redirectToTab(ArchiveEntry* entry) const;

	// General actions
	bool	undo() const;
	bool	redo() const;

	// Single archive actions
	bool	saveEntryChanges(Archive* archive) const;
	bool	saveArchive(Archive* archive) const;
	bool	saveArchiveAs(Archive* archive) const;
	bool	beforeCloseArchive(Archive* archive);
	bool	closeArchive(Archive* archive);

	void	createNewArchive(string format) const;
	bool	closeAll();
	void	saveAll() const;
	void	checkDirArchives();

	// Selected archives in the lists
	void	saveSelection() const;
	void	saveSelectionAs() const;
	bool	closeSelection();
	void	openSelection() const;
	void	removeSelection() const;

	// Bookmark functions
	void	deleteSelectedBookmarks() const;
	void	goToBookmark(long index = -1) const;

	// SAction handler
	bool	handleAction(string id) override;

	vector<int>	getSelectedArchives() const;
	vector<int>	getSelectedBookmarks() const;
	vector<int>	getSelectedFiles() const;

	void	onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data) override;

	// Event handlers
	void	onListArchivesChanged(wxListEvent& e);
	void	onListArchivesActivated(wxListEvent& e);
	void	onListArchivesRightClick(wxListEvent& e);
	void	onListRecentActivated(wxListEvent& e);
	void	onListRecentRightClick(wxListEvent& e);
	void	onListBookmarksActivated(wxListEvent& e);
	void	onListBookmarksRightClick(wxListEvent& e);
	void	onArchiveTabChanging(wxAuiNotebookEvent& e);
	void	onArchiveTabChanged(wxAuiNotebookEvent& e);
	void	onArchiveTabClose(wxAuiNotebookEvent& e);
	void	onArchiveTabClosed(wxAuiNotebookEvent& e);
	void	onAMTabChanged(wxAuiNotebookEvent& e);
	void	onDirArchiveCheckCompleted(wxThreadEvent& e);

private:
	STabCtrl*			stc_tabs_;
	STabCtrl*			stc_archives_;
	wxPanel*			panel_am_;
	wxPanel*			panel_archives_;
	wxPanel*			panel_rf_;
	ListView*			list_archives_;
	ListView*			list_recent_;
	ListView*			list_bookmarks_;
	WMFileBrowser*		file_browser_;
	wxButton*			btn_browser_open_;
	wxMenu*				menu_recent_;
	Archive*			current_maps_;
	Archive*			pending_closed_archive_;
	bool				asked_save_unchanged_;
	bool				checked_dir_archive_changes_;
	vector<Archive*>	checking_archives_;
};

#endif //__ARCHIVEMANAGERPANEL_H__
