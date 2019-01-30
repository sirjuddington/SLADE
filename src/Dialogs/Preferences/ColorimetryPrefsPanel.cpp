
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ColorimetryPrefsPanel.cpp
// Description: Panel containing colorimetry preference controls
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
#include "ColorimetryPrefsPanel.h"
#include "Graphics/Palette/Palette.h"
#include "UI/WxUtils.h"


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Int, col_match)
EXTERN_CVAR(Float, col_match_r)
EXTERN_CVAR(Float, col_match_g)
EXTERN_CVAR(Float, col_match_b)
EXTERN_CVAR(Float, col_match_h)
EXTERN_CVAR(Float, col_match_s)
EXTERN_CVAR(Float, col_match_l)
EXTERN_CVAR(Float, col_greyscale_r)
EXTERN_CVAR(Float, col_greyscale_g)
EXTERN_CVAR(Float, col_greyscale_b)
EXTERN_CVAR(Float, col_cie_tristim_x)
EXTERN_CVAR(Float, col_cie_tristim_z)
EXTERN_CVAR(Float, col_cie_kl)
EXTERN_CVAR(Float, col_cie_k1)
EXTERN_CVAR(Float, col_cie_k2)
EXTERN_CVAR(Float, col_cie_kc)
EXTERN_CVAR(Float, col_cie_kh)


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Creates a spin control with the given name and values
// -----------------------------------------------------------------------------
wxSpinCtrlDouble* createSpin(wxWindow* parent, const string& name, double min, double max, double initial, double inc)
{
	return new wxSpinCtrlDouble(
		parent,
		-1,
		name,
		wxDefaultPosition,
		{ UI::px(UI::Size::SpinCtrlWidth), -1 },
		wxSP_ARROW_KEYS | wxTE_PROCESS_ENTER,
		min,
		max,
		initial,
		inc);
}
} // namespace


