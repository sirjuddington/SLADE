
#include "Main.h"
#include "ArchiveFormatHandler.h"
#include "Archive.h"
#include "ArchiveDir.h"
#include "ArchiveEntry.h"
#include "ArchiveFormat.h"
#include "EntryType/EntryType.h"
#include "Formats/All.h"
#include "General/UndoRedo.h"
#include "Utility/FileUtils.h"
#include "Utility/Named.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
vector<Named<ArchiveFormat>> formats = { { "adat", ArchiveFormat::ADat }, { "bsp", ArchiveFormat::Bsp },
										 { "bz2", ArchiveFormat::Bz2 },   { "chasm_bin", ArchiveFormat::ChasmBin },
										 { "dat", ArchiveFormat::Dat },   { "folder", ArchiveFormat::Dir },
										 { "disk", ArchiveFormat::Disk }, { "gob", ArchiveFormat::Gob },
										 { "grp", ArchiveFormat::Grp },   { "gzip", ArchiveFormat::GZip },
										 { "hog", ArchiveFormat::Hog },   { "lfd", ArchiveFormat::Lfd },
										 { "lib", ArchiveFormat::Lib },   { "pak", ArchiveFormat::Pak },
										 { "pod", ArchiveFormat::Pod },   { "res", ArchiveFormat::Res },
										 { "rff", ArchiveFormat::Rff },   { "sin", ArchiveFormat::SiN },
										 { "tar", ArchiveFormat::Tar },   { "wad", ArchiveFormat::Wad },
										 { "wadj", ArchiveFormat::WadJ }, { "wad2", ArchiveFormat::Wad2 },
										 { "wolf", ArchiveFormat::Wolf }, { "zip", ArchiveFormat::Zip } };

vector<unique_ptr<ArchiveFormatHandler>> all_handlers;
} // namespace


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, backup_archives)


// -----------------------------------------------------------------------------
//
// Undo Steps
//
// -----------------------------------------------------------------------------

class EntryRenameUS : public UndoStep
{
public:
	EntryRenameUS(ArchiveEntry* entry, string_view new_name) :
		archive_{ entry->parent() },
		entry_path_{ entry->path() },
		entry_index_{ entry->index() },
		old_name_{ entry->name() },
		new_name_{ new_name }
	{
	}

	bool doUndo() override
	{
		// Get entry parent dir
		if (const auto dir = archive_->dirAtPath(entry_path_))
		{
			// Rename entry
			const auto entry = dir->entryAt(entry_index_);
			return archive_->renameEntry(entry, old_name_);
		}

		return false;
	}

	bool doRedo() override
	{
		// Get entry parent dir
		if (const auto dir = archive_->dirAtPath(entry_path_))
		{
			// Rename entry
			const auto entry = dir->entryAt(entry_index_);
			return archive_->renameEntry(entry, new_name_);
		}

		return false;
	}

private:
	Archive* archive_;
	string   entry_path_;
	int      entry_index_;
	string   old_name_;
	string   new_name_;
};

class DirRenameUS : public UndoStep
{
public:
	DirRenameUS(const ArchiveDir* dir, string_view new_name) :
		archive_{ dir->archive() },
		old_name_{ dir->name() },
		new_name_{ new_name },
		prev_state_{ dir->dirEntry()->state() }
	{
		path_ = fmt::format("{}/{}", dir->path(false), new_name);
	}

	void swapNames()
	{
		const auto dir = archive_->dirAtPath(path_);
		archive_->renameDir(dir, old_name_);
		old_name_ = new_name_;
		new_name_ = dir->name();
		path_     = dir->path();
	}

	bool doUndo() override
	{
		swapNames();
		archive_->dirAtPath(path_)->dirEntry()->setState(prev_state_);
		return true;
	}

	bool doRedo() override
	{
		swapNames();
		return true;
	}

private:
	Archive*   archive_;
	string     path_;
	string     old_name_;
	string     new_name_;
	EntryState prev_state_;
};

class EntrySwapUS : public UndoStep
{
public:
	EntrySwapUS(const ArchiveDir* dir, unsigned index1, unsigned index2) :
		archive_{ dir->archive() },
		path_{ dir->path() },
		index1_{ index1 },
		index2_{ index2 }
	{
	}

	bool doSwap() const
	{
		// Get parent dir
		if (const auto dir = archive_->dirAtPath(path_))
			return dir->swapEntries(index1_, index2_);
		return false;
	}

