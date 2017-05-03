
#ifndef __GENERAL_PREFS_PANEL_H__
#define __GENERAL_PREFS_PANEL_H__

#include "PrefsPanelBase.h"

class GeneralPrefsPanel : public PrefsPanelBase
{
private:
	wxCheckBox*	cb_gl_np2;
	wxCheckBox*	cb_archive_load;
	wxCheckBox*	cb_archive_close_tab;
	wxCheckBox*	cb_wads_root;
	wxCheckBox*	cb_update_check;
	wxCheckBox* cb_update_check_beta;
	wxCheckBox*	cb_confirm_exit;
	wxCheckBox*	cb_backup_archives;

public:
	GeneralPrefsPanel(wxWindow* parent);
	~GeneralPrefsPanel();

	void	init();
	void	applyPreferences();
};

#endif//__GENERAL_PREFS_PANEL_H__
