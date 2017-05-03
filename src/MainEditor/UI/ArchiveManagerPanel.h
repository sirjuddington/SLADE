
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

struct dir_archive_changelist_t
{
	Archive*					archive;
	vector<dir_entry_change_t>	changes;
};

class DirArchiveCheck : public wxThread
{
private:
	struct entry_info_t
	{
		string	entry_path;
		string	file_path;
		bool	is_dir;
		time_t	file_modified;
	};

	wxEvtHandler*				handler;
	string						dir_path;
	vector<entry_info_t>		entry_info;
	vector<string>				removed_files;
	dir_archive_changelist_t	change_list;

	void addChange(dir_entry_change_t change);

public:
	DirArchiveCheck(wxEvtHandler* handler, DirArchive* archive);
	virtual ~DirArchiveCheck();

	ExitCode Entry();
};



class WMFileBrowser : public wxGenericDirCtrl
{
private:

public:
	ArchiveManagerPanel*	parent;

	WMFileBrowser(wxWindow* parent, ArchiveManagerPanel* wm, int id);
	~WMFileBrowser();

	void onItemActivated(wxTreeEvent& e);
};


class ArchiveManagerPanel : public DockPanel, Listener, SActionHandler
{
private:
	STabCtrl*			stc_tabs;
	STabCtrl*			stc_archives;
	wxPanel*			panel_am;
	wxPanel*			panel_archives;
	wxPanel*			panel_rf;
	ListView*			list_archives;
	ListView*			list_recent;
	ListView*			list_bookmarks;
	WMFileBrowser*		file_browser;
	wxButton*			btn_browser_open;
	wxMenu*				menu_recent;
	Archive*			current_maps;
	Archive*			pending_closed_archive;
	bool				asked_save_unchanged;
	bool				checked_dir_archive_changes;
	vector<Archive*>	checking_archives;

public:
	ArchiveManagerPanel(wxWindow* parent, STabCtrl* nb_archives);
	~ArchiveManagerPanel();

	wxMenu*			getRecentMenu() { return menu_recent; }

	// DockPanel layout
	void	createArchivesPanel();
	void	createRecentPanel();
	void	layoutNormal();
	void	layoutHorizontal();

	void			disableArchiveListUpdate();
	void			refreshArchiveList();
	void			refreshRecentFileList();
	void			refreshBookmarkList();
	void			refreshAllTabs();
	void			updateOpenListItem(int index);
	void			updateRecentListItem(int index);
	void			updateBookmarkListItem(int index);
	void			updateArchiveTabTitle(int index);
	bool			isArchivePanel(int tab_index);
	bool			isEntryPanel(int tab_index);
	Archive*		getArchive(int tab_index);
	int				currentTabIndex();
	Archive*		currentArchive();
	wxWindow*		currentPanel();
	EntryPanel*		currentArea();
	bool			askedSaveUnchanged() { return asked_save_unchanged; }

	ArchiveEntry*			currentEntry();
	vector<ArchiveEntry*>	currentEntrySelection();

	void			openTab(int archive_index);
	ArchivePanel*	getArchiveTab(Archive* archive);
	void			openTab(Archive* archive);
	void			closeTab(int archive_index);
	void			openTextureTab(int archive_index, ArchiveEntry* entry = NULL);
	TextureXEditor*	getTextureTab(int archive_index);
	void			closeTextureTab(int archive_index);
	void			openEntryTab(ArchiveEntry* entry);
	void			closeEntryTabs(Archive* parent);
	void			openFile(string filename);
	void			openFiles(wxArrayString& files);
	void			openDirAsArchive(string dir);
	bool			redirectToTab(ArchiveEntry* entry);

	// General actions
	bool	undo();
	bool	redo();

	// Single archive actions
	bool	saveEntryChanges(Archive* archive);
	bool	saveArchive(Archive* archive);
	bool	saveArchiveAs(Archive* archive);
	bool	beforeCloseArchive(Archive* archive);
	bool	closeArchive(Archive* archive);

	void	createNewArchive(uint8_t type);
	bool	closeAll();
	void	saveAll();
	void	checkDirArchives();

	// Selected archives in the lists
	void	saveSelection();
	void	saveSelectionAs();
	bool	closeSelection();
	void	openSelection();
	void	removeSelection();

	// Bookmark functions
	void	deleteSelectedBookmarks();
	void	goToBookmark(long index = -1);

	// SAction handler
	bool	handleAction(string id);

	vector<int>	getSelectedArchives();
	vector<int>	getSelectedBookmarks();
	vector<int>	getSelectedFiles();

	void	onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data);

	// Event handlers
	void	onListArchivesChanged(wxListEvent& e);
	void	onListArchivesActivated(wxListEvent& e);
	void	onListArchivesRightClick(wxListEvent& e);
	void	onListRecentChanged(wxListEvent& e);
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
};

#endif //__ARCHIVEMANAGERPANEL_H__
