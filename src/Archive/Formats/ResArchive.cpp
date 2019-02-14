
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ResArchive.cpp
// Description: ResArchive, archive class to handle A&A format res archives
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
#include "ResArchive.h"
#include "General/UI.h"


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, archive_load_data)


// -----------------------------------------------------------------------------
//
// ResArchive Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the file byte offset for [entry]
// -----------------------------------------------------------------------------
uint32_t ResArchive::getEntryOffset(ArchiveEntry* entry)
{
	// Check entry
	if (!checkEntry(entry))
		return 0;

	return (uint32_t)(int)entry->exProp("Offset");
}

// -----------------------------------------------------------------------------
// Sets the file byte offset for [entry]
// -----------------------------------------------------------------------------
void ResArchive::setEntryOffset(ArchiveEntry* entry, uint32_t offset)
{
	// Check entry
	if (!checkEntry(entry))
		return;

	entry->exProp("Offset") = (int)offset;
}

// -----------------------------------------------------------------------------
// Reads a res directory from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool ResArchive::readDirectory(MemChunk& mc, size_t dir_offset, size_t num_lumps, ArchiveTreeNode* parent)
{
	if (!parent)
	{
		Log::error("ReadDir: No parent node");
		Global::error = "Archive is invalid and/or corrupt";
		return false;
	}
	mc.seek(dir_offset, SEEK_SET);
	for (uint32_t d = 0; d < num_lumps; d++)
	{
		// Update splash window progress
		UI::setSplashProgress(((float)d / (float)num_lumps));

		// Read lump info
		char     magic[4] = "";
		char     name[15] = "";
		uint32_t dumzero1, dumzero2;
		uint16_t dumff, dumze;
		uint8_t  flags  = 0;
		uint32_t offset = 0;
		uint32_t size   = 0;

		mc.read(magic, 4);   // ReS\0
		mc.read(name, 14);   // Name
		mc.read(&offset, 4); // Offset
		mc.read(&size, 4);   // Size

		// Check the identifier
		if (magic[0] != 'R' || magic[1] != 'e' || magic[2] != 'S' || magic[3] != 0)
		{
			Log::error(
				"ResArchive::readDir: Entry {} ({}@0x{:x}) has invalid directory entry", name, size, offset);
			Global::error = "Archive is invalid and/or corrupt";
			return false;
		}

		// Byteswap values for big endian if needed
		offset   = wxINT32_SWAP_ON_BE(offset);
		size     = wxINT32_SWAP_ON_BE(size);
		name[14] = '\0';

		mc.read(&dumze, 2);
		if (dumze)
			Log::info("Flag guard not null for entry {}", name);
		mc.read(&flags, 1);
		if (flags != 1 && flags != 17)
			Log::info("Unknown flag value for entry {}", name);
		mc.read(&dumzero1, 4);
		if (dumzero1)
			Log::info("Near-end values not set to zero for entry {}", name);
		mc.read(&dumff, 2);
		if (dumff != 0xFFFF)
			Log::info("Dummy set to a non-FF value for entry {}", name);
		mc.read(&dumzero2, 4);
		if (dumzero2)
			Log::info("Trailing values not set to zero for entry {}", name);

		// If the lump data goes past the end of the file,
		// the resfile is invalid
		if (offset + size > mc.size())
		{
			Log::error("ResArchive::readDirectory: Res archive is invalid or corrupt, offset overflow");
			Global::error = "Archive is invalid and/or corrupt";
			setMuted(false);
			return false;
		}

		// Create & setup lump
		auto nlump = std::make_shared<ArchiveEntry>(name, size);
		nlump->setLoaded(false);
		nlump->exProp("Offset") = (int)offset;
		nlump->setState(ArchiveEntry::State::Unmodified);

		// Read entry data if it isn't zero-sized
		if (nlump->size() > 0)
		{
			// Read the entry data
			MemChunk edata;
			mc.exportMemChunk(edata, offset, size);
			nlump->importMemChunk(edata);
		}

		// What if the entry is a directory?
		size_t d_o, n_l;
		if (isResArchive(nlump->data(), d_o, n_l))
		{
			auto ndir = createDir(name, parent);
			if (ndir)
			{
				UI::setSplashProgressMessage(fmt::format("Reading res archive data: {} directory", name));
				// Save offset to restore it once the recursion is done
				size_t myoffset = mc.currentPos();
				readDirectory(mc, d_o, n_l, ndir);
				ndir->dirEntry()->setState(ArchiveEntry::State::Unmodified);
				// Restore offset and clean out the entry
				mc.seek(myoffset, SEEK_SET);
			}
			else
				return false;
			// Not a directory, then add to entry list
		}
		else
		{
			parent->addEntry(nlump);
			// Detect entry type
			EntryType::detectEntryType(nlump.get());
			// Unload entry data if needed
			if (!archive_load_data)
				nlump->unloadData();
			// Set entry to unchanged
			nlump->setState(ArchiveEntry::State::Unmodified);
		}
	}
	return true;
}

