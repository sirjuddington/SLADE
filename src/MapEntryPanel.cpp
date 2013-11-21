
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2012 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MapEntryPanel.cpp
 * Description: MapEntryPanel class. Shows a basic (lines-only)
 *              preview of a map entry
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
#include "MapEntryPanel.h"
#include "Archive.h"
#include "MapLine.h"
#include "MapVertex.h"
#include "Parser.h"
#include "SImage.h"
#include "SIFormat.h"
#include <wx/filename.h>


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Int, map_image_width,  800, CVAR_SAVE)
CVAR(Int, map_image_height, 600, CVAR_SAVE)


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(String, dir_last)


/*******************************************************************
 * MAPENTRYPANEL CLASS FUNCTIONS
 *******************************************************************/

/* MapEntryPanel::MapEntryPanel
 * MapEntryPanel class constructor
 *******************************************************************/
MapEntryPanel::MapEntryPanel(wxWindow* parent) : EntryPanel(parent, "map")
{
	// Setup map canvas
	map_canvas = new MapPreviewCanvas(this);
	sizer_main->Add(map_canvas->toPanel(this), 1, wxEXPAND, 0);

	// Setup map toolbar buttons
	SToolBarGroup* group = new SToolBarGroup(toolbar, "Map");
	group->addActionButton("save_image", "Save Map Image", "t_export", "Save map overview to an image", true);
	group->addActionButton("pmap_open_text", "", true);
	toolbar->addGroup(group);

	// Remove save/revert buttons
	toolbar->deleteGroup("Entry");

	// Layout
	Layout();
}

/* MapEntryPanel::~MapEntryPanel
 * MapEntryPanel class destructor
 *******************************************************************/
MapEntryPanel::~MapEntryPanel()
{
}

/* MapEntryPanel::loadEntry
 * Loads [entry] into the EntryPanel. Returns false if the map was
 * invalid, true otherwise
 *******************************************************************/
bool MapEntryPanel::loadEntry(ArchiveEntry* entry)
{
	// Clear current map data
	map_canvas->clearMap();

	// Find map definition for entry
	vector<Archive::mapdesc_t> maps = entry->getParent()->detectMaps();
	Archive::mapdesc_t thismap;
	bool found = false;
	for (unsigned a = 0; a < maps.size(); a++)
	{
		if (maps[a].head == entry)
		{
			thismap = maps[a];
			found = true;
			break;
		}
	}

	// All errors = invalid map
	Global::error = "Invalid map";

	// There is no map entry for the map marker.
	// This may happen if a map marker lump is copy/pasted without the rest of the map lumps.
	if (!found)
	{
		entry->setType(EntryType::unknownType());
		EntryType::detectEntryType(entry);
		return false;
	}

	// Load map into preview canvas
	return map_canvas->openMap(thismap);
}

/* MapEntryPanel::saveEntry
 * Saves any changes to the entry (does nothing in map viewer)
 *******************************************************************/
bool MapEntryPanel::saveEntry()
{
	return true;
}

/* MapEntryPanel::createImage
 * Creates a PNG file of the map preview
 * TODO: Preference panel for background and line colors,
 * as well as for image size
 *******************************************************************/
bool MapEntryPanel::createImage()
{
	if (entry == NULL)
		return false;

	ArchiveEntry temp;
	// Stupid OpenGL grumble grumble grumble
	if (GLEW_ARB_framebuffer_object)
		map_canvas->createImage(temp, map_image_width, map_image_height);
	else
		map_canvas->createImage(temp, min<int>(map_image_width, map_canvas->GetSize().x),
		                        min<int>(map_image_height, map_canvas->GetSize().y));
	string name = S_FMT("%s_%s", CHR(entry->getParent()->getFilename(false)), CHR(entry->getName()));
	wxFileName fn(name);

	// Create save file dialog
	wxFileDialog dialog_save(this, S_FMT("Save Map Preview \"%s\"", name.c_str()),
	                         dir_last, fn.GetFullName(), "PNG (*.PNG)|*.png",
	                         wxFD_SAVE | wxFD_OVERWRITE_PROMPT, wxDefaultPosition);

	// Run the dialog & check that the user didn't cancel
	if (dialog_save.ShowModal() == wxID_OK)
	{
		// If a filename was selected, export it
		bool ret = temp.exportFile(dialog_save.GetPath());

		// Save 'dir_last'
		dir_last = dialog_save.GetDirectory();

		return ret;
	}
	return true;
}

/* MapEntryPanel::toolbarButtonClick
 * Called when a (EntryPanel) toolbar button is clicked
 *******************************************************************/
void MapEntryPanel::toolbarButtonClick(string action_id)
{
	// Save Map Image
	if (action_id == "save_image")
	{
		createImage();
	}
}
