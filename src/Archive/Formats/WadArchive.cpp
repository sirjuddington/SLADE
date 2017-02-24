
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    WadArchive.cpp
 * Description: WadArchive, archive class to handle doom format
 *              wad archives
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
#include "WadArchive.h"
#include "UI/SplashWindow.h"
#include "General/Misc.h"
#include "Utility/Tokenizer.h"

bool JaguarDecode(MemChunk& mc);

/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Bool, wad_force_uppercase, true, CVAR_SAVE)
CVAR(Bool, iwad_lock, true, CVAR_SAVE)

// Used for map detection
string map_lumps[NUMMAPLUMPS] =
{
	"THINGS",
	"VERTEXES",
	"LINEDEFS",
	"SIDEDEFS",
	"SECTORS",
	"SEGS",
	"SSECTORS",
	"NODES",
	"BLOCKMAP",
	"REJECT",
	"SCRIPTS",
	"BEHAVIOR",
	"LEAFS",
	"LIGHTS",
	"MACROS",
	"GL_MAP01",
	"GL_VERT",
	"GL_SEGS",
	"GL_SSECT",
	"GL_NODES",
	"GL_PVS",
	"TEXTMAP",
	"ZNODES"
};

// Special namespaces (at the moment these are just mapping to zdoom's "zip as wad" namespace folders)
// http://zdoom.org/wiki/Using_ZIPs_as_WAD_replacement#How_to
struct ns_special_t { string name; string letter; };
ns_special_t special_namespaces[] =
{
	{ "patches",	"p"		},
	{ "sprites",	"s"		},
	{ "flats",		"f"		},
	{ "textures",	"tx"	},
	{ "textures",	"t"		},	// alias for Jaguar Doom & Doom 64
	{ "hires",		"hi"	},
	{ "colormaps",	"c"		},
	{ "acs",		"a"		},
	{ "voices",		"v"		},
	{ "voxels",		"vx"	},
	{ "sounds",		"ds"	}, // Jaguar Doom and Doom 64 use it
};
const int n_special_namespaces = 11;

/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, archive_load_data)

/*******************************************************************
 * WADARCHIVE CLASS FUNCTIONS
 *******************************************************************/

/* WadArchive::WadArchive
 * WadArchive class constructor
 *******************************************************************/
WadArchive::WadArchive() : TreelessArchive(ARCHIVE_WAD)
{
	// Init variables
	desc.max_name_length = 8;
	desc.names_extensions = false;
	iwad = false;
}

/* WadArchive::~WadArchive
 * WadArchive class destructor
 *******************************************************************/
WadArchive::~WadArchive()
{
}

bool WadArchive::isWritable()
{
	return !(iwad && iwad_lock);
}

/* WadArchive::getEntryOffset
 * Returns the file byte offset for [entry]
 *******************************************************************/
uint32_t WadArchive::getEntryOffset(ArchiveEntry* entry)
{
	// Check entry
	if (!checkEntry(entry))
		return 0;

	return (uint32_t)(int)entry->exProp("Offset");
}

/* WadArchive::setEntryOffset
 * Sets the file byte offset for [entry]
 *******************************************************************/
void WadArchive::setEntryOffset(ArchiveEntry* entry, uint32_t offset)
{
	// Check entry
	if (!checkEntry(entry))
		return;

	entry->exProp("Offset") = (int)offset;
}

/* WadArchive::updateNamespaces
 * Updates the namespace list
 *******************************************************************/
