
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Archive.cpp
// Description: Archive, the 'base' archive class (Abstract)
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
#include "Archive.h"
#include "ArchiveDir.h"
#include "ArchiveEntry.h"
#include "ArchiveFormatHandler.h"
#include "EntryType/EntryType.h"
#include "General/UI.h"
#include "MapDesc.h"
#include "Utility/FileUtils.h"
#include "Utility/StringUtils.h"
#include <SFML/System/Clock.hpp>
#include <filesystem>

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
bool Archive::save_backup = true;


// -----------------------------------------------------------------------------
//
// MapDesc Struct Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns a list of all data entries (eg. LINEDEFS, TEXTMAP) for the map.
// If [include_head] is true, the map header entry is also added
// -----------------------------------------------------------------------------
vector<ArchiveEntry*> MapDesc::entries(const Archive& parent, bool include_head) const
{
	vector<ArchiveEntry*> list;

	// Map in zip, no map data entries
	if (archive)
		return list;

	auto       index = include_head ? parent.entryIndex(head.lock().get()) : parent.entryIndex(head.lock().get()) + 1;
	const auto index_end = parent.entryIndex(end.lock().get());
	if (index < 0 || index_end < 0)
		return list;

	while (index <= index_end)
		list.push_back(parent.entryAt(index++));

	return list;
}

// -----------------------------------------------------------------------------
// Sets the appropriate 'MapFormat' extra property in all entries for the map
// -----------------------------------------------------------------------------
void MapDesc::updateMapFormatHints() const
{
	// Get parent archive
	Archive* parent = nullptr;
	if (const auto entry = head.lock())
		parent = entry->parent();
	if (!parent)
		return;

	// Determine map format name
	string fmt_name;
	switch (format)
	{
	case MapFormat::Doom:    fmt_name = "doom"; break;
	case MapFormat::Hexen:   fmt_name = "hexen"; break;
	case MapFormat::Doom64:  fmt_name = "doom64"; break;
	case MapFormat::UDMF:    fmt_name = "udmf"; break;
	case MapFormat::Doom32X: fmt_name = "doom32x"; break;
	case MapFormat::Unknown:
	default:                 fmt_name = "unknown"; break;
	}

	// Set format hint in map entries
	auto m_entry = head.lock();
	auto m_index = parent->entryIndex(m_entry.get());
	while (m_entry)
	{
		m_entry->exProp("MapFormat") = fmt_name;

		if (m_entry == end.lock())
			break;

		m_entry = parent->rootDir()->sharedEntryAt(++m_index);
	}
}


// -----------------------------------------------------------------------------
//
// Archive Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Archive class constructor
// -----------------------------------------------------------------------------
Archive::Archive(string_view format) : dir_root_{ new ArchiveDir("", nullptr, this) }
{
	format_handler_ = archive::formatHandler(format);
	dir_root_->allowDuplicateNames(formatInfo().allow_duplicate_names);
	format_handler_->init(*this);
}
Archive::Archive(ArchiveFormat format) : dir_root_{ new ArchiveDir("", nullptr, this) }
{
	format_handler_ = archive::formatHandler(format);
	dir_root_->allowDuplicateNames(formatInfo().allow_duplicate_names);
	format_handler_->init(*this);
}

// -----------------------------------------------------------------------------
// Archive class destructor
// -----------------------------------------------------------------------------
Archive::~Archive()
{
	// Clear out subdir archive pointers in case any are being kept around
	// (eg. by a running script)
	dir_root_->setArchive(nullptr);
}

// -----------------------------------------------------------------------------
// Returns the ArchiveFormatInfo descriptor for this archive
// -----------------------------------------------------------------------------
const ArchiveFormatInfo& Archive::formatInfo() const
{
	return archive::formatInfo(format_handler_->format());
}

