
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    DECOHackPrefsPanel.cpp
// Description: Panel containing DECOHack preference controls
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
#include "DECOHackPrefsPanel.h"
#include "General/UI.h"
#include "UI/Controls/FileLocationPanel.h"
#include "UI/WxUtils.h"
#include "Utility/SFileDialog.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(String, path_decohack)
EXTERN_CVAR(String, path_java)
EXTERN_CVAR(Bool, decohack_always_show_output)


// -----------------------------------------------------------------------------
//
// DECOHackPrefsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// DECOHackPrefsPanel class constructor
// -----------------------------------------------------------------------------
DECOHackPrefsPanel::DECOHackPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create controls
	flp_java_path_ = new FileLocationPanel(
		this,
		path_java,
		true,
		"Browse For Java Executable",
		filedialog::executableExtensionString(),
		filedialog::executableFileName("java"));
	flp_decohack_path_ = new FileLocationPanel(
		this,
		path_decohack,
		true,
		"Browse For DoomTools Jar",
		"Jar Files|*.jar",
		"doomtools.jar");
	cb_always_show_output_ = new wxCheckBox(this, -1, "Always Show Compiler Output");

	setupLayout();
}

// -----------------------------------------------------------------------------
// Initialises panel controls
// -----------------------------------------------------------------------------
void DECOHackPrefsPanel::init()
{
	flp_decohack_path_->setLocation(path_decohack);
	flp_java_path_->setLocation(path_java);
	cb_always_show_output_->SetValue(decohack_always_show_output);
}

// -----------------------------------------------------------------------------
// Applies preferences from the panel controls
// -----------------------------------------------------------------------------
void DECOHackPrefsPanel::applyPreferences()
{
	path_decohack = wxutil::strToView(flp_decohack_path_->location());
	path_java = wxutil::strToView(flp_java_path_->location());

	decohack_always_show_output = cb_always_show_output_->GetValue();
}

// -----------------------------------------------------------------------------
// Lays out the controls on the panel
// -----------------------------------------------------------------------------
void DECOHackPrefsPanel::setupLayout()
{
	// Create sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// java path
	sizer->Add(
		wxutil::createLabelVBox(this, "Location of Java executable:", flp_java_path_), 0, wxEXPAND | wxBOTTOM, ui::pad());

	// doomtools.jar path
	sizer->Add(
		wxutil::createLabelVBox(this, "Location of DoomTools jar:", flp_decohack_path_), 0, wxEXPAND | wxBOTTOM, ui::pad());

	// 'Always Show Output' checkbox
	sizer->Add(cb_always_show_output_, 0, wxEXPAND);
}