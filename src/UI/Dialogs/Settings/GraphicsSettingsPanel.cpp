
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
#include "UI/Layout.h"
#include "UI/WxUtils.h"
#include "Utility/SFileDialog.h"

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(String, bgtx_colour1)
EXTERN_CVAR(String, bgtx_colour2)
EXTERN_CVAR(Bool, gfx_show_border)
EXTERN_CVAR(Bool, gfx_extraconv)
EXTERN_CVAR(Int, browser_bg_type)
EXTERN_CVAR(Bool, gfx_hilight_mouseover)
EXTERN_CVAR(Bool, translation_editor_condensed)
EXTERN_CVAR(String, path_pngout)
EXTERN_CVAR(String, path_pngcrush)
EXTERN_CVAR(String, path_deflopt)
EXTERN_CVAR(Bool, hud_bob)
EXTERN_CVAR(Bool, hud_center)
EXTERN_CVAR(Bool, hud_statusbar)
EXTERN_CVAR(Bool, hud_wide)


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
	Hide();
	Freeze();

	// Create sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Create tabs
	auto tabs = STabCtrl::createControl(this);
	tabs->AddPage(createGeneralPanel(), "General");
	tabs->AddPage(createPngPanel(), "PNG Tools");
	tabs->AddPage(colorimetry_panel_ = new ColorimetrySettingsPanel(this), "Colorimetry");
	sizer->Add(tabs, wxSizerFlags(1).Expand());

	init();

	// Bind events
	choice_presets_->Bind(wxEVT_CHOICE, &GraphicsSettingsPanel::onChoicePresetSelected, this);

	Thaw();
}

// -----------------------------------------------------------------------------
// Initialises panel controls
// -----------------------------------------------------------------------------
void GraphicsSettingsPanel::init() const
{
	// General
	cp_colour1_->SetColour(wxColour(bgtx_colour1));
	cp_colour2_->SetColour(wxColour(bgtx_colour2));
	cb_show_border_->SetValue(gfx_show_border);
	cb_extra_gfxconv_->SetValue(gfx_extraconv);
	choice_browser_bg_->SetSelection(browser_bg_type);
	cb_hilight_mouseover_->SetValue(gfx_hilight_mouseover);
	cb_condensed_trans_edit_->SetValue(translation_editor_condensed);

	// PNG
	flp_pngout_->setLocation(path_pngout);
	flp_pngcrush_->setLocation(path_pngcrush);
	flp_deflopt_->setLocation(path_deflopt);

	// Hud Offsets View
	cb_hud_bob_->SetValue(hud_bob);
	cb_hud_center_->SetValue(hud_center);
	cb_hud_statusbar_->SetValue(hud_statusbar);
	cb_hud_wide_->SetValue(hud_wide);

	colorimetry_panel_->init();
}

// -----------------------------------------------------------------------------
// Applies preferences from the panel controls
// -----------------------------------------------------------------------------
void GraphicsSettingsPanel::applySettings()
{
	// General
	wxColour wxc = cp_colour1_->GetColour();
	bgtx_colour1 = wxutil::strToView(wxc.GetAsString(wxC2S_CSS_SYNTAX));
	wxc          = cp_colour2_->GetColour();
	bgtx_colour2 = wxutil::strToView(wxc.GetAsString(wxC2S_CSS_SYNTAX));
	gl::Texture::resetBackgroundTexture();
	gfx_show_border              = cb_show_border_->GetValue();
	gfx_extraconv                = cb_extra_gfxconv_->GetValue();
	browser_bg_type              = choice_browser_bg_->GetSelection();
	gfx_hilight_mouseover        = cb_hilight_mouseover_->GetValue();
	translation_editor_condensed = cb_condensed_trans_edit_->GetValue();
	maineditor::windowWx()->Refresh();

	// PNG
	path_pngout   = wxutil::strToView(flp_pngout_->location());
	path_pngcrush = wxutil::strToView(flp_pngcrush_->location());
	path_deflopt  = wxutil::strToView(flp_deflopt_->location());

	// Hud Offsets View
	hud_bob       = cb_hud_bob_->GetValue();
	hud_center    = cb_hud_center_->GetValue();
	hud_statusbar = cb_hud_statusbar_->GetValue();
	hud_wide      = cb_hud_wide_->GetValue();

	colorimetry_panel_->apply();
}

