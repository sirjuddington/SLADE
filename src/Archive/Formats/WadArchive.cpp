
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
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
#include "Utility/StringUtils.h"
#include "Utility/Tokenizer.h"
#include "WadJArchive.h"

using namespace slade;


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
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Returns true if [entry] is a namespace marker (*_START / *_END)
// -----------------------------------------------------------------------------
bool isNamespaceEntry(const ArchiveEntry* entry)
{
	return strutil::endsWith(entry->upperName(), "_START") || strutil::endsWith(entry->upperName(), "_END");
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
		if (strutil::endsWith(entry->upperName(), "_START"))
		{
			log::debug("Found namespace start marker {} at index {}", entry->name(), entryIndex(entry));

			// Create new namespace
			NSPair      ns(entry, nullptr);
			string_view name = entry->name();
			ns.name          = name.substr(0, name.size() - 6);
			ns.start_index   = entryIndex(ns.start);
			strutil::lowerIP(ns.name);

			// Convert some special cases (because technically PP_START->P_END is a valid namespace)
			if (ns.name == "pp")
				ns.name = "p";
			if (ns.name == "ff")
				ns.name = "f";
			if (ns.name == "ss")
				ns.name = "s";
			if (ns.name == "tt")
				ns.name = "t";

			log::debug("Added namespace {}", ns.name);

			// Add to namespace list
			namespaces_.push_back(ns);
		}
		// Check for namespace end
		// else if (strutil::matches(entry->upperName(), "?_END") || strutil::matches(entry->upperName(), "??_END"))
		else if (strutil::endsWith(entry->upperName(), "_END"))
		{
			log::debug("Found namespace end marker {} at index {}", entry->name(), entryIndex(entry));

			// Get namespace 'name'
			auto ns_name = strutil::lower(entry->name());
			strutil::removeLastIP(ns_name, 4);

			// Convert some special cases (because technically P_START->PP_END is a valid namespace)
			if (ns_name == "pp")
				ns_name = "p";
			if (ns_name == "ff")
				ns_name = "f";
			if (ns_name == "ss")
				ns_name = "s";
			if (ns_name == "tt")
				ns_name = "t";

			log::debug("Namespace name {}", ns_name);

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
				if (strutil::equalCI(ns.name, ns_name))
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
	if (numEntries() > 2090 && rootDir()->entryAt(0)->upperName() == "WALLSTRT"
		&& rootDir()->entryAt(numEntries() - 2)->upperName() == "TABLES")
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
			if (ns.name == special_namespace.letter)
				ns.name = special_namespace.name;
		}

		ns.start_index = entryIndex(ns.start);
		ns.end_index   = entryIndex(ns.end);

		// Testing
		// log::info(1, "Namespace %s from %s (%d) to %s (%d)", ns.name,
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
		log::error("WadArchive::openFile: File {} has invalid header", filename_);
		global::error = "Invalid wad header";
		return false;
	}

	// Check for iwad
	if (wad_type[0] == 'I')
		iwad_ = true;

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	ArchiveModSignalBlocker sig_blocker{ *this };

	vector<uint32_t> offsets;

	// Read the directory
	MemChunk edata;
	mc.seek(dir_offset, SEEK_SET);
	ui::setSplashProgressMessage("Reading wad archive data");
	for (uint32_t d = 0; d < num_lumps; d++)
	{
		// Update splash window progress
		ui::setSplashProgress(d, num_lumps);

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
				log::info(2, "No.");
				continue;
			}
			if (VECTOR_EXISTS(offsets, offset))
			{
				log::warning("Ignoring entry {}: {}, is a clone of a previous entry", d, name);
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
			log::error("WadArchive::open: Wad archive is invalid or corrupt");
			global::error = fmt::format(
				"Archive is invalid and/or corrupt (lump {}: {} data goes past end of file)", d, name);
			return false;
		}

		// Create & setup lump
		auto nlump = std::make_shared<ArchiveEntry>(name, size);
		nlump->setOffsetOnDisk(offset);
		nlump->setSizeOnDisk(size);

		if (jaguarencrypt)
		{
			nlump->setEncryption(ArchiveEntry::Encryption::Jaguar);
			nlump->exProp("FullSize") = (int)size;
		}

		// Read entry data if it isn't zero-sized
		if (nlump->size() > 0)
		{
			// Read the entry data
			mc.exportMemChunk(edata, offset, size);
			if (nlump->encryption() != ArchiveEntry::Encryption::None)
			{
				if (nlump->exProps().contains("FullSize")
					&& static_cast<unsigned>(nlump->exProp<int>("FullSize")) > size)
					edata.reSize((nlump->exProp<int>("FullSize")), true);
				if (!WadJArchive::jaguarDecode(edata))
					log::warning(
						"{}: {} (following {}), did not decode properly",
						d,
						nlump->name(),
						d > 0 ? entryAt(d - 1)->name() : "nothing");
			}
			nlump->importMemChunk(edata);
		}

		nlump->setState(ArchiveEntry::State::Unmodified);

		// Add to entry list
		rootDir()->addEntry(nlump);
	}

	// Detect namespaces (needs to be done before type detection as some types
	// rely on being within certain namespaces)
	updateNamespaces();

	// Detect all entry types
	detectAllEntryTypes();

	// Identify #included lumps (DECORATE, GLDEFS, etc.)
	detectIncludes();

	// Detect maps (will detect map entry types)
	ui::setSplashProgressMessage("Detecting maps");
	detectMaps();

	// Setup variables
	sig_blocker.unblock();
	setModified(false);

	ui::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Writes the wad archive to a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool WadArchive::write(MemChunk& mc)
{
	// Don't write if iwad
	if (iwad_ && iwad_lock)
	{
		global::error = "IWAD saving disabled";
		return false;
	}

	// Determine directory offset & individual lump offsets
	uint32_t      dir_offset = 12;
	ArchiveEntry* entry;
	for (uint32_t l = 0; l < numEntries(); l++)
	{
		entry = entryAt(l);
		entry->setOffsetOnDisk(dir_offset);
		dir_offset += entry->size();
	}

	// Clear/init MemChunk
	mc.clear();
	mc.seek(0, SEEK_SET);
	if (!mc.reSize(dir_offset + numEntries() * 16))
	{
		global::error = "Failed to allocate sufficient memory";
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
		long offset  = entry->offsetOnDisk();
		long size    = entry->size();

		for (size_t c = 0; c < entry->name().length() && c < 8; c++)
			name[c] = entry->name()[c];

		mc.write(&offset, 4);
		mc.write(&size, 4);
		mc.write(name, 8);

		entry->setState(ArchiveEntry::State::Unmodified);
		entry->setSizeOnDisk();
	}

	return true;
}

