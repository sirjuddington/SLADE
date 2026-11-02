#pragma once

#include "SettingsPanel.h"

namespace slade
{
class VirtualListView;
namespace ui
{
	class ExternalEditorsSettingsPanel;
	class SettingsTable;
} // namespace ui
} // namespace slade

namespace slade::ui
{
class EditingSettingsPanel : public SettingsPanel
{
public:
	EditingSettingsPanel(wxWindow* parent);

	string title() const override { return "Editing Settings"; }

	void loadSettings() override;
	void applySettings() override;

private:
	SettingsTable*                settings_table_    = nullptr;
	ExternalEditorsSettingsPanel* ext_editors_panel_ = nullptr;

	wxPanel* createArchiveEditorPanel(wxWindow* parent);
};
} // namespace slade::ui
