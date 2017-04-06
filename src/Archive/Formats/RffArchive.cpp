
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    RffArchive.cpp
 * Description: RffArchive, archive class to handle Blood's
 *              encrypted RFF archives
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
/*
** Part of this file have been taken or adapted from ZDoom's rff_file.cpp.
**
**---------------------------------------------------------------------------
** Copyright 1998-2009 Randy Heit
** Copyright 2005-2009 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "RffArchive.h"
#include "General/UI.h"


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, wad_force_uppercase)
EXTERN_CVAR(Bool, archive_load_data)

/*******************************************************************
 * RFFARCHIVE HELPER FUNCTIONS AND STRUCTS
 *******************************************************************/

/* BloodCrypt
 * From ZDoom: decrypts RFF data
 *******************************************************************/
void BloodCrypt (void* data, int key, int len)
{
	int p = (uint8_t)key, i;

	for (i = 0; i < len; ++i)
	{
		((uint8_t*)data)[i] ^= (unsigned char)(p+(i>>1));
	}
}

/* RFFLump struct
 * From ZDoom: store lump data. We need it because of the encryption
 *******************************************************************/
struct RFFLump
{
	uint32_t	DontKnow1[4];
	uint32_t	FilePos;
	uint32_t	Size;
	uint32_t	DontKnow2;
	uint32_t	Time;
	uint8_t		Flags;
	char		Extension[3];
	char		Name[8];
	uint32_t	IndexNum;	// Used by .sfx, possibly others
};

/*******************************************************************
 * RFFARCHIVE CLASS FUNCTIONS
 *******************************************************************/

/* RffArchive::RffArchive
 * RffArchive class constructor
 *******************************************************************/
RffArchive::RffArchive() : TreelessArchive(ARCHIVE_RFF)
{
}

/* RffArchive::~RffArchive
 * RffArchive class destructor
 *******************************************************************/
RffArchive::~RffArchive()
{
}

/* RffArchive::getEntryOffset
 * Returns the file byte offset for [entry]
 *******************************************************************/
uint32_t RffArchive::getEntryOffset(ArchiveEntry* entry)
{
	// Check entry
	if (!checkEntry(entry))
		return 0;

	return (uint32_t)(int)entry->exProp("Offset");
}

/* RffArchive::setEntryOffset
 * Sets the file byte offset for [entry]
 *******************************************************************/
void RffArchive::setEntryOffset(ArchiveEntry* entry, uint32_t offset)
{
	// Check entry
	if (!checkEntry(entry))
		return;

	entry->exProp("Offset") = (int)offset;
}

/* RffArchive::getFileExtensionString
 * Gets the wxWidgets file dialog filter string for the archive type
 *******************************************************************/
string RffArchive::getFileExtensionString()
{
	return "Rff Files (*.rff)|*.rff";
}

/* RffArchive::getFormat
 * Returns the EntryDataFormat id of this archive type
 *******************************************************************/
string RffArchive::getFormat()
{
	return "archive_rff";
}

