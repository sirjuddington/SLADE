
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    GZipArchive.cpp
 * Description: GZipArchive, archive class to handle GZip files
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
#include "GZipArchive.h"
#include "UI/SplashWindow.h"
#include "Utility/Compression.h"
#include "General/Misc.h"


/*******************************************************************
 * CONSTANTS
 *******************************************************************/
#define GZIP_ID1 0x1F
#define GZIP_ID2 0x8B
#define GZIP_DEFLATE 0x08
#define GZIP_FLG_FTEXT 0x01
#define GZIP_FLG_FHCRC 0x02
#define GZIP_FLG_FXTRA 0x04
#define GZIP_FLG_FNAME 0x08
#define GZIP_FLG_FCMNT 0x10
#define GZIP_FLG_FUNKN 0xE0

/*******************************************************************
 * GZIPARCHIVE CLASS FUNCTIONS
 *******************************************************************/

/* GZipArchive::GZipArchive
 * GZipArchive class constructor
 *******************************************************************/
GZipArchive::GZipArchive() : TreelessArchive(ARCHIVE_GZIP)
{
	desc.names_extensions = true;
	desc.supports_dirs = true;
}

/* GZipArchive::~GZipArchive
 * GZipArchive class destructor
 *******************************************************************/
GZipArchive::~GZipArchive()
{
}

/* GZipArchive::getFileExtensionString
 * Gets the wxWidgets file dialog filter string for the archive type
 *******************************************************************/
string GZipArchive::getFileExtensionString()
{
	return "GZip Files (*.gz)|*.gz";
}

/* GZipArchive::getFormat
 * Returns the EntryDataFormat id of this archive type
 *******************************************************************/
string GZipArchive::getFormat()
{
	return "archive_gz";
}

/* GZipArchive::open
 * Reads gzip format data from a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool GZipArchive::open(MemChunk& mc)
{
	// Minimal metadata size is 18: 10 for header, 8 for footer
	size_t mds = 18;
	size_t size = mc.getSize();
	if (mds > size)
		return false;

	// Read header
	uint8_t header[4];
	mc.read(header, 4);

	// Check for GZip header; we'll only accept deflated gzip files
	// and reject any field using unknown flags
	if ((!(header[0] == GZIP_ID1 && header[1] == GZIP_ID2 && header[2] == GZIP_DEFLATE))
	        || (header[3] & GZIP_FLG_FUNKN))
		return false;

	bool ftext, fhcrc, fxtra, fname, fcmnt;
	ftext = (header[3] & GZIP_FLG_FTEXT) ? true : false;
	fhcrc = (header[3] & GZIP_FLG_FHCRC) ? true : false;
	fxtra = (header[3] & GZIP_FLG_FXTRA) ? true : false;
	fname = (header[3] & GZIP_FLG_FNAME) ? true : false;
	fcmnt = (header[3] & GZIP_FLG_FCMNT) ? true : false;
	flags = header[3];

	mc.read(&mtime, 4);
	mtime = wxUINT32_SWAP_ON_BE(mtime);

	mc.read(&xfl, 1);
	mc.read(&os, 1);

	// Skip extra fields which may be there
	if (fxtra)
	{
		uint16_t xlen;
		mc.read(&xlen, 2);
		xlen = wxUINT16_SWAP_ON_BE(xlen);
		mds += xlen + 2;
		if (mds > size)
			return false;
		mc.exportMemChunk(xtra, mc.currentPos(), xlen);
		mc.seek(xlen, SEEK_CUR);
	}

	// Skip past name, if any
	string name;
	if (fname)
	{
		char c;
		do
		{
			mc.read(&c, 1);
			if (c) name += c;
			++mds;
		}
		while (c != 0 && size > mds);
	}
	else
	{
		// Build name from filename
		name = getFilename(false);
		wxFileName fn(name);
		if (!fn.GetExt().CmpNoCase("tgz"))
			fn.SetExt("tar");
		else if (!fn.GetExt().CmpNoCase("gz"))
			fn.ClearExt();
		name = fn.GetFullName();
	}

	// Skip past comment
	if (fcmnt)
	{
		char c;
		do
		{
			mc.read(&c, 1);
			if (c) comment += c;
			++mds;
		}
		while (c != 0 && size > mds);
		LOG_MESSAGE(1, "Archive %s says:\n %s", getFilename(true), comment);
	}

	// Skip past CRC 16 check
	if (fhcrc)
	{
		uint8_t* crcbuffer = new uint8_t[mc.currentPos()];
		memcpy(crcbuffer, mc.getData(), mc.currentPos());
		uint32_t fullcrc = Misc::crc(crcbuffer, mc.currentPos());
		delete[] crcbuffer;
		uint16_t hcrc;
		mc.read(&hcrc, 2);
		hcrc = wxUINT16_SWAP_ON_BE(hcrc);
		mds += 2;
		if (hcrc  != (fullcrc & 0x0000FFFF))
		{
			LOG_MESSAGE(1, "CRC-16 mismatch for GZip header");
		}
	}

	// Header is over
	if (mds > size || mc.currentPos() + 8 > size)
		return false;

	// Let's create the entry
	setMuted(true);
	ArchiveEntry* entry = new ArchiveEntry(name, size - mds);
	MemChunk  xdata;
	if (Compression::GZipInflate(mc, xdata))
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

/* GZipArchive::write
 * Writes the gzip archive to a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool GZipArchive::write(MemChunk& mc, bool update)
{
	// Clear current data
	mc.clear();

	if (numEntries() == 1)
	{
		MemChunk stream;
		if (Compression::GZipDeflate(getEntry(0)->getMCData(), stream, 9))
		{
			const uint8_t* data = stream.getData();
			uint32_t working = 0;
			size_t size = stream.getSize();
			if (size < 18) return false;

			// zlib will have given us a minimal header, so we make our own
			uint8_t header[4];
			header[0] = GZIP_ID1; header[1] = GZIP_ID2;
			header[2] = GZIP_DEFLATE; header[3] = flags;
			mc.write(header, 4);

			// Update mtime if the file was modified
			if (getEntry(0)->getState())
			{
				mtime = ::wxGetLocalTime();
			}

			// Write mtime
			working = wxUINT32_SWAP_ON_BE(mtime);
			mc.write(&working, 4);

			// Write other stuff
			mc.write(&xfl, 1);
			mc.write(&os, 1);

			// Any extra content that may have been there
			if (flags & GZIP_FLG_FXTRA)
			{
				uint16_t xlen = wxUINT16_SWAP_ON_BE(xtra.getSize());
				mc.write(&xlen, 2);
				mc.write(xtra.getData(), xtra.getSize());
			}

			// File name, if not extrapolated from archive name
			if (flags & GZIP_FLG_FNAME)
			{
				mc.write(CHR(getEntry(0)->getName()), getEntry(0)->getName().length());
				uint8_t zero = 0; mc.write(&zero, 1);	// Terminate string
			}

			// Comment, if there were actually one
			if (flags & GZIP_FLG_FCMNT)
			{
				mc.write(CHR(comment), comment.length());
				uint8_t zero = 0; mc.write(&zero, 1);	// Terminate string
			}

			// And finally, the half CRC, which we recalculate
			if (flags & GZIP_FLG_FHCRC)
			{
				uint32_t fullcrc = Misc::crc(mc.getData(), mc.getSize());
				uint16_t hcrc = (fullcrc & 0x0000FFFF);
				hcrc = wxUINT16_SWAP_ON_BE(hcrc);
				mc.write(&hcrc, 2);
			}

			// Now that the pleasantries are dispensed with,
			// let's get with the meat of the matter
			return mc.write(data + 10, size - 10);
		}
	}
	return false;
}


/* GZipArchive::renameEntry
 * Renames the entry and set the fname flag
 *******************************************************************/
