
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
#include "SectorSpecialDialog.h"
#include "Game/Configuration.h"

/*******************************************************************
 * SECTORSPECIALPANEL CLASS FUNCTIONS
 *******************************************************************/

/* SectorSpecialPanel::SectorSpecialPanel
 * SectorSpecialPanel class constructor
 *******************************************************************/
SectorSpecialPanel::SectorSpecialPanel(wxWindow* parent) : wxPanel(parent, -1)
{
	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Special list
	wxStaticBox* frame = new wxStaticBox(this, -1, "Special");
	wxStaticBoxSizer* framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	lv_specials = new ListView(this, -1);
	framesizer->Add(lv_specials, 1, wxEXPAND|wxALL, 4);
	sizer->Add(framesizer, 1, wxEXPAND);

	lv_specials->enableSizeUpdate(false);
	lv_specials->AppendColumn("#");
	lv_specials->AppendColumn("Name");
	auto& types = Game::configuration().allSectorTypes();
	for (auto& type : types)
	{
		wxArrayString item;
		item.Add(S_FMT("%d", type.first));
		item.Add(type.second);
		lv_specials->addItem(999999, item);
	}
	lv_specials->enableSizeUpdate(true);
	lv_specials->updateSize();

	// Boom Flags
	int width = 300;
	if (Game::configuration().supportsSectorFlags())
	{
		frame = new wxStaticBox(this, -1, "Flags");
		framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
		sizer->Add(framesizer, 0, wxEXPAND|wxTOP, 4);

		// Damage
		wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
		framesizer->Add(hbox, 0, wxEXPAND|wxALL, 4);
		string damage_types[] ={ "None", "5%", "10%", "20%" };
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

	SetMinSize(wxSize(width, 200));
}

/* SectorSpecialPanel::~SectorSpecialPanel
 * SectorSpecialPanel class destructor
 *******************************************************************/
SectorSpecialPanel::~SectorSpecialPanel()
{
}

/* SectorSpecialPanel::setup
 * Sets up controls on the dialog to show [special]
 *******************************************************************/
void SectorSpecialPanel::setup(int special)
{
	int base_type = Game::configuration().baseSectorType(special);

	// Select base type
	auto& types = Game::configuration().allSectorTypes();
	int index = 0;
	for (auto& i : types)
	{
		if (i.first == base_type)
		{
			lv_specials->selectItem(index);
			lv_specials->EnsureVisible(index);
			break;
		}

		index++;
	}

	// Flags
	if (Game::configuration().supportsSectorFlags())
	{
		// Damage
		choice_damage->Select(Game::configuration().sectorBoomDamage(special));

		// Secret
		cb_secret->SetValue(Game::configuration().sectorBoomSecret(special));

		// Friction
		cb_friction->SetValue(Game::configuration().sectorBoomFriction(special));

		// Pusher/Puller
		cb_pushpull->SetValue(Game::configuration().sectorBoomPushPull(special));
	}
}

/* SectorSpecialPanel::getSelectedSpecial
 * Returns the currently selected sector special
 *******************************************************************/
int SectorSpecialPanel::getSelectedSpecial()
{
	auto& types = Game::configuration().allSectorTypes();
	int selection = 0;
	wxArrayInt items = lv_specials->selectedItems();
	if (items.GetCount())
		selection = items[0];

	// Get selected base type
	int base = 0;
	if (selection < (int)types.size())
	{
		int index = 0;
		for (auto& i : types)
		{
			if (index == selection)
			{
				base = i.first;
				break;
			}

			index++;
		}
	}

	if (Game::configuration().supportsSectorFlags())
	{
		return Game::configuration().boomSectorType(
			base,
			choice_damage->GetSelection(),
			cb_secret->GetValue(),
			cb_friction->GetValue(),
			cb_pushpull->GetValue()
		);
	}
	else
		return base;
}


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

	// Special panel
	panel_special = new SectorSpecialPanel(this);
	sizer->Add(panel_special, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 10);

	// Dialog buttons
	sizer->AddSpacer(4);
	sizer->Add(CreateButtonSizer(wxOK|wxCANCEL), 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 10);

	// Bind Events
	panel_special->getSpecialsList()->Bind(wxEVT_LIST_ITEM_ACTIVATED, &SectorSpecialDialog::onSpecialsListViewItemActivated, this);

	SetMinClientSize(sizer->GetMinSize());
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
void SectorSpecialDialog::setup(int special)
{
	panel_special->setup(special);
}

/* SectorSpecialDialog::getSelectedSpecial
 * Returns the currently selected sector special
 *******************************************************************/
int SectorSpecialDialog::getSelectedSpecial()
{
	return panel_special->getSelectedSpecial();
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
