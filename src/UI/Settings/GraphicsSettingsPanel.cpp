
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    GraphicsSettingsPanel.cpp
// Description: Panel containing graphics settings controls
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "GraphicsSettingsPanel.h"
#include "ColorimetrySettingsPanel.h"
#include "MainEditor/MainEditor.h"
#include "OpenGL/GLTexture.h"
#include "UI/Controls/FileLocationPanel.h"
#include "UI/Controls/STabCtrl.h"
#include "UI/Controls/SettingsTable.h"
#include "UI/Layout.h"
#include "UI/UI.h"
#include "UI/WxUtils.h"
#include "Utility/FileUtils.h"
#include "Utility/SFileDialog.h"

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
struct BackgroundPreset
{
	string   name;
	wxColour colour1;
	wxColour colour2;
};
vector<BackgroundPreset> bg_presets = {
	{ "Default", wxColour(64, 64, 80), wxColour(80, 80, 96) },
	{ "Default (Dark)", wxColour(44, 44, 58), wxColour(61, 61, 73) },
	{ "Black", wxColour(0, 0, 0), wxColour(0, 0, 0) },
	{ "Black (Checkered)", wxColour(0, 0, 0), wxColour(30, 30, 30) },
	{ "Cyan", wxColour(0, 255, 255), wxColour(0, 255, 255) },
	{ "Cyan (Checkered)", wxColour(0, 255, 255), wxColour(20, 225, 225) },
	{ "Magenta", wxColour(255, 0, 255), wxColour(255, 0, 255) },
	{ "Magenta (Checkered)", wxColour(255, 0, 255), wxColour(225, 20, 225) },
	{ "White", wxColour(255, 255, 255), wxColour(255, 255, 255) },
	{ "White (Checkered)", wxColour(255, 255, 255), wxColour(225, 225, 225) },
	{ "Yellow", wxColour(255, 255, 0), wxColour(255, 255, 0) },
	{ "Yellow (Checkered)", wxColour(255, 255, 0), wxColour(225, 225, 20) },
	{ "Vintage Id Software", wxColour(167, 107, 107), wxColour(167, 107, 107) },
};
} // namespace


// -----------------------------------------------------------------------------
//
// GraphicsSettingsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// GraphicsSettingsPanel class constructor
// -----------------------------------------------------------------------------
GraphicsSettingsPanel::GraphicsSettingsPanel(wxWindow* parent) : SettingsPanel(parent)
{
	// Create sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	colorimetry_panel_ = new ColorimetrySettingsPanel(this);

	// Create tabs
	auto tabs = STabCtrl::createControl(this);
	tabs->AddPage(createGeneralPanel(tabs), wxS("General"));
	tabs->AddPage(createPngPanel(tabs), wxS("PNG Tools"));
	tabs->AddPage(wxutil::createPadPanel(tabs, colorimetry_panel_, padLarge()), wxS("Colorimetry"));
	sizer->Add(tabs, wxSizerFlags(1).Expand());

	// Bind events
	choice_presets_->Bind(wxEVT_CHOICE, &GraphicsSettingsPanel::onChoicePresetSelected, this);
}

// -----------------------------------------------------------------------------
// Loads settings from cvars into the controls
// -----------------------------------------------------------------------------
void GraphicsSettingsPanel::loadSettings()
{
	settings_table_->loadSettings();

	// General
	cp_colour1_->SetColour(wxColour(wxString::FromUTF8(CVar::getString("bgtx_colour1"))));
	cp_colour2_->SetColour(wxColour(wxString::FromUTF8(CVar::getString("bgtx_colour2"))));

	// Set preset choice if it matches current colours
	for (size_t i = 0; i < bg_presets.size(); i++)
	{
		if (bg_presets[i].colour1 == cp_colour1_->GetColour() && bg_presets[i].colour2 == cp_colour2_->GetColour())
		{
			choice_presets_->SetSelection(i);
			break;
		}
	}

	// Try to find PNG tools' executables if paths not already set
	static string detected_pngcrush, detected_pngout, detected_deflopt;
	if (CVar::getString("path_pngcrush").empty())
	{
		if (detected_pngcrush.empty())
			detected_pngcrush = fileutil::findExecutable("pngcrush", "png");
		CVar::set("path_pngcrush", detected_pngcrush);
	}
	if (CVar::getString("path_pngout").empty())
	{
		if (detected_pngout.empty())
			detected_pngout = fileutil::findExecutable("pngout", "png");
		CVar::set("path_pngout", detected_pngout);
	}
	if (CVar::getString("path_deflopt").empty())
	{
		if (detected_deflopt.empty())
			detected_deflopt = fileutil::findExecutable("deflopt", "png");
		CVar::set("path_deflopt", detected_deflopt);
	}

	// PNG
	flp_pngout_->setLocation(CVar::getString("path_pngcrush"));
	flp_pngcrush_->setLocation(CVar::getString("path_pngout"));
	flp_deflopt_->setLocation(CVar::getString("path_deflopt"));

	colorimetry_panel_->loadSettings();
}

