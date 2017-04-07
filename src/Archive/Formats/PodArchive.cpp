
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    PodArchive.cpp
 * Description: PodArchive, archive class to handle the Terminal
 *              Velocity / Fury3 POD archive format
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
#include "PodArchive.h"
#include "General/UI.h"
#include "MainEditor/MainEditor.h"
#include "General/Console/Console.h"


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, archive_load_data)
EXTERN_CVAR(Bool, wad_force_uppercase)


/*******************************************************************
 * PODARCHIVE CLASS FUNCTIONS
 *******************************************************************/

/* PodArchive::PodArchive
 * PodArchive class constructor
 *******************************************************************/
PodArchive::PodArchive() : Archive(ARCHIVE_POD)
{
	// Blank id
	memset(id, 0, 80);
}

/* PodArchive::~PodArchive
 * PodArchive class destructor
 *******************************************************************/
PodArchive::~PodArchive()
{
}

/* PodArchive::setId
 * Sets the description/id of the pod archive
 *******************************************************************/
void PodArchive::setId(string id)
{
	memset(this->id, 0, 80);
	memcpy(this->id, CHR(id), id.Length());
}

/* PodArchive::getFileExtensionString
 * Gets the wxWidgets file dialog filter string for the archive type
 *******************************************************************/
string PodArchive::getFileExtensionString()
{
	return "POD Files (*.pod)|*.pod";
}

/* PodArchive::getFormat
 * Returns the EntryDataFormat id of this archive type
 *******************************************************************/
string PodArchive::getFormat()
{
	return "archive_pod";
}

/* PodArchive::open
 * Reads pod format data from a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool PodArchive::open(MemChunk& mc)
{
	// Check data was given
	if (!mc.hasData())
		return false;

	// Read no. of files
	mc.seek(0, 0);
	uint32_t num_files;
	mc.read(&num_files, 4);

	// Read id
	mc.read(id, 80);

	// Read directory
	file_entry_t* files = new file_entry_t[num_files];
	mc.read(files, num_files * sizeof(file_entry_t));

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	setMuted(true);

	// Create entries
	UI::setSplashProgressMessage("Reading pod archive data");
	for (unsigned a = 0; a < num_files; a++)
	{
		// Get the entry name as a wxFileName (so we can break it up)
		wxFileName fn(files[a].name);

		// Create entry
		ArchiveEntry* new_entry = new ArchiveEntry(fn.GetFullName(), files[a].size);
		new_entry->exProp("Offset") = files[a].offset;
		new_entry->setLoaded(false);

		// Add entry and directory to directory tree
		string path = fn.GetPath(false);
		ArchiveTreeNode* ndir = createDir(path);
		ndir->addEntry(new_entry);

		new_entry->setState(0);

		LOG_MESSAGE(5, "File size: %d, offset: %d, name: %s", files[a].size, files[a].offset, files[a].name);
	}

	// Detect entry types
	vector<ArchiveEntry*> all_entries;
	getEntryTreeAsList(all_entries);
	UI::setSplashProgressMessage("Detecting entry types");
	for (unsigned a = 0; a < all_entries.size(); a++)
	{
		// Skip dir/marker
		if (all_entries[a]->getSize() == 0 || all_entries[a]->getType() == EntryType::folderType())
		{
			all_entries[a]->setState(0);
			continue;
		}

		// Update splash window progress
		UI::setSplashProgress((((float)a / (float)all_entries.size())));

		// Read data
		MemChunk edata;
		mc.exportMemChunk(edata, all_entries[a]->exProp("Offset").getIntValue(), all_entries[a]->getSize());
		all_entries[a]->importMemChunk(edata);

		// Detect entry type
		EntryType::detectEntryType(all_entries[a]);

		// Unload entry data if needed
		if (!archive_load_data)
			all_entries[a]->unloadData();

		// Set entry to unchanged
		all_entries[a]->setState(0);
		LOG_MESSAGE(5, "entry %s size %d", CHR(all_entries[a]->getName()), all_entries[a]->getSize());
	}

	// Clean up
	delete[] files;

	// Setup variables
	setMuted(false);
	setModified(false);
	announce("opened");

	UI::setSplashProgressMessage("");

	return true;
}

/* PodArchive::write
 * Writes the pod archive to a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool PodArchive::write(MemChunk& mc, bool update)
{
	// Get all entries
	vector<ArchiveEntry*> entries;
	getEntryTreeAsList(entries);

	// Process entries
	int ndirs = 0;
	uint32_t data_size = 0;
	for (unsigned a = 0; a < entries.size(); a++)
	{
		if (entries[a]->getType() == EntryType::folderType())
			ndirs++;
		else
			data_size += entries[a]->getSize();
	}

	// Init MemChunk
	mc.clear();
	mc.reSize(4 + 80 + (entries.size() * 40) + data_size, false);
	LOG_MESSAGE(5, "MC size %d", mc.getSize());

	// Write no. entries
	uint32_t n_entries = entries.size() - ndirs;
	LOG_MESSAGE(5, "n_entries %d", n_entries);
	mc.write(&n_entries, 4);

	// Write id
	LOG_MESSAGE(5, "id %s", id);
	mc.write(id, 80);

	// Write directory
	file_entry_t fe;
	fe.offset = 4 + 80 + (n_entries * 40);
	for (unsigned a = 0; a < entries.size(); a++)
	{
		if (entries[a]->getType() == EntryType::folderType())
			continue;

		// Name
		memset(fe.name, 0, 32);
		string path = entries[a]->getPath(true);
		path.Replace("/", "\\");
		path = path.AfterFirst('\\');
		//LOG_MESSAGE(2, path);
		memcpy(fe.name, CHR(path), path.Len());

		// Size
		fe.size = entries[a]->getSize();

		// Write directory entry
		mc.write(fe.name, 32);
		mc.write(&fe.size, 4);
		mc.write(&fe.offset, 4);
		LOG_MESSAGE(5, "entry %s: old=%d new=%d size=%d", fe.name, entries[a]->exProp("Offset").getIntValue(), fe.offset, entries[a]->getSize());

		// Next offset
		fe.offset += fe.size;
	}

	// Write entry data
	for (unsigned a = 0; a < entries.size(); a++)
		if (entries[a]->getType() != EntryType::folderType())
			mc.write(entries[a]->getData(), entries[a]->getSize());

	return true;
}

/* PodArchive::loadEntryData
 * Loads an entry's data from the archive file
 * Returns true if successful, false otherwise
 *******************************************************************/
