
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    PakArchiveHandler.cpp
// Description: ArchiveFormatHandler for Quake engine PAK files
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
#include "PakArchiveHandler.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "UI/UI.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// PakArchiveHandler Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Reads pak format data from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool PakArchiveHandler::open(Archive& archive, const MemChunk& mc)
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

	bool isPack = pack[0] == 'P' && pack[1] == 'A' && pack[2] == 'C' && pack[3] == 'K';
	bool isHrot = pack[0] == 'H' && pack[1] == 'R' && pack[2] == 'O' && pack[3] == 'T';

	// Check it
	if (!isPack && !isHrot)
	{
		log::error("PakArchiveHandler::open: Opening failed, invalid header");
		global::error = "Invalid pak header";
		return false;
	}

	if (isHrot)
		hrot_archive_ = true;

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	ArchiveModSignalBlocker sig_blocker{ archive };

	// Read the directory
	size_t num_entries = dir_size / 64;
	if (hrot_archive_)
		num_entries = dir_size / 128;
	mc.seek(dir_offset, SEEK_SET);
	ui::setSplashProgressMessage("Reading pak archive data");
	int max_name_size = 56;
	if (hrot_archive_)
		max_name_size = 120;
	for (uint32_t d = 0; d < num_entries; d++)
	{
		// Update splash window progress
		ui::setSplashProgress(d, num_entries);

		// Read entry info
		char    name[120] = {};
		int32_t offset;
		int32_t size;
		mc.read(name, max_name_size);
		mc.read(&offset, 4);
		mc.read(&size, 4);

		// Byteswap if needed
		offset = wxINT32_SWAP_ON_BE(offset);
		size   = wxINT32_SWAP_ON_BE(size);

		// Check offset+size
		if (static_cast<unsigned>(offset + size) > mc.size())
		{
			log::error("PakArchiveHandler::open: Pak archive is invalid or corrupt (entry goes past end of file)");
			global::error = "Archive is invalid and/or corrupt";
			return false;
		}

		// Create directory if needed
		auto dir = createDir(archive, strutil::Path::pathOf(name));

		// Create entry
		auto entry = std::make_shared<ArchiveEntry>(strutil::Path::fileNameOf(name), size);
		entry->setOffsetOnDisk(offset);
		entry->setSizeOnDisk(size);

		// Read entry data if it isn't zero-sized
		if (entry->size() > 0)
			entry->importMemChunk(mc, offset, size);

		// Add to directory
		dir->addEntry(entry);
	}

	// Set all entries/directories to unmodified
	vector<ArchiveEntry*> entry_list;
	archive.putEntryTreeAsList(entry_list);
	for (auto& entry : entry_list)
		entry->setState(EntryState::Unmodified);

	// Detect all entry types
	detectAllEntryTypes(archive);

	// Setup variables
	sig_blocker.unblock();
	archive.setModified(false);

	ui::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Writes the pak archive to a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool PakArchiveHandler::write(Archive& archive, MemChunk& mc)
{
	// Clear current data
	mc.clear();

	// Get archive tree as a list
	vector<ArchiveEntry*> entries;
	archive.putEntryTreeAsList(entries);

	// Process entry list
	int32_t dir_offset = 12;
	int32_t dir_size   = 0;
	for (auto& entry : entries)
	{
		// Ignore folder entries
		if (entry->isFolderType())
			continue;

		// Increment directory offset and size
		dir_offset += entry->size();
		if (hrot_archive_)
			dir_size += 128;
		else
			dir_size += 64;
	}

	// Init data size
	mc.reSize(dir_offset + dir_size, false);

	// Write header
	char pack[4] = { 'P', 'A', 'C', 'K' };
	char hrot[4] = { 'H', 'R', 'O', 'T' };
	mc.seek(0, SEEK_SET);
	if (hrot_archive_)
		mc.write(hrot, 4);
	else
		mc.write(pack, 4);
	mc.write(&dir_offset, 4);
	mc.write(&dir_size, 4);

	// Write directory
	mc.seek(dir_offset, SEEK_SET);
	int32_t offset = 12;
	int max_name_size = 56;
	if (hrot_archive_)
		max_name_size = 120;
	for (auto& entry : entries)
	{
		// Skip folders
		if (entry->isFolderType())
			continue;

		// Update entry
		entry->setState(EntryState::Unmodified);
		entry->setOffsetOnDisk(offset);
		entry->setSizeOnDisk();

		// Check entry name
		auto name = entry->path(true);
		name.erase(name.begin()); // Remove leading /
		if (name.size() > max_name_size)
		{
			log::warning(
				"Warning: Entry {} path is too long (> {} characters), putting it in the root directory", name, max_name_size);
			name = strutil::Path::fileNameOf(name);
			if (name.size() > max_name_size)
				strutil::truncateIP(name, max_name_size);
		}

		// Write entry name
		char name_data[120] = {};
		memcpy(name_data, name.data(), name.size());
		mc.write(name_data, max_name_size);

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
		if (entry->isFolderType())
			continue;

		// Write data
		mc.write(entry->rawData(), entry->size());
	}

	return true;
}

// -----------------------------------------------------------------------------
// Checks if the given data is a valid Quake pak archive
// -----------------------------------------------------------------------------
bool PakArchiveHandler::isThisFormat(const MemChunk& mc)
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

	bool isPack = pack[0] == 'P' && pack[1] == 'A' && pack[2] == 'C' && pack[3] == 'K';
	bool isHrot = pack[0] == 'H' && pack[1] == 'R' && pack[2] == 'O' && pack[3] == 'T';

	// Check header
	if (!isPack && !isHrot)
		return false;

	// Check directory is sane
	if (dir_offset < 12 || static_cast<unsigned>(dir_offset + dir_size) > mc.size())
		return false;

	// That'll do
	return true;
}

// -----------------------------------------------------------------------------
// Checks if the file at [filename] is a valid Quake pak archive
// -----------------------------------------------------------------------------
bool PakArchiveHandler::isThisFormat(const string& filename)
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

	bool isPack = pack[0] == 'P' && pack[1] == 'A' && pack[2] == 'C' && pack[3] == 'K';
	bool isHrot = pack[0] == 'H' && pack[1] == 'R' && pack[2] == 'O' && pack[3] == 'T';

	// Check header
	if (!isPack && !isHrot)
		return false;

	// Check directory is sane
	if (dir_offset < 12 || dir_offset + dir_size > file.Length())
		return false;

	// That'll do
	return true;
}
