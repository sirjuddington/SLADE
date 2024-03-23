#pragma once

#include "PrefsPanelBase.h"

class wxListBox;

namespace slade
{
class FileLocationPanel;

class ACSPrefsPanel : public PrefsPanelBase
{
public:
	ACSPrefsPanel(wxWindow* parent);
	~ACSPrefsPanel() override = default;

	void init() override;
	void applyPreferences() override;

	wxString pageTitle() override { return "ACS Compiler Settings"; }

private:
	FileLocationPanel* flp_acc_path_          = nullptr;
	wxButton*          btn_incpath_add_       = nullptr;
	wxButton*          btn_incpath_remove_    = nullptr;
	wxListBox*         list_inc_paths_        = nullptr;
	wxCheckBox*        cb_always_show_output_ = nullptr;

	void setupLayout();

	// Events
	void onBtnAddIncPath(wxCommandEvent& e);
	void onBtnRemoveIncPath(wxCommandEvent& e);
};
} // namespace slade
