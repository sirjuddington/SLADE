
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    LfdArchive.cpp
 * Description: LfdArchive, archive class to handle LFD archives
 *              from Star Wars: Dark Forces
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
#include "LfdArchive.h"
#include "General/UI.h"


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, wad_force_uppercase)
EXTERN_CVAR(Bool, archive_load_data)

/*******************************************************************
 * LFDARCHIVE CLASS FUNCTIONS
 *******************************************************************/

/* LfdArchive::LfdArchive
 * LfdArchive class constructor
 *******************************************************************/
LfdArchive::LfdArchive() : TreelessArchive(ARCHIVE_LFD)
{
	desc.max_name_length = 8;
	desc.names_extensions = false;
	desc.supports_dirs = false;
}

/* LfdArchive::~LfdArchive
 * LfdArchive class destructor
 *******************************************************************/
LfdArchive::~LfdArchive()
{
}

/* LfdArchive::getEntryOffset
 * Returns the file byte offset for [entry]
 *******************************************************************/
uint32_t LfdArchive::getEntryOffset(ArchiveEntry* entry)
{
	// Check entry
	if (!checkEntry(entry))
		return 0;

	return (uint32_t)(int)entry->exProp("Offset");
}

/* LfdArchive::setEntryOffset
 * Sets the file byte offset for [entry]
 *******************************************************************/
void LfdArchive::setEntryOffset(ArchiveEntry* entry, uint32_t offset)
{
	// Check entry
	if (!checkEntry(entry))
		return;

	entry->exProp("Offset") = (int)offset;
}

/* LfdArchive::getFileExtensionString
 * Gets the wxWidgets file dialog filter string for the archive type
 *******************************************************************/
string LfdArchive::getFileExtensionString()
{
	return "Lfd Files (*.lfd)|*.lfd";
}

/* LfdArchive::getFormat
 * Returns the EntryDataFormat id of this archive type
 *******************************************************************/
string LfdArchive::getFormat()
{
	return "archive_lfd";
}

