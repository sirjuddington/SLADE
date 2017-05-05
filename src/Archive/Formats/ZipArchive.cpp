
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ZipArchive.cpp
 * Description: ZipArchive, archive class to handle zip format
 *              archives
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
#include "App.h"
#include "ZipArchive.h"
#include "WadArchive.h"
#include "General/UI.h"


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, archive_load_data)


/*******************************************************************
 * ZIPARCHIVE CLASS FUNCTIONS
 *******************************************************************/

/* ZipArchive::ZipArchive
 * ZipArchive class constructor
 *******************************************************************/
ZipArchive::ZipArchive() : Archive(ARCHIVE_ZIP)
{
	desc.names_extensions = true;
	desc.supports_dirs = true;
}

/* ZipArchive::~ZipArchive
 * ZipArchive class destructor
 *******************************************************************/
ZipArchive::~ZipArchive()
{
	if (wxFileExists(temp_file))
		wxRemoveFile(temp_file);
}

/* ZipArchive::getFileExtensionString
 * Gets the wxWidgets file dialog filter string for the archive type
 *******************************************************************/
string ZipArchive::getFileExtensionString()
{
	return "Any Zip Format File (*.zip;*.pk3;*.pke;*.jdf)|*.zip;*.pk3;*.pke;*.jdf|Zip File (*.zip)|*.zip|Pk3 File (*.pk3)|*.pk3|Eternity Pke File (*.pke)|*.pke|JDF File (*.jdf)|*.jdf";
}

/* ZipArchive::getFormat
 * Returns the EntryDataFormat id of this archive type
 *******************************************************************/
string ZipArchive::getFormat()
{
	return "archive_zip";
}

/* ZipArchive::open
 * Reads zip data from a file
 * Returns true if successful, false otherwise
 *******************************************************************/
bool ZipArchive::open(string filename)
{
	// Copy the zip to a temp file (for use when saving)
	generateTempFileName(filename);
	wxCopyFile(filename, temp_file);

	// Open the file
	wxFFileInputStream in(filename);
	if (!in.IsOk())
	{
		Global::error = "Unable to open file";
		return false;
	}

	// Create zip stream
	wxZipInputStream zip(in);
	if (!zip.IsOk())
	{
		Global::error = "Invalid zip file";
		return false;
	}

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	setMuted(true);

	// Go through all zip entries
	int entry_index = 0;
	wxZipEntry* entry = zip.GetNextEntry();
	UI::setSplashProgressMessage("Reading zip data");
	while (entry)
	{
		UI::setSplashProgress(-1.0f);
		if (entry->GetMethod() != wxZIP_METHOD_DEFLATE && entry->GetMethod() != wxZIP_METHOD_STORE)
		{
			Global::error = "Unsupported zip compression method";
			setMuted(false);
			return false;
		}

		if (!entry->IsDir())
		{
			// Get the entry name as a wxFileName (so we can break it up)
			wxFileName fn(entry->GetName(wxPATH_UNIX), wxPATH_UNIX);

			// Create entry
			ArchiveEntry* new_entry = new ArchiveEntry(fn.GetFullName(), entry->GetSize());

			// Setup entry info
			new_entry->setLoaded(false);
			new_entry->exProp("ZipIndex") = entry_index;

			// Add entry and directory to directory tree
			ArchiveTreeNode* ndir = createDir(fn.GetPath(true, wxPATH_UNIX));
			ndir->addEntry(new_entry);
			//zipdir_t* ndir = addDirectory(fn.GetPath(true, wxPATH_UNIX));
			//ndir->entries.push_back(new_entry);

			// Read the data, if possible
			if (entry->GetSize() < 250 * 1024 * 1024)
			{
				uint8_t* data = new uint8_t[entry->GetSize()];
				zip.Read(data, entry->GetSize());	// Note: this is where exceedingly large files cause an exception.
				new_entry->importMem(data, entry->GetSize());
				new_entry->setLoaded(true);

				// Determine its type
				EntryType::detectEntryType(new_entry);

				// Unload data if needed
				if (!archive_load_data)
					new_entry->unloadData();

				// Clean up
				delete[] data;
			}
			else
			{
				Global::error = S_FMT("Entry too large: %s is %u mb",
				                      entry->GetName(wxPATH_UNIX), entry->GetSize() / (1<<20));
				setMuted(false);
				return false;
			}
		}
		else
		{
			// Zip entry is a directory, add it to the directory tree
			wxFileName fn(entry->GetName(wxPATH_UNIX), wxPATH_UNIX);
			createDir(fn.GetPath(true, wxPATH_UNIX));
			//addDirectory(fn.GetPath(true, wxPATH_UNIX));
		}

		// Go to next entry in the zip file
		delete entry;
		entry = zip.GetNextEntry();
		entry_index++;
	}
	UI::updateSplash();

	// Set all entries/directories to unmodified
	vector<ArchiveEntry*> entry_list;
	getEntryTreeAsList(entry_list);
	for (size_t a = 0; a < entry_list.size(); a++)
		entry_list[a]->setState(0);

	// Enable announcements
	setMuted(false);

	// Setup variables
	this->filename = filename;
	setModified(false);
	on_disk = true;

	UI::setSplashProgressMessage("");

	return true;
}

