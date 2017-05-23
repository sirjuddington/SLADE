
#ifndef __INTERFACE_PREFS_PANEL_H__
#define __INTERFACE_PREFS_PANEL_H__

#include "PrefsPanelBase.h"

class InterfacePrefsPanel : public PrefsPanelBase
{
private:
	wxCheckBox*	cb_size_as_string;
	wxCheckBox* cb_filter_dirs;
	wxCheckBox*	cb_list_monospace;
	wxCheckBox*	cb_start_page;
	wxCheckBox*	cb_context_submenus;
	wxCheckBox*	cb_elist_bgcol;
	wxCheckBox* cb_file_browser;
	wxCheckBox*	cb_condensed_tabs;
	wxChoice*	choice_toolbar_size;
	wxChoice*	choice_tab_style;
	wxChoice*	choice_iconset_general;
	wxChoice*	choice_iconset_entry;

public:
	InterfacePrefsPanel(wxWindow* parent);
	~InterfacePrefsPanel();

	void	init();
	void	applyPreferences();
};

#endif//__INTERFACE_PREFS_PANEL_H__
