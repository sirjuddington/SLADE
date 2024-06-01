
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    DirArchiveHandler.cpp
// Description: ArchiveFormatHandler that opens a directory and treats it as an
//              archive. All entry data is still stored in memory and only
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
#include "DirArchiveHandler.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/EntryType/EntryType.h"
#include "Archive/MapDesc.h"
#include "UI/UI.h"
#include "Utility/FileUtils.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, archive_dir_ignore_hidden, true, CVar::Save)


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Int, max_entry_size_mb)


// -----------------------------------------------------------------------------
//
// DirArchiveHandler Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// DirArchiveHandler class constructor
// -----------------------------------------------------------------------------
DirArchiveHandler::DirArchiveHandler() :
	ArchiveFormatHandler(ArchiveFormat::Dir),
	ignore_hidden_{ archive_dir_ignore_hidden }
{
	// Setup separator character
#ifdef WIN32
	separator_ = '\\';
#else
	separator_ = '/';
#endif
}

// -----------------------------------------------------------------------------
// Reads files from the directory [filename] into the archive
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool DirArchiveHandler::open(Archive& archive, string_view filename)
{
	ui::setSplashProgressMessage("Reading directory structure");
	ui::setSplashProgress(0);
	vector<string>      files, dirs;
	DirArchiveTraverser traverser(files, dirs, ignore_hidden_);
	const wxDir         dir(string{ filename });
	dir.Traverse(traverser, "", wxDIR_FILES | wxDIR_DIRS);

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	const ArchiveModSignalBlocker sig_blocker{ archive };

	ui::setSplashProgressMessage("Reading files");
	auto n_files = files.size();
	for (unsigned a = 0; a < n_files; a++)
	{
		ui::setSplashProgress(a, n_files);

		// Cut off directory to get entry name + relative path
		auto name = files[a];
		name.erase(0, filename.size());
		if (strutil::startsWith(name, separator_))
			name.erase(0, 1);

		// log::info(3, fn.GetPath(true, wxPATH_UNIX));

		// Create entry
		auto fn        = strutil::Path{ name };
		auto new_entry = std::make_shared<ArchiveEntry>(fn.fileName());

		// Setup entry info
		new_entry->exProp("filePath") = files[a];

		// Add entry and directory to directory tree
		auto ndir = createDir(archive, fn.path());
		ndir->addEntry(new_entry);
		ndir->dirEntry()->exProp("filePath") = fmt::format("{}{}", filename, fn.path());

		// Read entry data
		if (!new_entry->importFile(files[a]))
			return false;

		file_modification_times_[new_entry.get()] = fileutil::fileModifiedTime(files[a]);

		// Detect entry type
		EntryType::detectEntryType(*new_entry);
	}

	// Add empty directories
	for (const auto& subdir : dirs)
	{
		auto name = subdir;
		name.erase(0, filename.size());
		strutil::removePrefixIP(name, separator_);
		std::replace(name.begin(), name.end(), '\\', '/');

		const auto ndir                      = createDir(archive, name);
		ndir->dirEntry()->exProp("filePath") = subdir;
	}

	// Set all entries/directories to unmodified
	vector<ArchiveEntry*> entry_list;
	archive.putEntryTreeAsList(entry_list);
	for (auto& entry : entry_list)
		entry->setState(EntryState::Unmodified);

	// Enable announcements
	sig_blocker.unblock();

	archive.setModified(false);

	ui::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Reads an archive from an ArchiveEntry (not implemented)
// -----------------------------------------------------------------------------
bool DirArchiveHandler::open(Archive& archive, ArchiveEntry* entry)
{
	global::error = "Cannot open Folder Archive from entry";
	return false;
}

// -----------------------------------------------------------------------------
// Reads data from a MemChunk (not implemented)
// -----------------------------------------------------------------------------
bool DirArchiveHandler::open(Archive& archive, const MemChunk& mc)
{
	global::error = "Cannot open Folder Archive from memory";
	return false;
}

// -----------------------------------------------------------------------------
// Writes the archive to a MemChunk (not implemented)
// -----------------------------------------------------------------------------
bool DirArchiveHandler::write(Archive& archive, MemChunk& mc)
{
	global::error = "Cannot write Folder Archive to memory";
	return false;
}

// -----------------------------------------------------------------------------
// Writes the archive to a file (not implemented)
// -----------------------------------------------------------------------------
bool DirArchiveHandler::write(Archive& archive, string_view filename)
{
	return true;
}

// -----------------------------------------------------------------------------
// Saves any changes to the directory to the file system
// -----------------------------------------------------------------------------
bool DirArchiveHandler::save(Archive& archive, string_view filename)
{
	save_errors_ = false;

	// Get flat entry list
	vector<ArchiveEntry*> entries;
	archive.putEntryTreeAsList(entries);

	// Get entry path list
	vector<string> entry_paths;
	for (auto& entry : entries)
	{
		entry_paths.push_back(archive.filename() + entry->path(true));
		if (separator_ != '/')
			std::replace(entry_paths.back().begin(), entry_paths.back().end(), '/', separator_);
	}

	// Get current directory structure
	long                time = app::runTimer();
	vector<string>      files, dirs;
	DirArchiveTraverser traverser(files, dirs, archive_dir_ignore_hidden);
	const wxDir         dir(archive.filename());
	dir.Traverse(traverser, "", wxDIR_FILES | wxDIR_DIRS);
	log::info(2, "GetAllFiles took {}ms", app::runTimer() - time);

	// Check for any files to remove
	time = app::runTimer();
	for (const auto& removed_file : removed_files_)
	{
		if (fileutil::fileExists(removed_file))
		{
			log::info(2, "Removing file {}", removed_file);
			if (!fileutil::removeFile(removed_file))
				save_errors_ = true;
		}
	}

	// Check for any directories to remove
	for (int a = static_cast<int>(dirs.size()) - 1; a >= 0; a--)
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
		if (!found)
		{
			if (!fileutil::removeDir(dirs[a]))
				save_errors_ = true;
		}
	}
	log::info(2, "Remove check took {}ms", app::runTimer() - time);

	// Go through entries
	vector<string> files_written;
	for (unsigned a = 0; a < entries.size(); a++)
	{
		// Check for folder
		auto path = entry_paths[a];
		if (entries[a]->type() == EntryType::folderType())
		{
			// Create if needed
			if (!fileutil::dirExists(path))
			{
				if (!fileutil::createDir(path))
					save_errors_ = true;
			}

			// Set unmodified
			entries[a]->exProp("filePath") = path;
			entries[a]->setState(EntryState::Unmodified);

			continue;
		}

		// Check if entry needs to be (re)written
		if (entries[a]->state() == EntryState::Unmodified && entries[a]->exProps().contains("filePath")
			&& path == entries[a]->exProp<string>("filePath"))
			continue;

		// Write entry to file
		if (!entries[a]->exportFile(path))
		{
			log::error("Unable to save entry {}: {}", entries[a]->name(), global::error);
			save_errors_ = true;
		}
		else
			files_written.push_back(path);

		// Set unmodified
		entries[a]->setState(EntryState::Unmodified);
		entries[a]->exProp("filePath")       = path;
		file_modification_times_[entries[a]] = fileutil::fileModifiedTime(path);
	}

	removed_files_.clear();
	archive.setModified(false);
	archive.signals().saved(archive);

	return true;
}

