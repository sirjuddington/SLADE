
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    GenLineSpecialPanel.cpp
 * Description: Panel with controls to show/set a Boom generalised
 *              line special
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
#include "Game/GenLineSpecial.h"
#include "MapEditor/UI/GenLineSpecialPanel.h"


/*******************************************************************
 * GENLINESPECIALPANEL CLASS FUNCTIONS
 *******************************************************************/

/* GenLineSpecialPanel::GenLineSpecialPanel
 * GenLineSpecialPanel class constructor
 *******************************************************************/
GenLineSpecialPanel::GenLineSpecialPanel(wxWindow* parent) : wxPanel(parent, -1)
{
	// --- Setup layout ---
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Special Type
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxALL, 4);
	hbox->Add(new wxStaticText(this, -1, "Type:"), 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 4);
	string types[] = { "Floor", "Ceiling", "Door", "Locked Door", "Lift", "Stairs", "Crusher" };
	choice_type = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, 7, types);
	choice_type->Bind(wxEVT_CHOICE, &GenLineSpecialPanel::onChoiceTypeChanged, this);
	hbox->Add(choice_type, 1, wxEXPAND);

	gb_sizer = new wxGridBagSizer(4, 4);
	sizer->Add(gb_sizer, 1, wxEXPAND|wxALL, 4);

	// Trigger
	string triggers[] = {"Cross (Once)","Cross (Repeatable)","Switch (Once)","Switch (Repeatable)","Shoot (Once)","Shoot (Repeatable)","Door (Once)","Door (Repeatable)"};
	label_props[0] = new wxStaticText(this, -1, "Trigger:", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_VERTICAL);
	choice_props[0] = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, 8, triggers);
	choice_props[0]->Bind(wxEVT_CHOICE, &GenLineSpecialPanel::onChoicePropertyChanged, this);

	// Other properties
	for (unsigned a = 1; a < 7; a++)
	{
		label_props[a] = new wxStaticText(this, -1, "");
		label_props[a]->Hide();
		choice_props[a] = new wxChoice(this, -1);
		choice_props[a]->Hide();
		choice_props[a]->Bind(wxEVT_CHOICE, &GenLineSpecialPanel::onChoicePropertyChanged, this);
	}

	// Default to floor type
	choice_type->Select(0);
	choice_props[0]->Select(0);
	setupForType(0);
}

/* GenLineSpecialPanel::~GenLineSpecialPanel
 * GenLineSpecialPanel class destructor
 *******************************************************************/
GenLineSpecialPanel::~GenLineSpecialPanel()
{
}

/* GenLineSpecialPanel::setupForType
 * Sets up generalised properties for special type [type]
 *******************************************************************/