bool PodArchive::loadEntryData(ArchiveEntry* entry)
{
	// Check the entry is valid and part of this archive
	if (!checkEntry(entry))
		return false;

	// Do nothing if the lump's size is zero,
	// or if it has already been loaded
	if (entry->getSize() == 0 || entry->isLoaded())
	{
		entry->setLoaded();
		return true;
	}

	// Open file
	wxFile file(filename);

	// Check if opening the file failed
	if (!file.IsOpened())
	{
		LOG_MESSAGE(1, "PodArchive::loadEntryData: Failed to open file %s", filename);
		return false;
	}

	// Seek to lump offset in file and read it in
	file.Seek((int)entry->exProp("Offset"), wxFromStart);
	entry->importFileStream(file, entry->getSize());

	// Set the lump to loaded
	entry->setLoaded();

	return true;
}

/* WadArchive::detectMaps
 * Searches for any maps in the archive and adds them to the map list
 *******************************************************************/
vector<Archive::mapdesc_t> PodArchive::detectMaps()
{
	vector<mapdesc_t> list;
	return list;
}


/*******************************************************************
 * PODARCHIVE CLASS STATIC FUNCTIONS
 *******************************************************************/

/* PodArchive::isPodArchive
 * Checks if the given data is a valid pod archive
 *******************************************************************/
bool PodArchive::isPodArchive(MemChunk& mc)
{
	// Check size for header
	if (mc.getSize() < 84)
		return false;

	// Read no. of files
	mc.seek(0, 0);
	uint32_t num_files;
	mc.read(&num_files, 4);

	// Read id
	char id[80];
	mc.read(id, 80);

	// Check size for directory
	if (mc.getSize() < 84 + (num_files * 40))
		return false;

	// Read directory
	file_entry_t* files = new file_entry_t[num_files];
	mc.read(files, num_files * sizeof(file_entry_t));

	// Check offsets
	for (unsigned a = 0; a < num_files; a++)
		if (files[a].offset + files[a].size > mc.getSize())
			return false;

	return true;
}

/* PodArchive::isPodArchive
 * Checks if the file at [filename] is a valid pod archive
 *******************************************************************/
bool PodArchive::isPodArchive(string filename)
{
	wxFile file;
	if (!file.Open(filename))
		return false;

	file.SeekEnd(0);
	uint32_t file_size = file.Tell();

	// Check size for header
	if (file_size < 84)
	{
		file.Close();
		return false;
	}

	// Read no. of files
	file.Seek(0);
	uint32_t num_files;
	file.Read(&num_files, 4);

	// Read id
	char id[80];
	file.Read(id, 80);

	// Check size for directory
	if (file_size < 84 + (num_files * 40))
	{
		file.Close();
		return false;
	}

	// Read directory
	file_entry_t* files = new file_entry_t[num_files];
	file.Read(files, num_files * sizeof(file_entry_t));

	// Check offsets
	for (unsigned a = 0; a < num_files; a++)
		if (files[a].offset + files[a].size > file_size)
		{
			file.Close();
			return false;
		}

	return true;
}


/*******************************************************************
 * CONSOLE COMMANDS
 *******************************************************************/

CONSOLE_COMMAND(pod_get_id, 0, 1)
{
	Archive* archive = MainEditor::currentArchive();
	if (archive && archive->getType() == ARCHIVE_POD)
		Log::console(((PodArchive*)archive)->getId());
	else
		Log::console("Current tab is not a POD archive");

}

CONSOLE_COMMAND(pod_set_id, 1, true)
{
	Archive* archive = MainEditor::currentArchive();
	if (archive && archive->getType() == ARCHIVE_POD)
		((PodArchive*)archive)->setId(args[0].Truncate(80));
	else
		Log::console("Current tab is not a POD archive");
}
