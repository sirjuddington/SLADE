
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ResArchiveHandler.cpp
// Description: ArchiveFormatHandler for Amulets & Armor RES archives
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
#include "ResArchiveHandler.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/EntryType/EntryType.h"
#include "UI/UI.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// ResArchiveHandler Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Reads a res directory from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool ResArchiveHandler::readDirectory(
	Archive&               archive,
	const MemChunk&        mc,
	size_t                 dir_offset,
	size_t                 num_lumps,
	shared_ptr<ArchiveDir> parent)
{
	if (!parent)
	{
		log::error("ReadDir: No parent node");
		global::error = "Archive is invalid and/or corrupt";
		return false;
	}
	mc.seek(dir_offset, SEEK_SET);
	for (uint32_t d = 0; d < num_lumps; d++)
	{
		// Update splash window progress
		ui::setSplashProgress(d, num_lumps);

		// Read lump info
		char     magic[4] = "";
		char     name[15] = "";
		uint32_t dumzero1, dumzero2;
		uint16_t dumff, dumze;
		uint8_t  flags  = 0;
		uint32_t offset = 0;
		uint32_t size   = 0;

		mc.read(magic, 4);   // ReS\0
		mc.read(name, 14);   // Name
		mc.read(&offset, 4); // Offset
		mc.read(&size, 4);   // Size

		// Check the identifier
		if (magic[0] != 'R' || magic[1] != 'e' || magic[2] != 'S' || magic[3] != 0)
		{
			log::error(
				"ResArchiveHandler::readDir: Entry {} ({}@0x{:x}) has invalid directory entry", name, size, offset);
			global::error = "Archive is invalid and/or corrupt";
			return false;
		}

		// Byteswap values for big endian if needed
		offset   = wxINT32_SWAP_ON_BE(offset);
		size     = wxINT32_SWAP_ON_BE(size);
		name[14] = '\0';

		mc.read(&dumze, 2);
		if (dumze)
			log::info("Flag guard not null for entry {}", name);
		mc.read(&flags, 1);
		if (flags != 1 && flags != 17)
			log::info("Unknown flag value for entry {}", name);
		mc.read(&dumzero1, 4);
		if (dumzero1)
			log::info("Near-end values not set to zero for entry {}", name);
		mc.read(&dumff, 2);
		if (dumff != 0xFFFF)
			log::info("Dummy set to a non-FF value for entry {}", name);
		mc.read(&dumzero2, 4);
		if (dumzero2)
			log::info("Trailing values not set to zero for entry {}", name);

		// If the lump data goes past the end of the file,
		// the resfile is invalid
		if (offset + size > mc.size())
		{
			log::error("ResArchiveHandler::readDirectory: Res archive is invalid or corrupt, offset overflow");
			global::error = "Archive is invalid and/or corrupt";
			return false;
		}

		// Create & setup lump
		auto nlump = std::make_shared<ArchiveEntry>(name, size);
		nlump->setOffsetOnDisk(offset);
		nlump->setSizeOnDisk();
		nlump->setState(EntryState::Unmodified);

		// Read entry data if it isn't zero-sized
		if (nlump->size() > 0)
			nlump->importMemChunk(mc, offset, size);

		// What if the entry is a directory?
		size_t d_o, n_l;
		if (isResArchive(nlump->data(), d_o, n_l))
		{
			if (auto ndir = createDir(archive, name, parent))
			{
				ui::setSplashProgressMessage(fmt::format("Reading res archive data: {} directory", name));
				// Save offset to restore it once the recursion is done
				size_t myoffset = mc.currentPos();
				readDirectory(archive, mc, d_o, n_l, ndir);
				ndir->dirEntry()->setState(EntryState::Unmodified);
				// Restore offset and clean out the entry
				mc.seek(myoffset, SEEK_SET);
			}
			else
				return false;
			// Not a directory, then add to entry list
		}
		else
		{
			parent->addEntry(nlump);

			// Detect entry type
			EntryType::detectEntryType(*nlump);

			// Set entry to unchanged
			nlump->setState(EntryState::Unmodified);
		}
	}
	return true;
}

