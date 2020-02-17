
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    BaseResourceWizardPage.cpp
// Description: Setup wizard page to set up the base resource archive
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
#include "BaseResourceWizardPage.h"
#include "Dialogs/Preferences/BaseResourceArchivesPanel.h"


// -----------------------------------------------------------------------------
//
// BaseResourceWizardPage Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// BaseResourceWizardPage class constructor
// -----------------------------------------------------------------------------
BaseResourceWizardPage::BaseResourceWizardPage(wxWindow* parent) : WizardPageBase(parent)
{
	// Setup sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Add Base Resource Archive panel
	bra_panel_ = new BaseResourceArchivesPanel(this);
	bra_panel_->init();
	bra_panel_->autodetect();
	sizer->Add(bra_panel_, 1, wxEXPAND);
}

// -----------------------------------------------------------------------------
// Applies any changes set on the wizard page
// -----------------------------------------------------------------------------
void BaseResourceWizardPage::applyChanges()
{
	bra_panel_->applyPreferences();
}

// -----------------------------------------------------------------------------
// Returns the description for the wizard page
// -----------------------------------------------------------------------------
wxString BaseResourceWizardPage::description()
{
	return "Add 'Base Resource' archives to the list. "
		   "These can be selected from the dropdown in the toolbar, and will be used as a base (eg. IWAD) for editing. "
		   "Usually these will be game IWADs: doom2.wad, heretic.wad, etc. "
		   "If no base resource archive is selected, certain features will not work correctly.";
}
