#pragma once

#include "Launcher/IdGames.h"

class IdGamesDetailsPanel
{
public:
	IdGamesDetailsPanel(wxWindow* parent);
	~IdGamesDetailsPanel();

	void	loadDetails(idGames::file_t file);

private:

};

class ListView;
class wxRadioButton;
class IdGamesPanel : public wxPanel
{
public:
	IdGamesPanel(wxWindow* parent);
	~IdGamesPanel();

private:
	vector<idGames::file_t>	files_latest;
	vector<idGames::file_t> files_search;
	vector<idGames::file_t> files_browse;

	ListView*		lv_files;
	wxButton*		btn_refresh;	// Temp
	wxRadioButton*	rb_latest;
	wxRadioButton*	rb_search;
	wxRadioButton*	rb_browse;

	// Search
	wxChoice*	choice_search_by;
	wxTextCtrl*	text_search;
	wxChoice*	choice_search_sort;
	wxButton*	btn_search_sort_dir;
	wxPanel*	panel_search;

	wxPanel*	setupSearchControlPanel();

	void	loadList(vector<idGames::file_t>& list);
	
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
};
