
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         https://slade.mancubus.net
// Filename:    ActionSpecialDialog.cpp
// Description: A dialog that allows selection of an action special
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
#include "ActionSpecialDialog.h"
#include "Game/ActionSpecial.h"
#include "Game/Configuration.h"
#include "General/Defs.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/UI/ActionSpecialPanel.h"
#include "MapEditor/UI/ArgsPanel.h"
#include "UI/Layout.h"
#include "UI/WxUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// ActionSpecialDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ActionSpecialDialog class constructor
// -----------------------------------------------------------------------------
ActionSpecialDialog::ActionSpecialDialog(wxWindow* parent, bool show_args) :
	SDialog{ parent, "Select Action Special", "actionspecial", 400, 500 }
{
	panel_args_ = nullptr;
	auto lh     = ui::LayoutHelper(this);
	auto sizer  = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// No args
	if (mapeditor::editContext().mapDesc().format == MapFormat::Doom || !show_args)
	{
		panel_special_ = new ActionSpecialPanel(this, false);
		sizer->Add(panel_special_, lh.sfWithLargeBorder(1, wxLEFT | wxRIGHT | wxTOP).Expand());
	}

	// Args (use tabs)
	else
	{
		stc_tabs_ = STabCtrl::createControl(this);
		sizer->Add(stc_tabs_, lh.sfWithLargeBorder(1, wxLEFT | wxRIGHT | wxTOP).Expand());

		// Special panel
		panel_special_ = new ActionSpecialPanel(this);
		stc_tabs_->AddPage(wxutil::createPadPanel(stc_tabs_, panel_special_), "Special");

		// Args panel
		panel_args_ = new ArgsPanel(this);
		stc_tabs_->AddPage(wxutil::createPadPanel(stc_tabs_, panel_args_), "Args");
		panel_special_->setArgsPanel(panel_args_);
	}

	// Add buttons
	sizer->AddSpacer(lh.pad());
	sizer->Add(CreateButtonSizer(wxOK | wxCANCEL), lh.sfWithLargeBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Expand());

	// Init
	SetSizerAndFit(sizer);
	CenterOnParent();
}

// -----------------------------------------------------------------------------
// Selects the item for special [special] in the specials tree
// -----------------------------------------------------------------------------
void ActionSpecialDialog::setSpecial(int special) const
{
	panel_special_->setSpecial(special);
}

// -----------------------------------------------------------------------------
// Sets the arg values
// -----------------------------------------------------------------------------
void ActionSpecialDialog::setArgs(int args[5]) const
{
	if (panel_args_)
		panel_args_->setValues(args);
}

// -----------------------------------------------------------------------------
// Returns the currently selected action special
// -----------------------------------------------------------------------------
int ActionSpecialDialog::selectedSpecial() const
{
	return panel_special_->selectedSpecial();
}

// -----------------------------------------------------------------------------
// Returns the value of arg [index]
// -----------------------------------------------------------------------------
int ActionSpecialDialog::argValue(int index) const
{
	if (panel_args_)
		return panel_args_->argValue(index);
	else
		return 0;
}

// -----------------------------------------------------------------------------
// Applies selected trigger(s) (hexen or udmf) to [lines]
// -----------------------------------------------------------------------------
void ActionSpecialDialog::applyTo(const vector<MapObject*>& lines, bool apply_special) const
{
	panel_special_->applyTo(lines, apply_special);
}

// -----------------------------------------------------------------------------
// Loads special/trigger/arg values from [lines]
// -----------------------------------------------------------------------------
void ActionSpecialDialog::openLines(const vector<MapObject*>& lines) const
{
	panel_special_->openLines(lines);
}
