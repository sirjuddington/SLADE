
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2012 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ColorimetryPrefsPanel.cpp
 * Description: Panel containing colorimetry preference controls
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/

/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "WxStuff.h"
#include "ColorimetryPrefsPanel.h"
#include "CIEDeltaEquations.h"
#include "Palette.h"


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Int,   col_match)
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


/*******************************************************************
 * ColorimetryPrefsPanel CLASS FUNCTIONS
 *******************************************************************/

/* ColorimetryPrefsPanel::ColorimetryPrefsPanel
 * ColorimetryPrefsPanel class constructor
 *******************************************************************/
ColorimetryPrefsPanel::ColorimetryPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent) {
	// Create sizer
	wxBoxSizer* psizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(psizer);

	// Create frame+sizer
	wxStaticBox *frame = new wxStaticBox(this, -1, "Colorimetry Preferences");
	wxStaticBoxSizer *sizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	psizer->Add(sizer, 1, wxEXPAND|wxALL, 4);

	// Let's stack horizontal sizers in a vertical sizer
	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);

	// The default size for spincontrols is far too large, so halve the actual default width.
	// This is a bit hacky, but better than hardcoding a value that won't scale with user's font size.
	// There must be a better way to do this, though.
	wxSpinCtrlDouble* dummy = new wxSpinCtrlDouble(this, -1, "Dummy", wxDefaultPosition, wxDefaultSize);
	wxSize spinsize;
	spinsize = dummy->GetSize();
	spinsize.SetWidth(spinsize.GetWidth() / 2);
	delete dummy;

	// RGB weights for greyscale luminance
	sizer->Add(new wxStaticText(this, -1, "RGB weights for greyscale luminance:"), 0, wxALL, 4);
	hbox->Add(new wxStaticText(this, -1, "R:"), 0, wxALL, 4);
	spin_grey_r = new wxSpinCtrlDouble(this, -1, "GreyscaleRed", wxDefaultPosition, spinsize, wxSP_ARROW_KEYS, 0.0, 1.0, 0.001, 0.001);
	hbox->Add(spin_grey_r, 0, wxEXPAND|wxBOTTOM, 4);
	hbox->Add(new wxStaticText(this, -1, "G:"), 0, wxALL, 4);
	spin_grey_g = new wxSpinCtrlDouble(this, -1, "GreyscaleGreen", wxDefaultPosition, spinsize, wxSP_ARROW_KEYS, 0.0, 1.0, 0.001, 0.001);
	hbox->Add(spin_grey_g, 0, wxEXPAND|wxBOTTOM, 4);
	hbox->Add(new wxStaticText(this, -1, "B:"), 0, wxALL, 4);
	spin_grey_b = new wxSpinCtrlDouble(this, -1, "GreyscaleBlue", wxDefaultPosition, spinsize, wxSP_ARROW_KEYS, 0.0, 1.0, 0.001, 0.001);
	hbox->Add(spin_grey_b, 0, wxEXPAND|wxBOTTOM, 4);
	string rbgweights[] = { "Default / Standard", "Carmack's Typo", "Linear RGB" };
	choice_presets_grey = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, 3, rbgweights);
	hbox->Add(new wxStaticText(this, -1, "Presets:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
	hbox->Add(choice_presets_grey, 1, wxEXPAND);
	vbox->Add(hbox, 0, wxEXPAND|wxBOTTOM, 4);
	sizer->Add(vbox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Color matching method
	vbox = new wxBoxSizer(wxVERTICAL);
	hbox = new wxBoxSizer(wxHORIZONTAL);
	string matchers[] = { "RGB (integer)", "RGB (double)", "HSL", "CIE 76", "CIE 94", "CIEDE 2000" };
	choice_colmatch = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, 6, matchers);
	hbox->Add(new wxStaticText(this, -1, "Color matching:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
	hbox->Add(choice_colmatch, 1, wxEXPAND);
	vbox->Add(hbox, 0, wxEXPAND|wxBOTTOM, 4);

	// RGB and HSL weights for color matching
	hbox = new wxBoxSizer(wxHORIZONTAL);
	hbox->Add(new wxStaticText(this, -1, "R:"), 0, wxALL, 4);
	spin_factor_r = new wxSpinCtrlDouble(this, -1, "RedFactor", wxDefaultPosition, spinsize, wxSP_ARROW_KEYS, 0.0, 10.0, 1.0, 0.1);
	hbox->Add(spin_factor_r, 0, wxEXPAND|wxBOTTOM, 4);
	hbox->Add(new wxStaticText(this, -1, "G:"), 0, wxALL, 4);
	spin_factor_g = new wxSpinCtrlDouble(this, -1, "GreenFactor", wxDefaultPosition, spinsize, wxSP_ARROW_KEYS, 0.0, 10.0, 1.0, 0.1);
	hbox->Add(spin_factor_g, 0, wxEXPAND|wxBOTTOM, 4);
	hbox->Add(new wxStaticText(this, -1, "B:"), 0, wxALL, 4);
	spin_factor_b = new wxSpinCtrlDouble(this, -1, "BlueFactor", wxDefaultPosition, spinsize, wxSP_ARROW_KEYS, 0.0, 10.0, 1.0, 0.1);
	hbox->Add(spin_factor_b, 0, wxEXPAND|wxBOTTOM, 4);
	hbox->Add(new wxStaticText(this, -1, "H:"), 0, wxALL, 4);
	spin_factor_h = new wxSpinCtrlDouble(this, -1, "HueFactor", wxDefaultPosition, spinsize, wxSP_ARROW_KEYS, 0.0, 10.0, 1.0, 0.1);
	hbox->Add(spin_factor_h, 0, wxEXPAND|wxBOTTOM, 4);
	hbox->Add(new wxStaticText(this, -1, "S:"), 0, wxALL, 4);
	spin_factor_s = new wxSpinCtrlDouble(this, -1, "SatFactor", wxDefaultPosition, spinsize, wxSP_ARROW_KEYS, 0.0, 10.0, 1.0, 0.1);
	hbox->Add(spin_factor_s, 0, wxEXPAND|wxBOTTOM, 4);
	hbox->Add(new wxStaticText(this, -1, "L:"), 0, wxALL, 4);
	spin_factor_l = new wxSpinCtrlDouble(this, -1, "LumFactor", wxDefaultPosition, spinsize, wxSP_ARROW_KEYS, 0.0, 10.0, 1.0, 0.1);
	hbox->Add(spin_factor_l, 0, wxEXPAND|wxBOTTOM, 4);
	vbox->Add(hbox, 0, wxEXPAND|wxBOTTOM, 4);
	sizer->Add(vbox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// CIE Lab funkiness: tristimulus values for RGB->Lab conversion, and KL, KC, KH, K1 and K2 factors for CIE94 and CIEDE2000 equations
	sizer->Add(new wxStaticText(this, -1, "CIE Lab Tristimulus:"), 0, wxALL, 4);
	vbox = new wxBoxSizer(wxVERTICAL);
	hbox = new wxBoxSizer(wxHORIZONTAL);
	hbox->Add(new wxStaticText(this, -1, "X:"), 0, wxALL, 4);
	spin_tristim_x = new wxSpinCtrlDouble(this, -1, "TriStimX", wxDefaultPosition, spinsize, wxSP_ARROW_KEYS, 0.0, 200.0, 100.0, 0.1);
	hbox->Add(spin_tristim_x, 0, wxEXPAND|wxBOTTOM, 4);
	hbox->Add(new wxStaticText(this, -1, "Z:"), 0, wxALL, 4);
	spin_tristim_z = new wxSpinCtrlDouble(this, -1, "TriStimZ", wxDefaultPosition, spinsize, wxSP_ARROW_KEYS, 0.0, 200.0, 100.0, 0.1);
	hbox->Add(spin_tristim_z, 0, wxEXPAND|wxBOTTOM, 4);
	string tristimuli[] = { 
		"Illuminant A, 2° Observer", "Illuminant A, 10° Observer", 
		"Illuminant C, 2° Observer", "Illuminant C, 10° Observer", 
		"Illuminant D50, 2° Observer", "Illuminant D50, 10° Observer", 
		"Illuminant D60, 2° Observer", "Illuminant D60, 10° Observer", 
		"Illuminant D65, 2° Observer", "Illuminant D65, 10° Observer", 
		"Illuminant D75, 2° Observer", "Illuminant D75, 10° Observer", 
		"Illuminant F2, 2° Observer", "Illuminant F2, 10° Observer", 
		"Illuminant TL4, 2° Observer", "Illuminant TL4, 10° Observer", 
		"Illuminant UL3000, 2° Observer", "Illuminant UL3000, 10° Observer", 
	};
	choice_presets_tristim = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, 18, tristimuli);
	hbox->Add(choice_presets_tristim, 1, wxEXPAND);
	vbox->Add(hbox, 0, wxEXPAND|wxBOTTOM, 4);
	sizer->Add(vbox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// CIE equation factors
	sizer->Add(new wxStaticText(this, -1, "CIE 94 and CIEDE 2000 Factors:"), 0, wxALL, 4);
	vbox = new wxBoxSizer(wxVERTICAL);
	hbox = new wxBoxSizer(wxHORIZONTAL);
	hbox->Add(new wxStaticText(this, -1, "KL:"), 0, wxALL, 4);
	spin_cie_kl = new wxSpinCtrlDouble(this, -1, "KL", wxDefaultPosition, spinsize, wxSP_ARROW_KEYS, 0.0, 10.0, 1.0, 0.1);
	hbox->Add(spin_cie_kl, 0, wxEXPAND|wxBOTTOM, 4);
	hbox->Add(new wxStaticText(this, -1, "K1:"), 0, wxALL, 4);
	spin_cie_k1 = new wxSpinCtrlDouble(this, -1, "K1", wxDefaultPosition, spinsize, wxSP_ARROW_KEYS, 0.0, 10.0, 1.0, 0.1);
	hbox->Add(spin_cie_k1, 0, wxEXPAND|wxBOTTOM, 4);
	hbox->Add(new wxStaticText(this, -1, "K2:"), 0, wxALL, 4);
	spin_cie_k2 = new wxSpinCtrlDouble(this, -1, "K2", wxDefaultPosition, spinsize, wxSP_ARROW_KEYS, 0.0, 10.0, 1.0, 0.1);
	hbox->Add(spin_cie_k2, 0, wxEXPAND|wxBOTTOM, 4);
	hbox->Add(new wxStaticText(this, -1, "KC:"), 0, wxALL, 4);
	spin_cie_kc = new wxSpinCtrlDouble(this, -1, "KC", wxDefaultPosition, spinsize, wxSP_ARROW_KEYS, 0.0, 10.0, 1.0, 0.1);
	hbox->Add(spin_cie_kc, 0, wxEXPAND|wxBOTTOM, 4);
	hbox->Add(new wxStaticText(this, -1, "KH:"), 0, wxALL, 4);
	spin_cie_kh = new wxSpinCtrlDouble(this, -1, "KH", wxDefaultPosition, spinsize, wxSP_ARROW_KEYS, 0.0, 10.0, 1.0, 0.1);
	hbox->Add(spin_cie_kh, 0, wxEXPAND|wxBOTTOM, 4);
	vbox->Add(hbox, 0, wxEXPAND|wxBOTTOM, 4);

	// Wrap it up
	sizer->Add(vbox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Bind events
	choice_presets_grey->Bind(wxEVT_COMMAND_CHOICE_SELECTED, &ColorimetryPrefsPanel::onChoiceGreyscalePresetSelected, this);
	choice_colmatch->Bind(wxEVT_COMMAND_CHOICE_SELECTED, &ColorimetryPrefsPanel::onChoiceColormatchSelected, this);
	choice_presets_tristim->Bind(wxEVT_COMMAND_CHOICE_SELECTED, &ColorimetryPrefsPanel::onChoiceTristimPresetSelected, this);
}

/* ColorimetryPrefsPanel::~ColorimetryPrefsPanel
 * ColorimetryPrefsPanel class destructor
 *******************************************************************/
ColorimetryPrefsPanel::~ColorimetryPrefsPanel() {
}

/* ColorimetryPrefsPanel::init
 * Initialises panel controls
 *******************************************************************/
void ColorimetryPrefsPanel::init() {
	spin_grey_r->SetValue(col_greyscale_r);
	spin_grey_g->SetValue(col_greyscale_g);
	spin_grey_b->SetValue(col_greyscale_b);
	if (col_match > Palette8bit::MATCH_DEFAULT && col_match < Palette8bit::MATCH_STOP) choice_colmatch->SetSelection(col_match - 1);
	spin_factor_r->SetValue(col_match_r);
	spin_factor_g->SetValue(col_match_g);
	spin_factor_b->SetValue(col_match_b);
	spin_factor_h->SetValue(col_match_h);
	spin_factor_s->SetValue(col_match_s);
	spin_factor_l->SetValue(col_match_l);
	spin_tristim_x->SetValue(col_cie_tristim_x);
	spin_tristim_z->SetValue(col_cie_tristim_z);
	spin_cie_kl->SetValue(col_cie_kl);
	spin_cie_k1->SetValue(col_cie_k1);
	spin_cie_k2->SetValue(col_cie_k2);
	spin_cie_kc->SetValue(col_cie_kc);
	spin_cie_kh->SetValue(col_cie_kh);
}

/* ColorimetryPrefsPanel::applyPreferences
 * Applies preferences from the panel controls
 *******************************************************************/
void ColorimetryPrefsPanel::applyPreferences() {
	col_greyscale_r = spin_grey_r->GetValue();
	col_greyscale_g = spin_grey_g->GetValue();
	col_greyscale_b = spin_grey_b->GetValue();
	col_match_r = spin_factor_r->GetValue();
	col_match_g = spin_factor_g->GetValue();
	col_match_b = spin_factor_b->GetValue();
	col_match_h = spin_factor_h->GetValue();
	col_match_s = spin_factor_s->GetValue();
	col_match_l = spin_factor_l->GetValue();
	col_cie_tristim_x = spin_tristim_x->GetValue();
	col_cie_tristim_z = spin_tristim_z->GetValue();
	col_cie_kl = spin_cie_kl->GetValue();
	col_cie_k1 = spin_cie_k1->GetValue();
	col_cie_k2 = spin_cie_k2->GetValue();
	col_cie_kc = spin_cie_kc->GetValue();
	col_cie_kh = spin_cie_kh->GetValue();
}

/* ColorimetryPrefsPanel::onChoiceColormatchSelected
 * Called when the 'color matching' dropdown choice is changed
 *******************************************************************/
void ColorimetryPrefsPanel::onChoiceColormatchSelected(wxCommandEvent& e) {
	col_match = 1 + choice_colmatch->GetSelection();
}

/* ColorimetryPrefsPanel::onChoiceGreyscalePresetSelected
 * Called when the greyscale 'preset' dropdown choice is changed
 * Standard NTSC weights: 0.299, 0.587, 0.114
 * Id Software's typoed weights: 0.299, 0.587, 0.144
 * http://www.doomworld.com/idgames/?id=16644
 * Grafica Obscura's weights for linear RGB: 0.3086, 0.6094, 0.0820
 * http://www.graficaobscura.com/matrix/index.html
 *******************************************************************/
void ColorimetryPrefsPanel::onChoiceGreyscalePresetSelected(wxCommandEvent& e) {
	int preset = choice_presets_grey->GetSelection();

	switch (preset) {
	case 0:		// Standard
		spin_grey_r->SetValue(0.299);
		spin_grey_g->SetValue(0.587);
		spin_grey_b->SetValue(0.114);
		break;
	case 1:		// Id Software's typoed variant
		spin_grey_r->SetValue(0.299);
		spin_grey_g->SetValue(0.587);
		spin_grey_b->SetValue(0.144);
		break;
	case 2:		// Some linear RGB formula
		spin_grey_r->SetValue(0.3086);
		spin_grey_g->SetValue(0.6094);
		spin_grey_b->SetValue(0.0820);
		break;
	};

	applyPreferences();
}

/* ColorimetryPrefsPanel::onChoiceTristimPresetSelected
 * Called when the tristimulus 'preset' dropdown choice is changed
 * CIE Lab tristimulus values are normalized so that Y is always 100.00.
 * X and Z depend on the observer and illuminant, here are a few values:
 *				2° observer		10° observer
 *	Illuminant	__Xn__	__Zn__	__Xn__	__Zn__
 *		A		109.83	 35.55	111.16	 35.19
 *		C		 98.04	118.11	 97.30	116.14
 *		D50		 96.38	 82.45	 96.72	 81.45
 *		D60		 95.23	100.86	 95.21	 99.60
 *		D65		 95.02	108.82	 94.83	107.38
 *		D75		 94.96	122.53	 94.45	120.70
 *		F2		 98.09	 67.53	102.13	 69.37
 *		TL4		101.40	 65.90	103.82	 66.90
 *		UL3000	107.99	 33.91	111.12	 35.21
 * Source: http://www.hunterlab.com/appnotes/an07_96a.pdf
 *******************************************************************/
void ColorimetryPrefsPanel::onChoiceTristimPresetSelected(wxCommandEvent& e) {
	int preset = choice_presets_tristim->GetSelection();

	switch (preset) {
	case 0:		// 2°A
		spin_tristim_x->SetValue(109.83);
		spin_tristim_z->SetValue( 35.55);
		break;
	case 1:		// 10°A
		spin_tristim_x->SetValue(111.16);
		spin_tristim_z->SetValue( 35.19);
		break;
	case 2:		// 2°C
		spin_tristim_x->SetValue( 98.04);
		spin_tristim_z->SetValue(118.11);
		break;
	case 3:		// 10°C
		spin_tristim_x->SetValue( 97.30);
		spin_tristim_z->SetValue(116.14);
		break;
	case 4:		// 2°D50
		spin_tristim_x->SetValue( 96.38);
		spin_tristim_z->SetValue( 82.45);
		break;
	case 5:		// 10°D50
		spin_tristim_x->SetValue( 96.72);
		spin_tristim_z->SetValue( 81.45);
		break;
	case 6:		// 2°D60
		spin_tristim_x->SetValue( 95.23);
		spin_tristim_z->SetValue(100.86);
		break;
	case 7:		// 10°D60
		spin_tristim_x->SetValue( 95.21);
		spin_tristim_z->SetValue( 99.60);
		break;
	case 8:		// 2°D65
		spin_tristim_x->SetValue( 95.02);
		spin_tristim_z->SetValue(108.82);
		break;
	case 9:		// 10°D65
		spin_tristim_x->SetValue( 94.83);
		spin_tristim_z->SetValue(107.38);
		break;
	case 10:		// 2°D75
		spin_tristim_x->SetValue( 94.96);
		spin_tristim_z->SetValue(122.53);
		break;
	case 11:		// 10°D75
		spin_tristim_x->SetValue( 94.45);
		spin_tristim_z->SetValue(120.70);
		break;
	case 12:		// 2°F2
		spin_tristim_x->SetValue( 98.09);
		spin_tristim_z->SetValue( 67.53);
		break;
	case 13:		// 10°F2
		spin_tristim_x->SetValue(102.13);
		spin_tristim_z->SetValue( 69.37);
		break;
	case 14:		// 2°TL4
		spin_tristim_x->SetValue(101.40);
		spin_tristim_z->SetValue( 65.90);
		break;
	case 15:		// 10°TL4
		spin_tristim_x->SetValue(103.82);
		spin_tristim_z->SetValue( 66.90);
		break;
	case 16:		// 2°UL3000
		spin_tristim_x->SetValue(107.99);
		spin_tristim_z->SetValue( 33.91);
		break;
	case 17:		// 10°UL3000
		spin_tristim_x->SetValue(111.12);
		spin_tristim_z->SetValue( 35.21);
		break;
	};

	applyPreferences();
}
