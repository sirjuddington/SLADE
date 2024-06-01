
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    LfdArchiveHandler.cpp
// Description: ArchiveFormatHandler for Star Wars: Dark Forces LFD archives
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
#include "LfdArchiveHandler.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "UI/UI.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// LfdArchiveHandler Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Reads lfd format data from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool LfdArchiveHandler::open(Archive& archive, const MemChunk& mc)
{
	// Check data was given
	if (!mc.hasData())
		return false;

	// Check size
	if (mc.size() < 16)
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
	if (mc.size() < (dir_len) || dir_len % 16)
		return false;

	// Guess number of lumps
	uint32_t num_lumps = dir_len / 16;

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	ArchiveModSignalBlocker sig_blocker{ archive };

	// Read each entry
	ui::setSplashProgressMessage("Reading lfd archive data");
	size_t offset = dir_len + 16;
	size_t size   = mc.size();
	for (uint32_t d = 0; offset < size; d++)
	{
		// Update splash window progress
		ui::setSplashProgress(d, num_lumps);

		// Read lump info
		uint32_t length  = 0;
		char     type[5] = "";
		char     name[9] = "";

		mc.read(type, 4);    // Type
		mc.read(name, 8);    // Name
		mc.read(&length, 4); // Size
		name[8] = '\0';
		type[4] = 0;

		// Move past the header
		offset += 16;

		// Byteswap values for big endian if needed
		length = wxINT32_SWAP_ON_BE(length);

		// If the lump data goes past the end of the file,
		// the gobfile is invalid
		if (offset + length > size)
		{
			log::error("LfdArchiveHandler::open: lfd archive is invalid or corrupt");
			global::error = "Archive is invalid and/or corrupt";
			return false;
		}

		// Create & setup lump
		strutil::Path fn(name);
		fn.setExtension(type);
		auto nlump = std::make_shared<ArchiveEntry>(fn.fileName(), length);
		nlump->setOffsetOnDisk(offset);
		nlump->setSizeOnDisk(length);

		// Read entry data if it isn't zero-sized
		if (nlump->size() > 0)
			nlump->importMemChunk(mc, offset, size);

		nlump->setState(EntryState::Unmodified);

		// Add to entry list
		archive.rootDir()->addEntry(nlump);

		// Move to next entry
		offset += length;
		mc.seek(offset, SEEK_SET);
	}

	if (num_lumps != archive.numEntries())
		log::warning("Computed {} lumps, but actually {} entries", num_lumps, archive.numEntries());

	// Detect all entry types
	detectAllEntryTypes(archive);

	// Setup variables
	sig_blocker.unblock();
	archive.setModified(false);

	ui::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Writes the lfd archive to a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool LfdArchiveHandler::write(Archive& archive, MemChunk& mc)
{
	// Determine total size
	uint32_t      dir_size   = (archive.numEntries() + 1) << 4;
	uint32_t      total_size = dir_size;
	ArchiveEntry* entry;
	for (uint32_t l = 0; l < archive.numEntries(); l++)
	{
		entry = archive.entryAt(l);
		total_size += 16;
		entry->setState(EntryState::Unmodified);
		entry->setOffsetOnDisk(total_size);
		entry->setSizeOnDisk();
		total_size += entry->size();
	}

	// Clear/init MemChunk
	mc.clear();
	mc.seek(0, SEEK_SET);
	mc.reSize(total_size);

	// Variables
	char   type[5] = "RMAP";
	char   name[9] = "resource";
	size_t size    = wxINT32_SWAP_ON_BE(archive.numEntries() << 4);


	// Write the resource map first
	mc.write(type, 4);
	mc.write(name, 8);
	mc.write(&size, 4);
	for (uint32_t l = 0; l < archive.numEntries(); l++)
	{
		entry = archive.entryAt(l);
		for (char& t : type)
			t = 0;
		for (char& n : name)
			n = 0;
		size = wxINT32_SWAP_ON_BE(entry->size());
		wxFileName fn(entry->name());

		for (size_t c = 0; c < fn.GetName().length() && c < 9; c++)
			name[c] = fn.GetName()[c];
		for (size_t c = 0; c < fn.GetExt().length() && c < 5; c++)
			type[c] = fn.GetExt()[c];

		mc.write(type, 4);
		mc.write(name, 8);
		mc.write(&size, 4);
	}

	// Write the lumps
	for (uint32_t l = 0; l < archive.numEntries(); l++)
	{
		entry = archive.entryAt(l);
		for (char& t : type)
			t = 0;
		for (char& n : name)
			n = 0;
		size = wxINT32_SWAP_ON_BE(entry->size());
		wxFileName fn(entry->name());

		for (size_t c = 0; c < fn.GetName().length() && c < 9; c++)
			name[c] = fn.GetName()[c];
		for (size_t c = 0; c < fn.GetExt().length() && c < 5; c++)
			type[c] = fn.GetExt()[c];

		mc.write(type, 4);
		mc.write(name, 8);
		mc.write(&size, 4);
		mc.write(entry->rawData(), entry->size());
	}

	return true;
}

// -----------------------------------------------------------------------------
// Checks if the given data is a valid Dark Forces lfd archive
// -----------------------------------------------------------------------------
bool LfdArchiveHandler::isThisFormat(const MemChunk& mc)
{
	// Check size
	if (mc.size() < 12)
		return false;

	// Check magic header
	if (mc[0] != 'R' || mc[1] != 'M' || mc[2] != 'A' || mc[3] != 'P')
		return false;

	// Get offset of first entry
	uint32_t dir_offset = 0;
	mc.seek(12, SEEK_SET);
	mc.read(&dir_offset, 4);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset) + 16;
	if (dir_offset % 16)
		return false;
	char     type1[5];
	char     type2[5];
	char     name1[9];
	char     name2[9];
	uint32_t len1;
	uint32_t len2;
	mc.read(type1, 4);
	type1[4] = 0;
	mc.read(name1, 8);
	name1[8] = 0;
	mc.read(&len1, 4);
	len1 = wxINT32_SWAP_ON_BE(len1);

	// Check size
	if ((unsigned)mc.size() < (dir_offset + 16 + len1))
		return false;

	// Compare
	mc.seek(dir_offset, SEEK_SET);
	mc.read(type2, 4);
	type2[4] = 0;
	mc.read(name2, 8);
	name2[8] = 0;
	mc.read(&len2, 4);
	len2 = wxINT32_SWAP_ON_BE(len2);

	if (strcmp(type1, type2) != 0 || strcmp(name1, name2) != 0 || len1 != len2)
		return false;

	// If it's passed to here it's probably a lfd file
	return true;
}

// -----------------------------------------------------------------------------
// Checks if the file at [filename] is a valid Dark Forces lfd archive
// -----------------------------------------------------------------------------
bool LfdArchiveHandler::isThisFormat(const string& filename)
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
	if (dir_offset % 16)
		return false;
	char     type1[5];
	char     type2[5];
	char     name1[9];
	char     name2[9];
	uint32_t len1;
	uint32_t len2;
	file.Read(type1, 4);
	type1[4] = 0;
	file.Read(name1, 8);
	name1[8] = 0;
	file.Read(&len1, 4);
	len1 = wxINT32_SWAP_ON_BE(len1);

	// Check size
	if (static_cast<unsigned>(file.Length()) < (dir_offset + 16 + len1))
		return false;

	// Compare
	file.Seek(dir_offset, wxFromStart);
	file.Read(type2, 4);
	type2[4] = 0;
	file.Read(name2, 8);
	name2[8] = 0;
	file.Read(&len2, 4);
	len2 = wxINT32_SWAP_ON_BE(len2);

	if (strcmp(type1, type2) != 0 || strcmp(name1, name2) != 0 || len1 != len2)
		return false;

	// If it's passed to here it's probably a lfd file
	return true;
}
