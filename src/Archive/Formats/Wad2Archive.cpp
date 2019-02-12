
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Wad2Archive.cpp
// Description: Wad2Archive, archive class to handle the Quake wad2 format,
//              which is also the same as the Half-Life wad3 format except for
//              one character in the header.
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
#include "Wad2Archive.h"
#include "General/UI.h"


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, archive_load_data)


// -----------------------------------------------------------------------------
//
// Wad2Archive Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Reads wad format data from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool Wad2Archive::open(MemChunk& mc)
{
	// Check data was given
	if (!mc.hasData())
		return false;

	// Read wad header
	uint32_t num_lumps   = 0;
	uint32_t dir_offset  = 0;
	char     wad_type[4] = "";
	mc.seek(0, SEEK_SET);
	mc.read(&wad_type, 4);   // Wad type
	mc.read(&num_lumps, 4);  // No. of lumps in wad
	mc.read(&dir_offset, 4); // Offset to directory

	// Byteswap values for big endian if needed
	num_lumps  = wxINT32_SWAP_ON_BE(num_lumps);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);

	// Check the header
	if (wad_type[0] != 'W' || wad_type[1] != 'A' || wad_type[2] != 'D' || (wad_type[3] != '2' && wad_type[3] != '3'))
	{
		Log::error("Wad2Archive::open: Invalid header");
		Global::error = "Invalid wad2 header";
		return false;
	}
	if (wad_type[3] == '3')
		wad3_ = true;

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	setMuted(true);

	// Read the directory
	mc.seek(dir_offset, SEEK_SET);
	UI::setSplashProgressMessage("Reading wad archive data");
	for (uint32_t d = 0; d < num_lumps; d++)
	{
		// Update splash window progress
		UI::setSplashProgress(((float)d / (float)num_lumps));

		// Read lump info
		Wad2Entry info;
		mc.read(&info, 32);

		// Byteswap values for big endian if needed
		info.offset = wxINT32_SWAP_ON_BE(info.offset);
		info.size   = wxINT32_SWAP_ON_BE(info.size);
		info.dsize  = wxINT32_SWAP_ON_BE(info.dsize);

		// If the lump data goes past the end of the file,
		// the wadfile is invalid
		if ((unsigned)(info.offset + info.dsize) > mc.size())
		{
			Log::error("Wad2Archive::open: Wad2 archive is invalid or corrupt");
			Global::error = "Archive is invalid and/or corrupt";
			setMuted(false);
			return false;
		}

		// Create & setup lump
		auto nlump = std::make_shared<ArchiveEntry>(info.name, info.dsize);
		nlump->setLoaded(false);
		nlump->exProp("Offset") = (int)info.offset;
		nlump->exProp("W2Type") = info.type;
		nlump->exProp("W2Size") = (int)info.size;
		nlump->exProp("W2Comp") = !!(info.cmprs);
		nlump->setState(ArchiveEntry::State::Unmodified);

		// Add to entry list
		rootDir()->addEntry(nlump);
	}

	// Detect all entry types
	MemChunk edata;
	UI::setSplashProgressMessage("Detecting entry types");
	for (size_t a = 0; a < numEntries(); a++)
	{
		// Update splash window progress
		UI::setSplashProgress((((float)a / (float)num_lumps)));

		// Get entry
		auto entry = entryAt(a);

		// Read entry data if it isn't zero-sized
		if (entry->size() > 0)
		{
			// Read the entry data
			mc.exportMemChunk(edata, (int)entry->exProp("Offset"), entry->size());
			entry->importMemChunk(edata);
		}

		// Detect entry type
		EntryType::detectEntryType(entry);

		// Unload entry data if needed
		if (!archive_load_data)
			entry->unloadData();

		// Set entry to unchanged
		entry->setState(ArchiveEntry::State::Unmodified);
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

// -----------------------------------------------------------------------------
// Writes the wad archive to a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool Wad2Archive::write(MemChunk& mc, bool update)
{
	// Determine directory offset & individual lump offsets
	uint32_t      dir_offset = 12;
	ArchiveEntry* entry      = nullptr;
	for (uint32_t l = 0; l < numEntries(); l++)
	{
		entry                   = entryAt(l);
		entry->exProp("Offset") = (int)dir_offset;
		dir_offset += entry->size();
	}

	// Clear/init MemChunk
	mc.clear();
	mc.seek(0, SEEK_SET);
	mc.reSize(dir_offset + numEntries() * 32);

	// Setup wad type
	char wad_type[4] = { 'W', 'A', 'D', '2' };
	if (wad3_)
		wad_type[3] = '3';

	// Write the header
	uint32_t num_lumps = numEntries();
	mc.write(wad_type, 4);
	mc.write(&num_lumps, 4);
	mc.write(&dir_offset, 4);

	// Write the lumps
	for (uint32_t l = 0; l < num_lumps; l++)
	{
		entry = entryAt(l);
		mc.write(entry->rawData(), entry->size());
	}

	// Write the directory
	for (uint32_t l = 0; l < num_lumps; l++)
	{
		entry = entryAt(l);

		// Setup directory entry
		Wad2Entry info;
		memset(info.name, 0, 16);
		memcpy(info.name, entry->name().data(), entry->name().size());
		info.cmprs  = (bool)entry->exProp("W2Comp");
		info.dsize  = entry->size();
		info.size   = entry->size();
		info.offset = (int)entry->exProp("Offset");
		info.type   = (int)entry->exProp("W2Type");

		// Write it
		mc.write(&info, 32);

		if (update)
			entry->setState(ArchiveEntry::State::Unmodified);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Loads an entry's data from the wadfile
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool Wad2Archive::loadEntryData(ArchiveEntry* entry)
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

	// Open wadfile
	wxFile file(filename_);

	// Check if opening the file failed
	if (!file.IsOpened())
	{
		Log::error(wxString::Format("Wad2Archive::loadEntryData: Failed to open wadfile %s", filename_));
		return false;
	}

	// Seek to lump offset in file and read it in
	file.Seek((int)entry->exProp("Offset"), wxFromStart);
	entry->importFileStream(file, entry->size());

	// Set the lump to loaded
	entry->setLoaded();

	return true;
}


// -----------------------------------------------------------------------------
//
// Wad2Archive Class Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Checks if the given data is a valid Quake wad2 archive
// -----------------------------------------------------------------------------
bool Wad2Archive::isWad2Archive(MemChunk& mc)
{
	// Check size
	if (mc.size() < 12)
		return false;

	// Check for IWAD/PWAD header
	if (mc[0] != 'W' || mc[1] != 'A' || mc[2] != 'D' || (mc[3] != '2' && mc[3] != '3'))
		return false;

	// Get number of lumps and directory offset
	int32_t num_lumps  = 0;
	int32_t dir_offset = 0;
	mc.seek(4, SEEK_SET);
	mc.read(&num_lumps, 4);
	mc.read(&dir_offset, 4);

	// Reset MemChunk (just in case)
	mc.seek(0, SEEK_SET);

	// Byteswap values for big endian if needed
	num_lumps  = wxINT32_SWAP_ON_BE(num_lumps);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);

	// Check directory offset is decent
	if ((unsigned)(dir_offset + (num_lumps * 32)) > mc.size() || dir_offset < 12)
		return false;

	// If it's passed to here it's probably a wad2 file
	return true;
}

// -----------------------------------------------------------------------------
// Checks if the file at [filename] is a valid Quake wad2 archive
// -----------------------------------------------------------------------------
bool Wad2Archive::isWad2Archive(const wxString& filename)
{
	// Open file for reading
	wxFile file(filename);

	// Check it opened ok
	if (!file.IsOpened())
		return false;

	// Read header
	char header[4];
	file.Read(header, 4);

	// Check for IWAD/PWAD header
	if (header[0] != 'W' || header[1] != 'A' || header[2] != 'D' || (header[3] != '2' && header[3] != '3'))
		return false;

	// Get number of lumps and directory offset
	int32_t num_lumps  = 0;
	int32_t dir_offset = 0;
	file.Read(&num_lumps, 4);
	file.Read(&dir_offset, 4);

	// Byteswap values for big endian if needed
	num_lumps  = wxINT32_SWAP_ON_BE(num_lumps);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);

	// Check directory offset is decent
	if ((dir_offset + (num_lumps * 32)) > file.Length() || dir_offset < 12)
		return false;

	// If it's passed to here it's probably a wad file
	return true;
}
