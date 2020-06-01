
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ArchiveDir.cpp
// Description: ArchiveDir class, representing a directory in an archive with
//              entries and sub-directories
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
#include "ArchiveDir.h"
#include "App.h"
#include "Archive.h"
#include "General/Misc.h"
#include "Utility/StringUtils.h"
#include <filesystem>

using namespace slade;


// -----------------------------------------------------------------------------
//
// ArchiveDir Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ArchiveDir class constructor
// -----------------------------------------------------------------------------
ArchiveDir::ArchiveDir(string_view name, const shared_ptr<ArchiveDir>& parent, Archive* archive) :
	archive_{ archive },
	parent_dir_{ parent }
{
	// Init dir entry
	dir_entry_          = std::make_unique<ArchiveEntry>(name);
	dir_entry_->type_   = EntryType::folderType();
	dir_entry_->parent_ = parent.get();

	if (parent)
		allow_duplicate_names_ = parent->allow_duplicate_names_;
}

// -----------------------------------------------------------------------------
// Returns the directory's name
// -----------------------------------------------------------------------------
const string& ArchiveDir::name() const
{
	static string no_name = "ERROR";

	// Check dir entry exists
	if (!dir_entry_)
		return no_name;

	return dir_entry_->name();
}

// -----------------------------------------------------------------------------
// Returns the path to this directory, including trailing /.
// If [include_name] is true, this directory's name is included with the path
// -----------------------------------------------------------------------------
string ArchiveDir::path(bool include_name) const
{
	auto parent = parent_dir_.lock();
	if (include_name)
		return parent ? fmt::format("{}{}/", parent->path(), name()) : name() + "/";
	else
		return parent ? parent->path() : "/";
}

// -----------------------------------------------------------------------------
// Sets this directory's parent [archive], and all subdirs recursively
// -----------------------------------------------------------------------------
void ArchiveDir::setArchive(Archive* archive)
{
	archive_ = archive;
	for (const auto& subdir : subdirs_)
		subdir->setArchive(archive);
}

// -----------------------------------------------------------------------------
// Returns the index of [entry] within this directory, or -1 if the entry
// doesn't exist
// -----------------------------------------------------------------------------
int ArchiveDir::entryIndex(ArchiveEntry* entry, size_t startfrom) const
{
	// Check entry was given
	if (!entry)
		return -1;

	// Search for it
	const size_t size = entries_.size();
	if (entry->index_guess_ < startfrom || entry->index_guess_ >= size)
	{
		for (auto a = startfrom; a < size; a++)
		{
			if (entries_[a].get() == entry)
			{
				entry->index_guess_ = a;
				return (int)a;
			}
		}
	}
	else
	{
		for (auto a = entry->index_guess_; a < size; a++)
		{
			if (entries_[a].get() == entry)
			{
				entry->index_guess_ = a;
				return (int)a;
			}
		}
		for (auto a = startfrom; a < entry->index_guess_; a++)
		{
			if (entries_[a].get() == entry)
			{
				entry->index_guess_ = a;
				return (int)a;
			}
		}
	}

	// Not found
	return -1;
}

// -----------------------------------------------------------------------------
// Returns a flat list of all entries in this directory, including entries in
// subdirectories (recursively)
// -----------------------------------------------------------------------------
vector<shared_ptr<ArchiveEntry>> ArchiveDir::allEntries() const
{
	vector<shared_ptr<ArchiveEntry>> entries;

	// Recursive lambda function to build entry list
	std::function<void(vector<shared_ptr<ArchiveEntry>>&, ArchiveDir const*)> build_list =
		[&](vector<shared_ptr<ArchiveEntry>>& list, ArchiveDir const* dir) {
			for (const auto& subdir : dir->subdirs_)
				build_list(list, subdir.get());

			for (auto& entry : dir->entries_)
				list.push_back(entry);
		};

	build_list(entries, this);

	return entries;
}

// -----------------------------------------------------------------------------
// Returns the entry at [index] in this directory, or null if [index] is out of
// bounds
// -----------------------------------------------------------------------------
ArchiveEntry* ArchiveDir::entryAt(unsigned index) const
{
	// Check index
	if (index >= entries_.size())
		return nullptr;

	return entries_[index].get();
}

