
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    GZipArchiveHandler.cpp
// Description: ArchiveFormatHandler for GZip files
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
#include "GZipArchiveHandler.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/EntryType/EntryType.h"
#include "General/Misc.h"
#include "Utility/Compression.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// GZipArchiveHandler Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Reads gzip format data from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool GZipArchiveHandler::open(Archive& archive, const MemChunk& mc, bool detect_types)
{
	// Minimal metadata size is 18: 10 for header, 8 for footer
	size_t mds  = 18;
	size_t size = mc.size();
	if (mds > size)
		return false;

	// Read header
	uint8_t header[4];
	mc.read(header, 4);

	// Check for GZip header; we'll only accept deflated gzip files
	// and reject any field using unknown flags
	if ((!(header[0] == ID1 && header[1] == ID2 && header[2] == DEFLATE)) || (header[3] & FLG_FUNKN))
		return false;

	bool ftext = (header[3] & FLG_FTEXT) != 0;
	bool fhcrc = (header[3] & FLG_FHCRC) != 0;
	bool fxtra = (header[3] & FLG_FXTRA) != 0;
	bool fname = (header[3] & FLG_FNAME) != 0;
	bool fcmnt = (header[3] & FLG_FCMNT) != 0;
	flags_     = header[3];

	mc.read(&mtime_, 4);
	mtime_ = wxUINT32_SWAP_ON_BE(mtime_);

	mc.read(&xfl_, 1);
	mc.read(&os_, 1);

	// Skip extra fields which may be there
	if (fxtra)
	{
		uint16_t xlen;
		mc.read(&xlen, 2);
		xlen = wxUINT16_SWAP_ON_BE(xlen);
		mds += xlen + 2;
		if (mds > size)
			return false;
		mc.exportMemChunk(xtra_, mc.currentPos(), xlen);
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
			if (c)
				name += c;
			++mds;
		} while (c != 0 && size > mds);
	}
	else
	{
		// Build name from filename
		name = archive.filename(false);
		strutil::Path fn(name);
		if (strutil::equalCI(fn.extension(), "tgz"))
			fn.setExtension("tar");
		else if (strutil::equalCI(fn.extension(), "gz"))
			fn.setExtension("");
		name = fn.fileName();
	}

	// Skip past comment
	if (fcmnt)
	{
		char c;
		do
		{
			mc.read(&c, 1);
			if (c)
				comment_ += c;
			++mds;
		} while (c != 0 && size > mds);
		log::info("Archive {} says:\n {}", archive.filename(true), comment_);
	}

	// Skip past CRC 16 check
	if (fhcrc)
	{
		uint8_t* crcbuffer = new uint8_t[mc.currentPos()];
		memcpy(crcbuffer, mc.data(), mc.currentPos());
		uint32_t fullcrc = misc::crc(crcbuffer, mc.currentPos());
		delete[] crcbuffer;
		uint16_t hcrc;
		mc.read(&hcrc, 2);
		hcrc = wxUINT16_SWAP_ON_BE(hcrc);
		mds += 2;
		if (hcrc != (fullcrc & 0x0000FFFF))
		{
			log::info(1, "CRC-16 mismatch for GZip header");
		}
	}

	// Header is over
	if (mds > size || mc.currentPos() + 8 > size)
		return false;

	// Let's create the entry
	ArchiveModSignalBlocker sig_blocker{ archive };
	auto                    entry = std::make_shared<ArchiveEntry>(name, size - mds);
	MemChunk                xdata;
	if (compression::gzipInflate(mc, xdata))
		entry->importMemChunk(xdata);
	else
		return false;
	archive.rootDir()->addEntry(entry);
	EntryType::detectEntryType(*entry);
	entry->setState(EntryState::Unmodified);

	sig_blocker.unblock();
	archive.setModified(false);

	// Finish
	return true;
}

// -----------------------------------------------------------------------------
// Writes the gzip archive to a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool GZipArchiveHandler::write(Archive& archive, MemChunk& mc)
{
	// Clear current data
	mc.clear();

	if (archive.numEntries() == 1)
	{
		MemChunk stream;
		if (compression::gzipDeflate(archive.entryAt(0)->data(), stream, 9))
		{
			auto     data = stream.data();
			uint32_t working;
			size_t   size = stream.size();
			if (size < 18)
				return false;

			// zlib will have given us a minimal header, so we make our own
			uint8_t header[4];
			header[0] = ID1;
			header[1] = ID2;
			header[2] = DEFLATE;
			header[3] = flags_;
			mc.write(header, 4);

			// Update mtime if the file was modified
			if (archive.entryAt(0)->state() != EntryState::Unmodified)
			{
				mtime_ = ::wxGetLocalTime();
			}

			// Write mtime
			working = wxUINT32_SWAP_ON_BE(mtime_);
			mc.write(&working, 4);

			// Write other stuff
			mc.write(&xfl_, 1);
			mc.write(&os_, 1);

			// Any extra content that may have been there
			if (flags_ & FLG_FXTRA)
			{
				uint16_t xlen = wxUINT16_SWAP_ON_BE(xtra_.size());
				mc.write(&xlen, 2);
				mc.write(xtra_.data(), xtra_.size());
			}

			// File name, if not extrapolated from archive name
			if (flags_ & FLG_FNAME)
			{
				mc.write(archive.entryAt(0)->name().data(), archive.entryAt(0)->name().size());
				uint8_t zero = 0;
				mc.write(&zero, 1); // Terminate string
			}

			// Comment, if there were actually one
			if (flags_ & FLG_FCMNT)
			{
				mc.write(comment_.data(), comment_.size());
				uint8_t zero = 0;
				mc.write(&zero, 1); // Terminate string
			}

			// And finally, the half CRC, which we recalculate
			if (flags_ & FLG_FHCRC)
			{
				uint32_t fullcrc = misc::crc(mc.data(), mc.size());
				uint16_t hcrc    = (fullcrc & 0x0000FFFF);
				hcrc             = wxUINT16_SWAP_ON_BE(hcrc);
				mc.write(&hcrc, 2);
			}

			// Now that the pleasantries are dispensed with,
			// let's get with the meat of the matter
			return mc.write(data + 10, size - 10);
		}
	}
	return false;
}


