
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    WadArchive.cpp
// Description: WadArchive, archive class to handle doom format wad archives
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
#include "WadArchive.h"
#include "General/Misc.h"
#include "General/UI.h"
#include "Utility/Tokenizer.h"
#include "WadJArchive.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, iwad_lock, true, CVar::Flag::Save)

namespace
{
// Used for map detection
enum MapLumpNames
{
	LUMP_THINGS,
	LUMP_VERTEXES,
	LUMP_LINEDEFS,
	LUMP_SIDEDEFS,
	LUMP_SECTORS,
	LUMP_SEGS,
	LUMP_SSECTORS,
	LUMP_NODES,
	LUMP_BLOCKMAP,
	LUMP_REJECT,
	LUMP_SCRIPTS,
	LUMP_BEHAVIOR,
	LUMP_LEAFS,
	LUMP_LIGHTS,
	LUMP_MACROS,
	LUMP_GL_HEADER,
	LUMP_GL_VERT,
	LUMP_GL_SEGS,
	LUMP_GL_SSECT,
	LUMP_GL_NODES,
	LUMP_GL_PVS,
	LUMP_TEXTMAP,
	LUMP_ZNODES,
	NUMMAPLUMPS
};
string map_lumps[] = { "THINGS",   "VERTEXES", "LINEDEFS", "SIDEDEFS", "SECTORS", "SEGS",    "SSECTORS", "NODES",
					   "BLOCKMAP", "REJECT",   "SCRIPTS",  "BEHAVIOR", "LEAFS",   "LIGHTS",  "MACROS",   "GL_MAP01",
					   "GL_VERT",  "GL_SEGS",  "GL_SSECT", "GL_NODES", "GL_PVS",  "TEXTMAP", "ZNODES" };

// Special namespaces (at the moment these are just mapping to zdoom's "zip as wad" namespace folders)
// http://zdoom.org/wiki/Using_ZIPs_as_WAD_replacement#How_to
struct SpecialNS
{
	string name;
	string letter;
};
vector<SpecialNS> special_namespaces = {
	{ "patches", "p" },   { "sprites", "s" },   { "flats", "f" },
	{ "textures", "tx" }, { "textures", "t" }, // alias for Jaguar Doom & Doom 64
	{ "hires", "hi" },    { "colormaps", "c" }, { "acs", "a" },
	{ "voices", "v" },    { "voxels", "vx" },   { "sounds", "ds" }, // Jaguar Doom and Doom 64 use it
};
} // namespace


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, archive_load_data)


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Returns true if [entry] is a namespace marker (*_START / *_END)
// -----------------------------------------------------------------------------
bool isNamespaceEntry(ArchiveEntry* entry)
{
	static string start = "_START";
	static string end   = "_END";

	auto uname = entry->upperName();
	return uname.EndsWith(start) || uname.EndsWith(end);
}
} // namespace


// -----------------------------------------------------------------------------
//
// WadArchive Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns true if the archive can be written to disk
// -----------------------------------------------------------------------------
bool WadArchive::isWritable()
{
	return !(iwad_ && iwad_lock);
}

// -----------------------------------------------------------------------------
// Returns the file byte offset for [entry]
// -----------------------------------------------------------------------------
uint32_t WadArchive::getEntryOffset(ArchiveEntry* entry)
{
	// Check entry
	if (!checkEntry(entry))
		return 0;

	return (uint32_t)(int)entry->exProp("Offset");
}

// -----------------------------------------------------------------------------
// Sets the file byte offset for [entry]
// -----------------------------------------------------------------------------
void WadArchive::setEntryOffset(ArchiveEntry* entry, uint32_t offset)
{
	// Check entry
	if (!checkEntry(entry))
		return;

	entry->exProp("Offset") = (int)offset;
}

