
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    RffArchive.cpp
// Description: RffArchive, archive class to handle Blood's encrypted RFF
//              archives
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

/*
** Parts of this file have been taken or adapted from ZDoom's rff_file.cpp.
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


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "RffArchive.h"
#include "General/UI.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, archive_load_data)


// -----------------------------------------------------------------------------
//
// Functions & Structs
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// From ZDoom: store lump data. We need it because of the encryption
// -----------------------------------------------------------------------------
struct RFFLump
{
	uint32_t DontKnow1[4];
	uint32_t FilePos;
	uint32_t Size;
	uint32_t DontKnow2;
	uint32_t Time;
	uint8_t  Flags;
	char     Extension[3];
	char     Name[8];
	uint32_t IndexNum; // Used by .sfx, possibly others
};

void bloodCrypt(uint32_t key, void* buffer, size_t len)
{
	uint8_t* data = (uint8_t*)buffer;
	while (len > 0)
	{
		*data++ ^= key++ >> 1;
		--len;
	}
}
} // namespace


// -----------------------------------------------------------------------------
//
// RffArchive Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the file byte offset for [entry]
// -----------------------------------------------------------------------------
uint32_t RffArchive::getEntryOffset(ArchiveEntry* entry)
{
	// Check entry
	if (!checkEntry(entry))
		return 0;

	return (uint32_t)entry->exProp<int>("Offset");
}

// -----------------------------------------------------------------------------
// Sets the file byte offset for [entry]
// -----------------------------------------------------------------------------
void RffArchive::setEntryOffset(ArchiveEntry* entry, uint32_t offset)
{
	// Check entry
	if (!checkEntry(entry))
		return;

	entry->exProp("Offset") = (int)offset;
}

// -----------------------------------------------------------------------------
// Reads rff format data from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool RffArchive::open(MemChunk& mc)
{
	// Check data was given
	if (!mc.hasData())
		return false;

	// Read rff header
	uint32_t magic, dir_offset, num_lumps;
	uint16_t version;

	mc.seek(0, SEEK_SET);
	mc.read(&magic, 4);
	mc.read(&version, 2);
	mc.seek(2, SEEK_CUR);
	mc.read(&dir_offset, 4); // Absolute offset to directory
	mc.read(&num_lumps, 4);  // Number of lumps in rff

	// Check the header
	if (magic != wxUINT32_SWAP_ON_LE(0x5246461A)) // 'RFF\x1A'
	{
		log::error("RffArchive::open: File {} has invalid header", filename_);
		global::error = "Invalid rff header";
		return false;
	}

	// Byteswap values for big endian if needed
	dir_offset = wxUINT32_SWAP_ON_BE(dir_offset);
	num_lumps  = wxUINT32_SWAP_ON_BE(num_lumps);
	version    = wxUINT16_SWAP_ON_BE(version);

	// Check the version and select the key if needed
	uint32_t key;
	switch (version)
	{
	case 0x200: break;
	case 0x300: key = dir_offset; break;
	case 0x301: key = dir_offset << 1; break;

	default:
		log::error("RffArchive::open: File {} has unknown version {}", filename_, version);
		global::error = "Unknown rff version";
		return false;
	};

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	ArchiveModSignalBlocker sig_blocker{ *this };

	// Read the directory
	auto lumps = new RFFLump[num_lumps];
	mc.seek(dir_offset, SEEK_SET);
	ui::setSplashProgressMessage("Reading rff archive data");
	mc.read(lumps, num_lumps * sizeof(RFFLump));

	// Decrypt if there is an encryption
	if (version >= 0x300)
		bloodCrypt(key, lumps, num_lumps * sizeof(RFFLump));

	for (uint32_t d = 0; d < num_lumps; ++d)
	{
		// Update splash window progress
		ui::setSplashProgress(((float)d / (float)num_lumps));

		// Read lump info
		char     name[13] = {};
		uint32_t offset   = wxUINT32_SWAP_ON_BE(lumps[d].FilePos);
		uint32_t size     = wxUINT32_SWAP_ON_BE(lumps[d].Size);

		// If the lump data goes past the end of the file,
		// the rff file is invalid
		if (offset + size >= mc.size())
		{
			log::error("RffArchive::open: rff archive is invalid or corrupt");
			global::error = "Archive is invalid and/or corrupt";
			return false;
		}

		// Reconstruct name
		int i, j = 0;
		for (i = 0; i < 8; ++i)
		{
			if (lumps[d].Name[i] == 0)
				break;
			name[i] = lumps[d].Name[i];
		}
		for (name[i++] = '.'; j < 3; ++j)
			name[i + j] = lumps[d].Extension[j];

		// Create & setup lump
		auto nlump = std::make_shared<ArchiveEntry>(name, size);
		nlump->setLoaded(false);
		nlump->exProp("Offset") = (int)offset;
		nlump->setState(ArchiveEntry::State::Unmodified);

		// Is the entry encrypted?
		if (lumps[d].Flags & 0x10)
			nlump->setEncryption(ArchiveEntry::Encryption::Blood);

		// Add to entry list
		rootDir()->addEntry(nlump);
	}
	delete[] lumps;

	// Detect all entry types
	MemChunk edata;
	ui::setSplashProgressMessage("Detecting entry types");
	for (size_t a = 0; a < numEntries(); ++a)
	{
		// Update splash window progress
		ui::setSplashProgress((((float)a / (float)num_lumps)));

		// Get entry
		auto entry = entryAt(a);

		// Read entry data if it isn't zero-sized
		if (entry->size() > 0)
		{
			// Read the entry data
			mc.exportMemChunk(edata, getEntryOffset(entry), entry->size());

			// If the entry is encrypted, decrypt it
			if (entry->encryption() != ArchiveEntry::Encryption::None)
			{
				uint8_t* cdata = new uint8_t[entry->size()];
				memcpy(cdata, edata.data(), entry->size());
				int cryptlen = entry->size() < 256 ? entry->size() : 256;
				bloodCrypt(0, cdata, cryptlen);
				edata.importMem(cdata, entry->size());
				delete[] cdata;
			}

			// Import data
			entry->importMemChunk(edata);
		}

		// Detect entry type
		EntryType::detectEntryType(*entry);

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
// Writes the rff archive to a MemChunk
// Not implemented because of encrypted directory and unknown stuff.
// -----------------------------------------------------------------------------
bool RffArchive::write(MemChunk& mc, bool update)
{
	log::warning("Saving RFF files is not implemented because the format is not entirely known.");
	return false;
}

// -----------------------------------------------------------------------------
// Loads an entry's data from the rff file
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool RffArchive::loadEntryData(ArchiveEntry* entry)
{
	// Check the entry is valid and part of this archive
	if (!checkEntry(entry))
		return false;

	// Do nothing if the lump's size is zero,
	// or if it has already been loaded
	if (entry->size() == 0 || entry->isLoaded())
	{
		entry->setLoaded();
		return true;
	}

	// Open rff file
	wxFile file(wxString::FromUTF8(filename_));

	// Check if opening the file failed
	if (!file.IsOpened())
	{
		log::error("RffArchive::loadEntryData: Failed to open rff file {}", filename_);
		return false;
	}

	// Seek to lump offset in file and read it in
	file.Seek(getEntryOffset(entry), wxFromStart);
	entry->importFileStream(file, entry->size());

	// Set the lump to loaded
	entry->setLoaded();

	return true;
}

// -----------------------------------------------------------------------------
// Checks if the given data is a valid Blood rff archive
// -----------------------------------------------------------------------------
bool RffArchive::isRffArchive(MemChunk& mc)
{
	// Check size
	if (mc.size() < 16)
		return false;

	// Read rff header
	uint32_t magic, dir_offset, num_lumps;
	uint16_t version;

	mc.seek(0, SEEK_SET);
	mc.read(&magic, 4);
	mc.read(&version, 2);
	mc.seek(2, SEEK_CUR);
	mc.read(&dir_offset, 4); // Absolute offset to directory
	mc.read(&num_lumps, 4);  // Number of lumps in rff

	// Check the header
	if (magic != wxUINT32_SWAP_ON_LE(0x5246461A)) // 'RFF\x1A'
		return false;

	// Byteswap values for big endian if needed
	dir_offset = wxUINT32_SWAP_ON_BE(dir_offset);
	num_lumps  = wxUINT32_SWAP_ON_BE(num_lumps);
	version    = wxUINT16_SWAP_ON_BE(version);

	// Check the version and select the key if needed
	uint32_t key;
	switch (version)
	{
	case 0x200: break;
	case 0x300: key = dir_offset; break;
	case 0x301: key = dir_offset << 1; break;
	default: return false;
	};

	// Compute total size
	auto lumps = new RFFLump[num_lumps];
	mc.seek(dir_offset, SEEK_SET);
	ui::setSplashProgressMessage("Reading rff archive data");
	mc.read(lumps, num_lumps * sizeof(RFFLump));

	// Decrypt if there is an encryption
	if (version >= 0x300)
		bloodCrypt(key, lumps, num_lumps * sizeof(RFFLump));

	uint32_t totalsize = 16 + num_lumps * sizeof(RFFLump);
	for (uint32_t d = 0; d < num_lumps; ++d)
		totalsize += wxUINT32_SWAP_ON_BE(lumps[d].Size);

	// Check if total size is correct
	if (totalsize > mc.size())
		return false;

	// If it's passed to here it's probably a rff file
	return true;
}

// -----------------------------------------------------------------------------
// Checks if the file at [filename] is a valid BLOOD rff archive
// -----------------------------------------------------------------------------
bool RffArchive::isRffArchive(const string& filename)
{
	// Open file for reading
	wxFile file(wxString::FromUTF8(filename));

	// Check it opened ok
	if (!file.IsOpened())
		return false;

	// Check size
	if (file.Length() < 16)
		return false;

	// Read rff header
	uint32_t magic, dir_offset, num_lumps;
	uint16_t version;

	file.Seek(0, wxFromStart);
	file.Read(&magic, 4);
	file.Read(&version, 2);
	file.Seek(2, wxFromCurrent);
	file.Read(&dir_offset, 4); // Absolute offset to directory
	file.Read(&num_lumps, 4);  // Number of lumps in rff

	// Check the header
	if (magic != wxUINT32_SWAP_ON_LE(0x5246461A)) // 'RFF\x1A'
		return false;

	// Byteswap values for big endian if needed
	dir_offset = wxUINT32_SWAP_ON_BE(dir_offset);
	num_lumps  = wxUINT32_SWAP_ON_BE(num_lumps);
	version    = wxUINT16_SWAP_ON_BE(version);

	// Check the version and select the key if needed
	uint32_t key;
	switch (version)
	{
	case 0x200: break;
	case 0x300: key = dir_offset; break;
	case 0x301: key = dir_offset << 1; break;
	default: return false;
	};

	// Compute total size
	auto lumps = new RFFLump[num_lumps];
	file.Seek(dir_offset, wxFromStart);
	ui::setSplashProgressMessage("Reading rff archive data");
	file.Read(lumps, num_lumps * sizeof(RFFLump));

	// Decrypt if there is an encryption
	if (version >= 0x300)
		bloodCrypt(key, lumps, num_lumps * sizeof(RFFLump));

	uint32_t totalsize = 16 + num_lumps * sizeof(RFFLump);
	for (uint32_t d = 0; d < num_lumps; ++d)
		totalsize += wxUINT32_SWAP_ON_BE(lumps[d].Size);

	// Check if total size is correct
	if (totalsize > file.Length())
		return false;

	// If it's passed to here it's probably a rff file
	return true;
}