/* RffArchive::open
 * Reads grp format data from a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool RffArchive::open(MemChunk& mc)
{
	// Check data was given
	if (!mc.hasData())
		return false;

	// Read grp header
	uint8_t magic[4];
	uint32_t version, dir_offset, num_lumps;

	mc.seek(0, SEEK_SET);
	mc.read(magic, 4);			// Should be "RFF\x18"
	mc.read(&version, 4);		// 0x01 0x03 \x00 \x00
	mc.read(&dir_offset, 4);	// Offset to directory
	mc.read(&num_lumps, 4);		// No. of lumps in rff

	// Byteswap values for big endian if needed
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);
	num_lumps = wxINT32_SWAP_ON_BE(num_lumps);
	version = wxINT32_SWAP_ON_BE(version);

	// Check the header
	if (magic[0] != 'R' || magic[1] != 'F' || magic[2] != 'F' || magic[3] != 0x1A || version != 0x301)
	{
		LOG_MESSAGE(1, "RffArchive::openFile: File %s has invalid header", filename);
		Global::error = "Invalid rff header";
		return false;
	}

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	setMuted(true);

	// Read the directory
	RFFLump* lumps = new RFFLump[num_lumps];
	mc.seek(dir_offset, SEEK_SET);
	UI::setSplashProgressMessage("Reading rff archive data");
	mc.read (lumps, num_lumps * sizeof(RFFLump));
	BloodCrypt (lumps, dir_offset, num_lumps * sizeof(RFFLump));
	for (uint32_t d = 0; d < num_lumps; d++)
	{
		// Update splash window progress
		UI::setSplashProgress(((float)d / (float)num_lumps));

		// Read lump info
		char name[13] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		uint32_t offset = wxINT32_SWAP_ON_BE(lumps[d].FilePos);
		uint32_t size = wxINT32_SWAP_ON_BE(lumps[d].Size);

		// Reconstruct name
		int i, j = 0;
		for (i = 0; i < 8; ++i)
		{
			if (lumps[d].Name[i] == 0) break;
			name[i] = lumps[d].Name[i];
		}
		for (name[i++] = '.'; j < 3; ++j)
			name[i+j] = lumps[d].Extension[j];

		// If the lump data goes past the end of the file,
		// the rfffile is invalid
		if (offset + size > mc.getSize())
		{
			LOG_MESSAGE(1, "RffArchive::open: rff archive is invalid or corrupt");
			Global::error = "Archive is invalid and/or corrupt";
			setMuted(false);
			return false;
		}

		// Create & setup lump
		ArchiveEntry* nlump = new ArchiveEntry(wxString::FromAscii(name), size);
		nlump->setLoaded(false);
		nlump->exProp("Offset") = (int)offset;
		nlump->setState(0);

		// Is the entry encrypted?
		if (lumps[d].Flags & 0x10)
			nlump->setEncryption(ENC_BLOOD);

		// Add to entry list
		getRoot()->addEntry(nlump);
	}
	delete[] lumps;

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
			mc.exportMemChunk(edata, getEntryOffset(entry), entry->getSize());

			// If the entry is encrypted, decrypt it
			if (entry->isEncrypted())
			{
				uint8_t* cdata = new uint8_t[entry->getSize()];
				memcpy(cdata, edata.getData(), entry->getSize());
				int cryptlen = entry->getSize() < 256 ? entry->getSize() : 256;
				BloodCrypt(cdata, 0, cryptlen);
				edata.importMem(cdata, entry->getSize());
				delete[] cdata;
			}

			// Import data
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

	// Detect maps (will detect map entry types)
	//UI::setSplashProgressMessage("Detecting maps");
	//detectMaps();

	// Setup variables
	setMuted(false);
	setModified(false);
	announce("opened");

	UI::setSplashProgressMessage("");

	return true;
}

/* RffArchive::write
 * Writes the rff archive to a MemChunk
 * Not implemented because of encrypted directory and unknown stuff.
 *******************************************************************/
bool RffArchive::write(MemChunk& mc, bool update)
{
	LOG_MESSAGE(1, "Saving RFF files is not implemented because the format is not entirely known.");
	return false;
}

/* RffArchive::loadEntryData
 * Loads an entry's data from the grpfile
 * Returns true if successful, false otherwise
 *******************************************************************/
bool RffArchive::loadEntryData(ArchiveEntry* entry)
{
	// Check the entry is valid and part of this archive
	if (!checkEntry(entry))
		return false;

	// Do nothing if the lump's size is zero,
	// or if it has already been loaded
	if (entry->getSize() == 0 || entry->isLoaded())
	{
		entry->setLoaded();
		return true;
	}

	// Open rfffile
	wxFile file(filename);

	// Check if opening the file failed
	if (!file.IsOpened())
	{
		LOG_MESSAGE(1, "RffArchive::loadEntryData: Failed to open rfffile %s", filename);
		return false;
	}

	// Seek to lump offset in file and read it in
	file.Seek(getEntryOffset(entry), wxFromStart);
	entry->importFileStream(file, entry->getSize());

	// Set the lump to loaded
	entry->setLoaded();

	return true;
}

/* RffArchive::addEntry
 * Override of Archive::addEntry to force entry addition to the root
 * directory, update namespaces if needed and rename the entry if
 * necessary to be rff-friendly  (12 characters max with extension)
 *******************************************************************/
ArchiveEntry* RffArchive::addEntry(ArchiveEntry* entry, unsigned position, ArchiveTreeNode* dir, bool copy)
{
	// Check entry
	if (!entry)
		return NULL;

	// Check if read-only
	if (isReadOnly())
		return NULL;

	// Copy if necessary
	if (copy)
		entry = new ArchiveEntry(*entry);

	// Process name (must be 12 characters max)
	string name = entry->getName().Truncate(12);
	if (wad_force_uppercase) name.MakeUpper();

	// Set new grp-friendly name
	entry->setName(name);

	// Do default entry addition (to root directory)
	Archive::addEntry(entry, position);

	return entry;
}

