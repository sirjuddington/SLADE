#pragma once

#include <wx/panel.h>
#include <wx/splitter.h>

// Forward declarations
class wxChoice;
class STabCtrl;
class ListView;
class WMFileBrowser;
class wxButton;

class LauncherPanel : public wxPanel
{
public:
	LauncherPanel(wxWindow* parent);
	~LauncherPanel();

	string	commandLine();
	void	addFile(string path);

private:
	wxChoice*	choice_game;
	wxChoice*	choice_port;
	ListView*	lv_files;
	wxButton*	btn_launch;

	STabCtrl*	tabs_library;
	ListView*	lv_recent_files;

	wxPanel*	setupControlsPanel(wxWindow* parent);
	wxPanel*	setupFileBrowserTab();

	void	loadExecutables();
	void	loadGames();

	// Events
	void	onSplitterSashPosChanged(wxSplitterEvent& e);
	void	onLaunchClicked(wxCommandEvent& e);
};
