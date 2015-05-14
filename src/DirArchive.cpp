
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


class DirArchiveTraverser : public wxDirTraverser
{
private:
	vector<string>&	paths;
	vector<string>&	dirs;

public:
	DirArchiveTraverser(vector<string>& pathlist, vector<string>& dirlist) : paths(pathlist), dirs(dirlist) {}
	~DirArchiveTraverser() {}

	virtual wxDirTraverseResult OnFile(const wxString& filename)
	{
		paths.push_back(filename);
		return wxDIR_CONTINUE;
	}

	virtual wxDirTraverseResult OnDir(const wxString& dirname)
	{
		dirs.push_back(dirname);
		return wxDIR_CONTINUE;
	}
};


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
	//wxArrayString files;
	//wxDir::GetAllFiles(filename, &files, wxEmptyString, wxDIR_FILES|wxDIR_DIRS);
	vector<string> files, dirs;
	DirArchiveTraverser traverser(files, dirs);
	wxDir dir(filename);
	dir.Traverse(traverser, "", wxDIR_FILES | wxDIR_DIRS);

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
		file_modification_times[new_entry] = modtime;

		// Detect entry type
		EntryType::detectEntryType(new_entry);

		// Unload data if needed
		if (!archive_load_data)
			new_entry->unloadData();
	}

	// Add empty directories
	for (unsigned a = 0; a < dirs.size(); a++)
	{
		string name = dirs[a];
		name.Remove(0, filename.Length());
		if (name.StartsWith(separator))
			name.Remove(0, 1);
		name.Replace("\\", "/");
		createDir(name);
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
	vector<string> files, dirs;
	DirArchiveTraverser traverser(files, dirs);
	wxDir dir(this->filename);
	dir.Traverse(traverser, "", wxDIR_FILES|wxDIR_DIRS);
	//wxDir::GetAllFiles(this->filename, &files, wxEmptyString, wxDIR_FILES|wxDIR_DIRS);
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

	// Check for any directories to remove
	for (int a = dirs.size() - 1; a >= 0; a--)
	{
		// Check if dir path matches an existing dir
		bool found = false;
		for (unsigned e = 0; e < entry_paths.size(); e++)
		{
			if (dirs[a] == entry_paths[e])
			{
				found = true;
				break;
			}
		}

		// Dir on disk isn't part of the archive in memory
		if (!found)
		{
			LOG_MESSAGE(2, "Removing directory %s", dirs[a]);
			wxRmDir(dirs[a]);
		}
	}

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
				wxMkdir(path);

			// Set unmodified
			entries[a]->setState(0);

			continue;
		}

		// Check if entry needs to be (re)written
		if (entries[a]->getState() == 0 && path == entries[a]->exProp("filePath").getStringValue())
			continue;

		// Write entry to file
		if (!entries[a]->exportFile(path))
		{
			LOG_MESSAGE(1, "Unable to save entry %s: %s", entries[a]->getName(), Global::error);
		}
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
	if (entry->importFile(entry->exProp("filePath").getStringValue()))
	{
		file_modification_times[entry] = wxFileModificationTime(entry->exProp("filePath").getStringValue());
		return true;
	}

	return false;
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

/* DirArchive::checkUpdatedFiles
 * Checks if any entries/folders have been changed on disk, adds any
 * detected changes to [changes]
 *******************************************************************/
