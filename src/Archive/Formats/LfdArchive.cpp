
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    LfdArchive.cpp
// Description: LfdArchive, archive class to handle LFD archives from
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
#include "LfdArchive.h"
#include "General/UI.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, archive_load_data)


// -----------------------------------------------------------------------------
//
// LfdArchive Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the file byte offset for [entry]
// -----------------------------------------------------------------------------
uint32_t LfdArchive::getEntryOffset(ArchiveEntry* entry)
{
	// Check entry
	if (!checkEntry(entry))
		return 0;

	return (uint32_t)entry->exProp<int>("Offset");
}

// -----------------------------------------------------------------------------
// Sets the file byte offset for [entry]
// -----------------------------------------------------------------------------
void LfdArchive::setEntryOffset(ArchiveEntry* entry, uint32_t offset)
{
	// Check entry
	if (!checkEntry(entry))
		return;

	entry->exProp("Offset") = (int)offset;
}

// -----------------------------------------------------------------------------
// Reads lfd format data from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool LfdArchive::open(MemChunk& mc)
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
	if ((unsigned)mc.size() < (dir_len) || dir_len % 16)
		return false;

	// Guess number of lumps
	uint32_t num_lumps = dir_len / 16;

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	ArchiveModSignalBlocker sig_blocker{ *this };

	// Read each entry
	ui::setSplashProgressMessage("Reading lfd archive data");
	size_t offset = dir_len + 16;
	size_t size   = mc.size();
	for (uint32_t d = 0; offset < size; d++)
	{
		// Update splash window progress
		ui::setSplashProgress(((float)d / (float)num_lumps));

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
			log::error("LfdArchive::open: lfd archive is invalid or corrupt");
			global::error = "Archive is invalid and/or corrupt";
			return false;
		}

		// Create & setup lump
		strutil::Path fn(name);
		fn.setExtension(type);
		auto nlump = std::make_shared<ArchiveEntry>(fn.fileName(), length);
		nlump->setLoaded(false);
		nlump->exProp("Offset") = (int)offset;
		nlump->setState(ArchiveEntry::State::Unmodified);

		// Add to entry list
		rootDir()->addEntry(nlump);

		// Move to next entry
		offset += length;
		mc.seek(offset, SEEK_SET);
	}

	if (num_lumps != numEntries())
		log::warning("Computed {} lumps, but actually {} entries", num_lumps, numEntries());

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
// Writes the lfd archive to a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool LfdArchive::write(MemChunk& mc, bool update)
{
	// Determine total size
	uint32_t      dir_size   = (numEntries() + 1) << 4;
	uint32_t      total_size = dir_size;
	ArchiveEntry* entry;
	for (uint32_t l = 0; l < numEntries(); l++)
	{
		entry = entryAt(l);
		total_size += 16;
		setEntryOffset(entry, total_size);
		if (update)
		{
			entry->setState(ArchiveEntry::State::Unmodified);
			entry->exProp("Offset") = (int)total_size;
		}
		total_size += entry->size();
	}

	// Clear/init MemChunk
	mc.clear();
	mc.seek(0, SEEK_SET);
	mc.reSize(total_size);

	// Variables
	char   type[5] = "RMAP";
	char   name[9] = "resource";
	size_t size    = wxINT32_SWAP_ON_BE(numEntries() << 4);


	// Write the resource map first
	mc.write(type, 4);
	mc.write(name, 8);
	mc.write(&size, 4);
	for (uint32_t l = 0; l < numEntries(); l++)
	{
		entry = entryAt(l);
		for (char& t : type)
			t = 0;
		for (char& n : name)
			n = 0;
		size = wxINT32_SWAP_ON_BE(entry->size());
		wxFileName fn(wxString::FromUTF8(entry->name()));

		for (size_t c = 0; c < fn.GetName().length() && c < 9; c++)
			name[c] = fn.GetName()[c];
		for (size_t c = 0; c < fn.GetExt().length() && c < 5; c++)
			type[c] = fn.GetExt()[c];

		mc.write(type, 4);
		mc.write(name, 8);
		mc.write(&size, 4);
	}

	// Write the lumps
	for (uint32_t l = 0; l < numEntries(); l++)
	{
		entry = entryAt(l);
		for (char& t : type)
			t = 0;
		for (char& n : name)
			n = 0;
		size = wxINT32_SWAP_ON_BE(entry->size());
		wxFileName fn(wxString::FromUTF8(entry->name()));

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
// Loads an entry's data from the lfdfile
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool LfdArchive::loadEntryData(ArchiveEntry* entry)
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

	// Open lfdfile
	wxFile file(wxString::FromUTF8(filename_));

	// Check if opening the file failed
	if (!file.IsOpened())
	{
		log::error("LfdArchive::loadEntryData: Failed to open lfdfile {}", filename_);
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
// Checks if the given data is a valid Dark Forces lfd archive
// -----------------------------------------------------------------------------
bool LfdArchive::isLfdArchive(MemChunk& mc)
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
bool LfdArchive::isLfdArchive(const string& filename)
{
	// Open file for reading
	wxFile file(wxString::FromUTF8(filename));

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
	if ((unsigned)file.Length() < (dir_offset + 16 + len1))
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
