
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    HogArchiveHandler.cpp
// Description: ArchiveFormatHandler for HOG archives from Descent 1 and 2
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
#include "HogArchiveHandler.h"
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
// TXB files are text files with a bit shift xor cipher. It makes an exception
// for the newline character probably so that standard string functions will
// continue to work. As an extension we also except the encoded version of 0xA
// in order to produce a lossless conversion. This allows us to semi-effectively
// handle this at the archive level instead of as a filter at the text editor.
// -----------------------------------------------------------------------------
void decodeTxb(MemChunk& mc)
{
	const uint8_t*       data    = mc.data();
	const uint8_t* const dataend = data + mc.size();
	uint8_t*             odata   = new uint8_t[mc.size()];
	uint8_t* const       ostart  = odata;
	while (data != dataend)
	{
		if (*data != 0xA && *data != 0x8F)
		{
			*odata++ = (((*data & 0x3F) << 2) | ((*data & 0xC0) >> 6)) ^ 0xA7;
			++data;
		}
		else
			*odata++ = *data++;
	}
	mc.importMem(ostart, mc.size());
	delete[] ostart;
}

// -----------------------------------------------------------------------------
// Opposite of DecodeTXB. Caller should delete[] returned pointer.
// -----------------------------------------------------------------------------
uint8_t* encodeTxb(const MemChunk& mc)
{
	const uint8_t*       data    = mc.data();
	const uint8_t* const dataend = data + mc.size();
	uint8_t*             odata   = new uint8_t[mc.size()];
	uint8_t* const       ostart  = odata;
	while (data != dataend)
	{
		if (*data != 0xA && *data != 0x8F)
		{
			*odata++ = (((*data & 0x3) << 6) | ((*data & 0xFC) >> 2)) ^ 0xE9;
			++data;
		}
		else
			*odata++ = *data++;
	}
	return ostart;
}

// -----------------------------------------------------------------------------
// Determines by filename being *.txb or *.ctb if we should encode.
// -----------------------------------------------------------------------------
bool shouldEncodeTxb(string_view name)
{
	return strutil::endsWithCI(name, ".txb") || strutil::endsWithCI(name, ".ctb");
}
} // namespace