// -----------------------------------------------------------------------------
// Gets the wxWidgets file dialog filter string for the archive type
// -----------------------------------------------------------------------------
string Archive::fileExtensionString() const
{
	auto fmt = formatInfo();

	// Multiple extensions
	if (fmt.extensions.size() > 1)
	{
		auto           ext_all = fmt::format("Any {} File|", fmt.name);
		vector<string> ext_strings;
		for (const auto& ext : fmt.extensions)
		{
			auto ext_case = fmt::format("*.{};", strutil::lower(ext.first));
			ext_case += fmt::format("*.{};", strutil::upper(ext.first));
			ext_case += fmt::format("*.{}", strutil::capitalize(ext.first));

			ext_all += fmt::format("{};", ext_case);
			ext_strings.push_back(fmt::format("{} File (*.{})|{}", ext.second, ext.first, ext_case));
		}

		ext_all.pop_back();
		for (const auto& ext : ext_strings)
			ext_all += fmt::format("|{}", ext);

		return ext_all;
	}

	// Single extension
	if (fmt.extensions.size() == 1)
	{
		auto& ext      = fmt.extensions[0];
		auto  ext_case = fmt::format("*.{};", strutil::lower(ext.first));
		ext_case += fmt::format("*.{};", strutil::upper(ext.first));
		ext_case += fmt::format("*.{}", strutil::capitalize(ext.first));

		return fmt::format("{} File (*.{})|{}", ext.second, ext.first, ext_case);
	}

	// No extension (probably unknown type)
	return "Any File|*.*";
}

// -----------------------------------------------------------------------------
// Returns the archive's filename, including the path if specified
// -----------------------------------------------------------------------------
string Archive::filename(bool full) const
{
	// If the archive is within another archive, return "<parent archive>/<entry name>"
	if (const auto parent = parent_.lock())
	{
		string parent_archive;
		if (parentArchive())
			parent_archive = parentArchive()->filename(false) + "/";

		return parent_archive.append(strutil::Path::fileNameOf(parent->name(), false));
	}

	return full ? filename_ : string{ strutil::Path::fileNameOf(filename_) };
}

// -----------------------------------------------------------------------------
// Returns the archive's parent archive (if it is embedded)
// -----------------------------------------------------------------------------
Archive* Archive::parentArchive() const
{
	return parent_.lock() ? parent_.lock()->parent() : nullptr;
}

// -----------------------------------------------------------------------------
// Returns the archive's format
// -----------------------------------------------------------------------------
ArchiveFormat Archive::format() const
{
	return format_handler_->format();
}

// -----------------------------------------------------------------------------
// Returns the archive's format id string
// -----------------------------------------------------------------------------
string Archive::formatId() const
{
	return archive::formatId(format());
}

// -----------------------------------------------------------------------------
// Reads an archive from disk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool Archive::open(string_view filename)
{
	// Update filename before opening
	const auto backupname = filename_;
	filename_             = filename;

	// Open via format handler
	const sf::Clock timer;
	if (format_handler_->open(*this, filename))
	{
		log::info(2, "Archive::open took {}ms", timer.getElapsedTime().asMilliseconds());
		file_modified_ = fileutil::fileModifiedTime(filename);
		on_disk_       = true;
		return true;
	}
	else
	{
		// Open failed, revert filename
		filename_ = backupname;
		return false;
	}
}

// -----------------------------------------------------------------------------
// Reads an archive from an ArchiveEntry
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool Archive::open(ArchiveEntry* entry)
{
	return format_handler_->open(*this, entry);
}

bool Archive::open(const MemChunk& mc)
{
	return format_handler_->open(*this, mc);
}

bool Archive::write(MemChunk& mc)
{
	return format_handler_->write(*this, mc);
}

bool Archive::isWritable() const
{
	return format_handler_->isWritable();
}

// -----------------------------------------------------------------------------
// Sets the Archive's modified status and announces the change
// -----------------------------------------------------------------------------
void Archive::setModified(bool modified)
{
	if (modified_ != modified)
	{
		modified_ = modified;               // Update modified flag
		signals_.modified(*this, modified); // Emit signal
	}
}

// -----------------------------------------------------------------------------
// Checks that the given entry is valid and part of this archive
// -----------------------------------------------------------------------------
bool Archive::checkEntry(const ArchiveEntry* entry) const
{
	// Check entry is valid
	if (!entry)
		return false;

	// Check entry is part of this archive
	if (entry->parent() != this)
		return false;

	// Entry is ok
	return true;
}

