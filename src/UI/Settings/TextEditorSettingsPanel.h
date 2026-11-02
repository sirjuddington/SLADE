#pragma once

#include "SettingsPanel.h"

namespace slade::ui
{
class RadioButtonPanel;
class TextEditorStyleSettingsPanel;
class SettingsTable;

class TextEditorSettingsPanel : public SettingsPanel
{
public:
	TextEditorSettingsPanel(wxWindow* parent);
	~TextEditorSettingsPanel() override = default;

	string title() const override { return "Text Editor Settings"; }

	void loadSettings() override;
	void applySettings() override;

private:
	RadioButtonPanel*             rbp_minimap_      = nullptr;
	SettingsTable*                settings_general_ = nullptr;
	SettingsTable*                settings_code_    = nullptr;
	TextEditorStyleSettingsPanel* style_panel_      = nullptr;

	wxPanel* createSettingsPanel(wxWindow* parent);
	wxPanel* createCodePanel(wxWindow* parent);
};
} // namespace slade::ui
