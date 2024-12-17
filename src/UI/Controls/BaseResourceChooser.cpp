
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         https://slade.mancubus.net
// Filename:    BaseResourceChooser.cpp
// Description: BaseResourceChooser class. A wxChoice that contains a list of
//              base resource archives.Updates itself when the paths list is
//              modified, and loads the selected archive if one is selected
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
#include "BaseResourceChooser.h"
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "UI/WxUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Int, base_resource)


// -----------------------------------------------------------------------------
//
// BaseResourceChooser Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// BaseResourceChooser class constructor
// -----------------------------------------------------------------------------
BaseResourceChooser::BaseResourceChooser(wxWindow* parent, bool load_change) :
	wxChoice{ parent, -1 },
	load_change_{ load_change }
{
	// Populate
	populateChoices();

	// Bind events
	Bind(
		wxEVT_CHOICE,
		[&](wxCommandEvent&)
		{
			// Open the selected base resource
			if (load_change_)
				app::archiveManager().openBaseResource(GetSelection() - 1);
		});

	// Handle base resource change signals from the Archive Manager
	auto& am_signals = app::archiveManager().signals();
	signal_connections_ += am_signals.base_res_path_added.connect([this](unsigned) { populateChoices(); });
	signal_connections_ += am_signals.base_res_path_removed.connect([this](unsigned) { populateChoices(); });
	signal_connections_ += am_signals.base_res_current_cleared.connect([this]() { SetSelection(0); });
	signal_connections_ += am_signals.base_res_current_changed.connect([this](unsigned)
																	   { SetSelection(base_resource + 1); });

	if (app::platform() != app::Platform::Linux)
		wxWindowBase::SetMinSize(wxutil::scaledSize(128, -1));
}

// -----------------------------------------------------------------------------
// Clears and repopulates the choice list with base resource paths from the
// ArchiveManager
// -----------------------------------------------------------------------------
void BaseResourceChooser::populateChoices()
{
	// Clear current items
	Clear();

	// Add <none> option
	AppendString("<none>");

	// Populate with base resource paths
	for (unsigned a = 0; a < app::archiveManager().numBaseResourcePaths(); a++)
	{
		wxFileName fn(app::archiveManager().getBaseResourcePath(a));
		AppendString(fn.GetFullName());
	}

	// Select current base resource
	SetSelection(base_resource + 1);
}
