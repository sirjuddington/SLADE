
#ifndef __ARCHIVEMANAGERPANEL_H__
#define __ARCHIVEMANAGERPANEL_H__

#include <wx/listctrl.h>
#include <wx/aui/auibook.h>
#include <wx/dirctrl.h>
#include <wx/listbox.h>
#include "ListenerAnnouncer.h"
#include "Archive.h"
#include "ListView.h"
#include "EntryPanel.h"
#include "MainApp.h"
#include "DockPanel.h"

class ArchiveManagerPanel;
class ArchivePanel;

class WMFileBrowser : public wxGenericDirCtrl
{
private:

public:
	ArchiveManagerPanel*	parent;

	WMFileBrowser(wxWindow* parent, ArchiveManagerPanel* wm, int id);
	~WMFileBrowser();

	void onItemActivated(wxTreeEvent& e);
};

class TextureXEditor;
class ArchiveManagerPanel : public DockPanel, Listener, SActionHandler
{
private:
	wxAuiNotebook*		notebook_tabs;
	wxAuiNotebook*		notebook_archives;
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

public:
	ArchiveManagerPanel(wxWindow* parent, wxAuiNotebook* nb_archives);
	~ArchiveManagerPanel();

	wxMenu*			getRecentMenu() { return menu_recent; }

	// DockPanel layout
	void	createArchivesPanel();
	void	createRecentPanel();
	void	layoutNormal();
	void	layoutHorizontal();

	void			refreshArchiveList();
	void			refreshRecentFileList();
	void			refreshBookmarkList();
	void			refreshAllTabs();
	void			updateOpenListItem(int index);
	void			updateRecentListItem(int index);
	void			updateBookmarkListItem(int index);
	bool			isArchivePanel(int tab_index);
	Archive*		getArchive(int tab_index);
	int				currentTabIndex();
	Archive*		currentArchive();
	wxWindow*		currentPanel();
	EntryPanel*		currentArea();

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

	// General actions
	bool	undo();
	bool	redo();

	// Single archive actions
	bool	saveEntryChanges(Archive* archive);
	bool	saveArchive(Archive* archive);
	bool	saveArchiveAs(Archive* archive);
	bool	closeArchive(Archive* archive);

	void	createNewArchive(uint8_t type);
	bool	closeAll();
	void	saveAll();

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
	void	onAMTabChanged(wxAuiNotebookEvent& e);
};

#endif //__ARCHIVEMANAGERPANEL_H__