// -----------------------------------------------------------------------------
// Updates the namespace list
// -----------------------------------------------------------------------------
void WadArchive::updateNamespaces()
{
	// Clear current namespace info
	while (!namespaces_.empty())
		namespaces_.pop_back();

	// Go through all entries
	for (unsigned a = 0; a < numEntries(); a++)
	{
		auto entry = rootDir()->entryAt(a);

		// Check for namespace begin
		if (entry->name().Matches("*_START"))
		{
			// Create new namespace
			NSPair ns(entry, nullptr);
			string name    = entry->name();
			ns.name        = name.Left(name.Length() - 6).Lower();
			ns.start_index = entryIndex(ns.start);

			// Convert some special cases (because technically PP_START->P_END is a valid namespace)
			if (ns.name == "pp")
				ns.name = "p";
			if (ns.name == "ff")
				ns.name = "f";
			if (ns.name == "ss")
				ns.name = "s";
			if (ns.name == "tt")
				ns.name = "t";

			// Add to namespace list
			namespaces_.push_back(ns);
		}
		// Check for namespace end
		else if (entry->name().Matches("?_END") || entry->name().Matches("??_END"))
		{
			// Get namespace 'name'
			int    len     = entry->name().Length() - 4;
			string ns_name = entry->name().Left(len).Lower();

			// Convert some special cases (because technically P_START->PP_END is a valid namespace)
			if (ns_name == "pp")
				ns_name = "p";
			if (ns_name == "ff")
				ns_name = "f";
			if (ns_name == "ss")
				ns_name = "s";
			if (ns_name == "tt")
				ns_name = "t";

			// Check if it's the end of an existing namespace
			// Remember entry is getEntry(a)? index is 'a'
			// size_t index = entryIndex(entry);

			bool found = false;
			for (auto& ns : namespaces_)
			{
				// Can't close a namespace that starts afterwards
				if (ns.start_index > a)
					break;
				// Can't close an already-closed namespace
				if (ns.end != nullptr)
					continue;
				if (S_CMP(ns_name, ns.name))
				{
					found        = true;
					ns.end       = entry;
					ns.end_index = a;
					break;
				}
			}
			// Flat hack: closing the flat namespace without opening it
			if (!found && ns_name == "f")
			{
				NSPair ns(rootDir()->entryAt(0), entry);
				ns.start_index = 0;
				ns.end_index   = a;
				ns.name        = "f";
				namespaces_.push_back(ns);
			}
		}
	}

	// ROTT stuff. The first lump in the archive is always WALLSTRT, the last lump is either
	// LICENSE (darkwar.wad) or VENDOR (huntbgin.wad), with TABLES just before in both cases.
	// The shareware version has 2091 lumps, the complete version has about 50% more.
	if (numEntries() > 2090 && rootDir()->entryAt(0)->name().Matches("WALLSTRT")
		&& rootDir()->entryAt(numEntries() - 2)->name().Matches("TABLES"))
	{
		NSPair ns(rootDir()->entryAt(0), rootDir()->entryAt(numEntries() - 1));
		ns.name        = "rott";
		ns.start_index = 0;
		ns.end_index   = entryIndex(ns.end);
		namespaces_.push_back(ns);
	}


	// Check namespaces
	for (unsigned a = 0; a < namespaces_.size(); a++)
	{
		auto& ns = namespaces_[a];

		// Check the namespace has an end
		if (!ns.end)
		{
			// If not, remove the namespace as it is invalid
			namespaces_.erase(namespaces_.begin() + a);
			a--;
			continue;
		}

		// Check namespace name for special cases
		for (auto& special_namespace : special_namespaces)
		{
			if (S_CMP(ns.name, special_namespace.letter))
				ns.name = special_namespace.name;
		}

		ns.start_index = entryIndex(ns.start);
		ns.end_index   = entryIndex(ns.end);

		// Testing
		// Log::info(1, "Namespace %s from %s (%d) to %s (%d)", ns.name,
		//	ns.start->getName(), ns.start_index, ns.end->getName(), ns.end_index);
	}
}

// -----------------------------------------------------------------------------
// Detects if the flat hack is used in this archive or not
// -----------------------------------------------------------------------------
bool WadArchive::hasFlatHack()
{
	for (auto& i : namespaces_)
		if (i.name == "f")
			return (i.start_index == 0 && i.start->size() != 0);

	return false;
}

