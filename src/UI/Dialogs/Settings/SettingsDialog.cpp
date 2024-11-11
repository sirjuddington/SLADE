
#include "Main.h"

#include "AdvancedSettingsPanel.h"
#include "App.h"
#include "AudioSettingsPanel.h"
#include "GeneralSettingsPanel.h"
#include "GraphicsSettingsPanel.h"
#include "InputSettingsPanel.h"
#include "InterfaceSettingsPanel.h"
#include "ScriptSettingsPanel.h"
#include "SettingsDialog.h"
#include "TextEditorSettingsPanel.h"
#include "UI/Layout.h"
#include "UI/SToolBar/SToolBarButton.h"
#include "UI/WxUtils.h"

using namespace slade;
using namespace ui;


wxColour sidePanelColour()
{
	auto bgcol = wxutil::systemPanelBGColour();
	return app::isDarkTheme() ? bgcol.ChangeLightness(105) : bgcol.ChangeLightness(95);
}

wxColour titlePanelColour()
{
	auto bgcol = wxutil::systemPanelBGColour();
	return app::isDarkTheme() ? bgcol.ChangeLightness(130) : bgcol.ChangeLightness(70);
}

SToolBarButton* createSectionButton(wxWindow* parent, const string& action, const string& text, const string& icon)
{
	auto btn = new SToolBarButton(parent, action, text, icon, text, true, 24);
	btn->setPadding(8, 0);
	btn->setExactFit(false);
	btn->setFontSize(1.1f);
	btn->SetBackgroundColour(sidePanelColour());
	btn->setFillChecked(true);
	return btn;
}

void updateMinSize(const wxWindow* window, int& min_width, int& min_height)
{
	auto size  = window->GetBestSize();
	min_width  = std::max(min_width, size.GetWidth());
	min_height = std::max(min_height, size.GetHeight());
}





SettingsDialog::SettingsDialog(wxWindow* parent) : SDialog(parent, "SLADE Settings", "settings")
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
	title_text_ = new wxStaticText(title_panel, -1, "General");
	title_text_->SetFont(GetFont().MakeLarger().MakeLarger().Bold());
	title_sizer->Add(title_text_, lh.sfWithLargeBorder(1, wxLEFT | wxTOP).Expand());
	content_sizer_->Add(title_panel, wxSizerFlags().Expand());

	// Settings page
	general_page_   = new GeneralSettingsPanel(this);
	interface_page_ = new InterfaceSettingsPanel(this);
	input_page_     = new InputSettingsPanel(this);
	graphics_page_  = new GraphicsSettingsPanel(this);
	audio_page_     = new AudioSettingsPanel(this);
	text_page_      = new TextEditorSettingsPanel(this);
	scripts_page_   = new ScriptSettingsPanel(this);
	advanced_page_  = new AdvancedSettingsPanel(this);
	tbb_general_->setChecked(true);
	content_sizer_->Add(general_page_, lh.sfWithLargeBorder(1).Expand());
	general_page_->Show();
	current_page_ = general_page_;

	// Buttons
	auto button_sizer = new wxBoxSizer(wxHORIZONTAL);
	button_sizer->Add(new wxButton(this, -1, "Apply"), wxSizerFlags(0).Expand());
	button_sizer->AddStretchSpacer();
	button_sizer->Add(new wxButton(this, -1, "OK"), lh.sfWithBorder(0, wxRIGHT).Expand());
	button_sizer->Add(new wxButton(this, -1, "Cancel"), wxSizerFlags(0).Expand());
	content_sizer_->Add(button_sizer, lh.sfWithLargeBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Expand());

	Bind(wxEVT_STOOLBAR_BUTTON_CLICKED, &SettingsDialog::onSectionButtonClicked, this);

	// Determine best minimum size based on larger pages
	int min_width  = 0;
	int min_height = 0;
	updateMinSize(interface_page_, min_width, min_height);
	updateMinSize(graphics_page_, min_width, min_height);
	updateMinSize(text_page_, min_width, min_height);
	SetMinSize({ sections_panel->GetBestSize().x + min_width,
				 min_height + button_sizer->CalcMin().y + title_panel->GetBestSize().y + FromDIP(100) });
}

