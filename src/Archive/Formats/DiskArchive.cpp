
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    DiskArchive.cpp
// Description: DiskArchive, archive class to handle Nerve files
//
// Note: specifications and snippets of code were taken from the Eternity Engine,
// by James Haley (a.k.a. Quasar).
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
#include "DiskArchive.h"
#include "General/UI.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// DiskArchive Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Reads disk format data from a MemChunk.
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool DiskArchive::open(MemChunk& mc)
{
	size_t mcsize = mc.size();

	// Check given data is valid
	if (mcsize < 80)
		return false;

	// Read disk header
	uint32_t num_entries;
	mc.seek(0, SEEK_SET);
	mc.read(&num_entries, 4);
	num_entries = wxUINT32_SWAP_ON_LE(num_entries);

	size_t start_offset = (72 * num_entries) + 8;

	if (mcsize < start_offset)
		return false;

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	ArchiveModSignalBlocker sig_blocker{ *this };

	// Read the directory
	ui::setSplashProgressMessage("Reading disk archive data");
	for (uint32_t d = 0; d < num_entries; d++)
	{
		// Update splash window progress
		ui::setSplashProgress(d, num_entries);

		// Read entry info
		DiskEntry dent;
		mc.read(&dent, 72);

		// Byteswap if needed
		dent.length = wxUINT32_SWAP_ON_LE(dent.length);
		dent.offset = wxUINT32_SWAP_ON_LE(dent.offset);

		// Increase offset to make it relative to start of archive
		dent.offset += start_offset;

		// Check offset+size
		if (dent.offset + dent.length > mcsize)
		{
			log::error("DiskArchive::open: Disk archive is invalid or corrupt (entry goes past end of file)");
			global::error = "Archive is invalid and/or corrupt";
			return false;
		}

		// Parse name
		string name = dent.name;
		std::replace(name.begin(), name.end(), '\\', '/');
		strutil::replaceIP(name, "GAME:/", "");
		strutil::Path fn(name);

		// Create directory if needed
		auto dir = createDir(fn.path());

		// Create entry
		auto entry = std::make_shared<ArchiveEntry>(fn.fileName(), dent.length);
		entry->setOffsetOnDisk(dent.offset);
		entry->setSizeOnDisk(dent.length);

		// Read entry data if it isn't zero-sized
		if (entry->size() > 0)
			entry->importMemChunk(mc, dent.offset, dent.length);

		entry->setState(ArchiveEntry::State::Unmodified);

		// Add to directory
		dir->addEntry(entry);
	}

	// Detect all entry types
	detectAllEntryTypes();

	// Setup variables
	sig_blocker.unblock();
	setModified(false);

	ui::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Writes the disk archive to a MemChunk.
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool DiskArchive::write(MemChunk& mc)
{
	// Clear current data
	mc.clear();

	// Get archive tree as a list
	vector<ArchiveEntry*> entries;
	putEntryTreeAsList(entries);

	// Process entry list
	uint32_t num_entries  = 0;
	uint32_t size_entries = 0;
	for (auto& entry : entries)
	{
		// Ignore folder entries
		if (entry->type() == EntryType::folderType())
			continue;

		// Increment directory offset and size
		size_entries += entry->size();
		++num_entries;
	}

	// Init data size
	uint32_t start_offset = 8 + (num_entries * 72);
	uint32_t offset       = start_offset;
	mc.reSize(size_entries + start_offset, false);

	// Write header
	num_entries = wxUINT32_SWAP_ON_LE(num_entries);
	mc.seek(0, SEEK_SET);
	mc.write(&num_entries, 4);

	// Write directory
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
		std::replace(name.begin(), name.end(), '/', '\\');
		// The leading "GAME:\" part of the name means there is only 58 usable characters for path
		if (name.size() > 58)
		{
			log::warning(
				"Warning: Entry {} path is too long (> 58 characters), putting it in the root directory", name);

			auto fname = strutil::Path::fileNameOf(name);
			name       = (fname.size() > 57) ? fname.substr(0, 57) : fname;
			name.insert(name.begin(), '\\'); // Add leading "\"
		}
		strutil::prependIP(name, "GAME:");

		DiskEntry dent;

		// Write entry name
		// The names field are padded with FD for doom.disk, FE for doom2.disk. No idea whether
		// a non-null padding is actually required, though. It probably should work with anything.
		memset(dent.name, 0xFE, 64);
		memcpy(dent.name, name.data(), name.size());
		dent.name[name.size()] = 0;

		// Write entry offset
		dent.offset = wxUINT32_SWAP_ON_LE(offset - start_offset);

		// Write entry size
		dent.length = wxUINT32_SWAP_ON_LE(entry->size());

		// Actually write stuff
		mc.write(&dent, 72);

		// Increment/update offset
		offset += wxUINT32_SWAP_ON_LE(dent.length);
	}

	// Finish writing header
	size_entries = wxUINT32_SWAP_ON_LE(size_entries);
	mc.write(&size_entries, 4);

	// Write entry data
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
// Loads an entry's data from the disk file.
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool DiskArchive::loadEntryData(const ArchiveEntry* entry, MemChunk& out)
{
	return genericLoadEntryData(entry, out);
}


// -----------------------------------------------------------------------------
//
// DiskArchive Class Static Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Checks if the given data is a valid Nerve disk archive
// -----------------------------------------------------------------------------
bool DiskArchive::isDiskArchive(MemChunk& mc)
{
	// Check given data is valid
	size_t mcsize = mc.size();
	if (mcsize < 80)
		return false;

	// Read disk header
	uint32_t num_entries;
	uint32_t size_entries;
	mc.seek(0, SEEK_SET);
	mc.read(&num_entries, 4);
	num_entries = wxUINT32_SWAP_ON_LE(num_entries);

	size_t start_offset = (72 * num_entries) + 8;

	if (mcsize < start_offset)
		return false;

	// Read the directory
	for (uint32_t d = 0; d < num_entries; d++)
	{
		// Read entry info
		DiskEntry entry;
		mc.read(&entry, 72);

		// Byteswap if needed
		entry.length = wxUINT32_SWAP_ON_LE(entry.length);
		entry.offset = wxUINT32_SWAP_ON_LE(entry.offset);

		// Increase offset to make it relative to start of archive
		entry.offset += start_offset;

		// Check offset+size
		if (entry.offset + entry.length > mcsize)
			return false;
	}
	mc.read(&size_entries, 4);
	size_entries = wxUINT32_SWAP_ON_LE(size_entries);
	if (size_entries + start_offset != mcsize)
		return false;

	// That'll do
	return true;
}

// -----------------------------------------------------------------------------
// Checks if the file at [filename] is a valid Quake disk archive
// -----------------------------------------------------------------------------
bool DiskArchive::isDiskArchive(const string& filename)
{
	// Open file for reading
	wxFile file(filename);

	// Check it opened ok
	if (!file.IsOpened() || file.Length() < 4)
		return false;
	// Check given data is valid
	size_t mcsize = file.Length();
	if (mcsize < 80)
		return false;

	// Read disk header
	uint32_t num_entries;
	uint32_t size_entries;
	file.Seek(0, wxFromStart);
	file.Read(&num_entries, 4);
	num_entries = wxUINT32_SWAP_ON_LE(num_entries);

	size_t start_offset = (72 * num_entries) + 8;

	if (mcsize < start_offset)
		return false;

	// Read the directory
	for (uint32_t d = 0; d < num_entries; d++)
	{
		// Read entry info
		DiskEntry entry;
		file.Read(&entry, 72);

		// Byteswap if needed
		entry.length = wxUINT32_SWAP_ON_LE(entry.length);
		entry.offset = wxUINT32_SWAP_ON_LE(entry.offset);

		// Increase offset to make it relative to start of archive
		entry.offset += start_offset;

		// Check offset+size
		if (entry.offset + entry.length > mcsize)
			return false;
	}
	file.Read(&size_entries, 4);
	size_entries = wxUINT32_SWAP_ON_LE(size_entries);
	if (size_entries + start_offset != mcsize)
		return false;

	// That'll do
	return true;
}
