
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SiNArchive.cpp
// Description: SiNArchive, archive class to handle the Ritual Entertainment SiN
//              format, a variant on Quake 2 pak files.
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
#include "SiNArchive.h"
#include "General/UI.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// SiNArchive Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Reads SiN format data from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool SiNArchive::open(const MemChunk& mc, bool detect_types)
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
	if (pack[0] != 'S' || pack[1] != 'P' || pack[2] != 'A' || pack[3] != 'K')
	{
		log::error("SiNArchive::open: Opening failed, invalid header");
		global::error = "Invalid pak header";
		return false;
	}

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	ArchiveModSignalBlocker sig_blocker{ *this };

	// Read the directory
	size_t num_entries = dir_size / 128;
	mc.seek(dir_offset, SEEK_SET);
	ui::setSplashProgressMessage("Reading SiN archive data");
	for (uint32_t d = 0; d < num_entries; d++)
	{
		// Update splash window progress
		ui::setSplashProgress(d, num_entries);

		// Read entry info
		char    name[120];
		int32_t offset;
		int32_t size;
		mc.read(name, 120);
		mc.read(&offset, 4);
		mc.read(&size, 4);

		// Byteswap if needed
		offset = wxINT32_SWAP_ON_BE(offset);
		size   = wxINT32_SWAP_ON_BE(size);

		// Check offset+size
		if ((unsigned)(offset + size) > mc.size())
		{
			log::error("SiNArchive::open: SiN archive is invalid or corrupt (entry goes past end of file)");
			global::error = "Archive is invalid and/or corrupt";
			return false;
		}

		// Create directory if needed
		auto dir = createDir(strutil::Path::pathOf(name));

		// Create entry
		auto entry = std::make_shared<ArchiveEntry>(strutil::Path::fileNameOf(name), size);
		entry->setOffsetOnDisk(offset);
		entry->setSizeOnDisk();

		// Read entry data if it isn't zero-sized
		if (entry->size() > 0)
			entry->importMemChunk(mc, offset, size);

		entry->setState(ArchiveEntry::State::Unmodified);

		// Add to directory
		dir->addEntry(entry);
	}

	// Detect all entry types
	if (detect_types)
		detectAllEntryTypes();

	// Setup variables
	sig_blocker.unblock();
	setModified(false);

	ui::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Writes the SiN archive to a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool SiNArchive::write(MemChunk& mc)
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
		dir_size += 128;
	}

	// Init data size
	mc.reSize(dir_offset + dir_size, false);

	// Write header
	char pack[4] = { 'S', 'P', 'A', 'K' };
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
		entry->setState(ArchiveEntry::State::Unmodified);
		entry->setOffsetOnDisk(offset);
		entry->setSizeOnDisk();

		// Check entry name
		auto name = entry->path(true);
		name.erase(name.begin()); // Remove leading /
		if (name.size() > 120)
		{
			log::warning("Entry {} path is too long (> 120 characters), putting it in the root directory", name);
			name = strutil::Path::fileNameOf(name);
			if (name.size() > 120)
				strutil::truncateIP(name, 120);
		}

		// Write entry name
		char name_data[120] = {};
		memcpy(name_data, name.data(), name.size());
		mc.write(name_data, 120);

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
// Loads an [entry]'s data from the archive file on disk into [out]
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool SiNArchive::loadEntryData(const ArchiveEntry* entry, MemChunk& out)
{
	return genericLoadEntryData(entry, out);
}


// -----------------------------------------------------------------------------
//
// SiNArchive Class Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Checks if the given data is a valid Ritual Entertainment SiN archive
// -----------------------------------------------------------------------------
bool SiNArchive::isSiNArchive(const MemChunk& mc)
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
	if (pack[0] != 'S' || pack[1] != 'P' || pack[2] != 'A' || pack[3] != 'K')
		return false;

	// Check directory is sane
	if (dir_offset < 12 || (unsigned)(dir_offset + dir_size) > mc.size())
		return false;

	// That'll do
	return true;
}

// -----------------------------------------------------------------------------
// Checks if the file at [filename] is a valid Ritual SiN archive
// -----------------------------------------------------------------------------
bool SiNArchive::isSiNArchive(const string& filename)
{
	// Open file for reading
	wxFile file(filename);

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
	if (pack[0] != 'S' || pack[1] != 'P' || pack[2] != 'A' || pack[3] != 'K')
		return false;

	// Check directory is sane
	if (dir_offset < 12 || dir_offset + dir_size > file.Length())
		return false;

	// That'll do
	return true;
}