wxPanel* GraphicsSettingsPanel::createGeneralPanel()
{
	auto panel = new wxPanel(this);
	auto lh    = ui::LayoutHelper(panel);

	// Create controls
	auto cp_flags   = wxCLRP_SHOW_LABEL | wxCLRP_USE_TEXTCTRL;
	cp_colour1_     = new wxColourPickerCtrl(panel, -1, *wxBLACK, wxDefaultPosition, wxDefaultSize, cp_flags);
	cp_colour2_     = new wxColourPickerCtrl(panel, -1, *wxBLACK, wxDefaultPosition, wxDefaultSize, cp_flags);
	choice_presets_ = new wxChoice(panel, -1);
	choice_presets_->Append(wxutil::arrayString({ "Default",
												  "Black",
												  "Black (Checkered)",
												  "Cyan",
												  "Cyan (Checkered)",
												  "Magenta",
												  "Magenta (Checkered)",
												  "White",
												  "White (Checkered)",
												  "Yellow",
												  "Yellow (Checkered)",
												  "Vintage Id Software" }));
	choice_browser_bg_ = new wxChoice(panel, -1);
	choice_browser_bg_->Append(
		wxutil::arrayString({ "Transparent background (as above)", "System background", "Black background" }));
	cb_show_border_          = new wxCheckBox(panel, -1, "Show outline around graphics and textures");
	cb_hilight_mouseover_    = new wxCheckBox(panel, -1, "Hilight graphics on mouse hover");
	cb_extra_gfxconv_        = new wxCheckBox(panel, -1, "Offer additional conversion options");
	cb_condensed_trans_edit_ = new wxCheckBox(panel, -1, "Condensed Translation Editor layout");
	cb_condensed_trans_edit_->SetToolTip(
		"On some displays the translation editor dialog can be too large to fit on the screen vertically. Enable this "
		"to reduce its vertical size.");

	cb_hud_bob_       = new wxCheckBox(panel, -1, "Show weapon bob outline");
	cb_hud_center_    = new wxCheckBox(panel, -1, "Show center line");
	cb_hud_statusbar_ = new wxCheckBox(panel, -1, "Show status bar lines");
	cb_hud_wide_      = new wxCheckBox(panel, -1, "Show widescreen borders");

	// Create sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	auto vbox = new wxBoxSizer(wxVERTICAL);
	sizer->Add(vbox, lh.sfWithLargeBorder(1).Expand());

	// Transparent background colours
	lh.layoutVertically(
		vbox,
		{ new wxStaticText(panel, -1, "Transparent background colours:"),
		  cp_colour1_,
		  cp_colour2_,
		  wxutil::createLabelHBox(panel, "Preset:", choice_presets_) },
		wxSizerFlags(0).Expand());

	vbox->Add(new wxStaticLine(panel, -1), lh.sfWithLargeBorder(0, wxTOP | wxBOTTOM).Expand());

	// Other gfx options
	lh.layoutVertically(
		vbox,
		{ wxutil::createLabelHBox(panel, "Browser Background:", choice_browser_bg_),
		  cb_show_border_,
		  cb_hilight_mouseover_,
		  cb_extra_gfxconv_,
		  cb_condensed_trans_edit_,
		  new wxStaticLine(panel, -1),
		  new wxStaticText(panel, -1, "Hud Offsets View:"),
		  cb_hud_bob_,
		  cb_hud_center_,
		  cb_hud_statusbar_,
		  cb_hud_wide_ },
		wxSizerFlags(0).Expand());

	return panel;
}

