
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MapBackupManager.cpp
 * Description: MapBackupManager class - creates/manages map backups
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
#include "MapBackupManager.h"
#include "Archive/Formats/ZipArchive.h"
#include "General/Misc.h"
#include "MapEditor.h"
#include "UI/MapBackupPanel.h"
#include "UI/SDialog.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Int, max_map_backups, 25, CVAR_SAVE)

// List of entry names to be ignored for backups
string mb_ignore_entries[] =
{
	"NODES",
	"SSECTORS",
	"ZNODES",
	"SEGS",
	"REJECT",
	"BLOCKMAP",
	"GL_VERT",
	"GL_SEGS",
	"GL_SSECT",
	"GL_NODES"
};


/*******************************************************************
 * MAPBACKUPMANAGER CLASS FUNCTIONS
 *******************************************************************/

/* MapBackupManager::MapBackupManager
 * MapBackupManager class constructor
 *******************************************************************/
MapBackupManager::MapBackupManager()
{
}

/* MapBackupManager::~MapBackupManager
 * MapBackupManager class destructor
 *******************************************************************/
MapBackupManager::~MapBackupManager()
{
}

/* MapBackupManager::writeBackup
 * Writes a backup for [map_name] in [archive_name], with the map
 * data entries in [map_data]
 *******************************************************************/
bool MapBackupManager::writeBackup(vector<ArchiveEntry*>& map_data, string archive_name, string map_name)
{
	// Create backup directory if needed
	string backup_dir = App::path("backups", App::Dir::User);
	if (!wxDirExists(backup_dir)) wxMkdir(backup_dir);

	// Open or create backup zip
	ZipArchive backup;
	archive_name.Replace(".", "_");
	string backup_file = backup_dir + "/" + archive_name + "_backup.zip";
	if (!backup.open(backup_file))
		backup.setFilename(backup_file);

	// Filter ignored entries
	vector<ArchiveEntry*> backup_entries;
	for (unsigned a = 0; a < map_data.size(); a++)
	{
		// Check for ignored entry
		bool ignored = false;
		for (unsigned b = 0; b < 10; b++)
		{
			if (S_CMPNOCASE(mb_ignore_entries[b], map_data[a]->getName()))
			{
				ignored = true;
				break;
			}
		}

		if (!ignored)
			backup_entries.push_back(map_data[a]);
	}

	// Compare with last backup (if any)
	ArchiveTreeNode* map_dir = backup.getDir(map_name);
	if (map_dir && map_dir->nChildren() > 0)
	{
		ArchiveTreeNode* last_backup = (ArchiveTreeNode*)map_dir->getChild(map_dir->nChildren() - 1);
		bool same = true;
		if (last_backup->numEntries() != backup_entries.size())
			same = false;
		else
		{
			for (unsigned a = 0; a < last_backup->numEntries(); a++)
			{
				ArchiveEntry* e1 = backup_entries[a];
				ArchiveEntry* e2 = last_backup->getEntry(a);
				if (e1->getSize() != e2->getSize())
				{
					same = false;
					break;
				}

				uint32_t crc1 = Misc::crc(e1->getData(), e1->getSize());
				uint32_t crc2 = Misc::crc(e2->getData(), e2->getSize());
				if (crc1 != crc2)
				{
					same = false;
					break;
				}
			}
		}

		if (same)
		{
			LOG_MESSAGE(2, "Same data as previous backup - ignoring");
			return true;
		}
	}

	// Add map data to backup
	string timestamp = wxDateTime::Now().FormatISOCombined('_');
	timestamp.Replace(":", "");
	string dir = map_name + "/" + timestamp;
	for (unsigned a = 0; a < backup_entries.size(); a++)
		backup.addEntry(backup_entries[a], dir, true);

	// Check for max backups & remove old ones if over
	map_dir = backup.getDir(map_name);
	while ((int)map_dir->nChildren() > max_map_backups)
		backup.removeDir(map_dir->getChild(0)->getName(), map_dir);

	// Save backup file
	Archive::save_backup = false;
	bool ok = backup.save();
	Archive::save_backup = true;

	return ok;
}

/* MapBackupManager::openBackp
 * Shows the map backups for [map_name] in [archive_name], returns
 * the selected map backup data in a WadArchive
 *******************************************************************/
Archive* MapBackupManager::openBackup(string archive_name, string map_name)
{
	SDialog dlg(MapEditor::windowWx(), S_FMT("Restore %s backup", CHR(map_name)), "map_backup", 500, 400);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	dlg.SetSizer(sizer);
	auto panel_backup = new MapBackupPanel(&dlg);
	sizer->Add(panel_backup, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 10);
	sizer->AddSpacer(4);
	sizer->Add(dlg.CreateButtonSizer(wxOK|wxCANCEL), 0, wxEXPAND|wxLEFT|wxRIGHT, 6);
	sizer->AddSpacer(10);

	if (panel_backup->loadBackups(archive_name, map_name))
	{
		if (dlg.ShowModal() == wxID_OK)
			return panel_backup->getSelectedMapData();
	}
	else
		wxMessageBox(
			S_FMT("No backups exist for %s of %s", CHR(map_name), CHR(archive_name)),
			"Restore Backup",
			wxICON_INFORMATION,
			MapEditor::windowWx()
		);

	return nullptr;
}
