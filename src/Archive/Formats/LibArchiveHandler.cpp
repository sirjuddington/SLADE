
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    LibArchiveHandler.cpp
// Description: ArchiveFormatHandler for Shadowcaster LIB archives
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
#include "LibArchiveHandler.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "UI/UI.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// LibArchiveHandler Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Reads wad format data from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool LibArchiveHandler::open(Archive& archive, const MemChunk& mc)
{
	// Check data was given
	if (!mc.hasData())
		return false;

	// Read lib footer
	mc.seek(2, SEEK_END);
	uint32_t num_lumps = 0;
	mc.read(&num_lumps, 2); // Size
	num_lumps           = wxINT16_SWAP_ON_BE(num_lumps);
	uint32_t dir_offset = mc.size() - (2 + (num_lumps * 21));

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	ArchiveModSignalBlocker sig_blocker{ archive };

	// Read the directory
	mc.seek(dir_offset, SEEK_SET);
	ui::setSplashProgressMessage("Reading lib archive data");
	for (uint32_t d = 0; d < num_lumps; d++)
	{
		// Update splash window progress
		ui::setSplashProgress(d, num_lumps);

		// Read lump info
		char     myname[13] = "";
		uint32_t offset     = 0;
		uint32_t size       = 0;

		mc.read(&size, 4);   // Size
		mc.read(&offset, 4); // Offset
		mc.read(myname, 13); // Name
		myname[12] = '\0';

		// Byteswap values for big endian if needed
		offset = wxINT32_SWAP_ON_BE(offset);
		size   = wxINT32_SWAP_ON_BE(size);

		// If the lump data goes past the directory,
		// the wadfile is invalid
		if (offset + size > dir_offset)
		{
			log::error("LibArchiveHandler::open: Lib archive is invalid or corrupt");
			global::error = "Archive is invalid and/or corrupt";
			return false;
		}

		// Create & setup lump
		auto nlump = std::make_shared<ArchiveEntry>(myname, size);
		nlump->setOffsetOnDisk(offset);
		nlump->setSizeOnDisk(size);

		// Read entry data if it isn't zero-sized
		if (nlump->size() > 0)
			nlump->importMemChunk(mc, offset, size);

		nlump->setState(EntryState::Unmodified);

		// Add to entry list
		archive.rootDir()->addEntry(nlump);
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
// Writes the wad archive to a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool LibArchiveHandler::write(Archive& archive, MemChunk& mc)
{
	// Only two bytes are used for storing entry amount,
	// so abort for excessively large files:
	if (archive.numEntries() > 65535)
		return false;

	uint16_t      num_files  = archive.numEntries();
	uint32_t      dir_offset = 0;
	ArchiveEntry* entry;
	for (uint16_t l = 0; l < num_files; l++)
	{
		entry = archive.entryAt(l);
		entry->setOffsetOnDisk(dir_offset);
		entry->setSizeOnDisk();
		dir_offset += entry->size();
	}

	// Clear/init MemChunk
	mc.clear();
	mc.seek(0, SEEK_SET);
	mc.reSize(2 + dir_offset + num_files * 21);

	// Write the files
	for (uint16_t l = 0; l < num_files; l++)
	{
		entry = archive.entryAt(l);
		mc.write(entry->rawData(), entry->size());
	}

	// Write the directory
	for (uint16_t l = 0; l < num_files; l++)
	{
		entry         = archive.entryAt(l);
		char name[13] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		long offset   = wxINT32_SWAP_ON_BE(entry->offsetOnDisk());
		long size     = wxINT32_SWAP_ON_BE(entry->size());

		for (size_t c = 0; c < entry->name().length() && c < 12; c++)
			name[c] = entry->name()[c];

		mc.write(&size, 4);   // Size
		mc.write(&offset, 4); // Offset
		mc.write(name, 13);   // Name

		entry->setState(EntryState::Unmodified);
	}

	// Write the footer
	num_files = wxINT16_SWAP_ON_BE(num_files);
	mc.write(&num_files, 2);

	// Finished!
	return true;
}

// -----------------------------------------------------------------------------
// Checks if the given data is a valid Shadowcaster lib archive
// -----------------------------------------------------------------------------
bool LibArchiveHandler::isThisFormat(const MemChunk& mc)
{
	if (mc.size() < 64)
		return false;

	// Read lib footer
	mc.seek(2, SEEK_END);
	uint32_t num_lumps = 0;
	mc.read(&num_lumps, 2); // Size
	num_lumps          = wxINT16_SWAP_ON_BE(num_lumps);
	int32_t dir_offset = mc.size() - (2 + (num_lumps * 21));

	// Check directory offset is valid
	if (dir_offset < 0)
		return false;

	// Check directory offset is decent
	mc.seek(dir_offset, SEEK_SET);
	char     myname[13] = "";
	uint32_t offset     = 0;
	uint32_t size       = 0;
	uint8_t  dummy      = 0;
	mc.read(&size, 4);   // Size
	mc.read(&offset, 4); // Offset
	mc.read(myname, 12); // Name
	mc.read(&dummy, 1);  // Separator
	offset     = wxINT32_SWAP_ON_BE(offset);
	size       = wxINT32_SWAP_ON_BE(size);
	myname[12] = '\0';

	// If the lump data goes past the directory,
	// the library is invalid
	if (dummy != 0 || offset != 0 || offset + size > mc.size())
	{
		return false;
	}

	// Check that the file name given for the first lump is acceptable
	int filnamlen = 0;
	for (; filnamlen < 13; ++filnamlen)
	{
		if (myname[filnamlen] == 0)
			break;
		if (myname[filnamlen] < 33 || myname[filnamlen] > 126 || myname[filnamlen] == '"' || myname[filnamlen] == '*'
			|| myname[filnamlen] == '/' || myname[filnamlen] == ':' || myname[filnamlen] == '<'
			|| myname[filnamlen] == '?' || myname[filnamlen] == '\\' || myname[filnamlen] == '|')
			return false;
	}
	// At a minimum, one character for the name and the dot separating it from the extension
	if (filnamlen < 2)
		return false;

	// If it's passed to here it's probably a lib file
	return true;
}

// -----------------------------------------------------------------------------
// Checks if the file at [filename] is a valid Shadowcaster lib archive
// -----------------------------------------------------------------------------
bool LibArchiveHandler::isThisFormat(const string& filename)
{
	// Open file for reading
	wxFile file(filename);

	// Check it opened ok
	if (!file.IsOpened())
		return false;
	// Read lib footer
	file.Seek(file.Length() - 2, wxFromStart); // wxFromEnd does not work for some reason
	uint32_t num_lumps = 0;
	file.Read(&num_lumps, 2); // Size
	num_lumps          = wxINT16_SWAP_ON_BE(num_lumps);
	int32_t dir_offset = file.Length() - (2 + (num_lumps * 21));

	// Check directory offset is valid
	if (dir_offset < 0)
		return false;

	// Check directory offset is decent
	file.Seek(dir_offset, wxFromStart);
	char     myname[13] = "";
	uint32_t offset     = 0;
	uint32_t size       = 0;
	uint8_t  dummy      = 0;
	file.Read(&size, 4);   // Size
	file.Read(&offset, 4); // Offset
	file.Read(myname, 12); // Name
	file.Read(&dummy, 1);  // Separator
	offset     = wxINT32_SWAP_ON_BE(offset);
	size       = wxINT32_SWAP_ON_BE(size);
	myname[12] = '\0';

	// If the lump data goes past the directory,
	// the library is invalid
	if (dummy != 0 || offset != 0 || offset + size > file.Length())
	{
		return false;
	}
	// Check that the file name given for the first lump is acceptable
	int filnamlen = 0;
	for (; filnamlen < 13; ++filnamlen)
	{
		if (myname[filnamlen] == 0)
			break;
		if (myname[filnamlen] < 33 || myname[filnamlen] > 126 || myname[filnamlen] == '"' || myname[filnamlen] == '*'
			|| myname[filnamlen] == '/' || myname[filnamlen] == ':' || myname[filnamlen] == '<'
			|| myname[filnamlen] == '?' || myname[filnamlen] == '\\' || myname[filnamlen] == '|')
			return false;
	}
	// At a minimum, one character for the name and the dot separating it from the extension
	if (filnamlen == 0)
		return false;

	// If it's passed to here it's probably a lib file
	return true;
}
