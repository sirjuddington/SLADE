
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    DatArchive.cpp
// Description: DatArchive, archive class to handle ravdata.dat
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
#include "DatArchive.h"
#include "General/UI.h"
#include "Utility/StringUtils.h"


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
bool isNamespaceEntry(ArchiveEntry* entry)
{
	return StrUtil::startsWith(entry->upperName(), "START") || StrUtil::startsWith(entry->upperName(), "END");
}
} // namespace


// -----------------------------------------------------------------------------
//
// DatArchive Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Reads wad format data from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool DatArchive::open(MemChunk& mc)
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
	num_lumps             = wxINT16_SWAP_ON_BE(num_lumps);
	dir_offset            = wxINT32_SWAP_ON_BE(dir_offset);
	unknown               = wxINT32_SWAP_ON_BE(unknown);
	std::string lastname  = "-noname-";
	size_t      namecount = 0;

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	setMuted(true);

	// Read the directory
	mc.seek(dir_offset, SEEK_SET);
	UI::setSplashProgressMessage("Reading dat archive data");
	for (uint32_t d = 0; d < num_lumps; d++)
	{
		// Update splash window progress
		UI::setSplashProgress(((float)d / (float)num_lumps));

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
			Log::error(wxString::Format("DatArchive::open: Dat archive is invalid or corrupt at entry %i", d));
			Global::error = "Archive is invalid and/or corrupt";
			setMuted(false);
			return false;
		}

		std::string myname;
		if (nameofs != 0)
		{
			size_t len   = 1;
			size_t start = nameofs + dir_offset;
			for (size_t i = start; mcdata[i] != 0; ++i)
			{
				++len;
			}
			myname.assign((const char*)mcdata + start, len);
			lastname  = myname;
			namecount = 0;
		}
		else
		{
			myname = fmt::format("{}+{}", lastname, ++namecount);
		}

		// Create & setup lump
		auto nlump = std::make_shared<ArchiveEntry>(myname, size);
		nlump->setLoaded(false);
		nlump->exProp("Offset") = (int)offset;
		nlump->setState(ArchiveEntry::State::Unmodified);

		if (flags & 1)
			nlump->setEncryption(ArchiveEntry::Encryption::SCRLE0);

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
		rootDir()->addEntry(nlump);
	}

	// Detect all entry types
	MemChunk edata;
	UI::setSplashProgressMessage("Detecting entry types");
	for (size_t a = 0; a < numEntries(); a++)
	{
		// Update splash window progress
		UI::setSplashProgress((((float)a / (float)num_lumps)));

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
		EntryType::detectEntryType(entry);

		// Set entry to unchanged
		entry->setState(ArchiveEntry::State::Unmodified);
	}

	// Detect maps (will detect map entry types)
	// UI::setSplashProgressMessage("Detecting maps");
	// detectMaps();

	// Setup variables
	setMuted(false);
	setModified(false);
	announce("opened");

	UI::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Returns the namespace that [entry] is within
// -----------------------------------------------------------------------------
wxString DatArchive::detectNamespace(ArchiveEntry* entry)
{
	return detectNamespace(entryIndex(entry));
}

// -----------------------------------------------------------------------------
// Returns the namespace that the entry at [index] in [dir] is within
// -----------------------------------------------------------------------------
wxString DatArchive::detectNamespace(size_t index, ArchiveTreeNode* dir)
{
	// Textures
	if (index > (unsigned)walls_[0] && index < (unsigned)walls_[1])
		return "textures";

	// Flats
	if (index > (unsigned)flats_[0] && index < (unsigned)flats_[1])
		return "flats";

	// Sprites
	if (index > (unsigned)sprites_[0] && index < (unsigned)sprites_[1])
		return "sprites";

	return "global";
}