/* ZipArchive::open
 * Reads zip format data from a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool ZipArchive::open(MemChunk& mc)
{
	// Write the MemChunk to a temp file
	string tempfile = App::path("slade-temp-open.zip", App::Dir::Temp);
	mc.exportFile(tempfile);

	// Load the file
	bool success = open(tempfile);

	// Clean up
	wxRemoveFile(tempfile);

	return success;
}

/* ZipArchive::write
 * Writes the zip archive to a MemChunk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool ZipArchive::write(MemChunk& mc, bool update)
{
	bool success = false;

	// Write to a temporary file
	string tempfile = App::path("slade-temp-write.zip", App::Dir::Temp);
	if (write(tempfile, true))
	{
		// Load file into MemChunk
		success = mc.importFile(tempfile);
	}

	// Clean up
	wxRemoveFile(tempfile);

	return success;
}

/* ZipArchive::write
 * Writes the zip archive to a file
 * Returns true if successful, false otherwise
 *******************************************************************/
bool ZipArchive::write(string filename, bool update)
{
	// Open the file
	wxFFileOutputStream out(filename);
	if (!out.IsOk())
	{
		Global::error = "Unable to open file for saving. Make sure it isn't in use by another program.";
		return false;
	}

	// Open as zip for writing
	wxZipOutputStream zip(out, 9);
	if (!zip.IsOk())
	{
		Global::error = "Unable to create zip for saving";
		return false;
	}

	// Open old zip for copying, from the temp file that was copied on opening.
	// This is used to copy any entries that have been previously saved/compressed
	// and are unmodified, to greatly speed up zip file saving by not having to
	// recompress unchanged entries
	wxFFileInputStream in(temp_file);
	wxZipInputStream inzip(in);

	// Get a list of all entries in the old zip
	wxZipEntry** c_entries = new wxZipEntry*[inzip.GetTotalEntries()];
	for (int a = 0; a < inzip.GetTotalEntries(); a++)
		c_entries[a] = inzip.GetNextEntry();

	// Get a linear list of all entries in the archive
	vector<ArchiveEntry*> entries;
	getEntryTreeAsList(entries);

	// Go through all entries
	for (size_t a = 0; a < entries.size(); a++)
	{
		if (entries[a]->getType() == EntryType::folderType())
		{
			// If the current entry is a folder, just write a directory entry and continue
			zip.PutNextDirEntry(entries[a]->getPath(true));
			if (update) entries[a]->setState(0);
			continue;
		}

		// Get entry zip index
		int index = -1;
		if (entries[a]->exProps().propertyExists("ZipIndex"))
			index = entries[a]->exProp("ZipIndex");

		if (!inzip.IsOk() || entries[a]->getState() > 0 || index < 0 || index >= inzip.GetTotalEntries())
		{
			// If the current entry has been changed, or doesn't exist in the old zip,
			// (re)compress its data and write it to the zip
			wxZipEntry* zipentry = new wxZipEntry(entries[a]->getPath() + entries[a]->getName());
			zip.PutNextEntry(zipentry);
			zip.Write(entries[a]->getData(), entries[a]->getSize());
		}
		else
		{
			// If the entry is unmodified and exists in the old zip, just copy it over
			c_entries[index]->SetName(entries[a]->getPath() + entries[a]->getName());
			zip.CopyEntry(c_entries[index], inzip);
			inzip.Reset();
		}

		// Update entry info
		if (update)
		{
			entries[a]->setState(0);
			entries[a]->exProp("ZipIndex") = (int)a;
		}
	}

	// Clean up
	delete[] c_entries;
	zip.Close();
	out.Close();

	// Update the temp file
	if (temp_file.IsEmpty())
		generateTempFileName(filename);
	wxCopyFile(filename, temp_file);

	return true;
}

