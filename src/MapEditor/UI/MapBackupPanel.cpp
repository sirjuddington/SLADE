
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MapBackupPanel.cpp
 * Description: User interface for selecting a map backup to restore
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
#include "App.h"
#include "MapBackupPanel.h"
#include "Archive/Formats/WadArchive.h"
#include "Archive/Formats/ZipArchive.h"
#include "UI/Canvas/MapPreviewCanvas.h"


/*******************************************************************
 * MAPBACKUPPANEL CLASS FUNCTIONS
 *******************************************************************/

/* MapBackupPanel::MapBackupPanel
 * MapBackupPanel class constructor
 *******************************************************************/
MapBackupPanel::MapBackupPanel(wxWindow* parent) : wxPanel(parent, -1)
{
	// Init variables
	archive_backups = new ZipArchive();
	dir_current = NULL;
	archive_mapdata = NULL;

	// Setup Sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(sizer);

	// Backups list
	sizer->Add(list_backups = new ListView(this, -1), 0, wxEXPAND|wxALL, 4);

	// Map preview
	sizer->Add(canvas_map = new MapPreviewCanvas(this), 1, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 4);

	// Bind events
	list_backups->Bind(wxEVT_LIST_ITEM_SELECTED, &MapBackupPanel::onBackupListChanged, this);

	Layout();
}

/* MapBackupPanel::~MapBackupPanel
 * MapBackupPanel class destructor
 *******************************************************************/
MapBackupPanel::~MapBackupPanel()
{
	delete archive_backups;
}

/* MapBackupPanel::loadBackups
 * Opens the map backup file for [map_name] in [archive_name] and
 * populates the list
 *******************************************************************/
bool MapBackupPanel::loadBackups(string archive_name, string map_name)
{
	// Open backup file
	archive_name.Replace(".", "_");
	string backup_file = App::path("backups", App::Dir::User) + "/" + archive_name + "_backup.zip";
	if (!archive_backups->open(backup_file))
		return false;

	// Get backup dir for map
	dir_current = archive_backups->getDir(map_name);
	if (dir_current == archive_backups->getRoot() || !dir_current)
		return false;

	// Populate backups list
	list_backups->ClearAll();
	list_backups->AppendColumn("Backup Date");
	list_backups->AppendColumn("Time");

	int index = 0;
	for (int a = dir_current->nChildren() - 1; a >= 0; a--)
	{
		string timestamp = dir_current->getChild(a)->getName();
		wxArrayString cols;

		// Date
		cols.Add(timestamp.Before('_'));

		// Time
		string time = timestamp.After('_');
		cols.Add(time.Left(2) + ":" + time.Mid(2, 2) + ":" + time.Right(2));

		// Add to list
		list_backups->addItem(index++, cols);
	}

	if (list_backups->GetItemCount() > 0)
		list_backups->selectItem(0);

	return true;
}

/* MapBackupPanel::updateMapPreview
 * Updates the map preview with the currently selected backup
 *******************************************************************/
void MapBackupPanel::updateMapPreview()
{
	// Clear current preview
	canvas_map->clearMap();

	// Check for selection
	if (list_backups->selectedItems().IsEmpty())
		return;
	int selection = (list_backups->GetItemCount()-1) - list_backups->selectedItems()[0];

	// Load map data to temporary wad
	if (archive_mapdata)
		delete archive_mapdata;
	archive_mapdata = new WadArchive();
	ArchiveTreeNode* dir = (ArchiveTreeNode*)dir_current->getChild(selection);
	for (unsigned a = 0; a < dir->numEntries(); a++)
		archive_mapdata->addEntry(dir->getEntry(a), "", true);

	// Open map preview
	vector<Archive::mapdesc_t> maps = archive_mapdata->detectMaps();
	if (!maps.empty())
		canvas_map->openMap(maps[0]);
}


/*******************************************************************
 * MAPBACKUPPANEL CLASS EVENTS
 *******************************************************************/

/* MapBackupPanel::onBackupListChanged
 * Called when the selection changes on the backup list
 *******************************************************************/
void MapBackupPanel::onBackupListChanged(wxListEvent& e)
{
	updateMapPreview();
}
