
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ModifyOffsetsDialog.cpp
// Description: A dialog UI containing options for modifying gfx entry offsets
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
#include "ModifyOffsetsDialog.h"
#include "General/UI.h"
#include "Graphics/Icons.h"


// -----------------------------------------------------------------------------
//
// ModifyOffsetsDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ModifyOffsetsDialog class constructor
// -----------------------------------------------------------------------------
ModifyOffsetsDialog::ModifyOffsetsDialog() :
	wxDialog(nullptr, -1, "Modify Gfx Offset(s)", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
	// Create main sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);
	wxBoxSizer* m_vbox = new wxBoxSizer(wxVERTICAL);
	sizer->Add(m_vbox, 1, wxEXPAND | wxALL, UI::padLarge());

	// Set dialog icon
	wxIcon icon;
	icon.CopyFromBitmap(Icons::getIcon(Icons::GENERAL, "offset"));
	SetIcon(icon);

	// Setup layout
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	m_vbox->Add(hbox, 0, wxEXPAND | wxBOTTOM, UI::padLarge());

	// 'Auto Offsets'
	opt_auto_ = new wxRadioButton(this, -1, "Automatic Offsets", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	hbox->Add(opt_auto_, 1, wxEXPAND | wxRIGHT, UI::pad());

	string offtypes[] = {
		"Monster",           "Monster (GL-friendly)", "Projectile",         "Hud/Weapon",
		"Hud/Weapon (Doom)", "Hud/Weapon (Heretic)",  "Hud/Weapon (Hexen)",
	};

	combo_aligntype_ = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, 7, offtypes);
	combo_aligntype_->Select(0);
	hbox->Add(combo_aligntype_, 0, wxEXPAND);

	hbox = new wxBoxSizer(wxHORIZONTAL);
	m_vbox->Add(hbox, 0, wxEXPAND | wxBOTTOM, UI::padLarge());

	// 'Set Offsets'
	opt_set_ = new wxRadioButton(this, -1, "Set Offsets");
	hbox->Add(opt_set_, 1, wxEXPAND | wxRIGHT, UI::pad());

	int width      = UI::scalePx(40);
	entry_xoff_    = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(width, -1));
	entry_yoff_    = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(width, -1));
	cbox_relative_ = new wxCheckBox(this, wxID_ANY, "Relative");
	hbox->Add(entry_xoff_, 0, wxEXPAND | wxRIGHT, UI::pad());
	hbox->Add(entry_yoff_, 0, wxEXPAND | wxRIGHT, UI::pad());
	hbox->Add(cbox_relative_, 0, wxEXPAND);
	entry_xoff_->Enable(false);
	entry_yoff_->Enable(false);
	cbox_relative_->Enable(false);

	// Add default dialog buttons
	m_vbox->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND);


	// Bind events
	opt_auto_->Bind(wxEVT_RADIOBUTTON, &ModifyOffsetsDialog::onOptAuto, this);
	opt_set_->Bind(wxEVT_RADIOBUTTON, &ModifyOffsetsDialog::onOptSet, this);


	// Apply layout and size
	Layout();
	SetInitialSize(wxDefaultSize);
}

// -----------------------------------------------------------------------------
// ModifyOffsetsDialog class destructor
// -----------------------------------------------------------------------------
ModifyOffsetsDialog::~ModifyOffsetsDialog() {}

// -----------------------------------------------------------------------------
// Returns the offsets that have been entered
// -----------------------------------------------------------------------------
point2_t ModifyOffsetsDialog::getOffset()
{
	long x = 0;
	long y = 0;
	entry_xoff_->GetValue().ToLong(&x);
	entry_yoff_->GetValue().ToLong(&y);

	return point2_t(x, y);
}

// -----------------------------------------------------------------------------
// Returns the selected alignment type
// -----------------------------------------------------------------------------
int ModifyOffsetsDialog::getAlignType()
{
	if (opt_auto_->GetValue())
		return combo_aligntype_->GetSelection();
	else
		return -1;
}

// -----------------------------------------------------------------------------
// Returns true if 'automatic offsets' is selected
// -----------------------------------------------------------------------------
bool ModifyOffsetsDialog::autoOffset()
{
	if (opt_auto_->GetValue())
		return true;
	else
		return false;
}

// -----------------------------------------------------------------------------
// Returns true if the 'relative' offset checkbox is checked
// -----------------------------------------------------------------------------
bool ModifyOffsetsDialog::relativeOffset()
{
	return cbox_relative_->GetValue();
}

// -----------------------------------------------------------------------------
// Returns true if the user has entered an x-offset
// -----------------------------------------------------------------------------
bool ModifyOffsetsDialog::xOffChange()
{
	if (entry_xoff_->GetValue().IsEmpty())
		return false;
	else
		return true;
}

// -----------------------------------------------------------------------------
// Returns true if the user has entered a y-offset
// -----------------------------------------------------------------------------
bool ModifyOffsetsDialog::yOffChange()
{
	if (entry_yoff_->GetValue().IsEmpty())
		return false;
	else
		return true;
}

// -----------------------------------------------------------------------------
// Calculates the modified offsets for a graphic with existing offsets
// [xoff,yoff] and size [width,height], based on the currently selected options
// in the dialog
// -----------------------------------------------------------------------------
point2_t ModifyOffsetsDialog::calculateOffsets(int xoff, int yoff, int width, int height)
{
	int      type   = getAlignType();
	point2_t offset = getOffset();
	int      x      = xoff;
	int      y      = yoff;

	if (type >= 0)
	{
		// Monster
		if (type == 0)
		{
			x = width * 0.5;
			y = height - 4;
		}

		// Monster (GL-friendly)
		else if (type == 1)
		{
			x = width * 0.5;
			y = height;
		}

		// Projectile
		else if (type == 2)
		{
			x = width * 0.5;
			y = height * 0.5;
		}

		// Weapon (Fullscreen)
		else if (type == 3)
		{
			x = -160 + (width * 0.5);
			y = -200 + height;
		}

		// Weapon (Doom status bar)
		else if (type == 4)
		{
			x = -160 + (width * 0.5);
			y = -200 + 32 + height;
		}

		// Weapon (Heretic status bar)
		else if (type == 5)
		{
			x = -160 + (width * 0.5);
			y = -200 + 42 + height;
		}

		// Weapon (Hexen status bar)
		else if (type == 6)
		{
			x = -160 + (width * 0.5);
			y = -200 + 38 + height;
		}
	}
	else
	{
		// Relative offset
		if (relativeOffset())
		{
			if (xOffChange())
				x = xoff + offset.x;
			if (yOffChange())
				y = yoff + offset.y;
		}

		// Set offset
		else
		{
			if (xOffChange())
				x = offset.x;
			if (yOffChange())
				y = offset.y;
		}
	}

	return point2_t(x, y);
}


// -----------------------------------------------------------------------------
//
// ModifyOffsetsDialog Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the 'Set Offsets' radio button is selected
// -----------------------------------------------------------------------------
void ModifyOffsetsDialog::onOptSet(wxCommandEvent& e)
{
	entry_xoff_->Enable(true);
	entry_yoff_->Enable(true);
	cbox_relative_->Enable(true);
	combo_aligntype_->Enable(false);
}

// -----------------------------------------------------------------------------
// Called when the 'Automatic Offsets' radio button is selected
// -----------------------------------------------------------------------------
void ModifyOffsetsDialog::onOptAuto(wxCommandEvent& e)
{
	entry_xoff_->Enable(false);
	entry_yoff_->Enable(false);
	cbox_relative_->Enable(false);
	combo_aligntype_->Enable(true);
}