/* ZipArchive::loadEntryData
 * Loads an entry's data from the saved copy of the archive if any.
 * Returns false if the entry is invalid, doesn't belong to the
 * archive or doesn't exist in the saved copy, true otherwise.
 *******************************************************************/
bool ZipArchive::loadEntryData(ArchiveEntry* entry)
{
	// Check that the entry belongs to this archive
	if (entry->getParent() != this)
	{
		LOG_MESSAGE(1, "ZipArchive::loadEntryData: Entry %s attempting to load data from wrong parent!", entry->getName());
		return false;
	}

	// Do nothing if the entry's size is zero,
	// or if it has already been loaded
	if (entry->getSize() == 0 || entry->isLoaded())
	{
		entry->setLoaded();
		return true;
	}

	// Check that the entry has a zip index
	int zip_index = 0;
	if (entry->exProps().propertyExists("ZipIndex"))
		zip_index = entry->exProp("ZipIndex");
	else
	{
		LOG_MESSAGE(1, "ZipArchive::loadEntryData: Entry %s has no zip entry index!", entry->getName());
		return false;
	}

	// Open the file
	wxFFileInputStream in(filename);
	if (!in.IsOk())
	{
		LOG_MESSAGE(1, "ZipArchive::loadEntryData: Unable to open zip file \"%s\"!", filename);
		return false;
	}

	// Create zip stream
	wxZipInputStream zip(in);
	if (!zip.IsOk())
	{
		LOG_MESSAGE(1, "ZipArchive::loadEntryData: Invalid zip file \"%s\"!", filename);
		return false;
	}

	// Lock entry state
	entry->lockState();

	// Skip to correct entry in zip
	wxZipEntry* zentry = zip.GetNextEntry();
	for (long a = 0; a < zip_index; a++)
	{
		delete zentry;
		zentry = zip.GetNextEntry();
	}

	// Abort if entry doesn't exist in zip (some kind of error)
	if (!zentry)
	{
		LOG_MESSAGE(1, "Error: ZipEntry for entry \"%s\" does not exist in zip", entry->getName());
		return false;
	}

	// Read the data
	uint8_t* data = new uint8_t[zentry->GetSize()];
	zip.Read(data, zentry->GetSize());
	entry->importMem(data, zentry->GetSize());

	// Set the entry to loaded
	entry->setLoaded();
	entry->unlockState();

	// Clean up
	delete[] data;
	delete zentry;

	return true;
}

