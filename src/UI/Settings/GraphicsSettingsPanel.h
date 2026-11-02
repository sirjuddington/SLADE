#pragma once

#include "SettingsPanel.h"

class wxColourPickerCtrl;
namespace slade
{
class FileLocationPanel;
}

namespace slade::ui
{
class SettingsTable;
class ColorimetrySettingsPanel;

class GraphicsSettingsPanel : public SettingsPanel
{
public:
	GraphicsSettingsPanel(wxWindow* parent);
	~GraphicsSettingsPanel() override = default;

	string title() const override { return "Graphics Settings"; }

	void loadSettings() override;
	void applySettings() override;

private:
	// General
	SettingsTable*      settings_table_ = nullptr;
	wxColourPickerCtrl* cp_colour1_     = nullptr;
	wxColourPickerCtrl* cp_colour2_     = nullptr;
	wxChoice*           choice_presets_ = nullptr;

	// PNG
	FileLocationPanel* flp_pngout_   = nullptr;
	FileLocationPanel* flp_pngcrush_ = nullptr;
	FileLocationPanel* flp_deflopt_  = nullptr;

	// Colorimetry
	ColorimetrySettingsPanel* colorimetry_panel_ = nullptr;

	wxPanel* createGeneralPanel(wxWindow* parent);
	wxPanel* createPngPanel(wxWindow* parent);

	// Events
	void onChoicePresetSelected(wxCommandEvent& e);
};
} // namespace slade::ui
