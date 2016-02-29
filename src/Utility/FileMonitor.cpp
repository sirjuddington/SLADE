
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    FileMonitor.cpp
 * Description: FileMonitor class, keeps track of a file and checks
 *              it for any modifications every second, also tracks
 *              an external process, and deletes itself when this
 *              process is terminated.
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
#include "UI/WxStuff.h"
#include "FileMonitor.h"
#include "Archive/Archive.h"
#include "Archive/Formats/WadArchive.h"


/*******************************************************************
 * FILEMONITOR CLASS FUNCTIONS
 *******************************************************************/

/* FileMonitor::FileMonitor
 * FileMonitor class constructor
 *******************************************************************/
FileMonitor::FileMonitor(string filename, bool start)
{
	// Init variables
	this->filename = filename;

	// Create process
	process = new wxProcess(this);

	// Start timer (updates every 1 sec)
	if (start)
	{
		file_modified = wxFileModificationTime(filename);
		Start(1000);
	}

	// Bind events
	Bind(wxEVT_END_PROCESS, &FileMonitor::onEndProcess, this);
}

/* FileMonitor::~FileMonitor
 * FileMonitor class destructor
 *******************************************************************/
FileMonitor::~FileMonitor()
{
	delete process;
}

/* FileMonitor::Notify
 * Override of wxTimer::Notify, called each time the timer updates
 *******************************************************************/
void FileMonitor::Notify()
{
	// Check if the file has been modified since last update
	time_t modified = wxFileModificationTime(filename);
	if (modified > file_modified)
	{
		// Modified, update modification time and run any custom code
		file_modified = modified;
		fileModified();
	}
}

/* FileMonitor::onEndProcess
 * Called when the process is terminated
 *******************************************************************/
void FileMonitor::onEndProcess(wxProcessEvent& e)
{
	// Call any custom code for when the external process terminates
	processTerminated();

	// Check if the file has been modified since last update
	time_t modified = wxFileModificationTime(filename);
	if (modified > file_modified)
	{
		// Modified, update modification time and run any custom code
		file_modified = modified;
		fileModified();
	}

	// Delete this FileMonitor (its job is done)
	delete this;
}


/*******************************************************************
 * DB2MAPFILEMONITOR CLASS FUNCTIONS
 *******************************************************************
 * A specialisation of FileMonitor to handle maps open externally
 * in Doom Builder 2. When the file (in this case, a wad file) is
 * modified, its map data is read back in to the parent archive
 */

/* DB2MapFileMonitor::DB2MapFileMonitor
 * DB2MapFileMonitor class constructor
 *******************************************************************/
DB2MapFileMonitor::DB2MapFileMonitor(string filename, Archive* archive, string map_name) : FileMonitor(filename)
{
	// Init variables
	this->archive = archive;
	this->map_name = map_name;
}

/* DB2MapFileMonitor::~DB2MapFileMonitor
 * DB2MapFileMonitor class destructor
 *******************************************************************/
DB2MapFileMonitor::~DB2MapFileMonitor()
{
}

/* DB2MapFileMonitor::fileModified
 * Called when the external wad file has been modified
 *******************************************************************/
void DB2MapFileMonitor::fileModified()
{
	// Check stuff
	if (!archive)
		return;

	// Load file into temp archive
	Archive* wad = new WadArchive();
	wad->open(filename);

	// Get map info for target archive
	vector<Archive::mapdesc_t> maps = archive->detectMaps();
	for (unsigned a = 0; a < maps.size(); a++)
	{
		if (S_CMPNOCASE(maps[a].name, map_name))
		{
			// Check for simple case (map is in zip archive)
			if (maps[a].archive)
			{
				maps[a].head->unlock();
				maps[a].head->importFile(filename);
				maps[a].head->lock();
				break;
			}

			// Delete existing map entries
			ArchiveEntry* entry = maps[a].head;
			bool done = false;
			while (!done)
			{
				ArchiveEntry* next = entry->nextEntry();

				if (entry == maps[a].end)
					done = true;

				entry->unlock();
				archive->removeEntry(entry);
				entry = next;
			}

			// Now re-add map entries from the temp archive
			unsigned index = archive->entryIndex(entry);
			for (unsigned b = 0; b < wad->numEntries(); b++)
			{
				ArchiveEntry* ne = archive->addEntry(wad->getEntry(b), index, NULL, true);
				if (index <= archive->numEntries()) index++;
				ne->lock();
			}
		}
	}

	// Clean up
	delete wad;
}

/* DB2MapFileMonitor::processTerminated
 * Called when the Doom Builder 2 process is terminated
 *******************************************************************/
void DB2MapFileMonitor::processTerminated()
{
	// Get map info for target archive
	vector<Archive::mapdesc_t> maps = archive->detectMaps();
	for (unsigned a = 0; a < maps.size(); a++)
	{
		if (S_CMPNOCASE(maps[a].name, map_name))
		{
			// Unlock map entries
			ArchiveEntry* entry = maps[a].head;
			while (true)
			{
				entry->unlock();
				if (entry == maps[a].end) break;
				entry = entry->nextEntry();
			}
		}
	}

	// Remove the temp wadfile
	wxRemoveFile(filename);
}
