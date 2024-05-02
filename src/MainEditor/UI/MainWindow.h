#pragma once

#include "General/SActionHandler.h"
#include "UI/STopWindow.h"

class wxAuiManager;

namespace slade
{
class ArchiveManagerPanel;
class PaletteChooser;
class SToolBar;
class STabCtrl;
class UndoManagerHistoryPanel;

class MainWindow : public STopWindow, SActionHandler
{
public:
	MainWindow();
	~MainWindow() override;

	// Layout save/load
	void loadLayout() const;
	void saveLayout() const;

	void setupLayout();
	bool exitProgram();

	bool startPageTabOpen() const;
	void openStartPageTab() const;

	bool libraryTabOpen() const;
	void openLibraryTab() const;

	ArchiveManagerPanel*     archiveManagerPanel() const { return panel_archivemanager_; }
	PaletteChooser*          paletteChooser() const { return palette_chooser_; }
	UndoManagerHistoryPanel* undoHistoryPanel() const { return panel_undo_history_; }

private:
	ArchiveManagerPanel*     panel_archivemanager_ = nullptr;
	UndoManagerHistoryPanel* panel_undo_history_   = nullptr;
	STabCtrl*                stc_tabs_             = nullptr;
	wxAuiManager*            aui_mgr_              = nullptr;
	PaletteChooser*          palette_chooser_      = nullptr;
	bool                     opengl_test_done      = false;

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