/* LfdArchive::open
 * Reads lfd format data from a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool LfdArchive::open(MemChunk& mc)
{
	// Check data was given
	if (!mc.hasData())
		return false;

	// Check size
	if (mc.getSize() < 16)
		return false;

	// Check magic header
	if (mc[0] != 'R' || mc[1] != 'M' || mc[2] != 'A' || mc[3] != 'P')
		return false;

	// Get directory length
	uint32_t dir_len = 0;
	mc.seek(12, SEEK_SET);
	mc.read(&dir_len, 4);
	dir_len = wxINT32_SWAP_ON_BE(dir_len);

	// Check size
	if ((unsigned)mc.getSize() < (dir_len) || dir_len % 16)
		return false;

	// Guess number of lumps
	uint32_t num_lumps = dir_len / 16;

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	setMuted(true);

	// Read each entry
	UI::setSplashProgressMessage("Reading lfd archive data");
	size_t offset = dir_len + 16;
	size_t size = mc.getSize();
	for (uint32_t d = 0; offset < size; d++)
	{
		// Update splash window progress
		UI::setSplashProgress(((float)d / (float)num_lumps));

		// Read lump info
		uint32_t length = 0;
		char type[5] = "";
		char name[9] = "";

		mc.read(type, 4);		// Type
		mc.read(name, 8);		// Name
		mc.read(&length, 4);	// Size
		name[8] = '\0'; type[4] = 0;

		// Move past the header
		offset += 16;

		// Byteswap values for big endian if needed
		length = wxINT32_SWAP_ON_BE(length);

		// If the lump data goes past the end of the file,
		// the gobfile is invalid
		if (offset + length > size)
		{
			LOG_MESSAGE(1, "LfdArchive::open: lfd archive is invalid or corrupt");
			Global::error = "Archive is invalid and/or corrupt";
			setMuted(false);
			return false;
		}

		// Create & setup lump
		wxFileName fn(name);
		fn.SetExt(type);
		ArchiveEntry* nlump = new ArchiveEntry(fn.GetFullName(), length);
		nlump->setLoaded(false);
		nlump->exProp("Offset") = (int)offset;
		nlump->setState(0);

		// Add to entry list
		getRoot()->addEntry(nlump);

		// Move to next entry
		offset += length;
		mc.seek(offset, SEEK_SET);
	}

	if (num_lumps != numEntries())
		LOG_MESSAGE(1, "Warning: computed %i lumps, but actually %i entries", num_lumps, numEntries());

	// Detect all entry types
	MemChunk edata;
	UI::setSplashProgressMessage("Detecting entry types");
	for (size_t a = 0; a < numEntries(); a++)
	{
		// Update splash window progress
		UI::setSplashProgress((((float)a / (float)num_lumps)));

		// Get entry
		ArchiveEntry* entry = getEntry(a);

		// Read entry data if it isn't zero-sized
		if (entry->getSize() > 0)
		{
			// Read the entry data
			mc.exportMemChunk(edata, getEntryOffset(entry), entry->getSize());
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

/* LfdArchive::write
 * Writes the lfd archive to a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool LfdArchive::write(MemChunk& mc, bool update)
{
	// Determine total size
	uint32_t dir_size = (numEntries() + 1)<<4;
	uint32_t total_size = dir_size;
	ArchiveEntry* entry = NULL;
	for (uint32_t l = 0; l < numEntries(); l++)
	{
		entry = getEntry(l);
		total_size += 16;
		setEntryOffset(entry, total_size);
		if (update)
		{
			entry->setState(0);
			entry->exProp("Offset") = (int)total_size;
		}
		total_size += entry->getSize();
	}

	// Clear/init MemChunk
	mc.clear();
	mc.seek(0, SEEK_SET);
	mc.reSize(total_size);

	// Variables
	char type[5] = "RMAP";
	char name[9] = "resource";
	size_t size = wxINT32_SWAP_ON_BE(numEntries()<<4);


	// Write the resource map first
	mc.write(type, 4);
	mc.write(name, 8);
	mc.write(&size,4);
	for (uint32_t l = 0; l < numEntries(); l++)
	{
		entry = getEntry(l);
		for (int t = 0; t < 5; ++t) type[t] = 0;
		for (int n = 0; n < 9; ++n) name[n] = 0;
		size = wxINT32_SWAP_ON_BE(entry->getSize());
		wxFileName fn(entry->getName());

		for (size_t c = 0; c < fn.GetName().length() && c < 9; c++)
			name[c] = fn.GetName()[c];
		for (size_t c = 0; c < fn.GetExt().length() && c < 5; c++)
			type[c] = fn.GetExt()[c];

		mc.write(type, 4);
		mc.write(name, 8);
		mc.write(&size,4);
	}

	// Write the lumps
	for (uint32_t l = 0; l < numEntries(); l++)
	{
		entry = getEntry(l);
		for (int t = 0; t < 5; ++t) type[t] = 0;
		for (int n = 0; n < 9; ++n) name[n] = 0;
		size = wxINT32_SWAP_ON_BE(entry->getSize());
		wxFileName fn(entry->getName());

		for (size_t c = 0; c < fn.GetName().length() && c < 9; c++)
			name[c] = fn.GetName()[c];
		for (size_t c = 0; c < fn.GetExt().length() && c < 5; c++)
			type[c] = fn.GetExt()[c];

		mc.write(type, 4);
		mc.write(name, 8);
		mc.write(&size,4);
		mc.write(entry->getData(), entry->getSize());
	}

	return true;
}

/* LfdArchive::loadEntryData
 * Loads an entry's data from the lfdfile
 * Returns true if successful, false otherwise
 *******************************************************************/
bool LfdArchive::loadEntryData(ArchiveEntry* entry)
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

	// Open lfdfile
	wxFile file(filename);

	// Check if opening the file failed
	if (!file.IsOpened())
	{
		LOG_MESSAGE(1, "LfdArchive::loadEntryData: Failed to open lfdfile %s", filename);
		return false;
	}

	// Seek to lump offset in file and read it in
	file.Seek(getEntryOffset(entry), wxFromStart);
	entry->importFileStream(file, entry->getSize());

	// Set the lump to loaded
	entry->setLoaded();

	return true;
}

/* LfdArchive::addEntry
 * Override of Archive::addEntry to force entry addition to the root
 * directory, update namespaces if needed and rename the entry if
 * necessary to be lfd-friendly (13 characters max with extension)
 *******************************************************************/
