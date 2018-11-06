
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ShapeDrawPanel.cpp
// Description: A bar that shows up during shape drawing that contains options
//              for shape drawing
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
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#include "ShapeDrawPanel.h"
#include "UI/WxUtils.h"


// ----------------------------------------------------------------------------
//
// External Variables
//
// ----------------------------------------------------------------------------
EXTERN_CVAR(Int, shapedraw_sides)
EXTERN_CVAR(Int, shapedraw_shape)
EXTERN_CVAR(Bool, shapedraw_centered)
EXTERN_CVAR(Bool, shapedraw_lockratio)


// ----------------------------------------------------------------------------
//
// ShapeDrawPanel Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// ShapeDrawPanel::ShapeDrawPanel
//
// ShapeDrawPanel class constructor
// ----------------------------------------------------------------------------
ShapeDrawPanel::ShapeDrawPanel(wxWindow* parent) : wxPanel{ parent, -1 }
{
	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Shape
	string shapes[] = { "Rectangle", "Ellipse" };
	choice_shape_ = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, 2, shapes);
	sizer_main_ = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(sizer_main_, 0, wxEXPAND|wxALL, UI::pad());
	sizer_main_->Add(WxUtils::createLabelHBox(this, "Shape:", choice_shape_), 0, wxEXPAND | wxRIGHT, UI::padLarge());

	// Centered
	cb_centered_ = new wxCheckBox(this, -1, "Centered");
	sizer_main_->Add(cb_centered_, 0, wxEXPAND|wxRIGHT, UI::padLarge());

	// Lock ratio (1:1)
	cb_lockratio_ = new wxCheckBox(this, -1, "1:1 Size");
	sizer_main_->Add(cb_lockratio_, 0, wxEXPAND|wxRIGHT, UI::padLarge());

	// Sides
	panel_sides_ = new wxPanel(this, -1);
	wxBoxSizer* hbox2 = new wxBoxSizer(wxHORIZONTAL);
	panel_sides_->SetSizer(hbox2);
	spin_sides_ = new wxSpinCtrl(panel_sides_, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxALIGN_LEFT|wxTE_PROCESS_ENTER, 3, 1000);
	hbox2->Add(WxUtils::createLabelHBox(panel_sides_, "Sides:", spin_sides_), 1, wxEXPAND);

	// Set control values
	choice_shape_->SetSelection(shapedraw_shape);
	cb_centered_->SetValue(shapedraw_centered);
	cb_lockratio_->SetValue(shapedraw_lockratio);
	spin_sides_->SetValue(shapedraw_sides);

	// Show shape controls with most options (to get minimum height)
	showShapeOptions(1);
	SetMinSize(GetBestSize());

	// Show controls for current shape
	showShapeOptions(shapedraw_shape);

	// Bind events
	choice_shape_->Bind(wxEVT_CHOICE, [&](wxCommandEvent&)
	{
		shapedraw_shape = choice_shape_->GetSelection();
		showShapeOptions(shapedraw_shape);
	});
	cb_centered_->Bind(wxEVT_CHECKBOX, [&](wxCommandEvent&) { shapedraw_centered = cb_centered_->GetValue(); });
	cb_lockratio_->Bind(wxEVT_CHECKBOX, [&](wxCommandEvent&) { shapedraw_lockratio = cb_lockratio_->GetValue(); });
	spin_sides_->Bind(wxEVT_SPINCTRL, [&](wxCommandEvent&) { shapedraw_sides = spin_sides_->GetValue(); });
	spin_sides_->Bind(wxEVT_TEXT_ENTER, [&](wxCommandEvent&) { shapedraw_sides = spin_sides_->GetValue(); });
}

// ----------------------------------------------------------------------------
// ShapeDrawPanel::showShapeOptions
//
// Shows option controls for [shape]
// ----------------------------------------------------------------------------
void ShapeDrawPanel::showShapeOptions(int shape)
{
	// Remove all extra options
	sizer_main_->Detach(panel_sides_);
	panel_sides_->Show(false);

	// Polygon/Ellipse options
	if (shape == 1)
	{
		// Sides
		sizer_main_->Add(panel_sides_, 0, wxEXPAND|wxRIGHT, UI::padLarge());
		panel_sides_->Show(true);
	}

	Layout();
}