bool GZipArchive::renameEntry(ArchiveEntry* entry, string name)
{
	// Check entry
	if (!checkEntry(entry))
		return false;

	// Do default rename
	bool ok = Archive::renameEntry(entry, name);
	if (ok) flags |= GZIP_FLG_FNAME;
	return ok;
}

/* GZipArchive::loadEntryData
 * Loads an entry's data from the gzip file
 * Returns true if successful, false otherwise
 *******************************************************************/
bool GZipArchive::loadEntryData(ArchiveEntry* entry)
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

	// Open gzip file
	wxFile file(filename);

	// Check if opening the file failed
	if (!file.IsOpened())
	{
		LOG_MESSAGE(1, "GZipArchive::loadEntryData: Failed to open gzip file %s", filename);
		return false;
	}

	// Seek to lump offset in file and read it in
	entry->importFileStream(file, entry->getSize());

	// Set the lump to loaded
	entry->setLoaded();
	entry->setState(0);

	return true;
}

/* GZipArchive::findFirst
 * Returns the entry if it matches the search criteria in [options],
 * or NULL otherwise
 *******************************************************************/
ArchiveEntry* GZipArchive::findFirst(search_options_t& options)
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

/* GZipArchive::findLast
 * Returns the last entry matching the search criteria in [options],
 * or NULL if no matching entry was found
 *******************************************************************/
ArchiveEntry* GZipArchive::findLast(search_options_t& options)
{
	return findFirst(options);
}

/* GZipArchive::findAll
 * Returns all entries matching the search criteria in [options]
 *******************************************************************/
vector<ArchiveEntry*> GZipArchive::findAll(search_options_t& options)
{
	// Init search variables
	options.match_name = options.match_name.Lower();
	vector<ArchiveEntry*> ret;
	if (findFirst(options))
		ret.push_back(getEntry(0));
	return ret;
}



/* GZipArchive::isGZipArchive
 * Checks if the given data is a valid GZip archive
 *******************************************************************/
