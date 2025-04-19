
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    GobArchive.cpp
// Description: GobArchive, archive class to handle GOB archives from
//              Star Wars: Dark Forces
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
#include "GobArchive.h"
#include "General/UI.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, archive_load_data)


// -----------------------------------------------------------------------------
//
// GobArchive Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the file byte offset for [entry]
// -----------------------------------------------------------------------------
uint32_t GobArchive::getEntryOffset(ArchiveEntry* entry)
{
	// Check entry
	if (!checkEntry(entry))
		return 0;

	return (uint32_t)entry->exProp<int>("Offset");
}

// -----------------------------------------------------------------------------
// Sets the file byte offset for [entry]
// -----------------------------------------------------------------------------
void GobArchive::setEntryOffset(ArchiveEntry* entry, uint32_t offset)
{
	// Check entry
	if (!checkEntry(entry))
		return;

	entry->exProp("Offset") = (int)offset;
}

// -----------------------------------------------------------------------------
// Reads gob format data from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool GobArchive::open(MemChunk& mc)
{
	// Check data was given
	if (!mc.hasData())
		return false;

	// Check size
	if (mc.size() < 12)
		return false;

	// Check magic header
	if (mc[0] != 'G' || mc[1] != 'O' || mc[2] != 'B' || mc[3] != 0xA)
		return false;

	// Get directory offset
	uint32_t dir_offset = 0;
	mc.seek(4, SEEK_SET);
	mc.read(&dir_offset, 4);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);

	// Check size
	if ((unsigned)mc.size() < (dir_offset + 4))
		return false;

	// Get number of lumps
	uint32_t num_lumps = 0;
	mc.seek(dir_offset, SEEK_SET);
	mc.read(&num_lumps, 4);
	num_lumps = wxINT32_SWAP_ON_BE(num_lumps);

	// Compute directory size
	uint32_t dir_size = (num_lumps * 21) + 4;
	if ((unsigned)mc.size() < (dir_offset + dir_size))
		return false;

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	ArchiveModSignalBlocker sig_blocker{ *this };

	// Read the directory
	ui::setSplashProgressMessage("Reading gob archive data");
	for (uint32_t d = 0; d < num_lumps; d++)
	{
		// Update splash window progress
		ui::setSplashProgress(((float)d / (float)num_lumps));

		// Read lump info
		uint32_t offset   = 0;
		uint32_t size     = 0;
		char     name[13] = "";

		mc.read(&offset, 4); // Offset
		mc.read(&size, 4);   // Size
		mc.read(name, 13);   // Name
		name[12] = '\0';

		// Byteswap values for big endian if needed
		size = wxINT32_SWAP_ON_BE(size);

		// If the lump data goes past the end of the file,
		// the gobfile is invalid
		if (offset + size > mc.size())
		{
			log::error("GobArchive::open: gob archive is invalid or corrupt");
			global::error = "Archive is invalid and/or corrupt";
			return false;
		}

		// Create & setup lump
		auto nlump = std::make_shared<ArchiveEntry>(name, size);
		nlump->setLoaded(false);
		nlump->exProp("Offset") = (int)offset;
		nlump->setState(ArchiveEntry::State::Unmodified);

		// Add to entry list
		rootDir()->addEntry(nlump);
	}

	// Detect all entry types
	MemChunk edata;
	ui::setSplashProgressMessage("Detecting entry types");
	for (size_t a = 0; a < numEntries(); a++)
	{
		// Update splash window progress
		ui::setSplashProgress((((float)a / (float)num_lumps)));

		// Get entry
		auto entry = entryAt(a);

		// Read entry data if it isn't zero-sized
		if (entry->size() > 0)
		{
			// Read the entry data
			mc.exportMemChunk(edata, getEntryOffset(entry), entry->size());
			entry->importMemChunk(edata);
		}

		// Detect entry type
		EntryType::detectEntryType(*entry);

		// Unload entry data if needed
		if (!archive_load_data)
			entry->unloadData();

		// Set entry to unchanged
		entry->setState(ArchiveEntry::State::Unmodified);
	}

	// Setup variables
	sig_blocker.unblock();
	setModified(false);

	ui::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Writes the gob archive to a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool GobArchive::write(MemChunk& mc, bool update)
{
	// Determine directory offset & individual lump offsets
	uint32_t      dir_offset = 8;
	ArchiveEntry* entry;
	for (uint32_t l = 0; l < numEntries(); l++)
	{
		entry = entryAt(l);
		setEntryOffset(entry, dir_offset);
		dir_offset += entry->size();
	}

	// Clear/init MemChunk
	mc.clear();
	mc.seek(0, SEEK_SET);
	mc.reSize(dir_offset + 4 + numEntries() * 21);

	// Write the header
	uint32_t num_lumps = wxINT32_SWAP_ON_BE(numEntries());
	dir_offset         = wxINT32_SWAP_ON_BE(dir_offset);
	char header[4]     = { 'G', 'O', 'B', 0xA };
	mc.write(header, 4);
	mc.write(&dir_offset, 4);

	// Write the lumps
	for (uint32_t l = 0; l < numEntries(); l++)
	{
		entry = entryAt(l);
		mc.write(entry->rawData(), entry->size());
	}

	// Write the directory
	mc.write(&num_lumps, 4);
	for (uint32_t l = 0; l < numEntries(); l++)
	{
		entry         = entryAt(l);
		char name[13] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		long offset   = wxINT32_SWAP_ON_BE(getEntryOffset(entry));
		long size     = wxINT32_SWAP_ON_BE(entry->size());

		for (size_t c = 0; c < entry->name().length() && c < 13; c++)
			name[c] = entry->name()[c];

		mc.write(&offset, 4);
		mc.write(&size, 4);
		mc.write(name, 13);

		if (update)
		{
			entry->setState(ArchiveEntry::State::Unmodified);
			entry->exProp("Offset") = (int)offset;
		}
	}

	return true;
}