	bool doUndo() override { return doSwap(); }
	bool doRedo() override { return doSwap(); }

private:
	Archive* archive_;
	string   path_;
	unsigned index1_;
	unsigned index2_;
};

class EntryCreateDeleteUS : public UndoStep
{
public:
	EntryCreateDeleteUS(bool created, ArchiveEntry* entry) :
		created_{ created },
		archive_{ entry->parent() },
		entry_copy_{ new ArchiveEntry(*entry) },
		path_{ entry->path() },
		index_{ entry->index() }
	{
	}

	bool deleteEntry() const
	{
		// Get parent dir
		const auto dir = archive_->dirAtPath(path_);
		return dir ? archive_->removeEntry(dir->entryAt(index_)) : false;
	}

	bool createEntry() const
	{
		// Get parent dir
		if (const auto dir = archive_->dirAtPath(path_))
		{
			archive_->addEntry(std::make_shared<ArchiveEntry>(*entry_copy_), index_, dir);
			return true;
		}

		return false;
	}

	bool doUndo() override { return created_ ? deleteEntry() : createEntry(); }

	bool doRedo() override { return !created_ ? deleteEntry() : createEntry(); }

private:
	bool                     created_;
	Archive*                 archive_;
	unique_ptr<ArchiveEntry> entry_copy_;
	string                   path_;
	int                      index_;
};


class DirCreateDeleteUS : public UndoStep
{
public:
	DirCreateDeleteUS(bool created, ArchiveDir* dir) :
		created_{ created },
		archive_{ dir->archive() },
		path_{ dir->path() }
	{
		strutil::removePrefixIP(path_, '/');

		// Backup child entries and subdirs if deleting
		if (!created)
			tree_ = dir->clone();
	}

	bool doUndo() override
	{
		if (created_)
			return archive_->removeDir(path_) != nullptr;
		else
		{
			// Create directory
			auto dir = archive_->createDir(path_);

			// Restore entries/subdirs if needed
			if (dir && tree_)
			{
				// Do merge
				vector<shared_ptr<ArchiveEntry>> created_entries;
				vector<shared_ptr<ArchiveDir>>   created_dirs;
				ArchiveDir::merge(dir, tree_.get(), 0, EntryState::Unmodified, &created_dirs, &created_entries);

				// Signal changes
				for (const auto& cdir : created_dirs)
					archive_->signals().dir_added(*archive_, *cdir);
				for (const auto& entry : created_entries)
					archive_->signals().entry_added(*archive_, *entry);
			}

			if (dir)
				dir->dirEntry()->setState(EntryState::Unmodified);

			return !!dir;
		}
	}

	bool doRedo() override
	{
		if (!created_)
			return archive_->removeDir(path_) != nullptr;
		else
			return archive_->createDir(path_) != nullptr;
	}

private:
	bool                   created_;
	Archive*               archive_;
	string                 path_;
	shared_ptr<ArchiveDir> tree_;
};


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
vector<unique_ptr<ArchiveFormatHandler>>& allFormatHandlers()
{
	// Populate list if needed
	if (all_handlers.empty())
	{
		for (int i = 0; i < static_cast<int>(ArchiveFormat::Unknown); ++i)
			all_handlers.push_back(ArchiveFormatHandler::getHandler(static_cast<ArchiveFormat>(i)));
	}

	return all_handlers;
}
} // namespace


// -----------------------------------------------------------------------------
//
// ArchiveFormatHandler Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// ArchiveFormatHandler class constructor
// -----------------------------------------------------------------------------
ArchiveFormatHandler::ArchiveFormatHandler(ArchiveFormat format_id, bool treeless) :
	format_id_{ format_id },
	treeless_{ treeless }
{
}

// -----------------------------------------------------------------------------
// Reads an archive from disk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool ArchiveFormatHandler::open(Archive& archive, string_view filename)
{
	// Read the file into a MemChunk
	MemChunk mc;
	if (!mc.importFile(filename))
	{
		global::error = "Unable to open file. Make sure it isn't in use by another program.";
		return false;
	}

	// Load from MemChunk
	return open(archive, mc);
}

// -----------------------------------------------------------------------------
// Reads an archive from an ArchiveEntry
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool ArchiveFormatHandler::open(Archive& archive, ArchiveEntry* entry)
{
	// Load from entry's data
	auto sp_entry = entry->getShared();
	if (sp_entry && open(archive, sp_entry->data()))
	{
		// Update variables and return success
		archive.parent_ = sp_entry;
		return true;
	}
	else
		return false;
}

