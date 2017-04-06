
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    LibArchive.cpp
 * Description: LibArchive, archive class to handle shadow.lib
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
#include "LibArchive.h"
#include "General/UI.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/

/*******************************************************************
 * LIBARCHIVE CLASS FUNCTIONS
 *******************************************************************/

/* LibArchive::LibArchive
 * LibArchive class constructor
 *******************************************************************/
LibArchive::LibArchive()
	: TreelessArchive(ARCHIVE_LIB)
{
	desc.max_name_length = 12;
	desc.names_extensions = false;
	desc.supports_dirs = false;
}

/* LibArchive::~LibArchive
 * LibArchive class destructor
 *******************************************************************/
LibArchive::~LibArchive()
{
}

/* LibArchive::getEntryOffset
 * Gets a lump entry's offset
 * Returns the lump entry's offset, or zero if it doesn't exist
 *******************************************************************/
uint32_t LibArchive::getEntryOffset(ArchiveEntry* entry)
{
	// Check entry
	if (!checkEntry(entry))
		return 0;

	return (uint32_t)(int)entry->exProp("Offset");
}

/* LibArchive::setEntryOffset
 * Sets a lump entry's offset
 *******************************************************************/
void LibArchive::setEntryOffset(ArchiveEntry* entry, uint32_t offset)
{
	// Check entry
	if (!checkEntry(entry))
		return;

	entry->exProp("Offset") = (int)offset;
}

/* LibArchive::getFileExtensionString
 * Gets the wxWidgets file dialog filter string for the archive type
 *******************************************************************/
string LibArchive::getFileExtensionString()
{
	return "Shadowcaster Lib Files (*.lib)|*.lib";
}

/* LibArchive::getFormat
 * Gives the "archive_lib" string
 *******************************************************************/
string LibArchive::getFormat()
{
	return "archive_lib";
}

/* LibArchive::open
 * Reads wad format data from a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool LibArchive::open(MemChunk& mc)
{
	// Check data was given
	if (!mc.hasData())
		return false;

	// Read lib footer
	mc.seek(2, SEEK_END);
	uint32_t num_lumps = 0;
	mc.read(&num_lumps, 2);		// Size
	num_lumps = wxINT16_SWAP_ON_BE(num_lumps);
	uint32_t dir_offset = mc.getSize() - (2 + (num_lumps * 21));

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	setMuted(true);

	// Read the directory
	mc.seek(dir_offset, SEEK_SET);
	UI::setSplashProgressMessage("Reading lib archive data");
	for (uint32_t d = 0; d < num_lumps; d++)
	{
		// Update splash window progress
		UI::setSplashProgress(((float)d / (float)num_lumps));

		// Read lump info
		char myname[13] = "";
		uint32_t offset = 0;
		uint32_t size = 0;

		mc.read(&size, 4);		// Size
		mc.read(&offset, 4);	// Offset
		mc.read(myname, 13);	// Name
		myname[12] = '\0';

		// Byteswap values for big endian if needed
		offset = wxINT32_SWAP_ON_BE(offset);
		size = wxINT32_SWAP_ON_BE(size);

		// If the lump data goes past the directory,
		// the wadfile is invalid
		if (offset + size > dir_offset)
		{
			LOG_MESSAGE(1, "LibArchive::open: Lib archive is invalid or corrupt");
			Global::error = "Archive is invalid and/or corrupt";
			setMuted(false);
			return false;
		}

		// Create & setup lump
		ArchiveEntry* nlump = new ArchiveEntry(wxString::FromAscii(myname), size);
		nlump->setLoaded(false);
		nlump->exProp("Offset") = (int)offset;
		nlump->setState(0);

		// Add to entry list
		getRoot()->addEntry(nlump);
		//entries.push_back(nlump);
	}

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

		// Set entry to unchanged
		entry->setState(0);
	}

	// Detect maps (will detect map entry types)
	UI::setSplashProgressMessage("Detecting maps");
	detectMaps();

	// Setup variables
	setMuted(false);
	setModified(false);
	announce("opened");

	UI::setSplashProgressMessage("");

	return true;
}

/* LibArchive::write
 * Writes the wad archive to a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool LibArchive::write(MemChunk& mc, bool update)
{
	// Only two bytes are used for storing entry amount,
	// so abort for excessively large files:
	if (numEntries() > 65535)
		return false;

	uint16_t num_files = numEntries();
	uint32_t dir_offset = 0;
	ArchiveEntry* entry = NULL;
	for (uint16_t l = 0; l < num_files; l++)
	{
		entry = getEntry(l);
		setEntryOffset(entry, dir_offset);
		dir_offset += entry->getSize();
	}

	// Clear/init MemChunk
	mc.clear();
	mc.seek(0, SEEK_SET);
	mc.reSize(2 + dir_offset + num_files * 21);

	// Write the files
	for (uint16_t l = 0; l < num_files; l++)
	{
		entry = getEntry(l);
		mc.write(entry->getData(), entry->getSize());
	}

	// Write the directory
	for (uint16_t l = 0; l < num_files; l++)
	{
		entry = getEntry(l);
		char name[13] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		long offset = wxINT32_SWAP_ON_BE(getEntryOffset(entry));
		long size = wxINT32_SWAP_ON_BE(entry->getSize());

		for (size_t c = 0; c < entry->getName().length() && c < 12; c++)
			name[c] = entry->getName()[c];

		mc.write(&size, 4);		// Size
		mc.write(&offset, 4);	// Offset
		mc.write(name, 13);		// Name

		if (update)
		{
			entry->setState(0);
			entry->exProp("Offset") = (int)wxINT32_SWAP_ON_BE(offset);
		}
	}

	// Write the footer
	num_files = wxINT16_SWAP_ON_BE(num_files);
	mc.write(&num_files, 2);

	// Finished!
	return true;
}

/* LibArchive::loadEntryData
 * Loads an entry's data from the wadfile
 * Returns true if successful, false otherwise
 *******************************************************************/
