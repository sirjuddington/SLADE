#pragma once

#include "SettingsPanel.h"

namespace slade
{
class VirtualListView;
}

namespace slade::ui
{
class ExternalEditorsSettingsPanel : public SettingsPanel
{
public:
	ExternalEditorsSettingsPanel(wxWindow* parent);
	~ExternalEditorsSettingsPanel() override = default;

	string title() const override { return "External Editors"; }

	void loadSettings() override;
	void applySettings() override;

private:
	VirtualListView* lv_ext_editors_  = nullptr;
	wxChoice*        choice_category_ = nullptr;
	wxBitmapButton*  btn_add_exe_     = nullptr;
	wxBitmapButton*  btn_remove_exe_  = nullptr;

	// Events
	void onBtnAddClicked(wxCommandEvent& e);
	void onBtnRemoveClicked(wxCommandEvent& e);
	void onExternalExeActivated(wxListEvent& e);
};
} // namespace slade::ui