// -----------------------------------------------------------------------------
//
// ColorimetryPrefsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ColorimetryPrefsPanel class constructor
// -----------------------------------------------------------------------------
ColorimetryPrefsPanel::ColorimetryPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create controls
	spin_grey_r_         = createSpin(this, "GreyscaleRed", 0.0, 1.0, 0.001, 0.001);
	spin_grey_g_         = createSpin(this, "GreyscaleGreen", 0.0, 1.0, 0.001, 0.001);
	spin_grey_b_         = createSpin(this, "GreyscaleBlue", 0.0, 1.0, 0.001, 0.001);
	string rbgweights[]  = { "Default / Standard", "Carmack's Typo", "Linear RGB" };
	choice_presets_grey_ = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, 3, rbgweights);
	string matchers[]    = { "RGB (integer)", "RGB (double)", "HSL", "CIE 76", "CIE 94", "CIEDE 2000" };
	choice_colmatch_     = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, 6, matchers);
	spin_factor_r_       = createSpin(this, "RedFactor", 0.0, 10.0, 1.0, 0.1);
	spin_factor_g_       = createSpin(this, "GreenFactor", 0.0, 10.0, 1.0, 0.1);
	spin_factor_b_       = createSpin(this, "BlueFactor", 0.0, 10.0, 1.0, 0.1);
	spin_factor_h_       = createSpin(this, "HueFactor", 0.0, 10.0, 1.0, 0.1);
	spin_factor_s_       = createSpin(this, "SatFactor", 0.0, 10.0, 1.0, 0.1);
	spin_factor_l_       = createSpin(this, "LumFactor", 0.0, 10.0, 1.0, 0.1);
	spin_tristim_x_      = createSpin(this, "TriStimX", 0.0, 200.0, 100.0, 0.1);
	spin_tristim_z_      = createSpin(this, "TriStimZ", 0.0, 200.0, 100.0, 0.1);
	string tristimuli[]  = {
        wxString::FromUTF8("Illuminant A, 2\xc2\xb0 Observer"),
        wxString::FromUTF8("Illuminant A, 10\xc2\xb0 Observer"),
        wxString::FromUTF8("Illuminant C, 2\xc2\xb0 Observer"),
        wxString::FromUTF8("Illuminant C, 10\xc2\xb0 Observer"),
        wxString::FromUTF8("Illuminant D50, 2\xc2\xb0 Observer"),
        wxString::FromUTF8("Illuminant D50, 10\xc2\xb0 Observer"),
        wxString::FromUTF8("Illuminant D60, 2\xc2\xb0 Observer"),
        wxString::FromUTF8("Illuminant D60, 10\xc2\xb0 Observer"),
        wxString::FromUTF8("Illuminant D65, 2\xc2\xb0 Observer"),
        wxString::FromUTF8("Illuminant D65, 10\xc2\xb0 Observer"),
        wxString::FromUTF8("Illuminant D75, 2\xc2\xb0 Observer"),
        wxString::FromUTF8("Illuminant D75, 10\xc2\xb0 Observer"),
        wxString::FromUTF8("Illuminant F2, 2\xc2\xb0 Observer"),
        wxString::FromUTF8("Illuminant F2, 10\xc2\xb0 Observer"),
        wxString::FromUTF8("Illuminant TL4, 2\xc2\xb0 Observer"),
        wxString::FromUTF8("Illuminant TL4, 10\xc2\xb0 Observer"),
        wxString::FromUTF8("Illuminant UL3000, 2\xc2\xb0 Observer"),
        wxString::FromUTF8("Illuminant UL3000, 10\xc2\xb0 Observer"),
	};
	choice_presets_tristim_ = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, 18, tristimuli);
	spin_cie_kl_            = createSpin(this, "KL", 0.0, 10.0, 1.0, 0.1);
	spin_cie_k1_            = createSpin(this, "K1", 0.0, 10.0, 1.0, 0.1);
	spin_cie_k2_            = createSpin(this, "K2", 0.0, 10.0, 1.0, 0.1);
	spin_cie_kc_            = createSpin(this, "KC", 0.0, 10.0, 1.0, 0.1);
	spin_cie_kh_            = createSpin(this, "KH", 0.0, 10.0, 1.0, 0.1);

	setupLayout();

	// Bind events
	choice_presets_grey_->Bind(wxEVT_CHOICE, &ColorimetryPrefsPanel::onChoiceGreyscalePresetSelected, this);
	choice_colmatch_->Bind(wxEVT_CHOICE, [&](wxCommandEvent&) { col_match = 1 + choice_colmatch_->GetSelection(); });
	choice_presets_tristim_->Bind(wxEVT_CHOICE, &ColorimetryPrefsPanel::onChoiceTristimPresetSelected, this);
}

// -----------------------------------------------------------------------------
// Initialises panel controls
// -----------------------------------------------------------------------------
void ColorimetryPrefsPanel::init()
{
	spin_grey_r_->SetValue(col_greyscale_r);
	spin_grey_g_->SetValue(col_greyscale_g);
	spin_grey_b_->SetValue(col_greyscale_b);
	if (col_match > (int)Palette::ColourMatch::Default && col_match < (int)Palette::ColourMatch::Stop)
		choice_colmatch_->SetSelection(col_match - 1);
	spin_factor_r_->SetValue(col_match_r);
	spin_factor_g_->SetValue(col_match_g);
	spin_factor_b_->SetValue(col_match_b);
	spin_factor_h_->SetValue(col_match_h);
	spin_factor_s_->SetValue(col_match_s);
	spin_factor_l_->SetValue(col_match_l);
	spin_tristim_x_->SetValue(col_cie_tristim_x);
	spin_tristim_z_->SetValue(col_cie_tristim_z);
	spin_cie_kl_->SetValue(col_cie_kl);
	spin_cie_k1_->SetValue(col_cie_k1);
	spin_cie_k2_->SetValue(col_cie_k2);
	spin_cie_kc_->SetValue(col_cie_kc);
	spin_cie_kh_->SetValue(col_cie_kh);
}

