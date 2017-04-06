
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    TarArchive.cpp
 * Description: TarArchive, archive class to handle Unix tape archives
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
#include "TarArchive.h"
#include "General/UI.h"


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, archive_load_data)


/*******************************************************************
 * TAR STUFF
 *******************************************************************/
#pragma pack(1)
struct tar_header
{
	/* byte offset */
	char name[100];			/*   0 */
	char mode[8];			/* 100 */
	char uid[8];			/* 108 */
	char gid[8];			/* 116 */
	char size[12];			/* 124 */
	char mtime[12];			/* 136 */
	char chksum[8];			/* 148 */
	char typeflag;			/* 156 */
	char linkname[100];		/* 157 */
	char magic[5];			/* 257 */
	char version[3];		/* 262 */
	char uname[32];			/* 265 */
	char gname[32];			/* 297 */
	char devmajor[8];		/* 329 */
	char devminor[8];		/* 337 */
	char prefix[155];		/* 345 */
	char padding[12];		/* 500 */
};
#pragma pack()

#define TMAGIC	"ustar"	/* ustar */
#define GMAGIC	"  "	/* two spaces */
enum typeflags
{
	AREGTYPE = 0,	/* regular file */
	REGTYPE  = '0',	/* regular file */
	LNKTYPE  = '1',	/* link */
	SYMTYPE  = '2',	/* reserved */
	CHRTYPE  = '3',	/* character special */
	BLKTYPE  = '4',	/* block special */
	DIRTYPE  = '5',	/* directory */
	FIFOTYPE = '6',	/* FIFO special */
	CONTTYPE = '7',	/* reserved */
};

/* TarSum
 * Returns the value of field from a tar header, where it was
 * written as an octal number in ASCII. Returns -1 if not a number.
 *******************************************************************/
static int TarSum(const char* field, int size)
{
	--size; // We can't use the last byte
	int sum = 0;
	for (int a = 0; a < size; ++a)
	{
		// Check for strictness of octal representation
		if ((field[a] < '0' || field[a] > '7') && field[a] != ' ')
			return -1;
		sum <<= 3;
		// Cheap hack for space padding instead of O-padding
		if (field[a] != ' ') sum += (field[a] - '0');
	}
	return sum;
}

/* TarWriteOctal
 * Writes the ASCII representation of the octal value of the
 * given <sum> in the given <field> using <size> characters
 *******************************************************************/
static bool TarWriteOctal(size_t sum, char* field, int size)
{
	// Check for overflow, which are possible on 8-byte fields
	if (size < 11 && sum >= (unsigned)(1<<(3*size)))
	{
		char errormessage[9] = "WOLFREVO";
		for (int a = 1; a <= (size + 1); ++a)
		{
			// Should write "OVERFLOW" in a 8-byte field and
			// be enough to tell the tar is completely kaput.
			field[size - a] = errormessage[a - 1];
		}
		return false;
	}
	// If the value is within bounds, write it, starting from the end
	field[size - 1] = 0; // The last byte must be null or zero and cannot be used
	for (int a = size - 2; a >= 0; --a)
	{
		int thisoct = sum % 8;			// Get a value between 0 and 7
		field[a] = ('0' + thisoct);		// then write it
		sum >>= 3;						// finally shift the sum.
	}
	return true;
}

/* TarChecksum
 * Computes the checksum of a tar header, both as signed and unsigned
 * bytes, and verifies that one of the two matches the existing value
 *******************************************************************/
static bool TarChecksum(tar_header* header)
{
	// Create our dummy header with three pointers so as to be able
	// to address it either as a tar header, as a block of signed char,
	// or as a block of unsigned char.
	int8_t*   sigblock = new int8_t [512];
	uint8_t* unsblock = (uint8_t*)sigblock;
	tar_header* block = (tar_header*)sigblock;
	memcpy(sigblock, header, 512);
	int32_t checksum = 0;
	// Blank out checksum
	for (int a = 0; a < 7; ++a)
	{
		if ((block->chksum[a] > '7' || block->chksum[a] < '0') && block->chksum[a] != ' ')
		{
			// This is not a checksum
			//delete[] sigblock;
			//return false;
		}
		else
		{
			checksum<<=3;
			if (block->chksum[a] != ' ') checksum+=(block->chksum[a] - '0');
		}
		block->chksum[a] = ' ';
	}
	block->chksum[7] = ' ';
	// Compute the sum
	int32_t sigsum = 0;
	uint32_t unssum = 0;
	for (int a = 0; a < 512; ++a)
	{
		sigsum += sigblock[a];
		unssum += unsblock[a];
	}

	// Cleanup
	delete[] sigblock;

	// Consider the checksum valid if it corresponds
	// to either a signed or an unsigned sum
	return (checksum == sigsum || checksum == unssum);
}

