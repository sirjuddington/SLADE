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
	vector<idGames::File>	files_latest;
	vector<idGames::File>	files_search;
	vector<idGames::File>	files_browse;

	ListView*				lv_files;
	wxButton*				btn_refresh;	// Temp
	wxRadioButton*			rb_latest;
	wxRadioButton*			rb_search;
	wxRadioButton*			rb_browse;
	IdGamesDetailsPanel*	panel_details;

	// Search
	wxChoice*	choice_search_by;
	wxTextCtrl*	text_search;
	wxChoice*	choice_search_sort;
	wxButton*	btn_search_sort_dir;
	wxPanel*	panel_search;

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