// -----------------------------------------------------------------------------
// Applies preferences from the panel controls
// -----------------------------------------------------------------------------
void ColorimetryPrefsPanel::applyPreferences()
{
	col_greyscale_r   = spin_grey_r_->GetValue();
	col_greyscale_g   = spin_grey_g_->GetValue();
	col_greyscale_b   = spin_grey_b_->GetValue();
	col_match_r       = spin_factor_r_->GetValue();
	col_match_g       = spin_factor_g_->GetValue();
	col_match_b       = spin_factor_b_->GetValue();
	col_match_h       = spin_factor_h_->GetValue();
	col_match_s       = spin_factor_s_->GetValue();
	col_match_l       = spin_factor_l_->GetValue();
	col_cie_tristim_x = spin_tristim_x_->GetValue();
	col_cie_tristim_z = spin_tristim_z_->GetValue();
	col_cie_kl        = spin_cie_kl_->GetValue();
	col_cie_k1        = spin_cie_k1_->GetValue();
	col_cie_k2        = spin_cie_k2_->GetValue();
	col_cie_kc        = spin_cie_kc_->GetValue();
	col_cie_kh        = spin_cie_kh_->GetValue();
}

// -----------------------------------------------------------------------------
// Lays out the controls on the panel
// -----------------------------------------------------------------------------
void ColorimetryPrefsPanel::setupLayout()
{
	// Create sizer
	auto gbsizer = new wxGridBagSizer(UI::pad(), UI::pad());
	SetSizer(gbsizer);

	// RGB weights for greyscale luminance
	gbsizer->Add(new wxStaticText(this, -1, "RGB weights for greyscale luminance:"), { 0, 0 }, { 1, 6 });
	gbsizer->Add(WxUtils::createLabelHBox(this, "R:", spin_grey_r_), { 1, 0 }, { 1, 1 }, wxALIGN_RIGHT);
	gbsizer->Add(WxUtils::createLabelHBox(this, "G:", spin_grey_g_), { 1, 1 }, { 1, 1 }, wxALIGN_RIGHT);
	gbsizer->Add(WxUtils::createLabelHBox(this, "B:", spin_grey_b_), { 1, 2 }, { 1, 1 }, wxALIGN_RIGHT);
	gbsizer->Add(WxUtils::createLabelHBox(this, "Presets:", choice_presets_grey_), { 1, 3 }, { 1, 3 }, wxEXPAND);

	gbsizer->Add(new wxStaticLine(this, -1), { 2, 0 }, { 1, 7 }, wxEXPAND | wxTOP | wxBOTTOM, UI::pad());

	// Color matching method
	gbsizer->Add(WxUtils::createLabelHBox(this, "Colour matching:", choice_colmatch_), { 3, 0 }, { 1, 6 }, wxEXPAND);

	// RGB and HSL weights for color matching
	gbsizer->Add(WxUtils::createLabelHBox(this, "R:", spin_factor_r_), { 4, 0 }, { 1, 1 }, wxALIGN_RIGHT);
	gbsizer->Add(WxUtils::createLabelHBox(this, "G:", spin_factor_g_), { 4, 1 }, { 1, 1 }, wxALIGN_RIGHT);
	gbsizer->Add(WxUtils::createLabelHBox(this, "B:", spin_factor_b_), { 4, 2 }, { 1, 1 }, wxALIGN_RIGHT);
	gbsizer->Add(WxUtils::createLabelHBox(this, "H:", spin_factor_h_), { 4, 3 }, { 1, 1 }, wxALIGN_RIGHT);
	gbsizer->Add(WxUtils::createLabelHBox(this, "S:", spin_factor_s_), { 4, 4 }, { 1, 1 }, wxALIGN_RIGHT);
	gbsizer->Add(WxUtils::createLabelHBox(this, "L:", spin_factor_l_), { 4, 5 }, { 1, 1 }, wxALIGN_RIGHT);

	gbsizer->Add(new wxStaticLine(this, -1), { 5, 0 }, { 1, 7 }, wxEXPAND | wxTOP | wxBOTTOM, UI::pad());

	// CIE Lab funkiness: tristimulus values for RGB->Lab conversion,
	// and KL, KC, KH, K1 and K2 factors for CIE94 and CIEDE2000 equations
	gbsizer->Add(new wxStaticText(this, -1, "CIE Lab Tristimulus:"), { 6, 0 }, { 1, 6 });
	gbsizer->Add(WxUtils::createLabelHBox(this, "X:", spin_tristim_x_), { 7, 0 }, { 1, 1 }, wxALIGN_RIGHT);
	gbsizer->Add(WxUtils::createLabelHBox(this, "Z:", spin_tristim_z_), { 7, 1 }, { 1, 1 }, wxALIGN_RIGHT);
	gbsizer->Add(choice_presets_tristim_, { 7, 2 }, { 1, 4 }, wxEXPAND);

	gbsizer->Add(new wxStaticLine(this, -1), { 8, 0 }, { 1, 7 }, wxEXPAND | wxTOP | wxBOTTOM, UI::pad());

	// CIE equation factors
	gbsizer->Add(new wxStaticText(this, -1, "CIE 94 and CIEDE 2000 Factors:"), { 9, 0 }, { 1, 6 }, wxEXPAND);
	gbsizer->Add(WxUtils::createLabelHBox(this, "KL:", spin_cie_kl_), { 10, 0 }, { 1, 1 }, wxALIGN_RIGHT);
	gbsizer->Add(WxUtils::createLabelHBox(this, "K1:", spin_cie_k1_), { 10, 1 }, { 1, 1 }, wxALIGN_RIGHT);
	gbsizer->Add(WxUtils::createLabelHBox(this, "K2:", spin_cie_k2_), { 10, 2 }, { 1, 1 }, wxALIGN_RIGHT);
	gbsizer->Add(WxUtils::createLabelHBox(this, "KC:", spin_cie_kc_), { 10, 3 }, { 1, 1 }, wxALIGN_RIGHT);
	gbsizer->Add(WxUtils::createLabelHBox(this, "KH:", spin_cie_kh_), { 10, 4 }, { 1, 1 }, wxALIGN_RIGHT);

	gbsizer->AddGrowableCol(6, 1);
}

