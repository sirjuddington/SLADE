#pragma once

#include <wx/panel.h>
#include <wx/splitter.h>

// Forward declarations
class wxChoice;
class STabCtrl;
class ListView;
class WMFileBrowser;
class wxButton;
class LibraryPanel;

class LauncherPanel : public wxPanel
{
public:
	LauncherPanel(wxWindow* parent);
	~LauncherPanel();

	string	commandLine();
	void	addFile(string path);

private:
	wxChoice*	choice_game_	= nullptr;
	wxChoice*	choice_port_	= nullptr;
	ListView*	lv_files_		= nullptr;
	wxButton*	btn_launch_		= nullptr;

	STabCtrl*		tabs_library_		= nullptr;
	ListView*		lv_recent_files_	= nullptr;
	LibraryPanel*	panel_library_		= nullptr;

	wxPanel*	setupControlsPanel(wxWindow* parent);
	wxPanel*	setupFileBrowserTab();

	void	loadExecutables();
	void	loadGames();

	// Events
	void	onSplitterSashPosChanged(wxSplitterEvent& e);
	void	onLaunchClicked(wxCommandEvent& e);
};
