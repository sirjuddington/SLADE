
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    GraphicsPrefsPanel.cpp
 * Description: Panel containing graphics preference controls
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
#include "GraphicsPrefsPanel.h"
#include "MainEditor/MainEditor.h"
#include "OpenGL/GLTexture.h"



/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(String, bgtx_colour1)
EXTERN_CVAR(String, bgtx_colour2)
EXTERN_CVAR(Bool, gfx_show_border)
EXTERN_CVAR(Bool, gfx_extraconv)
EXTERN_CVAR(Int, browser_bg_type)
EXTERN_CVAR(Bool, gfx_hilight_mouseover)


/*******************************************************************
 * GRAPHICSPREFSPANEL CLASS FUNCTIONS
 *******************************************************************/

/* GraphicsPrefsPanel::GraphicsPrefsPanel
 * GraphicsPrefsPanel class constructor
 *******************************************************************/
GraphicsPrefsPanel::GraphicsPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create sizer
	wxBoxSizer* psizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(psizer);

	// Create frame+sizer
	wxStaticBox* frame = new wxStaticBox(this, -1, "Graphics Preferences");
	wxStaticBoxSizer* sizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	psizer->Add(sizer, 1, wxEXPAND|wxALL, 4);

	// Colours for the chequered background texture
	sizer->Add(new wxStaticText(this, -1, "Transparent background colours:"), 0, wxALL, 4);
	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	cp_colour1 = new wxColourPickerCtrl(this, -1, WXCOL(COL_BLACK), wxDefaultPosition, wxDefaultSize, wxCLRP_SHOW_LABEL|wxCLRP_USE_TEXTCTRL);
	vbox->Add(cp_colour1, 0, wxEXPAND|wxBOTTOM, 4);
	cp_colour2 = new wxColourPickerCtrl(this, -1, WXCOL(COL_BLACK), wxDefaultPosition, wxDefaultSize, wxCLRP_SHOW_LABEL|wxCLRP_USE_TEXTCTRL);
	vbox->Add(cp_colour2, 0, wxEXPAND|wxBOTTOM, 4);

	// Quick colour presets
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	vbox->Add(hbox, 0, wxEXPAND|wxBOTTOM, 4);
	string schemes[] = { "Default",
	                     "Black", "Black (Checkered)",
	                     "Cyan", "Cyan (Checkered)",
	                     "Magenta", "Magenta (Checkered)",
	                     "White", "White (Checkered)",
	                     "Yellow", "Yellow (Checkered)",
	                     "Vintage Id Software"
	                   };
	choice_presets = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, 12, schemes);
	hbox->Add(new wxStaticText(this, -1, "Presets:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
	hbox->Add(choice_presets, 1, wxEXPAND);

	sizer->Add(vbox, 0, wxEXPAND|wxALL, 4);

	// 'Show outline around gfx'
	cb_show_border = new wxCheckBox(this, -1, "Show outline around graphics and textures");
	sizer->Add(cb_show_border, 0, wxEXPAND|wxALL, 4);

	// 'Hilight gfx on mouseover'
	cb_hilight_mouseover = new wxCheckBox(this, -1, "Hilight graphics on mouse hover");
	sizer->Add(cb_hilight_mouseover, 0, wxEXPAND|wxALL, 4);

	// 'Extra image conversion options'
	cb_extra_gfxconv = new wxCheckBox(this, -1, "Offer additional conversion options");
	sizer->Add(cb_extra_gfxconv, 0, wxEXPAND|wxALL, 4);

	// Browser background type
	hbox = new wxBoxSizer(wxHORIZONTAL);
	vbox->Add(hbox, 0, wxEXPAND|wxBOTTOM, 4);
	string browser_types[] = { "Transparent background (as above)", "System background", "Black background" };
	choice_browser_bg = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, 3, browser_types);
	hbox->Add(new wxStaticText(this, -1, "Browser Background:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
	hbox->Add(choice_browser_bg, 1, wxEXPAND);

	// Bind events
	choice_presets->Bind(wxEVT_CHOICE, &GraphicsPrefsPanel::onChoicePresetSelected, this);
}

/* GraphicsPrefsPanel::~GraphicsPrefsPanel
 * GraphicsPrefsPanel class destructor
 *******************************************************************/
GraphicsPrefsPanel::~GraphicsPrefsPanel()
{
}

/* GraphicsPrefsPanel::init
 * Initialises panel controls
 *******************************************************************/
void GraphicsPrefsPanel::init()
{
	cp_colour1->SetColour(wxColour(bgtx_colour1));
	cp_colour2->SetColour(wxColour(bgtx_colour2));
	cb_show_border->SetValue(gfx_show_border);
	cb_extra_gfxconv->SetValue(gfx_extraconv);
	choice_browser_bg->SetSelection(browser_bg_type);
	cb_hilight_mouseover->SetValue(gfx_hilight_mouseover);
}

/* GraphicsPrefsPanel::applyPreferences
 * Applies preferences from the panel controls
 *******************************************************************/
void GraphicsPrefsPanel::applyPreferences()
{
	wxColour wxc = cp_colour1->GetColour();
	bgtx_colour1 = wxc.GetAsString();
	wxc = cp_colour2->GetColour();
	bgtx_colour2 = wxc.GetAsString();
	GLTexture::resetBgTex();
	gfx_show_border = cb_show_border->GetValue();
	gfx_extraconv = cb_extra_gfxconv->GetValue();
	browser_bg_type = choice_browser_bg->GetSelection();
	gfx_hilight_mouseover = cb_hilight_mouseover->GetValue();
	MainEditor::windowWx()->Refresh();
}

/* GraphicsPrefsPanel::onChoiceSchemeSelected
 * Called when the 'preset' dropdown choice is changed
 *******************************************************************/
void GraphicsPrefsPanel::onChoicePresetSelected(wxCommandEvent& e)
{
	int preset = choice_presets->GetSelection();

	switch (preset)
	{
	case 1:		// Black
		cp_colour1->SetColour(wxColour(0, 0, 0));
		cp_colour2->SetColour(wxColour(0, 0, 0));
		break;
	case 2:		// Black (checkered)
		cp_colour1->SetColour(wxColour(0, 0, 0));
		cp_colour2->SetColour(wxColour(30, 30, 30));
		break;
	case 3:		// Cyan
		cp_colour1->SetColour(wxColour(0, 255, 255));
		cp_colour2->SetColour(wxColour(0, 255, 255));
		break;
	case 4:		// Cyan (checkered)
		cp_colour1->SetColour(wxColour(0, 255, 255));
		cp_colour2->SetColour(wxColour(20, 225, 225));
		break;
	case 5:		// Magenta
		cp_colour1->SetColour(wxColour(255, 0, 255));
		cp_colour2->SetColour(wxColour(255, 0, 255));
		break;
	case 6:		// Magenta (checkered)
		cp_colour1->SetColour(wxColour(255, 0, 255));
		cp_colour2->SetColour(wxColour(225, 20, 225));
		break;
	case 7:		// White
		cp_colour1->SetColour(wxColour(255, 255, 255));
		cp_colour2->SetColour(wxColour(255, 255, 255));
		break;
	case 8:		// White (checkered)
		cp_colour1->SetColour(wxColour(255, 255, 255));
		cp_colour2->SetColour(wxColour(225, 225, 225));
		break;
	case 9:		// Yellow
		cp_colour1->SetColour(wxColour(255, 255, 0));
		cp_colour2->SetColour(wxColour(255, 255, 0));
		break;
	case 10:	// Yellow (checkered)
		cp_colour1->SetColour(wxColour(255, 255, 0));
		cp_colour2->SetColour(wxColour(225, 225, 20));
		break;
	case 11:	// Vintage Id Software (aka Doom PLAYPAL index 255)
		cp_colour1->SetColour(wxColour(167, 107, 107));
		cp_colour2->SetColour(wxColour(167, 107, 107));
		break;
	default:	// Default
		cp_colour1->SetColour(wxColour(64, 64, 80));
		cp_colour2->SetColour(wxColour(80, 80, 96));
		break;
	};

	applyPreferences();
}