// -----------------------------------------------------------------------------
// Called when the greyscale 'preset' dropdown choice is changed
// Standard NTSC weights: 0.299, 0.587, 0.114
// Id Software's typoed weights: 0.299, 0.587, 0.144
// http://www.doomworld.com/idgames/?id=16644
// Grafica Obscura's weights for linear RGB: 0.3086, 0.6094, 0.0820
// http://www.graficaobscura.com/matrix/index.html
// -----------------------------------------------------------------------------
void ColorimetryPrefsPanel::onChoiceGreyscalePresetSelected(wxCommandEvent& e)
{
	int preset = choice_presets_grey_->GetSelection();

	switch (preset)
	{
	case 0: // Standard
		spin_grey_r_->SetValue(0.299);
		spin_grey_g_->SetValue(0.587);
		spin_grey_b_->SetValue(0.114);
		break;
	case 1: // Id Software's typoed variant
		spin_grey_r_->SetValue(0.299);
		spin_grey_g_->SetValue(0.587);
		spin_grey_b_->SetValue(0.144);
		break;
	case 2: // Some linear RGB formula
		spin_grey_r_->SetValue(0.3086);
		spin_grey_g_->SetValue(0.6094);
		spin_grey_b_->SetValue(0.0820);
		break;
	default: break;
	};

	applyPreferences();
}