bool LibArchive::loadEntryData(ArchiveEntry* entry)
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

	// Open wadfile
	wxFile file(filename);

	// Check if opening the file failed
	if (!file.IsOpened())
	{
		LOG_MESSAGE(1, "LibArchive::loadEntryData: Failed to open libfile %s", filename);
		return false;
	}

	// Seek to lump offset in file and read it in
	file.Seek(getEntryOffset(entry), wxFromStart);
	entry->importFileStream(file, entry->getSize());

	// Set the lump to loaded
	entry->setLoaded();

	return true;
}

/* LibArchive::addEntry
 * Override of Archive::addEntry to force entry addition to the root
 * directory and rename the entry if necessary to be lib-friendly
 * (12 characters max)
 *******************************************************************/
ArchiveEntry* LibArchive::addEntry(ArchiveEntry* entry, unsigned position, ArchiveTreeNode* dir, bool copy)
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

	// Process name (must be 12 characters max)
	string name = entry->getName().Truncate(12);

	// Set new wad-friendly name
	entry->setName(name);

	// Do default entry addition (to root directory)
	Archive::addEntry(entry, position);

	return entry;
}

/* LibArchive::addEntry
 * There is no namespaces in lib archives, so just put it at the end
 *******************************************************************/
ArchiveEntry* LibArchive::addEntry(ArchiveEntry* entry, string add_namespace, bool copy)
{
	return addEntry(entry, 0xFFFFFFFF, NULL, copy);
}

/* LibArchive::renameEntry
 * Override of Archive::renameEntry to rename the entry if necessary
 * to be lib-friendly (12 characters max)
 *******************************************************************/
bool LibArchive::renameEntry(ArchiveEntry* entry, string name)
{
	// Check entry
	if (!checkEntry(entry))
		return false;

	// Process name (must be 12 characters max)
	name = name.Truncate(12);

	// Do default rename
	return Archive::renameEntry(entry, name);
}


/* LibArchive::isLibArchive
 * Checks if the given data is a valid Shadowcaster lib archive
 *******************************************************************/