void WadArchive::updateNamespaces()
{
	// Clear current namespace info
	while (namespaces.size() > 0)
		namespaces.pop_back();

	// Go through all entries
	for (unsigned a = 0; a < numEntries(); a++)
	{
		ArchiveEntry* entry = getRoot()->getEntry(a);

		// Check for namespace begin
		if (entry->getName().Matches("*_START"))
		{
			// Create new namespace
			wad_ns_pair_t ns(entry, NULL);
			string name = entry->getName();
			ns.name = name.Left(name.Length() - 6).Lower();
			ns.start_index = entryIndex(ns.start);

			// Convert some special cases (because technically PP_START->P_END is a valid namespace)
			if (ns.name == "pp")
				ns.name = "p";
			if (ns.name == "ff")
				ns.name = "f";
			if (ns.name == "ss")
				ns.name = "s";
			if (ns.name == "tt")
				ns.name = "t";

			// Add to namespace list
			namespaces.push_back(ns);
		}
		// Check for namespace end
		else if (entry->getName().Matches("?_END") || entry->getName().Matches("??_END"))
		{
			// Get namespace 'name'
			int len = entry->getName().Length() - 4;
			string ns_name = entry->getName().Left(len).Lower();

			// Convert some special cases (because technically P_START->PP_END is a valid namespace)
			if (ns_name == "pp")
				ns_name = "p";
			if (ns_name == "ff")
				ns_name = "f";
			if (ns_name == "ss")
				ns_name = "s";
			if (ns_name == "tt")
				ns_name = "t";

			// Check if it's the end of an existing namespace
			// Remember entry is getEntry(a)? index is 'a'
			//size_t index = entryIndex(entry);

			bool found = false;
			for (unsigned b = 0; b < namespaces.size(); b++)
			{
				// Can't close a namespace that starts afterwards
				if (namespaces[b].start_index > a)
					break;
				// Can't close an already-closed namespace
				if (namespaces[b].end != NULL)
					continue;
				if (S_CMP(ns_name, namespaces[b].name))
				{
					found = true;
					namespaces[b].end = entry;
					namespaces[b].end_index = a;
					break;
				}
			}
			// Flat hack: closing the flat namespace without opening it
			if (found == false && ns_name == "f")
			{
				wad_ns_pair_t ns(getRoot()->getEntry(0), entry);
				ns.start_index = 0;
				ns.end_index = a;
				ns.name = "f";
				namespaces.push_back(ns);
			}
		}
	}

	// ROTT stuff. The first lump in the archive is always WALLSTRT, the last lump is either
	// LICENSE (darkwar.wad) or VENDOR (huntbgin.wad), with TABLES just before in both cases.
	// The shareware version has 2091 lumps, the complete version has about 50% more.
	if (numEntries() > 2090 && getRoot()->getEntry(0)->getName().Matches("WALLSTRT") &&
	        getRoot()->getEntry(numEntries()-2)->getName().Matches("TABLES"))
	{
		wad_ns_pair_t ns(getRoot()->getEntry(0), getRoot()->getEntry(numEntries()-1));
		ns.name = "rott";
		ns.start_index = 0;
		ns.end_index = entryIndex(ns.end);
		namespaces.push_back(ns);
	}


	// Check namespaces
	for (unsigned a = 0; a < namespaces.size(); a++)
	{
		wad_ns_pair_t& ns = namespaces[a];

		// Check the namespace has an end
		if (!ns.end)
		{
			// If not, remove the namespace as it is invalid
			namespaces.erase(namespaces.begin() + a);
			a--;
			continue;
		}

		// Check namespace name for special cases
		for (int n = 0; n < n_special_namespaces; n++)
		{
			if (S_CMP(ns.name, special_namespaces[n].letter))
				ns.name = special_namespaces[n].name;
		}

		ns.start_index = entryIndex(ns.start);
		ns.end_index = entryIndex(ns.end);

		// Testing
		//wxLogMessage("Namespace %s from %s (%d) to %s (%d)", ns.name,
		//	ns.start->getName(), ns.start_index, ns.end->getName(), ns.end_index);
	}
}

/* WadArchive::hasFlatHack
 * Detects if the flat hack is used in this archive or not
 *******************************************************************/
bool WadArchive::hasFlatHack()
{
	for (size_t i = 0; i < namespaces.size(); ++i)
	{
		if (namespaces[i].name == "f")
		{
			return (namespaces[i].start_index == 0 && namespaces[i].start->getSize() != 0);
		}
	}
	return false;
}

/* WadArchive::getFileExtensionString
 * Gets the wxWidgets file dialog filter string for the archive type
 *******************************************************************/
string WadArchive::getFileExtensionString()
{
	return "Wad Files (*.wad)|*.wad";
}

/* WadArchive::getFormat
 * Returns the EntryDataFormat id of this archive type
 *******************************************************************/
string WadArchive::getFormat()
{
	return "archive_wad";
}

