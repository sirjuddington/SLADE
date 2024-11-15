
#include "Main.h"
#include "InterfaceSettingsPanel.h"
#include "ColourSettingsPanel.h"
#include "Graphics/Icons.h"
#include "UI/Controls/RadioButtonPanel.h"
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
	vector<wxString> darkmodes = { "Off", "Use System Setting", "On" };
	rbp_windows_darkmode_      = new RadioButtonPanel(panel, darkmodes, "Use dark UI theme if supported:");
	cb_monospace_list_         = new wxCheckBox(panel, -1, "Use monospace font in lists");
	cb_condensed_tabs_         = new wxCheckBox(panel, -1, "Condensed tabs");
	wxString sizes[]           = { "16x16", "24x24", "32x32" };
	choice_toolbar_size_       = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, 3, sizes);
	auto sets_toolbar          = wxutil::arrayStringStd(icons::iconSets(icons::General));
	choice_toolbar_iconset_    = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, sets_toolbar);

	// Entry List
	cb_elist_bgcol_         = new wxCheckBox(panel, -1, "Colour entry list item background by entry type");
	auto sets_entry         = wxutil::arrayStringStd(icons::iconSets(icons::Entry));
	choice_iconset_entry_   = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, sets_entry);
	wxString icon_sizes[]   = { "16x16", "24x24", "32x32" };
	choice_elist_icon_size_ = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, 3, icon_sizes);
	spin_elist_icon_pad_    = new wxSpinCtrl(
        panel, -1, "1", wxDefaultPosition, lh.spinSize(), wxSP_ARROW_KEYS | wxTE_PROCESS_ENTER, 0, 4, 1);
	vector<wxString> tree_styles = { "Tree", "Flat List" };
	rbp_elist_tree_style_        = new RadioButtonPanel(
        panel, tree_styles, "Entry list style for archives that allow folders:");

	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	auto vbox = new wxBoxSizer(wxVERTICAL);
	sizer->Add(vbox, lh.sfWithLargeBorder(0).Expand());

	// Appearance settings
	vbox->Add(wxutil::createSectionSeparator(panel, "Appearance"), lh.sfWithBorder(0, wxBOTTOM).Expand());
	vbox->Add(layoutAppearanceSettings(panel), lh.sfWithBorder(0, wxLEFT));

	// Entry List settings
	vbox->AddSpacer(lh.padXLarge());
	vbox->Add(wxutil::createSectionSeparator(panel, "Entry List"), lh.sfWithBorder(0, wxBOTTOM).Expand());
	vbox->Add(layoutEntryListSettings(panel), lh.sfWithBorder(0, wxLEFT));

	return panel;
}

wxSizer* InterfaceSettingsPanel::layoutAppearanceSettings(wxWindow* panel) const
{
	auto sizer = new wxGridBagSizer(pad(panel), padLarge(panel));

	int row = 0;
#ifdef __WXMSW__
	sizer->Add(rbp_windows_darkmode_, { row++, 0 }, { 1, 3 }, wxEXPAND);
#else
	rbp_windows_darkmode_->Hide();
#endif
	sizer->Add(new wxStaticText(panel, -1, "Toolbar icon set:"), { row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(choice_toolbar_iconset_, { row, 1 }, { 1, 1 }, wxEXPAND);
	sizer->Add(choice_toolbar_size_, { row++, 2 }, { 1, 1 }, wxEXPAND);
	sizer->Add(cb_monospace_list_, { row++, 0 }, { 1, 3 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(cb_condensed_tabs_, { row, 0 }, { 1, 3 }, wxALIGN_CENTER_VERTICAL);

	sizer->AddGrowableCol(1);
	sizer->AddGrowableCol(2);

	return sizer;
}

wxSizer* InterfaceSettingsPanel::layoutEntryListSettings(wxWindow* panel) const
{
	auto sizer = new wxGridBagSizer(pad(panel), padLarge(panel));

	auto row = 0;
	sizer->Add(new wxStaticText(panel, -1, "Icon set:"), { row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(choice_iconset_entry_, { row, 1 }, { 1, 1 }, wxEXPAND);
	sizer->Add(choice_elist_icon_size_, { row++, 2 }, { 1, 1 }, wxEXPAND);
	sizer->Add(new wxStaticText(panel, -1, "Row spacing:"), { row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(spin_elist_icon_pad_, { row++, 1 }, { 1, 2 }, wxEXPAND);
	sizer->Add(rbp_elist_tree_style_, { row++, 0 }, { 1, 3 }, wxEXPAND);
	sizer->Add(cb_elist_bgcol_, { row++, 0 }, { 1, 3 }, wxALIGN_CENTER_VERTICAL);

	sizer->AddGrowableCol(1);

	return sizer;
}
