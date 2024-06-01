
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Wad2ArchiveHandler.cpp
// Description: ArchiveFormatHandler for the Quake WAD2 format, which is also
//              the same as the Half-Life WAD3 format except for one character
//              in the header.
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
#include "Wad2ArchiveHandler.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "UI/UI.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Wad2ArchiveHandler Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Reads wad format data from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool Wad2ArchiveHandler::open(Archive& archive, const MemChunk& mc)
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
		log::error("Wad2ArchiveHandler::open: Invalid header");
		global::error = "Invalid wad2 header";
		return false;
	}
	if (wad_type[3] == '3')
		wad3_ = true;

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	ArchiveModSignalBlocker sig_blocker{ archive };

	// Read the directory
	mc.seek(dir_offset, SEEK_SET);
	ui::setSplashProgressMessage("Reading wad archive data");
	for (uint32_t d = 0; d < num_lumps; d++)
	{
		// Update splash window progress
		ui::setSplashProgress(d, num_lumps);

		// Read lump info
		Wad2Entry info;
		mc.read(&info, 32);

		// Byteswap values for big endian if needed
		info.offset = wxINT32_SWAP_ON_BE(info.offset);
		info.size   = wxINT32_SWAP_ON_BE(info.size);
		info.dsize  = wxINT32_SWAP_ON_BE(info.dsize);

		// If the lump data goes past the end of the file,
		// the wadfile is invalid
		if (static_cast<unsigned>(info.offset + info.dsize) > mc.size())
		{
			log::error("Wad2ArchiveHandler::open: Wad2 archive is invalid or corrupt");
			global::error = "Archive is invalid and/or corrupt";
			return false;
		}

		// Create & setup lump
		auto nlump = std::make_shared<ArchiveEntry>(info.name, info.dsize);
		nlump->setOffsetOnDisk(info.offset);
		nlump->setSizeOnDisk();
		nlump->exProp("W2Type") = info.type;
		nlump->exProp("W2Size") = (int)info.size;
		nlump->exProp("W2Comp") = !!(info.cmprs);

		// Read entry data if it isn't zero-sized
		if (nlump->size() > 0)
			nlump->importMemChunk(mc, info.offset, info.dsize);

		nlump->setState(EntryState::Unmodified);

		// Add to entry list
		archive.rootDir()->addEntry(nlump);
	}

	// Detect all entry types
	detectAllEntryTypes(archive);

	// Setup variables
	sig_blocker.unblock();
	archive.setModified(false);

	ui::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Writes the wad archive to a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool Wad2ArchiveHandler::write(Archive& archive, MemChunk& mc)
{
	// Determine directory offset & individual lump offsets
	uint32_t      dir_offset = 12;
	ArchiveEntry* entry      = nullptr;
	for (uint32_t l = 0; l < archive.numEntries(); l++)
	{
		entry = archive.entryAt(l);
		entry->setOffsetOnDisk(dir_offset);
		dir_offset += entry->size();
	}

	// Clear/init MemChunk
	mc.clear();
	mc.seek(0, SEEK_SET);
	mc.reSize(dir_offset + archive.numEntries() * 32);

	// Setup wad type
	char wad_type[4] = { 'W', 'A', 'D', '2' };
	if (wad3_)
		wad_type[3] = '3';

	// Write the header
	uint32_t num_lumps = archive.numEntries();
	mc.write(wad_type, 4);
	mc.write(&num_lumps, 4);
	mc.write(&dir_offset, 4);

	// Write the lumps
	for (uint32_t l = 0; l < num_lumps; l++)
	{
		entry = archive.entryAt(l);
		mc.write(entry->rawData(), entry->size());
	}

	// Write the directory
	for (uint32_t l = 0; l < num_lumps; l++)
	{
		entry = archive.entryAt(l);

		// Setup directory entry
		Wad2Entry info;
		memset(info.name, 0, 16);
		memcpy(info.name, entry->name().data(), entry->name().size());
		info.cmprs  = entry->exProp<bool>("W2Comp");
		info.dsize  = entry->size();
		info.size   = entry->size();
		info.offset = entry->offsetOnDisk();
		info.type   = entry->exProp<int>("W2Type");

		// Write it
		mc.write(&info, 32);

		entry->setSizeOnDisk();
		entry->setState(EntryState::Unmodified);
	}

	return true;
}


// -----------------------------------------------------------------------------
//
// Wad2ArchiveHandler Class Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Checks if the given data is a valid Quake wad2 archive
// -----------------------------------------------------------------------------
bool Wad2ArchiveHandler::isThisFormat(const MemChunk& mc)
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
	if (static_cast<unsigned>(dir_offset + (num_lumps * 32)) > mc.size() || dir_offset < 12)
		return false;

	// If it's passed to here it's probably a wad2 file
	return true;
}

// -----------------------------------------------------------------------------
// Checks if the file at [filename] is a valid Quake wad2 archive
// -----------------------------------------------------------------------------
bool Wad2ArchiveHandler::isThisFormat(const string& filename)
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
