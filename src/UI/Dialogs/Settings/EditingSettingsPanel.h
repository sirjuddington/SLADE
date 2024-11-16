#pragma once

#include "SettingsPanel.h"

namespace slade
{
class VirtualListView;
namespace ui
{
	class RadioButtonPanel;
}
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
	// Archive Editor
	wxCheckBox*       cb_wad_force_uppercase_  = nullptr;
	wxCheckBox*       cb_zip_percent_encoding_ = nullptr;
	wxCheckBox*       cb_auto_entry_replace_   = nullptr;
	wxCheckBox*       cb_filter_dirs_          = nullptr;
	RadioButtonPanel* rbp_entry_mod_           = nullptr;
	wxCheckBox*       cb_confirm_entry_delete_ = nullptr;
	wxCheckBox*       cb_confirm_entry_revert_ = nullptr;
	RadioButtonPanel* rbp_dir_mod_             = nullptr;

	// External editors
	VirtualListView* lv_ext_editors_  = nullptr;
	wxChoice*        choice_category_ = nullptr;
	wxBitmapButton*  btn_add_exe_     = nullptr;
	wxBitmapButton*  btn_remove_exe_  = nullptr;

	wxPanel* createArchiveEditorPanel(wxWindow* parent);
	wxPanel* createExternalEditorsPanel(wxWindow* parent);

	// Events
	void onBtnAddClicked(wxCommandEvent& e);
	void onBtnRemoveClicked(wxCommandEvent& e);
	void onExternalExeActivated(wxListEvent& e);
};
} // namespace slade::ui
