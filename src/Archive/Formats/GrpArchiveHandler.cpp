
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    GrpArchiveHandler.cpp
// Description: ArchiveFormatHandler for Build engine GRP archives (Duke3d etc.)
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
#include "GrpArchiveHandler.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "General/UI.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// GrpArchiveHandler Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Reads grp format data from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool GrpArchiveHandler::open(Archive& archive, const MemChunk& mc, bool detect_types)
{
	// Check data was given
	if (!mc.hasData())
		return false;

	// Read grp header
	uint32_t num_lumps     = 0;
	char     ken_magic[13] = "";
	mc.seek(0, SEEK_SET);
	mc.read(ken_magic, 12); // "KenSilverman"
	mc.read(&num_lumps, 4); // No. of lumps in grp

	// Byteswap values for big endian if needed
	num_lumps = wxINT32_SWAP_ON_BE(num_lumps);

	// Null-terminate the magic header
	ken_magic[12] = 0;

	// Check the header
	if (string{ ken_magic } != "KenSilverman")
	{
		log::error("GrpArchiveHandler::openFile: File {} has invalid header", archive.filename());
		global::error = "Invalid grp header";
		return false;
	}

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	ArchiveModSignalBlocker sig_blocker{ archive };

	// The header takes as much space as a directory entry
	uint32_t entryoffset = 16 * (1 + num_lumps);

	// Read the directory
	ui::setSplashProgressMessage("Reading grp archive data");
	for (uint32_t d = 0; d < num_lumps; d++)
	{
		// Update splash window progress
		ui::setSplashProgress(d, num_lumps);

		// Read lump info
		char     name[13] = "";
		uint32_t offset   = entryoffset;
		uint32_t size     = 0;

		mc.read(name, 12); // Name
		mc.read(&size, 4); // Size
		name[12] = '\0';

		// Byteswap values for big endian if needed
		size = wxINT32_SWAP_ON_BE(size);

		// Increase offset of next entry by this entry's size
		entryoffset += size;

		// If the lump data goes past the end of the file,
		// the grpfile is invalid
		if (offset + size > mc.size())
		{
			log::error("GrpArchiveHandler::open: grp archive is invalid or corrupt");
			global::error = "Archive is invalid and/or corrupt";
			return false;
		}

		// Create & setup lump
		auto nlump = std::make_shared<ArchiveEntry>(name, size);
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
	if (detect_types)
		archive.detectAllEntryTypes();

	// Setup variables
	sig_blocker.unblock();
	archive.setModified(false);

	ui::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Writes the grp archive to a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool GrpArchiveHandler::write(Archive& archive, MemChunk& mc)
{
	// Clear/init MemChunk
	mc.clear();
	mc.seek(0, SEEK_SET);
	mc.reSize((1 + archive.numEntries()) * 16);
	ArchiveEntry* entry;

	// Write the header
	uint32_t num_lumps = archive.numEntries();
	mc.write("KenSilverman", 12);
	mc.write(&num_lumps, 4);

	// Write the directory
	uint32_t offset = 16 * (1 + num_lumps);
	for (uint32_t l = 0; l < num_lumps; l++)
	{
		entry         = archive.entryAt(l);
		char name[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		long size     = entry->size();

		for (size_t c = 0; c < entry->name().length() && c < 12; c++)
			name[c] = entry->name()[c];

		mc.write(name, 12);
		mc.write(&size, 4);

		entry->setState(EntryState::Unmodified);
		entry->setOffsetOnDisk(offset);
		entry->setSizeOnDisk();

		offset += size;
	}

	// Write the lumps
	for (uint32_t l = 0; l < num_lumps; l++)
	{
		entry = archive.entryAt(l);
		mc.write(entry->rawData(), entry->size());
	}

	return true;
}

// -----------------------------------------------------------------------------
// Checks if the given data is a valid Duke Nukem 3D grp archive
// -----------------------------------------------------------------------------
bool GrpArchiveHandler::isThisFormat(const MemChunk& mc)
{
	// Check size
	if (mc.size() < 16)
		return false;

	// Get number of lumps
	uint32_t num_lumps     = 0;
	char     ken_magic[13] = "";
	mc.seek(0, SEEK_SET);
	mc.read(ken_magic, 12); // "KenSilverman"
	mc.read(&num_lumps, 4); // No. of lumps in grp

	// Byteswap values for big endian if needed
	num_lumps = wxINT32_SWAP_ON_BE(num_lumps);

	// Null-terminate the magic header
	ken_magic[12] = 0;

	// Check the header
	if (string{ ken_magic } != "KenSilverman")
		return false;

	// Compute total size
	uint32_t totalsize = (1 + num_lumps) * 16;
	uint32_t size      = 0;
	for (uint32_t a = 0; a < num_lumps; ++a)
	{
		mc.read(ken_magic, 12);
		mc.read(&size, 4);
		totalsize += size;
	}

	// Check if total size is correct
	if (totalsize > mc.size())
		return false;

	// If it's passed to here it's probably a grp file
	return true;
}

// -----------------------------------------------------------------------------
// Checks if the file at [filename] is a valid DN3D grp archive
// -----------------------------------------------------------------------------
bool GrpArchiveHandler::isThisFormat(const string& filename)
{
	// Open file for reading
	wxFile file(filename);

	// Check it opened ok
	if (!file.IsOpened())
		return false;

	// Check size
	if (file.Length() < 16)
		return false;

	// Get number of lumps
	uint32_t num_lumps     = 0;
	char     ken_magic[13] = "";
	file.Seek(0, wxFromStart);
	file.Read(ken_magic, 12); // "KenSilverman"
	file.Read(&num_lumps, 4); // No. of lumps in grp

	// Byteswap values for big endian if needed
	num_lumps = wxINT32_SWAP_ON_BE(num_lumps);

	// Null-terminate the magic header
	ken_magic[12] = 0;

	// Check the header
	if (string{ ken_magic } != "KenSilverman")
		return false;

	// Compute total size
	uint32_t totalsize = (1 + num_lumps) * 16;
	uint32_t size      = 0;
	for (uint32_t a = 0; a < num_lumps; ++a)
	{
		file.Read(ken_magic, 12);
		file.Read(&size, 4);
		totalsize += size;
	}

	// Check if total size is correct
	if (totalsize > file.Length())
		return false;

	// If it's passed to here it's probably a grp file
	return true;
}


// -----------------------------------------------------------------------------
//
// Console Commands
//
// -----------------------------------------------------------------------------
#include "General/Console.h"
#include "MainEditor/MainEditor.h"

CONSOLE_COMMAND(lookupdat, 0, false)
{
	auto entry = maineditor::currentEntry();

	if (!entry)
		return;

	auto& mc = entry->data();
	if (mc.size() == 0)
		return;

	int index = entry->parent()->entryIndex(entry, entry->parentDir());
	mc.seek(0, SEEK_SET);

	// Create lookup table
	uint8_t numlookup = 0;
	uint8_t dummy     = 0;
	mc.read(&numlookup, 1);
	if (mc.size() < static_cast<uint32_t>((numlookup * 256) + (5 * 768) + 1))
		return;

	auto nentry = entry->parent()->addNewEntry("COLORMAP.DAT", index + 1, entry->parentDir());
	if (!nentry)
		return;

	auto data = new uint32_t[numlookup * 256];
	for (int i = 0; i < numlookup; ++i)
	{
		mc.read(&dummy, 1);
		mc.read(data, numlookup * 256);
	}
	nentry->importMem(data, numlookup * 256);
	delete[] data;

	// Create extra palettes
	data   = new uint32_t[768];
	nentry = entry->parent()->addNewEntry("WATERPAL.PAL", index + 2, entry->parentDir());
	if (!nentry)
	{
		delete[] data;
		return;
	}
	mc.read(data, 768);
	nentry->importMem(data, 768);
	nentry = entry->parent()->addNewEntry("SLIMEPAL.PAL", index + 3, entry->parentDir());
	if (!nentry)
	{
		delete[] data;
		return;
	}
	mc.read(data, 768);
	nentry->importMem(data, 768);
	nentry = entry->parent()->addNewEntry("TITLEPAL.PAL", index + 4, entry->parentDir());
	if (!nentry)
	{
		delete[] data;
		return;
	}
	mc.read(data, 768);
	nentry->importMem(data, 768);
	nentry = entry->parent()->addNewEntry("3DREALMS.PAL", index + 5, entry->parentDir());
	if (!nentry)
	{
		delete[] data;
		return;
	}
	mc.read(data, 768);
	nentry->importMem(data, 768);
	nentry = entry->parent()->addNewEntry("ENDINPAL.PAL", index + 6, entry->parentDir());
	if (!nentry)
	{
		delete[] data;
		return;
	}
	mc.read(data, 768);
	nentry->importMem(data, 768);

	// Clean up and go away
	delete[] data;
}

CONSOLE_COMMAND(palettedat, 0, false)
{
	auto entry = maineditor::currentEntry();

	if (!entry)
		return;

	auto& mc = entry->data();
	// Minimum size: 768 bytes for the palette, 2 for the number of lookup tables,
	// 0 for these tables if there are none, and 65536 for the transparency map.
	if (mc.size() < 66306)
		return;

	int index = entry->parent()->entryIndex(entry, entry->parentDir());
	mc.seek(0, SEEK_SET);

	// Create palette
	auto data   = new uint32_t[768];
	auto nentry = entry->parent()->addNewEntry("MAINPAL.PAL", index + 1, entry->parentDir());
	if (!nentry)
		return;
	mc.read(data, 768);
	nentry->importMem(data, 768);

	// Create lookup tables
	uint16_t numlookup = 0;
	mc.read(&numlookup, 2);
	numlookup = wxINT16_SWAP_ON_BE(numlookup);
	delete[] data;
	nentry = entry->parent()->addNewEntry("COLORMAP.DAT", index + 2, entry->parentDir());
	if (!nentry)
		return;
	data = new uint32_t[numlookup * 256];
	mc.read(data, numlookup * 256);
	nentry->importMem(data, numlookup * 256);

	// Create transparency tables
	delete[] data;
	nentry = entry->parent()->addNewEntry("TRANMAP.DAT", index + 3, entry->parentDir());
	if (!nentry)
		return;
	data = new uint32_t[65536];
	mc.read(data, 65536);
	nentry->importMem(data, 65536);

	// Clean up and go away
	delete[] data;
}

CONSOLE_COMMAND(tablesdat, 0, false)
{
	auto entry = maineditor::currentEntry();

	if (!entry)
		return;

	auto& mc = entry->data();
	// Sin/cos table: 4096; atn table 1280; gamma table 1024
	// Fonts: 1024 byte each.
	if (mc.size() != 8448)
		return;

	int index = entry->parent()->entryIndex(entry, entry->parentDir());
	mc.seek(5376, SEEK_SET);

	// Create fonts
	auto data   = new uint32_t[1024];
	auto nentry = entry->parent()->addNewEntry("VGAFONT1.FNT", index + 1, entry->parentDir());
	if (!nentry)
		return;
	mc.read(data, 1024);
	nentry->importMem(data, 1024);
	nentry = entry->parent()->addNewEntry("VGAFONT2.FNT", index + 2, entry->parentDir());
	if (!nentry)
		return;
	mc.read(data, 1024);
	nentry->importMem(data, 1024);

	// Clean up and go away
	delete[] data;
}
