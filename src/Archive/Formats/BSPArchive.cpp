
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    BSPArchive.cpp
 * Description: BSPArchive, archive class to handle the textures
 *              from the Quake 1 BSP format (but not the rest)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "BSPArchive.h"
#include "General/UI.h"

/*

The only thing interesting in Quake BSP files is the texture collection.
Quake 1 is the only game of the series to hold texture definitions in it,
so even if the BSP formats of the other Quake engine/Source engine games
are saner, it's not interesting for something that isn't a level editor
for these games.
*/

/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, archive_load_data)

/*******************************************************************
 * BSPARCHIVE CLASS FUNCTIONS
 *******************************************************************/

/* BSPArchive::BSPArchive
 * BSPArchive class constructor
 *******************************************************************/
BSPArchive::BSPArchive() : Archive(ARCHIVE_BSP)
{
}

/* BSPArchive::~BSPArchive
 * BSPArchive class destructor
 *******************************************************************/
BSPArchive::~BSPArchive()
{
}

/* WadArchive::getEntryOffset
 * Returns the file byte offset for [entry]
 *******************************************************************/
uint32_t BSPArchive::getEntryOffset(ArchiveEntry* entry)
{
	// Check entry
	if (!checkEntry(entry))
		return 0;

	return (uint32_t)(int)entry->exProp("Offset");
}

/* BSPArchive::getFileExtensionString
 * Returns the file extension string to use in the file open dialog
 *******************************************************************/
string BSPArchive::getFileExtensionString()
{
	return "BSP Files (*.bsp)|*.bsp";
}

/* BSPArchive::getFormat
 * Returns the string id for the pak EntryDataFormat
 *******************************************************************/
string BSPArchive::getFormat()
{
	return "archive_bsp";
}

