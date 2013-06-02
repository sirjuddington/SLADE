
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2012 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ModifyOffsetsDialog.cpp
 * Description: A dialog UI containing options for modifying gfx
 *              entry offsets
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
#include "ModifyOffsetsDialog.h"
#include "Console.h"
#include "Icons.h"


/*******************************************************************
 * MODIFYOFFSETSDIALOG CLASS FUNCTIONS
 *******************************************************************/

/* ModifyOffsetsDialog::ModifyOffsetsDialog
 * ModifyOffsetsDialog class constructor
 *******************************************************************/
ModifyOffsetsDialog::ModifyOffsetsDialog()
	:	wxDialog(NULL, -1, "Modify Gfx Offset(s)", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
	// Create main sizer
	wxBoxSizer* m_vbox = new wxBoxSizer(wxVERTICAL);
	SetSizer(m_vbox);

	// Set dialog icon
	wxIcon icon;
	icon.CopyFromBitmap(getIcon("t_offset"));
	SetIcon(icon);

	// Setup layout
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	m_vbox->Add(hbox, 0, wxEXPAND|wxALL, 4);

	// 'Set Offsets'
	opt_set = new wxRadioButton(this, -1, "Set Offsets", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	hbox->Add(opt_set, 1, wxEXPAND|wxALL, 4);

	entry_xoff = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxSize(40, -1));
	entry_yoff = new wxTextCtrl(this, -2, "", wxDefaultPosition, wxSize(40, -1));
	cbox_relative = new wxCheckBox(this, -1, "Relative");
	hbox->Add(entry_xoff, 0, wxEXPAND|wxALL, 4);
	hbox->Add(entry_yoff, 0, wxEXPAND|wxALL, 4);
	hbox->Add(cbox_relative, 0, wxEXPAND|wxALL, 4);

	hbox = new wxBoxSizer(wxHORIZONTAL);
	m_vbox->Add(hbox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// 'Auto Offsets'
	opt_auto = new wxRadioButton(this, -1, "Automatic Offsets");
	hbox->Add(opt_auto, 1, wxEXPAND|wxALL, 4);

	string offtypes[] =
	{
		"Monster",
		"Monster (GL-friendly)",
		"Projectile",
		"Hud/Weapon",
		"Hud/Weapon (Doom)",
		"Hud/Weapon (Heretic)",
		"Hud/Weapon (Hexen)",
	};

	combo_aligntype = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, 7, offtypes);
	combo_aligntype->Select(0);
	combo_aligntype->Enable(false);
	hbox->Add(combo_aligntype, 0, wxEXPAND|wxALL, 4);

	// Add default dialog buttons
	m_vbox->Add(CreateButtonSizer(wxOK|wxCANCEL), 0, wxEXPAND|wxALL, 4);


	// Bind events
	opt_auto->Bind(wxEVT_COMMAND_RADIOBUTTON_SELECTED, &ModifyOffsetsDialog::onOptAuto, this);
	opt_set->Bind(wxEVT_COMMAND_RADIOBUTTON_SELECTED, &ModifyOffsetsDialog::onOptSet, this);


	// Apply layout and size
	Layout();
	SetInitialSize(wxDefaultSize);
}

/* ModifyOffsetsDialog::~ModifyOffsetsDialog
 * ModifyOffsetsDialog class destructor
 *******************************************************************/
ModifyOffsetsDialog::~ModifyOffsetsDialog()
{
}

/* ModifyOffsetsDialog::getOffset
 * Returns the offsets that have been entered
 *******************************************************************/
point2_t ModifyOffsetsDialog::getOffset()
{
	long x = 0;
	long y = 0;
	entry_xoff->GetValue().ToLong(&x);
	entry_yoff->GetValue().ToLong(&y);

	return point2_t(x, y);
}

/* ModifyOffsetsDialog::getAlignType
 * Returns the selected alignment type (0=monster, 1=projectile,
 * 2=hud/weapon)
 *******************************************************************/
int	ModifyOffsetsDialog::getAlignType()
{
	if (opt_auto->GetValue())
		return combo_aligntype->GetSelection()-1;
	else
		return -1;
}

/* ModifyOffsetsDialog::autoOffset
 * Returns true if 'automatic offsets' is selected
 *******************************************************************/
bool ModifyOffsetsDialog::autoOffset()
{
	if (opt_auto->GetValue())
		return true;
	else
		return false;
}

/* ModifyOffsetsDialog::relativeOffset
 * Returns true if the 'relative' offset checkbox is checked
 *******************************************************************/
bool ModifyOffsetsDialog::relativeOffset()
{
	return cbox_relative->GetValue();
}

/* ModifyOffsetsDialog::xOffChange
 * Returns true if the user has entered an x-offset
 *******************************************************************/
bool ModifyOffsetsDialog::xOffChange()
{
	if (entry_xoff->GetValue().IsEmpty())
		return false;
	else
		return true;
}

/* ModifyOffsetsDialog::yOffChange
 * Returns true if the user has entered a y-offset
 *******************************************************************/
bool ModifyOffsetsDialog::yOffChange()
{
	if (entry_yoff->GetValue().IsEmpty())
		return false;
	else
		return true;
}


/*******************************************************************
 * MODIFYOFFSETSDIALOG EVENTS
 *******************************************************************/

/* ModifyOffsetsDialog::onOptSet
 * Called when the 'Set Offsets' radio button is selected
 *******************************************************************/
void ModifyOffsetsDialog::onOptSet(wxCommandEvent& e)
{
	entry_xoff->Enable(true);
	entry_yoff->Enable(true);
	cbox_relative->Enable(true);
	combo_aligntype->Enable(false);
}

/* ModifyOffsetsDialog::ModifyOffsetsDialog
 * Called when the 'Automatic Offsets' radio button is selected
 *******************************************************************/
void ModifyOffsetsDialog::onOptAuto(wxCommandEvent& e)
{
	entry_xoff->Enable(false);
	entry_yoff->Enable(false);
	cbox_relative->Enable(false);
	combo_aligntype->Enable(true);
}
