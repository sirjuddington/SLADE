
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    FileMonitor.cpp
// Description: FileMonitor class, keeps track of a file and checks it for any
//              modifications every second, also tracks an external process, and
//              deletes itself when this process is terminated.
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
#include "FileMonitor.h"
#include "Archive/Archive.h"
#include "Archive/Formats/WadArchive.h"


// -----------------------------------------------------------------------------
//
// FileMonitor Class Fucntions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// FileMonitor class constructor
// -----------------------------------------------------------------------------
FileMonitor::FileMonitor(const wxString& filename, bool start) : filename_{ filename }
{
	// Create process
	process_ = std::make_unique<wxProcess>(this);

	// Start timer (updates every 1 sec)
	if (start)
	{
		file_modified_ = wxFileModificationTime(filename);
		wxTimer::Start(1000);
	}

	// Bind events
	Bind(wxEVT_END_PROCESS, &FileMonitor::onEndProcess, this);
}

// -----------------------------------------------------------------------------
// Override of wxTimer::Notify, called each time the timer updates
// -----------------------------------------------------------------------------
void FileMonitor::Notify()
{
	// Check if the file has been modified since last update
	auto modified = wxFileModificationTime(filename_);
	if (modified > file_modified_)
	{
		// Modified, update modification time and run any custom code
		file_modified_ = modified;
		fileModified();
	}
}

// -----------------------------------------------------------------------------
// Called when the process is terminated
// -----------------------------------------------------------------------------
void FileMonitor::onEndProcess(wxProcessEvent& e)
{
	// Call any custom code for when the external process terminates
	processTerminated();

	// Check if the file has been modified since last update
	auto modified = wxFileModificationTime(filename_);
	if (modified > file_modified_)
	{
		// Modified, update modification time and run any custom code
		file_modified_ = modified;
		fileModified();
	}

	// Delete this FileMonitor (its job is done)
	delete this;
}


// -----------------------------------------------------------------------------
// DB2MapFileMonitor Class Functions
//
// A specialisation of FileMonitor to handle maps open externally in
// Doom Builder 2.
// When the file (in this case, a wad file) is modified, its map data is read
// back in to the parent archive
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// DB2MapFileMonitor class constructor
// -----------------------------------------------------------------------------
DB2MapFileMonitor::DB2MapFileMonitor(const wxString& filename, Archive* archive, const wxString& map_name) :
	FileMonitor(filename),
	archive_{ archive },
	map_name_{ map_name }
{
}

// -----------------------------------------------------------------------------
// Called when the external wad file has been modified
// -----------------------------------------------------------------------------
void DB2MapFileMonitor::fileModified()
{
	// Check stuff
	if (!archive_)
		return;

	// Load file into temp archive
	Archive::UPtr wad = std::make_unique<WadArchive>();
	wad->open(filename_);

	// Get map info for target archive
	for (auto& map : archive_->detectMaps())
	{
		if (S_CMPNOCASE(map.name, map_name_))
		{
			// Check for simple case (map is in zip archive)
			if (map.archive)
			{
				map.head->unlock();
				map.head->importFile(filename_);
				map.head->lock();
				break;
			}

			// Delete existing map entries
			auto entry = map.head;
			bool done  = false;
			while (!done)
			{
				auto next = entry->nextEntry();

				if (entry == map.end)
					done = true;

				entry->unlock();
				archive_->removeEntry(entry);
				entry = next;
			}

			// Now re-add map entries from the temp archive
			unsigned index = archive_->entryIndex(entry);
			for (unsigned b = 0; b < wad->numEntries(); b++)
			{
				auto ne = archive_->addEntry(wad->entryAt(b), index, nullptr, true);
				if (index <= archive_->numEntries())
					index++;
				ne->lock();
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Called when the Doom Builder 2 process is terminated
// -----------------------------------------------------------------------------
void DB2MapFileMonitor::processTerminated()
{
	// Get map info for target archive
	for (auto& map : archive_->detectMaps())
	{
		if (S_CMPNOCASE(map.name, map_name_))
		{
			// Unlock map entries
			auto entry = map.head;
			while (true)
			{
				entry->unlock();
				if (entry == map.end)
					break;
				entry = entry->nextEntry();
			}
		}
	}

	// Remove the temp wadfile
	wxRemoveFile(filename_);
}