/* RffArchive::addEntry
 * Since RFF files have no namespaces, just call the other function.
 *******************************************************************/
ArchiveEntry* RffArchive::addEntry(ArchiveEntry* entry, string add_namespace, bool copy)
{
	return addEntry(entry, 0xFFFFFFFF, NULL, copy);
}

/* RffArchive::renameEntry
 * Override of Archive::renameEntry to update namespaces if needed
 * and rename the entry if necessary to be grp-friendly (twelve
 * characters max)
 *******************************************************************/
bool RffArchive::renameEntry(ArchiveEntry* entry, string name)
{
	// Check entry
	if (!checkEntry(entry))
		return false;

	// Process name (must be 12 characters max)
	name.Truncate(12);
	if (wad_force_uppercase) name.MakeUpper();

	// Do default rename
	return Archive::renameEntry(entry, name);
}

/* RffArchive::isRffArchive
 * Checks if the given data is a valid Duke Nukem 3D grp archive
 *******************************************************************/
bool RffArchive::isRffArchive(MemChunk& mc)
{
	// Check size
	if (mc.getSize() < 12)
		return false;

	// Read grp header
	uint8_t magic[4];
	uint32_t version, dir_offset, num_lumps;

	mc.seek(0, SEEK_SET);
	mc.read(magic, 4);			// Should be "RFF\x18"
	mc.read(&version, 4);		// 0x01 0x03 \x00 \x00
	mc.read(&dir_offset, 4);	// Offset to directory
	mc.read(&num_lumps, 4);		// No. of lumps in rff

	// Byteswap values for big endian if needed
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);
	num_lumps = wxINT32_SWAP_ON_BE(num_lumps);
	version = wxINT32_SWAP_ON_BE(version);

	// Check the header
	if (magic[0] != 'R' || magic[1] != 'F' || magic[2] != 'F' || magic[3] != 0x1A || version != 0x301)
		return false;


	// Compute total size
	RFFLump* lumps = new RFFLump[num_lumps];
	mc.seek(dir_offset, SEEK_SET);
	UI::setSplashProgressMessage("Reading rff archive data");
	mc.read (lumps, num_lumps * sizeof(RFFLump));
	BloodCrypt (lumps, dir_offset, num_lumps * sizeof(RFFLump));
	uint32_t totalsize = 12 + num_lumps * sizeof(RFFLump);
	uint32_t size = 0;
	for (uint32_t a = 0; a < num_lumps; ++a)
	{
		totalsize += lumps[a].Size;
	}

	// Check if total size is correct
	if (totalsize > mc.getSize())
		return false;

	// If it's passed to here it's probably a grp file
	return true;
}

/* RffArchive::isRffArchive
 * Checks if the file at [filename] is a valid DN3D grp archive
 *******************************************************************/
bool RffArchive::isRffArchive(string filename)
{
	// Open file for reading
	wxFile file(filename);

	// Check it opened ok
	if (!file.IsOpened())
		return false;

	// Check size
	if (file.Length() < 12)
		return false;

	// Read grp header
	uint8_t magic[4];
	uint32_t version, dir_offset, num_lumps;

	file.Seek(0, wxFromStart);
	file.Read(magic, 4);			// Should be "RFF\x18"
	file.Read(&version, 4);		// 0x01 0x03 \x00 \x00
	file.Read(&dir_offset, 4);	// Offset to directory
	file.Read(&num_lumps, 4);		// No. of lumps in rff

	// Byteswap values for big endian if needed
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);
	num_lumps = wxINT32_SWAP_ON_BE(num_lumps);
	version = wxINT32_SWAP_ON_BE(version);

	// Check the header
	if (magic[0] != 'R' || magic[1] != 'F' || magic[2] != 'F' || magic[3] != 0x1A || version != 0x301)
		return false;


	// Compute total size
	RFFLump* lumps = new RFFLump[num_lumps];
	file.Seek(dir_offset, wxFromStart);
	UI::setSplashProgressMessage("Reading rff archive data");
	file.Read(lumps, num_lumps * sizeof(RFFLump));
	BloodCrypt (lumps, dir_offset, num_lumps * sizeof(RFFLump));
	uint32_t totalsize = 12 + num_lumps * sizeof(RFFLump);
	uint32_t size = 0;
	for (uint32_t a = 0; a < num_lumps; ++a)
	{
		totalsize += lumps[a].Size;
	}

	// Check if total size is correct
	if (totalsize > file.Length())
		return false;

	// If it's passed to here it's probably a grp file
	return true;
}
