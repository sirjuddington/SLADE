
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SiNReloadedArchive.cpp
// Description: SiNReloadedArchive, archive class to handle the Ritual Entertainment SiN
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
#include "SiNReloadedArchive.h"
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
// SiNReloadedArchive Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Reads SiN format data from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool SiNReloadedArchive::open(MemChunk& mc)
{
	// Check given data is valid
	if (mc.size() < 24)
		return false;

	// Read pak header
	char    pack[4];
	uint32_t name_len;
	uint64_t name_ofs;
	uint32_t num_files;
	uint64_t dir_ofs;
	mc.seek(0, SEEK_SET);
	mc.read(pack, sizeof(pack));

	// Check it
	if (pack[0] != 'S' || pack[1] != 'R' || pack[2] != 'P' || pack[3] != 'K')
	{
		log::error("SiNReloadedArchive::open: Opening failed, invalid header");
		global::error = "Invalid pak header";
		return false;
	}
	
	char    reserved[4];
	mc.read(reserved, sizeof(reserved));
	mc.read(&dir_ofs, sizeof(dir_ofs));
	mc.read(&name_ofs, sizeof(name_ofs));
	mc.read(&num_files, sizeof(num_files));
	mc.read(&name_len, sizeof(name_len));

	std::string name_chunk;
	name_chunk.resize(name_len);
	mc.seek(name_ofs, SEEK_SET);
	mc.read(name_chunk.data(), name_len);

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	ArchiveModSignalBlocker sig_blocker{ *this };

	// Read the directory
	mc.seek(dir_ofs, SEEK_SET);
	ui::setSplashProgressMessage("Reading SiN archive data");
	for (uint32_t d = 0; d < num_files; d++)
	{
		// Update splash window progress
		ui::setSplashProgress(((float)d / (float)num_files));

		// Read entry info
		uint64_t offset;
		uint32_t size;
		uint32_t name_ofs;
		mc.read(&offset, sizeof(offset));
		mc.read(&size, sizeof(size));
		mc.read(&name_ofs, sizeof(name_ofs));

		// Byteswap if needed
		offset = wxUINT64_SWAP_ON_BE(offset);
		size   = wxUINT32_SWAP_ON_BE(size);

		// Check offset+size
		if ((offset + size) > mc.size() ||
			name_ofs >= mc.size())
		{
			log::error("SiNReloadedArchive::open: SiN archive is invalid or corrupt (entry goes past end of file)");
			global::error = "Archive is invalid and/or corrupt";
			return false;
		}

		std::string_view name = name_chunk.data() + name_ofs;

		// Create directory if needed
		auto dir = createDir(strutil::Path::pathOf(name));

		// Create entry
		auto entry              = std::make_shared<ArchiveEntry>(strutil::Path::fileNameOf(name), size);
		entry->exProp("Offset") = (uint32_t)offset;
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
		ui::setSplashProgress((((float)a / (float)num_files)));

		// Get entry
		auto entry = all_entries[a];

		// Read entry data if it isn't zero-sized
		if (entry->size() > 0)
		{
			// Read the entry data
			mc.exportMemChunk(edata, entry->exProp<uint32_t>("Offset"), entry->size());
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
// Writes the SiN archive to a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool SiNReloadedArchive::write(MemChunk& mc, bool update)
{
	// Clear current data
	mc.clear();

	// Get archive tree as a list
	vector<ArchiveEntry*> entries;
	putEntryTreeAsList(entries);

	// Process entry list
	uint32_t dir_offset = 12;
	uint32_t dir_size   = 0;
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
	char pack[4] = { 'S', 'R', 'P', 'K' };
	mc.seek(0, SEEK_SET);
	mc.write(pack, 4);
	mc.write(&dir_offset, 4);
	mc.write(&dir_size, 4);

	// Write directory
	mc.seek(dir_offset, SEEK_SET);
	uint32_t offset = 12;
	for (auto& entry : entries)
	{
		// Skip folders
		if (entry->type() == EntryType::folderType())
			continue;

		// Update entry
		if (update)
		{
			entry->setState(ArchiveEntry::State::Unmodified);
			entry->exProp("Offset") = (uint32_t) offset;
		}

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
		char name_data[120];
		memset(name_data, 0, 120);
		memcpy(name_data, name.data(), name.size());
		mc.write(name_data, 120);

		// Write entry offset
		mc.write(&offset, 4);

		// Write entry size
		uint32_t size = entry->size();
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
// Loads an entry's data from the SiN file
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool SiNReloadedArchive::loadEntryData(ArchiveEntry* entry)
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
	wxFFile file(wxString::FromUTF8(filename_), "rb");

	// Check it opened
	if (!file.IsOpened())
	{
		log::error("SiNReloadedArchive::loadEntryData: Unable to open archive file {}", filename_);
		return false;
	}

	// Seek to entry offset in file and read it in
	file.Seek(entry->exProp<uint32_t>("Offset"), wxFromStart);
	entry->importFileStream(file, entry->size());

	// Set the lump to loaded
	entry->setLoaded();

	return true;
}


// -----------------------------------------------------------------------------
//
// SiNReloadedArchive Class Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Checks if the given data is a valid Ritual Entertainment SiN archive
// -----------------------------------------------------------------------------
bool SiNReloadedArchive::isSiNReloadedArchive(MemChunk& mc)
{
	// Check given data is valid
	if (mc.size() < 12)
		return false;

	// Read pak header
	char    pack[4];
	char    reserved[4];
	uint32_t name_len;
	uint64_t name_ofs;
	uint32_t num_files;
	uint64_t dir_ofs;
	mc.seek(0, SEEK_SET);
	mc.read(pack, sizeof(pack));
	mc.read(reserved, sizeof(reserved));
	mc.read(&dir_ofs, sizeof(dir_ofs));
	mc.read(&name_ofs, sizeof(name_ofs));
	mc.read(&num_files, sizeof(num_files));
	mc.read(&name_len, sizeof(name_len));

	// Byteswap values for big endian if needed
	name_len   = wxUINT32_SWAP_ON_BE(name_len);
	name_ofs   = wxUINT64_SWAP_ON_BE(name_ofs);
	num_files  = wxUINT32_SWAP_ON_BE(num_files);
	dir_ofs    = wxUINT64_SWAP_ON_BE(dir_ofs);

	// Check header
	if (pack[0] != 'S' || pack[1] != 'R' || pack[2] != 'P' || pack[3] != 'K')
		return false;

	// Check directory is sane
	if (dir_ofs < 24 || dir_ofs + (num_files * 16) > mc.size() ||
		name_ofs < 24 || name_ofs + name_len > mc.size())
		return false;

	// That'll do
	return true;
}

// -----------------------------------------------------------------------------
// Checks if the file at [filename] is a valid Ritual SiN archive
// -----------------------------------------------------------------------------
bool SiNReloadedArchive::isSiNReloadedArchive(const string& filename)
{
	// Open file for reading
	wxFFile file(wxString::FromUTF8(filename), "rb");

	// Check it opened ok
	if (!file.IsOpened() || file.Length() < 24)
		return false;

	// Read pak header
	char    pack[4];
	char    reserved[4];
	uint32_t name_len;
	uint64_t name_ofs;
	uint32_t num_files;
	uint64_t dir_ofs;
	file.Seek(0, wxFromStart);
	file.Read(pack, sizeof(pack));
	file.Read(reserved, sizeof(reserved));
	file.Read(&dir_ofs, sizeof(dir_ofs));
	file.Read(&name_ofs, sizeof(name_ofs));
	file.Read(&num_files, sizeof(num_files));
	file.Read(&name_len, sizeof(name_len));

	// Byteswap values for big endian if needed
	name_len   = wxUINT32_SWAP_ON_BE(name_len);
	name_ofs   = wxUINT64_SWAP_ON_BE(name_ofs);
	num_files  = wxUINT32_SWAP_ON_BE(num_files);
	dir_ofs    = wxUINT64_SWAP_ON_BE(dir_ofs);

	// Check header
	if (pack[0] != 'S' || pack[1] != 'R' || pack[2] != 'P' || pack[3] != 'K')
		return false;

	// Check directory is sane
	if (dir_ofs < 24 || dir_ofs + (num_files * 16) > file.Length() ||
		name_ofs < 24 || name_ofs + name_len > file.Length())
		return false;

	// That'll do
	return true;
}
