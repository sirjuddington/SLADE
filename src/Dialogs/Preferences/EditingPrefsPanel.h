
#ifndef __EDITING_PREFS_PANEL_H__
#define __EDITING_PREFS_PANEL_H__

#include "PrefsPanelBase.h"
#include "UI/Lists/VirtualListView.h"
#include "UI/STabCtrl.h"

class EditingPrefsPanel : public PrefsPanelBase
{
private:
	TabControl*	stc_tabs;

	// General
	wxCheckBox*	cb_wad_force_uppercase;
	wxCheckBox*	cb_zip_percent_encoding;
	wxCheckBox*	cb_auto_entry_replace;
	wxCheckBox*	cb_save_archive_with_map;
	wxChoice*	choice_entry_mod;
	wxCheckBox*	cb_confirm_entry_delete;
	wxCheckBox*	cb_confirm_entry_revert;

	// External editors
	VirtualListView*	lv_ext_editors;
	wxChoice*			choice_category;
	wxBitmapButton*		btn_add_exe;
	wxBitmapButton*		btn_remove_exe;

	wxPanel*	setupGeneralTab();
	wxPanel*	setupExternalTab();

	// Events
	void	onChoiceCategoryChanged(wxCommandEvent& e);
	void	onBtnAddClicked(wxCommandEvent& e);
	void	onBtnRemoveClicked(wxCommandEvent& e);
	void	onExternalExeActivated(wxListEvent& e);

public:
	EditingPrefsPanel(wxWindow* parent);
	~EditingPrefsPanel();

	void	init();
	void	applyPreferences();
	void	showSubSection(string subsection);
};

#endif//__EDITING_PREFS_PANEL_H__