// -----------------------------------------------------------------------------
// Reads wad format data from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool WadArchive::open(MemChunk& mc)
{
	// Check data was given
	if (!mc.hasData())
		return false;

	// Read wad header
	uint32_t num_lumps   = 0;
	uint32_t dir_offset  = 0;
	char     wad_type[4] = "";
	mc.seek(0, SEEK_SET);
	mc.read(&wad_type, 4);   // Wad type
	mc.read(&num_lumps, 4);  // No. of lumps in wad
	mc.read(&dir_offset, 4); // Offset to directory

	// Byteswap values for big endian if needed
	num_lumps  = wxINT32_SWAP_ON_BE(num_lumps);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);

	// Check the header
	if (wad_type[1] != 'W' || wad_type[2] != 'A' || wad_type[3] != 'D')
	{
		Log::error(S_FMT("WadArchive::openFile: File %s has invalid header", filename_));
		Global::error = "Invalid wad header";
		return false;
	}

	// Check for iwad
	if (wad_type[0] == 'I')
		iwad_ = true;

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	setMuted(true);

	vector<uint32_t> offsets;

	// Read the directory
	mc.seek(dir_offset, SEEK_SET);
	UI::setSplashProgressMessage("Reading wad archive data");
	for (uint32_t d = 0; d < num_lumps; d++)
	{
		// Update splash window progress
		UI::setSplashProgress(((float)d / (float)num_lumps));

		// Read lump info
		char     name[9] = "";
		uint32_t offset  = 0;
		uint32_t size    = 0;

		mc.read(&offset, 4); // Offset
		mc.read(&size, 4);   // Size
		mc.read(name, 8);    // Name
		name[8] = '\0';

		// Byteswap values for big endian if needed
		offset = wxINT32_SWAP_ON_BE(offset);
		size   = wxINT32_SWAP_ON_BE(size);

		// Check to catch stupid shit
		if (size > 0)
		{
			if (offset == 0)
			{
				Log::info(2, "No.");
				continue;
			}
			if (VECTOR_EXISTS(offsets, offset))
			{
				Log::warning(S_FMT("Ignoring entry %d: %s, is a clone of a previous entry", d, name));
				continue;
			}
			offsets.push_back(offset);
		}

		// Hack to open Operation: Rheingold WAD files
		if (size == 0 && offset > mc.size())
			offset = 0;

		// Is there a compression/encryption thing going on?
		bool jaguarencrypt = !!(name[0] & 0x80); // look at high bit
		name[0]            = name[0] & 0x7F;     // then strip it away

		// Look for encryption shenanigans
		size_t actualsize = size;
		if (jaguarencrypt)
		{
			if (d < num_lumps - 1)
			{
				size_t   pos        = mc.currentPos();
				uint32_t nextoffset = 0;
				for (int i = 0; i + d < num_lumps; ++i)
				{
					mc.read(&nextoffset, 4);
					if (nextoffset != 0)
						break;
					mc.seek(12, SEEK_CUR);
				}
				nextoffset = wxINT32_SWAP_ON_BE(nextoffset);
				if (nextoffset == 0)
					nextoffset = dir_offset;
				mc.seek(pos, SEEK_SET);
				actualsize = nextoffset - offset;
			}
			else
			{
				if (offset > dir_offset)
				{
					actualsize = mc.size() - offset;
				}
				else
				{
					actualsize = dir_offset - offset;
				}
			}
		}

		// If the lump data goes past the end of the file,
		// the wadfile is invalid
		if (offset + actualsize > mc.size())
		{
			Log::error("WadArchive::open: Wad archive is invalid or corrupt");
			Global::error = S_FMT(
				"Archive is invalid and/or corrupt (lump %d: %s data goes past end of file)", d, name);
			setMuted(false);
			return false;
		}

		// Create & setup lump
		auto nlump = std::make_shared<ArchiveEntry>(wxString::FromAscii(name), size);
		nlump->setLoaded(false);
		nlump->exProp("Offset") = (int)offset;
		nlump->setState(ArchiveEntry::State::Unmodified);

		if (jaguarencrypt)
		{
			nlump->setEncryption(ArchiveEntry::Encryption::Jaguar);
			nlump->exProp("FullSize") = (int)size;
		}

		// Add to entry list
		rootDir()->addEntry(nlump);
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
		UI::setSplashProgress((((float)a / (float)numEntries())));

		// Get entry
		auto entry = entryAt(a);

		// Read entry data if it isn't zero-sized
		if (entry->size() > 0)
		{
			// Read the entry data
			mc.exportMemChunk(edata, getEntryOffset(entry), entry->size());
			if (entry->encryption() != ArchiveEntry::Encryption::None)
			{
				if (entry->exProps().propertyExists("FullSize")
					&& (unsigned)(int)(entry->exProp("FullSize")) > entry->size())
					edata.reSize((int)(entry->exProp("FullSize")), true);
				if (!WadJArchive::jaguarDecode(edata))
					Log::warning(S_FMT(
						"%i: %s (following %s), did not decode properly",
						a,
						entry->name(),
						a > 0 ? entryAt(a - 1)->name() : "nothing"));
			}
			entry->importMemChunk(edata);
		}

		// Detect entry type
		EntryType::detectEntryType(entry);

		// Unload entry data if needed
		if (!archive_load_data)
			entry->unloadData();

		// Set entry to unchanged
		entry->setState(ArchiveEntry::State::Unmodified);
	}

	// Identify #included lumps (DECORATE, GLDEFS, etc.)
	detectIncludes();

	// Detect maps (will detect map entry types)
	UI::setSplashProgressMessage("Detecting maps");
	detectMaps();

	// Setup variables
	setMuted(false);
	setModified(false);
	announce("opened");

	UI::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Writes the wad archive to a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool WadArchive::write(MemChunk& mc, bool update)
{
	// Don't write if iwad
	if (iwad_ && iwad_lock)
	{
		Global::error = "IWAD saving disabled";
		return false;
	}

	// Determine directory offset & individual lump offsets
	uint32_t      dir_offset = 12;
	ArchiveEntry* entry;
	for (uint32_t l = 0; l < numEntries(); l++)
	{
		entry = entryAt(l);
		setEntryOffset(entry, dir_offset);
		dir_offset += entry->size();
	}

	// Clear/init MemChunk
	mc.clear();
	mc.seek(0, SEEK_SET);
	if (!mc.reSize(dir_offset + numEntries() * 16))
	{
		Global::error = "Failed to allocate sufficient memory";
		return false;
	}

	// Setup wad type
	char wad_type[4] = { 'P', 'W', 'A', 'D' };
	if (iwad_)
		wad_type[0] = 'I';

	// Write the header
	uint32_t num_lumps = numEntries();
	mc.write(wad_type, 4);
	mc.write(&num_lumps, 4);
	mc.write(&dir_offset, 4);

	// Write the lumps
	for (uint32_t l = 0; l < num_lumps; l++)
	{
		entry = entryAt(l);
		mc.write(entry->rawData(), entry->size());
	}

	// Write the directory
	for (uint32_t l = 0; l < num_lumps; l++)
	{
		entry        = entryAt(l);
		char name[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
		long offset  = getEntryOffset(entry);
		long size    = entry->size();

		for (size_t c = 0; c < entry->name().length() && c < 8; c++)
			name[c] = entry->name()[c];

		mc.write(&offset, 4);
		mc.write(&size, 4);
		mc.write(name, 8);

		if (update)
		{
			entry->setState(ArchiveEntry::State::Unmodified);
			entry->exProp("Offset") = (int)offset;
		}
	}

	return true;
}

// -----------------------------------------------------------------------------
// Writes the wad archive to a file at [filename]
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool WadArchive::write(const string& filename, bool update)
{
	// Don't write if iwad
	if (iwad_ && iwad_lock)
	{
		Global::error = "IWAD saving disabled";
		return false;
	}

	// Open file for writing
	wxFile file;
	file.Open(filename, wxFile::write);
	if (!file.IsOpened())
	{
		Global::error = "Unable to open file for writing";
		return false;
	}

	// Determine directory offset & individual lump offsets
	uint32_t      dir_offset = 12;
	ArchiveEntry* entry;
	for (uint32_t l = 0; l < numEntries(); l++)
	{
		entry = entryAt(l);
		setEntryOffset(entry, dir_offset);
		dir_offset += entry->size();
	}

	// Setup wad type
	char wad_type[4] = { 'P', 'W', 'A', 'D' };
	if (iwad_)
		wad_type[0] = 'I';

	// Write the header
	uint32_t num_lumps = numEntries();
	file.Write(wad_type, 4);
	file.Write(&num_lumps, 4);
	file.Write(&dir_offset, 4);

	// Write the lumps
	for (uint32_t l = 0; l < num_lumps; l++)
	{
		entry = entryAt(l);
		if (entry->size())
		{
			file.Write(entry->rawData(), entry->size());
		}
	}

	// Write the directory
	for (uint32_t l = 0; l < num_lumps; l++)
	{
		entry        = entryAt(l);
		char name[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
		long offset  = getEntryOffset(entry);
		long size    = entry->size();

		for (size_t c = 0; c < entry->name().length() && c < 8; c++)
			name[c] = entry->name()[c];

		file.Write(&offset, 4);
		file.Write(&size, 4);
		file.Write(name, 8);

		if (update)
		{
			entry->setState(ArchiveEntry::State::Unmodified);
			entry->exProp("Offset") = (int)offset;
		}
	}

	file.Close();

	return true;
}

// -----------------------------------------------------------------------------
// Loads an entry's data from the wadfile
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool WadArchive::loadEntryData(ArchiveEntry* entry)
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

	// Open wadfile
	wxFile file(filename_);

	// Check if opening the file failed
	if (!file.IsOpened())
	{
		Log::error(S_FMT("WadArchive::loadEntryData: Failed to open wadfile %s", filename_));
		return false;
	}

	// Seek to lump offset in file and read it in
	file.Seek(getEntryOffset(entry), wxFromStart);
	entry->importFileStream(file, entry->size());

	// Set the lump to loaded
	entry->setLoaded();
	entry->setState(ArchiveEntry::State::Unmodified);

	return true;
}

// -----------------------------------------------------------------------------
// Override of Archive::addEntry to force entry addition to the root directory,
// update namespaces if needed and rename the entry if necessary to be
// wad-friendly (8 characters max and no file extension)
// -----------------------------------------------------------------------------
ArchiveEntry* WadArchive::addEntry(ArchiveEntry* entry, unsigned position, ArchiveTreeNode* dir, bool copy)
{
	// Check entry
	if (!entry)
		return nullptr;

	// Check if read-only
	if (isReadOnly())
		return nullptr;

	// Copy if necessary
	if (copy)
		entry = new ArchiveEntry(*entry);

	// Do default entry addition (to root directory)
	TreelessArchive::addEntry(entry, position);

	// Update namespaces if necessary
	if (isNamespaceEntry(entry))
		updateNamespaces();

	return entry;
}

// -----------------------------------------------------------------------------
// Adds [entry] to the end of the namespace matching [add_namespace].
// If [copy] is true a copy of the entry is added.
// Returns the added entry or NULL if the entry is invalid
// -----------------------------------------------------------------------------
ArchiveEntry* WadArchive::addEntry(ArchiveEntry* entry, const string& add_namespace, bool copy)
{
	// Find requested namespace
	for (auto& ns : namespaces_)
	{
		if (S_CMPNOCASE(ns.name, add_namespace))
		{
			// Namespace found, add entry before end marker
			return addEntry(entry, ns.end_index++, nullptr, copy);
		}
	}

	// If the requested namespace is a special namespace and doesn't exist, create it
	for (auto& special_ns : special_namespaces)
	{
		if (add_namespace == special_ns.name)
		{
			addNewEntry(special_ns.letter + "_start");
			addNewEntry(special_ns.letter + "_end");
			return addEntry(entry, add_namespace, copy);
		}
	}

	// Unsupported namespace not found, so add to global namespace (ie end of archive)
	return addEntry(entry, 0xFFFFFFFF, nullptr, copy);
}

// -----------------------------------------------------------------------------
// Override of Archive::removeEntry to update namespaces if needed
// -----------------------------------------------------------------------------
bool WadArchive::removeEntry(ArchiveEntry* entry)
{
	// Check entry
	if (!checkEntry(entry))
		return false;

	// Get entry name (for later)
	string name = entry->upperName();

	// Do default remove
	if (Archive::removeEntry(entry))
	{
		// Update namespaces if necessary
		if (name.EndsWith("_START") || name.EndsWith("_END"))
			updateNamespaces();

		return true;
	}
	else
		return false;
}

// -----------------------------------------------------------------------------
// Override of Archive::renameEntry to update namespaces if needed and rename
// the entry if necessary to be wad-friendly (8 characters max and no file
// extension)
// -----------------------------------------------------------------------------
bool WadArchive::renameEntry(ArchiveEntry* entry, const string& name)
{
	// Check entry
	if (!checkEntry(entry))
		return false;

	// Get current name (for later)
	string name_prev = entry->upperName();

	// Do default rename
	if (Archive::renameEntry(entry, name))
	{
		// Update namespaces if necessary
		if (name_prev.EndsWith("_START") || name_prev.EndsWith("_END") || isNamespaceEntry(entry))
			updateNamespaces();

		return true;
	}
	return false;
}

// -----------------------------------------------------------------------------
// Override of Archive::swapEntries to update namespaces if needed
// -----------------------------------------------------------------------------
bool WadArchive::swapEntries(ArchiveEntry* entry1, ArchiveEntry* entry2)
{
	// Check entries
	if (!checkEntry(entry1) || !checkEntry(entry2))
		return false;

	// Do default swap (force root dir)
	if (Archive::swapEntries(entry1, entry2))
	{
		// Update namespaces if needed
		if (isNamespaceEntry(entry1) || isNamespaceEntry(entry2))
			updateNamespaces();

		return true;
	}
	else
		return false;
}

// -----------------------------------------------------------------------------
// Override of Archive::moveEntry to update namespaces if needed
// -----------------------------------------------------------------------------
bool WadArchive::moveEntry(ArchiveEntry* entry, unsigned position, ArchiveTreeNode* dir)
{
	// Check entry
	if (!checkEntry(entry))
		return false;

	// Do default move (force root dir)
	if (Archive::moveEntry(entry, position, nullptr))
	{
		// Update namespaces if necessary
		if (isNamespaceEntry(entry))
			updateNamespaces();

		return true;
	}
	else
		return false;
}

// -----------------------------------------------------------------------------
// Returns the MapDesc information about the map beginning at [maphead].
// If [maphead] is not really a map header entry, an invalid MapDesc will be
// returned (MapDesc::head == nullptr)
// -----------------------------------------------------------------------------
Archive::MapDesc WadArchive::mapDesc(ArchiveEntry* maphead)
{
	MapDesc map;

	if (!maphead)
		return map;

	// Check for embedded wads (e.g., Doom 64 maps)
	if (maphead->type()->formatId() == "archive_wad")
	{
		map.archive = true;
		map.head    = maphead;
		map.end     = maphead;
		map.name    = maphead->name();
		return map;
	}

	// Check for UDMF format map
	if (S_CMPNOCASE(maphead->nextEntry()->name(), "TEXTMAP"))
	{
		// Get map info
		map.head   = maphead;
		map.name   = maphead->name();
		map.format = MapFormat::UDMF;

		// All entries until we find ENDMAP
		auto entry = maphead->nextEntry();
		while (true)
		{
			if (!entry || S_CMPNOCASE(entry->name(), "ENDMAP"))
				break;

			// Check for unknown map lumps
			bool known = false;
			for (unsigned a = 0; a < NUMMAPLUMPS; a++)
			{
				if (S_CMPNOCASE(entry->name(), map_lumps[a]))
				{
					known = true;
					a     = NUMMAPLUMPS;
				}
			}
			if (!known)
				map.unk.push_back(entry);

			// Next entry
			entry = entry->nextEntry();
		}

		// If we got to the end before we found ENDMAP, something is wrong
		if (!entry)
			return MapDesc();

		// Set end entry
		map.end = entry;

		return map;
	}

	// Check for doom/hexen format map
	uint8_t existing_map_lumps[NUMMAPLUMPS];
	memset(existing_map_lumps, 0, NUMMAPLUMPS);
	auto entry = maphead->nextEntry();
	while (entry)
	{
		// Check that the entry is a valid map-related entry
		bool mapentry = false;
		for (unsigned a = 0; a < NUMMAPLUMPS; a++)
		{
			if (S_CMPNOCASE(entry->name(), map_lumps[a]))
			{
				mapentry              = true;
				existing_map_lumps[a] = 1;
				break;
			}
			else if (a == LUMP_GL_HEADER)
			{
				string name = maphead->name(true);
				name.Prepend("GL_");
				if (S_CMPNOCASE(entry->name(), name))
				{
					mapentry              = true;
					existing_map_lumps[a] = 1;
					break;
				}
			}
		}

		// If it wasn't a map entry, exit this loop
		if (!mapentry)
		{
			entry = entry->prevEntry();
			break;
		}

		// If we've reached the end of the archive, exit this loop
		if (!entry->nextEntry())
			break;

		// Go to next entry
		entry = entry->nextEntry();
	}

	// Check for the required map entries
	for (unsigned a = 0; a < 5; a++)
	{
		if (existing_map_lumps[a] == 0)
			return {};
	}

	// Setup map info
	map.head = maphead;
	map.end  = entry;
	map.name = maphead->name();

	// If BEHAVIOR lump exists, it's a hexen format map
	if (existing_map_lumps[LUMP_BEHAVIOR])
		map.format = MapFormat::Hexen;
	// If LEAFS, LIGHTS and MACROS exist, it's a doom 64 format map
	else if (existing_map_lumps[LUMP_LEAFS] && existing_map_lumps[LUMP_LIGHTS] && existing_map_lumps[LUMP_MACROS])
		map.format = MapFormat::Doom64;
	// Otherwise it's doom format
	else
		map.format = MapFormat::Doom;

	return map;
}

// -----------------------------------------------------------------------------
// Searches for any maps in the wad and adds them to the map list
// -----------------------------------------------------------------------------
vector<Archive::MapDesc> WadArchive::detectMaps()
{
	vector<MapDesc> maps;

	// Go through all lumps
	auto entry               = entryAt(0);
	bool lastentryismapentry = false;
	while (entry)
	{
		// UDMF format map check ********************************************************

		// Check for UDMF format map lump (TEXTMAP lump)
		if (entry->name() == "TEXTMAP" && entry->prevEntry())
		{
			// Get map info
			auto md = mapDesc(entry->prevEntry());

			// Add to map list
			if (md.head != nullptr)
			{
				entry = md.end;
				maps.push_back(md);
			}

			// Current index is ENDMAP, we don't want to check for a doom/hexen format
			// map so just go to the next index and continue the loop
			entry = entry->nextEntry();
			continue;
		}

		// Doom/Hexen format map check **************************************************
		// TODO maybe get rid of code duplication by calling getMapInfo() here too?

		// Array to keep track of what doom/hexen map lumps have been found
		uint8_t existing_map_lumps[NUMMAPLUMPS];
		memset(existing_map_lumps, 0, NUMMAPLUMPS);

		// Check if the current lump is a doom/hexen map lump
		bool maplump_found = false;
		for (int a = 0; a < 5; a++)
		{
			// Compare with all base map lump names
			if (S_CMP(entry->name(), map_lumps[a]))
			{
				maplump_found         = true;
				existing_map_lumps[a] = 1;
				break;
			}
		}

		// If we've found what might be a map
		if (maplump_found && entry->prevEntry())
		{
			// Save map header entry
			auto header_entry = entry->prevEntry();

			// Check off map lumps until we find a non-map lump
			bool done = false;
			while (!done)
			{
				// Loop will end if no map lump is found
				done = true;

				// Compare with all map lump names
				for (int a = 0; a < NUMMAPLUMPS; a++)
				{
					// Compare with all base map lump names
					if (S_CMP(entry->name(), map_lumps[a]))
					{
						existing_map_lumps[a] = 1;
						done                  = false;
						break;
					}
				}

				// If we're at the end of the wad, exit the loop
				if (!entry->nextEntry())
				{
					lastentryismapentry = true;
					break;
				}

				// Go to next lump if there is one
				if (!lastentryismapentry)
					entry = entry->nextEntry();
			}

			// Go back to the lump just after the last map lump found, but only if we actually moved
			if (!lastentryismapentry)
				entry = entry->prevEntry();

			// Check that we have all the required map lumps: VERTEXES, LINEDEFS, SIDEDEFS, THINGS & SECTORS
			if (!memchr(existing_map_lumps, 0, 5))
			{
				// Get map info
				MapDesc md;
				md.head = header_entry;         // Header lump
				md.name = header_entry->name(); // Map title
				md.end  = lastentryismapentry ? // End lump
							 entry :
							 entry->prevEntry();

				// If BEHAVIOR lump exists, it's a hexen format map
				if (existing_map_lumps[LUMP_BEHAVIOR])
					md.format = MapFormat::Hexen;
				// If LEAFS, LIGHTS and MACROS exist, it's a doom 64 format map
				else if (
					existing_map_lumps[LUMP_LEAFS] && existing_map_lumps[LUMP_LIGHTS]
					&& existing_map_lumps[LUMP_MACROS])
					md.format = MapFormat::Doom64;
				// Otherwise it's doom format
				else
					md.format = MapFormat::Doom;

				// Add map info to the maps list
				maps.push_back(md);
			}
		}

		// Embedded WAD check (for Doom 64)
		if (entry->type()->formatId() == "archive_wad")
		{
			// Detect map format (probably kinda slow but whatever, no better way to do it really)
			WadArchive tempwad;
			tempwad.open(entry->data());
			auto emaps = tempwad.detectMaps();
			if (!emaps.empty())
			{
				MapDesc md;
				md.head    = entry;
				md.end     = entry;
				md.archive = true;
				md.name    = entry->name(true).Upper();
				md.format  = emaps[0].format;
				maps.push_back(md);
			}
			entry->unlock();
		}

		// Not a UDMF or Doom/Hexen map lump, go to next lump
		entry = entry->nextEntry();
	}

	// Set all map header entries to ETYPE_MAP type
	for (auto& map : maps)
		if (!map.archive)
			map.head->setType(EntryType::mapMarkerType());

	// Update entry map format hints
	for (auto& map : maps)
	{
		string format;
		if (map.format == MapFormat::Doom)
			format = "doom";
		else if (map.format == MapFormat::Doom64)
			format = "doom64";
		else if (map.format == MapFormat::Hexen)
			format = "hexen";
		else
			format = "udmf";

		auto m_entry = map.head;
		while (m_entry && m_entry != map.end->nextEntry())
		{
			m_entry->exProp("MapFormat") = format;
			m_entry                      = m_entry->nextEntry();
		}
	}

	return maps;
}

// -----------------------------------------------------------------------------
// Returns the namespace that [entry] is within
// -----------------------------------------------------------------------------
string WadArchive::detectNamespace(ArchiveEntry* entry)
{
	return detectNamespace(entryIndex(entry));
}

// -----------------------------------------------------------------------------
// Returns the namespace that the entry at [index] in [dir] is within
// -----------------------------------------------------------------------------
string WadArchive::detectNamespace(size_t index, ArchiveTreeNode* dir)
{
	// Go through namespaces
	for (auto& ns : namespaces_)
	{
		// Get namespace start and end indices
		size_t start = ns.start_index;
		size_t end   = ns.end_index;

		// Check if the entry is within this namespace
		if (start <= index && index <= end)
			return ns.name;
	}

	// In no namespace
	return "global";
}

// -----------------------------------------------------------------------------
// Parses the DECORATE, GLDEFS, etc. lumps for included files, and mark them as
// being of the same type
// -----------------------------------------------------------------------------
void WadArchive::detectIncludes()
{
	// DECORATE: #include "lumpname"
	// GLDEFS: #include "lumpname"
	// SBARINFO: #include "lumpname"
	// ZMAPINFO: translator = "lumpname"
	// EMAPINFO: extradata = lumpname
	// EDFROOT: lumpinclude("lumpname")

	static const char* lumptypes[6]  = { "DECORATE", "GLDEFS", "SBARINFO", "ZMAPINFO", "EMAPINFO", "EDFROOT" };
	static const char* entrytypes[6] = { "decorate", "gldefslump", "sbarinfo", "xlat", "extradata", "edf" };
	static const char* tokens[6]     = { "#include", "#include", "#include", "translator", "extradata", "lumpinclude" };

	Archive::SearchOptions opt;
	opt.ignore_ext = true;
	Tokenizer tz;
	tz.setSpecialCharacters(";,:|={}/()");

	for (int i = 0; i < 6; ++i)
	{
		opt.match_name = lumptypes[i];
		auto entries   = findAll(opt);
		if (!entries.empty())
		{
			for (auto& entry : entries)
			{
				tz.openMem(entry->data(), lumptypes[i]);

				while (!tz.atEnd())
				{
					if (tz.checkNC(tokens[i]))
					{
						if (i >= 3) // skip '=' or '('
							tz.adv();
						string name = tz.next().text;
						if (i == 5) // skip ')'
							tz.adv();
						opt.match_name = name;
						auto match     = findFirst(opt);
						if (match)
							match->setType(EntryType::fromId(entrytypes[i]));
						tz.adv();
					}
					else
						tz.advToNextLine();
				}
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Returns the first entry matching the search criteria in [options], or null if
// no matching entry was found
// -----------------------------------------------------------------------------
ArchiveEntry* WadArchive::findFirst(SearchOptions& options)
{
	// Init search variables
	auto          start = entryAt(0);
	ArchiveEntry* end   = nullptr;
	options.match_name  = options.match_name.Lower();

	// "graphics" namespace is the global namespace in a wad
	if (options.match_namespace == "graphics")
		options.match_namespace = "";

	// Check for namespace to search
	if (!options.match_namespace.IsEmpty())
	{
		// Find matching namespace
		bool ns_found = false;
		for (auto& ns : namespaces_)
		{
			if (ns.name == options.match_namespace)
			{
				start    = ns.start->nextEntry();
				end      = ns.end;
				ns_found = true;
				break;
			}
		}

		// Return none if namespace not found
		if (!ns_found)
			return nullptr;
	}

	// Begin search
	auto entry = start;
	while (entry != end)
	{
		// Check type
		if (options.match_type)
		{
			if (entry->type() == EntryType::unknownType())
			{
				if (!options.match_type->isThisType(entry))
				{
					entry = entry->nextEntry();
					continue;
				}
			}
			else if (options.match_type != entry->type())
			{
				entry = entry->nextEntry();
				continue;
			}
		}

		// Check name
		if (!options.match_name.IsEmpty())
		{
			if (!options.match_name.Matches(entry->name().Lower()))
			{
				entry = entry->nextEntry();
				continue;
			}
		}

		// Entry passed all checks so far, so we found a match
		return entry;
	}

	// No match found
	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the last entry matching the search criteria in [options], or null if
// no matching entry was found
// -----------------------------------------------------------------------------
ArchiveEntry* WadArchive::findLast(SearchOptions& options)
{
	// Init search variables
	auto          start = entryAt(numEntries() - 1);
	ArchiveEntry* end   = nullptr;
	options.match_name  = options.match_name.Lower();

	// "graphics" namespace is the global namespace in a wad
	if (options.match_namespace == "graphics")
		options.match_namespace = "";

	// "global" namespace has no name, by the way
	if (options.match_namespace == "global")
		options.match_namespace = "";

	// Check for namespace to search
	if (!options.match_namespace.IsEmpty())
	{
		// Find matching namespace
		bool ns_found = false;
		for (auto& ns : namespaces_)
		{
			if (ns.name == options.match_namespace)
			{
				start    = ns.end->prevEntry();
				end      = ns.start;
				ns_found = true;
				break;
			}
		}

		// Return none if namespace not found
		if (!ns_found)
			return nullptr;
	}

	// Begin search
	auto entry = start;
	while (entry != end)
	{
		// Check type
		if (options.match_type)
		{
			if (entry->type() == EntryType::unknownType())
			{
				if (!options.match_type->isThisType(entry))
				{
					entry = entry->prevEntry();
					continue;
				}
			}
			else if (options.match_type != entry->type())
			{
				entry = entry->prevEntry();
				continue;
			}
		}

		// Check name
		if (!options.match_name.IsEmpty())
		{
			if (!options.match_name.Matches(entry->name().Lower()))
			{
				entry = entry->prevEntry();
				continue;
			}
		}

		// Entry passed all checks so far, so we found a match
		return entry;
	}

	// No match found
	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns all entries matching the search criteria in [options]
// -----------------------------------------------------------------------------
vector<ArchiveEntry*> WadArchive::findAll(SearchOptions& options)
{
	// Init search variables
	auto          start = entryAt(0);
	ArchiveEntry* end   = nullptr;
	options.match_name  = options.match_name.Upper();
	vector<ArchiveEntry*> ret;

	// "graphics" namespace is the global namespace in a wad
	if (options.match_namespace == "graphics")
		options.match_namespace = "";

	// Check for namespace to search
	if (!options.match_namespace.IsEmpty())
	{
		// Find matching namespace
		bool   ns_found        = false;
		size_t namespaces_size = namespaces_.size();
		for (unsigned a = 0; a < namespaces_size; a++)
		{
			if (namespaces_[a].name == options.match_namespace)
			{
				start    = namespaces_[a].start->nextEntry();
				end      = namespaces_[a].end;
				ns_found = true;
				break;
			}
		}

		// Return none if namespace not found
		if (!ns_found)
			return ret;
	}

	auto entry = start;
	while (entry != end)
	{
		// Check type
		if (options.match_type)
		{
			if (entry->type() == EntryType::unknownType())
			{
				if (!options.match_type->isThisType(entry))
				{
					entry = entry->nextEntry();
					continue;
				}
			}
			else if (options.match_type != entry->type())
			{
				entry = entry->nextEntry();
				continue;
			}
		}

		// Check name
		if (!options.match_name.IsEmpty())
		{
			if (!options.match_name.Matches(entry->upperName()))
			{
				entry = entry->nextEntry();
				continue;
			}
		}

		// Entry passed all checks so far, so we found a match
		ret.push_back(entry);
		entry = entry->nextEntry();
	}

	// Return search result
	return ret;
}


// -----------------------------------------------------------------------------
//
// WadArchive Class Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Checks if the given data is a valid Doom wad archive
// -----------------------------------------------------------------------------
bool WadArchive::isWadArchive(MemChunk& mc)
{
	// Check size
	if (mc.size() < 12)
		return false;

	// Check for IWAD/PWAD header
	if (!(mc[1] == 'W' && mc[2] == 'A' && mc[3] == 'D' && (mc[0] == 'P' || mc[0] == 'I')))
		return false;

	// Get number of lumps and directory offset
	uint32_t num_lumps  = 0;
	uint32_t dir_offset = 0;
	mc.seek(4, SEEK_SET);
	mc.read(&num_lumps, 4);
	mc.read(&dir_offset, 4);

	// Reset MemChunk (just in case)
	mc.seek(0, SEEK_SET);

	// Byteswap values for big endian if needed
	num_lumps  = wxINT32_SWAP_ON_BE(num_lumps);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);

	// Check directory offset is decent
	if ((dir_offset + (num_lumps * 16)) > mc.size() || dir_offset < 12)
		return false;

	// If it's passed to here it's probably a wad file
	return true;
}

// -----------------------------------------------------------------------------
// Checks if the file at [filename] is a valid Doom wad archive
// -----------------------------------------------------------------------------
bool WadArchive::isWadArchive(const string& filename)
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
	if (!(header[1] == 'W' && header[2] == 'A' && header[3] == 'D' && (header[0] == 'P' || header[0] == 'I')))
		return false;

	// Get number of lumps and directory offset
	uint32_t num_lumps  = 0;
	uint32_t dir_offset = 0;
	file.Read(&num_lumps, 4);
	file.Read(&dir_offset, 4);

	// Byteswap values for big endian if needed
	num_lumps  = wxINT32_SWAP_ON_BE(num_lumps);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);

	// Check directory offset is decent
	if ((dir_offset + (num_lumps * 16)) > file.Length() || dir_offset < 12)
		return false;

	// If it's passed to here it's probably a wad file
	return true;
}
