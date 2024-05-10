
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ChasmBinArchiveHandler.cpp
// Description: ArchiveFormatHanlder for Chasm: The Rift bin file format
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
#include "ChasmBinArchiveHandler.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/EntryType/EntryType.h"
#include "General/UI.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Fixes broken wav data
// -----------------------------------------------------------------------------
void fixBrokenWave(const ArchiveEntry* entry)
{
	static constexpr uint32_t MIN_WAVE_SIZE = 44;

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
// ChasmBinArchiveHandler Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Reads Chasm bin format data from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool ChasmBinArchiveHandler::open(Archive& archive, const MemChunk& mc, bool detect_types)
{
	// Check given data is valid
	if (mc.size() < HEADER_SIZE)
		return false;

	// Read .bin header and check it
	char magic[4] = {};
	mc.read(magic, sizeof magic);

	if (magic[0] != 'C' || magic[1] != 'S' || magic[2] != 'i' || magic[3] != 'd')
	{
		log::error("ChasmBinArchiveHandler::open: Opening failed, invalid header");
		global::error = "Invalid Chasm bin header";
		return false;
	}

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	ArchiveModSignalBlocker sig_blocker{ archive };

	uint16_t num_entries = 0;
	mc.read(&num_entries, sizeof num_entries);
	num_entries = wxUINT16_SWAP_ON_BE(num_entries);

	// Read the directory
	ui::setSplashProgressMessage("Reading Chasm bin archive data");

	for (uint16_t i = 0; i < num_entries; ++i)
	{
		// Update splash window progress
		ui::setSplashProgress(i, num_entries);

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
			log::error("ChasmBinArchiveHandler::open: Bin archive is invalid or corrupt (entry goes past end of file)");
			global::error = "Archive is invalid and/or corrupt";
			return false;
		}

		// Convert Pascal to zero-terminated string
		memmove(name, name + 1, sizeof name - 1);
		name[sizeof name - 1] = '\0';

		// Create entry
		auto entry = std::make_shared<ArchiveEntry>(name, size);
		entry->setOffsetOnDisk(offset);
		entry->setSizeOnDisk(size);

		// Read entry data if it isn't zero-sized
		if (entry->size() > 0)
			entry->importMemChunk(mc, offset, size);

		entry->setState(EntryState::Unmodified);

		archive.rootDir()->addEntry(entry);
	}

	// Detect all entry types
	ui::setSplashProgressMessage("Detecting entry types");

	vector<ArchiveEntry*> all_entries;
	archive.putEntryTreeAsList(all_entries);

	MemChunk edata;

	for (size_t i = 0; i < all_entries.size(); ++i)
	{
		// Update splash window progress
		ui::setSplashProgress(i, num_entries);

		// Get entry
		const auto entry = all_entries[i];

		// Detect entry type
		EntryType::detectEntryType(*entry);
		fixBrokenWave(entry);

		// Set entry to unchanged
		entry->setState(EntryState::Unmodified);
	}

	// Setup variables
	sig_blocker.unblock();
	archive.setModified(false);

	ui::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Writes Chasm bin archive to a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool ChasmBinArchiveHandler::write(Archive& archive, MemChunk& mc)
{
	// Clear current data
	mc.clear();

	// Get archive tree as a list
	vector<ArchiveEntry*> entries;
	archive.putEntryTreeAsList(entries);

	// Check limit of entries count
	const uint16_t num_entries = static_cast<uint16_t>(entries.size());

	if (num_entries > MAX_ENTRY_COUNT)
	{
		log::error("ChasmBinArchiveHandler::write: Bin archive can contain no more than {} entries", MAX_ENTRY_COUNT);
		global::error = "Maximum number of entries exceeded for Chasm: The Rift bin archive";
		return false;
	}

	// Init data size
	static constexpr uint32_t HEADER_TOC_SIZE = HEADER_SIZE + ENTRY_SIZE * MAX_ENTRY_COUNT;
	mc.reSize(HEADER_TOC_SIZE, false);
	mc.fillData(0);

	// Write header
	constexpr char magic[4] = { 'C', 'S', 'i', 'd' };
	mc.seek(0, SEEK_SET);
	mc.write(magic, 4);
	mc.write(&num_entries, sizeof num_entries);

	// Write directory
	uint32_t offset = HEADER_TOC_SIZE;

	for (uint16_t i = 0; i < num_entries; ++i)
	{
		const auto entry = entries[i];

		// Update entry
		entry->setState(EntryState::Unmodified);
		entry->setOffsetOnDisk(offset);
		entry->setSizeOnDisk(entry->size());

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
//
// ChasmBinArchiveHandler Class Static Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Checks if the given data is a valid Chasm bin archive
// -----------------------------------------------------------------------------
bool ChasmBinArchiveHandler::isThisFormat(const MemChunk& mc)
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
bool ChasmBinArchiveHandler::isThisFormat(const string& filename)
{
	// Open file for reading
	wxFile file(filename);

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
