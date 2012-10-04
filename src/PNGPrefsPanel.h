
#ifndef __PNG_PREFS_PANEL_H__
#define __PNG_PREFS_PANEL_H__

#include "PrefsPanelBase.h"

class PNGPrefsPanel : public PrefsPanelBase {
private:
	wxTextCtrl*	text_pngoutpath;
	wxTextCtrl*	text_pngcrushpath;
	wxTextCtrl*	text_defloptpath;
	wxButton*	btn_browse_pngoutpath;
	wxButton*	btn_browse_pngcrushpath;
	wxButton*	btn_browse_defloptpath;

public:
	PNGPrefsPanel(wxWindow* parent);
	~PNGPrefsPanel();

	void	init();
	void	applyPreferences();

	// Events
	void	onBtnBrowsePNGoutPath(wxCommandEvent& e);
	void	onBtnBrowsePNGCrushPath(wxCommandEvent& e);
	void	onBtnBrowseDeflOptPath(wxCommandEvent& e);
};

#endif//__PNG_PREFS_PANEL_H__