wxPanel* GraphicsSettingsPanel::createPngPanel()
{
	auto panel = new wxPanel(this);
	auto lh    = ui::LayoutHelper(panel);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	// Create sizer
	auto vbox = new wxBoxSizer(wxVERTICAL);
	sizer->Add(vbox, lh.sfWithLargeBorder(1).Expand());

	lh.layoutVertically(
		vbox,
		vector<wxObject*>{ wxutil::createLabelVBox(
							   panel,
							   "Location of PNGout:",
							   flp_pngout_ = new FileLocationPanel(
								   panel,
								   path_pngout,
								   true,
								   "Browse for PNGout Executable",
								   filedialog::executableExtensionString(),
								   filedialog::executableFileName("pngout"))),
						   wxutil::createLabelVBox(
							   panel,
							   "Location of PNGCrush:",
							   flp_pngcrush_ = new FileLocationPanel(
								   panel,
								   path_pngcrush,
								   true,
								   "Browse for PNGCrush Executable",
								   filedialog::executableExtensionString(),
								   filedialog::executableFileName("pngcrush"))),
						   wxutil::createLabelVBox(
							   panel,
							   "Location of DeflOpt:",
							   flp_deflopt_ = new FileLocationPanel(
								   panel,
								   path_deflopt,
								   true,
								   "Browse for DeflOpt Executable",
								   filedialog::executableExtensionString(),
								   filedialog::executableFileName("deflopt"))) },
		wxSizerFlags(0).Expand());

	return panel;
}

// -----------------------------------------------------------------------------
// Called when the 'preset' dropdown choice is changed
// -----------------------------------------------------------------------------
void GraphicsSettingsPanel::onChoicePresetSelected(wxCommandEvent& e)
{
	int preset = choice_presets_->GetSelection();

	switch (preset)
	{
	case 1: // Black
		cp_colour1_->SetColour(wxColour(0, 0, 0));
		cp_colour2_->SetColour(wxColour(0, 0, 0));
		break;
	case 2: // Black (checkered)
		cp_colour1_->SetColour(wxColour(0, 0, 0));
		cp_colour2_->SetColour(wxColour(30, 30, 30));
		break;
	case 3: // Cyan
		cp_colour1_->SetColour(wxColour(0, 255, 255));
		cp_colour2_->SetColour(wxColour(0, 255, 255));
		break;
	case 4: // Cyan (checkered)
		cp_colour1_->SetColour(wxColour(0, 255, 255));
		cp_colour2_->SetColour(wxColour(20, 225, 225));
		break;
	case 5: // Magenta
		cp_colour1_->SetColour(wxColour(255, 0, 255));
		cp_colour2_->SetColour(wxColour(255, 0, 255));
		break;
	case 6: // Magenta (checkered)
		cp_colour1_->SetColour(wxColour(255, 0, 255));
		cp_colour2_->SetColour(wxColour(225, 20, 225));
		break;
	case 7: // White
		cp_colour1_->SetColour(wxColour(255, 255, 255));
		cp_colour2_->SetColour(wxColour(255, 255, 255));
		break;
	case 8: // White (checkered)
		cp_colour1_->SetColour(wxColour(255, 255, 255));
		cp_colour2_->SetColour(wxColour(225, 225, 225));
		break;
	case 9: // Yellow
		cp_colour1_->SetColour(wxColour(255, 255, 0));
		cp_colour2_->SetColour(wxColour(255, 255, 0));
		break;
	case 10: // Yellow (checkered)
		cp_colour1_->SetColour(wxColour(255, 255, 0));
		cp_colour2_->SetColour(wxColour(225, 225, 20));
		break;
	case 11: // Vintage Id Software (aka Doom PLAYPAL index 255)
		cp_colour1_->SetColour(wxColour(167, 107, 107));
		cp_colour2_->SetColour(wxColour(167, 107, 107));
		break;
	default: // Default
		cp_colour1_->SetColour(wxColour(64, 64, 80));
		cp_colour2_->SetColour(wxColour(80, 80, 96));
		break;
	}

	applySettings();
}