// -----------------------------------------------------------------------------
// Writes the wad archive to a file at [filename]
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool WadArchive::write(string_view filename)
{
	// Don't write if iwad
	if (iwad_ && iwad_lock)
	{
		global::error = "IWAD saving disabled";
		return false;
	}

	// Open file for writing
	wxFile file;
	file.Open(wxString{ filename.data(), filename.size() }, wxFile::write);
	if (!file.IsOpened())
	{
		global::error = "Unable to open file for writing";
		return false;
	}

	// Determine directory offset & individual lump offsets
	uint32_t      dir_offset = 12;
	ArchiveEntry* entry;
	for (uint32_t l = 0; l < numEntries(); l++)
	{
		entry = entryAt(l);
		entry->setOffsetOnDisk(dir_offset);
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
		long offset  = entry->offsetOnDisk();
		long size    = entry->size();

		for (size_t c = 0; c < entry->name().length() && c < 8; c++)
			name[c] = entry->name()[c];

		file.Write(&offset, 4);
		file.Write(&size, 4);
		file.Write(name, 8);

		entry->setState(ArchiveEntry::State::Unmodified);
		entry->setSizeOnDisk();
	}

	file.Close();

	return true;
}

// -----------------------------------------------------------------------------
// Loads an [entry]'s data from the archive file on disk into [out]
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool WadArchive::loadEntryData(const ArchiveEntry* entry, MemChunk& out)
{
	return genericLoadEntryData(entry, out);
}

