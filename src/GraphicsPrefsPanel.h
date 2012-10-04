
#ifndef __GFX_PREFS_PANEL_H__
#define __GFX_PREFS_PANEL_H__

#include "PrefsPanelBase.h"
#include <wx/clrpicker.h>

class GraphicsPrefsPanel : public PrefsPanelBase {
private:
	wxColourPickerCtrl*	cp_colour1;
	wxColourPickerCtrl*	cp_colour2;
	wxChoice*			choice_presets;
	wxCheckBox*			cb_show_border;
	wxCheckBox*			cb_extra_gfxconv;
	wxChoice*			choice_browser_bg;
	wxCheckBox*			cb_hilight_mouseover;

public:
	GraphicsPrefsPanel(wxWindow* parent);
	~GraphicsPrefsPanel();

	void	init();
	void	applyPreferences();

	// Events
	void	onChoicePresetSelected(wxCommandEvent& e);
};

#endif//__GFX_PREFS_PANEL_H__
