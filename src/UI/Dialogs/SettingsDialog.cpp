
#include "Main.h"
#include "SettingsDialog.h"
#include "App.h"
#include "MainEditor/MainEditor.h"
#include "UI/Layout.h"
#include "UI/SToolBar/SToolBarButton.h"
#include "UI/Settings/AdvancedSettingsPanel.h"
#include "UI/Settings/AudioSettingsPanel.h"
#include "UI/Settings/BaseResourceArchiveSettingsPanel.h"
#include "UI/Settings/ColorimetrySettingsPanel.h"
#include "UI/Settings/ColourSettingsPanel.h"
#include "UI/Settings/EditingSettingsPanel.h"
#include "UI/Settings/ExternalEditorsSettingsPanel.h"
#include "UI/Settings/GeneralSettingsPanel.h"
#include "UI/Settings/GraphicsSettingsPanel.h"
#include "UI/Settings/InputSettingsPanel.h"
#include "UI/Settings/InterfaceSettingsPanel.h"
#include "UI/Settings/Map3DSettingsPanel.h"
#include "UI/Settings/MapDisplaySettingsPanel.h"
#include "UI/Settings/MapGeneralSettingsPanel.h"
#include "UI/Settings/NodeBuildersSettingsPanel.h"
#include "UI/Settings/ScriptSettingsPanel.h"
#include "UI/Settings/TextEditorSettingsPanel.h"
#include "UI/Settings/TextEditorStyleSettingsPanel.h"
#include "UI/WxUtils.h"
#include "Utility/StringUtils.h"

using namespace slade;
using namespace ui;


namespace
{
wxColour sidePanelColour()
{
	auto bgcol = wxutil::systemPanelBGColour();
	return app::isDarkTheme() ? bgcol.ChangeLightness(105) : bgcol.ChangeLightness(95);
}

void updateMinSize(const wxWindow* window, int& min_width, int& min_height)
{
	auto size  = window->GetBestSize();
	min_width  = std::max(min_width, size.GetWidth());
	min_height = std::max(min_height, size.GetHeight());
}
} // namespace




SettingsDialog::SettingsDialog(wxWindow* parent, SettingsPage initial_page) :
	SDialog(parent, "SLADE Settings", "settings")
{
	auto lh = LayoutHelper(this);

	// Set icon
	wxutil::setWindowIcon(this, "settings");

	// Setup main sizer
	auto sizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(sizer);

	// Sections
	auto sections_panel = createSectionsPanel();
	sizer->Add(sections_panel, wxSizerFlags(0).Expand());

	content_sizer_ = new wxBoxSizer(wxVERTICAL);
	sizer->Add(content_sizer_, wxSizerFlags(1).Expand());

	// Title
	auto title_panel = new wxPanel(this);
	auto title_sizer = new wxBoxSizer(wxHORIZONTAL);
	title_panel->SetSizer(title_sizer);
	title_text_ = new wxStaticText(title_panel, -1, "Title");
	title_text_->SetFont(GetFont().MakeLarger().MakeLarger().Bold());
	title_sizer->Add(title_text_, lh.sfWithLargeBorder(1, wxLEFT | wxTOP).Expand());
	content_sizer_->Add(title_panel, wxSizerFlags().Expand());

	// Settings pages
	settings_pages_[static_cast<size_t>(SettingsPage::General)]    = new GeneralSettingsPanel(this);
	settings_pages_[static_cast<size_t>(SettingsPage::Interface)]  = new InterfaceSettingsPanel(this);
	settings_pages_[static_cast<size_t>(SettingsPage::Keybinds)]   = new InputSettingsPanel(this);
	settings_pages_[static_cast<size_t>(SettingsPage::Editing)]    = new EditingSettingsPanel(this);
	settings_pages_[static_cast<size_t>(SettingsPage::Text)]       = new TextEditorSettingsPanel(this);
	settings_pages_[static_cast<size_t>(SettingsPage::Graphics)]   = new GraphicsSettingsPanel(this);
	settings_pages_[static_cast<size_t>(SettingsPage::Audio)]      = new AudioSettingsPanel(this);
	settings_pages_[static_cast<size_t>(SettingsPage::Scripting)]  = new ScriptSettingsPanel(this);
	settings_pages_[static_cast<size_t>(SettingsPage::MapGeneral)] = new MapGeneralSettingsPanel(this);
	settings_pages_[static_cast<size_t>(SettingsPage::MapDisplay)] = new MapDisplaySettingsPanel(this);
	settings_pages_[static_cast<size_t>(SettingsPage::Advanced)]   = new AdvancedSettingsPanel(this);

	// Show initial settings panel
	auto init_panel = settingsPanel(initial_page);
	sectionButton(initial_page)->setChecked(true);
	content_sizer_->Add(init_panel, lh.sfWithLargeBorder(1).Expand());
	init_panel->Show();
	current_page_ = init_panel;
	title_text_->SetLabel(init_panel->title());

	// Load settings
	for (auto* page : settings_pages_)
		page->loadSettings();

	// Dialog Buttons
	auto button_sizer = new wxBoxSizer(wxHORIZONTAL);
	button_sizer->Add(btn_apply_ = new wxButton(this, -1, "Apply"), wxSizerFlags(0).Expand());
	button_sizer->AddStretchSpacer();
	button_sizer->Add(btn_ok_ = new wxButton(this, -1, "OK"), lh.sfWithBorder(0, wxRIGHT).Expand());
	button_sizer->Add(btn_cancel_ = new wxButton(this, -1, "Cancel"), wxSizerFlags(0).Expand());
	content_sizer_->Add(button_sizer, lh.sfWithLargeBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Expand());

	// Bind events
	Bind(wxEVT_STOOLBAR_BUTTON_CLICKED, &SettingsDialog::onSectionButtonClicked, this);
	btn_apply_->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { applySettings(); });
	btn_ok_->Bind(
		wxEVT_BUTTON,
		[&](wxCommandEvent&)
		{
			applySettings();
			EndModal(wxID_OK);
		});
	btn_cancel_->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { EndModal(wxID_CANCEL); });

	// Determine best minimum size based on larger pages
	int min_width  = 0;
	int min_height = 0;
	for (const auto* page : settings_pages_)
		updateMinSize(page, min_width, min_height);
	SetMinSize({ sections_panel->GetBestSize().x + min_width + FromDIP(100),
				 min_height + button_sizer->CalcMin().y + title_panel->GetBestSize().y + FromDIP(100) });
}

