#pragma once

#include "PrefsPanelBase.h"

class InterfacePrefsPanel : public PrefsPanelBase
{
public:
	InterfacePrefsPanel(wxWindow* parent);
	~InterfacePrefsPanel();

	void init() override;
	void applyPreferences() override;

private:
	wxCheckBox* cb_size_as_string_;
	wxCheckBox* cb_filter_dirs_;
	wxCheckBox* cb_list_monospace_;
	wxCheckBox* cb_start_page_;
	wxCheckBox* cb_context_submenus_;
	wxCheckBox* cb_elist_bgcol_;
	wxCheckBox* cb_file_browser_;
	wxCheckBox* cb_condensed_tabs_;
	wxCheckBox* cb_web_dark_theme_;
	wxChoice*   choice_toolbar_size_;
	wxChoice*   choice_tab_style_;
	wxChoice*   choice_iconset_general_;
	wxChoice*   choice_iconset_entry_;

	wxPanel* setupGeneralTab(wxWindow* stc_tabs);
	wxPanel* setupEntryListTab(wxWindow* stc_tabs);
};
