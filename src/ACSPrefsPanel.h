
#ifndef __ACS_PREFS_PANEL_H__
#define __ACS_PREFS_PANEL_H__

#include "PrefsPanelBase.h"

class ACSPrefsPanel : public PrefsPanelBase {
private:
	wxTextCtrl*	text_accpath;
	wxButton*	btn_browse_accpath;

public:
	ACSPrefsPanel(wxWindow* parent);
	~ACSPrefsPanel();

	void	init();
	void	applyPreferences();

	// Events
	void	onBtnBrowseACCPath(wxCommandEvent& e);
};

#endif//__ACS_PREFS_PANEL_H__
