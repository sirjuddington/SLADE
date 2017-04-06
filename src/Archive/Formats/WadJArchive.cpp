
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    WadJArchive.cpp
 * Description: WadJArchive, archive class to handle doom format
 *              wad archives in Big Endianness (jagdoom)
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
#include "MainEditor/UI/MainWindow.h"
#include "WadJArchive.h"
#include "General/UI.h"

/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, archive_load_data)
EXTERN_CVAR(Bool, iwad_lock)
extern string map_lumps[];

/*******************************************************************
 * WADJARCHIVE HELPER FUNCTIONS
 *******************************************************************/

// Taken and adapted from Jaguar Doom source code
#define LENSHIFT 4		/* this must be log2(LOOKAHEAD_SIZE) */
bool JaguarDecode(MemChunk& mc)
{
	bool okay = false;
	uint8_t getidbyte = 0;
	int len;
	int pos;
	int i;
	uint8_t* source;

	// Get data
	size_t isize = mc.getSize();
	const uint8_t* istart = mc.getData();
	const uint8_t* input = istart;
	const uint8_t* iend = input + isize;

	// It seems that encoded lumps are given their actual uncompressed size in the directory.
	uint8_t* ostart = new uint8_t[isize + 1];
	uint8_t* output = ostart;
	uint8_t* oend = output + isize + 1;
	uint8_t idbyte = 0;

	size_t length = 0;

	while ((input < iend) && (output < oend))
	{
		/* get a new idbyte if necessary */
		if (!getidbyte) idbyte = *input++;
		getidbyte = (getidbyte + 1) & 7;

		if (idbyte&1)
		{
			/* decompress */
			pos = *input++ << LENSHIFT;
			pos = pos | (*input >> LENSHIFT);
			source = output - pos - 1;
			len = (*input++ & 0xf)+1;
			if (len==1)
			{
				okay = true;
				break;
			}
			length += len;
			if (output + len > oend)
				break;

			for (i=0 ; i<len ; i++)
				*output++ = *source++;
		}
		else
		{
			length++;
			*output++ = *input++;
		}
		idbyte = idbyte >> 1;
	}
	// Finalize stuff
	size_t osize = output - ostart;
	//LOG_MESSAGE(1, "Input size = %d, used input = %d, computed length = %d, output size = %d", isize, input - istart, length, osize);
	mc.importMem(ostart, osize);
	delete[] ostart;
	return okay;
}

/*******************************************************************
 * WADJARCHIVE CLASS FUNCTIONS
 *******************************************************************/

/* WadJArchive::WadJArchive
 * WadJArchive class constructor
 *******************************************************************/
WadJArchive::WadJArchive()
	: WadArchive()
{
}

/* WadJArchive::~WadJArchive
 * WadJArchive class destructor
 *******************************************************************/
WadJArchive::~WadJArchive()
{
}

