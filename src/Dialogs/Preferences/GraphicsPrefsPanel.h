#pragma once

#include "PrefsPanelBase.h"

class wxColourPickerCtrl;

class GraphicsPrefsPanel : public PrefsPanelBase
{
public:
	GraphicsPrefsPanel(wxWindow* parent);
	~GraphicsPrefsPanel();

	void init() override;
	void applyPreferences() override;

private:
	wxColourPickerCtrl* cp_colour1_;
	wxColourPickerCtrl* cp_colour2_;
	wxChoice*           choice_presets_;
	wxCheckBox*         cb_show_border_;
	wxCheckBox*         cb_extra_gfxconv_;
	wxChoice*           choice_browser_bg_;
	wxCheckBox*         cb_hilight_mouseover_;

	void setupLayout();

	// Events
	void onChoicePresetSelected(wxCommandEvent& e);
};