/* WadArchive::open
 * Reads wad format data from a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool WadArchive::open(MemChunk& mc)
{
	// Check data was given
	if (!mc.hasData())
		return false;

	// Read wad header
	uint32_t	num_lumps = 0;
	uint32_t	dir_offset = 0;
	char		wad_type[4] = "";
	mc.seek(0, SEEK_SET);
	mc.read(&wad_type, 4);		// Wad type
	mc.read(&num_lumps, 4);		// No. of lumps in wad
	mc.read(&dir_offset, 4);	// Offset to directory

	// Byteswap values for big endian if needed
	num_lumps = wxINT32_SWAP_ON_BE(num_lumps);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);

	// Check the header
	if (wad_type[1] != 'W' || wad_type[2] != 'A' || wad_type[3] != 'D')
	{
		wxLogMessage("WadArchive::openFile: File %s has invalid header", filename);
		Global::error = "Invalid wad header";
		return false;
	}

	// Check for iwad
	if (wad_type[0] == 'I')
		iwad = true;

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	setMuted(true);

	vector<uint32_t> offsets;

	// Read the directory
	mc.seek(dir_offset, SEEK_SET);
	theSplashWindow->setProgressMessage("Reading wad archive data");
	for (uint32_t d = 0; d < num_lumps; d++)
	{
		// Update splash window progress
		theSplashWindow->setProgress(((float)d / (float)num_lumps));

		// Read lump info
		char name[9] = "";
		uint32_t offset = 0;
		uint32_t size = 0;

		mc.read(&offset, 4);	// Offset
		mc.read(&size, 4);		// Size
		mc.read(name, 8);		// Name
		name[8] = '\0';

		// Byteswap values for big endian if needed
		offset = wxINT32_SWAP_ON_BE(offset);
		size = wxINT32_SWAP_ON_BE(size);

		// Check to catch stupid shit
		if (size > 0)
		{
			if (offset == 0)
			{
				LOG_MESSAGE(2, "No.");
				continue;
			}
			if (VECTOR_EXISTS(offsets, offset))
			{
				LOG_MESSAGE(1, "Ignoring entry %d: %s, is a clone of a previous entry", d, name);
				continue;
			}
			offsets.push_back(offset);
		}

		// Hack to open Operation: Rheingold WAD files
		if (size == 0 && offset > mc.getSize())
			offset = 0;

		// Is there a compression/encryption thing going on?
		bool jaguarencrypt = !!(name[0] & 0x80);	// look at high bit
		name[0] = name[0] & 0x7F;					// then strip it away

		// Look for encryption shenanigans
		size_t actualsize = size;
		if (jaguarencrypt)
		{
			if (d < num_lumps - 1)
			{
				size_t pos = mc.currentPos();
				uint32_t nextoffset = 0;
				for (int i = 0; i + d < num_lumps; ++i)
				{
					mc.read(&nextoffset, 4);
					if (nextoffset != 0) break;
					mc.seek(12, SEEK_CUR);
				}
				nextoffset = wxINT32_SWAP_ON_BE(nextoffset);
				if (nextoffset == 0) nextoffset = dir_offset;
				mc.seek(pos, SEEK_SET);
				actualsize = nextoffset - offset;
			}
			else
			{
				if (offset > dir_offset)
				{
					actualsize = mc.getSize() - offset;
				}
				else
				{
					actualsize = dir_offset - offset;
				}
			}
		}

		// If the lump data goes past the end of the file,
		// the wadfile is invalid
		if (offset + actualsize > mc.getSize())
		{
			wxLogMessage("WadArchive::open: Wad archive is invalid or corrupt");
			Global::error = S_FMT("Archive is invalid and/or corrupt (lump %d: %s data goes past end of file)", d, name);
			setMuted(false);
			return false;
		}

		// Create & setup lump
		ArchiveEntry* nlump = new ArchiveEntry(wxString::FromAscii(name), size);
		nlump->setLoaded(false);
		nlump->exProp("Offset") = (int)offset;
		nlump->setState(0);

		if (jaguarencrypt)
		{
			nlump->setEncryption(ENC_JAGUAR);
			nlump->exProp("FullSize") = (int)size;
		}

		// Add to entry list
		getRoot()->addEntry(nlump);
	}

	// Detect namespaces (needs to be done before type detection as some types
	// rely on being within certain namespaces)
	updateNamespaces();

	// Detect all entry types
	MemChunk edata;
	theSplashWindow->setProgressMessage("Detecting entry types");
	for (size_t a = 0; a < numEntries(); a++)
	{
		// Update splash window progress
		theSplashWindow->setProgress((((float)a / (float)numEntries())));

		// Get entry
		ArchiveEntry* entry = getEntry(a);

		// Read entry data if it isn't zero-sized
		if (entry->getSize() > 0)
		{
			// Read the entry data
			mc.exportMemChunk(edata, getEntryOffset(entry), entry->getSize());
			if (entry->isEncrypted())
			{
				if (entry->exProps().propertyExists("FullSize")
				        && (unsigned)(int)(entry->exProp("FullSize")) >  entry->getSize())
					edata.reSize((int)(entry->exProp("FullSize")), true);
				if (!JaguarDecode(edata))
					wxLogMessage("%i: %s (following %s), did not decode properly", a, entry->getName(), a>0?getEntry(a-1)->getName():"nothing");
			}
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

	// Identify #included lumps (DECORATE, GLDEFS, etc.)
	detectIncludes();

	// Detect maps (will detect map entry types)
	theSplashWindow->setProgressMessage("Detecting maps");
	detectMaps();

	// Setup variables
	setMuted(false);
	setModified(false);
	//if (iwad && iwad_lock) read_only = true;
	announce("opened");

	theSplashWindow->setProgressMessage("");

	return true;
}

/* WadArchive::write
 * Writes the wad archive to a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool WadArchive::write(MemChunk& mc, bool update)
{
	// Don't write if iwad
	if (iwad && iwad_lock)
	{
		Global::error = "IWAD saving disabled";
		return false;
	}

	// Determine directory offset & individual lump offsets
	uint32_t dir_offset = 12;
	ArchiveEntry* entry = NULL;
	for (uint32_t l = 0; l < numEntries(); l++)
	{
		entry = getEntry(l);
		setEntryOffset(entry, dir_offset);
		dir_offset += entry->getSize();
	}

	// Clear/init MemChunk
	mc.clear();
	mc.seek(0, SEEK_SET);
	if (!mc.reSize(dir_offset + numEntries() * 16))
	{
		Global::error = "Failed to allocate sufficient memory";
		return false;
	}

	// Setup wad type
	char wad_type[4] = { 'P', 'W', 'A', 'D' };
	if (iwad) wad_type[0] = 'I';

	// Write the header
	uint32_t num_lumps = numEntries();
	mc.write(wad_type, 4);
	mc.write(&num_lumps, 4);
	mc.write(&dir_offset, 4);

	// Write the lumps
	for (uint32_t l = 0; l < num_lumps; l++)
	{
		entry = getEntry(l);
		mc.write(entry->getData(), entry->getSize());
	}

	// Write the directory
	for (uint32_t l = 0; l < num_lumps; l++)
	{
		entry = getEntry(l);
		char name[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
		long offset = getEntryOffset(entry);
		long size = entry->getSize();

		for (size_t c = 0; c < entry->getName().length() && c < 8; c++)
			name[c] = entry->getName()[c];

		mc.write(&offset, 4);
		mc.write(&size, 4);
		mc.write(name, 8);

		if (update)
		{
			entry->setState(0);
			entry->exProp("Offset") = (int)offset;
		}
	}

	return true;
}

/* WadArchive::write
 * Writes the wad archive to a file at [filename]
 * Returns true if successful, false otherwise
 *******************************************************************/
