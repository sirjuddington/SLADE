#pragma once

#include "SettingsPanel.h"

namespace slade::ui
{
class GeneralSettingsPanel : public SettingsPanel
{
public:
	GeneralSettingsPanel(wxWindow* parent);
	~GeneralSettingsPanel() override = default;

	void applySettings() override;
};
} // namespace slade::ui