bool LibArchive::isLibArchive(MemChunk& mc)
{
	if (mc.getSize() < 64)
		return false;

	// Read lib footer
	mc.seek(2, SEEK_END);
	uint32_t num_lumps = 0;
	mc.read(&num_lumps, 2);		// Size
	num_lumps = wxINT16_SWAP_ON_BE(num_lumps);
	int32_t dir_offset = mc.getSize() - (2 + (num_lumps * 21));

	// Check directory offset is valid
	if (dir_offset < 0)
		return false;

	// Check directory offset is decent
	mc.seek(dir_offset, SEEK_SET);
	char myname[13] = "";
	uint32_t offset = 0;
	uint32_t size = 0;
	uint8_t dummy = 0;
	mc.read(&size, 4);		// Size
	mc.read(&offset, 4);	// Offset
	mc.read(myname, 12);	// Name
	mc.read(&dummy, 1);		// Separator
	offset = wxINT32_SWAP_ON_BE(offset);
	size = wxINT32_SWAP_ON_BE(size);
	myname[12] = '\0';

	// If the lump data goes past the directory,
	// the library is invalid
	if (dummy != 0 || offset != 0 || offset + size > mc.getSize())
	{
		return false;
	}

	// Check that the file name given for the first lump is acceptable
	int filnamlen = 0;
	for (; filnamlen < 13; ++filnamlen)
	{
		if (myname[filnamlen] == 0)
			break;
		if (myname[filnamlen] < 33 || myname[filnamlen] > 126 ||
		        myname[filnamlen] == '"' || myname[filnamlen] == '*' || myname[filnamlen] == '/' ||
		        myname[filnamlen] == ':' || myname[filnamlen] == '<' || myname[filnamlen] == '?' ||
		        myname[filnamlen] == '\\' || myname[filnamlen] == '|')
			return false;
	}
	// At a minimum, one character for the name and the dot separating it from the extension
	if (filnamlen < 2)
		return false;

	// If it's passed to here it's probably a lib file
	return true;
}

/* LibArchive::isLibArchive
 * Checks if the file at [filename] is a valid Shadowcaster lib archive
 *******************************************************************/
bool LibArchive::isLibArchive(string filename)
{
	// Open file for reading
	wxFile file(filename);

	// Check it opened ok
	if (!file.IsOpened())
		return false;
	// Read lib footer
	file.Seek(file.Length() - 2, wxFromStart); // wxFromEnd does not work for some reason
	uint32_t num_lumps = 0;
	file.Read(&num_lumps, 2);		// Size
	num_lumps = wxINT16_SWAP_ON_BE(num_lumps);
	int32_t dir_offset = file.Length() - (2 + (num_lumps * 21));

	// Check directory offset is valid
	if (dir_offset < 0)
		return false;

	// Check directory offset is decent
	file.Seek(dir_offset, wxFromStart);
	char myname[13] = "";
	uint32_t offset = 0;
	uint32_t size = 0;
	uint8_t dummy = 0;
	file.Read(&size, 4);	// Size
	file.Read(&offset, 4);	// Offset
	file.Read(myname, 12);	// Name
	file.Read(&dummy, 1);	// Separator
	offset = wxINT32_SWAP_ON_BE(offset);
	size = wxINT32_SWAP_ON_BE(size);
	myname[12] = '\0';

	// If the lump data goes past the directory,
	// the library is invalid
	if (dummy != 0 || offset != 0 || offset + size > file.Length())
	{
		return false;
	}
	// Check that the file name given for the first lump is acceptable
	int filnamlen = 0;
	for (; filnamlen < 13; ++filnamlen)
	{
		if (myname[filnamlen] == 0)
			break;
		if (myname[filnamlen] < 33 || myname[filnamlen] > 126 ||
		        myname[filnamlen] == '"' || myname[filnamlen] == '*' || myname[filnamlen] == '/' ||
		        myname[filnamlen] == ':' || myname[filnamlen] == '<' || myname[filnamlen] == '?' ||
		        myname[filnamlen] == '\\' || myname[filnamlen] == '|')
			return false;
	}
	// At a minimum, one character for the name and the dot separating it from the extension
	if (filnamlen == 0)
		return false;

	// If it's passed to here it's probably a lib file
	return true;
}