bool WadArchive::write(string filename, bool update)
{
	// Don't write if iwad
	if (iwad && iwad_lock)
	{
		Global::error = "IWAD saving disabled";
		return false;
	}

	// Open file for writing
	wxFile file;
	file.Open(filename, wxFile::write);
	if (!file.IsOpened())
	{
		Global::error = "Unable to open file for writing";
		return false;
	}

	// Determine directory offset & individual lump offsets
	uint32_t dir_offset = 12;
	ArchiveEntry* entry = NULL;
	for (uint32_t l = 0; l < numEntries(); l++)
	{
		entry = getEntry(l);
		setEntryOffset(entry, dir_offset);
		dir_offset += entry->getSize();
	}

	// Setup wad type
	char wad_type[4] = { 'P', 'W', 'A', 'D' };
	if (iwad) wad_type[0] = 'I';

	// Write the header
	uint32_t num_lumps = numEntries();
	file.Write(wad_type, 4);
	file.Write(&num_lumps, 4);
	file.Write(&dir_offset, 4);

	// Write the lumps
	for (uint32_t l = 0; l < num_lumps; l++)
	{
		entry = getEntry(l);
		if (entry->getSize())
		{
			file.Write(entry->getData(), entry->getSize());
		}
	}

	// Write the directory
	for (uint32_t l = 0; l < num_lumps; l++)
	{
		entry = getEntry(l);
		char name[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
		long offset = getEntryOffset(entry);
		long size = entry->getSize();

		for (size_t c = 0; c < entry->getName().length() && c < 8; c++)
			name[c] = entry->getName()[c];

		file.Write(&offset, 4);
		file.Write(&size, 4);
		file.Write(name, 8);

		if (update)
		{
			entry->setState(0);
			entry->exProp("Offset") = (int)offset;
		}
	}

	file.Close();

	return true;
}

/* WadArchive::loadEntryData
 * Loads an entry's data from the wadfile
 * Returns true if successful, false otherwise
 *******************************************************************/
bool WadArchive::loadEntryData(ArchiveEntry* entry)
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

	// Open wadfile
	wxFile file(filename);

	// Check if opening the file failed
	if (!file.IsOpened())
	{
		wxLogMessage("WadArchive::loadEntryData: Failed to open wadfile %s", filename);
		return false;
	}

	// Seek to lump offset in file and read it in
	file.Seek(getEntryOffset(entry), wxFromStart);
	entry->importFileStream(file, entry->getSize());

	// Set the lump to loaded
	entry->setLoaded();
	entry->setState(0);

	return true;
}

/* WadArchive::addEntry
 * Override of Archive::addEntry to force entry addition to the root
 * directory, update namespaces if needed and rename the entry if
 * necessary to be wad-friendly (8 characters max and no file
 * extension)
 *******************************************************************/
ArchiveEntry* WadArchive::addEntry(ArchiveEntry* entry, unsigned position, ArchiveTreeNode* dir, bool copy)
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

	// Set new wad-friendly name
	string name = processEntryName(entry->getName());
	entry->setName(name);

	// Do default entry addition (to root directory)
	Archive::addEntry(entry, position);

	// Update namespaces if necessary
	if (name.EndsWith("_START") ||
	        name.EndsWith("_END"))
		updateNamespaces();

	return entry;
}

/* WadArchive::addEntry
 * Adds [entry] to the end of the namespace matching [add_namespace].
 * If [copy] is true a copy of the entry is added. Returns the added
 * entry or NULL if the entry is invalid
 *******************************************************************/
ArchiveEntry* WadArchive::addEntry(ArchiveEntry* entry, string add_namespace, bool copy)
{
	// Find requested namespace
	for (unsigned a = 0; a < namespaces.size(); a++)
	{
		if (S_CMPNOCASE(namespaces[a].name, add_namespace))
		{
			// Namespace found, add entry before end marker
			return addEntry(entry, namespaces[a].end_index++, NULL, copy);
		}
	}

	// If the requested namespace is a special namespace and doesn't exist, create it
	for (int a = 0; a < n_special_namespaces; a++)
	{
		if (add_namespace == special_namespaces[a].name)
		{
			addNewEntry(special_namespaces[a].letter + "_start");
			addNewEntry(special_namespaces[a].letter + "_end");
			return addEntry(entry, add_namespace, copy);
		}
	}

	// Unsupported namespace not found, so add to global namespace (ie end of archive)
	return addEntry(entry, 0xFFFFFFFF, NULL, copy);
}

/* WadArchive::removeEntry
 * Override of Archive::removeEntry to update namespaces if needed
 *******************************************************************/
bool WadArchive::removeEntry(ArchiveEntry* entry)
{
	// Check entry
	if (!checkEntry(entry))
		return false;

	// Get entry name (for later)
	string name = entry->getName();

	// Do default remove
	bool ok = Archive::removeEntry(entry);

	if (ok)
	{
		// Update namespaces if necessary
		if (name.Upper().Matches("*_START") ||
		        name.Upper().Matches("*_END"))
			updateNamespaces();

		return true;
	}
	else
		return false;
}

/* WadArchive::processEntryName
 * Contains the WadArchive::renameEntry logic
 *******************************************************************/