// -----------------------------------------------------------------------------
// Loads an entry's data from the saved copy of the archive if any
// -----------------------------------------------------------------------------
bool DirArchiveHandler::loadEntryData(Archive& archive, const ArchiveEntry* entry, MemChunk& out)
{
	auto file_path = entry->exProps().getOr<string>("filePath", "");

	if (out.importFile(file_path))
	{
		file_modification_times_[const_cast<ArchiveEntry*>(entry)] = fileutil::fileModifiedTime(file_path);
		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Deletes the directory matching [path], starting from [base]. If [base] is
// null, the root directory is used.
// Returns false if the directory does not exist, true otherwise
//
// For DirArchiveHandler also adds all subdirs and entries to the removed files list,
// so they are ignored when checking for changes on disk
// -----------------------------------------------------------------------------
shared_ptr<ArchiveDir> DirArchiveHandler::removeDir(Archive& archive, string_view path, ArchiveDir* base)
{
	// Get the dir to remove
	const auto dir = archive.dirAtPath(path, base);

	// Check it exists (and that it isn't the root dir)
	if (!dir || dir == archive.rootDir().get())
		return nullptr;

	// Get all entries in the directory (and subdirectories)
	vector<ArchiveEntry*> entries;
	archive.putEntryTreeAsList(entries, dir);

	// Add to removed files list
	for (auto& entry : entries)
	{
		if (!entry->exProps().contains("filePath"))
			continue;

		log::info(2, entry->exProp<string>("filePath"));
		removed_files_.push_back(entry->exProp<string>("filePath"));
	}

	// Do normal dir remove
	return ArchiveFormatHandler::removeDir(archive, path, base);
}

// -----------------------------------------------------------------------------
// Renames [dir] to [new_name].
// Returns false if [dir] isn't part of the archive, true otherwise
// -----------------------------------------------------------------------------
bool DirArchiveHandler::renameDir(Archive& archive, ArchiveDir* dir, string_view new_name)
{
	auto path = dir->parent()->path();
	if (separator_ != '/')
		std::replace(path.begin(), path.end(), '/', separator_);
	const StringPair rename(path + dir->name(), fmt::format("{}{}", path, new_name));
	renamed_dirs_.push_back(rename);

	if (!ArchiveFormatHandler::renameDir(archive, dir, new_name))
	{
		// Rename failed (it can't currently but just to be sure)
		renamed_dirs_.pop_back();
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------
// Adds [entry] to the end of the namespace matching [add_namespace]. If [copy]
// is true a copy of the entry is added.
// Returns the added entry or NULL if the entry is invalid
//
// Namespaces in a folder are treated the same way as a zip archive
// -----------------------------------------------------------------------------
shared_ptr<ArchiveEntry> DirArchiveHandler::addEntry(
	Archive&                 archive,
	shared_ptr<ArchiveEntry> entry,
	string_view              add_namespace)
{
	// Check namespace
	if (add_namespace.empty() || add_namespace == "global")
		return ArchiveFormatHandler::addEntry(archive, entry, 0xFFFFFFFF, nullptr);

	// Get/Create namespace dir
	const auto dir = createDir(archive, strutil::lower(add_namespace));

	// Add the entry to the dir
	return ArchiveFormatHandler::addEntry(archive, entry, 0xFFFFFFFF, dir.get());
}

// -----------------------------------------------------------------------------
// Removes [entry] from the archive.
// If [delete_entry] is true, the entry will also be deleted.
// Returns true if the removal succeeded
// -----------------------------------------------------------------------------
bool DirArchiveHandler::removeEntry(Archive& archive, ArchiveEntry* entry, bool set_deleted)
{
	if (entry->exProps().contains("filePath"))
	{
		// If it exists on disk we need to update removed_files_
		const auto old_name = entry->exProp<string>("filePath");
		const bool success  = ArchiveFormatHandler::removeEntry(archive, entry, set_deleted);
		if (success)
			removed_files_.push_back(old_name);
		return success;
	}

	// Not on disk, just do default remove
	return ArchiveFormatHandler::removeEntry(archive, entry, set_deleted);
}

// -----------------------------------------------------------------------------
// Renames [entry].  Returns true if the rename succeeded
// -----------------------------------------------------------------------------
bool DirArchiveHandler::renameEntry(Archive& archive, ArchiveEntry* entry, string_view name, bool force)
{
	// Check if entry exists on disk
	if (entry->exProps().contains("filePath"))
	{
		const auto old_name = entry->exProp<string>("filePath");
		const bool success  = ArchiveFormatHandler::renameEntry(archive, entry, name, force);
		if (success)
			removed_files_.push_back(old_name);
		return success;
	}

	return ArchiveFormatHandler::renameEntry(archive, entry, name);
}

// -----------------------------------------------------------------------------
// Returns the mapdesc_t information about the map at [entry], if [entry] is
// actually a valid map (ie. a wad archive in the maps folder)
// -----------------------------------------------------------------------------
MapDesc DirArchiveHandler::mapDesc(Archive& archive, ArchiveEntry* entry)
{
	MapDesc map;

	// Check entry
	if (!archive.checkEntry(entry))
		return map;

	// Check entry type
	if (entry->type()->formatId() != "archive_wad")
		return map;

	// Check entry directory
	if (entry->parentDir()->parent() != archive.rootDir() || entry->parentDir()->name() != "maps")
		return map;

	// Setup map info
	map.archive = true;
	map.head    = entry->getShared();
	map.end     = entry->getShared();
	map.name    = entry->upperNameNoExt();

	return map;
}

// -----------------------------------------------------------------------------
// Detects all the maps in the archive and returns a vector of information about
// them.
// -----------------------------------------------------------------------------
vector<MapDesc> DirArchiveHandler::detectMaps(Archive& archive)
{
	vector<MapDesc> ret;

	// Get the maps directory
	const auto mapdir = archive.dirAtPath("maps");
	if (!mapdir)
		return ret;

	// Go through entries in map dir
	for (unsigned a = 0; a < mapdir->numEntries(); a++)
	{
		auto entry = mapdir->sharedEntryAt(a);

		// Maps can only be wad archives
		if (entry->type()->formatId() != "archive_wad")
			continue;

		// Detect map format (probably kinda slow but whatever, no better way to do it really)
		auto    format = MapFormat::Unknown;
		Archive tempwad(ArchiveFormat::Wad);
		tempwad.open(entry->data());
		auto emaps = tempwad.detectMaps();
		if (!emaps.empty())
			format = emaps[0].format;

		// Add map description
		MapDesc md;
		md.head    = entry;
		md.end     = entry;
		md.archive = true;
		md.name    = entry->upperNameNoExt();
		md.format  = format;
		ret.push_back(md);
	}

	return ret;
}

// -----------------------------------------------------------------------------
// Returns the first entry matching the search criteria in [options], or null if
// no matching entry was found
// -----------------------------------------------------------------------------
ArchiveEntry* DirArchiveHandler::findFirst(Archive& archive, ArchiveSearchOptions& options)
{
	// Init search variables
	auto dir = archive.rootDir().get();

	// Check for search directory (overrides namespace)
	if (options.dir)
	{
		dir = options.dir;
	}
	// Check for namespace
	else if (!options.match_namespace.empty())
	{
		dir = archive.dirAtPath(options.match_namespace);

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
	return ArchiveFormatHandler::findFirst(archive, opt);
}

// -----------------------------------------------------------------------------
// Returns the last entry matching the search criteria in [options], or null if
// no matching entry was found
// -----------------------------------------------------------------------------
ArchiveEntry* DirArchiveHandler::findLast(Archive& archive, ArchiveSearchOptions& options)
{
	// Init search variables
	auto dir = archive.rootDir().get();

	// Check for search directory (overrides namespace)
	if (options.dir)
	{
		dir = options.dir;
	}
	// Check for namespace
	else if (!options.match_namespace.empty())
	{
		dir = archive.dirAtPath(options.match_namespace);

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
	return ArchiveFormatHandler::findLast(archive, opt);
}

// -----------------------------------------------------------------------------
// Returns all entries matching the search criteria in [options]
// -----------------------------------------------------------------------------
vector<ArchiveEntry*> DirArchiveHandler::findAll(Archive& archive, ArchiveSearchOptions& options)
{
	// Init search variables
	auto dir = archive.rootDir().get();

	// Check for search directory (overrides namespace)
	if (options.dir)
	{
		dir = options.dir;
	}
	// Check for namespace
	else if (!options.match_namespace.empty())
	{
		dir = archive.dirAtPath(options.match_namespace);

		// If the requested namespace doesn't exist, return nothing
		if (!dir)
			return {};
		else
			options.search_subdirs = true; // Namespace search always includes namespace subdirs
	}

	// Do default search
	auto opt            = options;
	opt.dir             = dir;
	opt.match_namespace = "";
	return ArchiveFormatHandler::findAll(archive, opt);
}

// -----------------------------------------------------------------------------
// Remember to ignore the given files until they change again
// -----------------------------------------------------------------------------
void DirArchiveHandler::ignoreChangedEntries(const vector<DirEntryChange>& changes)
{
	for (auto& change : changes)
		ignored_file_changes_[change.file_path] = change;
}

// -----------------------------------------------------------------------------
// Updates entries/directories based on [changes] list
// -----------------------------------------------------------------------------
void DirArchiveHandler::updateChangedEntries(Archive& archive, vector<DirEntryChange>& changes)
{
	const bool was_modified = archive.isModified();

	for (auto& change : changes)
	{
		ignored_file_changes_.erase(change.file_path);

		// Modified Entries
		if (change.action == DirEntryChange::Action::Updated)
		{
			auto entry = archive.entryAtPath(change.entry_path);
			entry->importFile(change.file_path);
			EntryType::detectEntryType(*entry);
			file_modification_times_[entry] = fileutil::fileModifiedTime(change.file_path);
		}

		// Deleted Entries
		else if (change.action == DirEntryChange::Action::DeletedFile)
		{
			// If the parent directory was already removed, this entry no longer exists
			if (const auto entry = archive.entryAtPath(change.entry_path))
				removeEntry(archive, entry);
		}

		// Deleted Directories
		else if (change.action == DirEntryChange::Action::DeletedDir)
			removeDir(archive, change.entry_path);

		// New Directory
		else if (change.action == DirEntryChange::Action::AddedDir)
		{
			auto name = change.file_path;
			name.erase(0, archive.filename().size());
			strutil::removePrefixIP(name, separator_);
			std::replace(name.begin(), name.end(), '\\', '/');

			const auto ndir = createDir(archive, name);
			ndir->dirEntry()->setState(EntryState::Unmodified);
			ndir->dirEntry()->exProp("filePath") = change.file_path;
		}

		// New Entry
		else if (change.action == DirEntryChange::Action::AddedFile)
		{
			auto name = change.file_path;
			name.erase(0, archive.filename().size());
			if (strutil::startsWith(name, separator_))
				name.erase(0, 1);
			std::replace(name.begin(), name.end(), '\\', '/');

			// Create entry
			strutil::Path fn(name);
			auto          new_entry = std::make_shared<ArchiveEntry>(fn.fileName());

			// Setup entry info
			new_entry->exProp("filePath") = change.file_path;

			// Add entry and directory to directory tree
			auto ndir = createDir(archive, fn.path());
			ArchiveFormatHandler::addEntry(archive, new_entry, -1, ndir.get());

			// Read entry data
			new_entry->importFile(change.file_path);

			file_modification_times_[new_entry.get()] = fileutil::fileModifiedTime(change.file_path);

			// Detect entry type
			EntryType::detectEntryType(*new_entry);

			// Set entry not modified
			new_entry->setState(EntryState::Unmodified);
		}
	}

	// Preserve old modified state
	archive.setModified(was_modified);
}

// -----------------------------------------------------------------------------
// Returns true iff the user has previously indicated no interest in this change
// -----------------------------------------------------------------------------
bool DirArchiveHandler::shouldIgnoreEntryChange(const DirEntryChange& change)
{
	const auto it = ignored_file_changes_.find(change.file_path);
	// If we've never seen this file before, definitely don't ignore the change
	if (it == ignored_file_changes_.end())
		return false;

	const auto old_change = it->second;
	const bool was_deleted =
		(old_change.action == DirEntryChange::Action::DeletedFile
		 || old_change.action == DirEntryChange::Action::DeletedDir);
	const bool is_deleted =
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


// -----------------------------------------------------------------------------
//
// DirArchiveTraverser Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// DirArchiveTraverser class constructor
// -----------------------------------------------------------------------------
DirArchiveTraverser::DirArchiveTraverser(vector<string>& pathlist, vector<string>& dirlist, bool ignore_hidden) :
	paths_{ pathlist },
	dirs_{ dirlist },
	ignore_hidden_{ ignore_hidden }
{
}

// -----------------------------------------------------------------------------
// Called when a file is found during traversal
// -----------------------------------------------------------------------------
wxDirTraverseResult DirArchiveTraverser::OnFile(const wxString& filename)
{
	auto path_str = filename.ToStdString();

	if (ignore_hidden_ && strutil::startsWith(strutil::Path::fileNameOf(path_str), '.'))
		return wxDIR_CONTINUE;

	paths_.push_back(path_str);
	return wxDIR_CONTINUE;
}

// -----------------------------------------------------------------------------
// Called when a directory is found during traversal
// -----------------------------------------------------------------------------
wxDirTraverseResult DirArchiveTraverser::OnDir(const wxString& dirname)
{
	if (ignore_hidden_)
	{
		auto path_str = dirname.ToStdString();
		std::replace(path_str.begin(), path_str.end(), '\\', '/');
		auto dir = strutil::afterLastV(path_str, '/');
		if (strutil::startsWith(dir, '.'))
			return wxDIR_IGNORE;
	}

	dirs_.push_back(dirname.ToStdString());
	return wxDIR_CONTINUE;
}
