#pragma once

#include "UI/SDialog.h"

namespace slade
{
class ResourceArchiveChooser;
class Archive;

class RunDialog : public SDialog
{
public:
	RunDialog(wxWindow* parent, Archive* archive, bool show_start_3d_cb = false, bool run_map = false);
	~RunDialog();

	void     openGameExe(unsigned index) const;
	wxString selectedCommandLine(Archive* archive, const wxString& map_name, const wxString& map_file = "") const;
	wxString selectedResourceList() const;
	wxString selectedExeDir() const;
	wxString selectedExeId() const;
	bool     start3dModeChecked() const;

private:
	wxChoice*               choice_game_exes_  = nullptr;
	wxButton*               btn_add_game_      = nullptr;
	wxButton*               btn_remove_game_   = nullptr;
	wxTextCtrl*             text_exe_path_     = nullptr;
	wxButton*               btn_browse_exe_    = nullptr;
	wxChoice*               choice_config_     = nullptr;
	wxButton*               btn_add_config_    = nullptr;
	wxButton*               btn_edit_config_   = nullptr;
	wxButton*               btn_remove_config_ = nullptr;
	wxButton*               btn_run_           = nullptr;
	wxButton*               btn_cancel_        = nullptr;
	ResourceArchiveChooser* rac_resources_     = nullptr;
	wxTextCtrl*             text_extra_params_ = nullptr;
	wxCheckBox*             cb_start_3d_       = nullptr;
	bool                    run_map_           = false;

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
} // namespace slade
