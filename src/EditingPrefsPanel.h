
#ifndef __EDITING_PREFS_PANEL_H__
#define __EDITING_PREFS_PANEL_H__

#include "PrefsPanelBase.h"

class EditingPrefsPanel : public PrefsPanelBase {
private:
	wxCheckBox*	cb_wad_force_uppercase;
	wxChoice*	choice_entry_mod;

public:
	EditingPrefsPanel(wxWindow* parent);
	~EditingPrefsPanel();

	void	init();
	void	applyPreferences();
};

#endif//__EDITING_PREFS_PANEL_H__
