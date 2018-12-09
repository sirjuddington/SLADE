
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    PaletteDialog.cpp
// Description: A simple dialog that contains a palette canvas, and OK/Cancel
//              buttons, allowing the user to select a colour in the palette
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
#include "PaletteDialog.h"
#include "General/UI.h"
#include "Graphics/Palette/Palette.h"
#include "UI/Canvas/PaletteCanvas.h"


// -----------------------------------------------------------------------------
//
// PaletteDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// PaletteDialog class constructor
// -----------------------------------------------------------------------------
PaletteDialog::PaletteDialog(Palette* palette) :
	wxDialog(nullptr, -1, "Palette", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	int size = UI::scalePx(400);

	auto m_vbox = new wxBoxSizer(wxVERTICAL);
	SetSizer(m_vbox);

	pal_canvas_ = new PaletteCanvas(this, -1);
	pal_canvas_->palette().copyPalette(palette);
	pal_canvas_->SetInitialSize(wxSize(size, size));
	pal_canvas_->setSelectionType(PaletteCanvas::SelectionType::One);
	m_vbox->Add(pal_canvas_, 1, wxEXPAND | wxALL, UI::padLarge());

	m_vbox->AddSpacer(UI::pad());
	m_vbox->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, UI::padLarge());

	// Bind events
	pal_canvas_->Bind(wxEVT_LEFT_DCLICK, [&](wxMouseEvent&) { EndModal(wxID_OK); });

	// Autosize to fit contents (and set this as the minimum size)
	SetInitialSize(wxSize(-1, -1));
	wxTopLevelWindowBase::SetMinSize(GetSize());
}

// -----------------------------------------------------------------------------
// Returns the currently selected coloir on the palette canvas
// -----------------------------------------------------------------------------
ColRGBA PaletteDialog::selectedColour() const
{
	return pal_canvas_->selectedColour();
}
