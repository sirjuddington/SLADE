#pragma once

#include "General/SActionHandler.h"
#include "UI/STopWindow.h"

namespace slade::ui
{
class SAuiManager;
}

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

	void setupLayout();
	bool exitProgram();

	bool startPageTabOpen() const;
	void openStartPageTab() const;

	ArchiveManagerPanel*     archiveManagerPanel() const { return panel_archivemanager_; }
	PaletteChooser*          paletteChooser() const { return palette_chooser_; }
	UndoManagerHistoryPanel* undoHistoryPanel() const { return panel_undo_history_; }

private:
	ArchiveManagerPanel*     panel_archivemanager_ = nullptr;
	UndoManagerHistoryPanel* panel_undo_history_   = nullptr;
	STabCtrl*                stc_tabs_             = nullptr;
	ui::SAuiManager*         aui_mgr_              = nullptr;
	PaletteChooser*          palette_chooser_      = nullptr;
	bool                     opengl_test_done      = false;

	// Action handling
	bool handleAction(string_view id) override;

	// Events
	void onClose(wxCloseEvent& e);
	void onTabChanged(wxAuiNotebookEvent& e);
	void onSize(wxSizeEvent& e);
	void onActivate(wxActivateEvent& e);
};
} // namespace slade
