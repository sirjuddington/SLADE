
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ResArchive.cpp
 * Description: ResArchive, archive class to handle A&A format
 *              res archives
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
#include "ResArchive.h"
#include "General/UI.h"

/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, archive_load_data)

/*******************************************************************
 * RESARCHIVE CLASS FUNCTIONS
 *******************************************************************/

/* ResArchive::ResArchive
 * ResArchive class constructor
 *******************************************************************/
ResArchive::ResArchive() : Archive(ARCHIVE_RES)
{
	// Init variables
	desc.supports_dirs = true;
}

/* ResArchive::~ResArchive
 * ResArchive class destructor
 *******************************************************************/
ResArchive::~ResArchive()
{
}

/* ResArchive::getEntryOffset
 * Returns the file byte offset for [entry]
 *******************************************************************/
uint32_t ResArchive::getEntryOffset(ArchiveEntry* entry)
{
	// Check entry
	if (!checkEntry(entry))
		return 0;

	return (uint32_t)(int)entry->exProp("Offset");
}

/* ResArchive::setEntryOffset
 * Sets the file byte offset for [entry]
 *******************************************************************/
void ResArchive::setEntryOffset(ArchiveEntry* entry, uint32_t offset)
{
	// Check entry
	if (!checkEntry(entry))
		return;

	entry->exProp("Offset") = (int)offset;
}

/* ResArchive::getFileExtensionString
 * Gets the wxWidgets file dialog filter string for the archive type
 *******************************************************************/
string ResArchive::getFileExtensionString()
{
	return "Res Files (*.res)|*.res";
}

/* ResArchive::getFormat
 * Returns the EntryDataFormat id of this archive type
 *******************************************************************/
string ResArchive::getFormat()
{
	return "archive_res";
}

