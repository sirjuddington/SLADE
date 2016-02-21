
#ifndef __EDITING_PREFS_PANEL_H__
#define __EDITING_PREFS_PANEL_H__

#include "PrefsPanelBase.h"

class EditingPrefsPanel : public PrefsPanelBase
{
private:
	wxCheckBox*	cb_wad_force_uppercase;
	wxCheckBox*	cb_zip_percent_encoding;
	wxCheckBox*	cb_auto_entry_replace;
	wxCheckBox*	cb_save_archive_with_map;
	wxChoice*	choice_entry_mod;
	wxCheckBox*	cb_confirm_entry_delete;
	wxCheckBox*	cb_confirm_entry_revert;

public:
	EditingPrefsPanel(wxWindow* parent);
	~EditingPrefsPanel();

	void	init();
	void	applyPreferences();
};

#endif//__EDITING_PREFS_PANEL_H__