wxPanel* SettingsDialog::createSectionsPanel()
{
	auto panel = new wxPanel(this);
	auto lh    = LayoutHelper(panel);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	auto vbox = new wxBoxSizer(wxVERTICAL);
	sizer->Add(vbox, lh.sfWithLargeBorder(1).Expand());

	tbb_general_   = createSectionButton(panel, "general", "General", "logo");
	tbb_interface_ = createSectionButton(panel, "interface", "Interface", "settings");
	tbb_keybinds_  = createSectionButton(panel, "keybinds", "Keyboard Shortcuts", "settings");
	tbb_editing_   = createSectionButton(panel, "editing", "Editing", "wrench");
	tbb_text_      = createSectionButton(panel, "text", "Text Editor", "text");
	tbb_gfx_       = createSectionButton(panel, "gfx", "Graphics", "gfx");
	tbb_audio_     = createSectionButton(panel, "audio", "Audio", "sound");
	tbb_scripting_ = createSectionButton(panel, "scripts", "ACS Scripts", "script");
	tbb_advanced_  = createSectionButton(panel, "advanced", "Advanced", "settings");

	// Set all to width of 'Keyboard Shortcuts' button since it's the widest
	tbb_keybinds_->setExactFit(true);
	auto width = tbb_keybinds_->GetMinSize().GetWidth();
	tbb_general_->SetSize(wxSize(width, -1));
	tbb_interface_->SetSize(wxSize(width, -1));
	tbb_editing_->SetSize(wxSize(width, -1));
	tbb_text_->SetSize(wxSize(width, -1));
	tbb_gfx_->SetSize(wxSize(width, -1));
	tbb_audio_->SetSize(wxSize(width, -1));
	tbb_scripting_->SetSize(wxSize(width, -1));
	tbb_advanced_->SetSize(wxSize(width, -1));

	vbox->Add(tbb_general_, lh.sfWithSmallBorder(0, wxBOTTOM).Expand());
	vbox->Add(tbb_interface_, lh.sfWithSmallBorder(0, wxBOTTOM).Expand());
	vbox->Add(tbb_keybinds_, lh.sfWithSmallBorder(0, wxBOTTOM).Expand());
	vbox->Add(tbb_editing_, lh.sfWithSmallBorder(0, wxBOTTOM).Expand());
	vbox->Add(tbb_text_, lh.sfWithSmallBorder(0, wxBOTTOM).Expand());
	vbox->Add(tbb_gfx_, lh.sfWithSmallBorder(0, wxBOTTOM).Expand());
	vbox->Add(tbb_audio_, lh.sfWithSmallBorder(0, wxBOTTOM).Expand());
	vbox->Add(tbb_scripting_, lh.sfWithSmallBorder(0, wxBOTTOM).Expand());
	vbox->AddStretchSpacer();
	vbox->Add(tbb_advanced_, wxSizerFlags().Expand());

	panel->SetBackgroundColour(sidePanelColour());

	return panel;
}

// ReSharper disable CppParameterMayBeConstPtrOrRef
void SettingsDialog::onSectionButtonClicked(wxCommandEvent& e)
{
	auto btn = dynamic_cast<SToolBarButton*>(e.GetEventObject());
	if (!btn)
		return;

	// Uncheck all buttons
	tbb_general_->setChecked(false);
	tbb_interface_->setChecked(false);
	tbb_keybinds_->setChecked(false);
	tbb_editing_->setChecked(false);
	tbb_text_->setChecked(false);
	tbb_gfx_->setChecked(false);
	tbb_audio_->setChecked(false);
	tbb_scripting_->setChecked(false);
	tbb_advanced_->setChecked(false);

	// Check the clicked button
	btn->setChecked(true);

	// Show the appropriate panel
	wxWindow* new_page;
	if (btn == tbb_general_)
	{
		new_page = general_page_;
		title_text_->SetLabel("General Settings");
	}
	else if (btn == tbb_interface_)
	{
		new_page = interface_page_;
		title_text_->SetLabel("Interface Settings");
	}
	/*else if (btn == tbb_editing_)
	{
		if (!editing_page_)
			editing_page_ = new EditingSettingsPanel(this);

		new_page = editing_page_;
		title_text_->SetLabel("Editing Settings");
	}*/
	else if (btn == tbb_keybinds_)
	{
		new_page = input_page_;
		title_text_->SetLabel("Keyboard Shortcuts");
	}
	else if (btn == tbb_gfx_)
	{
		new_page = graphics_page_;
		title_text_->SetLabel("Graphics Settings");
	}
	else if (btn == tbb_audio_)
	{
		new_page = audio_page_;
		title_text_->SetLabel("Audio Settings");
	}
	else if (btn == tbb_text_)
	{
		new_page = text_page_;
		title_text_->SetLabel("Text Editor Settings");
	}
	else if (btn == tbb_scripting_)
	{
		new_page = scripts_page_;
		title_text_->SetLabel("ACS Script Settings");
	}
	else if (btn == tbb_advanced_)
	{
		new_page = advanced_page_;
		title_text_->SetLabel("Advanced Settings");
	}
	else
	{
		if (!blank_page_)
			blank_page_ = new wxPanel(this);

		new_page = blank_page_;
		title_text_->SetLabel(btn->actionName());
	}

	new_page->Hide();
	content_sizer_->Replace(current_page_, new_page);
	current_page_->Hide();
	current_page_ = new_page;
	current_page_->Show();

	Layout();
	Refresh();
}
