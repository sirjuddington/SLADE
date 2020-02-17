
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
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
	framesizer->Add(lv_specials_, 1, wxEXPAND | wxALL, UI::pad());
	sizer->Add(framesizer, 1, wxEXPAND);

	lv_specials_->enableSizeUpdate(false);
	lv_specials_->AppendColumn("#");
	lv_specials_->AppendColumn("Name");
	auto& types = Game::configuration().allSectorTypes();
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
	int width = UI::scalePx(300);
	if (Game::configuration().supportsSectorFlags())
	{
		frame      = new wxStaticBox(this, -1, "Flags");
		framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
		sizer->Add(framesizer, 0, wxEXPAND | wxTOP, UI::pad());

		// Damage
		wxString damage_types[] = { "None", "5%", "10%", "20%" };
		choice_damage_          = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, 4, damage_types);
		choice_damage_->Select(0);
		framesizer->Add(WxUtils::createLabelHBox(this, "Damage:", choice_damage_), 0, wxEXPAND | wxALL, UI::pad());

		// Secret | Friction | Pusher/Puller
		cb_secret_   = new wxCheckBox(this, -1, "Secret");
		cb_friction_ = new wxCheckBox(this, -1, "Friction Enabled");
		cb_pushpull_ = new wxCheckBox(this, -1, "Pushers/Pullers Enabled");
		WxUtils::layoutHorizontally(
			framesizer,
			{ cb_secret_, cb_friction_, cb_pushpull_ },
			wxSizerFlags(0).Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM, UI::pad()));

		width = -1;
	}

	wxWindowBase::SetMinSize(WxUtils::scaledSize(width, 300));
}

// -----------------------------------------------------------------------------
// Sets up controls on the dialog to show [special]
// -----------------------------------------------------------------------------
void SectorSpecialPanel::setup(int special) const
{
	int base_type = Game::configuration().baseSectorType(special);

	// Select base type
	auto& types = Game::configuration().allSectorTypes();
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
	if (Game::configuration().supportsSectorFlags())
	{
		// Damage
		choice_damage_->Select(Game::configuration().sectorBoomDamage(special));

		// Secret
		cb_secret_->SetValue(Game::configuration().sectorBoomSecret(special));

		// Friction
		cb_friction_->SetValue(Game::configuration().sectorBoomFriction(special));

		// Pusher/Puller
		cb_pushpull_->SetValue(Game::configuration().sectorBoomPushPull(special));
	}
}

// -----------------------------------------------------------------------------
// Returns the currently selected sector special
// -----------------------------------------------------------------------------
int SectorSpecialPanel::selectedSpecial() const
{
	auto& types     = Game::configuration().allSectorTypes();
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

	if (Game::configuration().supportsSectorFlags())
	{
		return Game::configuration().boomSectorType(
			base,
			choice_damage_->GetSelection(),
			cb_secret_->GetValue(),
			cb_friction_->GetValue(),
			cb_pushpull_->GetValue());
	}
	else
		return base;
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
	sizer->Add(panel_special_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, UI::padLarge());

	// Dialog buttons
	sizer->AddSpacer(UI::pad());
	sizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, UI::padLarge());

	// Bind Events
	panel_special_->getSpecialsList()->Bind(wxEVT_LIST_ITEM_ACTIVATED, [&](wxListEvent& e) { EndModal(wxID_OK); });

	wxWindowBase::SetMinClientSize(sizer->GetMinSize());
	CenterOnParent();
}