/* ResArchive::readDirectory
 * Reads a res directory from a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool ResArchive::readDirectory(MemChunk& mc, size_t dir_offset, size_t num_lumps, ArchiveTreeNode* parent)
{
	if (!parent)
	{
		LOG_MESSAGE(1, "ReadDir: No parent node");
		Global::error = "Archive is invalid and/or corrupt";
		return false;
	}
	mc.seek(dir_offset, SEEK_SET);
	for (uint32_t d = 0; d < num_lumps; d++)
	{
		// Update splash window progress
		UI::setSplashProgress(((float)d / (float)num_lumps));

		// Read lump info
		char magic[4] = "";
		char name[15] = "";
		uint32_t dumzero1, dumzero2;
		uint16_t dumff, dumze;
		uint8_t flags = 0;
		uint32_t offset = 0;
		uint32_t size = 0;

		mc.read(magic, 4);		// ReS\0
		mc.read(name, 14);		// Name
		mc.read(&offset, 4);	// Offset
		mc.read(&size, 4);		// Size

		// Check the identifier
		if (magic[0] != 'R' || magic[1] != 'e' || magic[2] != 'S' || magic[3] != 0)
		{
			LOG_MESSAGE(1, "ResArchive::readDir: Entry %s (%i@0x%x) has invalid directory entry", name, size, offset);
			Global::error = "Archive is invalid and/or corrupt";
			return false;
		}

		// Byteswap values for big endian if needed
		offset = wxINT32_SWAP_ON_BE(offset);
		size = wxINT32_SWAP_ON_BE(size);
		name[14] = '\0';

		mc.read(&dumze, 2); if (dumze) LOG_MESSAGE(1, "Flag guard not null for entry %s", name);
		mc.read(&flags, 1); if (flags != 1 && flags != 17) LOG_MESSAGE(1, "Unknown flag value for entry %s", name);
		mc.read(&dumzero1, 4); if (dumzero1) LOG_MESSAGE(1, "Near-end values not set to zero for entry %s", name);
		mc.read(&dumff, 2); if (dumff != 0xFFFF) LOG_MESSAGE(1, "Dummy set to a non-FF value for entry %s", name);
		mc.read(&dumzero2, 4); if (dumzero2) LOG_MESSAGE(1, "Trailing values not set to zero for entry %s", name);

		// If the lump data goes past the end of the file,
		// the resfile is invalid
		if (offset + size > mc.getSize())
		{
			LOG_MESSAGE(1, "ResArchive::readDirectory: Res archive is invalid or corrupt, offset overflow");
			Global::error = "Archive is invalid and/or corrupt";
			setMuted(false);
			return false;
		}

		// Create & setup lump
		ArchiveEntry* nlump = new ArchiveEntry(wxString::FromAscii(name), size);
		nlump->setLoaded(false);
		nlump->exProp("Offset") = (int)offset;
		nlump->setState(0);

		// Read entry data if it isn't zero-sized
		if (nlump->getSize() > 0)
		{
			// Read the entry data
			MemChunk edata;
			mc.exportMemChunk(edata, offset, size);
			nlump->importMemChunk(edata);
		}

		// What if the entry is a directory?
		size_t d_o, n_l;
		if (isResArchive(nlump->getMCData(), d_o, n_l))
		{
			ArchiveTreeNode* ndir = createDir(name, parent);
			if (ndir)
			{
				UI::setSplashProgressMessage(S_FMT("Reading res archive data: %s directory", name));
				// Save offset to restore it once the recursion is done
				size_t myoffset = mc.currentPos();
				readDirectory(mc, d_o, n_l, ndir);
				ndir->getDirEntry()->setState(0);
				// Restore offset and clean out the entry
				mc.seek(myoffset, SEEK_SET);
				delete nlump;
			}
			else
			{
				delete nlump;
				return false;
			}
			// Not a directory, then add to entry list
		}
		else
		{
			parent->addEntry(nlump);
			// Detect entry type
			EntryType::detectEntryType(nlump);
			// Unload entry data if needed
			if (!archive_load_data)
				nlump->unloadData();
			// Set entry to unchanged
			nlump->setState(0);
		}
	}
	return true;
}

/* ResArchive::open
 * Reads res format data from a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool ResArchive::open(MemChunk& mc)
{
	// Check data was given
	if (!mc.hasData())
		return false;

	// Read res header
	uint32_t	dir_size = 0;
	uint32_t	dir_offset = 0;
	char		magic[4] = "";
	mc.seek(0, SEEK_SET);
	mc.read(&magic, 4);			// "Res!"
	mc.read(&dir_offset, 4);	// Offset to directory
	mc.read(&dir_size, 4);		// No. of lumps in res

	// Byteswap values for big endian if needed
	dir_size = wxINT32_SWAP_ON_BE(dir_size);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);

	// Check the header
	if (magic[0] != 'R' || magic[1] != 'e' || magic[2] != 's' || magic[3] != '!')
	{
		LOG_MESSAGE(1, "ResArchive::openFile: File %s has invalid header", filename);
		Global::error = "Invalid res header";
		return false;
	}

	if (dir_size % RESDIRENTRYSIZE)
	{
		LOG_MESSAGE(1, "ResArchive::openFile: File %s has invalid directory size", filename);
		Global::error = "Invalid res directory size";
		return false;
	}
	uint32_t num_lumps = dir_size / RESDIRENTRYSIZE;

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	setMuted(true);

	// Read the directory
	mc.seek(dir_offset, SEEK_SET);
	UI::setSplashProgressMessage("Reading res archive data");
	if (!readDirectory(mc, dir_offset, num_lumps, getRoot()))
		return false;

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

/* ResArchive::write
 * Writes the res archive to a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool ResArchive::write(MemChunk& mc, bool update)
{
	/*	// Determine directory offset & individual lump offsets
		uint32_t dir_offset = 12;
		ArchiveEntry* entry = NULL;
		for (uint32_t l = 0; l < numEntries(); l++) {
			entry = getEntry(l);
			setEntryOffset(entry, dir_offset);
			dir_offset += entry->getSize();
		}

		// Clear/init MemChunk
		mc.clear();
		mc.seek(0, SEEK_SET);
		mc.reSize(dir_offset + numEntries() * 16);

		// Write the header
		uint32_t num_lumps = numEntries();
		mc.write("Res!", 4);
		mc.write(&num_lumps, 4);
		mc.write(&dir_offset, 4);

		// Write the lumps
		for (uint32_t l = 0; l < num_lumps; l++) {
			entry = getEntry(l);
			mc.write(entry->getData(), entry->getSize());
		}

		// Write the directory
		for (uint32_t l = 0; l < num_lumps; l++) {
			entry = getEntry(l);
			char name[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
			long offset = getEntryOffset(entry);
			long size = entry->getSize();

			for (size_t c = 0; c < entry->getName().length() && c < 8; c++)
				name[c] = entry->getName()[c];

			mc.write(&offset, 4);
			mc.write(&size, 4);
			mc.write(name, 8);

			if (update) {
				entry->setState(0);
				entry->exProp("Offset") = (int)offset;
			}
		}
	*/
	return true;
}

/* ResArchive::loadEntryData
 * Loads an entry's data from the resfile
 * Returns true if successful, false otherwise
 *******************************************************************/
bool ResArchive::loadEntryData(ArchiveEntry* entry)
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

	// Open resfile
	wxFile file(filename);

	// Check if opening the file failed
	if (!file.IsOpened())
	{
		LOG_MESSAGE(1, "ResArchive::loadEntryData: Failed to open resfile %s", filename);
		return false;
	}

	// Seek to lump offset in file and read it in
	file.Seek(getEntryOffset(entry), wxFromStart);
	entry->importFileStream(file, entry->getSize());

	// Set the lump to loaded
	entry->setLoaded();

	return true;
}