// -----------------------------------------------------------------------------
// Returns the entry matching [name] within [dir].
// If no dir is given the root dir is used
// -----------------------------------------------------------------------------
ArchiveEntry* Archive::entry(string_view name, bool cut_ext, const ArchiveDir* dir) const
{
	// Check if dir was given
	if (!dir || format_handler_->isTreeless())
		dir = dir_root_.get(); // None given, use root

	return dir->entry(name, cut_ext);
}

// -----------------------------------------------------------------------------
// Returns the entry at [index] within [dir].
// If no dir is given the  root dir is used.
// Returns null if [index] is out of bounds
// -----------------------------------------------------------------------------
ArchiveEntry* Archive::entryAt(unsigned index, const ArchiveDir* dir) const
{
	// Check if dir was given
	if (!dir || format_handler_->isTreeless())
		dir = dir_root_.get(); // None given, use root

	return dir->entryAt(index);
}

// -----------------------------------------------------------------------------
// Returns the index of [entry] within [dir].
// If no dir is given the root dir is used.
// Returns -1 if [entry] doesn't exist within [dir]
// -----------------------------------------------------------------------------
int Archive::entryIndex(ArchiveEntry* entry, const ArchiveDir* dir) const
{
	// Check if dir was given
	if (!dir || format_handler_->isTreeless())
		dir = dir_root_.get(); // None given, use root

	return dir->entryIndex(entry);
}

// -----------------------------------------------------------------------------
// Returns the entry at the given path in the archive, or null if it doesn't
// exist
// -----------------------------------------------------------------------------
ArchiveEntry* Archive::entryAtPath(string_view path) const
{
	// Get [path] as Path for processing
	const strutil::Path fn(strutil::startsWith(path, '/') ? path.substr(1) : path);

	// Get directory from path
	ArchiveDir* dir;
	if (fn.path(false).empty())
		dir = dir_root_.get();
	else
		dir = dirAtPath(fn.path(true));

	// If dir doesn't exist, return nullptr
	if (!dir)
		return nullptr;

	// Return entry
	return dir->entry(fn.fileName());
}

// -----------------------------------------------------------------------------
// Returns the entry at the given path in the archive, or null if it doesn't
// exist
// -----------------------------------------------------------------------------
shared_ptr<ArchiveEntry> Archive::entryAtPathShared(string_view path) const
{
	// Get path as wxFileName for processing
	const strutil::Path fn(strutil::startsWith(path, '/') ? path.substr(1) : path);

	// Get directory from path
	ArchiveDir* dir;
	if (fn.path(false).empty())
		dir = dir_root_.get();
	else
		dir = dirAtPath(fn.path(false));

	// If dir doesn't exist, return nullptr
	if (!dir)
		return nullptr;

	// Return entry
	return dir->sharedEntry(fn.fileName());
}

// -----------------------------------------------------------------------------
// Writes the archive to a file
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool Archive::write(string_view filename)
{
	return format_handler_->write(*this, filename);
}

// -----------------------------------------------------------------------------
// This is the general, all-purpose 'save archive' function.
// Takes into account whether the archive is contained within another, is
// already on the disk, etc etc.
// Does a 'save as' if [filename] is specified, unless the archive is contained
// within another.
// Returns false if saving was unsuccessful, true otherwise
// -----------------------------------------------------------------------------
bool Archive::save(string_view filename)
{
	return format_handler_->save(*this, filename);
}

bool Archive::loadEntryData(const ArchiveEntry* entry, MemChunk& out)
{
	// Check entry is ok
	if (!checkEntry(entry))
		return false;

	return format_handler_->loadEntryData(*this, entry, out);
}

// -----------------------------------------------------------------------------
// Returns the total number of entries in the archive
// -----------------------------------------------------------------------------
unsigned Archive::numEntries() const
{
	return dir_root_->numEntries(!format_handler_->isTreeless());
}

// -----------------------------------------------------------------------------
// 'Closes' the archive
// -----------------------------------------------------------------------------
void Archive::close()
{
	// Clear the root dir
	dir_root_->clear();

	// Announce
	signals_.closed(*this);
}

// -----------------------------------------------------------------------------
// Updates the archive variables and announces if necessary that an entry's
// state has changed
// -----------------------------------------------------------------------------
void Archive::entryStateChanged(ArchiveEntry* entry)
{
	// Check the entry is valid and part of this archive
	if (!checkEntry(entry))
		return;

	// Signal entry state change
	signals_.entry_state_changed(*this, *entry);

	// If entry was set to unmodified, don't set the archive to modified
	if (entry->state() == EntryState::Unmodified)
		return;

	// Set the archive state to modified
	setModified(true);
}