/* BSPArchive::open
 * Reads BSP format data from a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool BSPArchive::open(MemChunk& mc)
{
	// If size is less than 64, there's not even enough room for a full header
	size_t size = mc.getSize();
	if (size < 64)
	{
		LOG_MESSAGE(1, "BSPArchive::open: Opening failed, invalid header");
		Global::error = "Invalid BSP header";
		return false;
	}

	// Read BSP version; valid values are 0x17 (Qtest) or 0x1D (Quake, Hexen II)
	uint32_t version;
	uint32_t texoffset;
	uint32_t texsize;
	mc.seek(0, SEEK_SET);
	mc.read(&version, 4);
	version = wxINT32_SWAP_ON_BE(version);
	if (version != 0x17 && version != 0x1D)
	{
		LOG_MESSAGE(1, "BSPArchive::open: Opening failed, unknown BSP version");
		Global::error = "Unknown BSP version";
		return false;
	}

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	setMuted(true);

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
			LOG_MESSAGE(1, "BSPArchive::open: Opening failed, invalid header (data out of bounds)");
			Global::error = "Invalid BSP header";
			return false;
		}
		// Grab the miptex entry data
		if (a == 2)
		{
			texoffset = wxINT32_SWAP_ON_BE(ofs);
			texsize = wxINT32_SWAP_ON_BE(sz);

			// If there are no textures, no need to bother
			if (texsize == 0)
			{
				LOG_MESSAGE(1, "BSPArchive::open: Opening failed, no texture");
				Global::error = "No texture content";
				return false;
			}
		}
	}

	// Read the miptex directory
	uint32_t numtex;
	mc.seek(texoffset, wxFromStart);
	mc.read(&numtex, 4);
	numtex = wxINT32_SWAP_ON_BE(numtex);
	UI::setSplashProgressMessage("Reading BSP texture data");

	// Check that the offset table is within bounds
	if (texoffset + ((numtex + 1)<<2) > size)
	{
		LOG_MESSAGE(1, "BSPArchive::open: Opening failed, miptex entry out of bounds");
		Global::error = "Out of bounds";
		return false;
	}

	// Check that each texture is within bounds
	for (size_t a = 0; a < numtex; ++a)
	{
		// Update splash window progress
		UI::setSplashProgress(((float)a / (float)numtex));

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
			char name[17];
			uint32_t width, height, offset1, offset2, offset4, offset8;
			mc.read(name, 16);
			mc.read(&width, 4);
			mc.read(&height, 4);
			mc.read(&offset1, 4);
			mc.read(&offset2, 4);
			mc.read(&offset4, 4);
			mc.read(&offset8, 4);

			// Byteswap values for big endian if needed
			width	= wxINT32_SWAP_ON_BE(width);
			height	= wxINT32_SWAP_ON_BE(height);
			offset1	= wxINT32_SWAP_ON_BE(offset1);
			offset2	= wxINT32_SWAP_ON_BE(offset2);
			offset4	= wxINT32_SWAP_ON_BE(offset4);
			offset8	= wxINT32_SWAP_ON_BE(offset8);


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
			uint32_t texsize = width * height;
			if (texoffset + offset + offset1 + texsize > size)
				return false;
			if (texoffset + offset + offset2 + (texsize >> 2) > size)
				return false;
			if (texoffset + offset + offset4 + (texsize >> 4) > size)
				return false;
			if (texoffset + offset + offset8 + (texsize >> 6) > size)
				return false;

			uint32_t lumpsize = 40 + texsize + (texsize >> 2) + (texsize >> 4) + (texsize >> 6);


			// Create & setup lump
			ArchiveEntry* nlump = new ArchiveEntry(wxString::FromAscii(name), lumpsize);
			nlump->setLoaded(false);
			nlump->exProp("Offset") = (int)(offset + texoffset);
			nlump->setState(0);

			// Add to entry list
			getRoot()->addEntry(nlump);

			// Okay, that texture works, go back to where we were and check the next
			mc.seek(currentpos, SEEK_SET);
		}
	}

	// Detect all entry types
	MemChunk edata;
	UI::setSplashProgressMessage("Detecting entry types");
	for (size_t a = 0; a < numEntries(); a++)
	{
		// Update splash window progress
		UI::setSplashProgress((((float)a / (float)numtex)));

		// Get entry
		ArchiveEntry* entry = getEntry(a);

		// Read entry data if it isn't zero-sized
		if (entry->getSize() > 0)
		{
			// Read the entry data
			mc.exportMemChunk(edata, getEntryOffset(entry), entry->getSize());
			entry->importMemChunk(edata);
		}

		// Detect entry type
		EntryType::detectEntryType(entry);

		// Unload entry data if needed
		if (!archive_load_data)
			entry->unloadData();

		// Set entry to unchanged
		entry->setState(0);
	}

	// Setup variables
	setMuted(false);
	setModified(false);
	announce("opened");

	UI::setSplashProgressMessage("");

	return true;
}

/* BSPArchive::write
 * Writes the BSP archive to a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool BSPArchive::write(MemChunk& mc, bool update)
{
	Global::error = "Sorry, not implemented";
	return false;
}

/* BSPArchive::loadEntryData
 * Loads an entry's data from the pak file
 * Returns true if successful, false otherwise
 *******************************************************************/
bool BSPArchive::loadEntryData(ArchiveEntry* entry)
{
	// Check entry is ok
	if (!checkEntry(entry))
		return false;

	// Do nothing if the entry's size is zero,
	// or if it has already been loaded
	if (entry->getSize() == 0 || entry->isLoaded())
	{
		entry->setLoaded();
		return true;
	}

	// Open archive file
	wxFile file(filename);

	// Check it opened
	if (!file.IsOpened())
	{
		LOG_MESSAGE(1, "BSPArchive::loadEntryData: Unable to open archive file %s", filename);
		return false;
	}

	// Seek to entry offset in file and read it in
	file.Seek((int)entry->exProp("Offset"), wxFromStart);
	entry->importFileStream(file, entry->getSize());

	// Set the lump to loaded
	entry->setLoaded();

	return true;
}


/*******************************************************************
 * BSPARCHIVE CLASS STATIC FUNCTIONS
 *******************************************************************/

/* BSPArchive::isBSPArchive
 * Checks if the given data is a valid Quake BSP archive
 *******************************************************************/
