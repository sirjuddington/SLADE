
#ifndef __MAINWINDOW_H__
#define __MAINWINDOW_H__

#include "ArchiveManagerPanel.h"
#include <wx/aui/auibook.h>
#include <wx/aui/aui.h>
#include "PaletteChooser.h"
#include "MainApp.h"
#include "STopWindow.h"

class SToolBar;
class UndoManagerHistoryPanel;
class wxWebView;
class MainWindow : public STopWindow, SActionHandler
{
private:
	ArchiveManagerPanel*		panel_archivemanager;
	UndoManagerHistoryPanel*	panel_undo_history;
	wxAuiNotebook*				notebook_tabs;
	wxWebView*					html_startpage;
	wxAuiManager*				m_mgr;
	int							lasttipindex;
	PaletteChooser*				palette_chooser;

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
	void	createStartPage();
	bool	exitProgram();

	ArchiveManagerPanel*	getArchiveManagerPanel() { return panel_archivemanager; }
	PaletteChooser*			getPaletteChooser() { return palette_chooser; }

	Archive*				getCurrentArchive();
	ArchiveEntry*			getCurrentEntry();
	vector<ArchiveEntry*>	getCurrentEntrySelection();

	void	openTextureEditor(Archive* archive);
	void	openMapEditor(Archive* archive);
	void	openEntry(ArchiveEntry* entry);

	// Events
	void	onMenuItemClicked(wxCommandEvent& e);
	void	onHTMLLinkClicked(wxEvent& e);
	void	onClose(wxCloseEvent& e);
	void	onTabChanged(wxAuiNotebookEvent& e);
	void	onSize(wxSizeEvent& e);
	void	onMove(wxMoveEvent& e);
	void	onToolBarLayoutChanged(wxEvent& e);
};

// Define for less cumbersome MainWindow::getInstance()
#define theMainWindow MainWindow::getInstance()

// Define for much less cumbersome way to get the active entry panel and palette chooser
#define theActivePanel (theMainWindow ? (theMainWindow->getArchiveManagerPanel() ? theMainWindow->getArchiveManagerPanel()->currentArea() : NULL) : NULL)
#define thePaletteChooser (theMainWindow ? theMainWindow->getPaletteChooser() : NULL)

#endif //__MAINWINDOW_H__
