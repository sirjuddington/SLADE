#pragma once

#include "SettingsPanel.h"

namespace slade
{
class NumberTextCtrl;
namespace ui
{
	class NodeBuildersSettingsPanel;
	class SettingsTable;
} // namespace ui
} // namespace slade

namespace slade::ui
{
class MapGeneralSettingsPanel : public SettingsPanel
{
public:
	MapGeneralSettingsPanel(wxWindow* parent);
	~MapGeneralSettingsPanel() override = default;

	string title() const override { return "Map Editor Settings"; }

	void loadSettings() override;
	void applySettings() override;

private:
	SettingsTable*             settings_table_     = nullptr;
	NumberTextCtrl*            text_max_backups_   = nullptr;
	NodeBuildersSettingsPanel* nodebuilders_panel_ = nullptr;

	wxPanel* createGeneralPanel(wxWindow* parent);
};
} // namespace slade::ui
