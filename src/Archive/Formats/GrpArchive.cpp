
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    GrpArchive.cpp
 * Description: GrpArchive, archive class to handle GRP archives
 *              like those of Duke Nukem 3D
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
#include "GrpArchive.h"
#include "UI/SplashWindow.h"


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, wad_force_uppercase)
EXTERN_CVAR(Bool, archive_load_data)

/*******************************************************************
 * GRPARCHIVE CLASS FUNCTIONS
 *******************************************************************/

/* GrpArchive::GrpArchive
 * GrpArchive class constructor
 *******************************************************************/
GrpArchive::GrpArchive() : TreelessArchive(ARCHIVE_GRP)
{
	desc.max_name_length = 12;
	desc.names_extensions = false;
	desc.supports_dirs = false;
}

/* GrpArchive::~GrpArchive
 * GrpArchive class destructor
 *******************************************************************/
GrpArchive::~GrpArchive()
{
}

/* GrpArchive::getEntryOffset
 * Returns the file byte offset for [entry]
 *******************************************************************/
uint32_t GrpArchive::getEntryOffset(ArchiveEntry* entry)
{
	// Check entry
	if (!checkEntry(entry))
		return 0;

	return (uint32_t)(int)entry->exProp("Offset");
}

/* GrpArchive::setEntryOffset
 * Sets the file byte offset for [entry]
 *******************************************************************/
void GrpArchive::setEntryOffset(ArchiveEntry* entry, uint32_t offset)
{
	// Check entry
	if (!checkEntry(entry))
		return;

	entry->exProp("Offset") = (int)offset;
}

/* GrpArchive::getFileExtensionString
 * Gets the wxWidgets file dialog filter string for the archive type
 *******************************************************************/
string GrpArchive::getFileExtensionString()
{
	return "Grp Files (*.grp)|*.grp";
}

/* GrpArchive::getFormat
 * Returns the EntryDataFormat id of this archive type
 *******************************************************************/
string GrpArchive::getFormat()
{
	return "archive_grp";
}

