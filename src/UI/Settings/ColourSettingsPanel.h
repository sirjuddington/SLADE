#pragma once

#include "SettingsPanel.h"

class wxPropertyGrid;

namespace slade::ui
{
class ColourSettingsPanel : public SettingsPanel
{
public:
	ColourSettingsPanel(wxWindow* parent);
	~ColourSettingsPanel() override = default;

	string icon() const override { return "palette"; }
	string title() const override { return "Colours & Theme"; }

	void loadSettings() override;
	void applySettings() override;

private:
	wxChoice*       choice_configs_ = nullptr;
	wxPropertyGrid* pg_colours_     = nullptr;

	void refreshPropGrid() const;
};
} // namespace slade::ui
