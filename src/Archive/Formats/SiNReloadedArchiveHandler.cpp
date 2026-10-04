
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SiNReloadedArchiveHandler.cpp
// Description: ArchiveFormatHandler for the Ritual Entertainment SiN format,
//              a variant on Quake 2 pak files.
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
#include "SiNReloadedArchiveHandler.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/EntryType/EntryType.h"
#include "UI/UI.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// SiNReloadedArchiveHandler Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Reads SiN Reloaded format data from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool SiNReloadedArchiveHandler::open(Archive& archive, const MemChunk& mc)
{
	// Check given data is valid
	if (mc.size() < 24)
		return false;

	// Read pak header
	char     pack[4];
	uint32_t name_len;
	uint64_t name_ofs;
	uint32_t num_files;
	uint64_t dir_ofs;
	mc.seek(0, SEEK_SET);
	mc.read(pack, sizeof(pack));

	// Check it
	if (pack[0] != 'S' || pack[1] != 'R' || pack[2] != 'P' || pack[3] != 'K')
	{
		log::error("SiNReloadedArchiveHandler::open: Opening failed, invalid header");
		global::error = "Invalid pak header";
		return false;
	}

	char reserved[4];
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
	ArchiveModSignalBlocker sig_blocker{ archive };

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
		if ((offset + size) > mc.size() || name_ofs >= mc.size())
		{
			log::error(
				"SiNReloadedArchiveHandler::open: SiN archive is invalid or corrupt (entry goes past end of file)");
			global::error = "Archive is invalid and/or corrupt";
			return false;
		}

		std::string_view name = name_chunk.data() + name_ofs;

		// Create directory if needed
		auto dir = createDir(archive, strutil::Path::pathOf(name));

		// Create entry
		auto entry = std::make_shared<ArchiveEntry>(strutil::Path::fileNameOf(name), size);
		entry->setOffsetOnDisk(offset);
		entry->setSizeOnDisk();

		// Read entry data if it isn't zero-sized
		if (entry->size() > 0)
			entry->importMemChunk(mc, offset, size);

		entry->setState(EntryState::Unmodified);

		// Add to directory
		dir->addEntry(entry);
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
// Writes the SiN Reloaded archive to a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool SiNReloadedArchiveHandler::write(Archive& archive, MemChunk& mc)
{
	// Clear current data
	mc.clear();

	// Get archive tree as a list
	vector<ArchiveEntry*> entries;
	archive.putEntryTreeAsList(entries);

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
		entry->setState(EntryState::Unmodified);
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
// Checks if the given data is a valid SiN Reloaded archive
// -----------------------------------------------------------------------------
bool SiNReloadedArchiveHandler::isThisFormat(const MemChunk& mc)
{
	// Check given data is valid
	if (mc.size() < 12)
		return false;

	// Read pak header
	char     pack[4];
	char     reserved[4];
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
	name_len  = wxUINT32_SWAP_ON_BE(name_len);
	name_ofs  = wxUINT64_SWAP_ON_BE(name_ofs);
	num_files = wxUINT32_SWAP_ON_BE(num_files);
	dir_ofs   = wxUINT64_SWAP_ON_BE(dir_ofs);

	// Check header
	if (pack[0] != 'S' || pack[1] != 'R' || pack[2] != 'P' || pack[3] != 'K')
		return false;

	// Check directory is sane
	if (dir_ofs < 24 || dir_ofs + (num_files * 16) > mc.size() || name_ofs < 24 || name_ofs + name_len > mc.size())
		return false;

	// That'll do
	return true;
}

// -----------------------------------------------------------------------------
// Checks if the file at [filename] is a valid SiN Reloaded archive
// -----------------------------------------------------------------------------
bool SiNReloadedArchiveHandler::isThisFormat(const string& filename)
{
	// Open file for reading
	wxFFile file(wxString::FromUTF8(filename), "rb");

	// Check it opened ok
	if (!file.IsOpened() || file.Length() < 24)
		return false;

	// Read pak header
	char     pack[4];
	char     reserved[4];
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
	name_len  = wxUINT32_SWAP_ON_BE(name_len);
	name_ofs  = wxUINT64_SWAP_ON_BE(name_ofs);
	num_files = wxUINT32_SWAP_ON_BE(num_files);
	dir_ofs   = wxUINT64_SWAP_ON_BE(dir_ofs);

	// Check header
	if (pack[0] != 'S' || pack[1] != 'R' || pack[2] != 'P' || pack[3] != 'K')
		return false;

	// Check directory is sane
	if (dir_ofs < 24
		|| dir_ofs + (num_files * 16) > file.Length()
		|| name_ofs < 24
		|| name_ofs + name_len > file.Length())
		return false;

	// That'll do
	return true;
}
