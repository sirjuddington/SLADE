
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    PaletteDialog.cpp
 * Description: A simple dialog that contains a palette canvas, and
 *              OK/Cancel buttons, allowing the user to select a
 *              colour in the palette
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
#include "PaletteDialog.h"
#include "Graphics/Palette/Palette.h"
#include "UI/Canvas/PaletteCanvas.h"


/*******************************************************************
 * PALETTEDIALOG CLASS FUNCTIONS
 *******************************************************************/

/* PaletteDialog::PaletteDialog
 * PaletteDialog class constructor
 *******************************************************************/
PaletteDialog::PaletteDialog(Palette* palette)
	: wxDialog(nullptr, -1, "Palette", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
	wxBoxSizer* m_vbox = new wxBoxSizer(wxVERTICAL);
	SetSizer(m_vbox);

	pal_canvas = new PaletteCanvas(this, -1);
	pal_canvas->getPalette().copyPalette(palette);
	pal_canvas->SetInitialSize(wxSize(400, 400));
	pal_canvas->allowSelection(1);
	m_vbox->Add(pal_canvas, 1, wxEXPAND|wxALL, 10);

	m_vbox->AddSpacer(4);
	m_vbox->Add(CreateStdDialogButtonSizer(wxOK|wxCANCEL), 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 10);

	// Bind events
	pal_canvas->Bind(wxEVT_LEFT_DCLICK, &PaletteDialog::onLeftDoubleClick, this);

	// Autosize to fit contents (and set this as the minimum size)
	SetInitialSize(wxSize(-1, -1));
	SetMinSize(GetSize());
}

/* PaletteDialog::~PaletteDialog
 * PaletteDialog class destructor
 *******************************************************************/
PaletteDialog::~PaletteDialog()
{
	if (pal_canvas)
		delete pal_canvas;
}

/* PaletteDialog::getSelectedColour
 * Returns the currently selected coloir on the palette canvas
 *******************************************************************/
rgba_t PaletteDialog::getSelectedColour()
{
	return pal_canvas->getSelectedColour();
}


/*******************************************************************
 * PALETTEDIALOG EVENTS
 *******************************************************************/

/* PaletteDialog::onLeftDoubleClick
 * Called when the palette canvas is double clicked
 *******************************************************************/
void PaletteDialog::onLeftDoubleClick(wxMouseEvent& e)
{
	EndModal(wxID_OK);
}
