#pragma once

#include "PrefsPanelBase.h"

class wxColourPickerCtrl;

class GraphicsPrefsPanel : public PrefsPanelBase
{
public:
	GraphicsPrefsPanel(wxWindow* parent);
	~GraphicsPrefsPanel() = default;

	void init() override;
	void applyPreferences() override;

private:
	wxColourPickerCtrl* cp_colour1_           = nullptr;
	wxColourPickerCtrl* cp_colour2_           = nullptr;
	wxChoice*           choice_presets_       = nullptr;
	wxCheckBox*         cb_show_border_       = nullptr;
	wxCheckBox*         cb_extra_gfxconv_     = nullptr;
	wxChoice*           choice_browser_bg_    = nullptr;
	wxCheckBox*         cb_hilight_mouseover_ = nullptr;

	void setupLayout();

	// Events
	void onChoicePresetSelected(wxCommandEvent& e);
};
