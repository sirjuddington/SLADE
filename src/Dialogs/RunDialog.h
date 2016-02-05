
#ifndef __RUN_DIALOG_H__
#define __RUN_DIALOG_H__

#include "UI/SDialog.h"

class ResourceArchiveChooser;
class Archive;
class RunDialog : public SDialog
{
private:
	wxChoice*				choice_game_exes;
	wxButton*				btn_add_game;
	wxButton*				btn_remove_game;
	wxTextCtrl*				text_exe_path;
	wxButton*				btn_browse_exe;
	wxChoice*				choice_config;
	wxButton*				btn_add_config;
	wxButton*				btn_edit_config;
	wxButton*				btn_remove_config;
	wxButton*				btn_run;
	wxButton*				btn_cancel;
	ResourceArchiveChooser*	rac_resources;
	wxTextCtrl*				text_extra_params;

public:
	RunDialog(wxWindow* parent, Archive* archive);
	~RunDialog();

	void	openGameExe(unsigned index);
	string	getSelectedCommandLine(Archive* archive, string map_name, string map_file = "");
	string	getSelectedResourceList();
	string	getSelectedExeDir();
	string	getSelectedExeId();

	// Events
	void	onBtnAddGame(wxCommandEvent& e);
	void	onBtnBrowseExe(wxCommandEvent& e);
	void	onBtnAddConfig(wxCommandEvent& e);
	void	onBtnEditConfig(wxCommandEvent& e);
	void	onBtnRun(wxCommandEvent& e);
	void	onBtnCancel(wxCommandEvent& e);
	void	onChoiceGameExe(wxCommandEvent& e);
	void	onChoiceConfig(wxCommandEvent& e);
	void	onBtnRemoveGame(wxCommandEvent& e);
	void	onBtnRemoveConfig(wxCommandEvent& e);
};

#endif//__RUN_DIALOG_H__
