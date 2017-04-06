
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    DiskArchive.cpp
 * Description: DiskArchive, archive class to handle Nerve files
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
 * Note: specifications and snippets of code were taken from the
 * Eternity Engine, by James Haley (a.k.a. Quasar).
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "DiskArchive.h"
#include "General/UI.h"

struct diskentry_t
{
	char name[64];
	size_t offset;
	size_t length;
};

/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, archive_load_data)


/*******************************************************************
 * DISKARCHIVE CLASS FUNCTIONS
 *******************************************************************/

/* DiskArchive::DiskArchive
 * DiskArchive class constructor
 *******************************************************************/
DiskArchive::DiskArchive() : Archive(ARCHIVE_DISK)
{
	desc.max_name_length = 64;
	desc.names_extensions = true;
	desc.supports_dirs = true;
}

/* DiskArchive::~DiskArchive
 * DiskArchive class destructor
 *******************************************************************/
DiskArchive::~DiskArchive()
{
}

/* DiskArchive::getFileExtensionString
 * Returns the file extension string to use in the file open dialog
 *******************************************************************/
string DiskArchive::getFileExtensionString()
{
	return "Nerve Disk Files (*.disk)|*.disk";
}

/* DiskArchive::getFormat
 * Returns the string id for the disk EntryDataFormat
 *******************************************************************/
string DiskArchive::getFormat()
{
	return "archive_disk";
}

/* DiskArchive::open
 * Reads disk format data from a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool DiskArchive::open(MemChunk& mc)
{
	size_t mcsize = mc.getSize();

	// Check given data is valid
	if (mcsize < 80)
		return false;

	// Read disk header
	uint32_t num_entries;
	mc.seek(0, SEEK_SET);
	mc.read(&num_entries, 4);
	num_entries = wxUINT32_SWAP_ON_LE(num_entries);

	size_t start_offset = (72 * num_entries) + 8;

	if (mcsize < start_offset)
		return false;

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	setMuted(true);

	// Read the directory
	UI::setSplashProgressMessage("Reading disk archive data");
	for (uint32_t d = 0; d < num_entries; d++)
	{
		// Update splash window progress
		UI::setSplashProgress(((float)d / (float)num_entries));

		// Read entry info
		diskentry_t dent;
		mc.read(&dent, 72);

		// Byteswap if needed
		dent.length = wxUINT32_SWAP_ON_LE(dent.length);
		dent.offset = wxUINT32_SWAP_ON_LE(dent.offset);

		// Increase offset to make it relative to start of archive
		dent.offset += start_offset;

		// Check offset+size
		if (dent.offset + dent.length > mcsize)
		{
			LOG_MESSAGE(1, "DiskArchive::open: Disk archive is invalid or corrupt (entry goes past end of file)");
			Global::error = "Archive is invalid and/or corrupt";
			setMuted(false);
			return false;
		}

		// Parse name
		string name = wxString::FromAscii(dent.name, 64);
		name.Replace("\\", "/");
		name.Replace("GAME:/", "");
		wxFileName fn(name);

		// Create directory if needed
		ArchiveTreeNode* dir = createDir(fn.GetPath(true, wxPATH_UNIX));

		// Create entry
		ArchiveEntry* entry = new ArchiveEntry(fn.GetFullName(), dent.length);
		entry->exProp("Offset") = (int)dent.offset;
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

/* DiskArchive::write
 * Writes the disk archive to a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool DiskArchive::write(MemChunk& mc, bool update)
{
	// Clear current data
	mc.clear();

	// Get archive tree as a list
	vector<ArchiveEntry*> entries;
	getEntryTreeAsList(entries);

	// Process entry list
	uint32_t num_entries = 0;
	uint32_t size_entries = 0;
	for (unsigned a = 0; a < entries.size(); a++)
	{
		// Ignore folder entries
		if (entries[a]->getType() == EntryType::folderType())
			continue;

		// Increment directory offset and size
		size_entries += entries[a]->getSize();
		++num_entries;
	}

	// Init data size
	uint32_t start_offset = 8 + (num_entries * 72);
	uint32_t offset = start_offset;
	mc.reSize(size_entries + start_offset, false);

	// Write header
	num_entries = wxUINT32_SWAP_ON_LE(num_entries);
	mc.seek(0, SEEK_SET);
	mc.write(&num_entries, 4);

	// Write directory
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
		name.Replace("/", "\\");
		// The leading "GAME:\" part of the name means there is only 58 usable characters for path
		if (name.Len() > 58)
		{
			LOG_MESSAGE(1, "Warning: Entry %s path is too long (> 58 characters), putting it in the root directory", name);
			wxFileName fn(name);
			name = fn.GetFullName();
			if (name.Len() > 57)
				name.Truncate(57);
			// Add leading "\"
			name = "\\" + name;

		}
		name = "GAME:" + name;

		diskentry_t dent;

		// Write entry name
		// The names field are padded with FD for doom.disk, FE for doom2.disk. No idea whether
		// a non-null padding is actually required, though. It probably should work with anything.
		memset(dent.name, 0xFE, 64);
		memcpy(dent.name, CHR(name), name.Length());
		dent.name[name.Length()] = 0;

		// Write entry offset
		dent.offset = wxUINT32_SWAP_ON_LE(offset - start_offset);

		// Write entry size
		dent.length = wxUINT32_SWAP_ON_LE(entries[a]->getSize());

		// Actually write stuff
		mc.write(&dent, 72);

		// Increment/update offset
		offset += wxUINT32_SWAP_ON_LE(dent.length);
	}

	// Finish writing header
	size_entries = wxUINT32_SWAP_ON_LE(size_entries);
	mc.write(&size_entries, 4);

	// Write entry data
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

/* DiskArchive::loadEntryData
 * Loads an entry's data from the disk file
 * Returns true if successful, false otherwise
 *******************************************************************/
