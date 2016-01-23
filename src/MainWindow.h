
#ifndef __MAINWINDOW_H__
#define __MAINWINDOW_H__

#include "ArchiveManagerPanel.h"
#include <wx/aui/auibook.h>
#include <wx/aui/aui.h>
#include "PaletteChooser.h"
#include "MainApp.h"
#include "STopWindow.h"

#ifndef USE_WEBVIEW_STARTPAGE
#include <wx/html/htmlwin.h>
#endif

class SToolBar;
class STabCtrl;
class UndoManagerHistoryPanel;
#ifdef USE_WEBVIEW_STARTPAGE
class wxWebView;
class DocsPage;
#endif
class MainWindow : public STopWindow, SActionHandler
{
private:
	ArchiveManagerPanel*		panel_archivemanager;
	UndoManagerHistoryPanel*	panel_undo_history;
	STabCtrl*					stc_tabs;
	wxAuiManager*				m_mgr;
	int							lasttipindex;
	PaletteChooser*				palette_chooser;

	// Start page
#ifdef USE_WEBVIEW_STARTPAGE
	wxWebView*					html_startpage;
	DocsPage*					docs_page;
#else
	wxHtmlWindow*				html_startpage;
#endif

	// Singleton instance
	static MainWindow*		instance;

	// Action handling
	bool	handleAction(string id);

public:
	MainWindow();
	~MainWindow();

	// Singleton implementation
	static MainWindow* getInstance()
	{
		if (!instance)
			instance = new MainWindow();

		return instance;
	}

	// Layout save/load
	void	loadLayout();
	void	saveLayout();

	void	setupLayout();
	void	createStartPage(bool newtip = true);
	bool	exitProgram();

	ArchiveManagerPanel*		getArchiveManagerPanel() { return panel_archivemanager; }
	PaletteChooser*				getPaletteChooser() { return palette_chooser; }
	UndoManagerHistoryPanel*	getUndoHistoryPanel() { return panel_undo_history; }

	Archive*				getCurrentArchive();
	ArchiveEntry*			getCurrentEntry();
	vector<ArchiveEntry*>	getCurrentEntrySelection();

	void	openTextureEditor(Archive* archive, ArchiveEntry* entry = NULL);
	void	openMapEditor(Archive* archive);
	void	openEntry(ArchiveEntry* entry);
#ifdef USE_WEBVIEW_STARTPAGE
	void	openDocs(string page_name = "");
#endif

	// Events
	void	onMenuItemClicked(wxCommandEvent& e);
	void	onHTMLLinkClicked(wxEvent& e);
	void	onClose(wxCloseEvent& e);
	void	onTabChanged(wxAuiNotebookEvent& e);
	void	onSize(wxSizeEvent& e);
	void	onToolBarLayoutChanged(wxEvent& e);
	void	onActivate(wxActivateEvent& e);
};

// Define for less cumbersome MainWindow::getInstance()
#define theMainWindow MainWindow::getInstance()

// Define for much less cumbersome way to get the active entry panel and palette chooser
#define theActivePanel (theMainWindow ? (theMainWindow->getArchiveManagerPanel() ? theMainWindow->getArchiveManagerPanel()->currentArea() : NULL) : NULL)
#define thePaletteChooser (theMainWindow ? theMainWindow->getPaletteChooser() : NULL)

#endif //__MAINWINDOW_H__