// -----------------------------------------------------------------------------
//
// HogArchiveHandler Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Reads hog format data from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool HogArchiveHandler::open(Archive& archive, const MemChunk& mc, bool detect_types)
{
	// Check data was given
	if (!mc.hasData())
		return false;

	// Check size
	size_t archive_size = mc.size();
	if (archive_size < 3)
		return false;

	// Check magic header (DHF for "Descent Hog File")
	if (mc[0] != 'D' || mc[1] != 'H' || mc[2] != 'F')
		return false;

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	ArchiveModSignalBlocker sig_blocker{ archive };

	// Iterate through files to see if the size seems okay
	ui::setSplashProgressMessage("Reading hog archive data");
	size_t   iter_offset = 3;
	uint32_t num_lumps   = 0;
	MemChunk edata;
	while (iter_offset < archive_size)
	{
		// Update splash window progress
		ui::setSplashProgress(iter_offset, archive_size);

		// If the lump data goes past the end of the file,
		// the hogfile is invalid
		if (iter_offset + 17 > archive_size)
		{
			log::error("HogArchiveHandler::open: hog archive is invalid or corrupt");
			global::error = "Archive is invalid and/or corrupt";
			return false;
		}

		// Setup variables
		num_lumps++;
		size_t offset   = iter_offset + 17;
		size_t size     = mc.readL32(iter_offset + 13);
		char   name[14] = "";
		mc.seek(iter_offset, SEEK_SET);
		mc.read(name, 13);
		name[13] = 0;

		// Create & setup lump
		auto nlump = std::make_shared<ArchiveEntry>(name, size);
		nlump->setOffsetOnDisk(offset);
		nlump->setSizeOnDisk(size);

		// Handle txb/ctb as archive level encryption. This is not strictly
		// correct, but since we're not making a proper Descent editor this
		// prevents needless complication on loading text data.
		if (shouldEncodeTxb(nlump->name()))
			nlump->setEncryption(EntryEncryption::TXB);

		// Read entry data if it isn't zero-sized
		if (nlump->size() > 0)
		{
			// Read the entry data
			mc.exportMemChunk(edata, offset, size);

			// Decode if needed
			if (nlump->encryption() == EntryEncryption::TXB)
				decodeTxb(edata);

			nlump->importMemChunk(edata);
		}

		nlump->setState(EntryState::Unmodified);

		// Add to entry list
		archive.rootDir()->addEntry(nlump);

		// Update entry size to compute next offset
		iter_offset = offset + size;
	}

	// Detect all entry types
	if (detect_types)
		archive.detectAllEntryTypes();

	// Setup variables
	sig_blocker.unblock();
	archive.setModified(false);

	ui::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Writes the hog archive to a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool HogArchiveHandler::write(Archive& archive, MemChunk& mc)
{
	// Determine individual lump offsets
	uint32_t      offset = 3;
	ArchiveEntry* entry;
	for (uint32_t l = 0; l < archive.numEntries(); l++)
	{
		offset += 17;
		entry = archive.entryAt(l);
		entry->setState(EntryState::Unmodified);
		entry->setOffsetOnDisk(offset);
		entry->setSizeOnDisk();
		offset += entry->size();
	}

	// Clear/init MemChunk
	mc.clear();
	mc.seek(0, SEEK_SET);
	mc.reSize(offset);

	// Write the header
	char header[3] = { 'D', 'H', 'F' };
	mc.write(header, 3);

	// Write the directory
	for (uint32_t l = 0; l < archive.numEntries(); l++)
	{
		entry         = archive.entryAt(l);
		char name[13] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		long size     = wxINT32_SWAP_ON_BE(entry->size());

		for (size_t c = 0; c < entry->name().length() && c < 13; c++)
			name[c] = entry->name()[c];

		mc.write(name, 13);
		mc.write(&size, 4);
		if (entry->encryption() == EntryEncryption::TXB)
		{
			uint8_t* data = encodeTxb(entry->data());
			mc.write(data, entry->size());
			delete[] data;
		}
		else
			mc.write(entry->rawData(), entry->size());
	}

	return true;
}

// -----------------------------------------------------------------------------
// Override of Archive::addEntry to force entry addition to the root directory
// and set encryption for the entry
// -----------------------------------------------------------------------------
shared_ptr<ArchiveEntry> HogArchiveHandler::addEntry(
	Archive&                 archive,
	shared_ptr<ArchiveEntry> entry,
	unsigned                 position,
	ArchiveDir*              dir)
{
	if (shouldEncodeTxb(entry->name()))
		entry->setEncryption(EntryEncryption::TXB);

	// Do default entry addition (to root directory)
	return ArchiveFormatHandler::addEntry(archive, entry, position);
}

// -----------------------------------------------------------------------------
// Since hog files have no namespaces, just call the other function.
// -----------------------------------------------------------------------------
shared_ptr<ArchiveEntry> HogArchiveHandler::addEntry(
	Archive&                 archive,
	shared_ptr<ArchiveEntry> entry,
	string_view              add_namespace)
{
	return addEntry(archive, entry, 0xFFFFFFFF, nullptr);
}

// -----------------------------------------------------------------------------
// Override of Archive::renameEntry to update entry encryption info
// -----------------------------------------------------------------------------
bool HogArchiveHandler::renameEntry(Archive& archive, ArchiveEntry* entry, string_view name, bool force)
{
	// Do default rename
	if (ArchiveFormatHandler::renameEntry(archive, entry, name, force))
	{
		// Update encode status
		if (shouldEncodeTxb(entry->name()))
			entry->setEncryption(EntryEncryption::TXB);
		else
			entry->setEncryption(EntryEncryption::None);

		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Checks if the given data is a valid Descent hog archive
// -----------------------------------------------------------------------------
bool HogArchiveHandler::isThisFormat(const MemChunk& mc)
{
	// Check size
	size_t size = mc.size();
	if (size < 3)
		return false;

	// Check magic header
	if (mc[0] != 'D' || mc[1] != 'H' || mc[2] != 'F')
		return false;

	// Iterate through files to see if the size seems okay
	size_t offset = 3;
	while (offset < size)
	{
		// Enough room for the header?
		if (offset + 17 > size)
			return false;
		// Read entry size to compute next offset
		offset += 17 + mc.readL32(offset + 13);
	}

	// We should end on at exactly the end of the file
	return (offset == size);
}

// -----------------------------------------------------------------------------
// Checks if the file at [filename] is a valid Descent hog archive
// -----------------------------------------------------------------------------
bool HogArchiveHandler::isThisFormat(const string& filename)
{
	// Open file for reading
	wxFile file(filename);

	// Check it opened ok
	if (!file.IsOpened())
		return false;

	// Check size
	size_t size = file.Length();
	if (size < 3)
		return false;

	// Check magic header
	char magic[3] = "";
	file.Seek(0, wxFromStart);
	file.Read(magic, 3);
	if (magic[0] != 'D' || magic[1] != 'H' || magic[2] != 'F')
		return false;

	// Iterate through files to see if the size seems okay
	size_t offset = 3;
	while (offset < size)
	{
		// Enough room for the header?
		if (offset + 17 > size)
			return false;
		// Read entry size to compute next offset
		uint32_t lumpsize;
		file.Seek(offset + 13, wxFromStart);
		file.Read(&lumpsize, 4);
		offset += 17 + wxINT32_SWAP_ON_BE(lumpsize);
	}

	// We should end on at exactly the end of the file
	return (offset == size);
}