// -----------------------------------------------------------------------------
// Returns a shared pointer to the entry at [index] in this directory, or null
// if [index] is out of bounds
// -----------------------------------------------------------------------------
shared_ptr<ArchiveEntry> ArchiveDir::sharedEntryAt(unsigned index) const
{
	// Check index
	if (index >= entries_.size())
		return nullptr;

	return entries_[index];
}

// -----------------------------------------------------------------------------
// Returns the entry matching [name] in this directory, or null if no entries
// match
// -----------------------------------------------------------------------------
ArchiveEntry* ArchiveDir::entry(string_view name, bool cut_ext) const
{
	// Check name was given
	if (name.empty())
		return nullptr;

	// Go through entries
	for (auto& entry : entries_)
	{
		// Check for (non-case-sensitive) name match
		if (strutil::equalCI(cut_ext ? entry->nameNoExt() : entry->name(), name))
			return entry.get();
	}

	// Not found
	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns a shared pointer to the entry matching [name] in this directory, or
// null if no entries match
// -----------------------------------------------------------------------------
shared_ptr<ArchiveEntry> ArchiveDir::sharedEntry(string_view name, bool cut_ext) const
{
	// Check name was given
	if (name.empty())
		return nullptr;

	// Go through entries
	for (auto& entry : entries_)
	{
		// Check for (non-case-sensitive) name match
		if (strutil::equalCI(cut_ext ? entry->nameNoExt() : entry->name(), name))
			return entry;
	}

	// Not found
	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns a shared pointer to [entry] in this directory, or null if no entries
// match
// -----------------------------------------------------------------------------
shared_ptr<ArchiveEntry> ArchiveDir::sharedEntry(ArchiveEntry* entry) const
{
	// Find entry
	for (auto& e : entries_)
		if (entry == e.get())
			return e;

	// Not in this ArchiveDir
	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the number of entries in this directory
// -----------------------------------------------------------------------------
unsigned ArchiveDir::numEntries(bool inc_subdirs) const
{
	if (!inc_subdirs)
		return entries_.size();
	else
	{
		unsigned count = entries_.size();
		for (auto& subdir : subdirs_)
			count += subdir->numEntries(true);

		return count;
	}
}

// -----------------------------------------------------------------------------
// Adds [entry] to this directory at [index], or at the end if [index] is out
// of bounds
// -----------------------------------------------------------------------------
bool ArchiveDir::addEntry(shared_ptr<ArchiveEntry> entry, unsigned index)
{
	// Check entry
	if (!entry)
		return false;

	// Set entry's parent to this dir
	if (entry->parent_)
		entry->parent_->removeEntry(entry->index());
	entry->parent_ = this;

	// Check index
	if (index >= entries_.size())
		entries_.push_back(entry); // 'Invalid' index, add to end of list
	else
		entries_.insert(entries_.begin() + index, entry); // Add it at index

	// Check entry name if duplicate names aren't allowed
	if (!allow_duplicate_names_)
		ensureUniqueName(entry.get());

	return true;
}

// -----------------------------------------------------------------------------
// Removes the entry at [index] in this directory . Returns false if [index]
// was out of bounds, true otherwise
// -----------------------------------------------------------------------------
bool ArchiveDir::removeEntry(unsigned index)
{
	// Check index
	if (index >= entries_.size())
		return false;

	// De-parent entry
	entries_[index]->parent_ = nullptr;

	// Remove it from the entry list
	entries_.erase(entries_.begin() + index);

	return true;
}

// -----------------------------------------------------------------------------
// Swaps the entry at [index1] with the entry at [index2] within this directory
// Returns false if either index was invalid, true otherwise
// -----------------------------------------------------------------------------
bool ArchiveDir::swapEntries(unsigned index1, unsigned index2)
{
	// Check indices
	if (index1 >= entries_.size() || index2 >= entries_.size() || index1 == index2)
		return false;

	// Swap entries
	entries_[index1].swap(entries_[index2]);

	return true;
}

// -----------------------------------------------------------------------------
// Returns the subdirectory matching [name], or nullptr if none found.
// Name checking is case-insensitive and only checks direct subdirs
// -----------------------------------------------------------------------------
shared_ptr<ArchiveDir> ArchiveDir::subdir(string_view name)
{
	if (name.back() == '/')
		name.remove_suffix(1);

	for (auto&& dir : subdirs_)
		if (strutil::equalCI(dir->name(), name))
			return dir;

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the subdirectory at [index], or nullptr if [index] is out of bounds
// -----------------------------------------------------------------------------
shared_ptr<ArchiveDir> ArchiveDir::subdirAt(unsigned index)
{
	if (index < subdirs_.size())
		return subdirs_[index];

	return nullptr;
}

// -----------------------------------------------------------------------------
// Adds [subdir] to this directory at [index].
// If [index] is out of bounds the [subdir] will be added at the end.
// The given [subdir] must have this directory as its parent
// -----------------------------------------------------------------------------
bool ArchiveDir::addSubdir(shared_ptr<ArchiveDir> subdir, unsigned index)
{
	// Some checks
	if (!subdir)
	{
		log::warning("Attempt to add null subdir to dir \"{}\"", name());
		return false;
	}
	if (subdir->parent_dir_.lock().get() != this)
	{
		log::warning("Can't add subdir \"{}\" to dir \"{}\" - it is not the parent", subdir->name(), name());
		return false;
	}

	if (index >= subdirs_.size())
		subdirs_.push_back(subdir);
	else
		subdirs_.insert(subdirs_.begin() + index, subdir);

	// Copy some properties to the subdir
	subdir->archive_               = archive_;
	subdir->allow_duplicate_names_ = allow_duplicate_names_;

	return true;
}

// -----------------------------------------------------------------------------
// Removes the subdirectory matching [name] from this directory and returns it.
// Name checking is case-insensitive, and only checks direct subdirs
// -----------------------------------------------------------------------------
shared_ptr<ArchiveDir> ArchiveDir::removeSubdir(string_view name)
{
	shared_ptr<ArchiveDir> removed;

	auto count = subdirs_.size();
	for (unsigned i = 0; i < count; ++i)
		if (strutil::equalCI(name, subdirs_[i]->name()))
		{
			removed = subdirs_[i];
			subdirs_.erase(subdirs_.begin() + i);
			break;
		}

	return removed;
}

// -----------------------------------------------------------------------------
// Removes the subdirectory at [index] from this directory and returns it.
// If [index] is out of bounds this will return nullptr
// -----------------------------------------------------------------------------
shared_ptr<ArchiveDir> ArchiveDir::removeSubdirAt(unsigned index)
{
	if (index >= subdirs_.size())
		return nullptr;

	auto removed = subdirs_[index];
	subdirs_.erase(subdirs_.begin() + index);

	return removed;
}

// -----------------------------------------------------------------------------
// Clears all entries and subdirs
// -----------------------------------------------------------------------------
void ArchiveDir::clear()
{
	entries_.clear();
	subdirs_.clear();
}

// -----------------------------------------------------------------------------
// Returns a clone of this dir.
// If [parent] is given, the clone is created with the given dir as its parent,
// instead of this dir's parent
// -----------------------------------------------------------------------------
shared_ptr<ArchiveDir> ArchiveDir::clone(shared_ptr<ArchiveDir> parent)
{
	// Create copy
	auto copy        = std::make_shared<ArchiveDir>(name(), parent ? parent : parent_dir_.lock(), archive_);
	copy->dir_entry_ = std::make_shared<ArchiveEntry>(*dir_entry_);

	// Copy entries
	for (auto& entry : entries_)
	{
		auto entry_copy = std::make_shared<ArchiveEntry>(*entry);
		copy->addEntry(entry_copy);
	}

	// Copy subdirectories
	for (auto& subdir : subdirs_)
		copy->subdirs_.push_back(subdir->clone());

	return copy;
}

// -----------------------------------------------------------------------------
// Exports all entries and subdirs to the filesystem at [path]
// -----------------------------------------------------------------------------
bool ArchiveDir::exportTo(string_view path)
{
	// Create directory if needed
	if (!std::filesystem::exists(path))
		std::filesystem::create_directory(path);

	// Export entries as files
	for (auto& entry : entries_)
	{
		// Setup entry filename
		strutil::Path fn(entry->name());
		fn.setPath(path);

		// Add file extension if it doesn't exist
		if (!fn.hasExtension())
			fn.setExtension(entry->type()->extension());

		// Do export
		entry->exportFile(fn.fullPath());
	}

	// Export subdirectories
	for (auto&& subdir : subdirs_)
		subdir->exportTo(fmt::format("{}/{}", path, subdir->name()));

	return true;
}

// -----------------------------------------------------------------------------
// Ensures [entry] has an unique name within this directory
// -----------------------------------------------------------------------------
void ArchiveDir::ensureUniqueName(ArchiveEntry* entry)
{
	unsigned      index     = 0;
	unsigned      number    = 0;
	const auto    n_entries = entries_.size();
	strutil::Path fn(entry->name());
	auto          name = fn.fileName();
	while (index < n_entries)
	{
		if (entries_[index].get() == entry)
		{
			index++;
			continue;
		}

		if (strutil::equalCI(entries_[index]->name(), name))
		{
			fn.setFileName(fmt::format("{}{}", entry->nameNoExt(), ++number));
			name  = fn.fileName();
			index = 0;
			continue;
		}

		index++;
	}

	if (number > 0)
		entry->rename(name);
}


// -----------------------------------------------------------------------------
//
// ArchiveDir Class Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the subdir at [path] within the directory [root].
// [path] can contain multiple levels to get sub-subdirs eg. 'subdir/subsub',
// and also can contain relative path parts '.' or '..', eg 'subdir/..' will
// just return [root]
// -----------------------------------------------------------------------------
shared_ptr<ArchiveDir> ArchiveDir::subdirAtPath(const shared_ptr<ArchiveDir>& root, string_view path)
{
	if (path.empty())
		return root;
	if (path.back() == '/')
		path.remove_suffix(1);
	if (path[0] == '/')
		path.remove_prefix(1);

	// Split path into parts
	auto parts = strutil::splitV(path, '/');

	// Begin trace from this dir
	auto cur_dir = root;
	for (unsigned i = 0; i < parts.size(); ++i)
	{
		if (!cur_dir)
			return nullptr; // Path doesn't exist

		if (parts[i].empty() || parts[i] == ".")
			continue; // '.' or empty path part, stay at current dir

		if (parts[i] == "..")
		{
			// '..', go up one dir
			cur_dir = cur_dir->parent_dir_.lock();
			continue;
		}

		// Go to subdir with current path part name
		cur_dir = cur_dir->subdir(parts[i]);
	}

	return cur_dir;
}

// -----------------------------------------------------------------------------
// Returns the subdir at [path] within the directory [root].
// [path] can contain multiple levels to get sub-subdirs eg. 'subdir/subsub',
// and also can contain relative path parts '.' or '..', eg 'subdir/..' will
// just return [root]
// -----------------------------------------------------------------------------
ArchiveDir* ArchiveDir::subdirAtPath(ArchiveDir* root, string_view path)
{
	if (path.empty() || path == "/")
		return root;
	if (path[0] == '/')
		path.remove_prefix(1);
	if (path.back() == '/')
		path.remove_suffix(1);

	// Split path into parts
	auto parts = strutil::splitV(path, '/');

	// Begin trace from this dir
	auto cur_dir = root;
	for (unsigned i = 0; i < parts.size(); ++i)
	{
		if (!cur_dir)
			return nullptr; // Path doesn't exist

		if (parts[i].empty() || parts[i] == ".")
			continue; // '.' or empty path part, stay at current dir

		if (parts[i] == "..")
		{
			// '..', go up one dir
			cur_dir = cur_dir->parent_dir_.lock().get();
			continue;
		}

		// Go to subdir with current path part name
		cur_dir = cur_dir->subdir(parts[i]).get();
	}

	return cur_dir;
}

// -----------------------------------------------------------------------------
// Returns the entry at [path] within the directory [root], or nullptr if none
// was found. See subdirAtPath for path rules
// -----------------------------------------------------------------------------
shared_ptr<ArchiveEntry> ArchiveDir::entryAtPath(const shared_ptr<ArchiveDir>& root, string_view path)
{
	// Find given subdir
	auto subdir = subdirAtPath(root, strutil::Path::pathOf(path, false));
	if (!subdir)
		return nullptr;

	// Return entry in subdir (if any)
	return subdir->sharedEntry(strutil::Path::fileNameOf(path));
}

// -----------------------------------------------------------------------------
// Merges [dir] into [target].
// Entries within [dir] are added at [position] within [target].
// Returns false if [dir] is invalid, true otherwise
// -----------------------------------------------------------------------------
bool ArchiveDir::merge(shared_ptr<ArchiveDir>& target, ArchiveDir* dir, unsigned position, ArchiveEntry::State state)
{
	// Check dir was given to merge
	if (!dir)
		return false;

	// Merge entries
	for (auto&& entry : dir->entries_)
	{
		auto nentry = std::make_shared<ArchiveEntry>(*entry);
		target->addEntry(nentry, position);
		nentry->setState(state, true);

		if (position < target->entries_.size())
			++position;
	}

	// Merge subdirectories
	for (auto&& merge_subdir : dir->subdirs_)
	{
		auto target_subdir = getOrCreateSubdir(target, merge_subdir->name());
		merge(target_subdir, merge_subdir.get(), -1, state);
		target_subdir->dir_entry_->setState(state, true);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Returns the subdir at [path] within the directory [root].
// If the subdir doesn't exist, it will be created (including any other subdirs
// required to get to it)
// -----------------------------------------------------------------------------
shared_ptr<ArchiveDir> ArchiveDir::getOrCreateSubdir(shared_ptr<ArchiveDir>& root, string_view path)
{
	auto subdir_name = strutil::beforeFirstV(path, '/');

	// Find subdir in root
	auto subdir = root->subdir(subdir_name);
	if (!subdir)
	{
		// Not found, create it
		subdir = std::make_shared<ArchiveDir>(subdir_name, root, root->archive_);
		root->addSubdir(subdir, -1);
	}

	// Check if there is more of [path] to follow
	auto path_rest = strutil::afterFirstV(path, '/');
	if (path_rest.empty() || path_rest == path)
		return subdir;
	else
		return getOrCreateSubdir(subdir, path_rest);
}

// -----------------------------------------------------------------------------
// Puts entries from [root] and all its subdirectories into [list] recursively.
// If [include_dir_entry] is false, the directory entry of [root] will not be
// added (directory entries for subdirs will still be added). It should only
// really be set to false if [root] is an actual root directory and has no name
// -----------------------------------------------------------------------------
void ArchiveDir::entryTreeAsList(ArchiveDir* root, vector<shared_ptr<ArchiveEntry>>& list, bool include_dir_entry)
{
	if (include_dir_entry)
		list.push_back(root->dir_entry_);

	for (const auto& entry : root->entries_)
		list.push_back(entry);

	for (const auto& subdir : root->subdirs_)
		entryTreeAsList(subdir.get(), list, true);
}

// -----------------------------------------------------------------------------
// Attempts to get a shared_ptr to [dir] via its parent or archive.
// Will return nullptr if [dir] has no parent or archive
// -----------------------------------------------------------------------------
shared_ptr<ArchiveDir> ArchiveDir::getShared(ArchiveDir* dir)
{
	auto parent = dir->parent_dir_.lock();
	if (!parent)
	{
		// If it has no parent it might be the root dir of its archive
		if (dir->archive_ && dir == dir->archive_->rootDir().get())
			return dir->archive_->rootDir();

		return nullptr;
	}

	for (const auto& subdir : parent->subdirs_)
		if (subdir.get() == dir)
			return subdir;

	return nullptr;
}

// -----------------------------------------------------------------------------
// Finds the ArchiveDir for the given directory [entry] within (and including)
// [dir_root].
// Note that in this case [entry] is the target ArchiveDir's dirEntry(), *not*
// an entry contained within it
// -----------------------------------------------------------------------------
shared_ptr<ArchiveDir> slade::ArchiveDir::findDirByDirEntry(shared_ptr<ArchiveDir> dir_root, const ArchiveEntry& entry)
{
	if (dir_root->dir_entry_.get() == &entry)
		return dir_root;

	for (auto subdir : dir_root->subdirs_)
		if (auto dir = findDirByDirEntry(subdir, entry))
			return dir;

	return nullptr;
}
