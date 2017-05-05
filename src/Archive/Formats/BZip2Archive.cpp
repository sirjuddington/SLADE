
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    BZip2Archive.cpp
 * Description: BZip2Archive, archive class to handle BZip2 files
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
#include "BZip2Archive.h"
#include "UI/SplashWindow.h"
#include "Utility/Compression.h"
#include "General/Misc.h"


/*******************************************************************
 * BZIP2ARCHIVE CLASS FUNCTIONS
 *******************************************************************/

/* BZip2Archive::BZip2Archive
 * BZip2Archive class constructor
 *******************************************************************/
BZip2Archive::BZip2Archive() : TreelessArchive(ARCHIVE_BZ2)
{
	desc.names_extensions = true;
	desc.supports_dirs = true;
}

/* BZip2Archive::~BZip2Archive
 * BZip2Archive class destructor
 *******************************************************************/
BZip2Archive::~BZip2Archive()
{
}

/* BZip2Archive::getFileExtensionString
 * Gets the wxWidgets file dialog filter string for the archive type
 *******************************************************************/
string BZip2Archive::getFileExtensionString()
{
	return "BZip2 Files (*.bz2)|*.bz2";
}

/* BZip2Archive::getFormat
 * Returns the EntryDataFormat id of this archive type
 *******************************************************************/
string BZip2Archive::getFormat()
{
	return "archive_bz2";
}

/* BZip2Archive::open
 * Reads bzip2 format data from a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool BZip2Archive::open(MemChunk& mc)
{
	size_t size = mc.getSize();
	if (size < 14)
		return false;

	// Read header
	uint8_t header[4];
	mc.read(header, 4);

	// Check for BZip2 header (reject BZip1 headers)
	if (!(header[0] == 'B' && header[1] == 'Z' && header[2] == 'h' && (header[3] >= '1' && header[3] <= '9')))
		return false;

	// Build name from filename
	string name = getFilename(false);
	wxFileName fn(name);
	if (!fn.GetExt().CmpNoCase("tbz") || !fn.GetExt().CmpNoCase("tb2") || !fn.GetExt().CmpNoCase("tbz2"))
		fn.SetExt("tar");
	else if (!fn.GetExt().CmpNoCase("bz2"))
		fn.ClearExt();
	name = fn.GetFullName();

	// Let's create the entry
	setMuted(true);
	ArchiveEntry* entry = new ArchiveEntry(name, size);
	MemChunk xdata;
	if (Compression::BZip2Decompress(mc, xdata))
	{
		entry->importMemChunk(xdata);
	}
	else
	{
		delete entry;
		setMuted(false);
		return false;
	}
	getRoot()->addEntry(entry);
	EntryType::detectEntryType(entry);
	entry->setState(0);

	setMuted(false);
	setModified(false);
	announce("opened");

	// Finish
	return true;
}

/* BZip2Archive::write
 * Writes the BZip2 archive to a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool BZip2Archive::write(MemChunk& mc, bool update)
{
	if (numEntries() == 1)
	{
		return Compression::BZip2Compress(getEntry(0)->getMCData(), mc);
	}
	return false;
}

/* BZip2Archive::loadEntryData
 * Loads an entry's data from the BZip2 file
 * Returns true if successful, false otherwise
 *******************************************************************/
bool BZip2Archive::loadEntryData(ArchiveEntry* entry)
{
	return false;
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

	// Open archive file
	wxFile file(filename);

	// Check if opening the file failed
	if (!file.IsOpened())
	{
		LOG_MESSAGE(1, "BZip2Archive::loadEntryData: Failed to open gzip file %s", filename);
		return false;
	}

	// Seek to lump offset in file and read it in
	entry->importFileStream(file, entry->getSize());

	// Set the lump to loaded
	entry->setLoaded();
	entry->setState(0);

	return true;
}

/* BZip2Archive::findFirst
 * Returns the entry if it matches the search criteria in [options],
 * or NULL otherwise
 *******************************************************************/
ArchiveEntry* BZip2Archive::findFirst(search_options_t& options)
{
	// Init search variables
	options.match_name = options.match_name.Lower();
	ArchiveEntry* entry = getEntry(0);
	if (entry == NULL)
		return entry;

	// Check type
	if (options.match_type)
	{
		if (entry->getType() == EntryType::unknownType())
		{
			if (!options.match_type->isThisType(entry))
			{
				return NULL;
			}
		}
		else if (options.match_type != entry->getType())
		{
			return NULL;
		}
	}

	// Check name
	if (!options.match_name.IsEmpty())
	{
		if (!options.match_name.Matches(entry->getName().Lower()))
		{
			return NULL;
		}
	}

	// Entry passed all checks so far, so we found a match
	return entry;
}

/* BZip2Archive::findLast
 * Same as findFirst since there's just one entry
 *******************************************************************/
ArchiveEntry* BZip2Archive::findLast(search_options_t& options)
{
	return findFirst(options);
}

/* BZip2Archive::findAll
 * Returns all entries matching the search criteria in [options]
 *******************************************************************/
vector<ArchiveEntry*> BZip2Archive::findAll(search_options_t& options)
{
	// Init search variables
	options.match_name = options.match_name.Lower();
	vector<ArchiveEntry*> ret;
	if (findFirst(options))
		ret.push_back(getEntry(0));
	return ret;
}



/* BZip2Archive::isBZip2Archive
 * Checks if the given data is a valid BZip2 archive
 *******************************************************************/
bool BZip2Archive::isBZip2Archive(MemChunk& mc)
{
	size_t size = mc.getSize();
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

/* BZip2Archive::isBZip2Archive
 * Checks if the file at [filename] is a valid BZip2 archive
 *******************************************************************/
bool BZip2Archive::isBZip2Archive(string filename)
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