/* ZipArchive::addEntry
 * Adds [entry] to the end of the namespace matching [add_namespace].
 * If [copy] is true a copy of the entry is added. Returns the added
 * entry or NULL if the entry is invalid
 *
 * In a zip archive, a namespace is simply a first-level directory,
 * ie <root>/<namespace>
 *******************************************************************/
ArchiveEntry* ZipArchive::addEntry(ArchiveEntry* entry, string add_namespace, bool copy)
{
	// Check namespace
	if (add_namespace.IsEmpty() || add_namespace == "global")
		return Archive::addEntry(entry, 0xFFFFFFFF, NULL, copy);

	// Get/Create namespace dir
	ArchiveTreeNode* dir = createDir(add_namespace.Lower());

	// Add the entry to the dir
	return Archive::addEntry(entry, 0xFFFFFFFF, dir, copy);
}

/* ZipArchive::getMapInfo
 * Returns the mapdesc_t information about the map at [entry], if
 * [entry] is actually a valid map (ie. a wad archive in the maps
 * folder)
 *******************************************************************/
Archive::mapdesc_t ZipArchive::getMapInfo(ArchiveEntry* entry)
{
	mapdesc_t map;

	// Check entry
	if (!checkEntry(entry))
		return map;

	// Check entry type
	if (entry->getType()->getFormat() != "archive_wad")
		return map;

	// Check entry directory
	if (entry->getParentDir()->getParent() != getRoot() || entry->getParentDir()->getName() != "maps")
		return map;

	// Setup map info
	map.archive = true;
	map.head = entry;
	map.end = entry;
	map.name = entry->getName(true).Upper();

	return map;
}

/* ZipArchive::detectMaps
 * Detects all the maps in the archive and returns a vector of
 * information about them.
 *******************************************************************/
vector<Archive::mapdesc_t> ZipArchive::detectMaps()
{
	vector<mapdesc_t> ret;

	// Get the maps directory
	ArchiveTreeNode* mapdir = getDir("maps");
	if (!mapdir)
		return ret;

	// Go through entries in map dir
	for (unsigned a = 0; a < mapdir->numEntries(); a++)
	{
		ArchiveEntry* entry = mapdir->getEntry(a);

		// Maps can only be wad archives
		if (entry->getType()->getFormat() != "archive_wad")
			continue;

		// Detect map format (probably kinda slow but whatever, no better way to do it really)
		int format = MAP_UNKNOWN;
		Archive* tempwad = new WadArchive();
		tempwad->open(entry);
		vector<mapdesc_t> emaps = tempwad->detectMaps();
		if (emaps.size() > 0)
			format = emaps[0].format;
		delete tempwad;

		// Add map description
		mapdesc_t md;
		md.head = entry;
		md.end = entry;
		md.archive = true;
		md.name = entry->getName(true).Upper();
		md.format = format;
		ret.push_back(md);
	}

	return ret;
}

/* ZipArchive::findFirst
 * Returns the first entry matching the search criteria in [options],
 * or NULL if no matching entry was found
 *******************************************************************/
ArchiveEntry* ZipArchive::findFirst(search_options_t& options)
{
	// Init search variables
	ArchiveTreeNode* dir = getRoot();
	options.match_name = options.match_name.Lower();

	// Check for search directory (overrides namespace)
	if (options.dir)
	{
		dir = options.dir;
	}
	// Check for namespace
	else if (!options.match_namespace.IsEmpty())
	{
		dir = getDir(options.match_namespace);

		// If the requested namespace doesn't exist, return nothing
		if (!dir)
			return NULL;
		else
			options.search_subdirs = true;	// Namespace search always includes namespace subdirs
	}

	// Do default search
	search_options_t opt = options;
	opt.dir = dir;
	opt.match_namespace = "";
	return Archive::findFirst(opt);
}

/* ZipArchive::findLast
 * Returns the last entry matching the search criteria in [options],
 * or NULL if no matching entry was found
 *******************************************************************/
