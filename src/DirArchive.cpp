
/*******************************************************************
* SLADE - It's a Doom Editor
* Copyright (C) 2008-2014 Simon Judd
*
* Email:       sirjuddington@gmail.com
* Web:         http://slade.mancubus.net
* Filename:    DirArchive.cpp
* Description: DirArchive, archive class that opens a directory
*              and treats it as an archive. All entry data is still
*              stored in memory and only written to the file system
*              when saving the 'archive'
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
#include "DirArchive.h"
#include "SplashWindow.h"
#include "WadArchive.h"
#include "MainApp.h"
#include <wx/dir.h>
#include <wx/filename.h>


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, archive_load_data)


/*******************************************************************
 * DIRARCHIVE CLASS FUNCTIONS
 *******************************************************************/

/* DirArchive::DirArchive
 * DirArchive class constructor
 *******************************************************************/
DirArchive::DirArchive() : Archive(ARCHIVE_FOLDER)
{
	// Setup separator character
#ifdef WIN32
	separator = "\\";
#else
	separator = "/";
#endif
}

/* DirArchive::~DirArchive
 * DirArchive class destructor
 *******************************************************************/
DirArchive::~DirArchive()
{
}

/* DirArchive::getFileExtensionString
 * Gets the wxWidgets file dialog filter string for the archive type
 *******************************************************************/
string DirArchive::getFileExtensionString()
{
	return "";
}

/* DirArchive::getFormat
 * Returns the EntryDataFormat id of this archive type
 *******************************************************************/
string DirArchive::getFormat()
{
	return "";
}

/* DirArchive::open
 * Reads files from the directory [filename] into the archive
 * Returns true if successful, false otherwise
 *******************************************************************/
bool DirArchive::open(string filename)
{
	theSplashWindow->setProgressMessage("Reading directory structure");
	theSplashWindow->setProgress(0);
	wxArrayString files;
	wxDir::GetAllFiles(filename, &files, wxEmptyString, wxDIR_FILES|wxDIR_DIRS);

	theSplashWindow->setProgressMessage("Reading files");
	for (unsigned a = 0; a < files.size(); a++)
	{
		theSplashWindow->setProgress((float)a / (float)files.size());

		// Cut off directory to get entry name + relative path
		string name = files[a];
		name.Remove(0, filename.Length());
		if (name.StartsWith(separator))
			name.Remove(0, 1);

		//LOG_MESSAGE(3, fn.GetPath(true, wxPATH_UNIX));

		// Create entry
		wxFileName fn(name);
		ArchiveEntry* new_entry = new ArchiveEntry(fn.GetFullName());

		// Setup entry info
		new_entry->setLoaded(false);
		new_entry->exProp("filePath") = files[a];

		// Add entry and directory to directory tree
		ArchiveTreeNode* ndir = createDir(fn.GetPath(true, wxPATH_UNIX));
		ndir->addEntry(new_entry);

		// Read entry data
		new_entry->importFile(files[a]);
		new_entry->setLoaded(true);

		time_t modtime = wxFileModificationTime(files[a]);

		// Detect entry type
		EntryType::detectEntryType(new_entry);

		// Unload data if needed
		if (!archive_load_data)
			new_entry->unloadData();
	}

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

	theSplashWindow->setProgressMessage("");

	return true;
}

/* DirArchive::open
 * Reads an archive from an ArchiveEntry (not implemented)
 *******************************************************************/
bool DirArchive::open(ArchiveEntry* entry)
{
	Global::error = "Cannot open Folder Archive from entry";
	return false;
}

/* DirArchive::open
 * Reads data from a MemChunk (not implemented)
 *******************************************************************/
bool DirArchive::open(MemChunk& mc)
{
	Global::error = "Cannot open Folder Archive from memory";
	return false;
}

/* DirArchive::write
 * Writes the archive to a MemChunk (not implemented)
 *******************************************************************/
bool DirArchive::write(MemChunk& mc, bool update)
{
	Global::error = "Cannot write Folder Archive to memory";
	return false;
}

/* DirArchive::write
 * Writes the archive to a file (not implemented)
 *******************************************************************/
bool DirArchive::write(string filename, bool update)
{
	return true;
}

/* DirArchive::save
 * Saves any changes to the directory to the file system
 *******************************************************************/