bool BSPArchive::isBSPArchive(MemChunk& mc)
{
	// If size is less than 64, there's not even enough room for a full header
	size_t size = mc.getSize();
	if (size < 64)
		return false;

	// Read BSP version; valid values are 0x17 (Qtest) or 0x1D (Quake, Hexen II)
	uint32_t version;
	uint32_t texoffset;
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
			texsize = wxINT32_SWAP_ON_BE(sz);

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
	if (texoffset + ((numtex + 1)<<2) > size)
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
			char name[16];
			uint32_t width, height, offset1, offset2, offset4, offset8;
			mc.read(name, 16);
			mc.read(&width, 4);
			mc.read(&height, 4);
			mc.read(&offset1, 4);
			mc.read(&offset2, 4);
			mc.read(&offset4, 4);
			mc.read(&offset8, 4);

			// Byteswap values for big endian if needed
			width	= wxINT32_SWAP_ON_BE(width);
			height	= wxINT32_SWAP_ON_BE(height);
			offset1	= wxINT32_SWAP_ON_BE(offset1);
			offset2	= wxINT32_SWAP_ON_BE(offset2);
			offset4	= wxINT32_SWAP_ON_BE(offset4);
			offset8	= wxINT32_SWAP_ON_BE(offset8);

			// Dimensions must be multiples of 8 but cannot be null
			if (width % 8 | height % 8 || width == 0 || height == 0)
				return false;

			// Check that texture data is within bounds
			uint32_t texsize = width * height;
			if (texoffset + offset + offset1 + texsize > size)			return false;
			if (texoffset + offset + offset2 + (texsize >> 2) > size)	return false;
			if (texoffset + offset + offset4 + (texsize >> 4) > size)	return false;
			if (texoffset + offset + offset8 + (texsize >> 6) > size)	return false;

			// Okay, that texture works, go back to where we were and check the next
			mc.seek(currentpos, SEEK_SET);
		}
	}

	// That'll do
	return true;
}

/* BSPArchive::isBSPArchive
 * Checks if the file at [filename] is a valid Quake BSP archive
 *******************************************************************/
bool BSPArchive::isBSPArchive(string filename)
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
	uint32_t texoffset;
	uint32_t texsize;
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
		{
			texoffset = wxINT32_SWAP_ON_BE(ofs);
			texsize = wxINT32_SWAP_ON_BE(sz);
		}
	}

	// Now validate miptex entry
	uint32_t numtex;
	file.Seek(texoffset, wxFromStart);
	file.Read(&numtex, 4);
	numtex = wxINT32_SWAP_ON_BE(numtex);

	// Check that the offset table is within bounds
	if (texoffset + ((numtex + 1)<<2) > size)
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
		char name[16];
		uint32_t width, height, offset1, offset2, offset4, offset8;
		file.Read(name, 16);
		file.Read(&width, 4);
		file.Read(&height, 4);
		file.Read(&offset1, 4);
		file.Read(&offset2, 4);
		file.Read(&offset4, 4);
		file.Read(&offset8, 4);

		// Byteswap values for big endian if needed
		width	= wxINT32_SWAP_ON_BE(width);
		height	= wxINT32_SWAP_ON_BE(height);
		offset1	= wxINT32_SWAP_ON_BE(offset1);
		offset2	= wxINT32_SWAP_ON_BE(offset2);
		offset4	= wxINT32_SWAP_ON_BE(offset4);
		offset8	= wxINT32_SWAP_ON_BE(offset8);

		// Dimensions must be multiples of 8 but cannot be null
		if (width % 8 | height % 8 || width == 0 || height == 0)
			return false;

		// Check that texture data is within bounds
		uint32_t texsize = width * height;
		if (texoffset + offset + offset1 + texsize > size)
			return false;
		if (texoffset + offset + offset2 + (texsize >> 2) > size)
			return false;
		if (texoffset + offset + offset4 + (texsize >> 4) > size)
			return false;
		if (texoffset + offset + offset8 + (texsize >> 6) > size)
			return false;

		// Okay, that texture works, go back to where we were and check the next
		file.Seek(currentpos, wxFromStart);
	}

	// That'll do
	return true;
}
