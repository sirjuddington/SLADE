
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    LabArchive.cpp
// Description: LabArchive, archive class to handle LAB archives from
//              Outlaws
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
#include "LabArchive.h"
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
// LabArchive Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the file byte offset for [entry]
// -----------------------------------------------------------------------------
uint32_t LabArchive::getEntryOffset(ArchiveEntry* entry)
{
	// Check entry
	if (!checkEntry(entry))
		return 0;

	return (uint32_t)entry->exProp<int>("Offset");
}

// -----------------------------------------------------------------------------
// Sets the file byte offset for [entry]
// -----------------------------------------------------------------------------
void LabArchive::setEntryOffset(ArchiveEntry* entry, uint32_t offset)
{
	// Check entry
	if (!checkEntry(entry))
		return;

	entry->exProp("Offset") = (int)offset;
}

// -----------------------------------------------------------------------------
// Reads lab format data from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool LabArchive::open(MemChunk& mc)
{
	// Check data was given
	if (!mc.hasData())
		return false;

	// Check size
	if (mc.size() < 16)
		return false;

	// Check magic header
	if (mc[0] != 'L' || mc[1] != 'A' || mc[2] != 'B' || mc[3] != 'N')
		return false;

	// Check version
	uint32_t version = 0;
	mc.seek(4, SEEK_SET);
	mc.read(&version, 4);
	version = wxINT32_SWAP_ON_BE(version);
	if (version != 0x00010000)
		return false;

	// Get number of lumps
	uint32_t num_lumps = 0;
	mc.read(&num_lumps, 4);
	num_lumps = wxINT32_SWAP_ON_BE(num_lumps);

	// Compute directory size
	uint32_t dir_offset = 16; // header
	uint32_t dir_size = (num_lumps * 16);
	if ((unsigned)mc.size() < (dir_offset + dir_size))
		return false;

	// Compute name directory
	uint32_t namedir_offset = dir_offset + dir_size;
	uint32_t namedir_size = 0;
	mc.read(&namedir_size, 4);
	namedir_size = wxINT32_SWAP_ON_BE(namedir_size);
	if ((unsigned)mc.size() < (namedir_offset + namedir_size))
		return false;

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	ArchiveModSignalBlocker sig_blocker{ *this };

	// Read the directory
	ui::setSplashProgressMessage("Reading lab archive data");
	for (uint32_t d = 0; d < num_lumps; d++)
	{
		// Update splash window progress
		ui::setSplashProgress(((float)d / (float)num_lumps));

		// Read lump info
		uint32_t name_offset = 0;
		uint32_t offset      = 0;
		uint32_t size        = 0;
		char     type[4]     = { 0, 0, 0, 0 };

		mc.read(&name_offset, 4); // Name
		mc.read(&offset, 4);      // Offset
		mc.read(&size, 4);        // Size
		mc.read(&type, 4);        // Type

		// Byteswap values for big endian if needed
		name_offset = wxINT32_SWAP_ON_BE(name_offset);
		offset = wxINT32_SWAP_ON_BE(offset);
		size = wxINT32_SWAP_ON_BE(size);

		// If the lump data goes past the end of the file,
		// the labfile is invalid
		if (offset + size > mc.size())
		{
			log::error("LabArchive::open: lab archive is invalid or corrupt");
			global::error = "Archive is invalid and/or corrupt";
			return false;
		}

		// Read name (zero-terminated)
		const char* name = (const char*)&mc[namedir_offset + name_offset];

		// Create & setup lump
		auto nlump = std::make_shared<ArchiveEntry>(name, size);
		nlump->setLoaded(false);
		nlump->exProp("NameOffset") = (int)name_offset;
		nlump->exProp("Offset") = (int)offset;
		nlump->exProp("LabType") = string(type, std::size(type));
		nlump->setState(ArchiveEntry::State::Unmodified);

		// Add to entry list
		rootDir()->addEntry(nlump);
	}

	// Detect all entry types
	MemChunk edata;
	ui::setSplashProgressMessage("Detecting entry types");
	for (size_t a = 0; a < numEntries(); a++)
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
// Determines the lab FourCC based on file extension
// -----------------------------------------------------------------------------
string LabArchive::detectResType(ArchiveEntry* entry)
{
	string type = entry->exProp<string>("LabType");
	if (!type.empty())
		return type;

	string_view ext = entry->upperExt();
	if (ext == ".3DO") return "FOD3";
	if (ext == ".ATX") return "FXTA";
	if (ext == ".INF") return "FFNI";
	if (ext == ".ITM") return "METI";
	if (ext == ".LAF") return "TNFN";
	if (ext == ".LVB") return "FTVL";
	if (ext == ".LVT") return "FTVL";
	if (ext == ".MSC") return "BCSM";
	if (ext == ".NWX") return "FXAW";
	if (ext == ".OBB") return "FTBO";
	if (ext == ".OBT") return "FTBO";
	if (ext == ".PCX") return "MTXT";
	if (ext == ".PHY") return "SYHP";
	if (ext == ".RCA") return "BPCR";
	if (ext == ".RCM") return "BPCR";
	if (ext == ".RCS") return "BPCR";
	if (ext == ".WAV") return "DVAW";

	return "";
}

// -----------------------------------------------------------------------------
// Writes the lab archive to a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool LabArchive::write(MemChunk& mc, bool update)
{
	// Determine directory offset & individual lump offsets
	uint32_t      dir_offset = 16;
	uint32_t      name_offset = 0;
	ArchiveEntry* entry;

	dir_offset += 16 * numEntries();
	for (uint32_t l = 0; l < numEntries(); l++)
	{
		entry = entryAt(l);
		entry->exProp("NameOffset") = (int)name_offset;
		dir_offset += entry->name().length() + 1;
		name_offset += entry->name().length() + 1;
	}
	for (uint32_t l = 0; l < numEntries(); l++)
	{
		entry = entryAt(l);
		setEntryOffset(entry, dir_offset);
		dir_offset += entry->size();
	}

	// Clear/init MemChunk
	mc.clear();
	mc.seek(0, SEEK_SET);
	mc.reSize(dir_offset);

	// Write the header	
	char     header[4] = { 'L', 'A', 'B', 'N' };
	uint32_t version   = wxINT32_SWAP_ON_BE(0x00010000);
	uint32_t num_lumps = wxINT32_SWAP_ON_BE(numEntries());
	name_offset        = wxINT32_SWAP_ON_BE(name_offset);
	mc.write(header, 4);
	mc.write(&version, 4);
	mc.write(&num_lumps, 4);
	mc.write(&name_offset, 4);

	// Write the directory
	for (uint32_t l = 0; l < numEntries(); l++)
	{
		entry           = entryAt(l);
		uint32_t name   = wxINT32_SWAP_ON_BE(entry->exProp<int>("NameOffset"));
		uint32_t offset = wxINT32_SWAP_ON_BE(getEntryOffset(entry));
		uint32_t size   = wxINT32_SWAP_ON_BE(entry->size());
		char type[4]    = { 0, 0, 0, 0 };

		// Determine resource type
		string labType = detectResType(entry);
		for (size_t c = 0; c < labType.length() && c < std::size(type); c++)
			type[c] = labType[c];

		mc.write(&name, 4);
		mc.write(&offset, 4);
		mc.write(&size, 4);
		mc.write(type, 4);

		if (update)
		{
			entry->setState(ArchiveEntry::State::Unmodified);
			entry->exProp("NameOffset") = (int)name;
			entry->exProp("Offset") = (int)offset;
			entry->exProp("LabType") = string(type, std::size(type));
		}
	}

	// Write the lumps
	for (uint32_t l = 0; l < numEntries(); l++)
	{
		entry = entryAt(l);
		mc.write(entry->name().c_str(), entry->name().length());
		mc.write("\0", 1);
	}
	for (uint32_t l = 0; l < numEntries(); l++)
	{
		entry = entryAt(l);
		mc.write(entry->rawData(), entry->size());
	}

	return true;
}

// -----------------------------------------------------------------------------
// Loads an entry's data from the labfile
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool LabArchive::loadEntryData(ArchiveEntry* entry)
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

	// Open labfile
	wxFile file(wxString::FromUTF8(filename_));

	// Check if opening the file failed
	if (!file.IsOpened())
	{
		log::error("LabArchive::loadEntryData: Failed to open labfile {}", filename_);
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
// Checks if the given data is a valid Outlaws lab archive
// -----------------------------------------------------------------------------
bool LabArchive::isLabArchive(MemChunk& mc)
{
	// Check data was given
	if (!mc.hasData())
		return false;

	// Check size
	if (mc.size() < 16)
		return false;

	// Check magic header
	if (mc[0] != 'L' || mc[1] != 'A' || mc[2] != 'B' || mc[3] != 'N')
		return false;

	// Check version
	uint32_t version = 0;
	mc.seek(4, SEEK_SET);
	mc.read(&version, 4);
	version = wxINT32_SWAP_ON_BE(version);
	if (version != 0x00010000)
		return false;

	// Get number of lumps
	uint32_t num_lumps = 0;
	mc.read(&num_lumps, 4);
	num_lumps = wxINT32_SWAP_ON_BE(num_lumps);

	// Compute directory size
	uint32_t dir_offset = 16; // header
	uint32_t dir_size   = (num_lumps * 16);
	if ((unsigned)mc.size() < (dir_offset + dir_size))
		return false;

	// Compute name directory
	uint32_t namedir_offset = dir_offset + dir_size;
	uint32_t namedir_size   = 0;
	mc.read(&namedir_size, 4);
	namedir_size = wxINT32_SWAP_ON_BE(namedir_size);
	if ((unsigned)mc.size() < (namedir_offset + namedir_size))
		return false;

	// If it's passed to here it's probably a lab file
	return true;
}

// -----------------------------------------------------------------------------
// Checks if the file at [filename] is a valid Outlaws lab archive
// -----------------------------------------------------------------------------
bool LabArchive::isLabArchive(const string& filename)
{
	// Open file for reading
	wxFile file(wxString::FromUTF8(filename));

	// Check it opened ok
	if (!file.IsOpened())
		return false;

	// Check size
	if (file.Length() < 16)
		return false;

	// Read header
	char header[4];
	file.Read(header, 4);

	// Check magic header
	if (header[0] != 'L' || header[1] != 'A' || header[2] != 'B' || header[3] != 'N')
		return false;

	// Check version
	uint32_t version = 0;
	file.Read(&version, 4);
	version = wxINT32_SWAP_ON_BE(version);
	if (version != 0x00010000)
		return false;

	// Get number of lumps
	uint32_t num_lumps = 0;
	file.Read(&num_lumps, 4);
	num_lumps = wxINT32_SWAP_ON_BE(num_lumps);

	// Compute directory size
	uint32_t dir_offset = 16; // header
	uint32_t dir_size   = (num_lumps * 16);
	if ((unsigned)file.Length() < (dir_offset + dir_size))
		return false;

	// Compute name directory
	uint32_t namedir_offset = dir_offset + dir_size;
	uint32_t namedir_size   = 0;
	file.Read(&namedir_size, 4);
	namedir_size = wxINT32_SWAP_ON_BE(namedir_size);
	if ((unsigned)file.Length() < (namedir_offset + namedir_size))
		return false;

	// If it's passed to here it's probably a lab file
	return true;
}
