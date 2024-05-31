
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         https://slade.mancubus.net
// Filename:    SectorSpecialPanel.cpp
// Description: UI for selecting a sector special
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
#include "SectorSpecialPanel.h"
#include "Game/Configuration.h"
#include "UI/Lists/ListView.h"
#include "UI/WxUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
wxString damage_types[]     = { "None", "5%", "10%", "20%" };
wxString alt_damage_types[] = { "Instantly Kill Player w/o Radsuit or Invuln",
								"Instantly Kill Player",
								"Kill All Players, Exit Map (Normal Exit)",
								"Kill All Players, Exit Map (Secret Exit)" };
} // namespace


// -----------------------------------------------------------------------------
//
// SectorSpecialPanel Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// SectorSpecialPanel class constructor
// -----------------------------------------------------------------------------
SectorSpecialPanel::SectorSpecialPanel(wxWindow* parent) : wxPanel(parent, -1)
{
	namespace wx = wxutil;

	// Setup sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Special list
	auto frame      = new wxStaticBox(this, -1, "Special");
	auto framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	lv_specials_    = new ListView(this, -1);
	framesizer->Add(lv_specials_, wx::sfWithBorder(1).Expand());
	sizer->Add(framesizer, wxSizerFlags(1).Expand());

	lv_specials_->enableSizeUpdate(false);
	lv_specials_->AppendColumn("#");
	lv_specials_->AppendColumn("Name");
	auto& types = game::configuration().allSectorTypes();
	for (auto& type : types)
	{
		wxArrayString item;
		item.Add(wxString::Format("%d", type.first));
		item.Add(type.second);
		lv_specials_->addItem(999999, item);
	}
	lv_specials_->enableSizeUpdate(true);
	lv_specials_->updateSize();

	// Boom Flags
	int width = 300;
	if (game::configuration().supportsSectorFlags())
	{
		frame      = new wxStaticBox(this, -1, "Flags");
		framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
		sizer->Add(framesizer, wx::sfWithBorder(0, wxTOP).Expand());

		// Damage
		choice_damage_ = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, 4, damage_types);
		choice_damage_->Select(0);
		framesizer->Add(wx::createLabelHBox(this, "Damage:", choice_damage_), wx::sfWithBorder().Expand());

		// Secret | Friction | Pusher/Puller
		cb_secret_   = new wxCheckBox(this, -1, "Secret");
		cb_friction_ = new wxCheckBox(this, -1, "Friction Enabled");
		cb_pushpull_ = new wxCheckBox(this, -1, "Pushers/Pullers Enabled");
		wx::layoutHorizontally(
			framesizer,
			{ cb_secret_, cb_friction_, cb_pushpull_ },
			wx::sfWithBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Expand());

		// MBF21 Flags: Alternative Damage Mode | Kill Grounded Monsters
		cb_alt_damage_    = new wxCheckBox(this, -1, "Alternate Damage Mode");
		cb_kill_grounded_ = new wxCheckBox(this, -1, "Kill Grounded Monsters");
		if (game::configuration().featureSupported(game::Feature::MBF21))
		{
			wx::layoutHorizontally(
				framesizer,
				{ cb_alt_damage_, cb_kill_grounded_ },
				wx::sfWithBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Expand());

			cb_alt_damage_->Bind(wxEVT_CHECKBOX, [&](wxCommandEvent&) { updateDamageDropdown(); });
		}
		else
		{
			cb_alt_damage_->Hide();
			cb_kill_grounded_->Hide();
		}

		width = -1;
	}

	wxWindowBase::SetMinSize(FromDIP(wxSize(width, 300)));
}

// -----------------------------------------------------------------------------
// Sets up controls on the dialog to show [special]
// -----------------------------------------------------------------------------
void SectorSpecialPanel::setup(int special) const
{
	int base_type = game::configuration().baseSectorType(special);

	// Select base type
	auto& types = game::configuration().allSectorTypes();
	int   index = 0;
	for (auto& i : types)
	{
		if (i.first == base_type)
		{
			lv_specials_->selectItem(index);
			lv_specials_->EnsureVisible(index);
			break;
		}

		index++;
	}

	// Flags
	if (game::configuration().supportsSectorFlags())
	{
		// Damage
		choice_damage_->Select(game::configuration().sectorBoomDamage(special));

		// Secret
		cb_secret_->SetValue(game::configuration().sectorBoomSecret(special));

		// Friction
		cb_friction_->SetValue(game::configuration().sectorBoomFriction(special));

		// Pusher/Puller
		cb_pushpull_->SetValue(game::configuration().sectorBoomPushPull(special));

		// MBF21
		if (game::configuration().featureSupported(game::Feature::MBF21))
		{
			// Alternate Damage Mode
			cb_alt_damage_->SetValue(game::configuration().sectorMBF21AltDamageMode(special));
			updateDamageDropdown();

			// Kill Grounded Monsters
			cb_kill_grounded_->SetValue(game::configuration().sectorMBF21KillGroundedMonsters(special));
		}
	}
}

// -----------------------------------------------------------------------------
// Returns the currently selected sector special
// -----------------------------------------------------------------------------
int SectorSpecialPanel::selectedSpecial() const
{
	auto& types     = game::configuration().allSectorTypes();
	int   selection = 0;
	auto  items     = lv_specials_->selectedItems();
	if (items.GetCount())
		selection = items[0];

	// Get selected base type
	int base = 0;
	if (selection < static_cast<int>(types.size()))
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

	if (game::configuration().supportsSectorFlags())
	{
		return game::configuration().boomSectorType(
			base,
			choice_damage_->GetSelection(),
			cb_secret_->GetValue(),
			cb_friction_->GetValue(),
			cb_pushpull_->GetValue(),
			cb_alt_damage_->GetValue(),
			cb_kill_grounded_->GetValue());
	}
	else
		return base;
}

// -----------------------------------------------------------------------------
// Updates the Damage dropdown items based on the alt damage mode flag
// -----------------------------------------------------------------------------
void SectorSpecialPanel::updateDamageDropdown() const
{
	auto selection = choice_damage_->GetSelection();
	choice_damage_->Set(4, cb_alt_damage_->GetValue() ? alt_damage_types : damage_types);
	choice_damage_->Select(selection);
}