void SettingsDialog::applySettings() const
{
	// Apply settings from all pages (except advanced)
	auto advanced_page = settingsPanel(SettingsPage::Advanced);
	for (auto* page : settings_pages_)
		if (page != advanced_page)
			page->applySettings();

	// Apply advanced settings last
	advanced_page->applySettings();
}

// -----------------------------------------------------------------------------
// Opens a dialog containing a [page] settings panel and OK/Cancel buttons.
// Clicking OK will apply the settings and return true, clicking Cancel will
// return false
// -----------------------------------------------------------------------------
bool SettingsDialog::popupSettingsPage(wxWindow* parent, SettingsPage page)
{
	if (!parent)
		parent = maineditor::windowWx();

	SDialog dlg(parent, "Settings", fmt::format("settings_{}", pageId(page)));

	// Create settings page
	SettingsPanel* settings_panel;
	switch (page)
	{
	case SettingsPage::General:         settings_panel = new GeneralSettingsPanel(&dlg); break;
	case SettingsPage::Interface:       settings_panel = new InterfaceSettingsPanel(&dlg); break;
	case SettingsPage::Keybinds:        settings_panel = new InputSettingsPanel(&dlg); break;
	case SettingsPage::Editing:         settings_panel = new EditingSettingsPanel(&dlg); break;
	case SettingsPage::Text:            settings_panel = new TextEditorSettingsPanel(&dlg); break;
	case SettingsPage::Graphics:        settings_panel = new GraphicsSettingsPanel(&dlg); break;
	case SettingsPage::Audio:           settings_panel = new AudioSettingsPanel(&dlg); break;
	case SettingsPage::Scripting:       settings_panel = new ScriptSettingsPanel(&dlg); break;
	case SettingsPage::MapGeneral:      settings_panel = new MapGeneralSettingsPanel(&dlg); break;
	case SettingsPage::MapDisplay:      settings_panel = new MapDisplaySettingsPanel(&dlg); break;
	case SettingsPage::Advanced:        settings_panel = new AdvancedSettingsPanel(&dlg); break;
	case SettingsPage::BaseResource:    settings_panel = new BaseResourceArchiveSettingsPanel(&dlg); break;
	case SettingsPage::Colour:          settings_panel = new ColourSettingsPanel(&dlg); break;
	case SettingsPage::Colorimetry:     settings_panel = new ColorimetrySettingsPanel(&dlg); break;
	case SettingsPage::TextStyle:       settings_panel = new TextEditorStyleSettingsPanel(&dlg); break;
	case SettingsPage::Map3d:           settings_panel = new Map3DSettingsPanel(&dlg); break;
	case SettingsPage::NodeBuilders:    settings_panel = new NodeBuildersSettingsPanel(&dlg); break;
	case SettingsPage::ExternalEditors: settings_panel = new ExternalEditorsSettingsPanel(&dlg); break;
	default:                            return false;
	}
	dlg.SetTitle(settings_panel->title());

	// Layout dialog
	auto lh    = LayoutHelper(&dlg);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	dlg.SetSizer(sizer);
	wxutil::setWindowIcon(&dlg, settings_panel->icon());
	sizer->Add(settings_panel, lh.sfWithLargeBorder(1).Expand());
	sizer->Add(wxutil::createDialogButtonBox(&dlg), lh.sfWithLargeBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Expand());
	settings_panel->loadSettings();
	settings_panel->Show();
	dlg.SetMinSize(dlg.GetBestSize().Scale(1.2, 1.1));

	// Show dialog
	dlg.CenterOnParent();
	if (dlg.ShowModal() == wxID_OK)
	{
		settings_panel->applySettings();
		return true;
	}

	return false;
}

