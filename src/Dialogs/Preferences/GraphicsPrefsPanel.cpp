
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    GraphicsPrefsPanel.cpp
// Description: Panel containing graphics preference controls
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
#include "GraphicsPrefsPanel.h"
#include "MainEditor/MainEditor.h"
#include "OpenGL/GLTexture.h"
#include "UI/WxUtils.h"


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


// -----------------------------------------------------------------------------
//
// GraphicsPrefsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// GraphicsPrefsPanel class constructor
// -----------------------------------------------------------------------------
GraphicsPrefsPanel::GraphicsPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create controls
	auto cp_flags   = wxCLRP_SHOW_LABEL | wxCLRP_USE_TEXTCTRL;
	cp_colour1_     = new wxColourPickerCtrl(this, -1, *wxBLACK, wxDefaultPosition, wxDefaultSize, cp_flags);
	cp_colour2_     = new wxColourPickerCtrl(this, -1, *wxBLACK, wxDefaultPosition, wxDefaultSize, cp_flags);
	choice_presets_ = new wxChoice(this, -1);
	choice_presets_->Append(WxUtils::arrayString({ "Default",
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
	choice_browser_bg_ = new wxChoice(this, -1);
	choice_browser_bg_->Append(
		WxUtils::arrayString({ "Transparent background (as above)", "System background", "Black background" }));
	cb_show_border_       = new wxCheckBox(this, -1, "Show outline around graphics and textures");
	cb_hilight_mouseover_ = new wxCheckBox(this, -1, "Hilight graphics on mouse hover");
	cb_extra_gfxconv_     = new wxCheckBox(this, -1, "Offer additional conversion options");

	setupLayout();

	// Bind events
	choice_presets_->Bind(wxEVT_CHOICE, &GraphicsPrefsPanel::onChoicePresetSelected, this);
}

// -----------------------------------------------------------------------------
// Initialises panel controls
// -----------------------------------------------------------------------------
void GraphicsPrefsPanel::init()
{
	cp_colour1_->SetColour(wxColour(bgtx_colour1));
	cp_colour2_->SetColour(wxColour(bgtx_colour2));
	cb_show_border_->SetValue(gfx_show_border);
	cb_extra_gfxconv_->SetValue(gfx_extraconv);
	choice_browser_bg_->SetSelection(browser_bg_type);
	cb_hilight_mouseover_->SetValue(gfx_hilight_mouseover);
}

// -----------------------------------------------------------------------------
// Applies preferences from the panel controls
// -----------------------------------------------------------------------------
void GraphicsPrefsPanel::applyPreferences()
{
	wxColour wxc = cp_colour1_->GetColour();
	bgtx_colour1 = WxUtils::strToView(wxc.GetAsString(wxC2S_CSS_SYNTAX));
	wxc          = cp_colour2_->GetColour();
	bgtx_colour2 = WxUtils::strToView(wxc.GetAsString(wxC2S_CSS_SYNTAX));
	OpenGL::Texture::resetBackgroundTexture();
	gfx_show_border       = cb_show_border_->GetValue();
	gfx_extraconv         = cb_extra_gfxconv_->GetValue();
	browser_bg_type       = choice_browser_bg_->GetSelection();
	gfx_hilight_mouseover = cb_hilight_mouseover_->GetValue();
	MainEditor::windowWx()->Refresh();
}

// -----------------------------------------------------------------------------
// Lays out the controls on the panel
// -----------------------------------------------------------------------------
void GraphicsPrefsPanel::setupLayout()
{
	// Create sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Transparent background colours
	WxUtils::layoutVertically(
		sizer,
		{ new wxStaticText(this, -1, "Transparent background colours:"),
		  cp_colour1_,
		  cp_colour2_,
		  WxUtils::createLabelHBox(this, "Preset:", choice_presets_) },
		wxSizerFlags(0).Expand());

	sizer->Add(new wxStaticLine(this, -1), 0, wxEXPAND | wxTOP | wxBOTTOM, UI::padLarge());

	// Other gfx options
	WxUtils::layoutVertically(
		sizer,
		{ WxUtils::createLabelHBox(this, "Browser Background:", choice_browser_bg_),
		  cb_show_border_,
		  cb_hilight_mouseover_,
		  cb_extra_gfxconv_ },
		wxSizerFlags(0).Expand());
}

// -----------------------------------------------------------------------------
// Called when the 'preset' dropdown choice is changed
// -----------------------------------------------------------------------------
void GraphicsPrefsPanel::onChoicePresetSelected(wxCommandEvent& e)
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
	};

	applyPreferences();
}
