
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MapReplaceDialog.cpp
 * Description: Dialog for 'Replace in Maps' functionality, allows
 *              to replace all instances of a certain line special /
 *              thing type / etc in all maps in an archive
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
#include "MapReplaceDialog.h"
#include "MainEditor/ArchiveOperations.h"


/*******************************************************************
 * THINGTYPEREPLACEPANEL CLASS FUNCTIONS
 *******************************************************************/

/* ThingTypeReplacePanel::ThingTypeReplacePanel
 * ThingTypeReplacePanel class constructor
 *******************************************************************/
ThingTypeReplacePanel::ThingTypeReplacePanel(wxWindow* parent) : wxPanel(parent, -1)
{
	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	wxGridBagSizer* gbsizer = new wxGridBagSizer(4, 4);
	sizer->AddStretchSpacer();
	sizer->Add(gbsizer, 0, wxALIGN_CENTER|wxALL, 4);
	sizer->AddStretchSpacer();

	// From type
	gbsizer->Add(new wxStaticText(this, -1, "Replace Type:"), wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT);
	spin_from = new wxSpinCtrl(this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxTE_PROCESS_ENTER, 0, 999999);
	gbsizer->Add(spin_from, wxGBPosition(0, 1), wxDefaultSpan, wxEXPAND);
	//btn_browse_from = new wxButton(this, -1, "...", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	//gbsizer->Add(btn_browse_from, wxGBPosition(0, 2), wxDefaultSpan, wxEXPAND);

	// To type
	gbsizer->Add(new wxStaticText(this, -1, "With Type:"), wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT);
	spin_to = new wxSpinCtrl(this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxTE_PROCESS_ENTER, 0, 999999);
	gbsizer->Add(spin_to, wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND);
	//btn_browse_to = new wxButton(this, -1, "...", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	//gbsizer->Add(btn_browse_to, wxGBPosition(1, 2), wxDefaultSpan, wxEXPAND);
}

/* ThingTypeReplacePanel::~ThingTypeReplacePanel
 * ThingTypeReplacePanel class destructor
 *******************************************************************/
ThingTypeReplacePanel::~ThingTypeReplacePanel()
{
}

/* ThingTypeReplacePanel::doReplace
 * Performs replace using settings from the panel controls for
 * [archive]
 *******************************************************************/
void ThingTypeReplacePanel::doReplace(Archive* archive)
{
	size_t count = ArchiveOperations::replaceThings(archive, spin_from->GetValue(), spin_to->GetValue());
	wxMessageBox(S_FMT("Replaced %d occurrences. See console log for more detailed information.", count), "Replace Things");
}


/*******************************************************************
 * SPECIALREPLACEPANEL CLASS FUNCTIONS
 *******************************************************************/

/* SpecialReplacePanel::SpecialReplacePanel
 * SpecialReplacePanel class constructor
 *******************************************************************/