void SettingsDialog::reloadSettings() const
{
	for (auto* page : settings_pages_)
		page->loadSettings();
}

void SettingsDialog::createSectionButton(wxWindow* parent, SettingsPage page, const string& text, const string& icon)
{
	auto btn = new SToolBarButton(parent, pageId(page), text, icon, text, true, 24);
	btn->setPadding(4, 1);
	btn->setTextOffset(8);
	btn->setExactFit(false);
	btn->setFontSize(1.1f);
	btn->SetBackgroundColour(sidePanelColour());
	btn->setFillChecked(true);

	section_buttons_[static_cast<size_t>(page)] = btn;
}

wxPanel* SettingsDialog::createSectionsPanel()
{
	auto panel = new wxPanel(this);
	auto lh    = LayoutHelper(panel);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	auto vbox = new wxBoxSizer(wxVERTICAL);
	sizer->Add(vbox, lh.sfWithLargeBorder(1).Expand());

	// Create section buttons
	createSectionButton(panel, SettingsPage::General, "General", "logo");
	createSectionButton(panel, SettingsPage::Interface, "Interface", "sliders");
	createSectionButton(panel, SettingsPage::Keybinds, "Keyboard Shortcuts", "keyboard");
	createSectionButton(panel, SettingsPage::Editing, "Editing", "wrench");
	createSectionButton(panel, SettingsPage::Text, "Text Editor", "text");
	createSectionButton(panel, SettingsPage::Graphics, "Graphics", "gfx");
	createSectionButton(panel, SettingsPage::Audio, "Audio", "sound");
	createSectionButton(panel, SettingsPage::Scripting, "ACS Scripts", "script");
	createSectionButton(panel, SettingsPage::MapGeneral, "Map Editor", "mapeditor");
	createSectionButton(panel, SettingsPage::MapDisplay, "Map Editor Display", "flat_t");
	createSectionButton(panel, SettingsPage::Advanced, "Advanced", "settings");

	// Set all to width of 'Keyboard Shortcuts' button since it's the widest
	auto keybinds_button = sectionButton(SettingsPage::Keybinds);
	keybinds_button->setExactFit(true);
	auto width = keybinds_button->GetMinSize().GetWidth();
	for (auto* btn : section_buttons_)
		if (btn != keybinds_button)
			btn->SetSize(wxSize(width, -1));

	// Layout buttons
	auto advanced_button = sectionButton(SettingsPage::Advanced);
	for (auto* btn : section_buttons_)
		if (btn != advanced_button)
			vbox->Add(btn, lh.sfWithSmallBorder(0, wxBOTTOM).Expand());
	vbox->AddStretchSpacer();
	vbox->Add(advanced_button, wxSizerFlags().Expand());

	panel->SetBackgroundColour(sidePanelColour());

	return panel;
}

string SettingsDialog::pageId(SettingsPage page)
{
	switch (page)
	{
	case SettingsPage::General:      return "general";
	case SettingsPage::Interface:    return "interface";
	case SettingsPage::Keybinds:     return "keybinds";
	case SettingsPage::Editing:      return "editing";
	case SettingsPage::Text:         return "text";
	case SettingsPage::Graphics:     return "gfx";
	case SettingsPage::Audio:        return "audio";
	case SettingsPage::Scripting:    return "scripts";
	case SettingsPage::MapGeneral:   return "map_general";
	case SettingsPage::MapDisplay:   return "map_display";
	case SettingsPage::Advanced:     return "advanced";
	case SettingsPage::BaseResource: return "base_resource";
	case SettingsPage::Colour:       return "colour_theme";
	case SettingsPage::Colorimetry:  return "colorimetry";
	case SettingsPage::TextStyle:    return "text_style";
	case SettingsPage::Map3d:        return "map_3d";
	case SettingsPage::NodeBuilders: return "node_builders";
	}

	return "";
}

// ReSharper disable CppParameterMayBeConstPtrOrRef
void SettingsDialog::onSectionButtonClicked(wxCommandEvent& e)
{
	auto btn = dynamic_cast<SToolBarButton*>(e.GetEventObject());
	if (!btn)
		return;

	// Uncheck all buttons
	for (auto* b : section_buttons_)
		b->setChecked(false);

	// Check the clicked button
	btn->setChecked(true);

	// Show the appropriate panel
	SettingsPanel* new_page;
	auto           index = std::distance(
        section_buttons_.begin(), std::find(section_buttons_.begin(), section_buttons_.end(), btn));
	new_page = settings_pages_[index];
	title_text_->SetLabel(new_page->title());

	new_page->Hide();
	content_sizer_->Replace(current_page_, new_page);
	current_page_->Hide();
	current_page_ = new_page;
	current_page_->Show();

	Layout();
	Refresh();
}