// -----------------------------------------------------------------------------
// Adds the directory structure starting from [start] to [list]
// -----------------------------------------------------------------------------
void Archive::putEntryTreeAsList(vector<ArchiveEntry*>& list, const ArchiveDir* start) const
{
	// If no start dir is specified, use the root dir
	if (!start)
		start = dir_root_.get();

	// Get tree a flat list of shared pointers
	vector<shared_ptr<ArchiveEntry>> shared_list;
	ArchiveDir::entryTreeAsList(start, shared_list);

	// Build resulting list of raw pointers
	list.reserve(shared_list.size());
	for (const auto& entry : shared_list)
		list.emplace_back(entry.get());
}

// -----------------------------------------------------------------------------
// Adds the directory structure starting from [start] to [list]
// -----------------------------------------------------------------------------
void Archive::putEntryTreeAsList(vector<shared_ptr<ArchiveEntry>>& list, const ArchiveDir* start) const
{
	// If no start dir is specified, use the root dir
	if (!start)
		start = dir_root_.get();

	// Build list from [start]
	ArchiveDir::entryTreeAsList(start, list);
}

// -----------------------------------------------------------------------------
// 'Pastes' the [tree] into the archive, with its root entries starting at
// [position] in [base] directory.
// If [base] is null, the root directory is used.
//
// If the archive is treeless, pastes all entries in [tree] and its
// subdirectories straight into the root dir at [position]
// -----------------------------------------------------------------------------
bool Archive::paste(ArchiveDir* tree, unsigned position, shared_ptr<ArchiveDir> base)
{
	// Check tree was given to paste
	if (!tree)
		return false;

	// Treeless paste
	if (format_handler_->isTreeless())
	{
		// Paste root entries only
		for (unsigned a = 0; a < tree->numEntries(); a++)
		{
			// Add entry to archive
			addEntry(std::make_shared<ArchiveEntry>(*tree->entryAt(a)), position, nullptr);

			// Update [position] if needed
			if (position < 0xFFFFFFFF)
				position++;
		}

		// Go through paste tree subdirs and paste their entries recursively
		for (unsigned a = 0; a < tree->numSubdirs(); a++)
			paste(tree->subdirAt(a).get(), position);

		// Set modified
		setModified(true);

		return true;
	}

	// Paste to root dir if none specified
	if (!base)
		base = dir_root_;

	// Set modified
	setModified(true);

	// Do merge
	vector<shared_ptr<ArchiveEntry>> created_entries;
	vector<shared_ptr<ArchiveDir>>   created_dirs;
	if (!ArchiveDir::merge(base, tree, position, EntryState::New, &created_dirs, &created_entries))
		return false;

	// Signal changes
	for (const auto& cdir : created_dirs)
		signals_.dir_added(*this, *cdir);
	for (const auto& entry : created_entries)
		signals_.entry_added(*this, *entry);

	return true;
}

// -----------------------------------------------------------------------------
// Gets the directory matching [path], starting from [base].
// If [base] is null, the root directory is used.
// Returns null if the requested directory does not exist
// -----------------------------------------------------------------------------
ArchiveDir* Archive::dirAtPath(string_view path, ArchiveDir* base) const
{
	if (!base)
		base = dir_root_.get();

	return ArchiveDir::subdirAtPath(base, path);
}

// -----------------------------------------------------------------------------
// Creates a directory at [path], starting from [base].
// If [base] is null, the root directory is used.
// Returns the created directory, or if the directory requested to be created
// already exists, it will be returned
// -----------------------------------------------------------------------------
shared_ptr<ArchiveDir> Archive::createDir(string_view path, shared_ptr<ArchiveDir> base)
{
	return format_handler_->createDir(*this, path, base);
}

// -----------------------------------------------------------------------------
// Deletes the directory matching [path], starting from [base].
// If [base] is null, the root directory is used.
// Returns the directory that was removed
// -----------------------------------------------------------------------------
shared_ptr<ArchiveDir> Archive::removeDir(string_view path, ArchiveDir* base)
{
	// Abort if read only or treeless
	if (read_only_ || isTreeless())
		return nullptr;

	return format_handler_->removeDir(*this, path, base);
}

