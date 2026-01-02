
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ChasmBinArchive.cpp
// Description: ChasmBinArchive, archive class to handle Chasm: The Rift bin
//              file format
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
#include "ChasmBinArchive.h"
#include "General/UI.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, archive_load_data)


namespace
{
// -----------------------------------------------------------------------------
// Fixes broken wav data
// -----------------------------------------------------------------------------
void fixBrokenWave(ArchiveEntry* const entry)
{
	static const uint32_t MIN_WAVE_SIZE = 44;

	if ("snd_wav" != entry->type()->formatId() || entry->size() < MIN_WAVE_SIZE)
		return;

	// Some wave files have an incorrect size of the format chunk
	uint32_t* const format_size = reinterpret_cast<uint32_t*>(&entry->data()[0x10]);
	if (0x12 == *format_size)
		*format_size = 0x10;
}
} // namespace


// -----------------------------------------------------------------------------
//
// ChasmBinArchive Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Reads Chasm bin format data from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool ChasmBinArchive::open(MemChunk& mc)
{
	// Check given data is valid
	if (mc.size() < HEADER_SIZE)
		return false;

	// Read .bin header and check it
	char magic[4] = {};
	mc.read(magic, sizeof magic);

	if (magic[0] != 'C' || magic[1] != 'S' || magic[2] != 'i' || magic[3] != 'd')
	{
		log::error("ChasmBinArchive::open: Opening failed, invalid header");
		global::error = "Invalid Chasm bin header";
		return false;
	}

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	ArchiveModSignalBlocker sig_blocker{ *this };

	uint16_t num_entries = 0;
	mc.read(&num_entries, sizeof num_entries);
	num_entries = wxUINT16_SWAP_ON_BE(num_entries);

	// Read the directory
	ui::setSplashProgressMessage("Reading Chasm bin archive data");

	for (uint16_t i = 0; i < num_entries; ++i)
	{
		// Update splash window progress
		ui::setSplashProgress(static_cast<float>(i) / num_entries);

		// Read entry info
		char name[NAME_SIZE] = {};
		mc.read(name, sizeof name);

		uint32_t size;
		mc.read(&size, sizeof size);
		size = wxUINT32_SWAP_ON_BE(size);

		uint32_t offset;
		mc.read(&offset, sizeof offset);
		offset = wxUINT32_SWAP_ON_BE(offset);

		// Check offset+size
		if (offset + size > mc.size())
		{
			log::error("ChasmBinArchive::open: Bin archive is invalid or corrupt (entry goes past end of file)");
			global::error = "Archive is invalid and/or corrupt";
			return false;
		}

		// Convert Pascal to zero-terminated string
		memmove(name, name + 1, sizeof name - 1);
		name[sizeof name - 1] = '\0';

		// Create entry
		auto entry              = std::make_shared<ArchiveEntry>(name, size);
		entry->exProp("Offset") = static_cast<int>(offset);
		entry->setLoaded(false);
		entry->setState(ArchiveEntry::State::Unmodified);

		rootDir()->addEntry(entry);
	}

	// Detect all entry types
	ui::setSplashProgressMessage("Detecting entry types");

	vector<ArchiveEntry*> all_entries;
	putEntryTreeAsList(all_entries);

	MemChunk edata;

	for (size_t i = 0; i < all_entries.size(); ++i)
	{
		// Update splash window progress
		ui::setSplashProgress(static_cast<float>(i) / num_entries);

		// Get entry
		const auto entry = all_entries[i];

		// Read entry data if it isn't zero-sized
		if (entry->size() > 0)
		{
			// Read the entry data
			mc.exportMemChunk(edata, entry->exProp<int>("Offset"), entry->size());
			entry->importMemChunk(edata);
		}

		// Detect entry type
		EntryType::detectEntryType(*entry);
		fixBrokenWave(entry);

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
// Writes Chasm bin archive to a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool ChasmBinArchive::write(MemChunk& mc, bool update)
{
	// Clear current data
	mc.clear();

	// Get archive tree as a list
	vector<ArchiveEntry*> entries;
	putEntryTreeAsList(entries);

	// Check limit of entries count
	const uint16_t num_entries = static_cast<uint16_t>(entries.size());

	if (num_entries > MAX_ENTRY_COUNT)
	{
		log::error(
			"ChasmBinArchive::write: Bin archive can contain no more than {} entries", (unsigned)MAX_ENTRY_COUNT);
		global::error = "Maximum number of entries exceeded for Chasm: The Rift bin archive";
		return false;
	}

	// Init data size
	static const uint32_t HEADER_TOC_SIZE = HEADER_SIZE + ENTRY_SIZE * MAX_ENTRY_COUNT;
	mc.reSize(HEADER_TOC_SIZE, false);
	mc.fillData(0);

	// Write header
	const char magic[4] = { 'C', 'S', 'i', 'd' };
	mc.seek(0, SEEK_SET);
	mc.write(magic, 4);
	mc.write(&num_entries, sizeof num_entries);

	// Write directory
	uint32_t offset = HEADER_TOC_SIZE;

	for (uint16_t i = 0; i < num_entries; ++i)
	{
		const auto entry = entries[i];

		// Update entry
		if (update)
		{
			entry->setState(ArchiveEntry::State::Unmodified);
			entry->exProp("Offset") = static_cast<int>(offset);
		}

		// Check entry name
		auto    name        = entry->name();
		uint8_t name_length = static_cast<uint8_t>(name.size());

		if (name_length > NAME_SIZE - 1)
		{
			log::warning("Entry {} name is too long, it will be truncated", name);
			strutil::truncateIP(name, NAME_SIZE - 1);
			name_length = static_cast<uint8_t>(NAME_SIZE - 1);
		}

		// Write entry name
		char name_data[NAME_SIZE] = {};
		memcpy(name_data, &name_length, 1);
		memcpy(name_data + 1, name.data(), name_length);
		mc.write(name_data, NAME_SIZE);

		// Write entry size
		const uint32_t size = entry->size();
		mc.write(&size, sizeof size);

		// Write entry offset
		mc.write(&offset, sizeof offset);

		// Increment/update offset
		offset += size;
	}

	// Write entry data
	mc.reSize(offset);
	mc.seek(HEADER_TOC_SIZE, SEEK_SET);

	for (uint16_t i = 0; i < num_entries; ++i)
	{
		const auto entry = entries[i];
		mc.write(entry->rawData(), entry->size());
	}

	return true;
}

// -----------------------------------------------------------------------------
// Loads an entry's data from Chasm bin file
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool ChasmBinArchive::loadEntryData(ArchiveEntry* entry)
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
		log::error("ChasmBinArchive::loadEntryData: Unable to open archive file {}", filename_);
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
// ChasmBinArchive Class Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Checks if the given data is a valid Chasm bin archive
// -----------------------------------------------------------------------------
bool ChasmBinArchive::isChasmBinArchive(MemChunk& mc)
{
	// Check given data is valid
	if (mc.size() < HEADER_SIZE)
	{
		return false;
	}

	// Read bin header and check it
	char magic[4] = {};
	mc.read(magic, sizeof magic);

	if (magic[0] != 'C' || magic[1] != 'S' || magic[2] != 'i' || magic[3] != 'd')
	{
		return false;
	}

	uint16_t num_entries = 0;
	mc.read(&num_entries, sizeof num_entries);
	num_entries = wxUINT16_SWAP_ON_BE(num_entries);

	return num_entries > MAX_ENTRY_COUNT || (HEADER_SIZE + ENTRY_SIZE * MAX_ENTRY_COUNT) <= mc.size();
}

// -----------------------------------------------------------------------------
// Checks if the file at [filename] is a valid Chasm bin archive
// -----------------------------------------------------------------------------
bool ChasmBinArchive::isChasmBinArchive(const string& filename)
{
	// Open file for reading
	wxFile file(wxString::FromUTF8(filename));

	// Check it opened ok
	if (!file.IsOpened() || file.Length() < HEADER_SIZE)
	{
		return false;
	}

	// Read bin header and check it
	char magic[4] = {};
	file.Read(magic, sizeof magic);

	if (magic[0] != 'C' || magic[1] != 'S' || magic[2] != 'i' || magic[3] != 'd')
		return false;

	uint16_t num_entries = 0;
	file.Read(&num_entries, sizeof num_entries);
	num_entries = wxUINT16_SWAP_ON_BE(num_entries);

	return num_entries > MAX_ENTRY_COUNT
		   || (HEADER_SIZE + ENTRY_SIZE * MAX_ENTRY_COUNT) <= static_cast<uint32_t>(file.Length());
}
