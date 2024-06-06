
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ColourBox.cpp
// Description: ColourBox class. A simple box that allows the user to select a
//              colour. It shows the current colour and alpha level
//              (if enabled), left clicking on the box will open either an
//              OS-native colour chooser or a palette dialog if a palette is
//              supplied so the user can choose a colour. Right clicking the
//              box pops up a slider to change the alpha level of the colour
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
#include "ColourBox.h"
#include "Graphics/Palette/Palette.h"
#include "UI/Dialogs/PaletteDialog.h"
#include "UI/Layout.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
DEFINE_EVENT_TYPE(wxEVT_COLOURBOX_CHANGED)


// -----------------------------------------------------------------------------
//
// ColourBox Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ColourBox class constructor
// -----------------------------------------------------------------------------
ColourBox::ColourBox(wxWindow* parent, int id, bool enable_alpha, bool mode) :
	wxPanel{ parent, id, wxDefaultPosition, parent->FromDIP(wxSize(32, 22)), wxNO_BORDER },
	alpha_{ enable_alpha },
	altmode_{ mode }
{
	// Bind events
	Bind(wxEVT_PAINT, &ColourBox::onPaint, this);
	Bind(wxEVT_LEFT_DOWN, &ColourBox::onMouseLeftDown, this);
	Bind(wxEVT_RIGHT_DOWN, &ColourBox::onMouseRightDown, this);
}

// -----------------------------------------------------------------------------
// Alternate ColourBox class constructor
// -----------------------------------------------------------------------------
ColourBox::ColourBox(wxWindow* parent, int id, ColRGBA col, bool enable_alpha, bool mode, int size) :
	wxPanel{ parent, id, wxDefaultPosition, wxDefaultSize, wxNO_BORDER },
	colour_{ col },
	alpha_{ enable_alpha },
	altmode_{ mode }
{
	if (size > 0)
		SetInitialSize(FromDIP(wxSize{ size, size }));
	else
		SetInitialSize(FromDIP(wxSize(32, 22)));

	// Bind events
	Bind(wxEVT_PAINT, &ColourBox::onPaint, this);
	Bind(wxEVT_LEFT_DOWN, &ColourBox::onMouseLeftDown, this);
	Bind(wxEVT_RIGHT_DOWN, &ColourBox::onMouseRightDown, this);
}

// -----------------------------------------------------------------------------
// Generates and sends a wxEVT_COLOURBOX_CHANGED event
// -----------------------------------------------------------------------------
void ColourBox::sendChangeEvent()
{
	wxCommandEvent e(wxEVT_COLOURBOX_CHANGED, GetId());
	e.SetEventObject(this);
	GetEventHandler()->ProcessEvent(e);
}

// -----------------------------------------------------------------------------
// Pops up a palette dialog if palette data is available, and sends a change
// event after a colour is selected.
// -----------------------------------------------------------------------------
void ColourBox::popPalette()
{
	if (palette_)
	{
		PaletteDialog pd(palette_);
		if (pd.ShowModal() == wxID_OK)
		{
			auto col = pd.selectedColour();
			if (col.a > 0)
			{
				colour_ = col;
				sendChangeEvent();
				Refresh();
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Pops up a standard colour picker dialog, and sends a change event  after a
// colour is selected.
// -----------------------------------------------------------------------------
void ColourBox::popColourPicker()
{
	auto col = wxGetColourFromUser(GetParent(), wxColour(colour_.r, colour_.g, colour_.b));

	if (col.Ok())
	{
		colour_.r     = col.Red();
		colour_.g     = col.Green();
		colour_.b     = col.Blue();
		colour_.index = -1;

		if (palette_)
		{
			auto index = palette_->nearestColour(colour_);
			auto pcol  = palette_->colour(index);
			if (pcol.equals(colour_))
				colour_.index = index;
		}
		sendChangeEvent();
		Refresh();
	}
}

// -----------------------------------------------------------------------------
// Pops up an alpha slider control if alpha is enabled, and sends a change
// event after a value is selected.
// -----------------------------------------------------------------------------
void ColourBox::popAlphaSlider()
{
	// Do nothing if alpha disabled
	if (!alpha_)
		return;

	// Popup a dialog with a slider control for alpha
	wxDialog dlg(nullptr, -1, "Set Alpha", wxDefaultPosition, wxDefaultSize);
	auto     lh  = ui::LayoutHelper(&dlg);
	auto     box = new wxBoxSizer(wxVERTICAL);
	dlg.SetSizer(box);
	auto slider = new wxSlider(&dlg, -1, colour_.a, 0, 255, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
	box->Add(slider, lh.sfWithLargeBorder(1).Expand());
	box->Add(dlg.CreateButtonSizer(wxOK | wxCANCEL), lh.sfWithLargeBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Expand());
	dlg.SetInitialSize();

	if (dlg.ShowModal() == wxID_OK)
	{
		colour_.a = slider->GetValue();
		sendChangeEvent();
		Refresh();
	}
}


// -----------------------------------------------------------------------------
//
// ColourBox Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the colour box needs to be (re)drawn
// -----------------------------------------------------------------------------
void ColourBox::onPaint(wxPaintEvent& e)
{
	wxPaintDC dc(this);

	dc.SetBrush(wxBrush(wxColour(colour_.r, colour_.g, colour_.b)));
	const wxSize client_size = GetClientSize() * GetContentScaleFactor();
	dc.DrawRectangle(0, 0, client_size.x, client_size.y);

	if (alpha_)
	{
		int a_height       = FromDIP(4);
		int a_border_width = FromDIP(1);
		int a_point        = colour_.fa() * (client_size.x - (2 * a_border_width));

		dc.SetBrush(wxBrush(wxColour(0, 0, 0)));
		dc.DrawRectangle(0, 0, client_size.x, a_height);

		dc.SetBrush(wxBrush(wxColour(255, 255, 255)));
		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.DrawRectangle(a_border_width, a_border_width, a_point, a_height - (a_border_width * 2));
	}
}

// -----------------------------------------------------------------------------
// Called when the colour box is left clicked.
// -----------------------------------------------------------------------------
void ColourBox::onMouseLeftDown(wxMouseEvent& e)
{
	if (!palette_ || altmode_)
		popColourPicker();
	else
		popPalette();
}

// -----------------------------------------------------------------------------
// Called when the colour box is right clicked.
// -----------------------------------------------------------------------------
void ColourBox::onMouseRightDown(wxMouseEvent& e)
{
	if (altmode_ && palette_)
		popPalette();
	else if (alpha_)
		popAlphaSlider();
	else
		popColourPicker();
}
