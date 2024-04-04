
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    DatArchiveHandler.cpp
// Description: ArchiveFormatHandler for Raven dat/cd/hd archives
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
#include "DatArchiveHandler.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "General/UI.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Returns true if [entry] is a namespace marker
// -----------------------------------------------------------------------------
bool isNamespaceEntry(const ArchiveEntry* entry)
{
	return strutil::startsWith(entry->upperName(), "START") || strutil::startsWith(entry->upperName(), "END");
}
} // namespace


// -----------------------------------------------------------------------------
//
// DatArchiveHandler Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Reads wad format data from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool DatArchiveHandler::open(Archive& archive, const MemChunk& mc)
{
	// Check data was given
	if (!mc.hasData())
		return false;

	const uint8_t* mcdata = mc.data();

	// Read dat header
	mc.seek(0, SEEK_SET);
	uint16_t num_lumps;
	uint32_t dir_offset, unknown;
	mc.read(&num_lumps, 2);  // Size
	mc.read(&dir_offset, 4); // Directory offset
	mc.read(&unknown, 4);    // Unknown value
	num_lumps        = wxINT16_SWAP_ON_BE(num_lumps);
	dir_offset       = wxINT32_SWAP_ON_BE(dir_offset);
	unknown          = wxINT32_SWAP_ON_BE(unknown);
	string lastname  = "-noname-";
	size_t namecount = 0;

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	ArchiveModSignalBlocker sig_blocker{ archive };

	// Read the directory
	mc.seek(dir_offset, SEEK_SET);
	ui::setSplashProgressMessage("Reading dat archive data");
	for (uint32_t d = 0; d < num_lumps; d++)
	{
		// Update splash window progress
		ui::setSplashProgress(d, num_lumps);

		// Read lump info
		uint32_t offset  = 0;
		uint32_t size    = 0;
		uint16_t nameofs = 0;
		uint16_t flags   = 0;

		mc.read(&offset, 4);  // Offset
		mc.read(&size, 4);    // Size
		mc.read(&nameofs, 2); // Name offset
		mc.read(&flags, 2);   // Flags (only one: RLE encoded)

		// Byteswap values for big endian if needed
		offset  = wxINT32_SWAP_ON_BE(offset);
		size    = wxINT32_SWAP_ON_BE(size);
		nameofs = wxINT16_SWAP_ON_BE(nameofs);
		flags   = wxINT16_SWAP_ON_BE(flags);

		// If the lump data goes past the directory,
		// the data file is invalid
		if (offset + size > mc.size())
		{
			log::error("DatArchiveHandler::open: Dat archive is invalid or corrupt at entry {}", d);
			global::error = "Archive is invalid and/or corrupt";
			return false;
		}

		string myname;
		if (nameofs != 0)
		{
			size_t len   = 1;
			size_t start = nameofs + dir_offset;
			for (size_t i = start; mcdata[i] != 0; ++i)
			{
				++len;
			}
			myname.assign(reinterpret_cast<const char*>(mcdata) + start, len);
			lastname  = myname;
			namecount = 0;
		}
		else
		{
			myname = fmt::format("{}+{}", lastname, ++namecount);
		}

		// Create & setup lump
		auto nlump = std::make_shared<ArchiveEntry>(myname, size);
		nlump->setOffsetOnDisk(offset);
		nlump->setSizeOnDisk(size);

		// Read entry data if it isn't zero-sized
		if (size > 0)
			nlump->importMemChunk(mc, offset, size);

		nlump->setState(EntryState::Unmodified);

		if (flags & 1)
			nlump->setEncryption(EntryEncryption::SCRLE0);

		// Check for markers
		if (nlump->name() == "startflats")
			flats_[0] = d;
		if (nlump->name() == "endflats")
			flats_[1] = d;
		if (nlump->name() == "startsprites")
			sprites_[0] = d;
		if (nlump->name() == "endmonsters")
			sprites_[1] = d;
		if (nlump->name() == "startwalls")
			walls_[0] = d;
		if (nlump->name() == "endwalls")
			walls_[1] = d;

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
// Returns the namespace that [entry] is within
// -----------------------------------------------------------------------------
string DatArchiveHandler::detectNamespace(Archive& archive, ArchiveEntry* entry)
{
	return detectNamespace(archive, archive.entryIndex(entry));
}

// -----------------------------------------------------------------------------
// Returns the namespace that the entry at [index] in [dir] is within
// -----------------------------------------------------------------------------
string DatArchiveHandler::detectNamespace(Archive& archive, unsigned index, ArchiveDir* dir)
{
	// Textures
	if (index > static_cast<unsigned>(walls_[0]) && index < static_cast<unsigned>(walls_[1]))
		return "textures";

	// Flats
	if (index > static_cast<unsigned>(flats_[0]) && index < static_cast<unsigned>(flats_[1]))
		return "flats";

	// Sprites
	if (index > static_cast<unsigned>(sprites_[0]) && index < static_cast<unsigned>(sprites_[1]))
		return "sprites";

	return "global";
}

// -----------------------------------------------------------------------------
// Updates the namespace list
// -----------------------------------------------------------------------------
void DatArchiveHandler::updateNamespaces(Archive& archive)
{
	// Clear current namespace info
	sprites_[0] = sprites_[1] = flats_[0] = flats_[1] = walls_[0] = walls_[1] = -1;

	// Go through all entries
	for (unsigned a = 0; a < archive.numEntries(); a++)
	{
		auto entry = archive.rootDir()->entryAt(a);

		// Check for markers
		if (entry->name() == "startflats")
			flats_[0] = a;
		if (entry->name() == "endflats")
			flats_[1] = a;
		if (entry->name() == "startsprites")
			sprites_[0] = a;
		if (entry->name() == "endmonsters")
			sprites_[1] = a;
		if (entry->name() == "startwalls")
			walls_[0] = a;
		if (entry->name() == "endwalls")
			walls_[1] = a;
	}
}

// -----------------------------------------------------------------------------
// Override of Archive::addEntry to force entry addition to the root directory,
// and update namespaces if needed
// -----------------------------------------------------------------------------
shared_ptr<ArchiveEntry> DatArchiveHandler::addEntry(
	Archive&                 archive,
	shared_ptr<ArchiveEntry> entry,
	unsigned                 position,
	ArchiveDir*              dir)
{
	// Do default entry addition (to root directory)
	ArchiveFormatHandler::addEntry(archive, entry, position);

	// Update namespaces if necessary
	if (isNamespaceEntry(entry.get()))
		updateNamespaces(archive);

	return entry;
}

// -----------------------------------------------------------------------------
// Adds [entry] to the end of the namespace matching [add_namespace]. If [copy]
// is true a copy of the entry is added.
// Returns the added entry or NULL if the entry is invalid
// -----------------------------------------------------------------------------
shared_ptr<ArchiveEntry> DatArchiveHandler::addEntry(
	Archive&                 archive,
	shared_ptr<ArchiveEntry> entry,
	string_view              add_namespace)
{
	// Find requested namespace, only three non-global namespaces are valid in this format
	if (strutil::equalCI(add_namespace, "textures"))
	{
		if (walls_[1] >= 0)
			return addEntry(archive, entry, walls_[1], nullptr);
		else
		{
			addNewEntry(archive, "startwalls");
			addNewEntry(archive, "endwalls");
			return addEntry(archive, entry, add_namespace);
		}
	}
	else if (strutil::equalCI(add_namespace, "flats"))
	{
		if (flats_[1] >= 0)
			return addEntry(archive, entry, flats_[1], nullptr);
		else
		{
			addNewEntry(archive, "startflats");
			addNewEntry(archive, "endflats");
			return addEntry(archive, entry, add_namespace);
		}
	}
	else if (strutil::equalCI(add_namespace, "sprites"))
	{
		if (sprites_[1] >= 0)
			return addEntry(archive, entry, sprites_[1], nullptr);
		else
		{
			addNewEntry(archive, "startsprites");
			addNewEntry(archive, "endmonsters");
			return addEntry(archive, entry, add_namespace);
		}
	}
	else
		return addEntry(archive, entry, 0xFFFFFFFF, nullptr);
}

// -----------------------------------------------------------------------------
// Override of Archive::removeEntry to update namespaces if needed
// -----------------------------------------------------------------------------
bool DatArchiveHandler::removeEntry(Archive& archive, ArchiveEntry* entry, bool set_deleted)
{
	// Get entry name (for later)
	auto name = entry->upperName();

	// Do default remove
	if (ArchiveFormatHandler::removeEntry(archive, entry, set_deleted))
	{
		// Update namespaces if necessary
		if (strutil::startsWith(name, "START") || strutil::startsWith(name, "END"))
			updateNamespaces(archive);

		return true;
	}
	else
		return false;
}

// -----------------------------------------------------------------------------
// Override of Archive::renameEntry to update namespaces if needed
// -----------------------------------------------------------------------------
bool DatArchiveHandler::renameEntry(Archive& archive, ArchiveEntry* entry, string_view name, bool force)
{
	// Do default rename
	if (ArchiveFormatHandler::renameEntry(archive, entry, name, force))
	{
		// Update namespaces if necessary
		if (isNamespaceEntry(entry))
			updateNamespaces(archive);

		return true;
	}
	else
		return false;
}

// -----------------------------------------------------------------------------
// Override of Archive::swapEntries to update namespaces if needed
// -----------------------------------------------------------------------------
bool DatArchiveHandler::swapEntries(Archive& archive, ArchiveEntry* entry1, ArchiveEntry* entry2)
{
	// Do default swap (force root dir)
	if (ArchiveFormatHandler::swapEntries(archive, entry1, entry2))
	{
		// Update namespaces if needed
		if (isNamespaceEntry(entry1) || isNamespaceEntry(entry2))
			updateNamespaces(archive);

		return true;
	}
	else
		return false;
}

// -----------------------------------------------------------------------------
// Override of Archive::moveEntry to update namespaces if needed
// -----------------------------------------------------------------------------
bool DatArchiveHandler::moveEntry(Archive& archive, ArchiveEntry* entry, unsigned position, ArchiveDir* dir)
{
	// Do default move (force root dir)
	if (ArchiveFormatHandler::moveEntry(archive, entry, position, nullptr))
	{
		// Update namespaces if necessary
		if (isNamespaceEntry(entry))
			updateNamespaces(archive);

		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Writes the dat archive to a MemChunk.
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool DatArchiveHandler::write(Archive& archive, MemChunk& mc)
{
	// Only two bytes are used for storing entry amount,
	// so abort for excessively large files:
	if (archive.numEntries() > 65535)
		return false;

	// Determine directory offset, name offsets & individual lump offsets
	uint32_t      dir_offset  = 10;
	uint16_t      name_offset = archive.numEntries() * 12;
	uint32_t      name_size   = 0;
	string        previousname;
	auto          nameoffsets = new uint16_t[archive.numEntries()];
	ArchiveEntry* entry;
	for (unsigned l = 0; l < archive.numEntries(); l++)
	{
		entry = archive.entryAt(l);
		entry->setOffsetOnDisk(dir_offset);
		entry->setSizeOnDisk(entry->size());
		dir_offset += entry->size();

		// Does the entry has a name?
		auto name = entry->name();
		if (l > 0 && previousname.length() > 0 && name.length() > previousname.length()
			&& !previousname.compare(0, previousname.length(), name, 0, previousname.length())
			&& name.at(previousname.length()) == '+')
		{
			// This is a fake name
			name           = "";
			nameoffsets[l] = 0;
		}
		else
		{
			// This is a true name
			previousname   = name;
			nameoffsets[l] = static_cast<uint16_t>(name_offset + name_size);
			name_size += name.length() + 1;
		}
	}

	// Clear/init MemChunk
	mc.clear();
	mc.seek(0, SEEK_SET);
	mc.reSize(dir_offset + name_size + archive.numEntries() * 12);

	// Write the header
	uint16_t num_lumps = wxINT16_SWAP_ON_BE(archive.numEntries());
	dir_offset         = wxINT32_SWAP_ON_BE(dir_offset);
	uint32_t unknown   = 0;
	mc.write(&num_lumps, 2);
	mc.write(&dir_offset, 4);
	mc.write(&unknown, 4);

	// Write the lumps
	for (uint16_t l = 0; l < archive.numEntries(); l++)
	{
		entry = archive.entryAt(l);
		mc.write(entry->rawData(), entry->size());
	}

	// Write the directory
	for (uint16_t l = 0; l < num_lumps; l++)
	{
		entry = archive.entryAt(l);

		uint32_t offset  = wxINT32_SWAP_ON_BE(entry->offsetOnDisk());
		uint32_t size    = wxINT32_SWAP_ON_BE(entry->size());
		uint16_t nameofs = wxINT16_SWAP_ON_BE(nameoffsets[l]);
		uint16_t flags   = wxINT16_SWAP_ON_BE((entry->encryption() == EntryEncryption::SCRLE0) ? 1 : 0);

		mc.write(&offset, 4);  // Offset
		mc.write(&size, 4);    // Size
		mc.write(&nameofs, 2); // Name offset
		mc.write(&flags, 2);   // Flags

		entry->setState(EntryState::Unmodified);
	}

	// Write the names
	for (uint16_t l = 0; l < num_lumps; l++)
	{
		uint8_t zero = 0;
		entry        = archive.entryAt(l);
		if (nameoffsets[l])
		{
			mc.write(entry->name().data(), entry->name().length());
			mc.write(&zero, 1);
		}
	}

	// Clean-up
	delete[] nameoffsets;

	// Finished!
	return true;
}


// -----------------------------------------------------------------------------
//
// DatArchiveHandler Class Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Checks if the given data is a valid Shadowcaster dat archive
// -----------------------------------------------------------------------------
bool DatArchiveHandler::isThisFormat(const MemChunk& mc)
{
	// Read dat header
	mc.seek(0, SEEK_SET);
	uint16_t num_lumps;
	uint32_t dir_offset, junk;
	mc.read(&num_lumps, 2);  // Size
	mc.read(&dir_offset, 4); // Directory offset
	mc.read(&junk, 4);       // Unknown value
	num_lumps  = wxINT16_SWAP_ON_BE(num_lumps);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);
	junk       = wxINT32_SWAP_ON_BE(junk);

	if (dir_offset >= mc.size())
		return false;

	// Read the directory
	mc.seek(dir_offset, SEEK_SET);
	// Read lump info
	uint32_t offset  = 0;
	uint32_t size    = 0;
	uint16_t nameofs = 0;
	uint16_t flags   = 0;

	mc.read(&offset, 4);  // Offset
	mc.read(&size, 4);    // Size
	mc.read(&nameofs, 2); // Name offset
	mc.read(&flags, 2);   // Flags

	// Byteswap values for big endian if needed
	offset  = wxINT32_SWAP_ON_BE(offset);
	size    = wxINT32_SWAP_ON_BE(size);
	nameofs = wxINT16_SWAP_ON_BE(nameofs);
	flags   = wxINT16_SWAP_ON_BE(flags);

	// The first lump should have a name (subsequent lumps need not have one).
	// Also, sanity check the values.
	if (nameofs == 0 || nameofs >= mc.size() || offset + size >= mc.size())
	{
		return false;
	}

	size_t len   = 1;
	size_t start = nameofs + dir_offset;
	// Sanity checks again. Make sure there is actually a name.
	if (start > mc.size() || mc[start] < 33)
		return false;
	for (size_t i = start; (mc[i] != 0 && i < mc.size()); ++i, ++len)
	{
		// Names should not contain garbage characters
		if (mc[i] < 32 || mc[i] > 126)
			return false;
	}
	// Let's be reasonable here. While names aren't limited, if it's too long, it's suspicious.
	if (len > 60)
		return false;
	return true;
}

// -----------------------------------------------------------------------------
// Checks if the file at [filename] is a valid Shadowcaster dat archive
// -----------------------------------------------------------------------------
bool DatArchiveHandler::isThisFormat(const string& filename)
{
	// Open file for reading
	wxFile file(filename);

	// Check it opened ok
	if (!file.IsOpened())
		return false;

	// Read dat header
	file.Seek(0, wxFromStart);
	uint16_t num_lumps;
	uint32_t dir_offset, junk;
	file.Read(&num_lumps, 2);  // Size
	file.Read(&dir_offset, 4); // Directory offset
	file.Read(&junk, 4);       // Unknown value
	num_lumps  = wxINT16_SWAP_ON_BE(num_lumps);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);
	junk       = wxINT32_SWAP_ON_BE(junk);

	if (dir_offset >= file.Length())
		return false;

	// Read the directory
	file.Seek(dir_offset, wxFromStart);
	// Read lump info
	uint32_t offset  = 0;
	uint32_t size    = 0;
	uint16_t nameofs = 0;
	uint16_t flags   = 0;

	file.Read(&offset, 4);  // Offset
	file.Read(&size, 4);    // Size
	file.Read(&nameofs, 2); // Name offset
	file.Read(&flags, 2);   // Flags

	// Byteswap values for big endian if needed
	offset  = wxINT32_SWAP_ON_BE(offset);
	size    = wxINT32_SWAP_ON_BE(size);
	nameofs = wxINT16_SWAP_ON_BE(nameofs);
	flags   = wxINT16_SWAP_ON_BE(flags);

	// The first lump should have a name (subsequent lumps need not have one).
	// Also, sanity check the values.
	if (nameofs == 0 || nameofs >= file.Length() || offset + size >= file.Length())
	{
		return false;
	}

	size_t len   = 1;
	size_t start = nameofs + dir_offset;
	// Sanity checks again. Make sure there is actually a name.
	if (start > file.Length())
		return false;
	uint8_t temp;
	file.Seek(start, wxFromStart);
	file.Read(&temp, 1);
	if (temp < 33)
		return false;
	for (size_t i = start; i < file.Length(); ++i, ++len)
	{
		file.Read(&temp, 1);
		// Found end of name
		if (temp == 0)
			break;
		// Names should not contain garbage characters
		if (temp < 32 || temp > 126)
			return false;
	}
	// Let's be reasonable here. While names aren't limited, if it's too long, it's suspicious.
	if (len > 60)
		return false;
	return true;
}
