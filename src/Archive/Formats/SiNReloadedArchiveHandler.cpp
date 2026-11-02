
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
#include "Utility/FileUtils.h"
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

	dir_ofs   = wxUINT64_SWAP_ON_BE(dir_ofs);
	name_ofs  = wxUINT64_SWAP_ON_BE(name_ofs);
	num_files = wxUINT32_SWAP_ON_BE(num_files);
	name_len  = wxUINT32_SWAP_ON_BE(name_len);

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
		offset   = wxUINT64_SWAP_ON_BE(offset);
		size     = wxUINT32_SWAP_ON_BE(size);
		name_ofs = wxUINT32_SWAP_ON_BE(name_ofs);

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

		// Add to directory
		dir->addEntry(entry);
	}

	// Detect all entry types
	detectAllEntryTypes(archive);

	// Set all entries/directories to unmodified
	vector<ArchiveEntry*> entry_list;
	archive.putEntryTreeAsList(entry_list);
	for (auto& entry : entry_list)
		entry->setState(EntryState::Unmodified);

	// Setup variables
	sig_blocker.unblock();
	archive.setModified(false);

	ui::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Writes the SiN Reloaded archive to a file
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool SiNReloadedArchiveHandler::write(Archive& archive, string_view filename)
{
	// Open file for writing
	SFile file(filename, SFile::Mode::Write);
	if (!file.isOpen())
	{
		global::error = "Unable to open file for writing";
		return false;
	}

	// Get archive tree as a list
	vector<ArchiveEntry*> entries;
	archive.putEntryTreeAsList(entries);

	struct EntryInfo
	{
		ArchiveEntry* entry;
		string        name;
		u64           offset;
		u32           name_offset;
	};

	vector<EntryInfo> file_entries;
	string            name_chunk;
	for (auto* entry : entries)
	{
		if (entry->type() == EntryType::folderType())
			continue;

		auto name = entry->path(true);
		if (!name.empty() && name.front() == '/')
			name.erase(name.begin());

		if (name_chunk.size() + name.size() + 1 > std::numeric_limits<u32>::max())
		{
			global::error = "Archive has too many or too-long entry names";
			return false;
		}

		file_entries.push_back({ entry, std::move(name), 0, static_cast<u32>(name_chunk.size()) });
		name_chunk += file_entries.back().name;
		name_chunk.push_back('\0');
	}

	if (file_entries.size() > std::numeric_limits<u32>::max())
	{
		global::error = "Archive has too many entries";
		return false;
	}

	constexpr u64  header_size   = 32;
	constexpr auto max_file_size = std::numeric_limits<u64>::max();
	u64            dir_offset    = header_size;
	for (auto& info : file_entries)
	{
		info.offset = dir_offset;
		dir_offset += info.entry->size();
		if (dir_offset > max_file_size)
		{
			global::error = "Archive is too large to write";
			return false;
		}
	}

	const auto dir_size = file_entries.size() * 16;
	if (dir_offset + dir_size > max_file_size)
	{
		global::error = "Archive is too large to write";
		return false;
	}
	const auto name_offset = dir_offset + dir_size;
	if (name_offset + name_chunk.size() > max_file_size)
	{
		global::error = "Archive is too large to write";
		return false;
	}

	// Write the header
	char pack[4]     = { 'S', 'R', 'P', 'K' };
	char reserved[4] = {};
	file.write(pack, 4);
	file.write(reserved, 4);
	file.writeU64(dir_offset);
	file.writeU64(name_offset);
	file.writeU32(file_entries.size());
	file.writeU32(name_chunk.size());

	// Write entry data
	for (auto& info : file_entries)
	{
		auto size = info.entry->size();
		if (!file.write(info.entry->rawData(), size))
		{
			global::error = "Unable to write archive";
			return false;
		}
	}

	// Write directory
	for (auto& info : file_entries)
	{
		file.writeU64(info.offset);
		file.writeU32(info.entry->size());
		file.writeU32(info.name_offset);
	}

	// Write entry names
	if (!file.write(name_chunk.data(), name_chunk.size()))
	{
		global::error = "Unable to write archive";
		return false;
	}

	for (auto& info : file_entries)
	{
		info.entry->setState(EntryState::Unmodified);
		info.entry->setOffsetOnDisk(info.offset);
		info.entry->setSizeOnDisk();
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
	wxFFile file(wxString::FromUTF8(filename), wxString::FromUTF8("rb"));

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
