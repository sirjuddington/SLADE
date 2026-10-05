
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
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, pak_enable_daikatana, false, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Decompresses compressed pak data from [input] into [output].
// Reference: https://github.com/TrenchBroom/TrenchBroom/blob/master/lib/TbFsLib/src/DkPakFileSystem.cpp
// -----------------------------------------------------------------------------
bool dkDecompress(MemChunk& input, MemChunk& output)
{
	while (input.currentPos() < input.size())
	{
		uint8_t c;

		if (!input.read(&c, sizeof(c)))
			return false;

		if (c < 0x40)
		{
			// x+1 bytes of uncompressed data follow (just read+write them as they are)
			size_t len = c + 1;

			if (len > input.size() - input.currentPos() || len > output.size() - output.currentPos())
				return false;

			if (!output.write(input.data() + input.currentPos(), len))
				return false;

			if (!input.seek(len))
				return false;
		}
		else if (c < 0x80)
		{
			// run-length encoded zeros, write (x - 62) zero-bytes to output
			size_t len = c - 62;

			if (len > output.size() - output.currentPos())
				return false;

			memset(output.data() + output.currentPos(), 0, len);

			if (!output.seek(len))
				return false;
		}
		else if (c < 0xC0)
		{
			// run-length encoded data, read one byte, write it (x-126) times to output
			size_t  len = c - 126;
			uint8_t val;

			if (len > output.size() - output.currentPos())
				return false;

			if (!input.read(&val, sizeof(val)))
				return false;

			memset(output.data() + output.currentPos(), val, len);

			if (!output.seek(len))
				return false;
		}
		else if (c < 0xFE)
		{
			// this references previously uncompressed data
			// read one byte to get _offset_
			// read (x-190) bytes from the already uncompressed and written output data,
			// starting at (offset+2) bytes before the current write position (and add them
			// to output, of course)
			size_t  len = c - 190;
			uint8_t offset;

			if (!input.read(&offset, sizeof(offset)))
				return false;

			const size_t distance = offset + 2;
			const size_t out_pos  = output.currentPos();
			if (distance > out_pos || len > output.size() - out_pos)
				return false;

			for (size_t i = 0; i < len; ++i)
				output.data()[out_pos + i] = output.data()[out_pos + i - distance];

			if (!output.seek(len))
				return false;
		}
	}

	return input.currentPos() == input.size() && output.currentPos() == output.size();
}
} // namespace


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

	bool is_pack = pack[0] == 'P' && pack[1] == 'A' && pack[2] == 'C' && pack[3] == 'K';
	bool is_hrot = pack[0] == 'H' && pack[1] == 'R' && pack[2] == 'O' && pack[3] == 'T';

	// Check it
	if (!is_pack && !is_hrot)
	{
		log::error("PakArchiveHandler::open: Opening failed, invalid header");
		global::error = "Invalid pak header";
		return false;
	}

	// Detect if it's a Daikatana-style pak.
	// Annoyingly, the only real difference is there's two ints after each entry
	// that are effectively unused (so entries are 72 bytes, not 64).
	bool is_daikatana = pak_enable_daikatana && remainder(dir_size, 64) != 0;
	if (is_daikatana && dir_size % 576 == 0)
	{
		// try loading it as DK just to see
		size_t num_entries = dir_size / 72;
		mc.seek(dir_offset, SEEK_SET);

		for (uint32_t d = 0; d < num_entries; d++)
		{
			// Read entry info
			char    name[56];
			int32_t offset;
			int32_t size;
			mc.read(name, 56);
			mc.read(&offset, 4);
			mc.read(&size, 4);

			int i;

			for (i = 0; i < 56; i++)
			{
				if (name[i] == '\0' || !isalnum(name[i]))
					break;
			}

			if (i == 0 || !isalnum(name[i]))
			{
				is_daikatana = false;
				break;
			}

			// Byteswap if needed
			offset = wxINT32_SWAP_ON_BE(offset);
			size   = wxINT32_SWAP_ON_BE(size);

			{
				int complen, comptype;
				mc.read(&complen, 4);
				mc.read(&comptype, 4);

				if (comptype != 0 && comptype != 1)
				{
					is_daikatana = false;
					break;
				}

				if (comptype == 1)
				{
					size = complen;
				}
			}

			// Check offset+size
			if ((unsigned)(offset + size) > mc.size())
			{
				is_daikatana = false;
				break;
			}
		}
	}

	if (is_hrot)
		hrot_archive_ = true;

	// Stop announcements (don't want to be announcing modification due to
	// entries being added etc)
	ArchiveModSignalBlocker sig_blocker{ archive };

	// Read the directory
	size_t num_entries = dir_size / (is_daikatana ? 72 : 64);
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

		int complen = size;
		int comptype;

		if (is_daikatana)
		{
			mc.read(&complen, 4);
			mc.read(&comptype, 4);

			if (comptype)
			{
				if (comptype != 1)
				{
					log::error(
						"PakArchive::open: Pak archive is invalid or corrupt (Daikatana-type compression value not "
						"valid)");
					global::error = "Archive is invalid and/or corrupt";
					return false;
				}
			}
		}

		// Check offset+size
		if ((unsigned)(offset + complen) > mc.size())
		{
			log::error("PakArchiveHandler::open: Pak archive is invalid or corrupt (entry goes past end of file)");
			global::error = "Archive is invalid and/or corrupt";
			return false;
		}

		// Create directory if needed
		auto dir = createDir(archive, strutil::Path::pathOf(name));

		// Create entry
		auto entry = std::make_shared<ArchiveEntry>(strutil::Path::fileNameOf(name), complen);
		entry->setOffsetOnDisk(offset);
		entry->setSizeOnDisk(size);

		if (is_daikatana && comptype)
		{
			entry->exProp("Compression")   = comptype;
			entry->exProp("DecompressLen") = size;
		}

		// Read entry data if it isn't zero-sized
		if (entry->size() > 0)
		{
			// Check if entry is compressed (Daikatana)
			if (is_daikatana && comptype)
			{
				if (size < 0)
				{
					log::error("PakArchiveHandler::open: Pak archive has an invalid decompressed entry size");
					global::error = "Archive is invalid and/or corrupt";
					return false;
				}

				// Decompress entry data
				MemChunk compressed;
				mc.exportMemChunk(compressed, offset, complen);
				MemChunk decompressed(size);
				if (!dkDecompress(compressed, decompressed))
				{
					log::error(
						"PakArchiveHandler::open: Pak archive is invalid or corrupt (entry decompression failed)");
					global::error = "Archive is invalid and/or corrupt";
					return false;
				}

				entry->importMemChunk(decompressed);
			}
			else
				entry->importMemChunk(mc, offset, size);
		}

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
	int32_t offset        = 12;
	int     max_name_size = 56;
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
				"Warning: Entry {} path is too long (> {} characters), putting it in the root directory",
				name,
				max_name_size);
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
// Loads an [entry]'s data from the archive file on disk into [out].
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool PakArchiveHandler::loadEntryData(Archive& archive, const ArchiveEntry* entry, MemChunk& out)
{
	// Check if entry is compressed (Daikatana)
	if (auto comp = entry->exProps().getIf<int>("Compression"); comp && *comp == 1)
	{
		MemChunk compressed;
		if (!ArchiveFormatHandler::loadEntryData(archive, entry, compressed))
			return false;

		int decompress_len = entry->exProps().getIf<int>("DecompressLen").value_or(0);
		if (decompress_len < 0)
			return false;
		if (decompress_len == 0)
			out.clear();
		else if (!out.reSize(decompress_len))
			return false;
		out.seekFromStart(0);

		if (!dkDecompress(compressed, out))
			return false;

		return true;
	}

	return ArchiveFormatHandler::loadEntryData(archive, entry, out);
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