/* TarChecksum
 * Computes and returns the unsigned checksum of a tar header
 *******************************************************************/
static size_t TarMakeChecksum(tar_header* header)
{
	size_t checksum = 0;
	uint8_t* h = (uint8_t*) header;
	for (int a = 0; a < 512; ++a)
		checksum += h[a];
	return checksum;
}

/* TarDefaultHeader
 * Fill a tar_header with default preset values
 *******************************************************************/
static void TarDefaultHeader(tar_header* header)
{
	if (header == NULL)
		return;

	memset(header->name, 0, 100);			// Name: fill with zeroes
	TarWriteOctal(0777, header->mode, 8);	// Mode: free for all
	TarWriteOctal(1, header->uid, 8);		// UID: first non-root user
	TarWriteOctal(1, header->gid, 8);		// GID: first non-root group
	TarWriteOctal(0, header->size, 12);		// File size: 0 for now
	TarWriteOctal(::wxGetLocalTime(), header->mtime, 12);	// mtime: now
	memset(header->chksum, ' ', 8);			// Checksum: filled with spaces
	header->typeflag = 0;					// Typeflag: regular file
	memset(header->linkname, 0, 100);		// Linkname: fill with zeroes
	// Now pretend to be POSIX-compliant so as to be legible
	header->magic[0] = 'u'; header->magic[1] = 's'; header->magic[2] = 't';
	header->magic[3] = 'a'; header->magic[4] = 'r'; header->version[0] = 0;
	header->version[1] = '0'; header->version[2] = '0';
	// Username: slade3, of course
	memset(header->uname, 0, 32);			// First fill with zeroes
	header->uname[0] = 's'; header->uname[1] = 'l'; header->uname[2] = 'a';
	header->uname[3] = 'd'; header->uname[4] = 'e'; header->uname[5] = '3';
	// Usergroup: slade3, of course
	memset(header->gname, 0, 32);			// First fill with zeroes
	header->gname[0] = 's'; header->gname[1] = 'l'; header->gname[2] = 'a';
	header->gname[3] = 'd'; header->gname[4] = 'e'; header->gname[5] = '3';
	memset(header->devmajor, 0, 8);			// Unused field, zero it
	memset(header->devminor, 0, 8);			// Unused field, zero it
	memset(header->prefix, 0, 155);			// Unused field, zero it
	memset(header->padding, 0, 12);			// Unused field, zero it
}


/*******************************************************************
 * TARARCHIVE CLASS FUNCTIONS
 *******************************************************************/

/* TarArchive::TarArchive
 * TarArchive class constructor
 *******************************************************************/
TarArchive::TarArchive() : Archive(ARCHIVE_TAR)
{
}

/* TarArchive::~TarArchive
 * TarArchive class destructor
 *******************************************************************/
TarArchive::~TarArchive()
{
}

/* TarArchive::getFileExtensionString
 * Returns the file extension string to use in the file open dialog
 *******************************************************************/
string TarArchive::getFileExtensionString()
{
	return "Tar Files (*.tar)|*.tar";
}

/* TarArchive::getFormat
 * Returns the string id for the tar EntryDataFormat
 *******************************************************************/
string TarArchive::getFormat()
{
	return "archive_tar";
}

