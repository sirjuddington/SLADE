
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    GobArchiveHandler.cpp
// Description: ArchiveFormatHandler for Star Wars: Dark Forces GOB archives
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
#include "GobArchiveHandler.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "General/UI.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// GobArchiveHandler Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Reads gob format data from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool GobArchiveHandler::open(Archive& archive, const MemChunk& mc, bool detect_types)
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
	if (mc.size() < (dir_offset + 4))
		return false;

	// Get number of lumps
	uint32_t num_lumps = 0;
	mc.seek(dir_offset, SEEK_SET);
	mc.read(&num_lumps, 4);
	num_lumps = wxINT32_SWAP_ON_BE(num_lumps);

	// Compute directory size
	uint32_t dir_size = (num_lumps * 21) + 4;
	if (mc.size() < (dir_offset + dir_size))
		return false;

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	ArchiveModSignalBlocker sig_blocker{ archive };

	// Read the directory
	ui::setSplashProgressMessage("Reading gob archive data");
	for (uint32_t d = 0; d < num_lumps; d++)
	{
		// Update splash window progress
		ui::setSplashProgress(d, num_lumps);

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
			log::error("GobArchiveHandler::open: gob archive is invalid or corrupt");
			global::error = "Archive is invalid and/or corrupt";
			return false;
		}

		// Create & setup lump
		auto nlump = std::make_shared<ArchiveEntry>(name, size);
		nlump->setOffsetOnDisk(offset);
		nlump->setSizeOnDisk(size);

		// Read entry data if it isn't zero-sized
		if (nlump->size() > 0)
			nlump->importMemChunk(mc, offset, size);

		nlump->setState(EntryState::Unmodified);

		// Add to entry list
		archive.rootDir()->addEntry(nlump);
	}

	// Detect all entry types
	if (detect_types)
		archive.detectAllEntryTypes();

	// Setup variables
	sig_blocker.unblock();
	archive.setModified(false);

	ui::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Writes the gob archive to a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool GobArchiveHandler::write(Archive& archive, MemChunk& mc)
{
	// Determine directory offset & individual lump offsets
	uint32_t      dir_offset = 8;
	ArchiveEntry* entry;
	for (uint32_t l = 0; l < archive.numEntries(); l++)
	{
		entry = archive.entryAt(l);
		entry->setOffsetOnDisk(dir_offset);
		dir_offset += entry->size();
	}

	// Clear/init MemChunk
	mc.clear();
	mc.seek(0, SEEK_SET);
	mc.reSize(dir_offset + 4 + archive.numEntries() * 21);

	// Write the header
	uint32_t num_lumps = wxINT32_SWAP_ON_BE(archive.numEntries());
	dir_offset         = wxINT32_SWAP_ON_BE(dir_offset);
	char header[4]     = { 'G', 'O', 'B', 0xA };
	mc.write(header, 4);
	mc.write(&dir_offset, 4);

	// Write the lumps
	for (uint32_t l = 0; l < archive.numEntries(); l++)
	{
		entry = archive.entryAt(l);
		mc.write(entry->rawData(), entry->size());
	}

	// Write the directory
	mc.write(&num_lumps, 4);
	for (uint32_t l = 0; l < archive.numEntries(); l++)
	{
		entry         = archive.entryAt(l);
		char name[13] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		long offset   = wxINT32_SWAP_ON_BE(entry->offsetOnDisk());
		long size     = wxINT32_SWAP_ON_BE(entry->size());

		for (size_t c = 0; c < entry->name().length() && c < 13; c++)
			name[c] = entry->name()[c];

		mc.write(&offset, 4);
		mc.write(&size, 4);
		mc.write(name, 13);

		entry->setState(EntryState::Unmodified);
		entry->setOffsetOnDisk(offset);
		entry->setSizeOnDisk();
	}

	return true;
}

// -----------------------------------------------------------------------------
// Checks if the given data is a valid Dark Forces gob archive
// -----------------------------------------------------------------------------
bool GobArchiveHandler::isThisFormat(const MemChunk& mc)
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
bool GobArchiveHandler::isThisFormat(const string& filename)
{
	// Open file for reading
	wxFile file(filename);

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
	if (static_cast<unsigned>(file.Length()) < (dir_offset + 4))
		return false;

	// Get number of lumps
	uint32_t num_lumps = 0;
	file.Seek(dir_offset, wxFromStart);
	file.Read(&num_lumps, 4);
	num_lumps = wxINT32_SWAP_ON_BE(num_lumps);

	// Compute directory size
	uint32_t dir_size = (num_lumps * 21) + 4;
	if (static_cast<unsigned>(file.Length()) < (dir_offset + dir_size))
		return false;

	// If it's passed to here it's probably a gob file
	return true;
}