/* WadJArchive::open
 * Reads wad format data from a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool WadJArchive::open(MemChunk& mc)
{
	// Check data was given
	if (!mc.hasData())
		return false;

	// Read wad header
	uint32_t num_lumps = 0;
	uint32_t dir_offset = 0;
	mc.seek(0, SEEK_SET);
	mc.read(&wad_type, 4);		// Wad type
	mc.read(&num_lumps, 4);		// No. of lumps in wad
	mc.read(&dir_offset, 4);	// Offset to directory

	// Byteswap values for little endian
	num_lumps = wxINT32_SWAP_ON_LE(num_lumps);
	dir_offset = wxINT32_SWAP_ON_LE(dir_offset);

	// Check the header
	if (wad_type[1] != 'W' || wad_type[2] != 'A' || wad_type[3] != 'D')
	{
		LOG_MESSAGE(1, "WadJArchive::openFile: File %s has invalid header", filename);
		Global::error = "Invalid wad header";
		return false;
	}

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	setMuted(true);

	// Read the directory
	mc.seek(dir_offset, SEEK_SET);
	UI::setSplashProgressMessage("Reading wad archive data");
	for (uint32_t d = 0; d < num_lumps; d++)
	{
		// Update splash window progress
		UI::setSplashProgress(((float)d / (float)num_lumps));

		// Read lump info
		char name[9] = "";
		uint32_t offset = 0;
		uint32_t size = 0;

		mc.read(&offset, 4);	// Offset
		mc.read(&size, 4);		// Size
		mc.read(name, 8);		// Name
		name[8] = '\0';

		// Byteswap values for little endian
		offset = wxINT32_SWAP_ON_LE(offset);
		size = wxINT32_SWAP_ON_LE(size);

		// Is there a compression/encryption thing going on?
		bool jaguarencrypt = !!(name[0] & 0x80);	// look at high bit
		name[0] = name[0] & 0x7F;					// then strip it away

		// Look for encryption shenanigans
		size_t actualsize = size;
		if (jaguarencrypt)
		{
			if (d < num_lumps - 1)
			{
				size_t pos = mc.currentPos();
				uint32_t nextoffset = 0;
				for (int i = 0; i + d < num_lumps; ++i)
				{
					mc.read(&nextoffset, 4);
					if (nextoffset != 0) break;
					mc.seek(12, SEEK_CUR);
				}
				nextoffset = wxINT32_SWAP_ON_LE(nextoffset);
				if (nextoffset == 0) nextoffset = dir_offset;
				mc.seek(pos, SEEK_SET);
				actualsize = nextoffset - offset;
			}
			else
			{
				if (offset > dir_offset)
					actualsize = mc.getSize() - offset;
				else
					actualsize = dir_offset - offset;
			}
		}

		// If the lump data goes past the end of the file,
		// the wadfile is invalid
		if (offset + actualsize > mc.getSize())
		{
			LOG_MESSAGE(1, "WadJArchive::open: Wad archive is invalid or corrupt");
			Global::error = S_FMT("Archive is invalid and/or corrupt (lump %d: %s data goes past end of file)", d, name);
			setMuted(false);
			return false;
		}

		// Create & setup lump
		ArchiveEntry* nlump = new ArchiveEntry(wxString::FromAscii(name), actualsize);
		nlump->setLoaded(false);
		nlump->exProp("Offset") = (int)offset;
		nlump->setState(0);

		if (jaguarencrypt)
		{
			nlump->setEncryption(ENC_JAGUAR);
			nlump->exProp("FullSize") = (int)size;
		}

		// Add to entry list
		getRoot()->addEntry(nlump);
	}

	// Detect namespaces (needs to be done before type detection as some types
	// rely on being within certain namespaces)
	updateNamespaces();

	// Detect all entry types
	MemChunk edata;
	UI::setSplashProgressMessage("Detecting entry types");
	for (size_t a = 0; a < numEntries(); a++)
	{
		// Update splash window progress
		UI::setSplashProgress((((float)a / (float)num_lumps)));

		// Get entry
		ArchiveEntry* entry = getEntry(a);

		// Read entry data if it isn't zero-sized
		if (entry->getSize() > 0)
		{
			// Read the entry data
			edata.clear();
			mc.exportMemChunk(edata, getEntryOffset(entry), entry->getSize());
			if (entry->isEncrypted())
			{
				if (entry->exProps().propertyExists("FullSize")
				        && (unsigned)(int)(entry->exProp("FullSize")) >  entry->getSize())
					edata.reSize((int)(entry->exProp("FullSize")), true);
				if (!JaguarDecode(edata))
					LOG_MESSAGE(1, "%i: %s (following %s), did not decode properly", a, entry->getName(), a>0?getEntry(a-1)->getName():"nothing");
			}
			entry->importMemChunk(edata);
		}

		// Detect entry type
		EntryType::detectEntryType(entry);

		// Unload entry data if needed
		if (!archive_load_data)
			entry->unloadData();

		// Lock entry if IWAD
		if (wad_type[0] == 'I' && iwad_lock)
			entry->lock();

		// Set entry to unchanged
		entry->setState(0);
	}

	// Detect maps (will detect map entry types)
	UI::setSplashProgressMessage("Detecting maps");
	detectMaps();

	// Setup variables
	setMuted(false);
	setModified(false);
	//if (iwad && iwad_lock) read_only = true;
	announce("opened");

	UI::setSplashProgressMessage("");

	return true;
}

/* WadJArchive::write
 * Writes the wad archive to a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool WadJArchive::write(MemChunk& mc, bool update)
{
	// Determine directory offset & individual lump offsets
	uint32_t dir_offset = 12;
	ArchiveEntry* entry = NULL;
	for (uint32_t l = 0; l < numEntries(); l++)
	{
		entry = getEntry(l);
		setEntryOffset(entry, dir_offset);
		dir_offset += entry->getSize();
	}

	// Clear/init MemChunk
	mc.clear();
	mc.seek(0, SEEK_SET);
	mc.reSize(dir_offset + numEntries() * 16);

	// Setup wad type
	char wad_type[4] = { 'P', 'W', 'A', 'D' };
	if (iwad) wad_type[0] = 'I';

	// Write the header
	uint32_t num_lumps = wxINT32_SWAP_ON_LE(numEntries());
	dir_offset = wxINT32_SWAP_ON_LE(dir_offset);
	mc.write(wad_type, 4);
	mc.write(&num_lumps, 4);
	mc.write(&dir_offset, 4);

	// Write the lumps
	for (uint32_t l = 0; l < num_lumps; l++)
	{
		entry = getEntry(l);
		mc.write(entry->getData(), entry->getSize());
	}

	// Write the directory
	for (uint32_t l = 0; l < num_lumps; l++)
	{
		entry = getEntry(l);
		char name[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
		long offset = wxINT32_SWAP_ON_LE(getEntryOffset(entry));
		long size = wxINT32_SWAP_ON_LE(entry->getSize());

		for (size_t c = 0; c < entry->getName().length() && c < 8; c++)
			name[c] = entry->getName()[c];

		mc.write(&offset, 4);
		mc.write(&size, 4);
		mc.write(name, 8);

		if (update)
		{
			entry->setState(0);
			entry->exProp("Offset") = (int)wxINT32_SWAP_ON_LE(offset);
		}
	}

	return true;
}

/* WadJArchive::detectNamespace
 * Hack to account for Jaguar Doom's silly sprite scheme
 *******************************************************************/