// -----------------------------------------------------------------------------
// Reads an archive from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool ArchiveFormatHandler::open(Archive& archive, const MemChunk& mc)
{
	// Invalid
	return false;
}

// -----------------------------------------------------------------------------
// Writes the archive to a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool ArchiveFormatHandler::write(Archive& archive, MemChunk& mc)
{
	// Invalid
	return false;
}

// -----------------------------------------------------------------------------
// Writes the archive to a file
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool ArchiveFormatHandler::write(Archive& archive, string_view filename)
{
	// Write to a MemChunk, then export it to a file
	if (MemChunk mc; write(archive, mc))
		return mc.exportFile(filename);

	return false;
}

// -----------------------------------------------------------------------------
// This is the general, all-purpose 'save archive' function.
// Takes into account whether the archive is contained within another, is
// already on the disk, etc etc.
// Does a 'save as' if [filename] is specified, unless the archive is contained
// within another.
// Returns false if saving was unsuccessful, true otherwise
// -----------------------------------------------------------------------------
bool ArchiveFormatHandler::save(Archive& archive, string_view filename)
{
	bool success = false;

	// Check if the archive is read-only
	if (archive.read_only_)
	{
		global::error = "Archive is read-only";
		return false;
	}

	// If the archive has a parent ArchiveEntry, just write it to that
	if (auto parent = archive.parent_.lock())
	{
		success = write(archive, parent->data_);
		parent->setState(EntryState::Modified);
	}
	else
	{
		// Otherwise, file stuff
		if (!filename.empty())
		{
			// New filename is given (ie 'save as'), write to new file and change archive filename accordingly
			success = write(archive, filename);
			if (success)
				archive.filename_ = filename;

			// Update variables
			archive.on_disk_       = true;
			archive.file_modified_ = fileutil::fileModifiedTime(archive.filename_);
		}
		else if (!archive.filename_.empty())
		{
			// No filename is given, but the archive has a filename, so overwrite it (and make a backup)

			// Create backup
			if (backup_archives && wxFileName::FileExists(archive.filename_) && Archive::save_backup)
			{
				// Copy current file contents to new backup file
				const auto bakfile = archive.filename_ + ".bak";
				log::info("Creating backup {}", bakfile);
				wxCopyFile(archive.filename_, bakfile, true);
			}

			// Write it to the file
			success = write(archive, archive.filename_);

			// Update variables
			archive.on_disk_       = true;
			archive.file_modified_ = fileutil::fileModifiedTime(archive.filename_);
		}
	}

	// If saving was successful, update variables and announce save
	if (success)
	{
		archive.setModified(false);
		archive.signals_.saved(archive);
	}

	return success;
}

// -----------------------------------------------------------------------------
// Loads an [entry]'s data from the archive file on disk into [out]
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool ArchiveFormatHandler::loadEntryData(Archive& archive, const ArchiveEntry* entry, MemChunk& out)
{
	return archive.genericLoadEntryData(entry, out);
}

// -----------------------------------------------------------------------------
// Creates a directory at [path], starting from [base].
// If [base] is null, the root directory is used.
// Returns the created directory, or if the directory requested to be created
// already exists, it will be returned
// -----------------------------------------------------------------------------
shared_ptr<ArchiveDir> ArchiveFormatHandler::createDir(Archive& archive, string_view path, shared_ptr<ArchiveDir> base)
{
	// Abort if read only ir treeless
	if (archive.read_only_ || treeless_)
		return archive.dir_root_;

	// If no base dir specified, set it to root
	if (!base)
		base = archive.dir_root_;

	if (strutil::startsWith(path, '/'))
		path.remove_prefix(1);

	if (path.empty())
		return base;

	// Create the directory
	vector<shared_ptr<ArchiveDir>> created_dirs;
	auto                           dir = ArchiveDir::getOrCreateSubdir(base, path, &created_dirs);

	// Record undo step(s)
	if (undoredo::currentlyRecording())
		for (const auto& cdir : created_dirs)
			undoredo::currentManager()->recordUndoStep(std::make_unique<DirCreateDeleteUS>(true, cdir.get()));

	// Set the archive state to modified
	archive.setModified(true);

	// Signal directory addition
	for (const auto& cdir : created_dirs)
		archive.signals_.dir_added(archive, *cdir);

	return dir;
}

