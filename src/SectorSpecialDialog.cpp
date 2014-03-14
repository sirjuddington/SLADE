
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    SectorSpecialDialog.cpp
 * Description: A dialog that allows selection of a sector special
 *              (and other related classes)
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
#include "SectorSpecialDialog.h"
#include "GameConfiguration.h"


/*******************************************************************
 * SECTORSPECIALDIALOG CLASS FUNCTIONS
 *******************************************************************/

/* SectorSpecialDialog::SectorSpecialDialog
 * SectorSpecialDialog class constructor
 *******************************************************************/
SectorSpecialDialog::SectorSpecialDialog(wxWindow* parent)
: SDialog(parent, "Select Sector Special", "sectorspecial")
{
	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Special list
	wxStaticBox* frame = new wxStaticBox(this, -1, "Special");
	wxStaticBoxSizer* framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	lv_specials = new ListView(this, -1);
	framesizer->Add(lv_specials, 1, wxEXPAND|wxALL, 4);
	sizer->Add(framesizer, 1, wxEXPAND|wxALL, 8);

	lv_specials->AppendColumn("#");
	lv_specials->AppendColumn("Name");
	vector<sectype_t> types = theGameConfiguration->allSectorTypes();
	for (unsigned a = 0; a < types.size(); a++)
	{
		wxArrayString item;
		item.Add(S_FMT("%d", types[a].type));
		item.Add(types[a].name);
		lv_specials->addItem(999999, item);
	}

	// Boom Flags
	int width = 300;
	if (theGameConfiguration->isBoom())
	{
		frame = new wxStaticBox(this, -1, "Flags");
		framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
		sizer->Add(framesizer, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 8);

		// Damage
		wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
		framesizer->Add(hbox, 0, wxEXPAND|wxALL, 4);
		string damage_types[] = { "None", "5%", "10%", "20%" };
		choice_damage = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, 4, damage_types);
		choice_damage->Select(0);
		hbox->Add(new wxStaticText(this, -1, "Damage:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
		hbox->Add(choice_damage, 1, wxEXPAND);

		// Secret
		hbox = new wxBoxSizer(wxHORIZONTAL);
		framesizer->Add(hbox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);
		cb_secret = new wxCheckBox(this, -1, "Secret");
		hbox->Add(cb_secret, 0, wxEXPAND|wxALL, 4);

		// Friction
		cb_friction = new wxCheckBox(this, -1, "Friction Enabled");
		hbox->Add(cb_friction, 0, wxEXPAND|wxALL, 4);

		// Pusher/Puller
		cb_pushpull = new wxCheckBox(this, -1, "Pushers/Pullers Enabled");
		hbox->Add(cb_pushpull, 0, wxEXPAND|wxALL, 4);

		width = -1;
	}

	// Dialog buttons
	sizer->Add(CreateButtonSizer(wxOK|wxCANCEL), 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Bind Events
	lv_specials->Bind(wxEVT_LIST_ITEM_ACTIVATED, &SectorSpecialDialog::onSpecialsListViewItemActivated, this);

	SetMinSize(wxSize(width, 400));
	CenterOnParent();
}

/* SectorSpecialDialog::~SectorSpecialDialog
 * SectorSpecialDialog class destructor
 *******************************************************************/
SectorSpecialDialog::~SectorSpecialDialog()
{
}

/* SectorSpecialDialog::setup
 * Sets up controls on the dialog to show [special]
 *******************************************************************/
void SectorSpecialDialog::setup(int special, int map_format)
{
	int base_type = theGameConfiguration->baseSectorType(special, map_format);

	// Select base type
	vector<sectype_t> types = theGameConfiguration->allSectorTypes();
	for (unsigned a = 0; a < types.size(); a++)
	{
		if (types[a].type == base_type)
		{
			lv_specials->selectItem(a);
			lv_specials->EnsureVisible(a);
			break;
		}
	}

	// Flags
	if (theGameConfiguration->isBoom())
	{
		// Damage
		choice_damage->Select(theGameConfiguration->sectorBoomDamage(special, map_format));

		// Secret
		cb_secret->SetValue(theGameConfiguration->sectorBoomSecret(special, map_format));

		// Friction
		cb_friction->SetValue(theGameConfiguration->sectorBoomFriction(special, map_format));

		// Pusher/Puller
		cb_pushpull->SetValue(theGameConfiguration->sectorBoomPushPull(special, map_format));
	}
}

/* SectorSpecialDialog::getSelectedSpecial
 * Returns the currently selected sector special
 *******************************************************************/
int SectorSpecialDialog::getSelectedSpecial(int map_format)
{
	vector<sectype_t> types = theGameConfiguration->allSectorTypes();
	int selection = lv_specials->selectedItems()[0];

	// Get selected base type
	int base = 0;
	if (selection < (int)types.size())
		base = types[selection].type;

	if (theGameConfiguration->isBoom())
		return theGameConfiguration->boomSectorType(base, choice_damage->GetSelection(), cb_secret->GetValue(), cb_friction->GetValue(), cb_pushpull->GetValue(), map_format);
	else
		return base;
}


/*******************************************************************
 * SECTORSPECIALDIALOG CLASS EVENTS
 *******************************************************************/

/* SectorSpecialDialog::onSpecialsListViewItemActivated
 * Called when an item in the sector specials tree is activated
 *******************************************************************/
void SectorSpecialDialog::onSpecialsListViewItemActivated(wxListEvent& e)
{
	EndModal(wxID_OK);
}