// -----------------------------------------------------------------------------
// Loads an entry's data from the gobfile
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool GobArchive::loadEntryData(ArchiveEntry* entry)
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

	// Open gobfile
	wxFile file(wxString::FromUTF8(filename_));

	// Check if opening the file failed
	if (!file.IsOpened())
	{
		log::error("GobArchive::loadEntryData: Failed to open gobfile {}", filename_);
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
// Checks if the given data is a valid Dark Forces gob archive
// -----------------------------------------------------------------------------
bool GobArchive::isGobArchive(MemChunk& mc)
{
	// Check size
	if (mc.size() < 12)
		return false;

	// Check magic header
	if (mc[0] != 'G' || mc[1] != 'O' || mc[2] != 'B' || mc[3] != 0xA)
		return false;

	// Get directory offset
	uint32_t dir_offset = 0;
	mc.seek(4, SEEK_SET);
	mc.read(&dir_offset, 4);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);

	// Check size
	if ((unsigned)mc.size() < (dir_offset + 4))
		return false;

	// Get number of lumps
	uint32_t num_lumps = 0;
	mc.seek(dir_offset, SEEK_SET);
	mc.read(&num_lumps, 4);
	num_lumps = wxINT32_SWAP_ON_BE(num_lumps);

	// Compute directory size
	uint32_t dir_size = (num_lumps * 21) + 4;
	if ((unsigned)mc.size() < (dir_offset + dir_size))
		return false;

	// If it's passed to here it's probably a gob file
	return true;
}

// -----------------------------------------------------------------------------
// Checks if the file at [filename] is a valid Dark Forces gob archive
// -----------------------------------------------------------------------------
bool GobArchive::isGobArchive(const string& filename)
{
	// Open file for reading
	wxFile file(wxString::FromUTF8(filename));

	// Check it opened ok
	if (!file.IsOpened())
		return false;

	// Check size
	if (file.Length() < 12)
		return false;

	// Read header
	char header[4];
	file.Read(header, 4);

	// Check magic header
	if (header[0] != 'G' || header[1] != 'O' || header[2] != 'B' || header[3] != 0xA)
		return false;

	// Get directory offset
	uint32_t dir_offset = 0;
	file.Seek(4, wxFromStart);
	file.Read(&dir_offset, 4);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);

	// Check size
	if ((unsigned)file.Length() < (dir_offset + 4))
		return false;

	// Get number of lumps
	uint32_t num_lumps = 0;
	file.Seek(dir_offset, wxFromStart);
	file.Read(&num_lumps, 4);
	num_lumps = wxINT32_SWAP_ON_BE(num_lumps);

	// Compute directory size
	uint32_t dir_size = (num_lumps * 21) + 4;
	if ((unsigned)file.Length() < (dir_offset + dir_size))
		return false;

	// If it's passed to here it's probably a gob file
	return true;
}
