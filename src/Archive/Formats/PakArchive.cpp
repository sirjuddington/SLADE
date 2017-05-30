
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    PakArchive.cpp
 * Description: PakArchive, archive class to handle the Quake pak format
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
#include "PakArchive.h"
#include "General/UI.h"


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, archive_load_data)


/*******************************************************************
 * PAKARCHIVE CLASS FUNCTIONS
 *******************************************************************/

/* PakArchive::PakArchive
 * PakArchive class constructor
 *******************************************************************/
PakArchive::PakArchive() : Archive(ARCHIVE_PAK)
{
	desc.max_name_length = 56;
	desc.supports_dirs = true;
}

/* PakArchive::~PakArchive
 * PakArchive class destructor
 *******************************************************************/
PakArchive::~PakArchive()
{
}

/* PakArchive::getFileExtensionString
 * Returns the file extension string to use in the file open dialog
 *******************************************************************/
string PakArchive::getFileExtensionString()
{
	return "Pak Files (*.pak)|*.pak";
}

/* PakArchive::getFormat
 * Returns the string id for the pak EntryDataFormat
 *******************************************************************/
string PakArchive::getFormat()
{
	return "archive_pak";
}

/* PakArchive::open
 * Reads pak format data from a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool PakArchive::open(MemChunk& mc)
{
	// Check given data is valid
	if (mc.getSize() < 12)
		return false;

	// Read pak header
	char pack[4];
	int32_t dir_offset;
	int32_t dir_size;
	mc.seek(0, SEEK_SET);
	mc.read(pack, 4);
	mc.read(&dir_offset, 4);
	mc.read(&dir_size, 4);

	// Check it
	if (pack[0] != 'P' || pack[1] != 'A' || pack[2] != 'C' || pack[3] != 'K')
	{
		LOG_MESSAGE(1, "PakArchive::open: Opening failed, invalid header");
		Global::error = "Invalid pak header";
		return false;
	}

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	setMuted(true);

	// Read the directory
	size_t num_entries = dir_size / 64;
	mc.seek(dir_offset, SEEK_SET);
	UI::setSplashProgressMessage("Reading pak archive data");
	for (uint32_t d = 0; d < num_entries; d++)
	{
		// Update splash window progress
		UI::setSplashProgress(((float)d / (float)num_entries));

		// Read entry info
		char name[56];
		int32_t offset;
		int32_t size;
		mc.read(name, 56);
		mc.read(&offset, 4);
		mc.read(&size, 4);

		// Byteswap if needed
		offset = wxINT32_SWAP_ON_BE(offset);
		size = wxINT32_SWAP_ON_BE(size);

		// Check offset+size
		if ((unsigned)(offset + size) > mc.getSize())
		{
			LOG_MESSAGE(1, "PakArchive::open: Pak archive is invalid or corrupt (entry goes past end of file)");
			Global::error = "Archive is invalid and/or corrupt";
			setMuted(false);
			return false;
		}

		// Parse name
		wxFileName fn(wxString::FromAscii(name, 56));

		// Create directory if needed
		ArchiveTreeNode* dir = createDir(fn.GetPath(true, wxPATH_UNIX));

		// Create entry
		ArchiveEntry* entry = new ArchiveEntry(fn.GetFullName(), size);
		entry->exProp("Offset") = (int)offset;
		entry->setLoaded(false);
		entry->setState(0);

		// Add to directory
		dir->addEntry(entry);
	}

	// Detect all entry types
	MemChunk edata;
	vector<ArchiveEntry*> all_entries;
	getEntryTreeAsList(all_entries);
	UI::setSplashProgressMessage("Detecting entry types");
	for (size_t a = 0; a < all_entries.size(); a++)
	{
		// Update splash window progress
		UI::setSplashProgress((((float)a / (float)num_entries)));

		// Get entry
		ArchiveEntry* entry = all_entries[a];

		// Read entry data if it isn't zero-sized
		if (entry->getSize() > 0)
		{
			// Read the entry data
			mc.exportMemChunk(edata, (int)entry->exProp("Offset"), entry->getSize());
			entry->importMemChunk(edata);
		}

		// Detect entry type
		EntryType::detectEntryType(entry);

		// Unload entry data if needed
		if (!archive_load_data)
			entry->unloadData();

		// Set entry to unchanged
		entry->setState(0);
	}

	// Setup variables
	setMuted(false);
	setModified(false);
	announce("opened");

	UI::setSplashProgressMessage("");

	return true;
}

/* PakArchive::write
 * Writes the pak archive to a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool PakArchive::write(MemChunk& mc, bool update)
{
	// Clear current data
	mc.clear();

	// Get archive tree as a list
	vector<ArchiveEntry*> entries;
	getEntryTreeAsList(entries);

	// Process entry list
	int32_t dir_offset = 12;
	int32_t dir_size = 0;
	for (unsigned a = 0; a < entries.size(); a++)
	{
		// Ignore folder entries
		if (entries[a]->getType() == EntryType::folderType())
			continue;

		// Increment directory offset and size
		dir_offset += entries[a]->getSize();
		dir_size += 64;
	}

	// Init data size
	mc.reSize(dir_offset + dir_size, false);

	// Write header
	char pack[4] = { 'P', 'A', 'C', 'K' };
	mc.seek(0, SEEK_SET);
	mc.write(pack, 4);
	mc.write(&dir_offset, 4);
	mc.write(&dir_size, 4);

	// Write directory
	mc.seek(dir_offset, SEEK_SET);
	int32_t offset = 12;
	for (unsigned a = 0; a < entries.size(); a++)
	{
		// Skip folders
		if (entries[a]->getType() == EntryType::folderType())
			continue;

		// Update entry
		if (update)
		{
			entries[a]->setState(0);
			entries[a]->exProp("Offset") = (int)offset;
		}

		// Check entry name
		string name = entries[a]->getPath(true);
		name.Remove(0, 1);	// Remove leading /
		if (name.Len() > 56)
		{
			LOG_MESSAGE(1, "Warning: Entry %s path is too long (> 56 characters), putting it in the root directory", name);
			wxFileName fn(name);
			name = fn.GetFullName();
			if (name.Len() > 56)
				name.Truncate(56);
		}

		// Write entry name
		char name_data[56];
		memset(name_data, 0, 56);
		memcpy(name_data, CHR(name), name.Length());
		mc.write(name_data, 56);

		// Write entry offset
		mc.write(&offset, 4);

		// Write entry size
		int32_t size = entries[a]->getSize();
		mc.write(&size, 4);

		// Increment/update offset
		offset += size;
	}

	// Write entry data
	mc.seek(12, SEEK_SET);
	for (unsigned a = 0; a < entries.size(); a++)
	{
		// Skip folders
		if (entries[a]->getType() == EntryType::folderType())
			continue;

		// Write data
		mc.write(entries[a]->getData(), entries[a]->getSize());
	}

	return true;
}

/* PakArchive::loadEntryData
 * Loads an entry's data from the pak file
 * Returns true if successful, false otherwise
 *******************************************************************/