void GenLineSpecialPanel::setupForType(int type)
{
	// Clear properties
	gb_sizer->Clear();
	for (unsigned a = 1; a < 7; a++)
	{
		label_props[a]->Hide();
		choice_props[a]->Hide();
		choice_props[a]->Clear();
	}

	// Trigger
	unsigned n_props = 1;
	gb_sizer->Add(label_props[0], wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(choice_props[0], wxGBPosition(0, 1), wxDefaultSpan, wxEXPAND);
	if (!gb_sizer->IsColGrowable(1))
		gb_sizer->AddGrowableCol(1, 1);

	// Floor
	if (type == BoomGenLineSpecial::GS_FLOOR)
	{
		// Speed
		label_props[1]->SetLabel("Speed:");
		choice_props[1]->AppendString("Slow");
		choice_props[1]->AppendString("Normal");
		choice_props[1]->AppendString("Fast");
		choice_props[1]->AppendString("Turbo");

		// Model/Monsters
		label_props[2]->SetLabel("Monsters Activate:");
		choice_props[2]->AppendString("No");
		choice_props[2]->AppendString("Yes");

		// Direction
		label_props[3]->SetLabel("Direction:");
		choice_props[3]->AppendString("Down");
		choice_props[3]->AppendString("Up");

		// Target
		label_props[4]->SetLabel("Target:");
		choice_props[4]->AppendString("Highest Neighbouring Floor");
		choice_props[4]->AppendString("Lowest Neighbouring Floor");
		choice_props[4]->AppendString("Next Neighbouring Floor");
		choice_props[4]->AppendString("Lowest Neighbouring Ceiling");
		choice_props[4]->AppendString("Ceiling");
		choice_props[4]->AppendString("Move by Shortest Lower Texture");
		choice_props[4]->AppendString("Move 24 Units");
		choice_props[4]->AppendString("Move 32 Units");

		// Change
		label_props[5]->SetLabel("Change:");
		choice_props[5]->AppendString("No Change");
		choice_props[5]->AppendString("Zero Sector Type, Copy Texture");
		choice_props[5]->AppendString("Copy Texture Only");
		choice_props[5]->AppendString("Copy Type and Texture");

		// Crush
		label_props[6]->SetLabel("Crush:");
		choice_props[6]->AppendString("No");
		choice_props[6]->AppendString("Yes");

		n_props = 7;
	}

	// Ceiling
	else if (type == BoomGenLineSpecial::GS_CEILING)
	{
		// Speed
		label_props[1]->SetLabel("Speed:");
		choice_props[1]->AppendString("Slow");
		choice_props[1]->AppendString("Normal");
		choice_props[1]->AppendString("Fast");
		choice_props[1]->AppendString("Turbo");

		// Model/Monsters
		label_props[2]->SetLabel("Monsters Activate:");
		choice_props[2]->AppendString("No");
		choice_props[2]->AppendString("Yes");

		// Direction
		label_props[3]->SetLabel("Direction:");
		choice_props[3]->AppendString("Down");
		choice_props[3]->AppendString("Up");

		// Target
		label_props[4]->SetLabel("Target:");
		choice_props[4]->AppendString("Highest Neighbouring Ceiling");
		choice_props[4]->AppendString("Lowest Neighbouring Ceiling");
		choice_props[4]->AppendString("Next Neighbouring Ceiling");
		choice_props[4]->AppendString("Highest Neighbouring Floor");
		choice_props[4]->AppendString("Floor");
		choice_props[4]->AppendString("Move by Shortest Upper Texture");
		choice_props[4]->AppendString("Move 24 Units");
		choice_props[4]->AppendString("Move 32 Units");

		// Change
		label_props[5]->SetLabel("Change:");
		choice_props[5]->AppendString("No Change");
		choice_props[5]->AppendString("Zero Sector Type, Copy Texture");
		choice_props[5]->AppendString("Copy Texture Only");
		choice_props[5]->AppendString("Copy Type and Texture");

		// Crush
		label_props[6]->SetLabel("Crush:");
		choice_props[6]->AppendString("No");
		choice_props[6]->AppendString("Yes");

		n_props = 7;
	}

	// Door
	else if (type == BoomGenLineSpecial::GS_DOOR)
	{
		// Speed
		label_props[1]->SetLabel("Speed:");
		choice_props[1]->AppendString("Slow");
		choice_props[1]->AppendString("Normal");
		choice_props[1]->AppendString("Fast");
		choice_props[1]->AppendString("Turbo");

		// Kind
		label_props[2]->SetLabel("Kind:");
		choice_props[2]->AppendString("Open, Wait, Close");
		choice_props[2]->AppendString("Open");
		choice_props[2]->AppendString("Close, Wait, Open");
		choice_props[2]->AppendString("Close");

		// Monsters
		label_props[3]->SetLabel("Monsters Activate:");
		choice_props[3]->AppendString("No");
		choice_props[3]->AppendString("Yes");

		// Delay
		label_props[4]->SetLabel("Wait Time:");
		choice_props[4]->AppendString("1 Second");
		choice_props[4]->AppendString("4 Seconds");
		choice_props[4]->AppendString("9 Seconds");
		choice_props[4]->AppendString("30 Seconds");

		n_props = 5;
	}

	// Locked Door
	else if (type == BoomGenLineSpecial::GS_LOCKED_DOOR)
	{
		// Speed
		label_props[1]->SetLabel("Speed:");
		choice_props[1]->AppendString("Slow");
		choice_props[1]->AppendString("Normal");
		choice_props[1]->AppendString("Fast");
		choice_props[1]->AppendString("Turbo");

		// Kind
		label_props[2]->SetLabel("Kind:");
		choice_props[2]->AppendString("Open, Wait, Close");
		choice_props[2]->AppendString("Open");
		choice_props[2]->AppendString("Close, Wait, Open");
		choice_props[2]->AppendString("Close");

		// Lock
		label_props[3]->SetLabel("Lock:");
		choice_props[3]->AppendString("Any Key");
		choice_props[3]->AppendString("Red Card");
		choice_props[3]->AppendString("Blue Card");
		choice_props[3]->AppendString("Yellow Card");
		choice_props[3]->AppendString("Red Skull");
		choice_props[3]->AppendString("Blue Skull");
		choice_props[3]->AppendString("Yellow Skull");
		choice_props[3]->AppendString("All Keys");

		// Key Type
		label_props[4]->SetLabel("Key Type:");
		choice_props[4]->AppendString("Specific (Red Card <> Red Skull)");
		choice_props[4]->AppendString("Colour (Red Card = Red Skull)");

		n_props = 5;
	}

	// Lift
	else if (type == BoomGenLineSpecial::GS_LIFT)
	{
		// Speed
		label_props[1]->SetLabel("Speed:");
		choice_props[1]->AppendString("Slow");
		choice_props[1]->AppendString("Normal");
		choice_props[1]->AppendString("Fast");
		choice_props[1]->AppendString("Turbo");

		// Monsters
		label_props[2]->SetLabel("Monsters Activate:");
		choice_props[2]->AppendString("No");
		choice_props[2]->AppendString("Yes");

		// Delay
		label_props[3]->SetLabel("Wait Time:");
		choice_props[3]->AppendString("1 Second");
		choice_props[3]->AppendString("3 Seconds");
		choice_props[3]->AppendString("5 Seconds");
		choice_props[3]->AppendString("10 Seconds");

		// Target
		label_props[4]->SetLabel("Target:");
		choice_props[4]->AppendString("Lowest Neighbouring Floor");
		choice_props[4]->AppendString("Next Neighbouring Floor");
		choice_props[4]->AppendString("Lowest Neighbouring Ceiling");
		choice_props[4]->AppendString("Perpetual");

		n_props = 5;
	}

	// Stairs
	else if (type == BoomGenLineSpecial::GS_STAIRS)
	{
		// Speed
		label_props[1]->SetLabel("Speed:");
		choice_props[1]->AppendString("Slow");
		choice_props[1]->AppendString("Normal");
		choice_props[1]->AppendString("Fast");
		choice_props[1]->AppendString("Turbo");

		// Monsters
		label_props[2]->SetLabel("Monsters Activate:");
		choice_props[2]->AppendString("No");
		choice_props[2]->AppendString("Yes");

		// Step Height
		label_props[3]->SetLabel("Step Height");
		choice_props[3]->AppendString("4 Units");
		choice_props[3]->AppendString("8 Units");
		choice_props[3]->AppendString("16 Units");
		choice_props[3]->AppendString("24 Units");

		// Direction
		label_props[4]->SetLabel("Direction:");
		choice_props[4]->AppendString("Down");
		choice_props[4]->AppendString("Up");

		// Ignore Texture
		label_props[5]->SetLabel("Ignore Texture:");
		choice_props[5]->AppendString("No: Stop building on diff. texture");
		choice_props[5]->AppendString("Yes");

		n_props = 6;
	}

	// Crusher
	else if (type == BoomGenLineSpecial::GS_CRUSHER)
	{
		// Speed
		label_props[1]->SetLabel("Speed:");
		choice_props[1]->AppendString("Slow");
		choice_props[1]->AppendString("Normal");
		choice_props[1]->AppendString("Fast");
		choice_props[1]->AppendString("Turbo");

		// Monsters
		label_props[2]->SetLabel("Monsters Activate:");
		choice_props[2]->AppendString("No");
		choice_props[2]->AppendString("Yes");

		// Monsters
		label_props[3]->SetLabel("Silent:");
		choice_props[3]->AppendString("No");
		choice_props[3]->AppendString("Yes");

		n_props = 4;
	}

	// Show properties
	for (unsigned a = 1; a < n_props; a++)
	{
		gb_sizer->Add(label_props[a], wxGBPosition(a, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		gb_sizer->Add(choice_props[a], wxGBPosition(a, 1), wxDefaultSpan, wxEXPAND);
		label_props[a]->Show();
		choice_props[a]->Show();
		choice_props[a]->Select(0);
	}

	Layout();
	Update();
}

/* GenLineSpecialPanel::setProp
 * Sets the generalised property at [index] to [value]
 *******************************************************************/
void GenLineSpecialPanel::setProp(int prop, int value)
{
	if (prop < 0 || prop > 6)
		return;

	choice_props[prop]->Select(value);

	// Floor
	if (choice_type->GetSelection() == BoomGenLineSpecial::GS_FLOOR)
	{
		// Change
		if (prop == 5)
		{
			// 0 (No Change), prop 2 is monster
			if (value == 0)
			{
				label_props[2]->SetLabel("Monsters Activate:");
				choice_props[2]->Clear();
				choice_props[2]->AppendString("No");
				choice_props[2]->AppendString("Yes");
				choice_props[2]->Select(0);
			}

			// > 0, prop 2 is model
			else
			{
				label_props[2]->SetLabel("Model Sector:");
				choice_props[2]->Clear();
				choice_props[2]->AppendString("Trigger: Front Side of Trigger Line");
				choice_props[2]->AppendString("Numeric: Sector at Target Height");
				choice_props[2]->Select(0);
			}

			Layout();
		}
	}
}

/* GenLineSpecialPanel::loadSpecial
 * Opens boom generalised line special [special], setting up controls
 * as necessary
 *******************************************************************/
bool GenLineSpecialPanel::loadSpecial(int special)
{
	// Get special info
	int props[7];
	int type = BoomGenLineSpecial::getLineTypeProperties(special, props);

	if (type >= 0)
	{
		// Set special type
		choice_type->Select(type);
		setupForType(type);

		// Set selected properties
		for (unsigned a = 0; a < 7; a++)
		{
			if (choice_props[a]->IsShown())
				setProp(a, props[a]);
		}

		return true;
	}

	// Not a generalised special
	return false;
}

/* GenLineSpecialPanel::getSpecial
 * Returns the currently selected special
 *******************************************************************/
int GenLineSpecialPanel::getSpecial()
{
	int props[7];
	for (unsigned a = 0; a < 7; a++)
		props[a] = choice_props[a]->GetSelection();
	return BoomGenLineSpecial::generateSpecial(choice_type->GetSelection(), props);
}


/*******************************************************************
 * GENLINESPECIALPANEL CLASS EVENTS
 *******************************************************************/

/* GenLineSpecialPanel::onChoiceTypeChanged
 * Called when the special type dropdown is changed
 *******************************************************************/
void GenLineSpecialPanel::onChoiceTypeChanged(wxCommandEvent& e)
{
	setupForType(choice_type->GetSelection());
}

/* GenLineSpecialPanel::onChoicePropertyChanged
 * Called when a property dropdown is changed
 *******************************************************************/
void GenLineSpecialPanel::onChoicePropertyChanged(wxCommandEvent& e)
{
	int type = choice_type->GetSelection();
	wxChoice* choice_changed = (wxChoice*)e.GetEventObject();

	// Floor
	if (type == BoomGenLineSpecial::GS_FLOOR)
	{
		// Change
		if (choice_changed == choice_props[5])
		{
			// 0 (No Change), prop 2 is monster
			if (choice_changed->GetSelection() == 0)
			{
				label_props[2]->SetLabel("Monsters Activate:");
				choice_props[2]->Clear();
				choice_props[2]->AppendString("No");
				choice_props[2]->AppendString("Yes");
				choice_props[2]->Select(0);
			}

			// > 0, prop 2 is model
			else
			{
				label_props[2]->SetLabel("Model Sector:");
				choice_props[2]->Clear();
				choice_props[2]->AppendString("Trigger: Front Side of Trigger Line");
				choice_props[2]->AppendString("Numeric: Sector at Target Height");
				choice_props[2]->Select(0);
			}

			Layout();
		}
	}
}