// -----------------------------------------------------------------------------
// Applies settings from the panel controls to cvars
// -----------------------------------------------------------------------------
void GraphicsSettingsPanel::applySettings()
{
	// General
	settings_table_->applySettings();
	wxColour wxc = cp_colour1_->GetColour();
	CVar::set("bgtx_colour1", wxc.GetAsString(wxC2S_CSS_SYNTAX).utf8_string());
	wxc = cp_colour2_->GetColour();
	CVar::set("bgtx_colour2", wxc.GetAsString(wxC2S_CSS_SYNTAX).utf8_string());
	gl::Texture::resetBackgroundTexture();
	maineditor::windowWx()->Refresh();

	// PNG
	CVar::set("path_pngout", flp_pngout_->location());
	CVar::set("path_pngcrush", flp_pngcrush_->location());
	CVar::set("path_deflopt", flp_deflopt_->location());

	colorimetry_panel_->applySettings();
}

// -----------------------------------------------------------------------------
// Creates the general settings panel
// -----------------------------------------------------------------------------
wxPanel* GraphicsSettingsPanel::createGeneralPanel(wxWindow* parent)
{
	settings_table_ = new SettingsTable(parent);
	auto lh         = LayoutHelper(settings_table_);

	// Create controls
	auto cp_flags   = wxCLRP_SHOW_LABEL;
	cp_colour1_     = new wxColourPickerCtrl(settings_table_, -1, *wxBLACK, wxDefaultPosition, wxDefaultSize, cp_flags);
	cp_colour2_     = new wxColourPickerCtrl(settings_table_, -1, *wxBLACK, wxDefaultPosition, wxDefaultSize, cp_flags);
	choice_presets_ = new wxChoice(settings_table_, -1);
	for (auto& preset : bg_presets)
		choice_presets_->Append(wxString::FromUTF8(preset.name));

	// General
	settings_table_->addCheckBox("Show outline around graphics and textures", "gfx_show_border");
	settings_table_->addCheckBox("Highlight graphics on mouse hover", "gfx_hilight_mouseover");
	settings_table_->addCheckBox("Offer additional conversion options", "gfx_extraconv");
	settings_table_->addCheckBox(
		"Condensed Translation Editor layout|"
		"On some displays the translation editor dialog can be too large to fit on the screen vertically. Enable this "
		"to reduce its vertical size.",
		"translation_editor_condensed");

	// Transparent Background Colours
	settings_table_->addSectionSeparator("Transparent Background Colours");
	settings_table_->addCustomControl("Preset", choice_presets_, wxALIGN_CENTER_VERTICAL);
	settings_table_->addCustomSizer(
		"Colours", lh.layoutHorizontally({ cp_colour1_, cp_colour2_ }), wxALIGN_CENTER_VERTICAL);
	settings_table_->addRadioButtons(
		"Browser Background", "browser_bg_type", { "Transparent (as above)", "System", "Black" });

	// Hud Offsets View
	settings_table_->addSectionSeparator("Hud Offsets View");
	settings_table_->addCheckBox("Show weapon bob outline", "hud_bob");
	settings_table_->addCheckBox("Show center line", "hud_center");
	settings_table_->addCheckBox("Show status bar lines", "hud_statusbar");
	settings_table_->addCheckBox("Show widescreen borders", "hud_wide");

	return settings_table_;
}

// -----------------------------------------------------------------------------
// Creates the PNG tools panel
// -----------------------------------------------------------------------------
wxPanel* GraphicsSettingsPanel::createPngPanel(wxWindow* parent)
{
	auto panel = new wxPanel(parent);
	auto lh    = LayoutHelper(panel);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	// Create controls
	flp_pngout_ = new FileLocationPanel(
		panel,
		"",
		true,
		"Browse for PNGout Executable",
		filedialog::executableExtensionString(),
		filedialog::executableFileName("pngout"));
	flp_pngcrush_ = new FileLocationPanel(
		panel,
		"",
		true,
		"Browse for PNGCrush Executable",
		filedialog::executableExtensionString(),
		filedialog::executableFileName("pngcrush"));
	flp_deflopt_ = new FileLocationPanel(
		panel,
		"",
		true,
		"Browse for DeflOpt Executable",
		filedialog::executableExtensionString(),
		filedialog::executableFileName("deflopt"));

	// Layout
	auto gb_sizer = new wxGridBagSizer(lh.pad(), lh.padLarge());
	sizer->Add(gb_sizer, lh.sfWithLargeBorder(1).Expand());
	gb_sizer->Add(new wxStaticText(panel, -1, wxS("Location of PNGout")), { 0, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(flp_pngout_, { 0, 1 }, { 1, 1 }, wxEXPAND);
	gb_sizer->Add(
		new wxStaticText(panel, -1, wxS("Location of PNGCrush")), { 1, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(flp_pngcrush_, { 1, 1 }, { 1, 1 }, wxEXPAND);
	gb_sizer->Add(new wxStaticText(panel, -1, wxS("Location of DeflOpt")), { 2, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(flp_deflopt_, { 2, 1 }, { 1, 1 }, wxEXPAND);
	gb_sizer->AddGrowableCol(1, 1);

	return panel;
}

// -----------------------------------------------------------------------------
// Called when the 'preset' dropdown choice is changed
// -----------------------------------------------------------------------------
void GraphicsSettingsPanel::onChoicePresetSelected(wxCommandEvent& e)
{
	int preset = choice_presets_->GetSelection();
	if (preset >= 0 && preset < static_cast<int>(bg_presets.size()))
	{
		cp_colour1_->SetColour(bg_presets[preset].colour1);
		cp_colour2_->SetColour(bg_presets[preset].colour2);
	}

	applySettings();
}