// -----------------------------------------------------------------------------
// Deletes the directory matching [path], starting from [base].
// If [base] is null, the root directory is used.
// Returns the directory that was removed
// -----------------------------------------------------------------------------
shared_ptr<ArchiveDir> ArchiveFormatHandler::removeDir(Archive& archive, string_view path, ArchiveDir* base)
{
	// Abort if read only or treeless
	if (archive.read_only_ || treeless_)
		return nullptr;

	// Get the dir to remove
	auto dir = archive.dirAtPath(path, base);

	// Check it exists (and that it isn't the root dir)
	if (!dir || dir == archive.dir_root_.get())
		return nullptr;

	// Record undo step
	if (undoredo::currentlyRecording())
		undoredo::currentManager()->recordUndoStep(std::make_unique<DirCreateDeleteUS>(false, dir));

	// Remove the dir from its parent
	auto& parent  = *dir->parent();
	auto  removed = parent.removeSubdir(dir->name());

	// Set the archive state to modified
	archive.setModified(true);

	// Signal directory removal
	archive.signals_.dir_removed(archive, parent, *dir);

	return removed;
}

// -----------------------------------------------------------------------------
// Renames [dir] to [new_name].
// Returns false if [dir] isn't part of the archive, true otherwise
// -----------------------------------------------------------------------------
bool ArchiveFormatHandler::renameDir(Archive& archive, ArchiveDir* dir, string_view new_name)
{
	// Abort if read only or treeless
	if (archive.read_only_ || treeless_)
		return false;

	// Check the directory is part of this archive
	if (!dir || dir->archive() != &archive)
		return false;

	// Rename the directory if needed
	if (dir->name() != new_name)
	{
		if (undoredo::currentlyRecording())
			undoredo::currentManager()->recordUndoStep(std::make_unique<DirRenameUS>(dir, new_name));

		dir->setName(new_name);
		dir->dirEntry()->setState(EntryState::Modified);
	}
	else
		return true;

	// Update variables etc
	archive.setModified(true);

	return true;
}

// -----------------------------------------------------------------------------
// Adds [entry] to [dir] at [position].
// If [dir] is null it is added to the root dir.
// If [position] is out of bounds, it is added to the end of the dir.
// If [copy] is true, a copy of [entry] is added (rather than [entry] itself).
// Returns the added entry, or null if [entry] is invalid or the archive is
// read-only
// -----------------------------------------------------------------------------
shared_ptr<ArchiveEntry> ArchiveFormatHandler::addEntry(
	Archive&                 archive,
	shared_ptr<ArchiveEntry> entry,
	unsigned                 position,
	ArchiveDir*              dir)
{
	// Abort if read only
	if (archive.read_only_)
		return nullptr;

	// Check valid entry
	if (!entry)
		return nullptr;

	// If no dir given, set it to the root dir
	if (!dir || treeless_)
		dir = archive.dir_root_.get();

	// Add the entry
	dir->addEntry(entry, position);
	entry->formatName(archive.formatDesc());

	// Update variables etc
	archive.setModified(true);
	entry->state_ = EntryState::New;

	// Signal entry addition
	archive.signals_.entry_added(archive, *entry);

	// Create undo step
	if (undoredo::currentlyRecording())
		undoredo::currentManager()->recordUndoStep(std::make_unique<EntryCreateDeleteUS>(true, entry.get()));

	return entry;
}

// -----------------------------------------------------------------------------
// Adds [entry] to the end of the namespace matching [add_namespace].
// If [copy] is true a copy of the entry is added.
// Returns the added entry or NULL if the entry is invalid
// -----------------------------------------------------------------------------
shared_ptr<ArchiveEntry> ArchiveFormatHandler::addEntry(
	Archive&                 archive,
	shared_ptr<ArchiveEntry> entry,
	string_view              add_namespace)
{
	return addEntry(archive, entry, 0xFFFFFFFF, nullptr);
}

// -----------------------------------------------------------------------------
// Creates a new entry with [name] and adds it to [dir] at [position].
// If [dir] is null it is added to the root dir.
// If [position] is out of bounds, it is added tothe end of the dir.
// Return false if the entry is invalid, true otherwise
// -----------------------------------------------------------------------------
shared_ptr<ArchiveEntry> ArchiveFormatHandler::addNewEntry(
	Archive&    archive,
	string_view name,
	unsigned    position,
	ArchiveDir* dir)
{
	// Abort if read only
	if (archive.read_only_)
		return nullptr;

	// Create the new entry
	auto entry = std::make_shared<ArchiveEntry>(name);

	// Add it to the archive
	addEntry(archive, entry, position, dir);

	// Return the newly created entry
	return entry;
}