/* ResArchive::addEntry
 * Override of Archive::addEntry to force entry addition to the root
 * directory, update namespaces if needed and rename the entry if
 * necessary to be res-friendly (14 characters max)
 *******************************************************************/
ArchiveEntry* ResArchive::addEntry(ArchiveEntry* entry, unsigned position, ArchiveTreeNode* dir, bool copy)
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

	// Process name (must be 14 characters max)
	wxFileName fn(entry->getName());
	string name = fn.GetName().Truncate(14);

	// Set new res-friendly name
	entry->setName(name);

	// Do default entry addition (to root directory)
	Archive::addEntry(entry, position);

	return entry;
}

/* ResArchive::addEntry
 * Adds [entry] to the end of the namespace matching [add_namespace].
 * If [copy] is true a copy of the entry is added. Returns the added
 * entry or NULL if the entry is invalid
 *******************************************************************/
ArchiveEntry* ResArchive::addEntry(ArchiveEntry* entry, string add_namespace, bool copy)
{
	// Namespace not found, add to global namespace (ie end of archive)
	return addEntry(entry, 0xFFFFFFFF, NULL, copy);
}

/* ResArchive::renameEntry
 * Override of Archive::renameEntry to update namespaces if needed
 * and rename the entry if necessary to be res-friendly (14 chars max)
 *******************************************************************/
bool ResArchive::renameEntry(ArchiveEntry* entry, string name)
{
	// Check entry
	if (!checkEntry(entry))
		return false;

	// Process name (must be 14 characters max)
	wxFileName fn(name);
	name = fn.GetName().Truncate(14);

	// Do default rename
	return Archive::renameEntry(entry, name);

}

/* ResArchive::isResArchive
 * Checks if the given data is a valid A&A res archive
 *******************************************************************/
bool ResArchive::isResArchive(MemChunk& mc)
{
	size_t dummy1, dummy2;
	return isResArchive(mc, dummy1, dummy2);
}
bool ResArchive::isResArchive(MemChunk& mc, size_t& dir_offset, size_t& num_lumps)
{
	// Check size
	if (mc.getSize() < 12)
		return false;

	// Check for "Res!" header
	if (!(mc[0] == 'R' && mc[1] == 'e' && mc[2] == 's' && mc[3] == '!'))
		return false;

	// Get number of lumps and directory offset
	uint32_t offset_offset = 0;
	uint32_t rel_offset = 0;
	uint32_t dir_size = 0;
	mc.seek(4, SEEK_SET);
	mc.read(&dir_offset, 4);
	mc.read(&dir_size, 4);

	// Byteswap values for big endian if needed
	dir_size = wxINT32_SWAP_ON_BE(dir_size);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);

	// A&A contains nested resource files. The offsets are then always relative to
	// the top-level file. This causes problem with the embedded archive system
	// used by SLADE3. The solution is to compute the offset offset. :)
	offset_offset = dir_offset - (mc.getSize() - dir_size);
	rel_offset = dir_offset - offset_offset;

	// Check directory offset and size are both decent
	if (dir_size % RESDIRENTRYSIZE || (rel_offset + dir_size) > mc.getSize())
		return false;

	num_lumps = dir_size / RESDIRENTRYSIZE;

	// Reset MemChunk (just in case)
	mc.seek(0, SEEK_SET);

	// If it's passed to here it's probably a res file
	return true;
}

/* ResArchive::isResArchive
 * Checks if the file at [filename] is a valid A&A res archive
 *******************************************************************/
bool ResArchive::isResArchive(string filename)
{
	// Open file for reading
	wxFile file(filename);

	// Check it opened ok
	if (!file.IsOpened())
		return false;

	file.Seek(0, wxFromStart);

	// Read header
	char header[5];
	file.Read(header, 4);
	header[4] = 0;

	// Check for "Res!" header
	if (!(header[0] == 'R' && header[1] == 'e' && header[2] == 's' && header[3] == '!'))
		return false;

	// Get number of lumps and directory offset
	uint32_t dir_offset = 0;
	uint32_t dir_size = 0;
	file.Read(&dir_offset, 4);
	file.Read(&dir_size, 4);

	// Byteswap values for big endian if needed
	dir_size = wxINT32_SWAP_ON_BE(dir_size);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);

	// Check directory offset and size are both decent
	if (dir_size % RESDIRENTRYSIZE || (dir_offset + dir_size) > file.Length())
		return false;

	// If it's passed to here it's probably a res file
	return true;
}