/* GrpArchive::open
 * Reads grp format data from a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool GrpArchive::open(MemChunk& mc)
{
	// Check data was given
	if (!mc.hasData())
		return false;

	// Read grp header
	uint32_t	num_lumps = 0;
	char		ken_magic[13] = "";
	mc.seek(0, SEEK_SET);
	mc.read(ken_magic, 12);		// "KenSilverman"
	mc.read(&num_lumps, 4);		// No. of lumps in grp

	// Byteswap values for big endian if needed
	num_lumps = wxINT32_SWAP_ON_BE(num_lumps);

	// Null-terminate the magic header
	ken_magic[12] = 0;

	// Check the header
	if (!(S_CMP(wxString::FromAscii(ken_magic), "KenSilverman")))
	{
		wxLogMessage("GrpArchive::openFile: File %s has invalid header", filename);
		Global::error = "Invalid grp header";
		return false;
	}

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	setMuted(true);

	// The header takes as much space as a directory entry
	uint32_t	entryoffset = 16 * (1 + num_lumps);

	// Read the directory
	theSplashWindow->setProgressMessage("Reading grp archive data");
	for (uint32_t d = 0; d < num_lumps; d++)
	{
		// Update splash window progress
		theSplashWindow->setProgress(((float)d / (float)num_lumps));

		// Read lump info
		char name[13] = "";
		uint32_t offset = entryoffset;
		uint32_t size = 0;

		mc.read(name, 12);		// Name
		mc.read(&size, 4);		// Size
		name[12] = '\0';

		// Byteswap values for big endian if needed
		size = wxINT32_SWAP_ON_BE(size);

		// Increase offset of next entry by this entry's size
		entryoffset += size;

		// If the lump data goes past the end of the file,
		// the grpfile is invalid
		if (offset + size > mc.getSize())
		{
			wxLogMessage("GrpArchive::open: grp archive is invalid or corrupt");
			Global::error = "Archive is invalid and/or corrupt";
			setMuted(false);
			return false;
		}

		// Create & setup lump
		ArchiveEntry* nlump = new ArchiveEntry(wxString::FromAscii(name), size);
		nlump->setLoaded(false);
		nlump->exProp("Offset") = (int)offset;
		nlump->setState(0);

		// Add to entry list
		getRoot()->addEntry(nlump);
	}

	// Detect all entry types
	MemChunk edata;
	theSplashWindow->setProgressMessage("Detecting entry types");
	for (size_t a = 0; a < numEntries(); a++)
	{
		// Update splash window progress
		theSplashWindow->setProgress((((float)a / (float)num_lumps)));

		// Get entry
		ArchiveEntry* entry = getEntry(a);

		// Read entry data if it isn't zero-sized
		if (entry->getSize() > 0)
		{
			// Read the entry data
			mc.exportMemChunk(edata, getEntryOffset(entry), entry->getSize());
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

	// Detect maps (will detect map entry types)
	//theSplashWindow->setProgressMessage("Detecting maps");
	//detectMaps();

	// Setup variables
	setMuted(false);
	setModified(false);
	announce("opened");

	theSplashWindow->setProgressMessage("");

	return true;
}

/* GrpArchive::write
 * Writes the grp archive to a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool GrpArchive::write(MemChunk& mc, bool update)
{
	// Clear/init MemChunk
	mc.clear();
	mc.seek(0, SEEK_SET);
	mc.reSize((1 + numEntries()) * 16);
	ArchiveEntry* entry = NULL;

	// Write the header
	uint32_t num_lumps = numEntries();
	mc.write("KenSilverman", 12);
	mc.write(&num_lumps, 4);

	// Write the directory
	for (uint32_t l = 0; l < num_lumps; l++)
	{
		entry = getEntry(l);
		char name[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		long size = entry->getSize();

		for (size_t c = 0; c < entry->getName().length() && c < 12; c++)
			name[c] = entry->getName()[c];

		mc.write(name, 12);
		mc.write(&size, 4);

		if (update)
		{
			long offset = getEntryOffset(entry);
			entry->setState(0);
			entry->exProp("Offset") = (int)offset;
		}
	}

	// Write the lumps
	for (uint32_t l = 0; l < num_lumps; l++)
	{
		entry = getEntry(l);
		mc.write(entry->getData(), entry->getSize());
	}

	return true;
}

/* GrpArchive::loadEntryData
 * Loads an entry's data from the grpfile
 * Returns true if successful, false otherwise
 *******************************************************************/
bool GrpArchive::loadEntryData(ArchiveEntry* entry)
{
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

	// Open grpfile
	wxFile file(filename);

	// Check if opening the file failed
	if (!file.IsOpened())
	{
		wxLogMessage("GrpArchive::loadEntryData: Failed to open grpfile %s", filename);
		return false;
	}

	// Seek to lump offset in file and read it in
	file.Seek(getEntryOffset(entry), wxFromStart);
	entry->importFileStream(file, entry->getSize());

	// Set the lump to loaded
	entry->setLoaded();

	return true;
}

/* GrpArchive::addEntry
 * Override of Archive::addEntry to force entry addition to the root
 * directory, update namespaces if needed and rename the entry if
 * necessary to be grp-friendly (12 characters max with extension)
 *******************************************************************/
ArchiveEntry* GrpArchive::addEntry(ArchiveEntry* entry, unsigned position, ArchiveTreeNode* dir, bool copy)
{
	// Check entry
	if (!entry)
		return NULL;

	// Check if read-only
	if (isReadOnly())
		return NULL;

	// Copy if necessary
	if (copy)
		entry = new ArchiveEntry(*entry);

	// Process name (must be 12 characters max)
	string name = entry->getName().Truncate(12);
	if (wad_force_uppercase) name.MakeUpper();

	// Set new grp-friendly name
	entry->setName(name);

	// Do default entry addition (to root directory)
	Archive::addEntry(entry, position);

	return entry;
}

/* GrpArchive::addEntry
 * Since GRP files have no namespaces, just call the other function.
 *******************************************************************/
ArchiveEntry* GrpArchive::addEntry(ArchiveEntry* entry, string add_namespace, bool copy)
{
	return addEntry(entry, 0xFFFFFFFF, NULL, copy);
}

/* GrpArchive::renameEntry
 * Override of Archive::renameEntry to update namespaces if needed
 * and rename the entry if necessary to be grp-friendly (twelve
 * characters max)
 *******************************************************************/
