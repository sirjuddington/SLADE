#pragma once

#include "SettingsPanel.h"

namespace slade::ui
{
class SettingsTable;
class TextureEditorSettingsPanel : public SettingsPanel
{
public:
	TextureEditorSettingsPanel(wxWindow* parent);
	~TextureEditorSettingsPanel() override = default;

	string title() const override { return "Texture Editor Settings"; }

	void loadSettings() override;
	void applySettings() override;

private:
	SettingsTable* settings_general_ = nullptr;
};
} // namespace slade::ui