string WadArchive::processEntryName(string name)
{

	// Perform character substitution if needed
	name = Misc::fileNameToLumpName(name);

	// Check for \ character (e.g., from Arch-Viles graphics). They have to be kept.
	if (name.length() <= 8 && name.find('\\') != wxNOT_FOUND)
	{
	} // Don't process as a file name

	// Process name (must be 8 characters max, also cut any extension as wad entries don't usually want them)
	else
	{
		wxFileName fn(name);
		name = fn.GetName().Truncate(8);
	}
	if (wad_force_uppercase) name.MakeUpper();

	return name;
}
/* WadArchive::renameEntry
 * Override of Archive::renameEntry to update namespaces if needed
 * and rename the entry if necessary to be wad-friendly (8 characters
 * max and no file extension)
 *******************************************************************/
bool WadArchive::renameEntry(ArchiveEntry* entry, string name)
{
	// Check entry
	if (!checkEntry(entry))
		return false;
	name = processEntryName(name);

	// Do default rename
	bool ok = Archive::renameEntry(entry, name);

	if (ok)
	{
		// Update namespaces if necessary
		if (entry->getName().Upper().Matches("*_START") ||
		        entry->getName().Upper().Matches("*_END"))
			updateNamespaces();

		return true;
	}
	return false;
}

/* WadArchive::swapEntries
 * Override of Archive::swapEntries to update namespaces if needed
 *******************************************************************/
bool WadArchive::swapEntries(ArchiveEntry* entry1, ArchiveEntry* entry2)
{
	// Check entries
	if (!checkEntry(entry1) || !checkEntry(entry2))
		return false;

	// Do default swap (force root dir)
	bool ok = Archive::swapEntries(entry1, entry2);

	if (ok)
	{
		// Update namespaces if needed
		if (entry1->getName().Upper().Matches("*_START") ||
		        entry1->getName().Upper().Matches("*_END") ||
		        entry2->getName().Upper().Matches("*_START") ||
		        entry2->getName().Upper().Matches("*_END"))
			updateNamespaces();

		return true;
	}
	else
		return false;
}

/* WadArchive::moveEntry
 * Override of Archive::moveEntry to update namespaces if needed
 *******************************************************************/
bool WadArchive::moveEntry(ArchiveEntry* entry, unsigned position, ArchiveTreeNode* dir)
{
	// Check entry
	if (!checkEntry(entry))
		return false;

	// Do default move (force root dir)
	bool ok = Archive::moveEntry(entry, position, NULL);

	if (ok)
	{
		// Update namespaces if necessary
		if (entry->getName().Upper().Matches("*_START") ||
		        entry->getName().Upper().Matches("*_END"))
			updateNamespaces();

		return true;
	}
	else
		return false;
}

/* WadArchive::getMapInfo
 * Returns the mapdesc_t information about the map beginning at
 * [maphead]. If [maphead] is not really a map header entry, an
 * invalid mapdesc_t will be returned (mapdesc_t::head == NULL)
 *******************************************************************/
Archive::mapdesc_t WadArchive::getMapInfo(ArchiveEntry* maphead)
{
	mapdesc_t map;

	if (!maphead)
		return map;

	// Check for embedded wads (e.g., Doom 64 maps)
	if (maphead->getType()->getFormat() == "archive_wad")
	{
		map.archive = true;
		map.head = maphead;
		map.end = maphead;
		map.name = maphead->getName();
		return map;
	}

	// Check for UDMF format map
	if (S_CMPNOCASE(maphead->nextEntry()->getName(), "TEXTMAP"))
	{
		// Get map info
		map.head = maphead;
		map.name = maphead->getName();
		map.format = MAP_UDMF;

		// All entries until we find ENDMAP
		ArchiveEntry* entry = maphead->nextEntry();
		while (true)
		{
			if (!entry || S_CMPNOCASE(entry->getName(), "ENDMAP"))
				break;

			// Check for unknown map lumps
			bool known = false;
			for (unsigned a = 0; a < NUMMAPLUMPS; a++)
			{
				if (S_CMPNOCASE(entry->getName(), map_lumps[a]))
				{
					known = true;
					a = NUMMAPLUMPS;
				}
			}
			if (!known)
				map.unk.push_back(entry);

			// Next entry
			entry = entry->nextEntry();
		}

		// If we got to the end before we found ENDMAP, something is wrong
		if (!entry)
			return mapdesc_t();

		// Set end entry
		map.end = entry;

		return map;
	}

	// Check for doom/hexen format map
	uint8_t existing_map_lumps[NUMMAPLUMPS];
	memset(existing_map_lumps, 0, NUMMAPLUMPS);
	ArchiveEntry* entry = maphead->nextEntry();
	while (entry)
	{
		// Check that the entry is a valid map-related entry
		bool mapentry = false;
		for (unsigned a = 0; a < NUMMAPLUMPS; a++)
		{
			if (S_CMPNOCASE(entry->getName(), map_lumps[a]))
			{
				mapentry = true;
				existing_map_lumps[a] = 1;
				break;
			}
			else if (a == LUMP_GL_HEADER)
			{
				string name = maphead->getName(true);
				name.Prepend("GL_");
				if (S_CMPNOCASE(entry->getName(), name))
				{
					mapentry = true;
					existing_map_lumps[a] = 1;
					break;
				}
			}
		}

		// If it wasn't a map entry, exit this loop
		if (!mapentry)
		{
			entry = entry->prevEntry();
			break;
		}

		// If we've reached the end of the archive, exit this loop
		if (!entry->nextEntry())
			break;

		// Go to next entry
		entry = entry->nextEntry();
	}

	// Check for the required map entries
	for (unsigned a = 0; a < 5; a++)
	{
		if (existing_map_lumps[a] == 0)
			return mapdesc_t();
	}

	// Setup map info
	map.head = maphead;
	map.end = entry;
	map.name = maphead->getName();

	// If BEHAVIOR lump exists, it's a hexen format map
	if (existing_map_lumps[LUMP_BEHAVIOR])
		map.format = MAP_HEXEN;
	// If LEAFS, LIGHTS and MACROS exist, it's a doom 64 format map
	else if (existing_map_lumps[LUMP_LEAFS] && existing_map_lumps[LUMP_LIGHTS]
	         && existing_map_lumps[LUMP_MACROS])
		map.format = MAP_DOOM64;
	// Otherwise it's doom format
	else
		map.format = MAP_DOOM;

	return map;
}