// -----------------------------------------------------------------------------
// Renames the entry and set the fname flag
// -----------------------------------------------------------------------------
bool GZipArchiveHandler::renameEntry(Archive& archive, ArchiveEntry* entry, string_view name, bool force)
{
	// Do default rename
	bool ok = ArchiveFormatHandler::renameEntry(archive, entry, name, force);
	if (ok)
		flags_ |= FLG_FNAME;
	return ok;
}

// -----------------------------------------------------------------------------
// Loads an [entry]'s data from the archive file on disk into [out]
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool GZipArchiveHandler::loadEntryData(Archive& archive, const ArchiveEntry* entry, MemChunk& out)
{
	return false;
}

// -----------------------------------------------------------------------------
// Returns the entry if it matches the search criteria in [options], or null
// otherwise
// -----------------------------------------------------------------------------
ArchiveEntry* GZipArchiveHandler::findFirst(const Archive& archive, ArchiveSearchOptions& options)
{
	// Init search variables
	strutil::upperIP(options.match_name);
	auto entry = archive.entryAt(0);
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
// Returns the last entry matching the search criteria in [options], or null if
// no matching entry was found
// -----------------------------------------------------------------------------
ArchiveEntry* GZipArchiveHandler::findLast(const Archive& archive, ArchiveSearchOptions& options)
{
	return findFirst(archive, options);
}

// -----------------------------------------------------------------------------
// Returns all entries matching the search criteria in [options]
// -----------------------------------------------------------------------------
vector<ArchiveEntry*> GZipArchiveHandler::findAll(const Archive& archive, ArchiveSearchOptions& options)
{
	// Init search variables
	vector<ArchiveEntry*> ret;
	if (findFirst(archive, options))
		ret.push_back(archive.entryAt(0));
	return ret;
}


// -----------------------------------------------------------------------------
//
// GZipArchiveHandler Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Checks if the given data is a valid GZip archive
// -----------------------------------------------------------------------------
bool GZipArchiveHandler::isThisFormat(const MemChunk& mc)
{
	// Minimal metadata size is 18: 10 for header, 8 for footer
	size_t mds  = 18;
	size_t size = mc.size();
	if (size < mds)
		return false;

	// Read header
	uint8_t header[4];
	mc.read(header, 4);

	// Check for GZip header; we'll only accept deflated gzip files
	// and reject any field using unknown flags
	if (!(header[0] == ID1 && header[1] == ID2 && header[2] == DEFLATE) || (header[3] & FLG_FUNKN))
		return false;

	bool ftext = (header[3] & FLG_FTEXT) != 0;
	bool fhcrc = (header[3] & FLG_FHCRC) != 0;
	bool fxtra = (header[3] & FLG_FXTRA) != 0;
	bool fname = (header[3] & FLG_FNAME) != 0;
	bool fcmnt = (header[3] & FLG_FCMNT) != 0;

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
		char   c;
		do
		{
			mc.read(&c, 1);
			if (c)
				name += c;
			++mds;
		} while (c != 0 && size > mds);
	}

	// Skip past comment
	if (fcmnt)
	{
		string comment;
		char   c;
		do
		{
			mc.read(&c, 1);
			if (c)
				comment += c;
			++mds;
		} while (c != 0 && size > mds);
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

// -----------------------------------------------------------------------------
// Checks if the file at [filename] is a valid GZip archive
// -----------------------------------------------------------------------------
bool GZipArchiveHandler::isThisFormat(const string& filename)
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
	bool ftext = (header[3] & FLG_FTEXT) != 0;
	bool fhcrc = (header[3] & FLG_FHCRC) != 0;
	bool fxtra = (header[3] & FLG_FXTRA) != 0;
	bool fname = (header[3] & FLG_FNAME) != 0;
	bool fcmnt = (header[3] & FLG_FCMNT) != 0;

	// Check for GZip header; we'll only accept deflated gzip files
	// and reject any field using unknown flags
	if ((!(header[0] == ID1 && header[1] == ID2 && header[2] == DEFLATE)) || (header[3] & FLG_FUNKN))
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
		char   c;
		do
		{
			file.Read(&c, 1);
			if (c)
				name += c;
			++mds;
		} while (c != 0 && size > mds);
	}

	// Skip past comment
	if (fcmnt)
	{
		string comment;
		char   c;
		do
		{
			file.Read(&c, 1);
			if (c)
				comment += c;
			++mds;
		} while (c != 0 && size > mds);
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
