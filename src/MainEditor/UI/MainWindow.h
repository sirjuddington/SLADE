
#ifndef __MAINWINDOW_H__
#define __MAINWINDOW_H__

#include "General/SAction.h"
#include "UI/STopWindow.h"

class ArchiveManagerPanel;
class PaletteChooser;
class SToolBar;
class STabCtrl;
class UndoManagerHistoryPanel;
class wxAuiManager;
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

	// Action handling
	bool	handleAction(string id) override;

public:
	MainWindow();
	~MainWindow();

	// Layout save/load
	void	loadLayout();
	void	saveLayout();

	void	setupLayout();
	void	createStartPage(bool newtip = true);
	bool	exitProgram();

	ArchiveManagerPanel*		getArchiveManagerPanel() { return panel_archivemanager; }
	PaletteChooser*				getPaletteChooser() { return palette_chooser; }
	UndoManagerHistoryPanel*	getUndoHistoryPanel() { return panel_undo_history; }

#ifdef USE_WEBVIEW_STARTPAGE
	void	openDocs(string page_name = "");
#endif

	// Events
	void	onHTMLLinkClicked(wxEvent& e);
	void	onClose(wxCloseEvent& e);
	void	onTabChanged(wxAuiNotebookEvent& e);
	void	onSize(wxSizeEvent& e);
	void	onToolBarLayoutChanged(wxEvent& e);
	void	onActivate(wxActivateEvent& e);
};

#endif //__MAINWINDOW_H__