string WadJArchive::detectNamespace(size_t index, ArchiveTreeNode * dir)
{
	ArchiveEntry* nextentry = getEntry(index + 1);
	if (nextentry && S_CMPNOCASE(nextentry->getName(), "."))
		return "sprites";
	return WadArchive::detectNamespace(index);

}

string WadJArchive::detectNamespace(ArchiveEntry* entry)
{
	size_t index = entryIndex(entry);
	ArchiveEntry* nextentry = getEntry(index + 1);
	if (nextentry && S_CMPNOCASE(nextentry->getName(), "."))
		return "sprites";
	return WadArchive::detectNamespace(index);

}

/* WadJArchive::isWadJArchive
 * Checks if the given data is a valid Jaguar Doom wad archive
 *******************************************************************/
bool WadJArchive::isWadJArchive(MemChunk& mc)
{
	// Check size
	if (mc.getSize() < 12)
		return false;

	// Check for IWAD/PWAD header
	if (!(mc[1] == 'W' && mc[2] == 'A' && mc[3] == 'D' &&
	        (mc[0] == 'P' || mc[0] == 'I')))
		return false;

	// Get number of lumps and directory offset
	uint32_t num_lumps = 0;
	uint32_t dir_offset = 0;
	mc.seek(4, SEEK_SET);
	mc.read(&num_lumps, 4);
	mc.read(&dir_offset, 4);

	// Reset MemChunk (just in case)
	mc.seek(0, SEEK_SET);

	// Byteswap values for little endian
	num_lumps = wxINT32_SWAP_ON_LE(num_lumps);
	dir_offset = wxINT32_SWAP_ON_LE(dir_offset);

	// Check directory offset is decent
	if ((dir_offset + (num_lumps * 16)) > mc.getSize() ||
	        dir_offset < 12)
		return false;

	// If it's passed to here it's probably a wad file
	return true;
}

/* WadJArchive::isWadJArchive
 * Checks if the file at [filename] is a valid Jaguar Doom wad archive
 *******************************************************************/
bool WadJArchive::isWadJArchive(string filename)
{
	// Open file for reading
	wxFile file(filename);

	// Check it opened ok
	if (!file.IsOpened())
		return false;

	// Read header
	char header[4];
	file.Read(header, 4);

	// Check for IWAD/PWAD header
	if (!(header[1] == 'W' && header[2] == 'A' && header[3] == 'D' &&
	        (header[0] == 'P' || header[0] == 'I')))
		return false;

	// Get number of lumps and directory offset
	uint32_t num_lumps = 0;
	uint32_t dir_offset = 0;
	file.Read(&num_lumps, 4);
	file.Read(&dir_offset, 4);

	// Byteswap values for little endian
	num_lumps = wxINT32_SWAP_ON_LE(num_lumps);
	dir_offset = wxINT32_SWAP_ON_LE(dir_offset);

	// Check directory offset is decent
	if ((dir_offset + (num_lumps * 16)) > file.Length() ||
	        dir_offset < 12)
		return false;

	// If it's passed to here it's probably a wad file
	return true;
}