// -----------------------------------------------------------------------------
// Removes [entry] from the archive.
// If [delete_entry] is true, the entry will also be deleted.
// Returns true if the removal succeeded
// -----------------------------------------------------------------------------
bool ArchiveFormatHandler::removeEntry(Archive& archive, ArchiveEntry* entry, bool set_deleted)
{
	// Abort if read only
	if (archive.read_only_)
		return false;

	// Check entry
	if (!archive.checkEntry(entry))
		return false;

	// Check if entry is locked
	if (entry->isLocked())
		return false;

	// Get its directory
	auto dir = entry->parentDir();

	// Error if entry has no parent directory
	if (!dir)
		return false;

	// Create undo step
	if (undoredo::currentlyRecording())
		undoredo::currentManager()->recordUndoStep(std::make_unique<EntryCreateDeleteUS>(false, entry));

	// Remove the entry
	const int  index        = dir->entryIndex(entry);
	auto       entry_shared = entry->getShared();      // Ensure the entry is kept around until this function ends
	const bool ok           = dir->removeEntry(index); // Remove it from its directory

	// If it was removed ok
	if (ok)
	{
		// Set state
		if (set_deleted)
			entry_shared->setState(EntryState::Deleted);

		archive.signals_.entry_removed(archive, *dir, *entry); // Signal entry removed
		archive.setModified(true);                             // Update variables etc
	}

	return ok;
}

// -----------------------------------------------------------------------------
// Swaps [entry1] and [entry2].
// Returns false if either entry is invalid or if both entries are not in the
// same directory, true otherwise
// -----------------------------------------------------------------------------
bool ArchiveFormatHandler::swapEntries(Archive& archive, ArchiveEntry* entry1, ArchiveEntry* entry2)
{
	// Abort if read only
	if (archive.read_only_)
		return false;

	// Check both entries
	if (!archive.checkEntry(entry1) || !archive.checkEntry(entry2))
		return false;

	// Check neither entry is locked
	if (entry1->isLocked() || entry2->isLocked())
		return false;

	// Get their directory
	auto dir = entry1->parentDir();

	// Error if no dir
	if (!dir)
		return false;

	// Check they are both in the same directory
	if (entry2->parentDir() != dir)
	{
		log::error("Can't swap two entries in different directories");
		return false;
	}

	// Get entry indices
	int i1 = dir->entryIndex(entry1);
	int i2 = dir->entryIndex(entry2);

	// Check indices
	if (i1 < 0 || i2 < 0)
		return false;

	// Create undo step
	if (undoredo::currentlyRecording())
		undoredo::currentManager()->recordUndoStep(std::make_unique<EntrySwapUS>(dir, i1, i2));

	// Swap entries
	dir->swapEntries(i1, i2);

	// Set modified
	archive.setModified(true);

	// Signal
	archive.signals_.entries_swapped(archive, *dir, i1, i2);

	// Return success
	return true;
}

// -----------------------------------------------------------------------------
// Moves [entry] to [position] in [dir].
// If [dir] is null, the root dir is used.
// Returns false if the entry was invalid, true otherwise
// -----------------------------------------------------------------------------
bool ArchiveFormatHandler::moveEntry(Archive& archive, ArchiveEntry* entry, unsigned position, ArchiveDir* dir)
{
	// Abort if read only
	if (archive.read_only_)
		return false;

	// Check the entry
	if (!archive.checkEntry(entry))
		return false;

	// Check if the entry is locked
	if (entry->isLocked())
		return false;

	// Get the entry's current directory
	const auto cdir = entry->parentDir();

	// Error if no dir
	if (!cdir)
		return false;

	// If no destination dir specified, use root
	if (!dir || treeless_)
		dir = archive.dir_root_.get();

	// Remove the entry from its current dir
	const auto sptr = entry->getShared(); // Get a shared pointer so it isn't deleted
	removeEntry(archive, entry, false);

	// Add it to the destination dir
	addEntry(archive, sptr, position, dir);

	// Set modified
	archive.setModified(true);

	// Return success
	return true;
}

