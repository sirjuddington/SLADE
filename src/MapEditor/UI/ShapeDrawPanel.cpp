
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ShapeDrawPanel.cpp
 * Description: A bar that shows up during shape drawing that
 *              contains options for shape drawing
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
#include "ShapeDrawPanel.h"


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Int, shapedraw_sides)
EXTERN_CVAR(Int, shapedraw_shape)
EXTERN_CVAR(Bool, shapedraw_centered)
EXTERN_CVAR(Bool, shapedraw_lockratio)


/*******************************************************************
 * SHAPEDRAWPANEL CLASS FUNCTIONS
 *******************************************************************/

/* ShapeDrawPanel::ShapeDrawPanel
 * ShapeDrawPanel class constructor
 *******************************************************************/
ShapeDrawPanel::ShapeDrawPanel(wxWindow* parent) : wxPanel(parent, -1)
{
	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Shape
	string shapes[] = { "Rectangle", "Ellipse" };
	choice_shape = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, 2, shapes);
	sizer_main = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(sizer_main, 0, wxEXPAND|wxALL, 4);
	sizer_main->Add(new wxStaticText(this, -1, "Shape:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 2);
	sizer_main->Add(choice_shape, 0, wxEXPAND|wxRIGHT, 8);

	// Centered
	cb_centered = new wxCheckBox(this, -1, "Centered");
	sizer_main->Add(cb_centered, 0, wxEXPAND|wxRIGHT, 8);

	// Lock ratio (1:1)
	cb_lockratio = new wxCheckBox(this, -1, "1:1 Size");
	sizer_main->Add(cb_lockratio, 0, wxEXPAND|wxRIGHT, 8);

	// Sides
	panel_sides = new wxPanel(this, -1);
	wxBoxSizer* hbox2 = new wxBoxSizer(wxHORIZONTAL);
	panel_sides->SetSizer(hbox2);
	spin_sides = new wxSpinCtrl(panel_sides, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxALIGN_LEFT|wxTE_PROCESS_ENTER, 3, 1000);
	hbox2->Add(new wxStaticText(panel_sides, -1, "Sides:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 2);
	hbox2->Add(spin_sides, 1, wxEXPAND);

	// Set control values
	choice_shape->SetSelection(shapedraw_shape);
	cb_centered->SetValue(shapedraw_centered);
	cb_lockratio->SetValue(shapedraw_lockratio);
	spin_sides->SetValue(shapedraw_sides);

	// Show shape controls with most options (to get minimum height)
	showShapeOptions(1);
	SetMinSize(GetBestSize());

	// Show controls for current shape
	showShapeOptions(shapedraw_shape);

	// Bind events
	choice_shape->Bind(wxEVT_CHOICE, &ShapeDrawPanel::onShapeChanged, this);
	cb_centered->Bind(wxEVT_CHECKBOX, &ShapeDrawPanel::onCenteredChecked, this);
	cb_lockratio->Bind(wxEVT_CHECKBOX, &ShapeDrawPanel::onLockRatioChecked, this);
	spin_sides->Bind(wxEVT_SPINCTRL, &ShapeDrawPanel::onSidesChanged, this);
	spin_sides->Bind(wxEVT_TEXT_ENTER, &ShapeDrawPanel::onSidesChanged, this);
}

/* ShapeDrawPanel::~ShapeDrawPanel
 * ShapeDrawPanel class destructor
 *******************************************************************/
ShapeDrawPanel::~ShapeDrawPanel()
{
}

/* ShapeDrawPanel::showShapeOptions
 * Shows option controls for [shape]
 *******************************************************************/
void ShapeDrawPanel::showShapeOptions(int shape)
{
	// Remove all extra options
	sizer_main->Detach(panel_sides);
	panel_sides->Show(false);

	// Polygon/Ellipse options
	if (shape == 1)
	{
		// Sides
		sizer_main->Add(panel_sides, 0, wxEXPAND|wxRIGHT, 8);
		panel_sides->Show(true);
	}

	Layout();
}


/*******************************************************************
 * SHAPEDRAWPANEL CLASS EVENTS
 *******************************************************************/

/* ShapeDrawPanel::onShapeChanged
 * Called when the shape dropdown is changed
 *******************************************************************/
void ShapeDrawPanel::onShapeChanged(wxCommandEvent& e)
{
	shapedraw_shape = choice_shape->GetSelection();
	showShapeOptions(shapedraw_shape);
}

/* ShapeDrawPanel::onCenteredChecked
 * Called when the 'centered' checkbox is clicked
 *******************************************************************/
void ShapeDrawPanel::onCenteredChecked(wxCommandEvent& e)
{
	shapedraw_centered = cb_centered->GetValue();
}

/* ShapeDrawPanel::onLockRatioChecked
 * Called when the 'lock ratio' checkbox is clicked
 *******************************************************************/
void ShapeDrawPanel::onLockRatioChecked(wxCommandEvent& e)
{
	shapedraw_lockratio = cb_lockratio->GetValue();
}

/* ShapeDrawPanel::onSidesChanged
 * Called when the no. of sides spin control is changed
 *******************************************************************/
void ShapeDrawPanel::onSidesChanged(wxCommandEvent& e)
{
	shapedraw_sides = spin_sides->GetValue();
}