// -----------------------------------------------------------------------------
// Override of Archive::addEntry to force entry addition to the root directory,
// update namespaces if needed and rename the entry if necessary to be
// wad-friendly (8 characters max and no file extension)
// -----------------------------------------------------------------------------
shared_ptr<ArchiveEntry> WadArchive::addEntry(shared_ptr<ArchiveEntry> entry, unsigned position, ArchiveDir* dir)
{
	// Check entry
	if (!entry)
		return nullptr;

	// Check if read-only
	if (isReadOnly())
		return nullptr;

	// Do default entry addition (to root directory)
	TreelessArchive::addEntry(entry, position);

	// Update namespaces if necessary
	if (isNamespaceEntry(entry.get()))
		updateNamespaces();

	return entry;
}

// -----------------------------------------------------------------------------
// Adds [entry] to the end of the namespace matching [add_namespace].
// If [copy] is true a copy of the entry is added.
// Returns the added entry or NULL if the entry is invalid
// -----------------------------------------------------------------------------
shared_ptr<ArchiveEntry> WadArchive::addEntry(shared_ptr<ArchiveEntry> entry, string_view add_namespace)
{
	// Find requested namespace
	for (auto& ns : namespaces_)
	{
		if (strutil::equalCI(ns.name, add_namespace))
		{
			// Namespace found, add entry before end marker
			return addEntry(entry, ns.end_index++, nullptr);
		}
	}

	// If the requested namespace is a special namespace and doesn't exist, create it
	for (auto& special_ns : special_namespaces)
	{
		if (add_namespace == special_ns.name)
		{
			addNewEntry(special_ns.letter + "_start");
			addNewEntry(special_ns.letter + "_end");
			return addEntry(entry, add_namespace);
		}
	}

	// Unsupported namespace not found, so add to global namespace (ie end of archive)
	return addEntry(entry, 0xFFFFFFFF, nullptr);
}

