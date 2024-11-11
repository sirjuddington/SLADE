#pragma once

#include "SettingsPanel.h"

class wxPropertyGrid;

namespace slade::ui
{
class AdvancedSettingsPanel : public SettingsPanel
{
public:
	AdvancedSettingsPanel(wxWindow* parent);
	~AdvancedSettingsPanel() override = default;

	void applySettings() override;

private:
	wxPropertyGrid* pg_cvars_ = nullptr;

	void refreshPropGrid() const;
};
} // namespace slade::ui
