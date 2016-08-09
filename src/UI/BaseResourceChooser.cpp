
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    BaseResourceChooser.cpp
 * Description: BaseResourceChooser class. A wxChoice that contains a
 *              list of base resource archives. Updates itself when
 *              the paths list is modified, and loads the selected
 *              archive if one is selected
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
#include "BaseResourceChooser.h"
#include "Archive/ArchiveManager.h"


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Int, base_resource)


/*******************************************************************
 * BASERESOURCECHOOSER CLASS FUNCTIONS
 *******************************************************************/

/* BaseResourceChooser::BaseResourceChooser
 * BaseResourceChooser class constructor
 *******************************************************************/
BaseResourceChooser::BaseResourceChooser(wxWindow* parent, bool load_change)
	: wxChoice(parent, -1, wxDefaultPosition, wxSize(128, -1))
{
	// Init Variables
	this->load_change = load_change;

	// Populate
	populateChoices();

	// Listen to the archive manager
	listenTo(theArchiveManager);

	// Bind events
	Bind(wxEVT_CHOICE, &BaseResourceChooser::onChoiceChanged, this);
}

/* BaseResourceChooser::~BaseResourceChooser
 * BaseResourceChooser class destructor
 *******************************************************************/
BaseResourceChooser::~BaseResourceChooser()
{
}

/* BaseResourceChooser::populateChoices
 * Clears and repopulates the choice list with base resource paths
 * from the ArchiveManager
 *******************************************************************/
void BaseResourceChooser::populateChoices()
{
	// Clear current items
	Clear();

	// Add <none> option
	AppendString("<none>");

	// Populate with base resource paths
	for (unsigned a = 0; a < theArchiveManager->numBaseResourcePaths(); a++)
	{
		wxFileName fn(theArchiveManager->getBaseResourcePath(a));
		AppendString(fn.GetFullName());
	}

	// Select current base resource
	SetSelection(base_resource + 1);
}

/* BaseResourceChooser::onAnnouncement
 * Called when an announcement is received from the ArchiveManager
 *******************************************************************/
void BaseResourceChooser::onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data)
{
	// Check the announcer
	if (announcer != theArchiveManager)
		return;

	// Base resource archive changed
	if (event_name == "base_resource_changed")
		SetSelection(base_resource + 1);

	// Base resource path list changed
	if (event_name == "base_resource_path_added" || event_name == "base_resource_path_removed")
		populateChoices();
}


/*******************************************************************
 * BASERESOURCECHOOSER EVENTS
 *******************************************************************/

/* BaseResourceChooser::onChoiceChanged
 * Called when the selection is changed
 *******************************************************************/
void BaseResourceChooser::onChoiceChanged(wxCommandEvent& e)
{
	// Open the selected base resource
	if (load_change)
		theArchiveManager->openBaseResource(GetSelection() - 1);
}