void DirArchive::checkUpdatedFiles(vector<dir_entry_change_t>& changes)
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
	vector<string> files, dirs;
	DirArchiveTraverser traverser(files, dirs);
	wxDir dir(this->filename);
	dir.Traverse(traverser, "", wxDIR_FILES | wxDIR_DIRS);

	// Check for deleted files
	for (unsigned a = 0; a < entry_paths.size(); a++)
	{
		if (entries[a]->getType() == EntryType::folderType())
		{
			if (!wxDirExists(entry_paths[a]))
				changes.push_back(dir_entry_change_t(dir_entry_change_t::DELETED_DIR, entry_paths[a], entries[a]->getPath(true)));
		}
		else
		{
			if (!wxFileExists(entry_paths[a]))
				changes.push_back(dir_entry_change_t(dir_entry_change_t::DELETED_FILE, entry_paths[a], entries[a]->getPath(true)));
		}
	}

	// Check for new/updated files
	for (unsigned a = 0; a < files.size(); a++)
	{
		// Find file in archive
		ArchiveEntry * entry = NULL;
		for (unsigned b = 0; b < entry_paths.size(); b++)
		{
			if (entry_paths[b] == files[a])
			{
				entry = entries[b];
				break;
			}
		}

		// No match, added to archive
		if (!entry)
			changes.push_back(dir_entry_change_t(dir_entry_change_t::ADDED_FILE, files[a]));
		else
		{
			// Matched, check modification time
			time_t mod = wxFileModificationTime(files[a]);
			if (mod > file_modification_times[entry])
				changes.push_back(dir_entry_change_t(dir_entry_change_t::UPDATED, files[a], entry->getPath(true)));
		}
	}

	// Check for new dirs
	for (unsigned a = 0; a < dirs.size(); a++)
	{
		//LOG_MESSAGE(3, dirs[a]);

		// Find dir in archive
		ArchiveEntry * entry = NULL;
		for (unsigned b = 0; b < entry_paths.size(); b++)
		{
			if (entry_paths[b] == dirs[a])
			{
				entry = entries[b];
				break;
			}
		}

		// No match, added to archive
		if (!entry)
			changes.push_back(dir_entry_change_t(dir_entry_change_t::ADDED_DIR, dirs[a]));
	}
}

/* DirArchive::updateChangedEntries
 * Updates entries/directories based on [changes] list
 *******************************************************************/
void DirArchive::updateChangedEntries(vector<dir_entry_change_t>& changes)
{
	// Modified Entries
	for (unsigned a = 0; a < changes.size(); a++)
	{
		if (changes[a].action == dir_entry_change_t::UPDATED)
		{
			ArchiveEntry* entry = entryAtPath(changes[a].entry_path);
			entry->importFile(changes[a].file_path);
			EntryType::detectEntryType(entry);
			file_modification_times[entry] = wxFileModificationTime(changes[a].file_path);
		}
	}

	// Deleted Entries
	for (unsigned a = 0; a < changes.size(); a++)
	{
		if (changes[a].action == dir_entry_change_t::DELETED_FILE)
			removeEntry(entryAtPath(changes[a].entry_path));
	}

	// Deleted Directories
	for (unsigned a = 0; a < changes.size(); a++)
	{
		if (changes[a].action == dir_entry_change_t::DELETED_DIR)
			removeDir(changes[a].entry_path);
	}

	// Added Entries/Directories
	for (unsigned a = 0; a < changes.size(); a++)
	{
		// New Directory
		if (changes[a].action == dir_entry_change_t::ADDED_DIR)
		{
			string name = changes[a].file_path;
			name.Remove(0, filename.Length());
			if (name.StartsWith(separator))
				name.Remove(0, 1);
			name.Replace("\\", "/");

			ArchiveTreeNode* ndir = createDir(name);
			ndir->getDirEntry()->setState(0);
		}

		// New Entry
		else if (changes[a].action == dir_entry_change_t::ADDED_FILE)
		{
			string name = changes[a].file_path;
			name.Remove(0, filename.Length());
			if (name.StartsWith(separator))
				name.Remove(0, 1);
			name.Replace("\\", "/");

			// Create entry
			wxFileName fn(name);
			ArchiveEntry* new_entry = new ArchiveEntry(fn.GetFullName());

			// Setup entry info
			new_entry->setLoaded(false);
			new_entry->exProp("filePath") = changes[a].file_path;

			// Add entry and directory to directory tree
			ArchiveTreeNode* ndir = createDir(fn.GetPath(true, wxPATH_UNIX));
			ndir->addEntry(new_entry);

			// Read entry data
			new_entry->importFile(changes[a].file_path);
			new_entry->setLoaded(true);

			time_t modtime = wxFileModificationTime(changes[a].file_path);
			file_modification_times[new_entry] = modtime;

			// Detect entry type
			EntryType::detectEntryType(new_entry);

			// Unload data if needed
			if (!archive_load_data)
				new_entry->unloadData();

			// Set entry not modified
			new_entry->setState(0);
		}
	}
}
