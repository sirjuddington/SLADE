
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ADatArchive.cpp
 * Description: ADatArchive, archive class to handle the Anachronox
 *              dat format
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
#include "ADatArchive.h"
#include "General/Misc.h"
#include "General/UI.h"
#include "Utility/Compression.h"


/*******************************************************************
 * CONSTANTS
 *******************************************************************/
#define DIRENTRY 144

/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, archive_load_data)


/*******************************************************************
 * ADATARCHIVE CLASS FUNCTIONS
 *******************************************************************/

/* ADatArchive::ADatArchive
 * ADatArchive class constructor
 *******************************************************************/
ADatArchive::ADatArchive() : Archive(ARCHIVE_ADAT)
{
	desc.supports_dirs = true;
	desc.max_name_length = 128;
}

/* ADatArchive::~ADatArchive
 * ADatArchive class destructor
 *******************************************************************/
ADatArchive::~ADatArchive()
{
}

/* ADatArchive::getFileExtensionString
 * Returns the file extension string to use in the file open dialog
 *******************************************************************/
string ADatArchive::getFileExtensionString()
{
	return "Dat Files (*.dat)|*.dat";
}

/* ADatArchive::getFormat
 * Returns the string id for the dat EntryDataFormat
 *******************************************************************/
string ADatArchive::getFormat()
{
	return "archive_adat";
}

/* ADatArchive::open
 * Reads dat format data from a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool ADatArchive::open(MemChunk& mc)
{
	// Check given data is valid
	if (mc.getSize() < 16)
		return false;

	// Read dat header
	char magic[4];
	long dir_offset;
	long dir_size;
	mc.seek(0, SEEK_SET);
	mc.read(magic, 4);
	mc.read(&dir_offset, 4);
	mc.read(&dir_size, 4);

	// Check it
	if (magic[0] != 'A' || magic[1] != 'D' || magic[2] != 'A' || magic[3] != 'T')
	{
		LOG_MESSAGE(1, "ADatArchive::open: Opening failed, invalid header");
		Global::error = "Invalid dat header";
		return false;
	}

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	setMuted(true);

	// Read the directory
	size_t num_entries = dir_size / DIRENTRY;
	mc.seek(dir_offset, SEEK_SET);
	UI::setSplashProgressMessage("Reading dat archive data");
	for (uint32_t d = 0; d < num_entries; d++)
	{
		// Update splash window progress
		UI::setSplashProgress(((float)d / (float)num_entries));

		// Read entry info
		char name[128];
		long offset;
		long decsize;
		long compsize;
		long whatever;			// No idea what this could be
		mc.read(name, 128);
		mc.read(&offset, 4);
		mc.read(&decsize, 4);
		mc.read(&compsize, 4);
		mc.read(&whatever, 4);

		// Byteswap if needed
		offset = wxINT32_SWAP_ON_BE(offset);
		decsize = wxINT32_SWAP_ON_BE(decsize);
		compsize = wxINT32_SWAP_ON_BE(compsize);

		// Check offset+size
		if ((unsigned)(offset + compsize) > mc.getSize())
		{
			LOG_MESSAGE(1, "ADatArchive::open: dat archive is invalid or corrupt (entry goes past end of file)");
			Global::error = "Archive is invalid and/or corrupt";
			setMuted(false);
			return false;
		}

		// Parse name
		wxFileName fn(wxString::FromAscii(name, 128));

		// Create directory if needed
		ArchiveTreeNode* dir = createDir(fn.GetPath(true, wxPATH_UNIX));

		// Create entry
		ArchiveEntry* entry = new ArchiveEntry(fn.GetFullName(), compsize);
		entry->exProp("Offset") = (int)offset;
		entry->exProp("FullSize") = (int)decsize;
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
			MemChunk xdata;
			if (Compression::ZlibInflate(edata, xdata, (int)entry->exProp("FullSize")))
				entry->importMemChunk(xdata);
			else
			{
				LOG_MESSAGE(1, "Entry %s couldn't be inflated", entry->getName());
				entry->importMemChunk(edata);
			}
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

/* ADatArchive::write
 * Writes the dat archive to a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool ADatArchive::write(MemChunk& mc, bool update)
{
	// Clear current data
	mc.clear();
	MemChunk directory;
	MemChunk compressed;

	// Get archive tree as a list
	vector<ArchiveEntry*> entries;
	getEntryTreeAsList(entries);

	// Write header
	long dir_offset = wxINT32_SWAP_ON_BE(16);
	long dir_size = wxINT32_SWAP_ON_BE(0);
	char pack[4] = { 'A', 'D', 'A', 'T' };
	uint32_t version = wxINT32_SWAP_ON_BE(9);
	mc.seek(0, SEEK_SET);
	mc.write(pack, 4);
	mc.write(&dir_offset, 4);
	mc.write(&dir_size, 4);
	mc.write(&version, 4);

	// Write entry data
	for (unsigned a = 0; a < entries.size(); a++)
	{
		// Skip folders
		if (entries[a]->getType() == EntryType::folderType())
			continue;

		// Create compressed version of the lump
		MemChunk* entry = NULL;
		if (Compression::ZlibDeflate(entries[a]->getMCData(), compressed, 9))
		{
			entry = &compressed;
		}
		else
		{
			entry = &(entries[a]->getMCData());
			LOG_MESSAGE(1, "Entry %s couldn't be deflated", entries[a]->getName());
		}

		// Update entry
		int offset = mc.currentPos();
		if (update)
		{
			entries[a]->setState(0);
			entries[a]->exProp("Offset") = (int)offset;
		}

		///////////////////////////////////
		// Step 1: Write directory entry //
		///////////////////////////////////

		// Check entry name
		string name = entries[a]->getPath(true);
		name.Remove(0, 1);	// Remove leading /
		if (name.Len() > 128)
		{
			LOG_MESSAGE(1, "Warning: Entry %s path is too long (> 128 characters), putting it in the root directory", name);
			wxFileName fn(name);
			name = fn.GetFullName();
			if (name.Len() > 128)
				name.Truncate(128);
		}

		// Write entry name
		char name_data[128];
		memset(name_data, 0, 128);
		memcpy(name_data, CHR(name), name.Length());
		directory.write(name_data, 128);

		// Write entry offset
		long myoffset = wxINT32_SWAP_ON_BE(offset);
		directory.write(&myoffset, 4);

		// Write full entry size
		long decsize = wxINT32_SWAP_ON_BE(entries[a]->getSize());
		directory.write(&decsize, 4);

		// Write compressed entry size
		long compsize = wxINT32_SWAP_ON_BE(entry->getSize());
		directory.write(&compsize, 4);

		// Write whatever it is that should be there
		// TODO: Reverse engineer what exactly it is
		// and implement something valid for the game.
		long whatever = 0;
		directory.write(&whatever, 4);

		//////////////////////////////
		// Step 2: Write entry data //
		//////////////////////////////

		mc.write(entry->getData(), entry->getSize());
	}

	// Write directory
	dir_offset = wxINT32_SWAP_ON_BE(mc.currentPos());
	dir_size = wxINT32_SWAP_ON_BE(directory.getSize());
	mc.write(directory.getData(), directory.getSize());

	// Update directory offset and size in header
	mc.seek(4, SEEK_SET);
	mc.write(&dir_offset, 4);
	mc.write(&dir_size, 4);

	// Yay! Finished!
	return true;
}

/* ADatArchive::write
 * Writes the dat archive to a file
 * Returns true if successful, false otherwise
 *******************************************************************/