// -----------------------------------------------------------------------------
// Renames [entry] with [name].
// Returns false if the entry was invalid, true otherwise
// -----------------------------------------------------------------------------
bool ArchiveFormatHandler::renameEntry(Archive& archive, ArchiveEntry* entry, string_view name, bool force)
{
	// Abort if read only
	if (archive.read_only_)
		return false;

	// Check entry
	if (!archive.checkEntry(entry))
		return false;

	// Check if entry is locked
	if (entry->isLocked())
		return false;

	// Check for directory
	if (entry->type() == EntryType::folderType())
		return renameDir(archive, archive.dirAtPath(entry->path(true)), name);

	// Keep current name for renamed signal
	const auto prev_name = entry->name();

	// Create undo step
	if (undoredo::currentlyRecording())
		undoredo::currentManager()->recordUndoStep(std::make_unique<EntryRenameUS>(entry, name));

	// Rename the entry
	auto fmt_desc = archive.formatDesc();
	entry->setName(name);
	entry->formatName(fmt_desc);
	if (!force && !fmt_desc.allow_duplicate_names)
		entry->parentDir()->ensureUniqueName(entry);
	entry->setState(EntryState::Modified, true);

	// Announce modification
	archive.signals_.entry_renamed(archive, *entry, prev_name);
	archive.entryStateChanged(entry);

	return true;
}

// -----------------------------------------------------------------------------
// Returns the MapDesc information about the map beginning at [maphead].
// To be implemented in Archive sub-classes.
// -----------------------------------------------------------------------------
MapDesc ArchiveFormatHandler::mapDesc(Archive& archive, ArchiveEntry* maphead)
{
	return {};
}

// -----------------------------------------------------------------------------
// Returns the MapDesc information about all maps in the Archive.
// To be implemented in Archive sub-classes.
// -----------------------------------------------------------------------------
vector<MapDesc> ArchiveFormatHandler::detectMaps(Archive& archive)
{
	return {};
}

// -----------------------------------------------------------------------------
// Returns the namespace of the entry at [index] within [dir]
// -----------------------------------------------------------------------------
string ArchiveFormatHandler::detectNamespace(Archive& archive, unsigned index, ArchiveDir* dir)
{
	if (dir && index < dir->numEntries())
		return detectNamespace(archive, dir->entryAt(index));

	return "global";
}

// -----------------------------------------------------------------------------
// Returns the namespace that [entry] is within
// -----------------------------------------------------------------------------
string ArchiveFormatHandler::detectNamespace(Archive& archive, ArchiveEntry* entry)
{
	// Check entry
	if (!archive.checkEntry(entry))
		return "global";

	// If the entry is in the root dir, it's in the global namespace
	if (entry->parentDir() == archive.dir_root_.get())
		return "global";

	// Get the entry's *first* parent directory after root (ie <root>/namespace/)
	auto dir = entry->parentDir();
	while (dir && dir->parent() != archive.dir_root_)
		dir = dir->parent().get();

	// Namespace is the directory's name (in lowercase)
	if (dir)
		return strutil::lower(dir->name());
	else
		return "global"; // Error, just return global
}

