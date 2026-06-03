#pragma once

#include "SettingsPanel.h"

namespace slade::ui
{
class ColourSettingsPanel;
class SettingsTable;

class InterfaceSettingsPanel : public SettingsPanel
{
public:
	InterfaceSettingsPanel(wxWindow* parent);

	string title() const override { return "Interface Settings"; }

	void loadSettings() override;
	void applySettings() override;

private:
	wxChoice*            choice_toolbar_iconset_ = nullptr;
	wxChoice*            choice_toolbar_size_    = nullptr;
	wxChoice*            choice_iconset_entry_   = nullptr;
	wxChoice*            choice_elist_icon_size_ = nullptr;
	SettingsTable*       settings_table_         = nullptr;
	ColourSettingsPanel* colour_panel_           = nullptr;

	void setupInterfaceSettingsTable();
};
} // namespace slade::ui