bool GZipArchive::isGZipArchive(MemChunk& mc)
{
	// Minimal metadata size is 18: 10 for header, 8 for footer
	size_t mds = 18;
	size_t size = mc.getSize();
	if (size < mds)
		return false;

	// Read header
	uint8_t header[4];
	mc.read(header, 4);

	// Check for GZip header; we'll only accept deflated gzip files
	// and reject any field using unknown flags
	if (!(header[0] == GZIP_ID1 && header[1] == GZIP_ID2 && header[2] == GZIP_DEFLATE)
	        || (header[3] & GZIP_FLG_FUNKN))
		return false;

	bool ftext, fhcrc, fxtra, fname, fcmnt;
	ftext = (header[3] & GZIP_FLG_FTEXT) ? true : false;
	fhcrc = (header[3] & GZIP_FLG_FHCRC) ? true : false;
	fxtra = (header[3] & GZIP_FLG_FXTRA) ? true : false;
	fname = (header[3] & GZIP_FLG_FNAME) ? true : false;
	fcmnt = (header[3] & GZIP_FLG_FCMNT) ? true : false;

	uint32_t mtime;
	mc.read(&mtime, 4);

	uint8_t xfl;
	mc.read(&xfl, 1);
	uint8_t os;
	mc.read(&os, 1);

	// Skip extra fields which may be there
	if (fxtra)
	{
		uint16_t xlen;
		mc.read(&xlen, 2);
		xlen = wxUINT16_SWAP_ON_BE(xlen);
		mds += xlen + 2;
		if (mds > size)
			return false;
		mc.seek(xlen, SEEK_CUR);
	}

	// Skip past name, if any
	if (fname)
	{
		string name;
		char c;
		do
		{
			mc.read(&c, 1);
			if (c) name += c;
			++mds;
		}
		while (c != 0 && size > mds);
	}

	// Skip past comment
	if (fcmnt)
	{
		string comment;
		char c;
		do
		{
			mc.read(&c, 1);
			if (c) comment += c;
			++mds;
		}
		while (c != 0 && size > mds);
	}

	// Skip past CRC 16 check
	if (fhcrc)
	{
		uint16_t hcrc;
		mc.read(&hcrc, 2);
		mds += 2;
	}

	// Header is over
	if (mds > size || mc.currentPos() + 8 > size)
		return false;

	// If it's passed to here it's probably a gzip file
	return true;
}

/* GZipArchive::isGZipArchive
 * Checks if the file at [filename] is a valid GZip archive
 *******************************************************************/
bool GZipArchive::isGZipArchive(string filename)
{
	// Open file for reading
	wxFile file(filename);

	// Minimal metadata size is 18: 10 for header, 8 for footer
	size_t mds = 18;

	// Check it opened ok
	if (!file.IsOpened() || file.Length() < mds)
	{
		return false;
	}

	size_t size = file.Length();
	// Read header
	uint8_t header[4];
	file.Read(header, 4);
	bool ftext, fhcrc, fxtra, fname, fcmnt;
	ftext = (header[3] & GZIP_FLG_FTEXT) ? true : false;
	fhcrc = (header[3] & GZIP_FLG_FHCRC) ? true : false;
	fxtra = (header[3] & GZIP_FLG_FXTRA) ? true : false;
	fname = (header[3] & GZIP_FLG_FNAME) ? true : false;
	fcmnt = (header[3] & GZIP_FLG_FCMNT) ? true : false;

	// Check for GZip header; we'll only accept deflated gzip files
	// and reject any field using unknown flags
	if ((!(header[0] == GZIP_ID1 && header[1] == GZIP_ID2 && header[2] == GZIP_DEFLATE))
	        || (header[3] & GZIP_FLG_FUNKN))
	{
		return false;
	}

	uint32_t mtime;
	file.Read(&mtime, 4);

	uint8_t xfl;
	file.Read(&xfl, 1);
	uint8_t os;
	file.Read(&os, 1);

	// Skip extra fields which may be there
	if (fxtra)
	{
		uint16_t xlen;
		file.Read(&xlen, 2);
		xlen = wxUINT16_SWAP_ON_BE(xlen);
		mds += xlen + 2;
		if (mds > size)
			return false;
		file.Seek(xlen, wxFromCurrent);
	}

	// Skip past name
	if (fname)
	{
		string name;
		char c;
		do
		{
			file.Read(&c, 1);
			if (c) name += c;
			++mds;
		}
		while (c != 0 && size > mds);
	}

	// Skip past comment
	if (fcmnt)
	{
		string comment;
		char c;
		do
		{
			file.Read(&c, 1);
			if (c) comment += c;
			++mds;
		}
		while (c != 0 && size > mds);
	}

	// Skip past CRC 16 check
	if (fhcrc)
	{
		uint16_t hcrc;
		file.Read(&hcrc, 2);
		mds += 2;
	}

	// Header is over
	if (mds > size)
		return false;

	// If it's passed to here it's probably a gzip file
	return true;
}
