#pragma once

#include "General/SAction.h"
#include "UI/STopWindow.h"

class wxAuiManager;

namespace slade
{
class ArchiveManagerPanel;
class PaletteChooser;
class SToolBar;
class STabCtrl;
class UndoManagerHistoryPanel;
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
	void loadLayout() const;
	void saveLayout() const;

	void setupLayout();
	void createStartPage(bool newtip = true) const;
	bool exitProgram();

	bool startPageTabOpen() const;
	void openStartPageTab();

	ArchiveManagerPanel*     archiveManagerPanel() const { return panel_archivemanager_; }
	PaletteChooser*          paletteChooser() const { return palette_chooser_; }
	UndoManagerHistoryPanel* undoHistoryPanel() const { return panel_undo_history_; }
	SStartPage*              startPage() const { return start_page_; }

#ifdef USE_WEBVIEW_STARTPAGE
	void openDocs(string_view page_name = "");
#endif

private:
	ArchiveManagerPanel*     panel_archivemanager_ = nullptr;
	UndoManagerHistoryPanel* panel_undo_history_   = nullptr;
	STabCtrl*                stc_tabs_             = nullptr;
	wxAuiManager*            aui_mgr_              = nullptr;
	int                      lasttipindex_         = 0;
	PaletteChooser*          palette_chooser_      = nullptr;

	// Start page
	SStartPage* start_page_ = nullptr;
#ifdef USE_WEBVIEW_STARTPAGE
	DocsPage* docs_page_ = nullptr;
#endif

	// Action handling
	bool handleAction(string_view id) override;

	// Events
	void onClose(wxCloseEvent& e);
	void onTabChanged(wxAuiNotebookEvent& e);
	void onSize(wxSizeEvent& e);
	void onToolBarLayoutChanged(wxEvent& e);
	void onActivate(wxActivateEvent& e);
};
} // namespace slade