/* WadArchive::detectMaps
 * Searches for any maps in the wad and adds them to the map list
 *******************************************************************/
vector<Archive::mapdesc_t> WadArchive::detectMaps()
{
	vector<mapdesc_t> maps;

	// Go through all lumps
	ArchiveEntry* entry = getEntry(0);
	bool lastentryismapentry = false;
	while (entry)
	{
		// UDMF format map check ********************************************************

		// Check for UDMF format map lump (TEXTMAP lump)
		if (entry->getName() == "TEXTMAP" && entry->prevEntry())
		{
			// Get map info
			mapdesc_t md = getMapInfo(entry->prevEntry());

			// Add to map list
			if (md.head != NULL)
			{
				entry = md.end;
				maps.push_back(md);
			}

			// Current index is ENDMAP, we don't want to check for a doom/hexen format
			// map so just go to the next index and continue the loop
			entry = entry->nextEntry();
			continue;
		}

		// Doom/Hexen format map check **************************************************
		// TODO maybe get rid of code duplication by calling getMapInfo() here too?

		// Array to keep track of what doom/hexen map lumps have been found
		uint8_t existing_map_lumps[NUMMAPLUMPS];
		memset(existing_map_lumps, 0, NUMMAPLUMPS);

		// Check if the current lump is a doom/hexen map lump
		bool maplump_found = false;
		for (int a = 0; a < 5; a++)
		{
			// Compare with all base map lump names
			if (S_CMP(entry->getName(), map_lumps[a]))
			{
				maplump_found = true;
				existing_map_lumps[a] = 1;
				break;
			}
		}

		// If we've found what might be a map
		if (maplump_found && entry->prevEntry())
		{
			// Save map header entry
			ArchiveEntry* header_entry = entry->prevEntry();

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
					if (S_CMP(entry->getName(), map_lumps[a]))
					{
						existing_map_lumps[a] = 1;
						done = false;
						break;
					}
				}

				// If we're at the end of the wad, exit the loop
				if (!entry->nextEntry())
				{
					lastentryismapentry = true;
					break;
				}

				// Go to next lump if there is one
				if (!lastentryismapentry) entry = entry->nextEntry();
			}

			// Go back to the lump just after the last map lump found, but only if we actually moved
			if (!lastentryismapentry) entry = entry->prevEntry();

			// Check that we have all the required map lumps: VERTEXES, LINEDEFS, SIDEDEFS, THINGS & SECTORS
			if (!memchr(existing_map_lumps, 0, 5))
			{
				// Get map info
				mapdesc_t md;
				md.head = header_entry;				// Header lump
				md.name = header_entry->getName();	// Map title
				md.end = lastentryismapentry ?		// End lump
				         entry : entry->prevEntry();

				// If BEHAVIOR lump exists, it's a hexen format map
				if (existing_map_lumps[LUMP_BEHAVIOR])
					md.format = MAP_HEXEN;
				// If LEAFS, LIGHTS and MACROS exist, it's a doom 64 format map
				else if (existing_map_lumps[LUMP_LEAFS] && existing_map_lumps[LUMP_LIGHTS]
				         && existing_map_lumps[LUMP_MACROS])
					md.format = MAP_DOOM64;
				// Otherwise it's doom format
				else
					md.format = MAP_DOOM;

				// Add map info to the maps list
				maps.push_back(md);
			}
		}

		// Embedded WAD check (for Doom 64)
		if (entry->getType()->getFormat() == "archive_wad")
		{
			// Detect map format (probably kinda slow but whatever, no better way to do it really)
			Archive* tempwad = new WadArchive();
			tempwad->open(entry);
			vector<mapdesc_t> emaps = tempwad->detectMaps();
			if (emaps.size() > 0)
			{
				mapdesc_t md;
				md.head = entry;
				md.end = entry;
				md.archive = true;
				md.name = entry->getName(true).Upper();
				md.format = emaps[0].format;
				maps.push_back(md);
			}
			delete tempwad;
			entry->unlock();
		}

		// Not a UDMF or Doom/Hexen map lump, go to next lump
		entry = entry->nextEntry();
	}

	// Set all map header entries to ETYPE_MAP type
	for (size_t a = 0; a < maps.size(); a++)
		if (!maps[a].archive)
			maps[a].head->setType(EntryType::mapMarkerType());

	// Update entry map format hints
	for (unsigned a = 0; a < maps.size(); a++)
	{
		string format;
		if (maps[a].format == MAP_DOOM)
			format = "doom";
		else if (maps[a].format == MAP_DOOM64)
			format = "doom64";
		else if (maps[a].format == MAP_HEXEN)
			format = "hexen";
		else
			format = "udmf";

		ArchiveEntry* entry = maps[a].head;
		while (entry && entry != maps[a].end->nextEntry())
		{
			entry->exProp("MapFormat") = format;
			entry = entry->nextEntry();
		}
	}

	return maps;
}

