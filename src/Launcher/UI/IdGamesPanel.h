#pragma once

#include "Launcher/IdGames.h"

class IdGamesDetailsPanel : public wxPanel
{
public:
	IdGamesDetailsPanel(wxWindow* parent);
	~IdGamesDetailsPanel();

	void	loadDetails(idGames::File file);

private:
	wxTextCtrl*	text_textfile;
};

class ListView;
class wxRadioButton;
class IdGamesPanel : public wxPanel
{
public:
	IdGamesPanel(wxWindow* parent);
	~IdGamesPanel();

private:
	vector<idGames::File>	files_latest_;
	vector<idGames::File>	files_search_;
	vector<idGames::File>	files_browse_;

	ListView*				lv_files_		= nullptr;
	wxButton*				btn_refresh_	= nullptr;	// Temp
	wxRadioButton*			rb_latest_		= nullptr;
	wxRadioButton*			rb_search_		= nullptr;
	wxRadioButton*			rb_browse_		= nullptr;
	IdGamesDetailsPanel*	panel_details_	= nullptr;

	// Search
	wxChoice*	choice_search_by_		= nullptr;
	wxTextCtrl*	text_search_			= nullptr;
	wxChoice*	choice_search_sort_		= nullptr;
	wxButton*	btn_search_sort_dir_	= nullptr;
	wxPanel*	panel_search_			= nullptr;

	wxPanel*	setupSearchControlPanel();

	void	loadList(vector<idGames::File>& list);
	
	void	getLatestFiles();
	void	readLatestFiles(string& xml);

	void	searchFiles();
	void	readSearchResult(string& xml);

	// Events
	void	onApiCallCompleted(wxThreadEvent& e);
	void	onBtnRefreshClicked(wxCommandEvent& e);
	void	onRBSearchClicked(wxCommandEvent& e);
	void	onRBLatestClicked(wxCommandEvent& e);
	void	onRBBrowseClicked(wxCommandEvent& e);
	void	onListItemSelected(wxListEvent& e);
};