bool GrpArchive::renameEntry(ArchiveEntry* entry, string name)
{
	// Check entry
	if (!checkEntry(entry))
		return false;

	// Process name (must be 12 characters max)
	name.Truncate(12);
	if (wad_force_uppercase) name.MakeUpper();

	// Do default rename
	return Archive::renameEntry(entry, name);
}

/* GrpArchive::isGrpArchive
 * Checks if the given data is a valid Duke Nukem 3D grp archive
 *******************************************************************/
bool GrpArchive::isGrpArchive(MemChunk& mc)
{
	// Check size
	if (mc.getSize() < 16)
		return false;

	// Get number of lumps
	uint32_t	num_lumps = 0;
	char		ken_magic[13] = "";
	mc.seek(0, SEEK_SET);
	mc.read(ken_magic, 12);		// "KenSilverman"
	mc.read(&num_lumps, 4);		// No. of lumps in grp

	// Byteswap values for big endian if needed
	num_lumps = wxINT32_SWAP_ON_BE(num_lumps);

	// Null-terminate the magic header
	ken_magic[12] = 0;

	// Check the header
	if (!(S_CMP(wxString::From8BitData(ken_magic), "KenSilverman")))
		return false;

	// Compute total size
	uint32_t totalsize = (1 + num_lumps) * 16;
	uint32_t size = 0;
	for (uint32_t a = 0; a < num_lumps; ++a)
	{
		mc.read(ken_magic, 12);
		mc.read(&size, 4);
		totalsize += size;
	}

	// Check if total size is correct
	if (totalsize > mc.getSize())
		return false;

	// If it's passed to here it's probably a grp file
	return true;
}

/* GrpArchive::isGrpArchive
 * Checks if the file at [filename] is a valid DN3D grp archive
 *******************************************************************/
bool GrpArchive::isGrpArchive(string filename)
{
	// Open file for reading
	wxFile file(filename);

	// Check it opened ok
	if (!file.IsOpened())
		return false;

	// Check size
	if (file.Length() < 16)
		return false;

	// Get number of lumps
	uint32_t	num_lumps = 0;
	char		ken_magic[13] = "";
	file.Seek(0, wxFromStart);
	file.Read(ken_magic, 12);		// "KenSilverman"
	file.Read(&num_lumps, 4);		// No. of lumps in grp

	// Byteswap values for big endian if needed
	num_lumps = wxINT32_SWAP_ON_BE(num_lumps);

	// Null-terminate the magic header
	ken_magic[12] = 0;

	// Check the header
	if (!(S_CMP(wxString::From8BitData(ken_magic), "KenSilverman")))
		return false;

	// Compute total size
	uint32_t totalsize = (1 + num_lumps) * 16;
	uint32_t size = 0;
	for (uint32_t a = 0; a < num_lumps; ++a)
	{
		file.Read(ken_magic, 12);
		file.Read(&size, 4);
		totalsize += size;
	}

	// Check if total size is correct
	if (totalsize > file.Length())
		return false;

	// If it's passed to here it's probably a grp file
	return true;
}


/*******************************************************************
 * EXTRA CONSOLE COMMANDS
 *******************************************************************/
#include "General/Console/Console.h"
#include "MainEditor/MainWindow.h"