/* TarArchive::open
 * Reads tar format data from a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool TarArchive::open(MemChunk& mc)
{
	// Check given data is valid
	if (mc.getSize() < 1024)
		return false;

	mc.seek(0, SEEK_SET);

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	setMuted(true);
	UI::setSplashProgressMessage("Reading tar archive data");

	// Two consecutive empty blocks mark the end of the file
	int blankcount = 0;

	// Read all entries in the order they appear
	while ((mc.currentPos() + 512) < mc.getSize() && blankcount < 2)
	{
		// Update splash window progress
		// Since there is no directory in Unix tape archives, use the size
		UI::setSplashProgress(((float)mc.currentPos() / (float)mc.getSize()));

		// Read tar header
		tar_header header;
		mc.read(&header, 512);
		if (wxString::FromAscii(header.magic, 5).CmpNoCase(TMAGIC))
		{
			if (TarMakeChecksum(&header) == 0)
			{
				++blankcount;
			}
			// Invalid block, ignore
			mc.seek(512, SEEK_CUR);
			continue;
		}
		else if (blankcount)
		{
			// Avoid premature end of file
			--blankcount;
		}

		if (!TarChecksum(&header))
		{
			LOG_MESSAGE(1, "Invalid checksum for block at 0x%x", mc.currentPos() - 512);
			continue;
		}

		// Find name
		string name;
		for (int a = 0; a < 100; ++a)
		{
			if (header.name[a] == 0)
				break;
			name += header.name[a];
		}

		// Find size
		size_t size = TarSum(header.size, 12);

		if ((int)header.typeflag == AREGTYPE || (int)header.typeflag == REGTYPE)
		{
			// Normal entry
			wxFileName fn(name);

			// Create directory if needed
			ArchiveTreeNode* dir = createDir(fn.GetPath(true, wxPATH_UNIX));

			// Create entry
			ArchiveEntry* entry = new ArchiveEntry(fn.GetFullName(), size);
			entry->exProp("Offset") = (int)mc.currentPos();
			entry->setLoaded(false);
			entry->setState(0);

			// Add to directory
			dir->addEntry(entry);
		}
		else if ((int)header.typeflag == DIRTYPE)
		{
			// Directory
			ArchiveTreeNode* dir = createDir(name);
		}
		else
		{
			// Something different that we will ignore
		}

		// Move to next header
		size_t sum = size % 512;	// Do we need padding?
		if (sum) sum = 512 - sum;	// Compute it
		sum += size;				// then add it
		mc.seek(sum, SEEK_CUR);		// and move on

	}

	// Detect all entry types
	MemChunk edata;
	vector<ArchiveEntry*> all_entries;
	getEntryTreeAsList(all_entries);
	UI::setSplashProgressMessage("Detecting entry types");
	for (size_t a = 0; a < all_entries.size(); a++)
	{
		// Update splash window progress
		UI::setSplashProgress((((float)a / (float)all_entries.size())));

		// Get entry
		ArchiveEntry* entry = all_entries[a];

		// Read entry data if it isn't zero-sized
		if (entry->getSize() > 0)
		{
			// Read the entry data
			mc.exportMemChunk(edata, (int)entry->exProp("Offset"), entry->getSize());
			entry->importMemChunk(edata);
		}

		// Detect entry type
		EntryType::detectEntryType(entry);

		// Unload entry data if needed
		if (!archive_load_data)
			entry->unloadData();

		// Set entry to unchanged
		entry->setState(0);
	}

	// Setup variables
	setMuted(false);
	setModified(false);
	announce("opened");

	UI::setSplashProgressMessage("");

	return true;
}

/* TarArchive::write
 * Writes the tar archive to a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool TarArchive::write(MemChunk& mc, bool update)
{
	// Clear current data
	mc.clear();

	// We'll use that
	uint8_t padding[512];
	memset(padding, 0, 512);

	// Get archive tree as a list
	vector<ArchiveEntry*> entries;
	getEntryTreeAsList(entries);
	size_t listsize = entries.size();

	for (size_t a = 0; a < listsize; ++a)
	{
		// MAYBE TODO: store the header variables as ExProps for the entries, then only change
		// header.mtime for modified entries, and only use the default values for new entries.
		tar_header header;
		TarDefaultHeader(&header);

		// Write entry name
		string name = entries[a]->getPath(true);
		name.Remove(0, 1);	// Remove leading /
		if (name.Len() > 99)
		{
			LOG_MESSAGE(1, "Warning: Entry %s path is too long (> 99 characters), putting it in the root directory", name);
			wxFileName fn(name);
			name = fn.GetFullName();
			if (name.Len() > 99)
				name.Truncate(99);
		}
		memcpy(header.name, CHR(name), name.Length());

		// Address folders
		if (entries[a]->getType() == EntryType::folderType())
		{
			header.typeflag = DIRTYPE;
			TarWriteOctal(TarMakeChecksum(&header), header.chksum, 7);
			mc.write(&header, 512);

			// Else we've got a file
		}
		else
		{
			header.typeflag = REGTYPE;
			TarWriteOctal(entries[a]->getSize(), header.size, 12);
			TarWriteOctal(TarMakeChecksum(&header), header.chksum, 7);
			size_t padsize = entries[a]->getSize() % 512;
			if (padsize) padsize = 512 - padsize;
			mc.write(&header, 512);
			mc.write(entries[a]->getData(), entries[a]->getSize());
			if (padsize)
				mc.write(padding, padsize);
		}
	}

	// Finished, so write two pages of zeroes and return a success
	mc.write(padding, 512);
	mc.write(padding, 512);
	return true;
}

/* TarArchive::loadEntryData
 * Loads an entry's data from the tar file
 * Returns true if successful, false otherwise
 *******************************************************************/
