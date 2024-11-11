#pragma once

#include "SettingsPanel.h"

namespace slade::ui
{
class ColourSettingsPanel;

class InterfaceSettingsPanel : public SettingsPanel
{
public:
	InterfaceSettingsPanel(wxWindow* parent);

	void applySettings() override;

private:
	wxChoice*            choice_windows_darkmode_ = nullptr;
	wxCheckBox*          cb_monospace_list_       = nullptr;
	wxCheckBox*          cb_condensed_tabs_       = nullptr;
	wxChoice*            choice_toolbar_iconset_  = nullptr;
	wxChoice*            choice_toolbar_size_     = nullptr;
	wxCheckBox*          cb_filter_dirs_          = nullptr;
	wxCheckBox*          cb_elist_bgcol_          = nullptr;
	wxChoice*            choice_iconset_entry_    = nullptr;
	wxChoice*            choice_elist_icon_size_  = nullptr;
	wxSpinCtrl*          spin_elist_icon_pad_     = nullptr;
	wxChoice*            choice_elist_tree_style_ = nullptr;
	ColourSettingsPanel* colour_panel_            = nullptr;

	wxPanel* createInterfacePanel();
	wxSizer* layoutAppearanceSettings(wxWindow* panel) const;
	wxSizer* layoutEntryListSettings(wxWindow* panel) const;
};
} // namespace slade::ui
