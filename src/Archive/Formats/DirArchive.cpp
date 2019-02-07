
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    DirArchive.cpp
// Description: DirArchive, archive class that opens a directory and treats it
//              as an archive. All entry data is still stored in memory and only
//              written to the file system when saving the 'archive'
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
#include "DirArchive.h"
#include "App.h"
#include "General/UI.h"
#include "WadArchive.h"


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, archive_load_data)


// -----------------------------------------------------------------------------
//
// DirArchive Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// DirArchive class constructor
// -----------------------------------------------------------------------------
DirArchive::DirArchive() : Archive("folder")
{
	// Setup separator character
#ifdef WIN32
	separator_ = "\\";
#else
	separator_ = "/";
#endif

	rootDir()->allowDuplicateNames(false);
}

// -----------------------------------------------------------------------------
// Reads files from the directory [filename] into the archive
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool DirArchive::open(const wxString& filename)
{
	UI::setSplashProgressMessage("Reading directory structure");
	UI::setSplashProgress(0);
	vector<wxString>    files, dirs;
	DirArchiveTraverser traverser(files, dirs);
	wxDir               dir(filename);
	dir.Traverse(traverser, "", wxDIR_FILES | wxDIR_DIRS);

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	setMuted(true);

	UI::setSplashProgressMessage("Reading files");
	for (unsigned a = 0; a < files.size(); a++)
	{
		UI::setSplashProgress((float)a / (float)files.size());

		// Cut off directory to get entry name + relative path
		wxString name = files[a];
		name.Remove(0, filename.Length());
		if (name.StartsWith(separator_))
			name.Remove(0, 1);

		// Log::info(3, fn.GetPath(true, wxPATH_UNIX));

		// Create entry
		wxFileName fn(name);
		auto       new_entry = std::make_shared<ArchiveEntry>(fn.GetFullName());

		// Setup entry info
		new_entry->setLoaded(false);
		new_entry->exProp("filePath") = files[a];

		// Add entry and directory to directory tree
		auto ndir = createDir(fn.GetPath(true, wxPATH_UNIX));
		ndir->addEntry(new_entry);
		ndir->dirEntry()->exProp("filePath") = filename + fn.GetPath(true, wxPATH_UNIX);

		// Read entry data
		new_entry->importFile(files[a]);
		new_entry->setLoaded(true);

		file_modification_times_[new_entry.get()] = wxFileModificationTime(files[a]);

		// Detect entry type
		EntryType::detectEntryType(new_entry.get());

		// Unload data if needed
		if (!archive_load_data)
			new_entry->unloadData();
	}

	// Add empty directories
	for (const auto& subdir : dirs)
	{
		wxString name = subdir;
		name.Remove(0, filename.Length());
		if (name.StartsWith(separator_))
			name.Remove(0, 1);
		name.Replace("\\", "/");

		auto ndir                            = createDir(name);
		ndir->dirEntry()->exProp("filePath") = subdir;
	}

	// Set all entries/directories to unmodified
	vector<ArchiveEntry*> entry_list;
	putEntryTreeAsList(entry_list);
	for (auto& entry : entry_list)
		entry->setState(ArchiveEntry::State::Unmodified);

	// Enable announcements
	setMuted(false);

	// Setup variables
	this->filename_ = filename;
	setModified(false);
	on_disk_ = true;

	UI::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Reads an archive from an ArchiveEntry (not implemented)
// -----------------------------------------------------------------------------
bool DirArchive::open(ArchiveEntry* entry)
{
	Global::error = "Cannot open Folder Archive from entry";
	return false;
}

// -----------------------------------------------------------------------------
// Reads data from a MemChunk (not implemented)
// -----------------------------------------------------------------------------
bool DirArchive::open(MemChunk& mc)
{
	Global::error = "Cannot open Folder Archive from memory";
	return false;
}

// -----------------------------------------------------------------------------
// Writes the archive to a MemChunk (not implemented)
// -----------------------------------------------------------------------------
bool DirArchive::write(MemChunk& mc, bool update)
{
	Global::error = "Cannot write Folder Archive to memory";
	return false;
}

// -----------------------------------------------------------------------------
// Writes the archive to a file (not implemented)
// -----------------------------------------------------------------------------
bool DirArchive::write(const wxString& filename, bool update)
{
	return true;
}

// -----------------------------------------------------------------------------
// Saves any changes to the directory to the file system
// -----------------------------------------------------------------------------
bool DirArchive::save(const wxString& filename)
{
	// Get flat entry list
	vector<ArchiveEntry*> entries;
	putEntryTreeAsList(entries);

	// Get entry path list
	vector<wxString> entry_paths;
	for (auto& entry : entries)
	{
		entry_paths.push_back(this->filename_ + entry->path(true));
		if (separator_ != "/")
			entry_paths.back().Replace("/", separator_);
	}

	// Get current directory structure
	long                time = App::runTimer();
	vector<wxString>    files, dirs;
	DirArchiveTraverser traverser(files, dirs);
	wxDir               dir(this->filename_);
	dir.Traverse(traverser, "", wxDIR_FILES | wxDIR_DIRS);
	Log::info(2, wxString::Format("GetAllFiles took %lums", App::runTimer() - time));

	// Check for any files to remove
	time = App::runTimer();
	for (const auto& removed_file : removed_files_)
	{
		if (wxFileExists(removed_file))
		{
			Log::info(2, wxString::Format("Removing file %s", removed_file));
			wxRemoveFile(removed_file);
		}
	}

	// Check for any directories to remove
	for (int a = dirs.size() - 1; a >= 0; a--)
	{
		// Check if dir path matches an existing dir
		bool found = false;
		for (const auto& entry_path : entry_paths)
		{
			if (dirs[a] == entry_path)
			{
				found = true;
				break;
			}
		}

		// Dir on disk isn't part of the archive in memory
		// (Note that this will fail if there are any untracked files in the
		// directory)
		if (!found && wxRmdir(dirs[a]))
			Log::info(2, wxString::Format("Removing directory %s", dirs[a]));
	}
	Log::info(2, wxString::Format("Remove check took %lums", App::runTimer() - time));

	// Go through entries
	vector<wxString> files_written;
	for (unsigned a = 0; a < entries.size(); a++)
	{
		// Check for folder
		wxString path = entry_paths[a];
		if (entries[a]->type() == EntryType::folderType())
		{
			// Create if needed
			if (!wxDirExists(path))
				wxMkdir(path);

			// Set unmodified
			entries[a]->exProp("filePath") = path;
			entries[a]->setState(ArchiveEntry::State::Unmodified);

			continue;
		}

		// Check if entry needs to be (re)written
		if (entries[a]->state() == ArchiveEntry::State::Unmodified
			&& path == entries[a]->exProp("filePath").stringValue())
			continue;

		// Write entry to file
		if (!entries[a]->exportFile(path))
			Log::error(wxString::Format("Unable to save entry %s: %s", entries[a]->name(), Global::error));
		else
			files_written.push_back(path);

		// Set unmodified
		entries[a]->setState(ArchiveEntry::State::Unmodified);
		entries[a]->exProp("filePath")       = path;
		file_modification_times_[entries[a]] = wxFileModificationTime(path);
	}

	removed_files_.clear();
	setModified(false);

	return true;
}

// -----------------------------------------------------------------------------
// Loads an entry's data from the saved copy of the archive if any
// -----------------------------------------------------------------------------
bool DirArchive::loadEntryData(ArchiveEntry* entry)
{
	if (entry->importFile(entry->exProp("filePath").stringValue()))
	{
		file_modification_times_[entry] = wxFileModificationTime(entry->exProp("filePath").stringValue());
		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Deletes the directory matching [path], starting from [base]. If [base] is
// null, the root directory is used.
// Returns false if the directory does not exist, true otherwise
//
// For DirArchive also adds all subdirs and entries to the removed files list,
// so they are ignored when checking for changes on disk
// -----------------------------------------------------------------------------
bool DirArchive::removeDir(const wxString& path, ArchiveTreeNode* base)
{
	// Abort if read only
	if (read_only_)
		return false;

	// Get the dir to remove
	auto dir = this->dir(path, base);

	// Check it exists (and that it isn't the root dir)
	if (!dir || dir == rootDir())
		return false;

	// Get all entries in the directory (and subdirectories)
	vector<ArchiveEntry*> entries;
	putEntryTreeAsList(entries, dir);

	// Add to removed files list
	for (auto& entry : entries)
	{
		Log::info(2, entry->exProp("filePath").stringValue());
		removed_files_.push_back(entry->exProp("filePath").stringValue());
	}

	// Do normal dir remove
	return Archive::removeDir(path, base);
}

// -----------------------------------------------------------------------------
// Renames [dir] to [new_name].
// Returns false if [dir] isn't part of the archive, true otherwise
// -----------------------------------------------------------------------------
bool DirArchive::renameDir(ArchiveTreeNode* dir, const wxString& new_name)
{
	wxString path = dir->parent()->path();
	if (separator_ != "/")
		path.Replace("/", separator_);
	StringPair rename(path + dir->name(), path + new_name);
	renamed_dirs_.push_back(rename);
	Log::info(2, wxString::Format("RENAME %s to %s", rename.first, rename.second));

	return Archive::renameDir(dir, new_name);
}

// -----------------------------------------------------------------------------
// Adds [entry] to the end of the namespace matching [add_namespace]. If [copy]
// is true a copy of the entry is added.
// Returns the added entry or NULL if the entry is invalid
//
// Namespaces in a folder are treated the same way as a zip archive
// -----------------------------------------------------------------------------
ArchiveEntry* DirArchive::addEntry(ArchiveEntry* entry, const wxString& add_namespace, bool copy)
{
	// Check namespace
	if (add_namespace.IsEmpty() || add_namespace == "global")
		return Archive::addEntry(entry, 0xFFFFFFFF, nullptr, copy);

	// Get/Create namespace dir
	auto dir = createDir(add_namespace.Lower());

	// Add the entry to the dir
	return Archive::addEntry(entry, 0xFFFFFFFF, dir, copy);
}

// -----------------------------------------------------------------------------
// Removes [entry] from the archive.
// If [delete_entry] is true, the entry will also be deleted.
// Returns true if the removal succeeded
// -----------------------------------------------------------------------------
bool DirArchive::removeEntry(ArchiveEntry* entry)
{
	wxString old_name = entry->exProp("filePath").stringValue();
	bool     success  = Archive::removeEntry(entry);
	if (success)
		removed_files_.push_back(old_name);
	return success;
}

// -----------------------------------------------------------------------------
// Renames [entry].  Returns true if the rename succeeded
// -----------------------------------------------------------------------------
bool DirArchive::renameEntry(ArchiveEntry* entry, const wxString& name)
{
	// Check rename won't result in duplicated name
	if (entry->parentDir()->entry(name))
	{
		Global::error = wxString::Format("An entry named %s already exists", CHR(name));
		return false;
	}

	wxString old_name = entry->exProp("filePath").stringValue();
	bool     success  = Archive::renameEntry(entry, name);
	if (success)
		removed_files_.push_back(old_name);
	return success;
}

// -----------------------------------------------------------------------------
// Returns the mapdesc_t information about the map at [entry], if [entry] is
// actually a valid map (ie. a wad archive in the maps folder)
// -----------------------------------------------------------------------------
Archive::MapDesc DirArchive::mapDesc(ArchiveEntry* entry)
{
	MapDesc map;

	// Check entry
	if (!checkEntry(entry))
		return map;

	// Check entry type
	if (entry->type()->formatId() != "archive_wad")
		return map;

	// Check entry directory
	if (entry->parentDir()->parent() != rootDir() || entry->parentDir()->name() != "maps")
		return map;

	// Setup map info
	map.archive = true;
	map.head    = entry;
	map.end     = entry;
	map.name    = entry->name(true).Upper();

	return map;
}

// -----------------------------------------------------------------------------
// Detects all the maps in the archive and returns a vector of information about
// them.
// -----------------------------------------------------------------------------
vector<Archive::MapDesc> DirArchive::detectMaps()
{
	vector<MapDesc> ret;

	// Get the maps directory
	auto mapdir = dir("maps");
	if (!mapdir)
		return ret;

	// Go through entries in map dir
	for (unsigned a = 0; a < mapdir->numEntries(); a++)
	{
		auto entry = mapdir->entryAt(a);

		// Maps can only be wad archives
		if (entry->type()->formatId() != "archive_wad")
			continue;

		// Detect map format (probably kinda slow but whatever, no better way to do it really)
		auto       format = MapFormat::Unknown;
		WadArchive tempwad;
		tempwad.open(entry->data());
		auto emaps = tempwad.detectMaps();
		if (!emaps.empty())
			format = emaps[0].format;

		// Add map description
		MapDesc md;
		md.head    = entry;
		md.end     = entry;
		md.archive = true;
		md.name    = entry->name(true).Upper();
		md.format  = format;
		ret.push_back(md);
	}

	return ret;
}

// -----------------------------------------------------------------------------
// Returns the first entry matching the search criteria in [options], or null if
// no matching entry was found
// -----------------------------------------------------------------------------
ArchiveEntry* DirArchive::findFirst(SearchOptions& options)
{
	// Init search variables
	auto dir           = rootDir();
	options.match_name = options.match_name.Lower();

	// Check for search directory (overrides namespace)
	if (options.dir)
	{
		dir = options.dir;
	}
	// Check for namespace
	else if (!options.match_namespace.IsEmpty())
	{
		dir = this->dir(options.match_namespace);

		// If the requested namespace doesn't exist, return nothing
		if (!dir)
			return nullptr;
		else
			options.search_subdirs = true; // Namespace search always includes namespace subdirs
	}

	// Do default search
	auto opt            = options;
	opt.dir             = dir;
	opt.match_namespace = "";
	return Archive::findFirst(opt);
}

// -----------------------------------------------------------------------------
// Returns the last entry matching the search criteria in [options], or null if
// no matching entry was found
// -----------------------------------------------------------------------------
ArchiveEntry* DirArchive::findLast(SearchOptions& options)
{
	// Init search variables
	auto dir           = rootDir();
	options.match_name = options.match_name.Lower();

	// Check for search directory (overrides namespace)
	if (options.dir)
	{
		dir = options.dir;
	}
	// Check for namespace
	else if (!options.match_namespace.IsEmpty())
	{
		dir = this->dir(options.match_namespace);

		// If the requested namespace doesn't exist, return nothing
		if (!dir)
			return nullptr;
		else
			options.search_subdirs = true; // Namespace search always includes namespace subdirs
	}

	// Do default search
	auto opt            = options;
	opt.dir             = dir;
	opt.match_namespace = "";
	return Archive::findLast(opt);
}

// -----------------------------------------------------------------------------
// Returns all entries matching the search criteria in [options]
// -----------------------------------------------------------------------------
vector<ArchiveEntry*> DirArchive::findAll(SearchOptions& options)
{
	// Init search variables
	auto dir           = rootDir();
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
		dir = this->dir(options.match_namespace);

		// If the requested namespace doesn't exist, return nothing
		if (!dir)
			return ret;
		else
			options.search_subdirs = true; // Namespace search always includes namespace subdirs
	}

	// Do default search
	auto opt            = options;
	opt.dir             = dir;
	opt.match_namespace = "";
	return Archive::findAll(opt);
}

// -----------------------------------------------------------------------------
// Remember to ignore the given files until they change again
// -----------------------------------------------------------------------------
void DirArchive::ignoreChangedEntries(vector<DirEntryChange>& changes)
{
	for (auto& change : changes)
		ignored_file_changes_[change.file_path] = change;
}

// -----------------------------------------------------------------------------
// Updates entries/directories based on [changes] list
// -----------------------------------------------------------------------------
void DirArchive::updateChangedEntries(vector<DirEntryChange>& changes)
{
	bool was_modified = isModified();

	for (auto& change : changes)
	{
		ignored_file_changes_.erase(change.file_path);

		// Modified Entries
		if (change.action == DirEntryChange::Action::Updated)
		{
			auto entry = entryAtPath(change.entry_path);
			entry->importFile(change.file_path);
			EntryType::detectEntryType(entry);
			file_modification_times_[entry] = wxFileModificationTime(change.file_path);
		}

		// Deleted Entries
		else if (change.action == DirEntryChange::Action::DeletedFile)
		{
			auto entry = entryAtPath(change.entry_path);
			// If the parent directory was already removed, this entry no longer exists
			if (entry)
				removeEntry(entry);
		}

		// Deleted Directories
		else if (change.action == DirEntryChange::Action::DeletedDir)
			removeDir(change.entry_path);

		// New Directory
		else if (change.action == DirEntryChange::Action::AddedDir)
		{
			wxString name = change.file_path;
			name.Remove(0, filename_.Length());
			if (name.StartsWith(separator_))
				name.Remove(0, 1);
			name.Replace("\\", "/");

			auto ndir = createDir(name);
			ndir->dirEntry()->setState(ArchiveEntry::State::Unmodified);
			ndir->dirEntry()->exProp("filePath") = change.file_path;
		}

		// New Entry
		else if (change.action == DirEntryChange::Action::AddedFile)
		{
			wxString name = change.file_path;
			name.Remove(0, filename_.Length());
			if (name.StartsWith(separator_))
				name.Remove(0, 1);
			name.Replace("\\", "/");

			// Create entry
			wxFileName fn(name);
			auto       new_entry = new ArchiveEntry(fn.GetFullName());

			// Setup entry info
			new_entry->setLoaded(false);
			new_entry->exProp("filePath") = change.file_path;

			// Add entry and directory to directory tree
			auto ndir = createDir(fn.GetPath(true, wxPATH_UNIX));
			ndir->addEntry(new_entry);

			// Read entry data
			new_entry->importFile(change.file_path);
			new_entry->setLoaded(true);

			time_t modtime                      = wxFileModificationTime(change.file_path);
			file_modification_times_[new_entry] = modtime;

			// Detect entry type
			EntryType::detectEntryType(new_entry);

			// Unload data if needed
			if (!archive_load_data)
				new_entry->unloadData();

			// Set entry not modified
			new_entry->setState(ArchiveEntry::State::Unmodified);
		}
	}

	// Preserve old modified state
	setModified(was_modified);
}

// -----------------------------------------------------------------------------
// Returns true iff the user has previously indicated no interest in this change
// -----------------------------------------------------------------------------
bool DirArchive::shouldIgnoreEntryChange(DirEntryChange& change)
{
	auto it = ignored_file_changes_.find(change.file_path);
	// If we've never seen this file before, definitely don't ignore the change
	if (it == ignored_file_changes_.end())
		return false;

	auto old_change = it->second;
	bool was_deleted =
		(old_change.action == DirEntryChange::Action::DeletedFile
		 || old_change.action == DirEntryChange::Action::DeletedDir);
	bool is_deleted =
		(change.action == DirEntryChange::Action::DeletedFile || change.action == DirEntryChange::Action::DeletedDir);

	// Was deleted, is still deleted, nothing's changed
	if (was_deleted && is_deleted)
		return true;

	// Went from deleted to not, or vice versa; interesting
	if (was_deleted != is_deleted)
		return false;

	// Otherwise, it was modified both times, which is only interesting if the
	// mtime is different.  (You might think it's interesting if the mtime is
	// /greater/, but this is more robust against changes to the system clock,
	// and an unmodified file will never change mtime.)
	return (old_change.mtime == change.mtime);
}
