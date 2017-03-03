
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ColourBox.cpp
 * Description: ColourBox class. A simple box that allows the user
 *              to select a colour. It shows the current colour and
 *              alpha level (if enabled), left clicking on the box
 *              will open either an OS-native colour chooser or a
 *              palette dialog if a palette is supplied so the user
 *              can choose a colour. Right clicking the box pops up
 *              a slider to change the alpha level of the colour
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
#include "ColourBox.h"
#include "Dialogs/PaletteDialog.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
DEFINE_EVENT_TYPE(wxEVT_COLOURBOX_CHANGED)


/*******************************************************************
 * COLOURBOX CLASS FUNCTIONS
 *******************************************************************/

/* ColourBox::ColourBox
 * ColourBox class constructor
 *******************************************************************/
ColourBox::ColourBox(wxWindow* parent, int id, bool enable_alpha, bool mode)
	: wxPanel(parent, id, wxDefaultPosition, wxSize(32, 24), wxSUNKEN_BORDER)
{
	alpha = enable_alpha;
	palette = NULL;
	colour = COL_BLACK;
	altmode = mode;

	// Bind events
	Bind(wxEVT_PAINT, &ColourBox::onPaint, this);
	Bind(wxEVT_LEFT_DOWN, &ColourBox::onMouseLeftDown, this);
	Bind(wxEVT_RIGHT_DOWN, &ColourBox::onMouseRightDown, this);
}

/* ColourBox::ColourBox
 * Alternate ColourBox class constructor
 *******************************************************************/
ColourBox::ColourBox(wxWindow* parent, int id, rgba_t col, bool enable_alpha, bool mode)
	: wxPanel(parent, id, wxDefaultPosition, wxSize(32, 24), wxSUNKEN_BORDER)
{
	alpha = enable_alpha;
	palette = NULL;
	colour = col;
	altmode = mode;

	// Bind events
	Bind(wxEVT_PAINT, &ColourBox::onPaint, this);
	Bind(wxEVT_LEFT_DOWN, &ColourBox::onMouseLeftDown, this);
	Bind(wxEVT_RIGHT_DOWN, &ColourBox::onMouseRightDown, this);
}

/* ColourBox::~ColourBox
 * ColourBox class destructor
 *******************************************************************/
ColourBox::~ColourBox()
{
}

/* ColourBox::sendChangeEvent
 * Generates and sends a wxEVT_COLOURBOX_CHANGED event
 *******************************************************************/
void ColourBox::sendChangeEvent()
{
	wxCommandEvent e(wxEVT_COLOURBOX_CHANGED, GetId());
	e.SetEventObject(this);
	GetEventHandler()->ProcessEvent(e);
}

/* ColourBox::PopPalette
 * Pops up a palette dialog if palette data is available, and sends
 * a change event after a colour is selected.
 *******************************************************************/
void ColourBox::popPalette()
{
	if (palette)
	{
		PaletteDialog pd(palette);
		if (pd.ShowModal() == wxID_OK)
		{
			rgba_t col = pd.getSelectedColour();
			if (col.a > 0)
			{
				colour = col;
				sendChangeEvent();
				Refresh();
			}
		}
	}
}

/* ColourBox::PopColourPicker
 * Pops up a standard colour picker dialog, and sends a change event 
 * after a colour is selected.
 *******************************************************************/
void ColourBox::popColourPicker()
{
	wxColour col = wxGetColourFromUser(GetParent(), wxColour(colour.r, colour.g, colour.b));

	if (col.Ok())
	{
		colour.r = col.Red();
		colour.g = col.Green();
		colour.b = col.Blue();
		sendChangeEvent();
		Refresh();
	}
}

/* ColourBox::PopAlphaSlider
 * Pops up an alpha slider control if alpha is enabled, and sends a
 * change event after a value is selected.
 *******************************************************************/
void ColourBox::popAlphaSlider()
{
	// Do nothing if alpha disabled
	if (!alpha)
		return;

	// Popup a dialog with a slider control for alpha
	wxDialog dlg(NULL, -1, "Set Alpha", wxDefaultPosition, wxDefaultSize);
	wxBoxSizer* box = new wxBoxSizer(wxVERTICAL);
	dlg.SetSizer(box);
	wxSlider* slider = new wxSlider(&dlg, -1, colour.a, 0, 255, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
	box->Add(slider, 1, wxEXPAND | wxALL, 4);
	box->Add(dlg.CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 4);
	dlg.SetInitialSize();

	if (dlg.ShowModal() == wxID_OK)
	{
		colour.a = slider->GetValue();
		sendChangeEvent();
		Refresh();
	}
}


/*******************************************************************
 * COLOURBOX EVENTS
 *******************************************************************/

/* ColourBox::onPaint
 * Called when the colour box needs to be (re)drawn
 *******************************************************************/
void ColourBox::onPaint(wxPaintEvent& e)
{
	wxPaintDC dc(this);

	dc.SetBrush(wxBrush(wxColour(colour.r, colour.g, colour.b)));
	dc.DrawRectangle(0, 0, GetClientSize().x, GetClientSize().y);

	if (alpha)
	{
		int a_point = colour.fa() * (GetClientSize().x - 2);

		dc.SetBrush(wxBrush(wxColour(0, 0, 0)));
		dc.DrawRectangle(0, 0, GetClientSize().x, 4);

		dc.SetBrush(wxBrush(wxColour(255, 255, 255)));
		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.DrawRectangle(1, 1, a_point, 2);
	}
}

/* ColourBox::onMouseLeftDown
 * Called when the colour box is left clicked. 
 *******************************************************************/
void ColourBox::onMouseLeftDown(wxMouseEvent& e)
{
	if (!palette || altmode)
	{
		popColourPicker();
	}
	else
	{
		popPalette();
	}
}

/* ColourBox::onMouseRightDown
 * Called when the colour box is right clicked. 
 *******************************************************************/
void ColourBox::onMouseRightDown(wxMouseEvent& e)
{
	if (altmode && palette)
	{
		popPalette();
	}
	else if (alpha)
	{
		popAlphaSlider();
	}
	else
	{
		popColourPicker();
	}
}