/* WadArchive::detectNamespace
 * Returns the namespace that [entry] is within
 *******************************************************************/
string WadArchive::detectNamespace(ArchiveEntry* entry)
{
	return detectNamespace(entryIndex(entry));
}

/* WadArchive::detectNamespace
 * Returns the namespace that the entry at [index] in [dir] is within
 *******************************************************************/
string WadArchive::detectNamespace(size_t index, ArchiveTreeNode * dir)
{
	// Go through namespaces
	for (unsigned a = 0; a < namespaces.size(); a++)
	{
		// Get namespace start and end indices
		size_t start = namespaces[a].start_index;
		size_t end = namespaces[a].end_index;

		// Check if the entry is within this namespace
		if (start <= index && index <= end)
			return namespaces[a].name;
	}

	// In no namespace
	return "global";
}

/* WadArchive::detectIncludes
 * Parses the DECORATE, GLDEFS, etc. lumps for included files, and
 * mark them as being of the same type
 *******************************************************************/
void WadArchive::detectIncludes()
{
	// DECORATE: #include "lumpname"
	// GLDEFS: #include "lumpname"
	// SBARINFO: #include "lumpname"
	// ZMAPINFO: translator = "lumpname"
	// EMAPINFO: extradata = lumpname
	// EDFROOT: lumpinclude("lumpname")

	const char * lumptypes[6]  = { "DECORATE", "GLDEFS", "SBARINFO", "ZMAPINFO", "EMAPINFO", "EDFROOT" };
	const char * entrytypes[6] = { "decorate", "gldefslump", "sbarinfo", "xlat", "extradata", "edf" };
	const char * tokens[6] = { "#include", "#include", "#include", "translator", "extradata", "lumpinclude" };
	Archive::search_options_t opt;
	opt.ignore_ext = true;
	Tokenizer tz;
	tz.setSpecialCharacters(";,:|={}/()");

	for (int i = 0; i < 6; ++i)
	{
		opt.match_name = lumptypes[i];
		vector<ArchiveEntry*> entries = findAll(opt);
		if (entries.size())
		{
			for (size_t j = 0; j < entries.size(); ++j)
			{
				tz.openMem(&entries[j]->getMCData(), lumptypes[i]);
				string token = tz.getToken();
				while (token.length())
				{
					if (token.Lower() == tokens[i])
					{
						if (i >= 3) // skip '=' or '('
							tz.getToken();
						string name = tz.getToken();
						if (i == 5) // skip ')'
							tz.getToken();
						opt.match_name = name;
						ArchiveEntry * entry = findFirst(opt);
						if (entry)
							entry->setType(EntryType::getType(entrytypes[i]));
					}
					else tz.skipLineComment();
					token = tz.getToken();
				}
			}
		}
	}
}

/* WadArchive::findFirst
 * Returns the first entry matching the search criteria in [options],
 * or NULL if no matching entry was found
 *******************************************************************/
ArchiveEntry* WadArchive::findFirst(search_options_t& options)
{
	// Init search variables
	ArchiveEntry* start = getEntry(0);
	ArchiveEntry* end = NULL;
	options.match_name = options.match_name.Lower();

	// "graphics" namespace is the global namespace in a wad
	if (options.match_namespace == "graphics")
		options.match_namespace = "";

	// Check for namespace to search
	if (!options.match_namespace.IsEmpty())
	{
		// Find matching namespace
		bool ns_found = false;
		for (unsigned a = 0; a < namespaces.size(); a++)
		{
			if (namespaces[a].name == options.match_namespace)
			{
				start = namespaces[a].start->nextEntry();
				end = namespaces[a].end;
				ns_found = true;
				break;
			}
		}

		// Return none if namespace not found
		if (!ns_found)
			return NULL;
	}

	// Begin search
	ArchiveEntry* entry = start;
	while (entry != end)
	{
		// Check type
		if (options.match_type)
		{
			if (entry->getType() == EntryType::unknownType())
			{
				if (!options.match_type->isThisType(entry))
				{
					entry = entry->nextEntry();
					continue;
				}
			}
			else if (options.match_type != entry->getType())
			{
				entry = entry->nextEntry();
				continue;
			}
		}

		// Check name
		if (!options.match_name.IsEmpty())
		{
			if (!options.match_name.Matches(entry->getName().Lower()))
			{
				entry = entry->nextEntry();
				continue;
			}
		}

		// Entry passed all checks so far, so we found a match
		return entry;
	}

	// No match found
	return NULL;
}

/* WadArchive::findLast
 * Returns the last entry matching the search criteria in [options],
 * or NULL if no matching entry was found
 *******************************************************************/
