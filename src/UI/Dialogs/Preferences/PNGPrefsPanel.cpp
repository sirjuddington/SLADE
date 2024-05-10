
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    PNGPrefsPanel.cpp
// Description: Panel containing PNG tools preference controls
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
#include "PNGPrefsPanel.h"
#include "UI/Controls/FileLocationPanel.h"
#include "UI/WxUtils.h"
#include "Utility/SFileDialog.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(String, path_pngout)
EXTERN_CVAR(String, path_pngcrush)
EXTERN_CVAR(String, path_deflopt)


// -----------------------------------------------------------------------------
//
// PNGPrefsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// PNGPrefsPanel class constructor
// -----------------------------------------------------------------------------
PNGPrefsPanel::PNGPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	wxutil::layoutVertically(
		sizer,
		vector<wxObject*>{ wxutil::createLabelVBox(
							   this,
							   "Location of PNGout:",
							   flp_pngout_ = new FileLocationPanel(
								   this,
								   path_pngout,
								   true,
								   "Browse for PNGout Executable",
								   filedialog::executableExtensionString(),
								   filedialog::executableFileName("pngout"))),
						   wxutil::createLabelVBox(
							   this,
							   "Location of PNGCrush:",
							   flp_pngcrush_ = new FileLocationPanel(
								   this,
								   path_pngcrush,
								   true,
								   "Browse for PNGCrush Executable",
								   filedialog::executableExtensionString(),
								   filedialog::executableFileName("pngcrush"))),
						   wxutil::createLabelVBox(
							   this,
							   "Location of DeflOpt:",
							   flp_deflopt_ = new FileLocationPanel(
								   this,
								   path_deflopt,
								   true,
								   "Browse for DeflOpt Executable",
								   filedialog::executableExtensionString(),
								   filedialog::executableFileName("deflopt"))) },
		wxSizerFlags(0).Expand());
}

// -----------------------------------------------------------------------------
// Initialises panel controls
// -----------------------------------------------------------------------------
void PNGPrefsPanel::init()
{
	flp_pngout_->setLocation(path_pngout);
	flp_pngcrush_->setLocation(path_pngcrush);
	flp_deflopt_->setLocation(path_deflopt);
}

// -----------------------------------------------------------------------------
// Applies preferences from the panel controls
// -----------------------------------------------------------------------------
void PNGPrefsPanel::applyPreferences()
{
	path_pngout   = wxutil::strToView(flp_pngout_->location());
	path_pngcrush = wxutil::strToView(flp_pngcrush_->location());
	path_deflopt  = wxutil::strToView(flp_deflopt_->location());
}
