#pragma once

#include "PrefsPanelBase.h"
#include "UI/Controls/STabCtrl.h"
#include "UI/Lists/VirtualListView.h"

class EditingPrefsPanel : public PrefsPanelBase
{
public:
	EditingPrefsPanel(wxWindow* parent);
	~EditingPrefsPanel() = default;

	void init() override;
	void applyPreferences() override;
	void showSubSection(const string& subsection) override;

private:
	TabControl* stc_tabs_ = nullptr;

	// General
	wxCheckBox* cb_wad_force_uppercase_   = nullptr;
	wxCheckBox* cb_zip_percent_encoding_  = nullptr;
	wxCheckBox* cb_auto_entry_replace_    = nullptr;
	wxCheckBox* cb_save_archive_with_map_ = nullptr;
	wxChoice*   choice_entry_mod_         = nullptr;
	wxCheckBox* cb_confirm_entry_delete_  = nullptr;
	wxCheckBox* cb_confirm_entry_revert_  = nullptr;
	wxChoice*   choice_dir_mod_           = nullptr;

	// External editors
	VirtualListView* lv_ext_editors_  = nullptr;
	wxChoice*        choice_category_ = nullptr;
	wxBitmapButton*  btn_add_exe_     = nullptr;
	wxBitmapButton*  btn_remove_exe_  = nullptr;

	wxPanel* setupGeneralTab();
	wxPanel* setupExternalTab();

	// Events
	void onBtnAddClicked(wxCommandEvent& e);
	void onBtnRemoveClicked(wxCommandEvent& e);
	void onExternalExeActivated(wxListEvent& e);
};