// -----------------------------------------------------------------------------
// Returns the first entry matching the search criteria in [options], or null if
// no matching entry was found
// -----------------------------------------------------------------------------
ArchiveEntry* ArchiveFormatHandler::findFirst(Archive& archive, ArchiveSearchOptions& options)
{
	// Init search variables
	auto dir = options.dir;
	if (!dir)
		dir = archive.dir_root_.get();
	strutil::upperIP(options.match_name); // Force case-insensitive

	// Begin search

	// Search entries
	for (unsigned a = 0; a < dir->numEntries(); a++)
	{
		const auto entry = dir->entryAt(a);

		// Check type
		if (options.match_type)
		{
			if (entry->type() == EntryType::unknownType())
			{
				if (!options.match_type->isThisType(*entry))
					continue;
			}
			else if (options.match_type != entry->type())
				continue;
		}

		// Check name
		if (!options.match_name.empty())
		{
			// Cut extension if ignoring
			const auto check_name = options.ignore_ext ? entry->upperNameNoExt() : entry->upperName();
			if (!strutil::matches(check_name, options.match_name))
				continue;
		}

		// Check namespace
		if (!options.match_namespace.empty())
		{
			if (!strutil::equalCI(detectNamespace(archive, entry), options.match_namespace))
				continue;
		}

		// Entry passed all checks so far, so we found a match
		return entry;
	}

	// Search subdirectories (if needed)
	if (options.search_subdirs)
	{
		for (unsigned a = 0; a < dir->numSubdirs(); a++)
		{
			auto opt = options;
			opt.dir  = dir->subdirAt(a).get();

			// If a match was found in this subdir, return it
			if (const auto match = findFirst(archive, opt))
				return match;
		}
	}

	// No matches found
	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the last entry matching the search criteria in [options], or null if
// no matching entry was found
// -----------------------------------------------------------------------------
ArchiveEntry* ArchiveFormatHandler::findLast(Archive& archive, ArchiveSearchOptions& options)
{
	// Init search variables
	auto dir = options.dir;
	if (!dir)
		dir = archive.dir_root_.get();
	strutil::upperIP(options.match_name); // Force case-insensitive

	// Begin search

	// Search entries (bottom-up)
	for (int a = static_cast<int>(dir->numEntries()) - 1; a >= 0; a--)
	{
		const auto entry = dir->entryAt(a);

		// Check type
		if (options.match_type)
		{
			if (entry->type() == EntryType::unknownType())
			{
				if (!options.match_type->isThisType(*entry))
					continue;
			}
			else if (options.match_type != entry->type())
				continue;
		}

		// Check name
		if (!options.match_name.empty())
		{
			// Cut extension if ignoring
			const auto check_name = options.ignore_ext ? entry->upperNameNoExt() : entry->upperName();
			if (!strutil::matches(check_name, options.match_name))
				continue;
		}

		// Check namespace
		if (!options.match_namespace.empty())
		{
			if (!strutil::equalCI(detectNamespace(archive, entry), options.match_namespace))
				continue;
		}

		// Entry passed all checks so far, so we found a match
		return entry;
	}

	// Search subdirectories (if needed) (bottom-up)
	if (options.search_subdirs)
	{
		for (int a = static_cast<int>(dir->numSubdirs()) - 1; a >= 0; a--)
		{
			auto opt = options;
			opt.dir  = dir->subdirAt(a).get();

			// If a match was found in this subdir, return it
			if (const auto match = findLast(archive, opt))
				return match;
		}
	}

	// No matches found
	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns a list of entries matching the search criteria in [options]
// -----------------------------------------------------------------------------
vector<ArchiveEntry*> ArchiveFormatHandler::findAll(Archive& archive, ArchiveSearchOptions& options)
{
	// Init search variables
	auto dir = options.dir;
	if (!dir)
		dir = archive.dir_root_.get();
	vector<ArchiveEntry*> ret;
	strutil::upperIP(options.match_name); // Force case-insensitive

	// Begin search

	// Search entries
	for (unsigned a = 0; a < dir->numEntries(); a++)
	{
		auto entry = dir->entryAt(a);

		// Check type
		if (options.match_type)
		{
			if (entry->type() == EntryType::unknownType())
			{
				if (!options.match_type->isThisType(*entry))
					continue;
			}
			else if (options.match_type != entry->type())
				continue;
		}

		// Check name
		if (!options.match_name.empty())
		{
			// Cut extension if ignoring
			const auto check_name = options.ignore_ext ? entry->upperNameNoExt() : entry->upperName();
			if (!strutil::matches(check_name, options.match_name))
				continue;
		}

		// Check namespace
		if (!options.match_namespace.empty())
		{
			if (!strutil::equalCI(detectNamespace(archive, entry), options.match_namespace))
				continue;
		}

		// Entry passed all checks so far, so we found a match
		ret.push_back(entry);
	}

	// Search subdirectories (if needed)
	if (options.search_subdirs)
	{
		for (unsigned a = 0; a < dir->numSubdirs(); a++)
		{
			auto opt = options;
			opt.dir  = dir->subdirAt(a).get();

			// Add any matches to the list
			auto vec = findAll(archive, opt);
			ret.insert(ret.end(), vec.begin(), vec.end());
		}
	}

	// Return matches
	return ret;
}

void ArchiveFormatHandler::detectAllEntryTypes(const Archive& archive)
{
	archive.detectAllEntryTypes();
}

bool ArchiveFormatHandler::isThisFormat(const MemChunk& mc)
{
	return false;
}
bool ArchiveFormatHandler::isThisFormat(const string& filename)
{
	return false;
}


// -----------------------------------------------------------------------------
//
// ArchiveFormatHandler Class Static Functions
//
// -----------------------------------------------------------------------------

unique_ptr<ArchiveFormatHandler> ArchiveFormatHandler::getHandler(ArchiveFormat format)
{
	switch (format)
	{
	case ArchiveFormat::ADat:     return std::make_unique<ADatArchiveHandler>();
	case ArchiveFormat::Bsp:      return std::make_unique<BSPArchiveHandler>();
	case ArchiveFormat::Bz2:      return std::make_unique<BZip2ArchiveHandler>();
	case ArchiveFormat::ChasmBin: return std::make_unique<ChasmBinArchiveHandler>();
	case ArchiveFormat::Dat:      return std::make_unique<DatArchiveHandler>();
	case ArchiveFormat::Dir:      return std::make_unique<DirArchiveHandler>();
	case ArchiveFormat::Disk:     return std::make_unique<DiskArchiveHandler>();
	case ArchiveFormat::Gob:      return std::make_unique<GobArchiveHandler>();
	case ArchiveFormat::Grp:      return std::make_unique<GrpArchiveHandler>();
	case ArchiveFormat::GZip:     return std::make_unique<GZipArchiveHandler>();
	case ArchiveFormat::Hog:      return std::make_unique<HogArchiveHandler>();
	case ArchiveFormat::Lfd:      return std::make_unique<LfdArchiveHandler>();
	case ArchiveFormat::Lib:      return std::make_unique<LibArchiveHandler>();
	case ArchiveFormat::Pak:      return std::make_unique<PakArchiveHandler>();
	case ArchiveFormat::Pod:      return std::make_unique<PodArchiveHandler>();
	case ArchiveFormat::Res:      return std::make_unique<ResArchiveHandler>();
	case ArchiveFormat::Rff:      return std::make_unique<RffArchiveHandler>();
	case ArchiveFormat::SiN:      return std::make_unique<SiNArchiveHandler>();
	case ArchiveFormat::Tar:      return std::make_unique<TarArchiveHandler>();
	case ArchiveFormat::Wad:      return std::make_unique<WadArchiveHandler>();
	case ArchiveFormat::WadJ:     return std::make_unique<WadJArchiveHandler>();
	case ArchiveFormat::Wad2:     return std::make_unique<Wad2ArchiveHandler>();
	case ArchiveFormat::Wolf:     return std::make_unique<WolfArchiveHandler>();
	case ArchiveFormat::Zip:      return std::make_unique<ZipArchiveHandler>();
	default:                      break;
	}

	// Unknown format
	return std::make_unique<ArchiveFormatHandler>(ArchiveFormat::Unknown);
}

unique_ptr<ArchiveFormatHandler> ArchiveFormatHandler::getHandler(string_view format)
{
	return getHandler(formatIdFromString(format));
}

string ArchiveFormatHandler::formatIdString(ArchiveFormat format)
{
	for (const auto& [name, id] : formats)
		if (id == format)
			return name;

	return "unknown";
}

ArchiveFormat ArchiveFormatHandler::formatIdFromString(string_view format_id_string)
{
	for (const auto& [name, id] : formats)
		if (name == format_id_string)
			return id;

	return ArchiveFormat::Unknown;
}

ArchiveFormat ArchiveFormatHandler::detectArchiveFormat(const MemChunk& mc)
{
	for (const auto& handler : allFormatHandlers())
		if (handler->isThisFormat(mc))
			return handler->formatId();

	return ArchiveFormat::Unknown;
}

ArchiveFormat ArchiveFormatHandler::detectArchiveFormat(const string& filename)
{
	for (const auto& handler : allFormatHandlers())
		if (handler->isThisFormat(filename))
			return handler->formatId();

	return ArchiveFormat::Unknown;
}

bool ArchiveFormatHandler::isFormat(const MemChunk& mc, ArchiveFormat format)
{
	auto index = static_cast<int>(format);
	if (index < 0 || index >= static_cast<int>(ArchiveFormat::Unknown))
		return false;

	return allFormatHandlers()[index]->isThisFormat(mc);
}

bool ArchiveFormatHandler::isFormat(const string& filename, ArchiveFormat format)
{
	auto index = static_cast<int>(format);
	if (index < 0 || index >= static_cast<int>(ArchiveFormat::Unknown))
		return false;

	return allFormatHandlers()[index]->isThisFormat(filename);
}
