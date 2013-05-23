
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2012 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    DockPanel.cpp
 * Description: DockPanel class, a wxPanel that can change layout
 *              depending on whether it's floating, docked
 *              horizontally or docked vertically
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "WxStuff.h"
#include "DockPanel.h"
#include <wx/aui/aui.h>


/*******************************************************************
 * DOCKPANEL CLASS FUNCTIONS
 *******************************************************************/

/* DockPanel::DockPanel
 * DockPanel class constructor
 *******************************************************************/
DockPanel::DockPanel(wxWindow* parent) : wxPanel(parent, -1)
{
	// Init variables
	current_layout = 0;

	// Bind events
	Bind(wxEVT_SIZE, &DockPanel::onSize, this);
}

/* DockPanel::~DockPanel
 * DockPanel class destructor
 *******************************************************************/
DockPanel::~DockPanel()
{
}

/* DockPanel::onSize
 * Called when the panel is resized
 *******************************************************************/
void DockPanel::onSize(wxSizeEvent& e)
{
	// Get parent's AUI manager (if it exists)
	wxAuiManager* mgr = wxAuiManager::GetManager(GetParent());
	if (!mgr)
	{
		e.Skip();
		return;
	}

	// Check if floating
	if (mgr->GetPane(this).IsFloating())
	{
		if (current_layout != 0) layoutNormal();
		current_layout = 0;
	}
	else
	{
		// Not floating, layout horizontally or vertically depending on size
		if (GetSize().x >= GetSize().y)
		{
			if (current_layout != 1) layoutHorizontal();
			current_layout = 1;
		}
		else
		{
			if (current_layout != 2) layoutVertical();
			current_layout = 2;
		}
	}

	e.Skip();
}