// -----------------------------------------------------------------------------
// Override of Archive::removeEntry to update namespaces if needed
// -----------------------------------------------------------------------------
bool WadArchive::removeEntry(ArchiveEntry* entry)
{
	// Check entry
	if (!checkEntry(entry))
		return false;

	// Check if namespace entry
	bool ns_entry = isNamespaceEntry(entry);

	// Do default remove
	if (Archive::removeEntry(entry))
	{
		// Update namespaces if necessary
		if (ns_entry)
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
bool WadArchive::renameEntry(ArchiveEntry* entry, string_view name)
{
	// Check entry
	if (!checkEntry(entry))
		return false;

	// Check if current name is a namespace marker
	bool ns_entry = isNamespaceEntry(entry);

	// Do default rename
	if (Archive::renameEntry(entry, name))
	{
		// Update namespaces if necessary
		if (ns_entry || isNamespaceEntry(entry))
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
bool WadArchive::moveEntry(ArchiveEntry* entry, unsigned position, ArchiveDir* dir)
{
	// Check entry
	if (!checkEntry(entry))
		return false;

	// Do default move (force root dir)
	if (TreelessArchive::moveEntry(entry, position, nullptr))
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

	auto s_maphead = maphead->getShared();
	if (!s_maphead)
		return map;

	// Check for embedded wads (e.g., Doom 64 maps)
	if (maphead->type()->formatId() == "archive_wad")
	{
		map.archive = true;
		map.head    = s_maphead;
		map.end     = s_maphead;
		map.name    = maphead->name();
		return map;
	}

	auto dir         = maphead->parentDir();
	auto head_index  = dir->entryIndex(maphead);
	int  entry_count = dir->numEntries();

	// Check for UDMF format map
	if (head_index < entry_count && dir->entryAt(head_index + 1)->upperName() == "TEXTMAP")
	{
		// Get map info
		map.head   = s_maphead;
		map.name   = maphead->name();
		map.format = MapFormat::UDMF;

		// All entries until we find ENDMAP
		auto index = head_index + 1;
		auto entry = dir->sharedEntryAt(index);
		while (index < entry_count)
		{
			if (!entry || entry->upperName() == "ENDMAP")
				break;

			// Check for unknown map lumps
			bool known = false;
			for (unsigned a = 0; a < NUMMAPLUMPS; a++)
			{
				if (entry->upperName() == map_lumps[a])
				{
					known = true;
					a     = NUMMAPLUMPS;
				}
			}
			if (!known)
				map.unk.push_back(entry.get());

			// Next entry
			entry = dir->sharedEntryAt(++index);
		}

		// If we got to the end before we found ENDMAP, something is wrong
		if (!entry)
			return {};

		// Set end entry
		map.end = entry;

		return map;
	}

	// Check for doom/hexen format map
	uint8_t existing_map_lumps[NUMMAPLUMPS] = {};
	auto    index                           = head_index + 1;
	auto    entry                           = dir->sharedEntryAt(index);
	while (entry)
	{
		// Check that the entry is a valid map-related entry
		bool mapentry = false;
		for (unsigned a = 0; a < NUMMAPLUMPS; a++)
		{
			if (entry->upperName() == map_lumps[a])
			{
				mapentry              = true;
				existing_map_lumps[a] = 1;
				break;
			}
			else if (a == LUMP_GL_HEADER)
			{
				string name{ maphead->upperNameNoExt() };
				strutil::prependIP(name, "GL_");
				if (entry->upperName() == name)
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
			entry = dir->sharedEntryAt(--index);
			break;
		}

		// If we've reached the end of the archive, exit this loop
		if (index == entry_count - 1)
			break;

		// Go to next entry
		entry = dir->sharedEntryAt(++index);
	}

	// Check for the required map entries
	for (unsigned a = 0; a < 5; a++)
	{
		if (existing_map_lumps[a] == 0)
			return {};
	}

	// Setup map info
	map.head = s_maphead;
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
	auto index               = 0;
	auto entry_count         = numEntries();
	auto entry               = rootDir()->sharedEntryAt(index);
	bool lastentryismapentry = false;
	while (entry)
	{
		// UDMF format map check ********************************************************

		// Check for UDMF format map lump (TEXTMAP lump)
		if (entry->name() == "TEXTMAP" && index > 0)
		{
			// Get map info
			auto md = mapDesc(entryAt(index - 1));

			// Add to map list
			if (md.head.lock())
			{
				entry = md.end.lock();
				maps.push_back(md);
			}

			// Current index is ENDMAP, we don't want to check for a doom/hexen format
			// map so just go to the next index and continue the loop
			entry = rootDir()->sharedEntryAt(++index);
			continue;
		}

		// Doom/Hexen format map check **************************************************
		// TODO maybe get rid of code duplication by calling getMapInfo() here too?

		// Array to keep track of what doom/hexen map lumps have been found
		uint8_t existing_map_lumps[NUMMAPLUMPS] = {};

		// Check if the current lump is a doom/hexen map lump
		bool maplump_found = false;
		for (int a = 0; a < 5; a++)
		{
			// Compare with all base map lump names
			if (entry->upperName() == map_lumps[a])
			{
				maplump_found         = true;
				existing_map_lumps[a] = 1;
				break;
			}
		}

		// If we've found what might be a map
		if (maplump_found && index > 0)
		{
			// Save map header entry
			auto header_entry = rootDir()->sharedEntryAt(index - 1);

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
					if (entry->upperName() == map_lumps[a])
					{
						existing_map_lumps[a] = 1;
						done                  = false;
						break;
					}
				}

				// If we're at the end of the wad, exit the loop
				if (index == entry_count - 1)
				{
					lastentryismapentry = true;
					break;
				}

				// Go to next lump if there is one
				if (!lastentryismapentry)
					entry = rootDir()->sharedEntryAt(++index);
			}

			// Go back to the lump just after the last map lump found, but only if we actually moved
			if (!lastentryismapentry)
				entry = rootDir()->sharedEntryAt(--index);

			// Check that we have all the required map lumps: VERTEXES, LINEDEFS, SIDEDEFS, THINGS & SECTORS
			if (!memchr(existing_map_lumps, 0, 5))
			{
				// Get map info
				MapDesc md;
				md.head = header_entry;         // Header lump
				md.name = header_entry->name(); // Map title
				md.end  = lastentryismapentry ? // End lump
                             entry :
                              rootDir()->sharedEntryAt(--index);

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
				md.name    = entry->upperNameNoExt();
				md.format  = emaps[0].format;
				maps.push_back(md);
			}
			entry->unlock();
		}

		// Not a UDMF or Doom/Hexen map lump, go to next lump
		entry = rootDir()->sharedEntryAt(++index);
	}

	// Set all map header entries to ETYPE_MAP type
	for (auto& map : maps)
		if (!map.archive && map.head.lock())
			map.head.lock()->setType(EntryType::mapMarkerType());

	// Update entry map format hints
	for (auto& map : maps)
		map.updateMapFormatHints();

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
string WadArchive::detectNamespace(unsigned index, ArchiveDir* dir)
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
						auto name = tz.next().text;
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
	unsigned index     = 0;
	auto     index_end = numEntries();
	strutil::upperIP(options.match_name);

	// "graphics" namespace is the global namespace in a wad
	if (options.match_namespace == "graphics")
		options.match_namespace = "";

	// Check for namespace to search
	if (!options.match_namespace.empty())
	{
		// Find matching namespace
		bool ns_found = false;
		for (auto& ns : namespaces_)
		{
			if (ns.name == options.match_namespace)
			{
				index     = ns.start_index + 1;
				index_end = ns.end_index + 1;
				ns_found  = true;
				break;
			}
		}

		// Return none if namespace not found
		if (!ns_found)
			return nullptr;
	}

	// Begin search
	ArchiveEntry* entry;
	for (; index < index_end; ++index)
	{
		entry = entryAt(index);

		// Check type
		if (options.match_type)
		{
			if (entry->type() == EntryType::unknownType())
			{
				if (!options.match_type->isThisType(*entry))
					continue;
			}
			else if (options.match_type != entry->type())
				continue;
		}

		// Check name
		if (!options.match_name.empty())
		{
			if (!strutil::matches(options.match_name, entry->upperName()))
				continue;
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
	int index       = numEntries() - 1;
	int index_start = 0;
	strutil::upperIP(options.match_name);

	// "graphics" namespace is the global namespace in a wad
	if (options.match_namespace == "graphics")
		options.match_namespace = "";

	// "global" namespace has no name, by the way
	if (options.match_namespace == "global")
		options.match_namespace = "";

	// Check for namespace to search
	if (!options.match_namespace.empty())
	{
		// Find matching namespace
		bool ns_found = false;
		for (auto& ns : namespaces_)
		{
			if (ns.name == options.match_namespace)
			{
				index       = ns.end_index - 1;
				index_start = ns.start_index + 1;
				ns_found    = true;
				break;
			}
		}

		// Return none if namespace not found
		if (!ns_found)
			return nullptr;
	}

	// Begin search
	ArchiveEntry* entry;
	for (; index >= index_start; --index)
	{
		entry = entryAt(index);

		// Check type
		if (options.match_type)
		{
			if (entry->type() == EntryType::unknownType())
			{
				if (!options.match_type->isThisType(*entry))
					continue;
			}
			else if (options.match_type != entry->type())
				continue;
		}

		// Check name
		if (!options.match_name.empty())
		{
			if (!strutil::matches(entry->upperName(), options.match_name))
				continue;
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
	unsigned index     = 0;
	auto     index_end = numEntries();
	strutil::upperIP(options.match_name);
	vector<ArchiveEntry*> ret;

	// "graphics" namespace is the global namespace in a wad
	if (options.match_namespace == "graphics")
		options.match_namespace = "";

	// Check for namespace to search
	if (!options.match_namespace.empty())
	{
		// Find matching namespace
		bool   ns_found        = false;
		size_t namespaces_size = namespaces_.size();
		for (unsigned a = 0; a < namespaces_size; a++)
		{
			if (namespaces_[a].name == options.match_namespace)
			{
				index     = namespaces_[a].start_index + 1;
				index_end = namespaces_[a].end_index + 1;
				ns_found  = true;
				break;
			}
		}

		// Return none if namespace not found
		if (!ns_found)
			return ret;
	}

	ArchiveEntry* entry;
	for (; index < index_end; ++index)
	{
		entry = entryAt(index);

		// Check type
		if (options.match_type)
		{
			if (entry->type() == EntryType::unknownType())
			{
				if (!options.match_type->isThisType(*entry))
					continue;
			}
			else if (options.match_type != entry->type())
				continue;
		}

		// Check name
		if (!options.match_name.empty())
		{
			if (!strutil::matches(entry->upperName(), options.match_name))
				continue;
		}

		// Entry passed all checks so far, so we found a match
		ret.push_back(entry);
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
