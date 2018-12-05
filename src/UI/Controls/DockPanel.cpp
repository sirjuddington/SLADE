
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    DockPanel.cpp
// Description: DockPanel class, a wxPanel that can change layout depending on
//              whether it's floating, docked horizontally or docked vertically
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
#include "DockPanel.h"


// -----------------------------------------------------------------------------
//
// DockPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// DockPanel class constructor
// -----------------------------------------------------------------------------
DockPanel::DockPanel(wxWindow* parent) : wxPanel(parent, -1)
{
	// Init variables
	current_layout_ = Orient::Uninitialised;

	// Size event
	Bind(wxEVT_SIZE, [&](wxSizeEvent& e) {
		// Get parent's AUI manager (if it exists)
		auto mgr = wxAuiManager::GetManager(GetParent());
		if (!mgr)
		{
			e.Skip();
			return;
		}

		// Check if floating
		if (mgr->GetPane(this).IsFloating())
		{
			if (current_layout_ != Orient::Normal)
				layoutNormal();
			current_layout_ = Orient::Normal;
		}
		else
		{
			// Not floating, layout horizontally or vertically depending on size
			if (GetSize().x >= GetSize().y)
			{
				if (current_layout_ != Orient::Horizontal)
					layoutHorizontal();
				current_layout_ = Orient::Horizontal;
			}
			else
			{
				if (current_layout_ != Orient::Vertical)
					layoutVertical();
				current_layout_ = Orient::Vertical;
			}
		}

		e.Skip();
	});
}