SpecialReplacePanel::SpecialReplacePanel(wxWindow* parent) : wxPanel(parent, -1)
{
	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	wxGridBagSizer* gbsizer = new wxGridBagSizer(4, 4);
	sizer->AddStretchSpacer();
	sizer->Add(gbsizer, 0, wxALIGN_CENTER|wxALL, 4);

	// From special
	gbsizer->Add(new wxStaticText(this, -1, "Replace Special:"), wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT);
	spin_from = new wxSpinCtrl(this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 999999);
	gbsizer->Add(spin_from, wxGBPosition(0, 1), wxDefaultSpan, wxEXPAND);

	// To special
	gbsizer->Add(new wxStaticText(this, -1, "With Special:"), wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT);
	spin_to = new wxSpinCtrl(this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 999999);
	gbsizer->Add(spin_to, wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND);

	// Replace line specials
	cb_line_specials = new wxCheckBox(this, -1, "Replace Line Specials");
	gbsizer->Add(cb_line_specials, wxGBPosition(0, 2), wxDefaultSpan, wxEXPAND);

	// Replace thing specials
	cb_thing_specials = new wxCheckBox(this, -1, "Replace Thing Specials");
	gbsizer->Add(cb_thing_specials, wxGBPosition(1, 2), wxDefaultSpan, wxEXPAND);

	sizer->Add(new wxStaticLine(this, -1, wxDefaultPosition, wxDefaultSize, wxHORIZONTAL), 0, wxEXPAND|wxALL, 4);

	// Args
	gbsizer = new wxGridBagSizer(4, 4);
	sizer->Add(gbsizer, 0, wxALIGN_CENTER|wxALL, 4);
	for (unsigned a = 0; a < 5; a++)
	{
		// Create controls
		cb_args[a] = new wxCheckBox(this, -1, S_FMT("Arg %d", a));
		spin_args_from[a] = new wxSpinCtrl(this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxTE_PROCESS_ENTER, 0, 255);
		spin_args_to[a] = new wxSpinCtrl(this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxTE_PROCESS_ENTER, 0, 255);

		// Add to grid
		gbsizer->Add(cb_args[a], wxGBPosition(a, 0), wxDefaultSpan, wxEXPAND);
		gbsizer->Add(new wxStaticText(this, -1, "Replace:"), wxGBPosition(a, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT);
		gbsizer->Add(spin_args_from[a], wxGBPosition(a, 2), wxDefaultSpan, wxEXPAND);
		gbsizer->Add(new wxStaticText(this, -1, "With:"), wxGBPosition(a, 3), wxDefaultSpan, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT);
		gbsizer->Add(spin_args_to[a], wxGBPosition(a, 4), wxDefaultSpan, wxEXPAND);
	}

	sizer->AddStretchSpacer();

	cb_line_specials->SetValue(true);
}

/* SpecialReplacePanel::~SpecialReplacePanel
 * SpecialReplacePanel class destructor
 *******************************************************************/
SpecialReplacePanel::~SpecialReplacePanel()
{
}

/* SpecialReplacePanel::doReplace
 * Performs replace using settings from the panel controls for
 * [archive]
 *******************************************************************/
void SpecialReplacePanel::doReplace(Archive* archive)
{
	size_t count = ArchiveOperations::replaceSpecials(archive, spin_from->GetValue(), spin_to->GetValue(),
	               cb_line_specials->GetValue(), cb_thing_specials->GetValue(),
	               cb_args[0]->GetValue(), spin_args_from[0]->GetValue(), spin_args_to[0]->GetValue(),
	               cb_args[1]->GetValue(), spin_args_from[1]->GetValue(), spin_args_to[1]->GetValue(),
	               cb_args[2]->GetValue(), spin_args_from[2]->GetValue(), spin_args_to[2]->GetValue(),
	               cb_args[3]->GetValue(), spin_args_from[3]->GetValue(), spin_args_to[3]->GetValue(),
	               cb_args[4]->GetValue(), spin_args_from[4]->GetValue(), spin_args_to[4]->GetValue());

	wxMessageBox(S_FMT("Replaced %d occurrences. See console log for more detailed information.", count), "Replace Specials");
}


/*******************************************************************
 * TEXTUREREPLACEPANEL CLASS FUNCTIONS
 *******************************************************************/

/* TextureReplacePanel::TextureReplacePanel
 * TextureReplacePanel class constructor
 *******************************************************************/
TextureReplacePanel::TextureReplacePanel(wxWindow* parent) : wxPanel(parent, -1)
{
	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	wxGridBagSizer* gbsizer = new wxGridBagSizer(4, 4);
	sizer->AddStretchSpacer();
	sizer->Add(gbsizer, 0, wxALIGN_CENTER|wxALL, 4);

	// From texture
	gbsizer->Add(new wxStaticText(this, -1, "Replace Texture:"), wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT);
	text_from = new wxTextCtrl(this, -1);
	gbsizer->Add(text_from, wxGBPosition(0, 1), wxDefaultSpan, wxEXPAND);

	// To texture
	gbsizer->Add(new wxStaticText(this, -1, "With Texture:"), wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT);
	text_to = new wxTextCtrl(this, -1);
	gbsizer->Add(text_to, wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND);

	sizer->Add(new wxStaticLine(this, -1, wxDefaultPosition, wxDefaultSize, wxHORIZONTAL), 0, wxEXPAND|wxALL, 4);

	gbsizer = new wxGridBagSizer(4, 4);
	sizer->Add(gbsizer, 0, wxALIGN_CENTER|wxALL, 4);

	// Upper
	cb_upper = new wxCheckBox(this, -1, "Upper Textures");
	gbsizer->Add(cb_upper, wxGBPosition(0, 0), wxDefaultSpan, wxEXPAND);

	// Middle
	cb_middle = new wxCheckBox(this, -1, "Middle Textures");
	gbsizer->Add(cb_middle, wxGBPosition(1, 0), wxDefaultSpan, wxEXPAND);

	// Lower
	cb_lower = new wxCheckBox(this, -1, "Lower Textures");
	gbsizer->Add(cb_lower, wxGBPosition(2, 0), wxDefaultSpan, wxEXPAND);

	// Floors
	cb_floor = new wxCheckBox(this, -1, "Floor Textures");
	gbsizer->Add(cb_floor, wxGBPosition(0, 1), wxDefaultSpan, wxEXPAND);

	// Ceilings
	cb_ceiling = new wxCheckBox(this, -1, "Ceiling Textures");
	gbsizer->Add(cb_ceiling, wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND);

	sizer->AddStretchSpacer();
}

/* TextureReplacePanel::~TextureReplacePanel
 * TextureReplacePanel class destructor
 *******************************************************************/
TextureReplacePanel::~TextureReplacePanel()
{
}

/* TextureReplacePanel::doReplace
 * Performs replace using settings from the panel controls for
 * [archive]
 *******************************************************************/
void TextureReplacePanel::doReplace(Archive* archive)
{
	size_t count = ArchiveOperations::replaceTextures(archive, text_from->GetValue(), text_to->GetValue(),
	               cb_floor->GetValue(), cb_ceiling->GetValue(),
	               cb_lower->GetValue(), cb_middle->GetValue(), cb_upper->GetValue());

	wxMessageBox(S_FMT("Replaced %d occurrences. See console log for more detailed information.", count), "Replace Textures");
}


/*******************************************************************
 * MAPREPLACEDIALOG CLASS FUNCTIONS
 *******************************************************************/

/* MapReplaceDialog::MapReplaceDialog
 * MapReplaceDialog class constructor
 *******************************************************************/
MapReplaceDialog::MapReplaceDialog(wxWindow* parent, Archive* archive)
	: wxDialog(parent, -1, "Replace In Maps", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
	// Init variables
	this->archive = archive;

	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Add tabs
	stc_tabs = STabCtrl::createControl(this);
	sizer->Add(stc_tabs, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 10);

	// Thing type tab
	panel_thing = new ThingTypeReplacePanel(stc_tabs);
	stc_tabs->AddPage(panel_thing, "Thing Types");

	// Specials tab
	panel_special = new SpecialReplacePanel(stc_tabs);
	stc_tabs->AddPage(panel_special, "Specials");

	// Textures tab
	panel_texture = new TextureReplacePanel(stc_tabs);
	stc_tabs->AddPage(panel_texture, "Textures");

	// Dialog buttons
	btn_replace = new wxButton(this, -1, "Replace");
	btn_done = new wxButton(this, -1, "Close");
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	hbox->AddStretchSpacer();
	hbox->Add(btn_replace, 0, wxEXPAND|wxRIGHT, 4);
	hbox->Add(btn_done, 0, wxEXPAND, 4);
	sizer->AddSpacer(4);
	sizer->Add(hbox, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxEXPAND, 10);

	// Setup dialog layout
	SetInitialSize(wxSize(-1, -1));
	Layout();
	Fit();
	SetMinSize(GetBestSize());
	CenterOnParent();

	// Bind events
	btn_done->Bind(wxEVT_BUTTON, &MapReplaceDialog::onBtnDone, this);
	btn_replace->Bind(wxEVT_BUTTON, &MapReplaceDialog::onBtnReplace, this);
}

/* MapReplaceDialog::~MapReplaceDialog
 * MapReplaceDialog class destructor
 *******************************************************************/
MapReplaceDialog::~MapReplaceDialog()
{
}


/*******************************************************************
 * MAPREPLACEDIALOG CLASS EVENTS
 *******************************************************************/

/* MapReplaceDialog::onBtnDone
 * Called when the 'Done' button is clicked
 *******************************************************************/
void MapReplaceDialog::onBtnDone(wxCommandEvent& e)
{
	this->EndModal(wxID_OK);
}

/* MapReplaceDialog::onBtnReplace
 * Called when the 'Replace' button is clicked
 *******************************************************************/
void MapReplaceDialog::onBtnReplace(wxCommandEvent& e)
{
	// Get current tab
	int current = stc_tabs->GetSelection();

	// Thing types
	if (current == 0)
		panel_thing->doReplace(archive);

	// Specials
	else if (current == 1)
		panel_special->doReplace(archive);

	// Textures
	else if (current == 2)
		panel_texture->doReplace(archive);
}