CONSOLE_COMMAND(lookupdat, 0, false)
{
	ArchiveEntry* entry = theMainWindow->getCurrentEntry();

	if (!entry)
		return;

	MemChunk& mc = entry->getMCData();
	if (mc.getSize() == 0)
		return;

	ArchiveEntry* nentry = NULL;
	uint32_t* data = NULL;
	int index = entry->getParent()->entryIndex(entry, entry->getParentDir());
	mc.seek(0, SEEK_SET);

	// Create lookup table
	uint8_t numlookup = 0;
	uint8_t dummy = 0;
	mc.read(&numlookup, 1);
	if (mc.getSize() < (uint32_t)((numlookup * 256)+(5*768)+1))
		return;

	nentry = entry->getParent()->addNewEntry("COLORMAP.DAT", index+1, entry->getParentDir());
	if (!nentry)
		return;

	data = new uint32_t[numlookup * 256];
	for (int i = 0; i < numlookup; ++i)
	{
		mc.read(&dummy, 1);
		mc.read(data, numlookup * 256);
	}
	nentry->importMem(data, numlookup * 256);
	delete[] data;

	// Create extra palettes
	data = new uint32_t[768];
	nentry = entry->getParent()->addNewEntry("WATERPAL.PAL", index+2, entry->getParentDir());
	if (!nentry) { delete[] data; return; }
	mc.read(data, 768); nentry->importMem(data, 768);
	nentry = entry->getParent()->addNewEntry("SLIMEPAL.PAL", index+3, entry->getParentDir());
	if (!nentry) { delete[] data; return; }
	mc.read(data, 768); nentry->importMem(data, 768);
	nentry = entry->getParent()->addNewEntry("TITLEPAL.PAL", index+4, entry->getParentDir());
	if (!nentry) { delete[] data; return; }
	mc.read(data, 768); nentry->importMem(data, 768);
	nentry = entry->getParent()->addNewEntry("3DREALMS.PAL", index+5, entry->getParentDir());
	if (!nentry) { delete[] data; return; }
	mc.read(data, 768); nentry->importMem(data, 768);
	nentry = entry->getParent()->addNewEntry("ENDINPAL.PAL", index+6, entry->getParentDir());
	if (!nentry) { delete[] data; return; }
	mc.read(data, 768); nentry->importMem(data, 768);

	// Clean up and go away
	delete[] data;
	mc.clear();
}

CONSOLE_COMMAND(palettedat, 0, false)
{
	ArchiveEntry* entry = theMainWindow->getCurrentEntry();

	if (!entry)
		return;

	MemChunk& mc = entry->getMCData();
	// Minimum size: 768 bytes for the palette, 2 for the number of lookup tables,
	// 0 for these tables if there are none, and 65536 for the transparency map.
	if (mc.getSize() < 66306)
		return;

	ArchiveEntry* nentry = NULL;
	uint32_t* data = NULL;
	int index = entry->getParent()->entryIndex(entry, entry->getParentDir());
	mc.seek(0, SEEK_SET);

	// Create palette
	data = new uint32_t[768];
	nentry = entry->getParent()->addNewEntry("MAINPAL.PAL", index+1, entry->getParentDir()); if (!nentry) return;
	mc.read(data, 768); nentry->importMem(data, 768);

	// Create lookup tables
	uint16_t numlookup = 0; mc.read(&numlookup, 2);
	numlookup = wxINT16_SWAP_ON_BE(numlookup);
	delete[] data;
	nentry = entry->getParent()->addNewEntry("COLORMAP.DAT", index+2, entry->getParentDir()); if (!nentry) return;
	data = new uint32_t[numlookup * 256];
	mc.read(data, numlookup * 256); nentry->importMem(data, numlookup * 256);

	// Create transparency tables
	delete[] data;
	nentry = entry->getParent()->addNewEntry("TRANMAP.DAT", index+3, entry->getParentDir()); if (!nentry) return;
	data = new uint32_t[65536];
	mc.read(data, 65536); nentry->importMem(data, 65536);

	// Clean up and go away
	delete[] data;
	mc.clear();
}

CONSOLE_COMMAND(tablesdat, 0, false)
{
	ArchiveEntry* entry = theMainWindow->getCurrentEntry();

	if (!entry)
		return;

	MemChunk& mc = entry->getMCData();
	// Sin/cos table: 4096; atn table 1280; gamma table 1024
	// Fonts: 1024 byte each.
	if (mc.getSize() != 8448)
		return;

	ArchiveEntry* nentry = NULL;
	uint32_t* data = NULL;
	int index = entry->getParent()->entryIndex(entry, entry->getParentDir());
	mc.seek(5376, SEEK_SET);

	// Create fonts
	data = new uint32_t[1024];
	nentry = entry->getParent()->addNewEntry("VGAFONT1.FNT", index+1, entry->getParentDir()); if (!nentry) return;
	mc.read(data, 1024); nentry->importMem(data, 1024);
	nentry = entry->getParent()->addNewEntry("VGAFONT2.FNT", index+2, entry->getParentDir()); if (!nentry) return;
	mc.read(data, 1024); nentry->importMem(data, 1024);

	// Clean up and go away
	delete[] data;
	mc.clear();
}
