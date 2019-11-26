#pragma once

#include "PrefsPanelBase.h"

class InterfacePrefsPanel : public PrefsPanelBase
{
public:
	InterfacePrefsPanel(wxWindow* parent);
	~InterfacePrefsPanel() = default;

	void init() override;
	void applyPreferences() override;

private:
	wxCheckBox* cb_size_as_string_      = nullptr;
	wxCheckBox* cb_filter_dirs_         = nullptr;
	wxCheckBox* cb_list_monospace_      = nullptr;
	wxCheckBox* cb_start_page_          = nullptr;
	wxCheckBox* cb_context_submenus_    = nullptr;
	wxCheckBox* cb_elist_bgcol_         = nullptr;
	wxCheckBox* cb_file_browser_        = nullptr;
	wxCheckBox* cb_condensed_tabs_      = nullptr;
	wxCheckBox* cb_web_dark_theme_      = nullptr;
	wxChoice*   choice_toolbar_size_    = nullptr;
	wxChoice*   choice_tab_style_       = nullptr;
	wxChoice*   choice_iconset_general_ = nullptr;
	wxChoice*   choice_iconset_entry_   = nullptr;

	wxPanel* setupGeneralTab(wxWindow* stc_tabs);
	wxPanel* setupEntryListTab(wxWindow* stc_tabs);
};