// -----------------------------------------------------------------------------
// Called when the tristimulus 'preset' dropdown choice is changed
// CIE Lab tristimulus values are normalized so that Y is always 100.00.
// X and Z depend on the observer and illuminant, here are a few values:
//				2� observer		10� observer
//	Illuminant	__Xn__	__Zn__	__Xn__	__Zn__
//		A		109.83	 35.55	111.16	 35.19
//		C		 98.04	118.11	 97.30	116.14
//		D50		 96.38	 82.45	 96.72	 81.45
//		D60		 95.23	100.86	 95.21	 99.60
//		D65		 95.02	108.82	 94.83	107.38
//		D75		 94.96	122.53	 94.45	120.70
//		F2		 98.09	 67.53	102.13	 69.37
//		TL4		101.40	 65.90	103.82	 66.90
//		UL3000	107.99	 33.91	111.12	 35.21
// Source: http://www.hunterlab.com/appnotes/an07_96a.pdf
// -----------------------------------------------------------------------------
void ColorimetryPrefsPanel::onChoiceTristimPresetSelected(wxCommandEvent& e)
{
	int preset = choice_presets_tristim_->GetSelection();

	switch (preset)
	{
	case 0: // 2�A
		spin_tristim_x_->SetValue(109.83);
		spin_tristim_z_->SetValue(35.55);
		break;
	case 1: // 10�A
		spin_tristim_x_->SetValue(111.16);
		spin_tristim_z_->SetValue(35.19);
		break;
	case 2: // 2�C
		spin_tristim_x_->SetValue(98.04);
		spin_tristim_z_->SetValue(118.11);
		break;
	case 3: // 10�C
		spin_tristim_x_->SetValue(97.30);
		spin_tristim_z_->SetValue(116.14);
		break;
	case 4: // 2�D50
		spin_tristim_x_->SetValue(96.38);
		spin_tristim_z_->SetValue(82.45);
		break;
	case 5: // 10�D50
		spin_tristim_x_->SetValue(96.72);
		spin_tristim_z_->SetValue(81.45);
		break;
	case 6: // 2�D60
		spin_tristim_x_->SetValue(95.23);
		spin_tristim_z_->SetValue(100.86);
		break;
	case 7: // 10�D60
		spin_tristim_x_->SetValue(95.21);
		spin_tristim_z_->SetValue(99.60);
		break;
	case 8: // 2�D65
		spin_tristim_x_->SetValue(95.02);
		spin_tristim_z_->SetValue(108.82);
		break;
	case 9: // 10�D65
		spin_tristim_x_->SetValue(94.83);
		spin_tristim_z_->SetValue(107.38);
		break;
	case 10: // 2�D75
		spin_tristim_x_->SetValue(94.96);
		spin_tristim_z_->SetValue(122.53);
		break;
	case 11: // 10�D75
		spin_tristim_x_->SetValue(94.45);
		spin_tristim_z_->SetValue(120.70);
		break;
	case 12: // 2�F2
		spin_tristim_x_->SetValue(98.09);
		spin_tristim_z_->SetValue(67.53);
		break;
	case 13: // 10�F2
		spin_tristim_x_->SetValue(102.13);
		spin_tristim_z_->SetValue(69.37);
		break;
	case 14: // 2�TL4
		spin_tristim_x_->SetValue(101.40);
		spin_tristim_z_->SetValue(65.90);
		break;
	case 15: // 10�TL4
		spin_tristim_x_->SetValue(103.82);
		spin_tristim_z_->SetValue(66.90);
		break;
	case 16: // 2�UL3000
		spin_tristim_x_->SetValue(107.99);
		spin_tristim_z_->SetValue(33.91);
		break;
	case 17: // 10�UL3000
		spin_tristim_x_->SetValue(111.12);
		spin_tristim_z_->SetValue(35.21);
		break;
	default: break;
	};

	applyPreferences();
}
