
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    PakArchive.cpp
// Description: PakArchive, archive class to handle the Quake pak format
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
#include "PakArchive.h"
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
// PakArchive Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Reads pak format data from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool PakArchive::open(MemChunk& mc)
{
	// Check given data is valid
	if (mc.size() < 12)
		return false;

	// Read pak header
	char    pack[4];
	int32_t dir_offset;
	int32_t dir_size;
	mc.seek(0, SEEK_SET);
	mc.read(pack, 4);
	mc.read(&dir_offset, 4);
	mc.read(&dir_size, 4);

	// Check it
	if (pack[0] != 'P' || pack[1] != 'A' || pack[2] != 'C' || pack[3] != 'K')
	{
		log::error("PakArchive::open: Opening failed, invalid header");
		global::error = "Invalid pak header";
		return false;
	}

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	ArchiveModSignalBlocker sig_blocker{ *this };

	// Read the directory
	size_t num_entries = dir_size / 64;
	mc.seek(dir_offset, SEEK_SET);
	ui::setSplashProgressMessage("Reading pak archive data");
	for (uint32_t d = 0; d < num_entries; d++)
	{
		// Update splash window progress
		ui::setSplashProgress(((float)d / (float)num_entries));

		// Read entry info
		char    name[56];
		int32_t offset;
		int32_t size;
		mc.read(name, 56);
		mc.read(&offset, 4);
		mc.read(&size, 4);

		// Byteswap if needed
		offset = wxINT32_SWAP_ON_BE(offset);
		size   = wxINT32_SWAP_ON_BE(size);

		// Check offset+size
		if ((unsigned)(offset + size) > mc.size())
		{
			log::error("PakArchive::open: Pak archive is invalid or corrupt (entry goes past end of file)");
			global::error = "Archive is invalid and/or corrupt";
			return false;
		}

		// Create directory if needed
		auto dir = createDir(strutil::Path::pathOf(name));

		// Create entry
		auto entry              = std::make_shared<ArchiveEntry>(strutil::Path::fileNameOf(name), size);
		entry->exProp("Offset") = (int)offset;
		entry->setLoaded(false);
		entry->setState(ArchiveEntry::State::Unmodified);

		// Add to directory
		dir->addEntry(entry);
	}

	// Detect all entry types
	MemChunk              edata;
	vector<ArchiveEntry*> all_entries;
	putEntryTreeAsList(all_entries);
	ui::setSplashProgressMessage("Detecting entry types");
	for (size_t a = 0; a < all_entries.size(); a++)
	{
		// Update splash window progress
		ui::setSplashProgress((((float)a / (float)num_entries)));

		// Get entry
		auto entry = all_entries[a];

		// Read entry data if it isn't zero-sized
		if (entry->size() > 0)
		{
			// Read the entry data
			mc.exportMemChunk(edata, entry->exProp<int>("Offset"), entry->size());
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
// Writes the pak archive to a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool PakArchive::write(MemChunk& mc, bool update)
{
	// Clear current data
	mc.clear();

	// Get archive tree as a list
	vector<ArchiveEntry*> entries;
	putEntryTreeAsList(entries);

	// Process entry list
	int32_t dir_offset = 12;
	int32_t dir_size   = 0;
	for (auto& entry : entries)
	{
		// Ignore folder entries
		if (entry->type() == EntryType::folderType())
			continue;

		// Increment directory offset and size
		dir_offset += entry->size();
		dir_size += 64;
	}

	// Init data size
	mc.reSize(dir_offset + dir_size, false);

	// Write header
	char pack[4] = { 'P', 'A', 'C', 'K' };
	mc.seek(0, SEEK_SET);
	mc.write(pack, 4);
	mc.write(&dir_offset, 4);
	mc.write(&dir_size, 4);

	// Write directory
	mc.seek(dir_offset, SEEK_SET);
	int32_t offset = 12;
	for (auto& entry : entries)
	{
		// Skip folders
		if (entry->type() == EntryType::folderType())
			continue;

		// Update entry
		if (update)
		{
			entry->setState(ArchiveEntry::State::Unmodified);
			entry->exProp("Offset") = (int)offset;
		}

		// Check entry name
		auto name = entry->path(true);
		name.erase(name.begin()); // Remove leading /
		if (name.size() > 56)
		{
			log::warning(
				"Warning: Entry {} path is too long (> 56 characters), putting it in the root directory", name);
			name = strutil::Path::fileNameOf(name);
			if (name.size() > 56)
				strutil::truncateIP(name, 56);
		}

		// Write entry name
		char name_data[56];
		memset(name_data, 0, 56);
		memcpy(name_data, name.data(), name.size());
		mc.write(name_data, 56);

		// Write entry offset
		mc.write(&offset, 4);

		// Write entry size
		int32_t size = entry->size();
		mc.write(&size, 4);

		// Increment/update offset
		offset += size;
	}

	// Write entry data
	mc.seek(12, SEEK_SET);
	for (auto& entry : entries)
	{
		// Skip folders
		if (entry->type() == EntryType::folderType())
			continue;

		// Write data
		mc.write(entry->rawData(), entry->size());
	}

	return true;
}

// -----------------------------------------------------------------------------
// Loads an entry's data from the pak file
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool PakArchive::loadEntryData(ArchiveEntry* entry)
{
	// Check entry is ok
	if (!checkEntry(entry))
		return false;

	// Do nothing if the entry's size is zero,
	// or if it has already been loaded
	if (entry->size() == 0 || entry->isLoaded())
	{
		entry->setLoaded();
		return true;
	}

	// Open archive file
	wxFile file(wxString::FromUTF8(filename_));

	// Check it opened
	if (!file.IsOpened())
	{
		log::error("PakArchive::loadEntryData: Unable to open archive file {}", filename_);
		return false;
	}

	// Seek to entry offset in file and read it in
	file.Seek(entry->exProp<int>("Offset"), wxFromStart);
	entry->importFileStream(file, entry->size());

	// Set the lump to loaded
	entry->setLoaded();

	return true;
}


// -----------------------------------------------------------------------------
//
// PakArchive Class Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Checks if the given data is a valid Quake pak archive
// -----------------------------------------------------------------------------
bool PakArchive::isPakArchive(MemChunk& mc)
{
	// Check given data is valid
	if (mc.size() < 12)
		return false;

	// Read pak header
	char    pack[4];
	int32_t dir_offset;
	int32_t dir_size;
	mc.seek(0, SEEK_SET);
	mc.read(pack, 4);
	mc.read(&dir_offset, 4);
	mc.read(&dir_size, 4);

	// Byteswap values for big endian if needed
	dir_size   = wxINT32_SWAP_ON_BE(dir_size);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);

	// Check header
	if (pack[0] != 'P' || pack[1] != 'A' || pack[2] != 'C' || pack[3] != 'K')
		return false;

	// Check directory is sane
	if (dir_offset < 12 || (unsigned)(dir_offset + dir_size) > mc.size())
		return false;

	// That'll do
	return true;
}

// -----------------------------------------------------------------------------
// Checks if the file at [filename] is a valid Quake pak archive
// -----------------------------------------------------------------------------
bool PakArchive::isPakArchive(const string& filename)
{
	// Open file for reading
	wxFile file(wxString::FromUTF8(filename));

	// Check it opened ok
	if (!file.IsOpened() || file.Length() < 12)
		return false;

	// Read pak header
	char    pack[4];
	int32_t dir_offset;
	int32_t dir_size;
	file.Seek(0, wxFromStart);
	file.Read(pack, 4);
	file.Read(&dir_offset, 4);
	file.Read(&dir_size, 4);

	// Byteswap values for big endian if needed
	dir_size   = wxINT32_SWAP_ON_BE(dir_size);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);

	// Check header
	if (pack[0] != 'P' || pack[1] != 'A' || pack[2] != 'C' || pack[3] != 'K')
		return false;

	// Check directory is sane
	if (dir_offset < 12 || dir_offset + dir_size > file.Length())
		return false;

	// That'll do
	return true;
}
