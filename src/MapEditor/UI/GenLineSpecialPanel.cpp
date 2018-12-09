
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         https://slade.mancubus.net
// Filename:    GenLineSpecialPanel.cpp
// Description: Panel with controls to show/set a Boom generalised line special
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
#include "MapEditor/UI/GenLineSpecialPanel.h"
#include "Game/GenLineSpecial.h"
#include "UI/WxUtils.h"


// -----------------------------------------------------------------------------
//
// GenLineSpecialPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// GenLineSpecialPanel class constructor
// -----------------------------------------------------------------------------
GenLineSpecialPanel::GenLineSpecialPanel(wxWindow* parent) : wxPanel(parent, -1)
{
	// --- Setup layout ---
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Special Type
	choice_type_ = new wxChoice(this, -1);
	choice_type_->Set(WxUtils::arrayString({ "Floor", "Ceiling", "Door", "Locked Door", "Lift", "Stairs", "Crusher" }));
	choice_type_->Bind(wxEVT_CHOICE, &GenLineSpecialPanel::onChoiceTypeChanged, this);
	sizer->Add(WxUtils::createLabelHBox(this, "Type:", choice_type_), 0, wxEXPAND | wxBOTTOM, UI::pad());

	gb_sizer_ = new wxGridBagSizer(UI::pad(), UI::pad());
	sizer->Add(gb_sizer_, 1, wxEXPAND);

	// Trigger
	label_props_[0]  = new wxStaticText(this, -1, "Trigger:", { -1, -1 }, { -1, -1 }, wxALIGN_CENTER_VERTICAL);
	choice_props_[0] = new wxChoice(this, -1);
	choice_props_[0]->Set(WxUtils::arrayString({ "Cross (Once)",
												 "Cross (Repeatable)",
												 "Switch (Once)",
												 "Switch (Repeatable)",
												 "Shoot (Once)",
												 "Shoot (Repeatable)",
												 "Door (Once)",
												 "Door (Repeatable)" }));
	choice_props_[0]->Bind(wxEVT_CHOICE, &GenLineSpecialPanel::onChoicePropertyChanged, this);

	// Other properties
	for (unsigned a = 1; a < 7; a++)
	{
		label_props_[a] = new wxStaticText(this, -1, "");
		label_props_[a]->Hide();
		choice_props_[a] = new wxChoice(this, -1);
		choice_props_[a]->Hide();
		choice_props_[a]->Bind(wxEVT_CHOICE, &GenLineSpecialPanel::onChoicePropertyChanged, this);
	}

	// Default to floor type
	choice_type_->Select(0);
	choice_props_[0]->Select(0);
	setupForType(0);
}

