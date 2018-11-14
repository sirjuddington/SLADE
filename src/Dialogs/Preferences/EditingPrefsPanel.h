#pragma once

#include "PrefsPanelBase.h"
#include "UI/Controls/STabCtrl.h"
#include "UI/Lists/VirtualListView.h"

class EditingPrefsPanel : public PrefsPanelBase
{
public:
	EditingPrefsPanel(wxWindow* parent);
	~EditingPrefsPanel();

	void init() override;
	void applyPreferences() override;
	void showSubSection(string subsection) override;

private:
	TabControl* stc_tabs_;

	// General
	wxCheckBox* cb_wad_force_uppercase_;
	wxCheckBox* cb_zip_percent_encoding_;
	wxCheckBox* cb_auto_entry_replace_;
	wxCheckBox* cb_save_archive_with_map_;
	wxChoice*   choice_entry_mod_;
	wxCheckBox* cb_confirm_entry_delete_;
	wxCheckBox* cb_confirm_entry_revert_;
	wxChoice*   choice_dir_mod_;

	// External editors
	VirtualListView* lv_ext_editors_;
	wxChoice*        choice_category_;
	wxBitmapButton*  btn_add_exe_;
	wxBitmapButton*  btn_remove_exe_;

	wxPanel* setupGeneralTab();
	wxPanel* setupExternalTab();

	// Events
	void onChoiceCategoryChanged(wxCommandEvent& e);
	void onBtnAddClicked(wxCommandEvent& e);
	void onBtnRemoveClicked(wxCommandEvent& e);
	void onExternalExeActivated(wxListEvent& e);
};
