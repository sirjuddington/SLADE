#pragma once

#include "SettingsPanel.h"

namespace slade::ui
{
class BaseResourceArchiveSettingsPanel;
class SettingsTable;

class GeneralSettingsPanel : public SettingsPanel
{
public:
	GeneralSettingsPanel(wxWindow* parent);
	~GeneralSettingsPanel() override = default;

	string title() const override { return "General Settings"; }

	void loadSettings() override;
	void applySettings() override;

private:
	SettingsTable*                    settings_table_      = nullptr;
	BaseResourceArchiveSettingsPanel* base_resource_panel_ = nullptr;

	void     createProgramSettingsTable(wxWindow* parent);
	wxPanel* createBaseResourceArchivePanel(wxWindow* parent);
};
} // namespace slade::ui