bool DirArchive::save(string filename)
{
	// Get flat entry list
	vector<ArchiveEntry*> entries;
	getEntryTreeAsList(entries);

	// Get entry path list
	vector<string> entry_paths;
	for (unsigned a = 0; a < entries.size(); a++)
	{
		entry_paths.push_back(this->filename + entries[a]->getPath(true));
		if (separator != "/") entry_paths.back().Replace("/", separator);
	}

	// Get current directory structure
	long time = theApp->runTimer();
	wxArrayString files;
	wxDir::GetAllFiles(this->filename, &files, wxEmptyString, wxDIR_FILES|wxDIR_DIRS);
	LOG_MESSAGE(2, "GetAllFiles took %dms", theApp->runTimer() - time);

	// Check for any files to remove
	time = theApp->runTimer();
	for (unsigned a = 0; a < files.size(); a++)
	{
		// Check if filename matches an existing entry
		bool found = false;
		for (unsigned e = 0; e < entry_paths.size(); e++)
		{
			if (files[a] == entry_paths[e])
			{
				found = true;
				break;
			}
		}

		// File on disk isn't part of the archive in memory
		// (eg. has been removed or renamed/deleted)
		if (!found)
		{
			LOG_MESSAGE(2, "Removing file %s", files[a]);
			wxRemoveFile(files[a]);
		}
	}
	LOG_MESSAGE(2, "Remove check took %dms", theApp->runTimer() - time);

	// Go through entries
	vector<string> files_written;
	for (unsigned a = 0; a < entries.size(); a++)
	{
		// Check for folder
		string path = entry_paths[a];
		if (entries[a]->getType() == EntryType::folderType())
		{
			// Create if needed
			if (!wxDirExists(path))
				wxMkDir(path);

			// Set unmodified
			entries[a]->setState(0);

			continue;
		}

		// Check if entry needs to be (re)written
		if (entries[a]->getState() == 0 && path == entries[a]->exProp("filePath"))
			continue;

		// Write entry to file
		if (!entries[a]->exportFile(path))
			LOG_MESSAGE(1, "Unable to save entry %s: %s", entries[a]->getName(), Global::error);
		else
			files_written.push_back(path);

		// Set unmodified
		entries[a]->setState(0);
		entries[a]->exProp("filePath") = path;
	}

	setModified(false);

	return true;
}

/* DirArchive::loadEntryData
 * Loads an entry's data from the saved copy of the archive if any
 *******************************************************************/
bool DirArchive::loadEntryData(ArchiveEntry* entry)
{
	return entry->importFile(entry->exProp("filePath").getStringValue());
}

/* DirArchive::renameDir
 * Renames [dir] to [new_name]. Returns false if [dir] isn't part of
 * the archive, true otherwise
 *******************************************************************/
bool DirArchive::renameDir(ArchiveTreeNode* dir, string new_name)
{
	string path = dir->getParent()->getPath();
	if (separator != "/") path.Replace("/", separator);
	key_value_t rename(path + dir->getName(), path + new_name);
	renamed_dirs.push_back(rename);
	LOG_MESSAGE(2, "RENAME %s to %s", rename.key, rename.value);

	return Archive::renameDir(dir, new_name);
}

/* DirArchive::addEntry
 * Adds [entry] to the end of the namespace matching [add_namespace].
 * If [copy] is true a copy of the entry is added. Returns the added
 * entry or NULL if the entry is invalid
 *
 * Namespaces in a folder are treated the same way as a zip archive
 *******************************************************************/
ArchiveEntry* DirArchive::addEntry(ArchiveEntry* entry, string add_namespace, bool copy)
{
	// Check namespace
	if (add_namespace.IsEmpty() || add_namespace == "global")
		return Archive::addEntry(entry, 0xFFFFFFFF, NULL, copy);

	// Get/Create namespace dir
	ArchiveTreeNode* dir = createDir(add_namespace.Lower());

	// Add the entry to the dir
	return Archive::addEntry(entry, 0xFFFFFFFF, dir, copy);
}

/* DirArchive::getMapInfo
 * Returns the mapdesc_t information about the map at [entry], if
 * [entry] is actually a valid map (ie. a wad archive in the maps
 * folder)
 *******************************************************************/
Archive::mapdesc_t DirArchive::getMapInfo(ArchiveEntry* entry)
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

/* DirArchive::detectMaps
 * Detects all the maps in the archive and returns a vector of
 * information about them.
 *******************************************************************/
vector<Archive::mapdesc_t> DirArchive::detectMaps()
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

/* DirArchive::detectNamespace
 * Returns the namespace that [entry] is within
 *******************************************************************/
string DirArchive::detectNamespace(ArchiveEntry* entry)
{
	// Check entry
	if (!checkEntry(entry))
		return "global";

	// If the entry is in the root dir, it's in the global namespace
	if (entry->getParentDir() == getRoot())
		return "global";

	// Get the entry's *first* parent directory after root (ie <root>/namespace/)
	ArchiveTreeNode* dir = entry->getParentDir();
	while (dir && dir->getParent() != getRoot())
		dir = (ArchiveTreeNode*)dir->getParent();

	// Namespace is the directory's name (in lowercase)
	if (dir)
		return dir->getName().Lower();
	else
		return "global"; // Error, just return global
}

/* DirArchive::findFirst
 * Returns the first entry matching the search criteria in [options],
 * or NULL if no matching entry was found
 *******************************************************************/
ArchiveEntry* DirArchive::findFirst(search_options_t& options)
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

/* DirArchive::findLast
 * Returns the last entry matching the search criteria in [options],
 * or NULL if no matching entry was found
 *******************************************************************/
ArchiveEntry* DirArchive::findLast(search_options_t& options)
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

/* DirArchive::findAll
 * Returns all entries matching the search criteria in [options]
 *******************************************************************/
vector<ArchiveEntry*> DirArchive::findAll(search_options_t& options)
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