// -----------------------------------------------------------------------------
// Reads res format data from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool ResArchiveHandler::open(Archive& archive, const MemChunk& mc)
{
	// Check data was given
	if (!mc.hasData())
		return false;

	// Read res header
	uint32_t dir_size   = 0;
	uint32_t dir_offset = 0;
	char     magic[4]   = "";
	mc.seek(0, SEEK_SET);
	mc.read(&magic, 4);      // "Res!"
	mc.read(&dir_offset, 4); // Offset to directory
	mc.read(&dir_size, 4);   // No. of lumps in res

	// Byteswap values for big endian if needed
	dir_size   = wxINT32_SWAP_ON_BE(dir_size);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);

	// Check the header
	if (magic[0] != 'R' || magic[1] != 'e' || magic[2] != 's' || magic[3] != '!')
	{
		log::error("ResArchiveHandler::openFile: File {} has invalid header", archive.filename());
		global::error = "Invalid res header";
		return false;
	}

	if (dir_size % RESDIRENTRYSIZE)
	{
		log::error("ResArchiveHandler::openFile: File {} has invalid directory size", archive.filename());
		global::error = "Invalid res directory size";
		return false;
	}
	uint32_t num_lumps = dir_size / RESDIRENTRYSIZE;

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	ArchiveModSignalBlocker sig_blocker{ archive };

	// Read the directory
	mc.seek(dir_offset, SEEK_SET);
	ui::setSplashProgressMessage("Reading res archive data");
	if (!readDirectory(archive, mc, dir_offset, num_lumps, archive.rootDir()))
		return false;

	// Setup variables
	sig_blocker.unblock();
	archive.setModified(false);

	ui::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Writes the res archive to a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool ResArchiveHandler::write(Archive& archive, MemChunk& mc)
{
	/*	// Determine directory offset & individual lump offsets
		uint32_t dir_offset = 12;
		ArchiveEntry* entry = NULL;
		for (uint32_t l = 0; l < numEntries(); l++) {
			entry = getEntry(l);
			setEntryOffset(entry, dir_offset);
			dir_offset += entry->getSize();
		}

		// Clear/init MemChunk
		mc.clear();
		mc.seek(0, SEEK_SET);
		mc.reSize(dir_offset + numEntries() * 16);

		// Write the header
		uint32_t num_lumps = numEntries();
		mc.write("Res!", 4);
		mc.write(&num_lumps, 4);
		mc.write(&dir_offset, 4);

		// Write the lumps
		for (uint32_t l = 0; l < num_lumps; l++) {
			entry = getEntry(l);
			mc.write(entry->getData(), entry->getSize());
		}

		// Write the directory
		for (uint32_t l = 0; l < num_lumps; l++) {
			entry = getEntry(l);
			char name[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
			long offset = getEntryOffset(entry);
			long size = entry->getSize();

			for (size_t c = 0; c < entry->getName().length() && c < 8; c++)
				name[c] = entry->getName()[c];

			mc.write(&offset, 4);
			mc.write(&size, 4);
			mc.write(name, 8);

			if (update) {
				entry->setState(EntryState::Unmodified);
				entry->exProp("Offset") = (int)offset;
			}
		}
	*/
	return true;
}

// -----------------------------------------------------------------------------
// Checks if the given data is a valid A&A res archive
// -----------------------------------------------------------------------------
bool ResArchiveHandler::isThisFormat(const MemChunk& mc)
{
	size_t dummy1, dummy2;
	return isResArchive(mc, dummy1, dummy2);
}
bool ResArchiveHandler::isResArchive(const MemChunk& mc, size_t& dir_offset, size_t& num_lumps)
{
	// Check size
	if (mc.size() < 12)
		return false;

	// Check for "Res!" header
	if (!(mc[0] == 'R' && mc[1] == 'e' && mc[2] == 's' && mc[3] == '!'))
		return false;

	uint32_t dir_size = 0;
	mc.seek(4, SEEK_SET);
	mc.read(&dir_offset, 4);
	mc.read(&dir_size, 4);

	// Byteswap values for big endian if needed
	dir_size   = wxINT32_SWAP_ON_BE(dir_size);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);

	// A&A contains nested resource files. The offsets are then always relative to
	// the top-level file. This causes problem with the embedded archive system
	// used by SLADE3. The solution is to compute the offset offset. :)
	uint32_t offset_offset = dir_offset - (mc.size() - dir_size);
	uint32_t rel_offset    = dir_offset - offset_offset;

	// Check directory offset and size are both decent
	if (dir_size % RESDIRENTRYSIZE || (rel_offset + dir_size) > mc.size())
		return false;

	num_lumps = dir_size / RESDIRENTRYSIZE;

	// Reset MemChunk (just in case)
	mc.seek(0, SEEK_SET);

	// If it's passed to here it's probably a res file
	return true;
}

// -----------------------------------------------------------------------------
// Checks if the file at [filename] is a valid A&A res archive
// -----------------------------------------------------------------------------
bool ResArchiveHandler::isThisFormat(const string& filename)
{
	// Open file for reading
	wxFile file(wxString::FromUTF8(filename));

	// Check it opened ok
	if (!file.IsOpened())
		return false;

	file.Seek(0, wxFromStart);

	// Read header
	char header[5];
	file.Read(header, 4);
	header[4] = 0;

	// Check for "Res!" header
	if (!(header[0] == 'R' && header[1] == 'e' && header[2] == 's' && header[3] == '!'))
		return false;

	// Get number of lumps and directory offset
	uint32_t dir_offset = 0;
	uint32_t dir_size   = 0;
	file.Read(&dir_offset, 4);
	file.Read(&dir_size, 4);

	// Byteswap values for big endian if needed
	dir_size   = wxINT32_SWAP_ON_BE(dir_size);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);

	// Check directory offset and size are both decent
	if (dir_size % RESDIRENTRYSIZE || (dir_offset + dir_size) > file.Length())
		return false;

	// If it's passed to here it's probably a res file
	return true;
}
