
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         https://slade.mancubus.net
// Filename:    SectorSpecialDialog.cpp
// Description: A dialog that allows selection of a sector special
//              (and other related classes)
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
#include "SectorSpecialDialog.h"
#include "Game/Configuration.h"
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
	// Setup sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Special list
	auto frame      = new wxStaticBox(this, -1, "Special");
	auto framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	lv_specials_    = new ListView(this, -1);
	framesizer->Add(lv_specials_, 1, wxEXPAND | wxALL, ui::pad());
	sizer->Add(framesizer, 1, wxEXPAND);

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
	int width = ui::scalePx(300);
	if (game::configuration().supportsSectorFlags())
	{
		frame      = new wxStaticBox(this, -1, "Flags");
		framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
		sizer->Add(framesizer, 0, wxEXPAND | wxTOP, ui::pad());

		// Damage
		choice_damage_ = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, 4, damage_types);
		choice_damage_->Select(0);
		framesizer->Add(wxutil::createLabelHBox(this, "Damage:", choice_damage_), 0, wxEXPAND | wxALL, ui::pad());

		// Secret | Friction | Pusher/Puller
		cb_secret_   = new wxCheckBox(this, -1, "Secret");
		cb_friction_ = new wxCheckBox(this, -1, "Friction Enabled");
		cb_pushpull_ = new wxCheckBox(this, -1, "Pushers/Pullers Enabled");
		wxutil::layoutHorizontally(
			framesizer,
			{ cb_secret_, cb_friction_, cb_pushpull_ },
			wxSizerFlags(0).Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM, ui::pad()));

		// MBF21 Flags: Alternative Damage Mode | Kill Grounded Monsters
		if (game::configuration().featureSupported(game::Feature::MBF21))
		{
			cb_alt_damage_    = new wxCheckBox(this, -1, "Alternate Damage Mode");
			cb_kill_grounded_ = new wxCheckBox(this, -1, "Kill Grounded Monsters");
			wxutil::layoutHorizontally(
				framesizer,
				{ cb_alt_damage_, cb_kill_grounded_ },
				wxSizerFlags(0).Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM, ui::pad()));

			cb_alt_damage_->Bind(wxEVT_CHECKBOX, [&](wxCommandEvent&) { updateDamageDropdown(); });
		}

		width = -1;
	}

	wxWindowBase::SetMinSize(wxutil::scaledSize(width, 300));
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


// -----------------------------------------------------------------------------
//
// SectorSpecialDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SectorSpecialDialog class constructor
// -----------------------------------------------------------------------------
SectorSpecialDialog::SectorSpecialDialog(wxWindow* parent) : SDialog(parent, "Select Sector Special", "sectorspecial")
{
	// Setup sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Special panel
	panel_special_ = new SectorSpecialPanel(this);
	sizer->Add(panel_special_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, ui::padLarge());

	// Dialog buttons
	sizer->AddSpacer(ui::pad());
	sizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, ui::padLarge());

	// Bind Events
	panel_special_->getSpecialsList()->Bind(wxEVT_LIST_ITEM_ACTIVATED, [&](wxListEvent& e) { EndModal(wxID_OK); });

	wxWindowBase::SetMinClientSize(sizer->GetMinSize());
	CenterOnParent();
}
