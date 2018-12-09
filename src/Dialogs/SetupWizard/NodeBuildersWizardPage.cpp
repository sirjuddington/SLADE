
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    NodeBuildersWizardPage.cpp
// Description: Setup wizard page to set up node builders
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
#include "NodeBuildersWizardPage.h"
#include "Dialogs/Preferences/NodesPrefsPanel.h"


// -----------------------------------------------------------------------------
//
// NodeBuildersWizardPage Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// NodeBuildersWizardPage class constructor
// -----------------------------------------------------------------------------
NodeBuildersWizardPage::NodeBuildersWizardPage(wxWindow* parent) : WizardPageBase(parent)
{
	// Setup sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Add Base Resource Archive panel
	panel_nodes_ = new NodesPrefsPanel(this, false);
	sizer->Add(panel_nodes_, 1, wxEXPAND);
}

// -----------------------------------------------------------------------------
// Returns the description for the wizard page
// -----------------------------------------------------------------------------
string NodeBuildersWizardPage::description()
{
	return "If you plan to do any map editing, set up the paths to any node builders you have. "
		   "You can also set up the build options for each node builder. "
		   "Note that ZDoom does not require nodes to be built, so if you are creating ZDoom-only maps, you can select "
		   "\"Don't Build Nodes\" and continue.";
}