// -----------------------------------------------------------------------------
// Sets up generalised properties for special type [type]
// -----------------------------------------------------------------------------
void GenLineSpecialPanel::setupForType(int type)
{
	// Clear properties
	gb_sizer_->Clear();
	for (unsigned a = 1; a < 7; a++)
	{
		label_props_[a]->Hide();
		choice_props_[a]->Hide();
		choice_props_[a]->Clear();
	}

	// Trigger
	int n_props = 1;
	gb_sizer_->Add(label_props_[0], { 0, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer_->Add(choice_props_[0], { 0, 1 }, { 1, 1 }, wxEXPAND);
	if (!gb_sizer_->IsColGrowable(1))
		gb_sizer_->AddGrowableCol(1, 1);

	// Floor
	if (type == BoomGenLineSpecial::SpecialType::Floor)
	{
		// Speed
		label_props_[1]->SetLabel("Speed:");
		choice_props_[1]->AppendString("Slow");
		choice_props_[1]->AppendString("Normal");
		choice_props_[1]->AppendString("Fast");
		choice_props_[1]->AppendString("Turbo");

		// Model/Monsters
		label_props_[2]->SetLabel("Monsters Activate:");
		choice_props_[2]->AppendString("No");
		choice_props_[2]->AppendString("Yes");

		// Direction
		label_props_[3]->SetLabel("Direction:");
		choice_props_[3]->AppendString("Down");
		choice_props_[3]->AppendString("Up");

		// Target
		label_props_[4]->SetLabel("Target:");
		choice_props_[4]->AppendString("Highest Neighbouring Floor");
		choice_props_[4]->AppendString("Lowest Neighbouring Floor");
		choice_props_[4]->AppendString("Next Neighbouring Floor");
		choice_props_[4]->AppendString("Lowest Neighbouring Ceiling");
		choice_props_[4]->AppendString("Ceiling");
		choice_props_[4]->AppendString("Move by Shortest Lower Texture");
		choice_props_[4]->AppendString("Move 24 Units");
		choice_props_[4]->AppendString("Move 32 Units");

		// Change
		label_props_[5]->SetLabel("Change:");
		choice_props_[5]->AppendString("No Change");
		choice_props_[5]->AppendString("Zero Sector Type, Copy Texture");
		choice_props_[5]->AppendString("Copy Texture Only");
		choice_props_[5]->AppendString("Copy Type and Texture");

		// Crush
		label_props_[6]->SetLabel("Crush:");
		choice_props_[6]->AppendString("No");
		choice_props_[6]->AppendString("Yes");

		n_props = 7;
	}

	// Ceiling
	else if (type == BoomGenLineSpecial::SpecialType::Ceiling)
	{
		// Speed
		label_props_[1]->SetLabel("Speed:");
		choice_props_[1]->AppendString("Slow");
		choice_props_[1]->AppendString("Normal");
		choice_props_[1]->AppendString("Fast");
		choice_props_[1]->AppendString("Turbo");

		// Model/Monsters
		label_props_[2]->SetLabel("Monsters Activate:");
		choice_props_[2]->AppendString("No");
		choice_props_[2]->AppendString("Yes");

		// Direction
		label_props_[3]->SetLabel("Direction:");
		choice_props_[3]->AppendString("Down");
		choice_props_[3]->AppendString("Up");

		// Target
		label_props_[4]->SetLabel("Target:");
		choice_props_[4]->AppendString("Highest Neighbouring Ceiling");
		choice_props_[4]->AppendString("Lowest Neighbouring Ceiling");
		choice_props_[4]->AppendString("Next Neighbouring Ceiling");
		choice_props_[4]->AppendString("Highest Neighbouring Floor");
		choice_props_[4]->AppendString("Floor");
		choice_props_[4]->AppendString("Move by Shortest Upper Texture");
		choice_props_[4]->AppendString("Move 24 Units");
		choice_props_[4]->AppendString("Move 32 Units");

		// Change
		label_props_[5]->SetLabel("Change:");
		choice_props_[5]->AppendString("No Change");
		choice_props_[5]->AppendString("Zero Sector Type, Copy Texture");
		choice_props_[5]->AppendString("Copy Texture Only");
		choice_props_[5]->AppendString("Copy Type and Texture");

		// Crush
		label_props_[6]->SetLabel("Crush:");
		choice_props_[6]->AppendString("No");
		choice_props_[6]->AppendString("Yes");

		n_props = 7;
	}

	// Door
	else if (type == BoomGenLineSpecial::SpecialType::Door)
	{
		// Speed
		label_props_[1]->SetLabel("Speed:");
		choice_props_[1]->AppendString("Slow");
		choice_props_[1]->AppendString("Normal");
		choice_props_[1]->AppendString("Fast");
		choice_props_[1]->AppendString("Turbo");

		// Kind
		label_props_[2]->SetLabel("Kind:");
		choice_props_[2]->AppendString("Open, Wait, Close");
		choice_props_[2]->AppendString("Open");
		choice_props_[2]->AppendString("Close, Wait, Open");
		choice_props_[2]->AppendString("Close");

		// Monsters
		label_props_[3]->SetLabel("Monsters Activate:");
		choice_props_[3]->AppendString("No");
		choice_props_[3]->AppendString("Yes");

		// Delay
		label_props_[4]->SetLabel("Wait Time:");
		choice_props_[4]->AppendString("1 Second");
		choice_props_[4]->AppendString("4 Seconds");
		choice_props_[4]->AppendString("9 Seconds");
		choice_props_[4]->AppendString("30 Seconds");

		n_props = 5;
	}

	// Locked Door
	else if (type == BoomGenLineSpecial::SpecialType::LockedDoor)
	{
		// Speed
		label_props_[1]->SetLabel("Speed:");
		choice_props_[1]->AppendString("Slow");
		choice_props_[1]->AppendString("Normal");
		choice_props_[1]->AppendString("Fast");
		choice_props_[1]->AppendString("Turbo");

		// Kind
		label_props_[2]->SetLabel("Kind:");
		choice_props_[2]->AppendString("Open, Wait, Close");
		choice_props_[2]->AppendString("Open");
		choice_props_[2]->AppendString("Close, Wait, Open");
		choice_props_[2]->AppendString("Close");

		// Lock
		label_props_[3]->SetLabel("Lock:");
		choice_props_[3]->AppendString("Any Key");
		choice_props_[3]->AppendString("Red Card");
		choice_props_[3]->AppendString("Blue Card");
		choice_props_[3]->AppendString("Yellow Card");
		choice_props_[3]->AppendString("Red Skull");
		choice_props_[3]->AppendString("Blue Skull");
		choice_props_[3]->AppendString("Yellow Skull");
		choice_props_[3]->AppendString("All Keys");

		// Key Type
		label_props_[4]->SetLabel("Key Type:");
		choice_props_[4]->AppendString("Specific (Red Card <> Red Skull)");
		choice_props_[4]->AppendString("Colour (Red Card = Red Skull)");

		n_props = 5;
	}

	// Lift
	else if (type == BoomGenLineSpecial::SpecialType::Lift)
	{
		// Speed
		label_props_[1]->SetLabel("Speed:");
		choice_props_[1]->AppendString("Slow");
		choice_props_[1]->AppendString("Normal");
		choice_props_[1]->AppendString("Fast");
		choice_props_[1]->AppendString("Turbo");

		// Monsters
		label_props_[2]->SetLabel("Monsters Activate:");
		choice_props_[2]->AppendString("No");
		choice_props_[2]->AppendString("Yes");

		// Delay
		label_props_[3]->SetLabel("Wait Time:");
		choice_props_[3]->AppendString("1 Second");
		choice_props_[3]->AppendString("3 Seconds");
		choice_props_[3]->AppendString("5 Seconds");
		choice_props_[3]->AppendString("10 Seconds");

		// Target
		label_props_[4]->SetLabel("Target:");
		choice_props_[4]->AppendString("Lowest Neighbouring Floor");
		choice_props_[4]->AppendString("Next Neighbouring Floor");
		choice_props_[4]->AppendString("Lowest Neighbouring Ceiling");
		choice_props_[4]->AppendString("Perpetual");

		n_props = 5;
	}

	// Stairs
	else if (type == BoomGenLineSpecial::SpecialType::Stairs)
	{
		// Speed
		label_props_[1]->SetLabel("Speed:");
		choice_props_[1]->AppendString("Slow");
		choice_props_[1]->AppendString("Normal");
		choice_props_[1]->AppendString("Fast");
		choice_props_[1]->AppendString("Turbo");

		// Monsters
		label_props_[2]->SetLabel("Monsters Activate:");
		choice_props_[2]->AppendString("No");
		choice_props_[2]->AppendString("Yes");

		// Step Height
		label_props_[3]->SetLabel("Step Height");
		choice_props_[3]->AppendString("4 Units");
		choice_props_[3]->AppendString("8 Units");
		choice_props_[3]->AppendString("16 Units");
		choice_props_[3]->AppendString("24 Units");

		// Direction
		label_props_[4]->SetLabel("Direction:");
		choice_props_[4]->AppendString("Down");
		choice_props_[4]->AppendString("Up");

		// Ignore Texture
		label_props_[5]->SetLabel("Ignore Texture:");
		choice_props_[5]->AppendString("No: Stop building on diff. texture");
		choice_props_[5]->AppendString("Yes");

		n_props = 6;
	}

	// Crusher
	else if (type == BoomGenLineSpecial::SpecialType::Crusher)
	{
		// Speed
		label_props_[1]->SetLabel("Speed:");
		choice_props_[1]->AppendString("Slow");
		choice_props_[1]->AppendString("Normal");
		choice_props_[1]->AppendString("Fast");
		choice_props_[1]->AppendString("Turbo");

		// Monsters
		label_props_[2]->SetLabel("Monsters Activate:");
		choice_props_[2]->AppendString("No");
		choice_props_[2]->AppendString("Yes");

		// Monsters
		label_props_[3]->SetLabel("Silent:");
		choice_props_[3]->AppendString("No");
		choice_props_[3]->AppendString("Yes");

		n_props = 4;
	}

	// Show properties
	for (int a = 1; a < n_props; a++)
	{
		gb_sizer_->Add(label_props_[a], { a, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
		gb_sizer_->Add(choice_props_[a], { a, 1 }, { 1, 1 }, wxEXPAND);
		label_props_[a]->Show();
		choice_props_[a]->Show();
		choice_props_[a]->Select(0);
	}

	Layout();
	Update();
}

// -----------------------------------------------------------------------------
// Sets the generalised property at [index] to [value]
// -----------------------------------------------------------------------------
void GenLineSpecialPanel::setProp(int prop, int value)
{
	if (prop < 0 || prop > 6)
		return;

	choice_props_[prop]->Select(value);

	// Floor
	if (choice_type_->GetSelection() == BoomGenLineSpecial::SpecialType::Floor)
	{
		// Change
		if (prop == 5)
		{
			// 0 (No Change), prop 2 is monster
			if (value == 0)
			{
				label_props_[2]->SetLabel("Monsters Activate:");
				choice_props_[2]->Clear();
				choice_props_[2]->AppendString("No");
				choice_props_[2]->AppendString("Yes");
				choice_props_[2]->Select(0);
			}

			// > 0, prop 2 is model
			else
			{
				label_props_[2]->SetLabel("Model Sector:");
				choice_props_[2]->Clear();
				choice_props_[2]->AppendString("Trigger: Front Side of Trigger Line");
				choice_props_[2]->AppendString("Numeric: Sector at Target Height");
				choice_props_[2]->Select(0);
			}

			Layout();
		}
	}
}

// -----------------------------------------------------------------------------
// Opens boom generalised line special [special], setting up controls as
// necessary
// -----------------------------------------------------------------------------
bool GenLineSpecialPanel::loadSpecial(int special)
{
	// Get special info
	int props[7];
	int type = BoomGenLineSpecial::getLineTypeProperties(special, props);

	if (type >= 0)
	{
		// Set special type
		choice_type_->Select(type);
		setupForType(type);

		// Set selected properties
		for (unsigned a = 0; a < 7; a++)
		{
			if (choice_props_[a]->IsShown())
				setProp(a, props[a]);
		}

		return true;
	}

	// Not a generalised special
	return false;
}

// -----------------------------------------------------------------------------
// Returns the currently selected special
// -----------------------------------------------------------------------------
int GenLineSpecialPanel::special()
{
	int props[7];
	for (unsigned a = 0; a < 7; a++)
		props[a] = choice_props_[a]->GetSelection();
	return BoomGenLineSpecial::generateSpecial((BoomGenLineSpecial::SpecialType)choice_type_->GetSelection(), props);
}


// -----------------------------------------------------------------------------
//
// GenLineSpecialPanel Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the special type dropdown is changed
// -----------------------------------------------------------------------------
void GenLineSpecialPanel::onChoiceTypeChanged(wxCommandEvent& e)
{
	setupForType(choice_type_->GetSelection());
}

// -----------------------------------------------------------------------------
// Called when a property dropdown is changed
// -----------------------------------------------------------------------------
void GenLineSpecialPanel::onChoicePropertyChanged(wxCommandEvent& e)
{
	int       type           = choice_type_->GetSelection();
	wxChoice* choice_changed = (wxChoice*)e.GetEventObject();

	// Floor
	if (type == BoomGenLineSpecial::SpecialType::Floor)
	{
		// Change
		if (choice_changed == choice_props_[5])
		{
			// 0 (No Change), prop 2 is monster
			if (choice_changed->GetSelection() == 0)
			{
				label_props_[2]->SetLabel("Monsters Activate:");
				choice_props_[2]->Clear();
				choice_props_[2]->AppendString("No");
				choice_props_[2]->AppendString("Yes");
				choice_props_[2]->Select(0);
			}

			// > 0, prop 2 is model
			else
			{
				label_props_[2]->SetLabel("Model Sector:");
				choice_props_[2]->Clear();
				choice_props_[2]->AppendString("Trigger: Front Side of Trigger Line");
				choice_props_[2]->AppendString("Numeric: Sector at Target Height");
				choice_props_[2]->Select(0);
			}

			Layout();
		}
	}
}