ArchiveEntry* LfdArchive::addEntry(ArchiveEntry* entry, unsigned position, ArchiveTreeNode* dir, bool copy)
{
	// Check entry
	if (!entry)
		return NULL;

	// Check if read-only
	if (isReadOnly())
		return NULL;

	// Copy if necessary
	if (copy)
		entry = new ArchiveEntry(*entry);

	// Process name (must be 13 characters max)
	string name = entry->getName().Truncate(13);
	if (wad_force_uppercase) name.MakeUpper();

	// Set new lfd-friendly name
	entry->setName(name);

	// Do default entry addition (to root directory)
	Archive::addEntry(entry, position);

	return entry;
}

/* LfdArchive::addEntry
 * Since lfd files have no namespaces, just call the other function.
 *******************************************************************/
ArchiveEntry* LfdArchive::addEntry(ArchiveEntry* entry, string add_namespace, bool copy)
{
	return addEntry(entry, 0xFFFFFFFF, NULL, copy);
}

/* LfdArchive::renameEntry
 * Override of Archive::renameEntry to update namespaces if needed
 * and rename the entry if necessary to be lfd-friendly (twelve
 * characters max)
 *******************************************************************/
bool LfdArchive::renameEntry(ArchiveEntry* entry, string name)
{
	// Check entry
	if (!checkEntry(entry))
		return false;

	// Process name (must be 13 characters max)
	name.Truncate(13);
	if (wad_force_uppercase) name.MakeUpper();

	// Do default rename
	return Archive::renameEntry(entry, name);
}

/* LfdArchive::isLfdArchive
 * Checks if the given data is a valid Dark Forces lfd archive
 *******************************************************************/
bool LfdArchive::isLfdArchive(MemChunk& mc)
{
	// Check size
	if (mc.getSize() < 12)
		return false;

	// Check magic header
	if (mc[0] != 'R' || mc[1] != 'M' || mc[2] != 'A' || mc[3] != 'P')
		return false;

	// Get offset of first entry
	uint32_t dir_offset = 0;
	mc.seek(12, SEEK_SET);
	mc.read(&dir_offset, 4);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset) + 16;
	if (dir_offset % 16) return false;
	char type1[5]; char type2[5];
	char name1[9]; char name2[9];
	uint32_t len1; uint32_t len2;
	mc.read(type1, 4); type1[4] = 0;
	mc.read(name1, 8); name1[8] = 0;
	mc.read(&len1, 4); len1 = wxINT32_SWAP_ON_BE(len1);

	// Check size
	if ((unsigned)mc.getSize() < (dir_offset + 16 + len1))
		return false;

	// Compare
	mc.seek(dir_offset, SEEK_SET);
	mc.read(type2, 4); type2[4] = 0;
	mc.read(name2, 8); name2[8] = 0;
	mc.read(&len2, 4); len2 = wxINT32_SWAP_ON_BE(len2);

	if (strcmp(type1, type2) || strcmp(name1, name2) || len1 != len2)
		return false;

	// If it's passed to here it's probably a lfd file
	return true;
}

/* LfdArchive::isLfdArchive
 * Checks if the file at [filename] is a valid Dark Forces lfd archive
 *******************************************************************/
bool LfdArchive::isLfdArchive(string filename)
{
	// Open file for reading
	wxFile file(filename);

	// Check it opened ok
	if (!file.IsOpened())
		return false;

	// Check size
	if (file.Length() < 16)
		return false;

	// Read header
	char header[4];
	file.Read(header, 4);

	// Check magic header
	if (header[0] != 'R' || header[1] != 'M' || header[2] != 'A' || header[3] != 'P')
		return false;

	// Get offset of first entry
	uint32_t dir_offset = 0;
	file.Seek(12, wxFromStart);
	file.Read(&dir_offset, 4);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset) + 16;
	if (dir_offset % 16) return false;
	char type1[5]; char type2[5];
	char name1[9]; char name2[9];
	uint32_t len1; uint32_t len2;
	file.Read(type1, 4); type1[4] = 0;
	file.Read(name1, 8); name1[8] = 0;
	file.Read(&len1, 4); len1 = wxINT32_SWAP_ON_BE(len1);

	// Check size
	if ((unsigned)file.Length() < (dir_offset + 16 + len1))
		return false;

	// Compare
	file.Seek(dir_offset, wxFromStart);
	file.Read(type2, 4); type2[4] = 0;
	file.Read(name2, 8); name2[8] = 0;
	file.Read(&len2, 4); len2 = wxINT32_SWAP_ON_BE(len2);

	if (strcmp(type1, type2) || strcmp(name1, name2) || len1 != len2)
		return false;

	// If it's passed to here it's probably a lfd file
	return true;
}

