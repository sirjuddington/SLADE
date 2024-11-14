#pragma once

#include "SettingsPanel.h"

namespace slade::ui
{
class ColourSettingsPanel;
class RadioButtonPanel;

class InterfaceSettingsPanel : public SettingsPanel
{
public:
	InterfaceSettingsPanel(wxWindow* parent);

	string title() const override { return "Interface Settings"; }

	void applySettings() override;

private:
	RadioButtonPanel*    rbp_windows_darkmode_   = nullptr;
	wxCheckBox*          cb_monospace_list_      = nullptr;
	wxCheckBox*          cb_condensed_tabs_      = nullptr;
	wxChoice*            choice_toolbar_iconset_ = nullptr;
	wxChoice*            choice_toolbar_size_    = nullptr;
	wxCheckBox*          cb_elist_bgcol_         = nullptr;
	wxChoice*            choice_iconset_entry_   = nullptr;
	wxChoice*            choice_elist_icon_size_ = nullptr;
	wxSpinCtrl*          spin_elist_icon_pad_    = nullptr;
	RadioButtonPanel*    rbp_elist_tree_style_   = nullptr;
	ColourSettingsPanel* colour_panel_           = nullptr;

	wxPanel* createInterfacePanel(wxWindow* parent);
	wxSizer* layoutAppearanceSettings(wxWindow* panel) const;
	wxSizer* layoutEntryListSettings(wxWindow* panel) const;
};
} // namespace slade::ui
