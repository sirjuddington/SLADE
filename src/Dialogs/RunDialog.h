#pragma once

#include "UI/SDialog.h"

class ResourceArchiveChooser;
class Archive;
class RunDialog : public SDialog
{
public:
	RunDialog(wxWindow* parent, Archive* archive, bool show_start_3d_cb = false);
	~RunDialog();

	void   openGameExe(unsigned index);
	string selectedCommandLine(Archive* archive, string map_name, string map_file = "");
	string selectedResourceList();
	string selectedExeDir();
	string selectedExeId();
	bool   start3dModeChecked();

private:
	wxChoice*               choice_game_exes_;
	wxButton*               btn_add_game_;
	wxButton*               btn_remove_game_;
	wxTextCtrl*             text_exe_path_;
	wxButton*               btn_browse_exe_;
	wxChoice*               choice_config_;
	wxButton*               btn_add_config_;
	wxButton*               btn_edit_config_;
	wxButton*               btn_remove_config_;
	wxButton*               btn_run_;
	wxButton*               btn_cancel_;
	ResourceArchiveChooser* rac_resources_;
	wxTextCtrl*             text_extra_params_;
	wxCheckBox*             cb_start__3_d_;

	// Events
	void onBtnAddGame(wxCommandEvent& e);
	void onBtnBrowseExe(wxCommandEvent& e);
	void onBtnAddConfig(wxCommandEvent& e);
	void onBtnEditConfig(wxCommandEvent& e);
	void onBtnRun(wxCommandEvent& e);
	void onBtnCancel(wxCommandEvent& e);
	void onChoiceGameExe(wxCommandEvent& e);
	void onChoiceConfig(wxCommandEvent& e);
	void onBtnRemoveGame(wxCommandEvent& e);
	void onBtnRemoveConfig(wxCommandEvent& e);
};
