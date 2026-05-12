#pragma once

#include "SettingsPanel.h"

namespace slade::ui
{
class InputSettingsPanel : public SettingsPanel
{
public:
	InputSettingsPanel(wxWindow* parent);
	~InputSettingsPanel() override = default;

	wxTreeListItem getListGroupItem(const string& group) const;
	void           initBindsList() const;
	void           updateBindsList() const;
	void           changeKey(wxTreeListItem item);
	void           addKey();
	void           removeKey(wxTreeListItem item) const;

	string title() const override { return "Keyboard Shortcuts"; }

	void loadSettings() override { updateBindsList(); }
	void applySettings() override;

private:
	wxTreeListCtrl* list_binds_   = nullptr;
	wxButton*       btn_add_      = nullptr;
	wxButton*       btn_remove_   = nullptr;
	wxButton*       btn_change_   = nullptr;
	wxButton*       btn_defaults_ = nullptr;

	// Events
	void onSize(wxSizeEvent& e);
	void onListSelectionChanged(wxTreeListEvent& e);
	void onListItemActivated(wxTreeListEvent& e);
	void onBtnChangeKey(wxCommandEvent& e);
	void onBtnDefaults(wxCommandEvent& e);
	void onListKeyDown(wxKeyEvent& e);
};
} // namespace slade::ui
