
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    BZip2Archive.cpp
// Description: BZip2Archive, archive class to handle BZip2 files
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
#include "BZip2Archive.h"
#include "Utility/Compression.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// BZip2Archive Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Reads bzip2 format data from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool BZip2Archive::open(const MemChunk& mc, bool detect_types)
{
	size_t size = mc.size();
	if (size < 14)
		return false;

	// Read header
	uint8_t header[4];
	mc.read(header, 4);

	// Check for BZip2 header (reject BZip1 headers)
	if (!(header[0] == 'B' && header[1] == 'Z' && header[2] == 'h' && (header[3] >= '1' && header[3] <= '9')))
		return false;

	// Build name from filename
	strutil::Path fn(filename(false));
	auto          ext = fn.extension();
	if (strutil::equalCI(ext, "tbz") || strutil::equalCI(ext, "tb2") || strutil::equalCI(ext, "tbz2"))
		fn.setExtension("tar");
	else if (strutil::equalCI(ext, "bz2"))
		fn.setExtension({});

	// Let's create the entry
	ArchiveModSignalBlocker sig_blocker{ *this };
	auto                    entry = std::make_shared<ArchiveEntry>(fn.fileName(), size);
	MemChunk                xdata;
	if (compression::bzip2Decompress(mc, xdata))
		entry->importMemChunk(xdata);
	else
		return false;
	rootDir()->addEntry(entry);
	EntryType::detectEntryType(*entry);
	entry->setState(ArchiveEntry::State::Unmodified);

	sig_blocker.unblock();
	setModified(false);

	// Finish
	return true;
}

// -----------------------------------------------------------------------------
// Writes the BZip2 archive to a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool BZip2Archive::write(MemChunk& mc)
{
	if (numEntries() == 1)
		return compression::bzip2Compress(entryAt(0)->data(), mc);

	return false;
}

// -----------------------------------------------------------------------------
// Loads an entry's data from the BZip2 file
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool BZip2Archive::loadEntryData(const ArchiveEntry* entry, MemChunk& out)
{
	return false;

#if 0
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

	// Open archive file
	wxFile file(filename_);

	// Check if opening the file failed
	if (!file.IsOpened())
	{
		log::error("BZip2Archive::loadEntryData: Failed to open gzip file {}", filename_);
		return false;
	}

	// Seek to lump offset in file and read it in
	entry->importFileStream(file, entry->size());

	// Set the lump to loaded
	entry->setLoaded();
	entry->setState(ArchiveEntry::State::Unmodified);

	return true;
#endif
}

// -----------------------------------------------------------------------------
// Returns the entry if it matches the search criteria in [options],
// or null otherwise
// -----------------------------------------------------------------------------
ArchiveEntry* BZip2Archive::findFirst(SearchOptions& options)
{
	// Init search variables
	strutil::upperIP(options.match_name);
	auto entry = entryAt(0);
	if (entry == nullptr)
		return entry;

	// Check type
	if (options.match_type)
	{
		if (entry->type() == EntryType::unknownType())
		{
			if (!options.match_type->isThisType(*entry))
			{
				return nullptr;
			}
		}
		else if (options.match_type != entry->type())
		{
			return nullptr;
		}
	}

	// Check name
	if (!options.match_name.empty())
	{
		if (!strutil::matches(entry->upperName(), options.match_name))
		{
			return nullptr;
		}
	}

	// Entry passed all checks so far, so we found a match
	return entry;
}

// -----------------------------------------------------------------------------
// Same as findFirst since there's just one entry
// -----------------------------------------------------------------------------
ArchiveEntry* BZip2Archive::findLast(SearchOptions& options)
{
	return findFirst(options);
}

// -----------------------------------------------------------------------------
// Returns all entries matching the search criteria in [options]
// -----------------------------------------------------------------------------
vector<ArchiveEntry*> BZip2Archive::findAll(SearchOptions& options)
{
	// Init search variables
	vector<ArchiveEntry*> ret;
	if (findFirst(options))
		ret.push_back(entryAt(0));
	return ret;
}


// -----------------------------------------------------------------------------
//
// BZip2Archive Class Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Checks if the given data is a valid BZip2 archive
// -----------------------------------------------------------------------------
bool BZip2Archive::isBZip2Archive(const MemChunk& mc)
{
	size_t size = mc.size();
	if (size < 14)
		return false;

	// Read header
	uint8_t header[4];
	mc.read(header, 4);

	// Check for BZip2 header (reject BZip1 headers)
	if (header[0] == 'B' && header[1] == 'Z' && header[2] == 'h' && (header[3] >= '1' && header[3] <= '9'))
		return true;

	return false;
}

// -----------------------------------------------------------------------------
// Checks if the file at [filename] is a valid BZip2 archive
// -----------------------------------------------------------------------------
bool BZip2Archive::isBZip2Archive(const string& filename)
{
	// Open file for reading
	wxFile file(filename);

	// Check it opened ok
	if (!file.IsOpened() || file.Length() < 14)
	{
		return false;
	}

	// Read header
	uint8_t header[4];
	file.Read(header, 4);

	// Check for BZip2 header (reject BZip1 headers)
	if (header[0] == 'B' && header[1] == 'Z' && header[2] == 'h' && (header[3] >= '1' && header[3] <= '9'))
		return true;

	return false;
}