// -----------------------------------------------------------------------------
// Reads res format data from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool ResArchive::open(MemChunk& mc)
{
	// Check data was given
	if (!mc.hasData())
		return false;

	// Read res header
	uint32_t dir_size   = 0;
	uint32_t dir_offset = 0;
	char     magic[4]   = "";
	mc.seek(0, SEEK_SET);
	mc.read(&magic, 4);      // "Res!"
	mc.read(&dir_offset, 4); // Offset to directory
	mc.read(&dir_size, 4);   // No. of lumps in res

	// Byteswap values for big endian if needed
	dir_size   = wxINT32_SWAP_ON_BE(dir_size);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);

	// Check the header
	if (magic[0] != 'R' || magic[1] != 'e' || magic[2] != 's' || magic[3] != '!')
	{
		Log::error("ResArchive::openFile: File {} has invalid header", filename_);
		Global::error = "Invalid res header";
		return false;
	}

	if (dir_size % RESDIRENTRYSIZE)
	{
		Log::error("ResArchive::openFile: File {} has invalid directory size", filename_);
		Global::error = "Invalid res directory size";
		return false;
	}
	uint32_t num_lumps = dir_size / RESDIRENTRYSIZE;

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	setMuted(true);

	// Read the directory
	mc.seek(dir_offset, SEEK_SET);
	UI::setSplashProgressMessage("Reading res archive data");
	if (!readDirectory(mc, dir_offset, num_lumps, rootDir()))
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

// -----------------------------------------------------------------------------
// Writes the res archive to a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
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
				entry->setState(ArchiveEntry::State::Unmodified);
				entry->exProp("Offset") = (int)offset;
			}
		}
	*/
	return true;
}

// -----------------------------------------------------------------------------
// Loads an entry's data from the resfile
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool ResArchive::loadEntryData(ArchiveEntry* entry)
{
	// Check the entry is valid and part of this archive
	if (!checkEntry(entry))
		return false;

	// Do nothing if the lump's size is zero,
	// or if it has already been loaded
	if (entry->size() == 0 || entry->isLoaded())
	{
		entry->setLoaded();
		return true;
	}

	// Open resfile
	wxFile file(filename_);

	// Check if opening the file failed
	if (!file.IsOpened())
	{
		Log::error("ResArchive::loadEntryData: Failed to open resfile {}", filename_);
		return false;
	}

	// Seek to lump offset in file and read it in
	file.Seek(getEntryOffset(entry), wxFromStart);
	entry->importFileStream(file, entry->size());

	// Set the lump to loaded
	entry->setLoaded();

	return true;
}

// -----------------------------------------------------------------------------
// Checks if the given data is a valid A&A res archive
// -----------------------------------------------------------------------------
bool ResArchive::isResArchive(MemChunk& mc)
{
	size_t dummy1, dummy2;
	return isResArchive(mc, dummy1, dummy2);
}
bool ResArchive::isResArchive(MemChunk& mc, size_t& dir_offset, size_t& num_lumps)
{
	// Check size
	if (mc.size() < 12)
		return false;

	// Check for "Res!" header
	if (!(mc[0] == 'R' && mc[1] == 'e' && mc[2] == 's' && mc[3] == '!'))
		return false;

	uint32_t dir_size = 0;
	mc.seek(4, SEEK_SET);
	mc.read(&dir_offset, 4);
	mc.read(&dir_size, 4);

	// Byteswap values for big endian if needed
	dir_size   = wxINT32_SWAP_ON_BE(dir_size);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);

	// A&A contains nested resource files. The offsets are then always relative to
	// the top-level file. This causes problem with the embedded archive system
	// used by SLADE3. The solution is to compute the offset offset. :)
	uint32_t offset_offset = dir_offset - (mc.size() - dir_size);
	uint32_t rel_offset    = dir_offset - offset_offset;

	// Check directory offset and size are both decent
	if (dir_size % RESDIRENTRYSIZE || (rel_offset + dir_size) > mc.size())
		return false;

	num_lumps = dir_size / RESDIRENTRYSIZE;

	// Reset MemChunk (just in case)
	mc.seek(0, SEEK_SET);

	// If it's passed to here it's probably a res file
	return true;
}

// -----------------------------------------------------------------------------
// Checks if the file at [filename] is a valid A&A res archive
// -----------------------------------------------------------------------------
bool ResArchive::isResArchive(const std::string& filename)
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
	uint32_t dir_size   = 0;
	file.Read(&dir_offset, 4);
	file.Read(&dir_size, 4);

	// Byteswap values for big endian if needed
	dir_size   = wxINT32_SWAP_ON_BE(dir_size);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);

	// Check directory offset and size are both decent
	if (dir_size % RESDIRENTRYSIZE || (dir_offset + dir_size) > file.Length())
		return false;

	// If it's passed to here it's probably a res file
	return true;
}