bool DiskArchive::loadEntryData(ArchiveEntry* entry)
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
		LOG_MESSAGE(1, "DiskArchive::loadEntryData: Unable to open archive file %s", filename);
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
 * DISKARCHIVE CLASS STATIC FUNCTIONS
 *******************************************************************/

/* DiskArchive::isDiskArchive
 * Checks if the given data is a valid Nerve disk archive
 *******************************************************************/
bool DiskArchive::isDiskArchive(MemChunk& mc)
{
	// Check given data is valid
	size_t mcsize = mc.getSize();
	if (mcsize < 80)
		return false;

	// Read disk header
	uint32_t num_entries;
	uint32_t size_entries;
	mc.seek(0, SEEK_SET);
	mc.read(&num_entries, 4);
	num_entries = wxUINT32_SWAP_ON_LE(num_entries);

	size_t start_offset = (72 * num_entries) + 8;

	if (mcsize < start_offset)
		return false;

	// Read the directory
	for (uint32_t d = 0; d < num_entries; d++)
	{
		// Read entry info
		diskentry_t entry;
		mc.read(&entry, 72);

		// Byteswap if needed
		entry.length = wxUINT32_SWAP_ON_LE(entry.length);
		entry.offset = wxUINT32_SWAP_ON_LE(entry.offset);

		// Increase offset to make it relative to start of archive
		entry.offset += start_offset;

		// Check offset+size
		if (entry.offset + entry.length > mcsize)
			return false;
	}
	mc.read(&size_entries, 4);
	size_entries = wxUINT32_SWAP_ON_LE(size_entries);
	if (size_entries + start_offset != mcsize)
		return false;

	// That'll do
	return true;
}

/* DiskArchive::isDiskArchive
 * Checks if the file at [filename] is a valid Quake disk archive
 *******************************************************************/
bool DiskArchive::isDiskArchive(string filename)
{
	// Open file for reading
	wxFile file(filename);

	// Check it opened ok
	if (!file.IsOpened() || file.Length() < 4)
		return false;
	// Check given data is valid
	size_t mcsize = file.Length();
	if (mcsize < 80)
		return false;

	// Read disk header
	uint32_t num_entries;
	uint32_t size_entries;
	file.Seek(0, wxFromStart);
	file.Read(&num_entries, 4);
	num_entries = wxUINT32_SWAP_ON_LE(num_entries);

	size_t start_offset = (72 * num_entries) + 8;

	if (mcsize < start_offset)
		return false;

	// Read the directory
	for (uint32_t d = 0; d < num_entries; d++)
	{
		// Read entry info
		diskentry_t entry;
		file.Read(&entry, 72);

		// Byteswap if needed
		entry.length = wxUINT32_SWAP_ON_LE(entry.length);
		entry.offset = wxUINT32_SWAP_ON_LE(entry.offset);

		// Increase offset to make it relative to start of archive
		entry.offset += start_offset;

		// Check offset+size
		if (entry.offset + entry.length > mcsize)
			return false;
	}
	file.Read(&size_entries, 4);
	size_entries = wxUINT32_SWAP_ON_LE(size_entries);
	if (size_entries + start_offset != mcsize)
		return false;

	// That'll do
	return true;
}
