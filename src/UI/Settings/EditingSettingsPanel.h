#pragma once

#include "SettingsPanel.h"

namespace slade
{
class VirtualListView;
namespace ui
{
	class ExternalEditorsSettingsPanel;
	class RadioButtonPanel;
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
	wxCheckBox*       cb_wad_force_uppercase_  = nullptr;
	wxCheckBox*       cb_zip_percent_encoding_ = nullptr;
	wxCheckBox*       cb_auto_entry_replace_   = nullptr;
	wxCheckBox*       cb_filter_dirs_          = nullptr;
	RadioButtonPanel* rbp_entry_mod_           = nullptr;
	wxCheckBox*       cb_confirm_entry_delete_ = nullptr;
	wxCheckBox*       cb_confirm_entry_revert_ = nullptr;
	RadioButtonPanel* rbp_dir_mod_             = nullptr;

	ExternalEditorsSettingsPanel* ext_editors_panel_ = nullptr;

	wxPanel* createArchiveEditorPanel(wxWindow* parent);
	wxPanel* createExternalEditorsPanel(wxWindow* parent);
};
} // namespace slade::ui
