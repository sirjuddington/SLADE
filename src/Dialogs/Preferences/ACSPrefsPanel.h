
#ifndef __ACS_PREFS_PANEL_H__
#define __ACS_PREFS_PANEL_H__

#include "PrefsPanelBase.h"

class wxListBox;
class ACSPrefsPanel : public PrefsPanelBase
{
private:
	wxTextCtrl*	text_accpath;
	wxButton*	btn_browse_accpath;
	wxButton*	btn_incpath_add;
	wxButton*	btn_incpath_remove;
	wxListBox*	list_inc_paths;

public:
	ACSPrefsPanel(wxWindow* parent);
	~ACSPrefsPanel();

	void	init();
	void	applyPreferences();

	// Events
	void	onBtnBrowseACCPath(wxCommandEvent& e);
	void	onBtnAddIncPath(wxCommandEvent& e);
	void	onBtnRemoveIncPath(wxCommandEvent& e);
};

#endif//__ACS_PREFS_PANEL_H__
