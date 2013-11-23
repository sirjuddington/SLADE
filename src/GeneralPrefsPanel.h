
#ifndef __GENERAL_PREFS_PANEL_H__
#define __GENERAL_PREFS_PANEL_H__

#include "PrefsPanelBase.h"

class GeneralPrefsPanel : public PrefsPanelBase
{
private:
	wxCheckBox*	cb_gl_np2;
	wxCheckBox*	cb_archive_load;
	wxCheckBox*	cb_archive_close_tab;
	//wxCheckBox*	cb_temp_dir;

public:
	GeneralPrefsPanel(wxWindow* parent);
	~GeneralPrefsPanel();

	void	init();
	void	applyPreferences();
};

#endif//__GENERAL_PREFS_PANEL_H__