ArchiveEntry* ZipArchive::findLast(search_options_t& options)
{
	// Init search variables
	ArchiveTreeNode* dir = getRoot();
	options.match_name = options.match_name.Lower();

	// Check for search directory (overrides namespace)
	if (options.dir)
	{
		dir = options.dir;
	}
	// Check for namespace
	else if (!options.match_namespace.IsEmpty())
	{
		dir = getDir(options.match_namespace);

		// If the requested namespace doesn't exist, return nothing
		if (!dir)
			return NULL;
		else
			options.search_subdirs = true;	// Namespace search always includes namespace subdirs
	}

	// Do default search
	search_options_t opt = options;
	opt.dir = dir;
	opt.match_namespace = "";
	return Archive::findLast(opt);
}

/* ZipArchive::findAll
 * Returns all entries matching the search criteria in [options]
 *******************************************************************/
vector<ArchiveEntry*> ZipArchive::findAll(search_options_t& options)
{
	// Init search variables
	ArchiveTreeNode* dir = getRoot();
	options.match_name = options.match_name.Lower();
	vector<ArchiveEntry*> ret;

	// Check for search directory (overrides namespace)
	if (options.dir)
	{
		dir = options.dir;
	}
	// Check for namespace
	else if (!options.match_namespace.IsEmpty())
	{
		dir = getDir(options.match_namespace);

		// If the requested namespace doesn't exist, return nothing
		if (!dir)
			return ret;
		else
			options.search_subdirs = true;	// Namespace search always includes namespace subdirs
	}

	// Do default search
	search_options_t opt = options;
	opt.dir = dir;
	opt.match_namespace = "";
	return Archive::findAll(opt);
}

/* ZipArchive::generateTempFileName
 * Generates the temp file path to use, from [filename]. The temp
 * file will be in the configured temp folder
 *******************************************************************/
void ZipArchive::generateTempFileName(string filename)
{
	wxFileName tfn(filename);
	temp_file = App::path(tfn.GetFullName(), App::Dir::Temp);
	if (wxFileExists(temp_file))
	{
		// Make sure we don't overwrite an existing temp file
		// (in case there are multiple zips open with the same name)
		int n = 1;
		while (1)
		{
			temp_file = App::path(S_FMT("%s.%d", CHR(tfn.GetFullName()), n), App::Dir::Temp);
			if (!wxFileExists(temp_file))
				break;

			n++;
		}
	}
}


// Struct representing a zip file header
struct zip_file_header_t
{
	uint32_t	sig;
	uint16_t	version;
	uint16_t	flag;
	uint16_t	compression;
	uint16_t	mod_time;
	uint16_t	mod_date;
	uint32_t	crc;
	uint32_t	size_comp;
	uint32_t	size_orig;
	uint16_t	len_fn;
	uint16_t	len_extra;
};

/* ZipArchive::isZipArchive
 * Checks if the given data is a valid zip archive
 *******************************************************************/
bool ZipArchive::isZipArchive(MemChunk& mc)
{
	// Check size
	if (mc.getSize() < sizeof(zip_file_header_t))
		return false;

	// Read first file header
	zip_file_header_t header;
	mc.seek(0, SEEK_SET);
	mc.read(&header, sizeof(zip_file_header_t));

	// Check header signature
	if (header.sig != 0x04034b50)
		return false;

	// The zip format is horrendous, so this will do for checking
	return true;
}

/* ZipArchive::isZipArchive
 * Checks if the file at [filename] is a valid zip archive
 *******************************************************************/
bool ZipArchive::isZipArchive(string filename)
{
	// Open the file for reading
	wxFile file(filename);

	// Check it opened
	if (!file.IsOpened())
		return false;

	// Read first file header
	zip_file_header_t header;
	file.Read(&header, sizeof(zip_file_header_t));

	// Check header signature
	if (header.sig != 0x04034b50)
		return false;

	// The zip format is horrendous, so this will do for checking
	return true;
}