// -----------------------------------------------------------------------------
// Renames [dir] to [new_name].
// Returns false if [dir] isn't part of the archive, true otherwise
// -----------------------------------------------------------------------------
bool Archive::renameDir(ArchiveDir* dir, string_view new_name)
{
	// Abort if read only or treeless
	if (read_only_ || isTreeless())
		return false;

	// Check the directory is part of this archive
	if (!dir || dir->archive() != this)
		return false;

	return format_handler_->renameDir(*this, dir, new_name);
}

// -----------------------------------------------------------------------------
// Adds [entry] to [dir] at [position].
// If [dir] is null it is added to the root dir.
// If [position] is out of bounds, it is added to the end of the dir.
// If [copy] is true, a copy of [entry] is added (rather than [entry] itself).
// Returns the added entry, or null if [entry] is invalid or the archive is
// read-only
// -----------------------------------------------------------------------------
shared_ptr<ArchiveEntry> Archive::addEntry(shared_ptr<ArchiveEntry> entry, unsigned position, ArchiveDir* dir)
{
	// Abort if read only
	if (read_only_)
		return nullptr;

	// Check valid entry
	if (!entry)
		return nullptr;

	return format_handler_->addEntry(*this, entry, position, dir);
}

// -----------------------------------------------------------------------------
// Adds [entry] to the end of the namespace matching [add_namespace].
// Returns the added entry or NULL if the entry is invalid
// -----------------------------------------------------------------------------
shared_ptr<ArchiveEntry> Archive::addEntry(shared_ptr<ArchiveEntry> entry, string_view add_namespace)
{
	// Abort if read only
	if (read_only_)
		return nullptr;

	// Check valid entry
	if (!entry)
		return nullptr;

	return format_handler_->addEntry(*this, entry, add_namespace);
}

// -----------------------------------------------------------------------------
// Creates a new entry with [name] and adds it to [dir] at [position].
// If [dir] is null it is added to the root dir.
// If [position] is out of bounds, it is added tothe end of the dir.
// Return false if the entry is invalid, true otherwise
// -----------------------------------------------------------------------------
shared_ptr<ArchiveEntry> Archive::addNewEntry(string_view name, unsigned position, ArchiveDir* dir)
{
	// Abort if read only
	if (read_only_)
		return nullptr;

	return format_handler_->addNewEntry(*this, name, position, dir);
}

// -----------------------------------------------------------------------------
// Creates a new entry with [name] and adds it to [namespace] within the archive
// -----------------------------------------------------------------------------
shared_ptr<ArchiveEntry> Archive::addNewEntry(string_view name, string_view add_namespace)
{
	// Abort if read only
	if (read_only_)
		return nullptr;

	// Create the new entry
	auto entry = std::make_shared<ArchiveEntry>(name);

	// Add it to the archive
	addEntry(entry, add_namespace);

	// Return the newly created entry
	return entry;
}

// -----------------------------------------------------------------------------
// Removes [entry] from the archive.
// If [delete_entry] is true, the entry will also be deleted.
// Returns true if the removal succeeded
// -----------------------------------------------------------------------------
bool Archive::removeEntry(ArchiveEntry* entry, bool set_deleted)
{
	// Abort if read only
	if (read_only_)
		return false;

	// Check entry
	if (!checkEntry(entry))
		return false;

	// Check if entry is locked
	if (entry->isLocked())
		return false;

	return format_handler_->removeEntry(*this, entry, set_deleted);
}

// -----------------------------------------------------------------------------
// Swaps the entries at [index1] and [index2] in [dir].
// If [dir] is not specified, the root dir is used.
// Returns true if the swap succeeded, false otherwise
// -----------------------------------------------------------------------------
bool Archive::swapEntries(unsigned index1, unsigned index2, const ArchiveDir* dir)
{
	// Get directory
	if (!dir)
		dir = dir_root_.get();

	// Get entries
	auto e1 = dir->entryAt(index1);
	auto e2 = dir->entryAt(index2);

	// Swap
	return format_handler_->swapEntries(*this, e1, e2);
}

