
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    BSPArchiveHandler.cpp
// Description: ArchiveFormatHandler for the textures from the Quake 1 BSP
//              format (but not the rest)
//
// The only thing interesting in Quake BSP files is the texture collection.
// Quake 1 is the only game of the series to hold texture definitions in it,
// so even if the BSP formats of the other Quake engine/Source engine games
// are saner, it's not interesting for something that isn't a level editor
// for these games.
//
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
#include "BSPArchiveHandler.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "General/UI.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// BSPArchiveHandler Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Reads BSP format data from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool BSPArchiveHandler::open(Archive& archive, const MemChunk& mc, bool detect_types)
{
	// If size is less than 64, there's not even enough room for a full header
	size_t size = mc.size();
	if (size < 64)
	{
		log::error("BSPArchiveHandler::open: Opening failed, invalid header");
		global::error = "Invalid BSP header";
		return false;
	}

	// Read BSP version; valid values are 0x17 (Qtest) or 0x1D (Quake, Hexen II)
	uint32_t version;
	uint32_t texoffset = 0;
	uint32_t texsize;
	mc.seek(0, SEEK_SET);
	mc.read(&version, 4);
	version = wxINT32_SWAP_ON_BE(version);
	if (version != 0x17 && version != 0x1D)
	{
		log::error("BSPArchiveHandler::open: Opening failed, unknown BSP version");
		global::error = "Unknown BSP version";
		return false;
	}

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	ArchiveModSignalBlocker sig_blocker{ archive };

	// Validate directory to make sure it's the correct format.
	// This mean checking each of the 15 entries, even if only
	// the third has content we want.
	for (int a = 0; a < 15; ++a)
	{
		uint32_t ofs, sz;
		mc.read(&ofs, 4);
		mc.read(&sz, 4);

		// Check that content stays within bounds
		if (wxINT32_SWAP_ON_BE(sz) + wxINT32_SWAP_ON_BE(ofs) > size)
		{
			log::error("BSPArchiveHandler::open: Opening failed, invalid header (data out of bounds)");
			global::error = "Invalid BSP header";
			return false;
		}
		// Grab the miptex entry data
		if (a == 2)
		{
			texoffset = wxINT32_SWAP_ON_BE(ofs);
			texsize   = wxINT32_SWAP_ON_BE(sz);

			// If there are no textures, no need to bother
			if (texsize == 0)
			{
				log::error("BSPArchiveHandler::open: Opening failed, no texture");
				global::error = "No texture content";
				return false;
			}
		}
	}

	// Read the miptex directory
	uint32_t numtex;
	mc.seek(texoffset, wxFromStart);
	mc.read(&numtex, 4);
	numtex = wxINT32_SWAP_ON_BE(numtex);
	ui::setSplashProgressMessage("Reading BSP texture data");

	// Check that the offset table is within bounds
	if (texoffset + ((numtex + 1) << 2) > size)
	{
		log::error("BSPArchiveHandler::open: Opening failed, miptex entry out of bounds");
		global::error = "Out of bounds";
		return false;
	}

	// Check that each texture is within bounds
	for (size_t a = 0; a < numtex; ++a)
	{
		// Update splash window progress
		ui::setSplashProgress(a, numtex);

		size_t offset;
		mc.read(&offset, 4);
		offset = wxINT32_SWAP_ON_BE(offset);

		// Skip entries with an offset of -1. (No, I don't know why they are included at all.)
		if (offset != 0xFFFFFFFF)
		{
			// A texture header takes 40 bytes (16 bytes for name, 6 int32 for records),
			// and offsets are measured from the start of the miptex lump.
			if (offset + texoffset + 40 > size)
				return false;

			// Keep track of where we are now to return to it later.
			size_t currentpos = mc.currentPos();

			// Move to texture header
			mc.seek(texoffset + offset, SEEK_SET);
			char     name[17];
			uint32_t width, height, offset1, offset2, offset4, offset8;
			mc.read(name, 16);
			mc.read(&width, 4);
			mc.read(&height, 4);
			mc.read(&offset1, 4);
			mc.read(&offset2, 4);
			mc.read(&offset4, 4);
			mc.read(&offset8, 4);

			// Byteswap values for big endian if needed
			width   = wxINT32_SWAP_ON_BE(width);
			height  = wxINT32_SWAP_ON_BE(height);
			offset1 = wxINT32_SWAP_ON_BE(offset1);
			offset2 = wxINT32_SWAP_ON_BE(offset2);
			offset4 = wxINT32_SWAP_ON_BE(offset4);
			offset8 = wxINT32_SWAP_ON_BE(offset8);


			// Cap texture name if needed and clean out garbage characters
			bool nameend = false;
			for (size_t d = 1; d < 17; ++d)
			{
				if (name[d] == 0 || d == 16)
					nameend = true;
				if (nameend)
					name[d] = 0;
			}

			// Dimensions must be multiples of 8 but cannot be null
			if (width % 8 | height % 8 || width == 0 || height == 0)
				return false;

			// Check that texture data is within bounds
			uint32_t tsize = width * height;
			if (texoffset + offset + offset1 + tsize > size)
				return false;
			if (texoffset + offset + offset2 + (tsize >> 2) > size)
				return false;
			if (texoffset + offset + offset4 + (tsize >> 4) > size)
				return false;
			if (texoffset + offset + offset8 + (tsize >> 6) > size)
				return false;

			uint32_t lumpsize = 40 + tsize + (tsize >> 2) + (tsize >> 4) + (tsize >> 6);


			// Create & setup lump
			auto nlump = std::make_shared<ArchiveEntry>(name, lumpsize);
			nlump->setSizeOnDisk(lumpsize);
			nlump->setOffsetOnDisk(offset + texoffset);
			nlump->importMemChunk(mc, offset + texoffset, lumpsize);
			nlump->setState(EntryState::Unmodified);

			// Add to entry list
			archive.rootDir()->addEntry(nlump);

			// Okay, that texture works, go back to where we were and check the next
			mc.seek(currentpos, SEEK_SET);
		}
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
// Writes the BSP archive to a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool BSPArchiveHandler::write(Archive& archive, MemChunk& mc)
{
	global::error = "Sorry, not implemented";
	return false;
}


// -----------------------------------------------------------------------------
//
// BSPArchiveHandler Class Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Checks if the given data is a valid Quake BSP archive
// -----------------------------------------------------------------------------
bool BSPArchiveHandler::isThisFormat(const MemChunk& mc)
{
	// If size is less than 64, there's not even enough room for a full header
	size_t size = mc.size();
	if (size < 64)
		return false;

	// Read BSP version; valid values are 0x17 (Qtest) or 0x1D (Quake, Hexen II)
	uint32_t version;
	uint32_t texoffset = 0;
	uint32_t texsize;
	mc.seek(0, SEEK_SET);
	mc.read(&version, 4);
	version = wxINT32_SWAP_ON_BE(version);
	if (version != 0x17 && version != 0x1D)
		return false;

	// Validate directory to make sure it's the correct format.
	// This mean checking each of the 15 entries, even if only
	// the third has content we want.
	for (int a = 0; a < 15; ++a)
	{
		uint32_t ofs, sz;
		mc.read(&ofs, 4);
		mc.read(&sz, 4);

		// Check that content stays within bounds
		if (wxINT32_SWAP_ON_BE(sz) + wxINT32_SWAP_ON_BE(ofs) > size)
			return false;
		// Grab the miptex entry data
		if (a == 2)
		{
			texoffset = wxINT32_SWAP_ON_BE(ofs);
			texsize   = wxINT32_SWAP_ON_BE(sz);

			// If there are no textures, no need to bother
			if (texsize == 0)
				return false;
		}
	}

	// Now validate miptex entry
	uint32_t numtex;
	mc.seek(texoffset, wxFromStart);
	mc.read(&numtex, 4);
	numtex = wxINT32_SWAP_ON_BE(numtex);

	// Check that the offset table is within bounds
	if (texoffset + ((numtex + 1) << 2) > size)
		return false;

	// Check that each texture is within bounds
	for (size_t a = 0; a < numtex; ++a)
	{
		size_t offset;
		mc.read(&offset, 4);
		offset = wxINT32_SWAP_ON_BE(offset);

		// A texture header takes 40 bytes (16 bytes for name, 6 int32 for records),
		// and offsets are measured from the start of the miptex lump.
		if (offset + texoffset + 40 > size)
			return false;

		if (offset != 0xFFFFFFFF)
		{
			// Keep track of where we are now to return to it later.
			size_t currentpos = mc.currentPos();

			// Move to texture header
			mc.seek(texoffset + offset, SEEK_SET);
			char     name[16];
			uint32_t width, height, offset1, offset2, offset4, offset8;
			mc.read(name, 16);
			mc.read(&width, 4);
			mc.read(&height, 4);
			mc.read(&offset1, 4);
			mc.read(&offset2, 4);
			mc.read(&offset4, 4);
			mc.read(&offset8, 4);

			// Byteswap values for big endian if needed
			width   = wxINT32_SWAP_ON_BE(width);
			height  = wxINT32_SWAP_ON_BE(height);
			offset1 = wxINT32_SWAP_ON_BE(offset1);
			offset2 = wxINT32_SWAP_ON_BE(offset2);
			offset4 = wxINT32_SWAP_ON_BE(offset4);
			offset8 = wxINT32_SWAP_ON_BE(offset8);

			// Dimensions must be multiples of 8 but cannot be null
			if (width % 8 | height % 8 || width == 0 || height == 0)
				return false;

			// Check that texture data is within bounds
			uint32_t tsize = width * height;
			if (texoffset + offset + offset1 + tsize > size)
				return false;
			if (texoffset + offset + offset2 + (tsize >> 2) > size)
				return false;
			if (texoffset + offset + offset4 + (tsize >> 4) > size)
				return false;
			if (texoffset + offset + offset8 + (tsize >> 6) > size)
				return false;

			// Okay, that texture works, go back to where we were and check the next
			mc.seek(currentpos, SEEK_SET);
		}
	}

	// That'll do
	return true;
}

// -----------------------------------------------------------------------------
// Checks if the file at [filename] is a valid Quake BSP archive
// -----------------------------------------------------------------------------
bool BSPArchiveHandler::isThisFormat(const string& filename)
{
	// Open file for reading
	wxFile file(filename);

	// Check it opened ok
	if (!file.IsOpened())
		return false;

	// If size is less than 64, there's not even enough room for a full header
	size_t size = file.Length();
	if (size < 64)
		return false;

	// Read BSP version; valid values are 0x17 (Qtest) or 0x1D (Quake, Hexen II)
	uint32_t version;
	uint32_t texoffset = 0;
	file.Seek(0, wxFromStart);
	file.Read(&version, 4);
	version = wxINT32_SWAP_ON_BE(version);
	if (version != 0x17 && version != 0x1D)
		return false;

	// Validate directory to make sure it's the correct format.
	// This mean checking each of the 15 entries, even if only
	// the third has content we want.
	for (int a = 0; a < 15; ++a)
	{
		uint32_t ofs, sz;
		file.Read(&ofs, 4);
		file.Read(&sz, 4);

		// Check that content stays within bounds
		if (wxINT32_SWAP_ON_BE(sz) + wxINT32_SWAP_ON_BE(ofs) > size)
			return false;
		// Grab the miptex entry data
		if (a == 2)
			texoffset = wxINT32_SWAP_ON_BE(ofs);
	}

	// Now validate miptex entry
	uint32_t numtex;
	file.Seek(texoffset, wxFromStart);
	file.Read(&numtex, 4);
	numtex = wxINT32_SWAP_ON_BE(numtex);

	// Check that the offset table is within bounds
	if (texoffset + ((numtex + 1) << 2) > size)
		return false;

	// Check that each texture is within bounds
	for (size_t a = 0; a < numtex; ++a)
	{
		size_t offset;
		file.Read(&offset, 4);
		offset = wxINT32_SWAP_ON_BE(offset);
		// A texture header takes 40 bytes (16 bytes for name, 6 int32 for records),
		// and offsets are measured from the start of the miptex lump.
		if (offset + texoffset + 40 > size)
			return false;

		// Keep track of where we are now to return to it later.
		size_t currentpos = file.Tell();

		// Move to texture header
		file.Seek(texoffset + offset, wxFromStart);
		char     name[16];
		uint32_t width, height, offset1, offset2, offset4, offset8;
		file.Read(name, 16);
		file.Read(&width, 4);
		file.Read(&height, 4);
		file.Read(&offset1, 4);
		file.Read(&offset2, 4);
		file.Read(&offset4, 4);
		file.Read(&offset8, 4);

		// Byteswap values for big endian if needed
		width   = wxINT32_SWAP_ON_BE(width);
		height  = wxINT32_SWAP_ON_BE(height);
		offset1 = wxINT32_SWAP_ON_BE(offset1);
		offset2 = wxINT32_SWAP_ON_BE(offset2);
		offset4 = wxINT32_SWAP_ON_BE(offset4);
		offset8 = wxINT32_SWAP_ON_BE(offset8);

		// Dimensions must be multiples of 8 but cannot be null
		if (width % 8 | height % 8 || width == 0 || height == 0)
			return false;

		// Check that texture data is within bounds
		uint32_t tsize = width * height;
		if (texoffset + offset + offset1 + tsize > size)
			return false;
		if (texoffset + offset + offset2 + (tsize >> 2) > size)
			return false;
		if (texoffset + offset + offset4 + (tsize >> 4) > size)
			return false;
		if (texoffset + offset + offset8 + (tsize >> 6) > size)
			return false;

		// Okay, that texture works, go back to where we were and check the next
		file.Seek(currentpos, wxFromStart);
	}

	// That'll do
	return true;
}
