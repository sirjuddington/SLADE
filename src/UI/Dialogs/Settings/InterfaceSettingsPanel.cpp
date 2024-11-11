
#include "Main.h"
#include "InterfaceSettingsPanel.h"
#include "ColourSettingsPanel.h"
#include "Graphics/Icons.h"
#include "UI/Controls/STabCtrl.h"
#include "UI/Layout.h"
#include "UI/UI.h"
#include "UI/WxUtils.h"

using namespace slade;
using namespace ui;

InterfaceSettingsPanel::InterfaceSettingsPanel(wxWindow* parent) : SettingsPanel(parent)
{
	auto lh    = LayoutHelper(this);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	auto tabs = STabCtrl::createControl(this);
	tabs->AddPage(createInterfacePanel(tabs), "Interface");
	tabs->AddPage(colour_panel_ = new ColourSettingsPanel(tabs), "Colours && Theme");
	sizer->Add(tabs, wxSizerFlags(1).Expand());
}

void InterfaceSettingsPanel::applySettings()
{
	colour_panel_->apply();
}

wxPanel* InterfaceSettingsPanel::createInterfacePanel(wxWindow* parent)
{
	auto panel = new wxPanel(parent, -1);
	auto lh    = LayoutHelper(panel);

	// Create controls

	// Appearance
	wxString darkmodes[]     = { "Off", "Use System Setting", "On" };
	choice_windows_darkmode_ = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, 3, darkmodes);
	cb_monospace_list_       = new wxCheckBox(panel, -1, "Use monospace font in lists");
	cb_condensed_tabs_       = new wxCheckBox(panel, -1, "Condensed tabs");
	wxString sizes[]         = { "16x16", "24x24", "32x32" };
	choice_toolbar_size_     = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, 3, sizes);
	auto sets_toolbar        = wxutil::arrayStringStd(icons::iconSets(icons::General));
	choice_toolbar_iconset_  = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, sets_toolbar);

	// Entry List
	cb_filter_dirs_         = new wxCheckBox(panel, -1, "Ignore directories when filtering by name");
	cb_elist_bgcol_         = new wxCheckBox(panel, -1, "Colour entry list item background by entry type");
	auto sets_entry         = wxutil::arrayStringStd(icons::iconSets(icons::Entry));
	choice_iconset_entry_   = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, sets_entry);
	wxString icon_sizes[]   = { "16x16", "24x24", "32x32" };
	choice_elist_icon_size_ = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, 3, icon_sizes);
	spin_elist_icon_pad_    = new wxSpinCtrl(
        panel, -1, "1", wxDefaultPosition, lh.spinSize(), wxSP_ARROW_KEYS | wxTE_PROCESS_ENTER, 0, 4, 1);
	wxString tree_styles[]   = { "Tree", "Flat List" };
	choice_elist_tree_style_ = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, 2, tree_styles);

	auto sizer = new wxBoxSizer(wxHORIZONTAL);
	panel->SetSizer(sizer);

	auto vbox_left = new wxBoxSizer(wxVERTICAL);
	sizer->Add(vbox_left, lh.sfWithLargeBorder(1).Expand());

	// Appearance settings group
	auto sbs_appearnace = new wxStaticBoxSizer(wxVERTICAL, panel, "Appearance");
	sbs_appearnace->Add(layoutAppearanceSettings(panel), lh.sfWithBorder(1).Expand());
	vbox_left->Add(sbs_appearnace, wxSizerFlags().Expand());

	auto vbox_right = new wxBoxSizer(wxVERTICAL);
	sizer->Add(vbox_right, lh.sfWithLargeBorder(1, wxTOP | wxBOTTOM | wxRIGHT).Expand());

	// Entry List settings group
	auto sbs_entrylist = new wxStaticBoxSizer(wxVERTICAL, panel, "Entry List");
	sbs_entrylist->Add(layoutEntryListSettings(panel), lh.sfWithBorder(1).Expand());
	vbox_right->Add(sbs_entrylist, wxSizerFlags().Expand());

	return panel;
}

wxSizer* InterfaceSettingsPanel::layoutAppearanceSettings(wxWindow* panel) const
{
	auto sizer = new wxGridBagSizer(ui::pad(), ui::padLarge());

	sizer->Add(
		new wxStaticText(panel, -1, "Use dark UI theme if supported"), { 0, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(choice_windows_darkmode_, { 0, 1 }, { 1, 2 }, wxEXPAND);
	sizer->Add(new wxStaticText(panel, -1, "Toolbar icon set"), { 1, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(choice_toolbar_iconset_, { 1, 1 }, { 1, 1 }, wxEXPAND);
	sizer->Add(choice_toolbar_size_, { 1, 2 }, { 1, 1 }, wxEXPAND);
	sizer->Add(cb_monospace_list_, { 2, 0 }, { 1, 3 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(cb_condensed_tabs_, { 3, 0 }, { 1, 3 }, wxALIGN_CENTER_VERTICAL);

	sizer->AddGrowableCol(1);
	sizer->AddGrowableCol(2);

	return sizer;
}

wxSizer* InterfaceSettingsPanel::layoutEntryListSettings(wxWindow* panel) const
{
	auto sizer = new wxGridBagSizer(ui::pad(), ui::padLarge());

	auto row = 0;
	sizer->Add(new wxStaticText(panel, -1, "Icon set"), { row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(choice_iconset_entry_, { row, 1 }, { 1, 1 }, wxEXPAND);
	sizer->Add(choice_elist_icon_size_, { row++, 2 }, { 1, 1 }, wxEXPAND);
	sizer->Add(new wxStaticText(panel, -1, "Row spacing"), { row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(spin_elist_icon_pad_, { row++, 1 }, { 1, 2 }, wxEXPAND);
	sizer->Add(new wxStaticText(panel, -1, "Folder List style"), { row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(choice_elist_tree_style_, { row++, 1 }, { 1, 2 }, wxEXPAND);
	sizer->Add(cb_filter_dirs_, { row++, 0 }, { 1, 3 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(cb_elist_bgcol_, { row++, 0 }, { 1, 3 }, wxALIGN_CENTER_VERTICAL);

	sizer->AddGrowableCol(1);

	return sizer;
}