// -----------------------------------------------------------------------------
// Swaps [entry1] and [entry2].
// Returns false if either entry is invalid or if both entries are not in the
// same directory, true otherwise
// -----------------------------------------------------------------------------
bool Archive::swapEntries(ArchiveEntry* entry1, ArchiveEntry* entry2)
{
	// Abort if read only
	if (read_only_)
		return false;

	// Check both entries
	if (!checkEntry(entry1) || !checkEntry(entry2))
		return false;

	// Check neither entry is locked
	if (entry1->isLocked() || entry2->isLocked())
		return false;

	return format_handler_->swapEntries(*this, entry1, entry2);
}

// -----------------------------------------------------------------------------
// Moves [entry] to [position] in [dir].
// If [dir] is null, the root dir is used.
// Returns false if the entry was invalid, true otherwise
// -----------------------------------------------------------------------------
bool Archive::moveEntry(ArchiveEntry* entry, unsigned position, ArchiveDir* dir)
{
	// Abort if read only
	if (read_only_)
		return false;

	// Check the entry
	if (!checkEntry(entry))
		return false;

	// Check if the entry is locked
	if (entry->isLocked())
		return false;

	return format_handler_->moveEntry(*this, entry, position, dir);
}

// -----------------------------------------------------------------------------
// Renames [entry] with [name].
// Returns false if the entry was invalid, true otherwise
// -----------------------------------------------------------------------------
bool Archive::renameEntry(ArchiveEntry* entry, string_view name, bool force)
{
	// Abort if read only
	if (read_only_)
		return false;

	// Check entry
	if (!checkEntry(entry))
		return false;

	// Check if entry is locked
	if (entry->isLocked())
		return false;

	// Check for directory
	if (entry->type() == EntryType::folderType())
		return renameDir(dirAtPath(entry->path(true)), name);

	return format_handler_->renameEntry(*this, entry, name, force);
}

