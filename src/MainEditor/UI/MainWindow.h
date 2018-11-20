#pragma once

#include "General/SAction.h"
#include "UI/STopWindow.h"

class ArchiveManagerPanel;
class PaletteChooser;
class SToolBar;
class STabCtrl;
class UndoManagerHistoryPanel;
class wxAuiManager;
class SStartPage;
#ifdef USE_WEBVIEW_STARTPAGE
class DocsPage;
#endif

class MainWindow : public STopWindow, SActionHandler
{
public:
	MainWindow();
	~MainWindow();

	// Layout save/load
	void loadLayout();
	void saveLayout();

	void setupLayout();
	void createStartPage(bool newtip = true) const;
	bool exitProgram();

	bool startPageTabOpen() const;
	void openStartPageTab();

	ArchiveManagerPanel*     getArchiveManagerPanel() { return panel_archivemanager_; }
	PaletteChooser*          getPaletteChooser() { return palette_chooser_; }
	UndoManagerHistoryPanel* getUndoHistoryPanel() { return panel_undo_history_; }
	SStartPage*              startPage() const { return start_page_; }

#ifdef USE_WEBVIEW_STARTPAGE
	void openDocs(string page_name = "");
#endif

private:
	ArchiveManagerPanel*     panel_archivemanager_;
	UndoManagerHistoryPanel* panel_undo_history_;
	STabCtrl*                stc_tabs_;
	wxAuiManager*            aui_mgr_;
	int                      lasttipindex_;
	PaletteChooser*          palette_chooser_;

	// Start page
	SStartPage* start_page_;
#ifdef USE_WEBVIEW_STARTPAGE
	DocsPage* docs_page_;
#endif

	// Action handling
	bool handleAction(string id) override;

	// Events
	void onClose(wxCloseEvent& e);
	void onTabChanged(wxAuiNotebookEvent& e);
	void onSize(wxSizeEvent& e);
	void onToolBarLayoutChanged(wxEvent& e);
	void onActivate(wxActivateEvent& e);
};