bool TarArchive::loadEntryData(ArchiveEntry* entry)
{
	// Check entry is ok
	if (!checkEntry(entry))
		return false;

	// Do nothing if the entry's size is zero,
	// or if it has already been loaded
	if (entry->getSize() == 0 || entry->isLoaded())
	{
		entry->setLoaded();
		return true;
	}

	// Open archive file
	wxFile file(filename);

	// Check it opened
	if (!file.IsOpened())
	{
		LOG_MESSAGE(1, "TarArchive::loadEntryData: Unable to open archive file %s", filename);
		return false;
	}

	// Seek to entry offset in file and read it in
	file.Seek((int)entry->exProp("Offset"), wxFromStart);
	entry->importFileStream(file, entry->getSize());

	// Set the lump to loaded
	entry->setLoaded();

	return true;
}

/*******************************************************************
 * TARARCHIVE CLASS STATIC FUNCTIONS
 *******************************************************************/

/* TarArchive::isTarArchive
 * Checks if the given data is a valid Unix tar archive
 *******************************************************************/
bool TarArchive::isTarArchive(MemChunk& mc)
{
	mc.seek(0, SEEK_SET);
	int blankcount = 0;
	while ((mc.currentPos() + 512) <= mc.getSize() && blankcount < 3)
	{
		// Read tar header
		tar_header header;
		mc.read(&header, 512);
		if (string(wxString::From8BitData(header.magic, 5)).CmpNoCase(TMAGIC))
		{
			if (TarMakeChecksum(&header) == 0)
			{
				++blankcount;
			}
			else
			{
				return false;
			}
			// Move to next
			continue;
		}
		else if (blankcount)
		{
			// Avoid premature end of file
			--blankcount;
		}

		if (!TarChecksum(&header))
		{
			return false;
		}

		// Find size and move to next header
		size_t size = TarSum(header.size, 12);
		size_t sum = size % 512;	// Do we need padding?
		if (sum) sum = 512 - sum;	// Compute it
		sum += size;				// then add it
		mc.seek(sum, SEEK_CUR);		// and move on

	}
	// We should end with a blankcount of precisely 2
	return (blankcount == 2);
}

/* TarArchive::isTarArchive
 * Checks if the file at [filename] is a valid Unix tar archive
 *******************************************************************/
bool TarArchive::isTarArchive(string filename)
{
	// Open file for reading
	wxFile file(filename);

	// Check it opened ok
	if (!file.IsOpened() || file.Length() < 512)
		return false;

	int blankcount = 0;
	while ((file.Tell() + 512) <= file.Length() && blankcount < 3)
	{

		// Read tar header
		tar_header header;
		file.Read(&header, 512);
		if (string(wxString::FromAscii(header.magic, 5)).CmpNoCase(TMAGIC))
		{
			if (TarMakeChecksum(&header) == 0)
			{
				++blankcount;
			}
			else
			{
				return false;
			}
			// Move to next
			continue;
		}
		else if (blankcount)
		{
			// Avoid premature end of file
			--blankcount;
		}

		if (!TarChecksum(&header))
		{
			return false;
		}

		// Find size and move to next header
		size_t size = TarSum(header.size, 12);
		size_t sum = size % 512;		// Do we need padding?
		if (sum) sum = 512 - sum;		// Compute it
		sum += size;					// then add it
		file.Seek(sum, wxFromCurrent);	// and move on

	}
	// We should end with a blankcount of precisely 2
	return (blankcount == 2);
}