// -----------------------------------------------------------------------------
// Updates the namespace list
// -----------------------------------------------------------------------------
void DatArchive::updateNamespaces()
{
	// Clear current namespace info
	sprites_[0] = sprites_[1] = flats_[0] = flats_[1] = walls_[0] = walls_[1] = -1;

	// Go through all entries
	for (unsigned a = 0; a < numEntries(); a++)
	{
		auto entry = rootDir()->entryAt(a);

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
ArchiveEntry* DatArchive::addEntry(ArchiveEntry* entry, unsigned position, ArchiveTreeNode* dir, bool copy)
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
// Adds [entry] to the end of the namespace matching [add_namespace]. If [copy]
// is true a copy of the entry is added.
// Returns the added entry or NULL if the entry is invalid
// -----------------------------------------------------------------------------
ArchiveEntry* DatArchive::addEntry(ArchiveEntry* entry, const wxString& add_namespace, bool copy)
{
	// Find requested namespace, only three non-global namespaces are valid in this format
	if (S_CMPNOCASE(add_namespace, "textures"))
	{
		if (walls_[1] >= 0)
			return addEntry(entry, walls_[1], nullptr, copy);
		else
		{
			addNewEntry("startwalls");
			addNewEntry("endwalls");
			return addEntry(entry, add_namespace, copy);
		}
	}
	else if (S_CMPNOCASE(add_namespace, "flats"))
	{
		if (flats_[1] >= 0)
			return addEntry(entry, flats_[1], nullptr, copy);
		else
		{
			addNewEntry("startflats");
			addNewEntry("endflats");
			return addEntry(entry, add_namespace, copy);
		}
	}
	else if (S_CMPNOCASE(add_namespace, "sprites"))
	{
		if (sprites_[1] >= 0)
			return addEntry(entry, sprites_[1], nullptr, copy);
		else
		{
			addNewEntry("startsprites");
			addNewEntry("endmonsters");
			return addEntry(entry, add_namespace, copy);
		}
	}
	else
		return addEntry(entry, 0xFFFFFFFF, nullptr, copy);
}

// -----------------------------------------------------------------------------
// Override of Archive::removeEntry to update namespaces if needed
// -----------------------------------------------------------------------------
bool DatArchive::removeEntry(ArchiveEntry* entry)
{
	// Check entry
	if (!checkEntry(entry))
		return false;

	// Get entry name (for later)
	wxString name = entry->name();

	// Do default remove
	bool ok = Archive::removeEntry(entry);

	if (ok)
	{
		// Update namespaces if necessary
		if (name.Upper().Matches("START*") || name.Upper().Matches("END*"))
			updateNamespaces();

		return true;
	}
	else
		return false;
}

// -----------------------------------------------------------------------------
// Override of Archive::renameEntry to update namespaces if needed
// -----------------------------------------------------------------------------
bool DatArchive::renameEntry(ArchiveEntry* entry, const wxString& name)
{
	// Check entry
	if (!checkEntry(entry))
		return false;

	// Do default rename
	bool ok = Archive::renameEntry(entry, name);

	if (ok)
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
// Override of Archive::swapEntries to update namespaces if needed
// -----------------------------------------------------------------------------
bool DatArchive::swapEntries(ArchiveEntry* entry1, ArchiveEntry* entry2)
{
	// Check entries
	if (!checkEntry(entry1) || !checkEntry(entry2))
		return false;

	// Do default swap (force root dir)
	bool ok = Archive::swapEntries(entry1, entry2);

	if (ok)
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
bool DatArchive::moveEntry(ArchiveEntry* entry, unsigned position, ArchiveTreeNode* dir)
{
	// Check entry
	if (!checkEntry(entry))
		return false;

	// Do default move (force root dir)
	bool ok = Archive::moveEntry(entry, position, nullptr);

	if (ok)
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
// Writes the dat archive to a MemChunk.
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool DatArchive::write(MemChunk& mc, bool update)
{
	// Only two bytes are used for storing entry amount,
	// so abort for excessively large files:
	if (numEntries() > 65535)
		return false;

	// Determine directory offset, name offsets & individual lump offsets
	uint32_t      dir_offset   = 10;
	uint16_t      name_offset  = numEntries() * 12;
	uint32_t      name_size    = 0;
	wxString      previousname = "";
	uint16_t*     nameoffsets  = new uint16_t[numEntries()];
	ArchiveEntry* entry;
	for (uint16_t l = 0; l < numEntries(); l++)
	{
		entry = entryAt(l);
		setEntryOffset(entry, dir_offset);
		dir_offset += entry->size();

		// Does the entry has a name?
		wxString name = entry->name();
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
			nameoffsets[l] = uint16_t(name_offset + name_size);
			name_size += name.length() + 1;
		}
	}

	// Clear/init MemChunk
	mc.clear();
	mc.seek(0, SEEK_SET);
	mc.reSize(dir_offset + name_size + numEntries() * 12);

	// Write the header
	uint16_t num_lumps = wxINT16_SWAP_ON_BE(numEntries());
	dir_offset         = wxINT32_SWAP_ON_BE(dir_offset);
	uint32_t unknown   = 0;
	mc.write(&num_lumps, 2);
	mc.write(&dir_offset, 4);
	mc.write(&unknown, 4);

	// Write the lumps
	for (uint16_t l = 0; l < numEntries(); l++)
	{
		entry = entryAt(l);
		mc.write(entry->rawData(), entry->size());
	}

	// Write the directory
	for (uint16_t l = 0; l < num_lumps; l++)
	{
		entry = entryAt(l);

		uint32_t offset  = wxINT32_SWAP_ON_BE(getEntryOffset(entry));
		uint32_t size    = wxINT32_SWAP_ON_BE(entry->size());
		uint16_t nameofs = wxINT16_SWAP_ON_BE(nameoffsets[l]);
		uint16_t flags   = wxINT16_SWAP_ON_BE((entry->encryption() == ArchiveEntry::Encryption::SCRLE0) ? 1 : 0);

		mc.write(&offset, 4);  // Offset
		mc.write(&size, 4);    // Size
		mc.write(&nameofs, 2); // Name offset
		mc.write(&flags, 2);   // Flags

		if (update)
		{
			entry->setState(ArchiveEntry::State::Unmodified);
			entry->exProp("Offset") = (int)wxINT32_SWAP_ON_BE(offset);
		}
	}

	// Write the names
	for (uint16_t l = 0; l < num_lumps; l++)
	{
		uint8_t zero = 0;
		entry        = entryAt(l);
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
// Loads an entry's data from the datfile
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool DatArchive::loadEntryData(ArchiveEntry* entry)
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
		Log::error(wxString::Format("DatArchive::loadEntryData: Failed to open datfile %s", filename_));
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
//
// DatArchive Class Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Checks if the given data is a valid Shadowcaster dat archive
// -----------------------------------------------------------------------------
bool DatArchive::isDatArchive(MemChunk& mc)
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
bool DatArchive::isDatArchive(const wxString& filename)
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

	wxString name;
	size_t   len   = 1;
	size_t   start = nameofs + dir_offset;
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