// -----------------------------------------------------------------------------
// Imports all files (including subdirectories) from [directory] into the
// archive.
// If [ignore_hidden] is true, files and directories beginning with a '.' will
// not be imported
// -----------------------------------------------------------------------------
bool Archive::importDir(string_view directory, bool ignore_hidden, shared_ptr<ArchiveDir> base)
{
	// Get a list of all files in the directory
	vector<string> files;
	for (const auto& item : std::filesystem::recursive_directory_iterator{ directory })
		if (item.is_regular_file())
			files.push_back(item.path().string());

	// Go through files
	for (const auto& file : files)
	{
		strutil::Path fn{ strutil::replace(file, directory, "") }; // Remove directory from entry name

		// Split filename into dir+name
		const auto ename = fn.fileName();
		auto       edir  = fn.path();

		// Ignore if hidden
		if (ignore_hidden)
		{
			// Hidden file
			if (strutil::startsWith(ename, '.'))
				continue;

			// Check if within hidden dir
			bool hidden = false;
			for (const auto dir : fn.pathParts())
				if (strutil::startsWith(dir, '.'))
				{
					hidden = true;
					break;
				}

			if (hidden)
				continue;
		}

		// Remove beginning \ or / from dir
		if (strutil::startsWith(edir, '\\') || strutil::startsWith(edir, '/'))
			edir.remove_prefix(1);

		// Add the entry
		auto dir   = createDir(edir, base);
		auto entry = std::make_shared<ArchiveEntry>(ename);

		// Load data
		if (entry->importFile(file))
			addEntry(entry, dir->numEntries() + 1, dir.get());
		else
			log::error(global::error);

		// Set unmodified
		entry->setState(EntryState::Unmodified);
		dir->dirEntry()->setState(EntryState::Unmodified);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Reverts [entry] to the data it contained at the last time the archive was
// saved.
// Returns false if entry was invalid, true otherwise
// -----------------------------------------------------------------------------
bool Archive::revertEntry(ArchiveEntry* entry)
{
	// Check entry
	if (!checkEntry(entry))
		return false;

	// Check if entry is locked
	if (entry->isLocked())
		return false;

	// No point if entry is unmodified or newly created
	if (entry->state() != EntryState::Modified)
		return true;

	// Reload entry data from the archive on disk
	MemChunk entry_data;
	if (loadEntryData(entry, entry_data))
	{
		entry->importMemChunk(entry_data);
		EntryType::detectEntryType(*entry);
		entry->setState(EntryState::Unmodified);
		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Returns the MapDesc information about the map beginning at [maphead].
// To be implemented in Archive sub-classes.
// -----------------------------------------------------------------------------
MapDesc Archive::mapDesc(ArchiveEntry* maphead)
{
	return format_handler_->mapDesc(*this, maphead);
}

// -----------------------------------------------------------------------------
// Returns the MapDesc information about all maps in the Archive.
// To be implemented in Archive sub-classes.
// -----------------------------------------------------------------------------
vector<MapDesc> Archive::detectMaps()
{
	return format_handler_->detectMaps(*this);
}

// -----------------------------------------------------------------------------
// Returns the namespace of the entry at [index] within [dir]
// -----------------------------------------------------------------------------
string Archive::detectNamespace(unsigned index, ArchiveDir* dir)
{
	return format_handler_->detectNamespace(*this, index, dir);
}

// -----------------------------------------------------------------------------
// Returns the namespace that [entry] is within
// -----------------------------------------------------------------------------
string Archive::detectNamespace(ArchiveEntry* entry)
{
	return format_handler_->detectNamespace(*this, entry);
}

// -----------------------------------------------------------------------------
// Returns the first entry matching the search criteria in [options], or null if
// no matching entry was found
// -----------------------------------------------------------------------------
ArchiveEntry* Archive::findFirst(ArchiveSearchOptions& options)
{
	return format_handler_->findFirst(*this, options);
}

// -----------------------------------------------------------------------------
// Returns the last entry matching the search criteria in [options], or null if
// no matching entry was found
// -----------------------------------------------------------------------------
ArchiveEntry* Archive::findLast(ArchiveSearchOptions& options)
{
	return format_handler_->findLast(*this, options);
}

// -----------------------------------------------------------------------------
// Returns a list of entries matching the search criteria in [options]
// -----------------------------------------------------------------------------
vector<ArchiveEntry*> Archive::findAll(ArchiveSearchOptions& options)
{
	return format_handler_->findAll(*this, options);
}

// -----------------------------------------------------------------------------
// Returns a list of modified entries, and set archive to unmodified status if
// the list is empty
// -----------------------------------------------------------------------------
vector<ArchiveEntry*> Archive::findModifiedEntries(ArchiveDir* dir)
{
	// Init search variables
	if (dir == nullptr)
		dir = dir_root_.get();
	vector<ArchiveEntry*> ret;

	// Search entries
	for (unsigned a = 0; a < dir->numEntries(); a++)
	{
		auto entry = dir->entryAt(a);

		// Add new and modified entries
		if (entry->state() != EntryState::Unmodified)
			ret.push_back(entry);
	}

	// Search subdirectories
	for (unsigned a = 0; a < dir->numSubdirs(); a++)
	{
		auto vec = findModifiedEntries(dir->subdirAt(a).get());
		ret.insert(ret.end(), vec.begin(), vec.end());
	}

	// If there aren't actually any, set archive to be unmodified
	if (ret.empty())
		setModified(false);

	// Return matches
	return ret;
}

// -----------------------------------------------------------------------------
// Blocks or unblocks signals for archive/entry modifications
// -----------------------------------------------------------------------------
void Archive::blockModificationSignals(bool block)
{
	if (block)
	{
		signals_.modified.block();
		signals_.entry_added.block();
		signals_.entry_removed.block();
		signals_.entry_state_changed.block();
		signals_.entry_renamed.block();
	}
	else
	{
		signals_.modified.unblock();
		signals_.entry_added.unblock();
		signals_.entry_removed.unblock();
		signals_.entry_state_changed.unblock();
		signals_.entry_renamed.unblock();
	}
}

// -----------------------------------------------------------------------------
// Detects the type of all entries in the archive
// -----------------------------------------------------------------------------
void Archive::detectAllEntryTypes() const
{
	auto entries   = dir_root_->allEntries();
	auto n_entries = entries.size();
	ui::setSplashProgressMessage("Detecting entry types");
	for (size_t i = 0; i < n_entries; i++)
	{
		ui::setSplashProgress(i, n_entries);
		EntryType::detectEntryType(*entries[i]);
		entries[i]->setState(EntryState::Unmodified);
	}
}
