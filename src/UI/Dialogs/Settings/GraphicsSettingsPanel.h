#pragma once

#include "SettingsPanel.h"

class wxColourPickerCtrl;
namespace slade
{
class FileLocationPanel;
}

namespace slade::ui
{
class ColorimetrySettingsPanel;

class GraphicsSettingsPanel : public SettingsPanel
{
public:
	GraphicsSettingsPanel(wxWindow* parent);
	~GraphicsSettingsPanel() override = default;

	string title() const override { return "Graphics Settings"; }

	void applySettings() override;

private:
	// General
	wxColourPickerCtrl* cp_colour1_              = nullptr;
	wxColourPickerCtrl* cp_colour2_              = nullptr;
	wxChoice*           choice_presets_          = nullptr;
	wxCheckBox*         cb_show_border_          = nullptr;
	wxCheckBox*         cb_extra_gfxconv_        = nullptr;
	wxChoice*           choice_browser_bg_       = nullptr;
	wxCheckBox*         cb_hilight_mouseover_    = nullptr;
	wxCheckBox*         cb_condensed_trans_edit_ = nullptr;

	// PNG
	FileLocationPanel* flp_pngout_   = nullptr;
	FileLocationPanel* flp_pngcrush_ = nullptr;
	FileLocationPanel* flp_deflopt_  = nullptr;

	// Hud Offsets View
	wxCheckBox* cb_hud_bob_       = nullptr;
	wxCheckBox* cb_hud_center_    = nullptr;
	wxCheckBox* cb_hud_statusbar_ = nullptr;
	wxCheckBox* cb_hud_wide_      = nullptr;

	// Colorimetry
	ColorimetrySettingsPanel* colorimetry_panel_ = nullptr;

	void init() const;

	wxPanel* createGeneralPanel(wxWindow* parent);
	wxPanel* createPngPanel(wxWindow* parent);

	// Events
	void onChoicePresetSelected(wxCommandEvent& e);
};
} // namespace slade::ui
