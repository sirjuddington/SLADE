
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
#include "MapEditor/UI/SectorSpecialPanel.h"
#include "UI/Layout.h"
#include "UI/Lists/ListView.h"

using namespace slade;


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
	auto lh = ui::LayoutHelper(this);

	// Setup sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Special panel
	panel_special_ = new SectorSpecialPanel(this);
	sizer->Add(panel_special_, lh.sfWithLargeBorder(1, wxLEFT | wxRIGHT | wxTOP).Expand());

	// Dialog buttons
	sizer->AddSpacer(lh.pad());
	sizer->Add(CreateButtonSizer(wxOK | wxCANCEL), lh.sfWithLargeBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Expand());

	// Bind Events
	panel_special_->getSpecialsList()->Bind(wxEVT_LIST_ITEM_ACTIVATED, [&](wxListEvent& e) { EndModal(wxID_OK); });

	wxWindowBase::SetMinClientSize(sizer->GetMinSize());
	CenterOnParent();
}

// -----------------------------------------------------------------------------
// Sets up the dialog to show [special]
// -----------------------------------------------------------------------------
void SectorSpecialDialog::setup(int special) const
{
	panel_special_->setup(special);
}

// -----------------------------------------------------------------------------
// Returns the currently selected special
// -----------------------------------------------------------------------------
int SectorSpecialDialog::getSelectedSpecial() const
{
	return panel_special_->selectedSpecial();
}