bool ADatArchive::write(string filename, bool update)
{
	// Write to a MemChunk, then export it to a file
	MemChunk mc;
	if (write(mc, true))
		return mc.exportFile(filename);
	else
		return false;
}

/* ADatArchive::loadEntryData
 * Loads an entry's data from the dat file
 * Returns true if successful, false otherwise
 *******************************************************************/
bool ADatArchive::loadEntryData(ArchiveEntry* entry)
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
		LOG_MESSAGE(1, "ADatArchive::loadEntryData: Unable to open archive file %s", filename);
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
 * ADATARCHIVE CLASS STATIC FUNCTIONS
 *******************************************************************/

/* ADatArchive::isADatArchive
 * Checks if the given data is a valid Anachronox dat archive
 *******************************************************************/
bool ADatArchive::isADatArchive(MemChunk& mc)
{
	// Check it opened ok
	if (mc.getSize() < 16)
		return false;

	// Read dat header
	char magic[4];
	long dir_offset;
	long dir_size;
	long version;
	mc.seek(0, SEEK_SET);
	mc.read(magic, 4);
	mc.read(&dir_offset, 4);
	mc.read(&dir_size, 4);
	mc.read(&version, 4);

	// Byteswap values for big endian if needed
	dir_size = wxINT32_SWAP_ON_BE(dir_size);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);

	// Check version
	if (wxINT32_SWAP_ON_BE(version) != 9)
		return false;

	// Check header
	if (magic[0] != 'A' || magic[1] != 'D' || magic[2] != 'A' || magic[3] != 'T')
		return false;

	// Check directory is sane
	if (dir_offset < 16 || (unsigned)(dir_offset + dir_size) > mc.getSize())
		return false;

	// That'll do
	return true;
}

/* ADatArchive::isADatArchive
 * Checks if the file at [filename] is a valid Anachronox dat archive
 *******************************************************************/
bool ADatArchive::isADatArchive(string filename)
{
	// Open file for reading
	wxFile file(filename);

	// Check it opened ok
	if (!file.IsOpened() || file.Length() < 16)
		return false;

	// Read dat header
	char magic[4];
	long dir_offset;
	long dir_size;
	long version;
	file.Seek(0, wxFromStart);
	file.Read(magic, 4);
	file.Read(&dir_offset, 4);
	file.Read(&dir_size, 4);
	file.Read(&version, 4);

	// Byteswap values for big endian if needed
	dir_size = wxINT32_SWAP_ON_BE(dir_size);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);

	// Check version
	if (wxINT32_SWAP_ON_BE(version) != 9)
		return false;

	// Check header
	if (magic[0] != 'A' || magic[1] != 'D' || magic[2] != 'A' || magic[3] != 'T')
		return false;

	// Check directory is sane
	if (dir_offset < 16 || dir_offset + dir_size > file.Length())
		return false;

	// That'll do
	return true;
}