ArchiveEntry* WadArchive::findLast(search_options_t& options)
{
	// Init search variables
	ArchiveEntry* start = getEntry(numEntries()-1);
	ArchiveEntry* end = NULL;
	options.match_name = options.match_name.Lower();

	// "graphics" namespace is the global namespace in a wad
	if (options.match_namespace == "graphics")
		options.match_namespace = "";

	// "global" namespace has no name, by the way
	if (options.match_namespace == "global")
		options.match_namespace = "";

	// Check for namespace to search
	if (!options.match_namespace.IsEmpty())
	{
		// Find matching namespace
		bool ns_found = false;
		for (unsigned a = 0; a < namespaces.size(); a++)
		{
			if (namespaces[a].name == options.match_namespace)
			{
				start = namespaces[a].end->prevEntry();
				end = namespaces[a].start;
				ns_found = true;
				break;
			}
		}

		// Return none if namespace not found
		if (!ns_found)
			return NULL;
	}

	// Begin search
	ArchiveEntry* entry = start;
	while (entry != end)
	{
		// Check type
		if (options.match_type)
		{
			if (entry->getType() == EntryType::unknownType())
			{
				if (!options.match_type->isThisType(entry))
				{
					entry = entry->prevEntry();
					continue;
				}
			}
			else if (options.match_type != entry->getType())
			{
				entry = entry->prevEntry();
				continue;
			}
		}

		// Check name
		if (!options.match_name.IsEmpty())
		{
			if (!options.match_name.Matches(entry->getName().Lower()))
			{
				entry = entry->prevEntry();
				continue;
			}
		}

		// Entry passed all checks so far, so we found a match
		return entry;
	}

	// No match found
	return NULL;
}

/* WadArchive::findAll
 * Returns all entries matching the search criteria in [options]
 *******************************************************************/
vector<ArchiveEntry*> WadArchive::findAll(search_options_t& options)
{
	// Init search variables
	ArchiveEntry* start = getEntry(0);
	ArchiveEntry* end = NULL;
	options.match_name = options.match_name.Upper();
	vector<ArchiveEntry*> ret;

	// "graphics" namespace is the global namespace in a wad
	if (options.match_namespace == "graphics")
		options.match_namespace = "";

	// Check for namespace to search
	if (!options.match_namespace.IsEmpty())
	{
		// Find matching namespace
		bool ns_found = false;
		size_t namespaces_size = namespaces.size();
		for (unsigned a = 0; a < namespaces_size; a++)
		{
			if (namespaces[a].name == options.match_namespace)
			{
				start = namespaces[a].start->nextEntry();
				end = namespaces[a].end;
				ns_found = true;
				break;
			}
		}

		// Return none if namespace not found
		if (!ns_found)
			return ret;
	}

	ArchiveEntry* entry = start;
	while (entry != end)
	{
		// Check type
		if (options.match_type)
		{
			if (entry->getType() == EntryType::unknownType())
			{
				if (!options.match_type->isThisType(entry))
				{
					entry = entry->nextEntry();
					continue;
				}
			}
			else if (options.match_type != entry->getType())
			{
				entry = entry->nextEntry();
				continue;
			}
		}

		// Check name
		if (!options.match_name.IsEmpty())
		{
			if (!options.match_name.Matches(entry->getUpperName()))
			{
				entry = entry->nextEntry();
				continue;
			}
		}

		// Entry passed all checks so far, so we found a match
		ret.push_back(entry);
		entry = entry->nextEntry();
	}

	// Return search result
	return ret;
}



/* WadArchive::isWadArchive
 * Checks if the given data is a valid Doom wad archive
 *******************************************************************/
bool WadArchive::isWadArchive(MemChunk& mc)
{
	// Check size
	if (mc.getSize() < 12)
		return false;

	// Check for IWAD/PWAD header
	if (!(mc[1] == 'W' && mc[2] == 'A' && mc[3] == 'D' &&
	        (mc[0] == 'P' || mc[0] == 'I')))
		return false;

	// Get number of lumps and directory offset
	uint32_t num_lumps = 0;
	uint32_t dir_offset = 0;
	mc.seek(4, SEEK_SET);
	mc.read(&num_lumps, 4);
	mc.read(&dir_offset, 4);

	// Reset MemChunk (just in case)
	mc.seek(0, SEEK_SET);

	// Byteswap values for big endian if needed
	num_lumps = wxINT32_SWAP_ON_BE(num_lumps);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);

	// Check directory offset is decent
	if ((dir_offset + (num_lumps * 16)) > mc.getSize() ||
	        dir_offset < 12)
		return false;

	// If it's passed to here it's probably a wad file
	return true;
}

/* WadArchive::isWadArchive
 * Checks if the file at [filename] is a valid Doom wad archive
 *******************************************************************/
bool WadArchive::isWadArchive(string filename)
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
	if (!(header[1] == 'W' && header[2] == 'A' && header[3] == 'D' &&
	        (header[0] == 'P' || header[0] == 'I')))
		return false;

	// Get number of lumps and directory offset
	uint32_t num_lumps = 0;
	uint32_t dir_offset = 0;
	file.Read(&num_lumps, 4);
	file.Read(&dir_offset, 4);

	// Byteswap values for big endian if needed
	num_lumps = wxINT32_SWAP_ON_BE(num_lumps);
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);

	// Check directory offset is decent
	if ((dir_offset + (num_lumps * 16)) > file.Length() ||
	        dir_offset < 12)
		return false;

	// If it's passed to here it's probably a wad file
	return true;
}