bool PakArchive::loadEntryData(ArchiveEntry* entry)
{
	// Check entry is ok
	if (!checkEntry(entry))
		return false;

	// Do nothing if the entry's size is zero,
	// or if it has already been loaded
	if (entry->getSize() == 0 || entry->isLoaded())
	{
		entry->setLoaded();
		return true;
	}

	// Open archive file
	wxFile file(filename);

	// Check it opened
	if (!file.IsOpened())
	{
		LOG_MESSAGE(1, "PakArchive::loadEntryData: Unable to open archive file %s", filename);
		return false;
	}

	// Seek to entry offset in file and read it in
	file.Seek((int)entry->exProp("Offset"), wxFromStart);
	entry->importFileStream(file, entry->getSize());

	// Set the lump to loaded
	entry->setLoaded();

	return true;
}

/*******************************************************************
 * PAKARCHIVE CLASS STATIC FUNCTIONS
 *******************************************************************/

/* PakArchive::isPakArchive
 * Checks if the given data is a valid Quake pak archive
 *******************************************************************/
bool PakArchive::isPakArchive(MemChunk& mc)
{
	// Check given data is valid
	if (mc.getSize() < 12)
		return false;

	// Read pak header
	char pack[4];
	int32_t dir_offset;
	int32_t dir_size;
	mc.seek(0, SEEK_SET);
	mc.read(pack, 4);
	mc.read(&dir_offset, 4);
	mc.read(&dir_size, 4);

	// Byteswap values for big endian if needed
	dir_size = wxINT32_SWAP_ON_BE(dir_size);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);

	// Check header
	if (pack[0] != 'P' || pack[1] != 'A' || pack[2] != 'C' || pack[3] != 'K')
		return false;

	// Check directory is sane
	if (dir_offset < 12 || (unsigned)(dir_offset + dir_size) > mc.getSize())
		return false;

	// That'll do
	return true;
}

/* PakArchive::isPakArchive
 * Checks if the file at [filename] is a valid Quake pak archive
 *******************************************************************/
bool PakArchive::isPakArchive(string filename)
{
	// Open file for reading
	wxFile file(filename);

	// Check it opened ok
	if (!file.IsOpened() || file.Length() < 12)
		return false;

	// Read pak header
	char pack[4];
	int32_t dir_offset;
	int32_t dir_size;
	file.Seek(0, wxFromStart);
	file.Read(pack, 4);
	file.Read(&dir_offset, 4);
	file.Read(&dir_size, 4);

	// Byteswap values for big endian if needed
	dir_size = wxINT32_SWAP_ON_BE(dir_size);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);

	// Check header
	if (pack[0] != 'P' || pack[1] != 'A' || pack[2] != 'C' || pack[3] != 'K')
		return false;

	// Check directory is sane
	if (dir_offset < 12 || dir_offset + dir_size > file.Length())
		return false;

	// That'll do
	return true;
}
