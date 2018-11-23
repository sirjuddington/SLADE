
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    Archive.cpp
 * Description: Archive, the 'base' archive class (Abstract)
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
#include "Archive.h"
#include "General/UndoRedo.h"
#include "General/Clipboard.h"
#include "Utility/Parser.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Bool, archive_load_data, false, CVAR_SAVE)
CVAR(Bool, backup_archives, true, CVAR_SAVE)
bool Archive::save_backup = true;
vector<ArchiveFormat> Archive::formats;


/*******************************************************************
 * UNDO STEPS
 *******************************************************************/

class EntryRenameUS : public UndoStep
{
private:
	Archive*	archive;
	string		entry_path;
	int			entry_index;
	string		old_name;
	string		new_name;

public:
	EntryRenameUS(ArchiveEntry* entry, string new_name) : UndoStep()
	{
		archive = entry->getParent();
		entry_path = entry->getPath();
		entry_index = entry->getParentDir()->entryIndex(entry);
		old_name = entry->getName();
		this->new_name = new_name;
	}

	bool doUndo() override
	{
		// Get entry parent dir
		ArchiveTreeNode* dir = archive->getDir(entry_path);
		if (dir)
		{
			// Rename entry
			ArchiveEntry* entry = dir->entryAt(entry_index);
			return archive->renameEntry(entry, old_name);
		}

		return false;
	}

	bool doRedo() override
	{
		// Get entry parent dir
		ArchiveTreeNode* dir = archive->getDir(entry_path);
		if (dir)
		{
			// Rename entry
			ArchiveEntry* entry = dir->entryAt(entry_index);
			return archive->renameEntry(entry, new_name);
		}

		return false;
	}
};

class DirRenameUS : public UndoStep
{
private:
	Archive*	archive;
	string		path;
	string		old_name;
	string		new_name;
	int			prev_state;

public:
	DirRenameUS(ArchiveTreeNode* dir, string new_name) : UndoStep()
	{
		this->archive = dir->archive();
		this->new_name = new_name;
		this->old_name = dir->getName();
		this->path = dir->getParent()->getPath() + new_name;
		this->prev_state = dir->dirEntry()->getState();
	}

	void swapNames()
	{
		ArchiveTreeNode* dir = archive->getDir(path);
		archive->renameDir(dir, old_name);
		old_name = new_name;
		new_name = dir->getName();
		path = dir->getPath();
	}

	bool doUndo() override
	{
		swapNames();
		archive->getDir(path)->dirEntry()->setState(prev_state);
		return true;
	}

	bool doRedo() override
	{
		swapNames();
		return true;
	}
};

class EntrySwapUS : public UndoStep
{
private:
	Archive*	archive;
	string		path;
	unsigned	index1;
	unsigned	index2;

public:
	EntrySwapUS(ArchiveTreeNode* dir, unsigned index1, unsigned index2)
	{
		archive = dir->archive();
		path = dir->getPath();
		this->index1 = index1;
		this->index2 = index2;
	}

	bool doSwap() const
	{
		// Get parent dir
		ArchiveTreeNode* dir = archive->getDir(path);
		if (dir)
			return dir->swapEntries(index1, index2);
		return false;
	}

	bool doUndo() override { return doSwap(); }
	bool doRedo() override { return doSwap(); }
};

class EntryCreateDeleteUS : public UndoStep
{
private:
	bool			created;
	Archive*		archive;
	ArchiveEntry*	entry_copy;
	string			path;
	unsigned		index;

public:
	EntryCreateDeleteUS(bool created, ArchiveEntry* entry)
	{
		this->created = created;
		archive = entry->getParent();
		entry_copy = new ArchiveEntry(*entry);
		path = entry->getPath();
		index = entry->getParentDir()->entryIndex(entry);
	}

	~EntryCreateDeleteUS()
	{
		if (entry_copy)
			delete entry_copy;
	}

	bool deleteEntry() const
	{
		// Get parent dir
		ArchiveTreeNode* dir = archive->getDir(path);
		if (dir)
			return archive->removeEntry(dir->entryAt(index));
		else
			return false;
	}

	bool createEntry() const
	{
		// Get parent dir
		ArchiveTreeNode* dir = archive->getDir(path);
		if (dir)
		{
			archive->addEntry(entry_copy, index, dir, true);
			return true;
		}
		else
			return false;
	}

	bool doUndo() override
	{
		if (created)
			return deleteEntry();
		else
			return createEntry();
	}

	bool doRedo() override
	{
		if (!created)
			return deleteEntry();
		else
			return createEntry();
	}
};


class DirCreateDeleteUS : public UndoStep
{
private:
	bool					created;
	Archive*				archive;
	string					path;
	EntryTreeClipboardItem*	cb_tree;

public:
	DirCreateDeleteUS(bool created, ArchiveTreeNode* dir)
	{
		this->created = created;
		this->archive = dir->archive();
		this->path = dir->getPath();
		cb_tree = nullptr;

		if (path.StartsWith("/"))
			path.Remove(0, 1);

		// Backup child entries and subdirs if deleting
		if (!created)
		{
			// Get child entries
			vector<ArchiveEntry*> entries;
			for (unsigned a = 0; a < dir->numEntries(); a++)
				entries.push_back(dir->entryAt(a));

			// Get subdirectories
			vector<ArchiveTreeNode*> subdirs;
			for (unsigned a = 0; a < dir->nChildren(); a++)
				subdirs.push_back((ArchiveTreeNode*)dir->getChild(a));

			// Backup to clipboard item
			if (!entries.empty() || !subdirs.empty())
				cb_tree = new EntryTreeClipboardItem(entries, subdirs);
		}
	}

	~DirCreateDeleteUS()
	{
		if (cb_tree)
			delete cb_tree;
	}

	bool doUndo() override
	{
		if (created)
			return archive->removeDir(path);
		else
		{
			// Create directory
			ArchiveTreeNode* dir = archive->createDir(path);

			// Restore entries/subdirs if needed
			if (dir && cb_tree)
				dir->merge(cb_tree->getTree(), 0, 0);

			if (dir)
				dir->dirEntry()->setState(0);

			return !!dir;
		}
	}

	bool doRedo() override
	{
		if (!created)
			return archive->removeDir(path);
		else
			return archive->createDir(path) != nullptr;
	}
};


/*******************************************************************
 * ARCHIVE CLASS FUNCTIONS
 *******************************************************************/

/* Archive::Archive
 * Archive class constructor
 *******************************************************************/
Archive::Archive(string format) :
	format_{ format },
	parent_{ nullptr },
	on_disk_{ false },
	read_only_{ false },
	modified_{ true },
	dir_root_{ nullptr, this }
{
}

/* Archive::~Archive
 * Archive class destructor
 *******************************************************************/
Archive::~Archive()
{
	if (parent_)
		parent_->unlock();
}

/* Archive::formatDesc
 * Returns the ArchiveFormat descriptor for this archive
 *******************************************************************/
ArchiveFormat Archive::formatDesc() const
{
	for (auto fmt : formats)
		if (fmt.id == format_)
			return fmt;

	return ArchiveFormat("unknown");
}

/* Archive::fileExtensionString
 * Gets the wxWidgets file dialog filter string for the archive type
 *******************************************************************/
string Archive::fileExtensionString() const
{
	auto fmt = formatDesc();

	// Multiple extensions
	if (fmt.extensions.size() > 1)
	{
		string ext_all = S_FMT("Any %s File|", CHR(fmt.name));
		vector<string> ext_strings;
		for (auto ext : fmt.extensions)
		{
			string ext_case = S_FMT("*.%s;", CHR(ext.key.Lower()));
			ext_case += S_FMT("*.%s;", CHR(ext.key.Upper()));
			ext_case += S_FMT("*.%s", CHR(ext.key.Capitalize()));

			ext_all += S_FMT("%s;", CHR(ext_case));
			ext_strings.push_back(S_FMT("%s File (*.%s)|%s", CHR(ext.value), CHR(ext.key), CHR(ext_case)));
		}

		ext_all.RemoveLast(1);
		for (auto ext : ext_strings)
			ext_all += S_FMT("|%s", ext);

		return ext_all;
	}

	// Single extension
	if (fmt.extensions.size() == 1)
	{
		auto& ext = fmt.extensions[0];
		string ext_case = S_FMT("*.%s;", CHR(ext.key.Lower()));
		ext_case += S_FMT("*.%s;", CHR(ext.key.Upper()));
		ext_case += S_FMT("*.%s", CHR(ext.key.Capitalize()));

		return S_FMT("%s File (*.%s)|%s", CHR(ext.value), CHR(ext.key), CHR(ext_case));
	}

	// No extension (probably unknown type)
	return "Any File|*.*";
}

/* Archive::filename
 * Returns the archive's filename, including the path if specified
 *******************************************************************/
string Archive::filename(bool full)
{
	// If the archive is within another archive, return "<parent archive>/<entry name>"
	if (parent_)
	{
		string parent_archive = "";
		if (parentArchive())
			parent_archive = parentArchive()->filename(false) + "/";

		wxFileName fn(parent_->getName());
		return parent_archive + fn.GetName();
	}

	if (full)
		return filename_;
	else
	{
		// Get the filename without the path
		wxFileName fn(filename_);
		return fn.GetFullName();
	}
}

/* Archive::open
 * Reads an archive from disk
 * Returns true if successful, false otherwise
 *******************************************************************/
bool Archive::open(string filename)
{
	// Read the file into a MemChunk
	MemChunk mc;
	if (!mc.importFile(filename))
	{
		Global::error = "Unable to open file. Make sure it isn't in use by another program.";
		return false;
	}

	// Update filename before opening
	string backupname = this->filename_;
	this->filename_ = filename;

	// Load from MemChunk
	sf::Clock timer;
	if (open(mc))
	{
		LOG_MESSAGE(2, "Archive::open took %dms", timer.getElapsedTime().asMilliseconds());
		this->on_disk_ = true;
		return true;
	}
	else
	{
		this->filename_ = backupname;
		return false;
	}
}

/* Archive::open
 * Reads an archive from an ArchiveEntry
 * Returns true if successful, false otherwise
 *******************************************************************/
bool Archive::open(ArchiveEntry* entry)
{
	// Load from entry's data
	if (entry && open(entry->getMCData()))
	{
		// Update variables and return success
		parent_ = entry;
		parent_->lock();
		return true;
	}
	else
		return false;
}

/* Archive::setModified
 * Sets the Archive's modified status and announces the change
 *******************************************************************/
void Archive::setModified(bool modified)
{
	// Set modified
	this->modified_ = modified;

	// Announce
	announce("modified");
}

/* Archive::checkEntry
 * Checks that the given entry is valid and part of this archive
 *******************************************************************/
bool Archive::checkEntry(ArchiveEntry* entry)
{
	// Check entry is valid
	if (!entry)
		return false;

	// Check entry is part of this archive
	if (entry->getParent() != this)
		return false;

	// Entry is ok
	return true;
}

/* Archive::getEntry
 * Returns the entry matching [name] within [dir]. If no dir is given
 * the root dir is used
 *******************************************************************/
ArchiveEntry* Archive::getEntry(string name, bool cut_ext, ArchiveTreeNode* dir)
{
	// Check if dir was given
	if (!dir)
		dir = &dir_root_;	// None given, use root

	return dir->entry(name, cut_ext);
}

/* Archive::getEntry
 * Returns the entry at [index] within [dir]. If no dir is given the
 * root dir is used. Returns null if [index] is out of bounds
 *******************************************************************/
ArchiveEntry* Archive::getEntry(unsigned index, ArchiveTreeNode* dir)
{
	// Check if dir was given
	if (!dir)
		dir = &dir_root_;	// None given, use root

	return dir->entryAt(index);
}

/* Archive::entryIndex
 * Returns the index of [entry] within [dir]. IF no dir is given the
 * root dir is used. Returns -1 if [entry] doesn't exist within [dir]
 *******************************************************************/
int	Archive::entryIndex(ArchiveEntry* entry, ArchiveTreeNode* dir)
{
	// Check if dir was given
	if (!dir)
		dir = &dir_root_;	// None given, use root

	return dir->entryIndex(entry);
}

/* Archive::entryAtPath
 * Returns the entry at the given path in the archive, or null if it
 * doesn't exist
 *******************************************************************/
ArchiveEntry* Archive::entryAtPath(string path)
{
	// Remove leading / from path if needed
	if (path.StartsWith("/"))
		path.Remove(0, 1);

	// Get path as wxFileName for processing
	wxFileName fn(path);

	// Get directory from path
	ArchiveTreeNode* dir;
	if (fn.GetPath(false, wxPATH_UNIX).IsEmpty())
		dir = &dir_root_;
	else
		dir = getDir(fn.GetPath(true, wxPATH_UNIX));

	// If dir doesn't exist, return nullptr
	if (!dir)
		return nullptr;

	// Return entry
	return dir->entry(fn.GetFullName());
}

/* Archive::entryAtPath
 * Returns the entry at the given path in the archive, or null if it
 * doesn't exist
 *******************************************************************/
ArchiveEntry::SPtr Archive::entryAtPathShared(string path)
{
	// Remove leading / from path if needed
	if (path.StartsWith("/"))
		path.Remove(0, 1);

	// Get path as wxFileName for processing
	wxFileName fn(path);

	// Get directory from path
	ArchiveTreeNode* dir;
	if (fn.GetPath(false, wxPATH_UNIX).IsEmpty())
		dir = &dir_root_;
	else
		dir = getDir(fn.GetPath(true, wxPATH_UNIX));

	// If dir doesn't exist, return nullptr
	if (!dir)
		return nullptr;

	// Return entry
	return dir->sharedEntry(fn.GetFullName());
}

/* Archive::write
 * Writes the archive to a file
 * Returns true if successful, false otherwise
 *******************************************************************/
bool Archive::write(string filename, bool update)
{
	// Write to a MemChunk, then export it to a file
	MemChunk mc;
	if (write(mc, true))
		return mc.exportFile(filename);
	else
		return false;
}

/* Archive::save
 * This is the general, all-purpose 'save archive' function. Takes
 * into account whether the archive is contained within another,
 * is already on the disk, etc etc. Does a 'save as' if [filename]
 * is specified, unless the archive is contained within another.
 * Returns false if saving was unsuccessful, true otherwise
 *******************************************************************/
bool Archive::save(string filename)
{
	bool success = false;

	// Check if the archive is read-only
	if (read_only_)
	{
		Global::error = "Archive is read-only";
		return false;
	}

	// If the archive has a parent ArchiveEntry, just write it to that
	if (parent_)
	{
		success = write(parent_->getMCData());
		parent_->setState(1);
	}
	else
	{
		// Otherwise, file stuff
		if (!filename.IsEmpty())
		{
			// New filename is given (ie 'save as'), write to new file and change archive filename accordingly
			success = write(filename);
			if (success) this->filename_ = filename;

			// Update variables
			this->on_disk_ = true;
		}
		else if (!this->filename_.IsEmpty())
		{
			// No filename is given, but the archive has a filename, so overwrite it (and make a backup)

			// Create backup
			if (backup_archives && wxFileName::FileExists(this->filename_) && save_backup)
			{
				// Copy current file contents to new backup file
				string bakfile = this->filename_ + ".bak";
				LOG_MESSAGE(1, "Creating backup %s", bakfile);
				wxCopyFile(this->filename_, bakfile, true);
			}

			// Write it to the file
			success = write(this->filename_);

			// Update variables
			this->on_disk_ = true;
		}
	}

	// If saving was successful, update variables and announce save
	if (success)
	{
		setModified(false);
		announce("saved");
	}

	return success;
}

/* Archive::numEntries
 * Returns the total number of entries in the archive
 *******************************************************************/
unsigned Archive::numEntries()
{
	return dir_root_.numEntries(true);
}

/* Archive::close
 * 'Closes' the archive
 *******************************************************************/
void Archive::close()
{
	// Announce
	announce("closing");

	// Clear the root dir
	dir_root_.clear();

	// Unlock parent entry if it exists
	if (parent_)
		parent_->unlock();

	// Announce
	announce("closed");
}

/* Archive::entryStateChanged
 * Updates the archive variables and announces if necessary that an
 * entry's state has changed
 *******************************************************************/
void Archive::entryStateChanged(ArchiveEntry* entry)
{
	// Check the entry is valid and part of this archive
	if (!checkEntry(entry))
		return;

	// Get the entry index and announce the change
	MemChunk mc(8);
	wxUIntPtr ptr = wxPtrToUInt(entry);
	uint32_t index = entryIndex(entry);
	mc.write(&index, sizeof(uint32_t));
	mc.write(&ptr, sizeof(wxUIntPtr));
	announce("entry_state_changed", mc);


	// If entry was set to unmodified, don't set the archive to modified
	if (entry->getState() == 0)
		return;

	// Set the archive state to modified
	setModified(true);
}

/* Archive::getEntryTreeAsList
 * Adds the directory structure starting from [start] to [list]
 *******************************************************************/
void Archive::getEntryTreeAsList(vector<ArchiveEntry*>& list, ArchiveTreeNode* start)
{
	// If no start dir is specified, use the root dir
	if (!start)
		start = &dir_root_;

	// Add the directory entry to the list if it isn't the root dir
	if (start != &dir_root_)
		list.push_back(start->dir_entry_.get());

	// Add all entries to the list
	for (unsigned a = 0; a < start->numEntries(); a++)
		list.push_back(start->entryAt(a));

	// Go through subdirectories and add them to the list
	for (unsigned a = 0; a < start->nChildren(); a++)
		getEntryTreeAsList(list, (ArchiveTreeNode*)start->getChild(a));
}

/* Archive::getEntryTreeAsList
 * Adds the directory structure starting from [start] to [list]
 *******************************************************************/
void Archive::getEntryTreeAsList(vector<ArchiveEntry::SPtr>& list, ArchiveTreeNode* start)
{
	// If no start dir is specified, use the root dir
	if (!start)
		start = &dir_root_;

	// Add the directory entry to the list if it isn't the root dir
	if (start != &dir_root_)
		list.push_back(start->dir_entry_);

	// Add all entries to the list
	for (unsigned a = 0; a < start->numEntries(); a++)
		list.push_back(start->sharedEntryAt(a));

	// Go through subdirectories and add them to the list
	for (unsigned a = 0; a < start->nChildren(); a++)
		getEntryTreeAsList(list, (ArchiveTreeNode*)start->getChild(a));
}

/* Archive::paste
 * 'Pastes' the [tree] into the archive, with its root entries
 * starting at [position] in [base] directory. If [base] is null,
 * the root directory is used
 *******************************************************************/
bool Archive::paste(ArchiveTreeNode* tree, unsigned position, ArchiveTreeNode* base)
{
	// Check tree was given to paste
	if (!tree)
		return false;

	// Paste to root dir if none specified
	if (!base)
		base = &dir_root_;

	// Set modified
	setModified(true);

	// Just do a merge
	return base->merge(tree, position);
}

/* Archive::getDir
 * Gets the directory matching [path], starting from [base]. If
 * [base] is null, the root directory is used. Returns null if
 * the requested directory does not exist
 *******************************************************************/
ArchiveTreeNode* Archive::getDir(string path, ArchiveTreeNode* base)
{
	// Check if base dir was given
	if (!base)
	{
		// None given, use root
		base = &dir_root_;

		// If empty directory, just return the root
		if (path == "/" || path == "")
			return &dir_root_;

		// Remove starting '/'
		if (path.StartsWith("/"))
			path.Remove(0, 1);
	}

	return (ArchiveTreeNode*)base->getChild(path);
}

/* Archive::createDir
 * Creates a directory at [path], starting from [base]. If
 * [base] is null, the root directory is used. Returns the created
 * directory. If the directory requested to be created already
 * exists, it will be returned
 *******************************************************************/
ArchiveTreeNode* Archive::createDir(string path, ArchiveTreeNode* base)
{
	// Abort if read only
	if (read_only_)
		return &dir_root_;

	// If no base dir specified, set it to root
	if (!base)
		base = &dir_root_;

	if (path.IsEmpty())
		return base;

	// Create the directory
	ArchiveTreeNode* dir = (ArchiveTreeNode*)((STreeNode*)base)->addChild(path);

	// Record undo step
	if (UndoRedo::currentlyRecording())
		UndoRedo::currentManager()->recordUndoStep(new DirCreateDeleteUS(true, dir));

	// Set the archive state to modified
	setModified(true);

	// Announce
	MemChunk mc;
	wxUIntPtr ptr = wxPtrToUInt(dir);
	mc.write(&ptr, sizeof(wxUIntPtr));
	announce("directory_added", mc);

	return dir;
}

/* Archive::removeDir
 * Deletes the directory matching [path], starting from [base]. If
 * [base] is null, the root directory is used. Returns false if
 * the directory does not exist, true otherwise
 *******************************************************************/
bool Archive::removeDir(string path, ArchiveTreeNode* base)
{
	// Abort if read only
	if (read_only_)
		return false;

	// Get the dir to remove
	ArchiveTreeNode* dir = getDir(path, base);

	// Check it exists (and that it isn't the root dir)
	if (!dir || dir == &dir_root_)
		return false;

	// Record undo step
	if (UndoRedo::currentlyRecording())
		UndoRedo::currentManager()->recordUndoStep(new DirCreateDeleteUS(false, dir));

	// Remove the directory from its parent
	if (dir->getParent())
		dir->getParent()->removeChild(dir);

	// Delete the directory
	delete dir;

	// Set the archive state to modified
	setModified(true);

	return true;
}

/* Archive::renameDir
 * Renames [dir] to [new_name]. Returns false if [dir] isn't part of
 * the archive, true otherwise
 *******************************************************************/
bool Archive::renameDir(ArchiveTreeNode* dir, string new_name)
{
	// Abort if read only
	if (read_only_)
		return false;

	// Check the directory is part of this archive
	if (dir->archive() != this)
		return false;

	// Rename the directory if needed
	if (!(S_CMPNOCASE(dir->getName(), new_name)))
	{
		if (UndoRedo::currentlyRecording())
			UndoRedo::currentManager()->recordUndoStep(new DirRenameUS(dir, new_name));

		dir->setName(new_name);
		dir->dirEntry()->setState(1);
	}
	else
		return true;

	// Announce
	MemChunk mc;
	wxUIntPtr ptr = wxPtrToUInt(dir);
	mc.write(&ptr, sizeof(wxUIntPtr));
	announce("directory_modified", mc);

	// Update variables etc
	setModified(true);

	return true;
}

/* Archive::addEntry
 * Adds [entry] to [dir] at [position]. If [dir] is null it is added
 * to the root dir. If [position] is out of bounds, it is added to
 * the end of the dir. If [copy] is true, a copy of [entry] is added
 * (rather than [entry] itself). Returns the added entry, or null if
 * [entry] is invalid or the archive is read-only
 *******************************************************************/
ArchiveEntry* Archive::addEntry(ArchiveEntry* entry, unsigned position, ArchiveTreeNode* dir, bool copy)
{
	// Abort if read only
	if (read_only_)
		return nullptr;

	// Check valid entry
	if (!entry)
		return nullptr;

	// If no dir given, set it to the root dir
	if (!dir)
		dir = &dir_root_;

	// Make a copy of the entry to add if needed
	if (copy)
		entry = new ArchiveEntry(*entry);

	// Add the entry
	dir->addEntry(entry, position);

	// Update variables etc
	setModified(true);
	entry->state = 2;

	// Announce
	MemChunk mc;
	wxUIntPtr ptr = wxPtrToUInt(entry);
	mc.write(&position, sizeof(uint32_t));
	mc.write(&ptr, sizeof(wxUIntPtr));
	announce("entry_added", mc);

	// Create undo step
	if (UndoRedo::currentlyRecording())
		UndoRedo::currentManager()->recordUndoStep(new EntryCreateDeleteUS(true, entry));

	return entry;
}

/* Archive::addNewEntry
 * Creates a new entry with [name] and adds it to [dir] at [position].
 * If [dir] is null it is added to the root dir. If [position] is out
 * of bounds, it is added tothe end of the dir. Return false if the
 * entry is invalid, true otherwise
 *******************************************************************/
ArchiveEntry* Archive::addNewEntry(string name, unsigned position, ArchiveTreeNode* dir)
{
	// Abort if read only
	if (read_only_)
		return nullptr;

	// Create the new entry
	ArchiveEntry* entry = new ArchiveEntry(name);

	// Add it to the archive
	addEntry(entry, position, dir);

	// Return the newly created entry
	return entry;
}

/* Archive::addNewEntry
 * Creates a new entry with [name] and adds it to [namespace]
 * within the archive
 *******************************************************************/
ArchiveEntry* Archive::addNewEntry(string name, string add_namespace)
{
	// Abort if read only
	if (read_only_)
		return nullptr;

	// Create the new entry
	ArchiveEntry* entry = new ArchiveEntry(name);

	// Add it to the archive
	addEntry(entry, add_namespace);

	// Return the newly created entry
	return entry;
}

/* Archive::removeEntry
 * Removes [entry] from the archive. If [delete_entry] is true, the
 * entry will also be deleted. Returns true if the removal succeeded
 *******************************************************************/
bool Archive::removeEntry(ArchiveEntry* entry)
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

	// Get its directory
	ArchiveTreeNode* dir = entry->getParentDir();

	// Error if entry has no parent directory
	if (!dir)
		return false;

	// Create undo step
	if (UndoRedo::currentlyRecording())
		UndoRedo::currentManager()->recordUndoStep(new EntryCreateDeleteUS(false, entry));

	// Get the entry index
	int index = dir->entryIndex(entry);

	// Announce (before actually removing in case entry is still needed)
	MemChunk mc;
	wxUIntPtr ptr = wxPtrToUInt(entry);
	mc.write(&index, sizeof(int));
	mc.write(&ptr, sizeof(wxUIntPtr));
	announce("entry_removing", mc);

	// Remove it from its directory
	bool ok = dir->removeEntry(index);

	// If it was removed ok
	if (ok)
	{
		// Announce removed
		announce("entry_removed", mc);

		// Delete if necessary
		//if (delete_entry)
		//	delete entry;

		// Update variables etc
		setModified(true);
	}

	return ok;
}

/* Archive::swapEntries
 * Swaps the entries at [index1] and [index2] in [dir]. If [dir] is
 * not specified, the root dir is used. Returns true if the swap
 * succeeded, false otherwise
 *******************************************************************/
bool Archive::swapEntries(unsigned index1, unsigned index2, ArchiveTreeNode* dir)
{
	// Get directory
	if (!dir)
		dir = &dir_root_;

	// Check if either entry is locked
	if (dir->entryAt(index1)->isLocked() || dir->entryAt(index2)->isLocked())
		return false;

	// Create undo step
	if (UndoRedo::currentlyRecording())
		UndoRedo::currentManager()->recordUndoStep(new EntrySwapUS(dir, index1, index2));

	// Do swap
	if (dir->swapEntries(index1, index2))
	{
		// Announce the swap
		announce("entries_swapped");

		// Set modified
		setModified(true);

		return true;
	}
	else
		return false;
}

/* Archive::swapEntries
 * Swaps [entry1] and [entry2]. Returns false if either entry is
 * invalid or if both entries are not in the same directory, true
 * otherwise
 *******************************************************************/
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

	// Get their directory
	ArchiveTreeNode* dir = entry1->getParentDir();

	// Error if no dir
	if (!dir)
		return false;

	// Check they are both in the same directory
	if (entry2->getParentDir() != dir)
	{
		LOG_MESSAGE(1, "Error: Can't swap two entries in different directories");
		return false;
	}

	// Get entry indices
	int i1 = dir->entryIndex(entry1);
	int i2 = dir->entryIndex(entry2);

	// Check indices
	if (i1 < 0 || i2 < 0)
		return false;

	// Create undo step
	if (UndoRedo::currentlyRecording())
		UndoRedo::currentManager()->recordUndoStep(new EntrySwapUS(dir, i1, i2));

	// Swap entries
	dir->swapEntries(i1, i2);

	// Announce the swap
	announce("entries_swapped");

	// Set modified
	setModified(true);

	// Return success
	return true;
}

/* Archive::moveEntry
 * Moves [entry] to [position] in [dir]. If [dir] is null, the root
 * dir is used. Returns false if the entry was invalid, true
 * otherwise
 *******************************************************************/
bool Archive::moveEntry(ArchiveEntry* entry, unsigned position, ArchiveTreeNode* dir)
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

	// Get the entry's current directory
	ArchiveTreeNode* cdir = entry->getParentDir();

	// Error if no dir
	if (!cdir)
		return false;

	// If no destination dir specified, use root
	if (!dir)
		dir = &dir_root_;

	// Remove the entry from its current dir
	auto sptr = dir->sharedEntryAt(dir->entryIndex(entry)); // Get a shared pointer so it isn't deleted
	removeEntry(entry);

	// Add it to the destination dir
	addEntry(entry, position, dir);

	// Set modified
	setModified(true);

	// Return success
	return true;
}

/* Archive::renameEntry
 * Renames [entry] with [name]. Returns false if the entry was
 * invalid, true otherwise
 *******************************************************************/
bool Archive::renameEntry(ArchiveEntry* entry, string name)
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
	if (entry->getType() == EntryType::folderType())
		return renameDir(getDir(entry->getPath(true)), name);

	// Announce (before actually renaming in case old name is still needed)
	MemChunk mc;
	int index = entryIndex(entry);
	wxUIntPtr ptr = wxPtrToUInt(entry);
	mc.write(&index, sizeof(int));
	mc.write(&ptr, sizeof(wxUIntPtr));
	announce("entry_renaming", mc);

	// Create undo step
	if (UndoRedo::currentlyRecording())
		UndoRedo::currentManager()->recordUndoStep(new EntryRenameUS(entry, name));

	// Rename the entry
	entry->rename(name);

	return true;
}

/* Archive::importDir
 * Imports all files (including subdirectories) from [directory] into
 * the archive
 *******************************************************************/
bool Archive::importDir(string directory)
{
	// Get a list of all files in the directory
	wxArrayString files;
	wxDir::GetAllFiles(directory, &files);

	// Go through files
	for (unsigned a = 0; a < files.size(); a++)
	{
		string name = files[a];
		name.Replace(directory, "", false);	// Remove directory from entry name

		// Split filename into dir+name
		wxFileName fn(name);
		string ename = fn.GetFullName();
		string edir = fn.GetPath();

		// Remove beginning \ or / from dir
		if (edir.StartsWith("\\") || edir.StartsWith("/"))
			edir.Remove(0, 1);

		// Add the entry
		ArchiveTreeNode* dir = createDir(edir);
		ArchiveEntry* entry = addNewEntry(ename, dir->numEntries()+1, dir);

		// Load data
		entry->importFile(files[a]);

		// Set unmodified
		entry->setState(0);
		dir->dirEntry()->setState(0);
	}

	return true;
}

/* Archive::revertEntry
 * Reverts [entry] to the data it contained at the last time the
 * archive was saved. Returns false if entry was invalid, true
 * otherwise
 *******************************************************************/
bool Archive::revertEntry(ArchiveEntry* entry)
{
	// Check entry
	if (!checkEntry(entry))
		return false;

	// Check if entry is locked
	if (entry->isLocked())
		return false;

	// No point if entry is unmodified or newly created
	if (entry->getState() != 1)
		return true;

	// Reload entry data from the archive on disk
	entry->setState(0);
	entry->unloadData();
	if (loadEntryData(entry))
	{
		EntryType::detectEntryType(entry);
		return true;
	}
	else
		return false;
}

/* Archive::detectNamespace
 * Returns the namespace that [entry] is within
 *******************************************************************/
string Archive::detectNamespace(size_t index, ArchiveTreeNode * dir)
{
	if (dir && index < dir->numEntries())
	{
		return detectNamespace(dir->entryAt(index));
	}
	return "global";
}

string Archive::detectNamespace(ArchiveEntry* entry)
{
	// Check entry
	if (!checkEntry(entry))
		return "global";

	// If the entry is in the root dir, it's in the global namespace
	if (entry->getParentDir() == &dir_root_)
		return "global";

	// Get the entry's *first* parent directory after root (ie <root>/namespace/)
	ArchiveTreeNode* dir = entry->getParentDir();
	while (dir && dir->getParent() != &dir_root_)
		dir = (ArchiveTreeNode*)dir->getParent();

	// Namespace is the directory's name (in lowercase)
	if (dir)
		return dir->getName().Lower();
	else
		return "global"; // Error, just return global
}

/* Archive::findFirst
 * Returns the first entry matching the search criteria in [options],
 * or null if no matching entry was found
 *******************************************************************/
ArchiveEntry* Archive::findFirst(SearchOptions& options)
{
	// Init search variables
	ArchiveTreeNode* dir = options.dir;
	if (!dir) dir = &dir_root_;
	options.match_name.MakeLower();		// Force case-insensitive

	// Begin search

	// Search entries
	for (unsigned a = 0; a < dir->numEntries(); a++)
	{
		ArchiveEntry* entry = dir->entryAt(a);

		// Check type
		if (options.match_type)
		{
			if (entry->getType() == EntryType::unknownType())
			{
				if (!options.match_type->isThisType(entry))
					continue;
			}
			else if (options.match_type != entry->getType())
				continue;
		}

		// Check name
		if (!options.match_name.IsEmpty())
		{
			// Cut extension if ignoring
			wxFileName fn(entry->getName());
			if (options.ignore_ext)
			{
				if (!options.match_name.Matches(fn.GetName().MakeLower()))
					continue;
			}
			else if (!options.match_name.Matches(fn.GetFullName().MakeLower()))
				continue;
		}

		// Check namespace
		if (!options.match_namespace.IsEmpty())
		{
			if (!(S_CMPNOCASE(detectNamespace(entry), options.match_namespace)))
				continue;
		}

		// Entry passed all checks so far, so we found a match
		return entry;
	}

	// Search subdirectories (if needed)
	if (options.search_subdirs)
	{
		for (unsigned a = 0; a < dir->nChildren(); a++)
		{
			SearchOptions opt = options;
			opt.dir = (ArchiveTreeNode*)dir->getChild(a);
			ArchiveEntry* match = findFirst(opt);

			// If a match was found in this subdir, return it
			if (match)
				return match;
		}
	}

	// No matches found
	return nullptr;
}

/* Archive::findLast
 * Returns the last entry matching the search criteria in [options],
 * or null if no matching entry was found
 *******************************************************************/
ArchiveEntry* Archive::findLast(SearchOptions& options)
{
	// Init search variables
	ArchiveTreeNode* dir = options.dir;
	if (!dir) dir = &dir_root_;
	options.match_name.MakeLower();		// Force case-insensitive

	// Begin search

	// Search subdirectories (if needed) (bottom-up)
	if (options.search_subdirs)
	{
		for (int a = dir->nChildren() - 1; a >= 0; a--)
		{
			SearchOptions opt = options;
			opt.dir = (ArchiveTreeNode*)dir->getChild(a);
			ArchiveEntry* match = findLast(opt);

			// If a match was found in this subdir, return it
			if (match)
				return match;
		}
	}

	// Search entries (bottom-up)
	for (int a = dir->numEntries() - 1; a >= 0; a--)
	{
		ArchiveEntry* entry = dir->entryAt(a);

		// Check type
		if (options.match_type)
		{
			if (entry->getType() == EntryType::unknownType())
			{
				if (!options.match_type->isThisType(entry))
					continue;
			}
			else if (options.match_type != entry->getType())
				continue;
		}

		// Check name
		if (!options.match_name.IsEmpty())
		{
			// Cut extension if ignoring
			wxFileName fn(entry->getName());
			if (options.ignore_ext)
			{
				if (!options.match_name.Matches(fn.GetName().MakeLower()))
					continue;
			}
			else if (!options.match_name.Matches(fn.GetFullName().MakeLower()))
				continue;
		}

		// Check namespace
		if (!options.match_namespace.IsEmpty())
		{
			if (!(S_CMPNOCASE(detectNamespace(entry), options.match_namespace)))
				continue;
		}

		// Entry passed all checks so far, so we found a match
		return entry;
	}

	// No matches found
	return nullptr;
}

/* Archive::findAll
 * Returns a list of entries matching the search criteria in
 * [options]
 *******************************************************************/
vector<ArchiveEntry*> Archive::findAll(SearchOptions& options)
{
	// Init search variables
	ArchiveTreeNode* dir = options.dir;
	if (!dir) dir = &dir_root_;
	vector<ArchiveEntry*> ret;
	options.match_name.MakeLower();		// Force case-insensitive

	// Begin search

	// Search entries
	for (unsigned a = 0; a < dir->numEntries(); a++)
	{
		ArchiveEntry* entry = dir->entryAt(a);

		// Check type
		if (options.match_type)
		{
			if (entry->getType() == EntryType::unknownType())
			{
				if (!options.match_type->isThisType(entry))
					continue;
			}
			else if (options.match_type != entry->getType())
				continue;
		}

		// Check name
		if (!options.match_name.IsEmpty())
		{
			// Cut extension if ignoring
			wxFileName fn(entry->getName());
			if (options.ignore_ext)
			{
				if (!fn.GetName().MakeLower().Matches(options.match_name))
					continue;
			}
			else if (!fn.GetFullName().MakeLower().Matches(options.match_name))
				continue;
		}

		// Check namespace
		if (!options.match_namespace.IsEmpty())
		{
			if (!(S_CMPNOCASE(detectNamespace(entry), options.match_namespace)))
				continue;
		}

		// Entry passed all checks so far, so we found a match
		ret.push_back(entry);
	}

	// Search subdirectories (if needed)
	if (options.search_subdirs)
	{
		for (unsigned a = 0; a < dir->nChildren(); a++)
		{
			SearchOptions opt = options;
			opt.dir = (ArchiveTreeNode*)dir->getChild(a);

			// Add any matches to the list
			vector<ArchiveEntry*> vec = findAll(opt);
			ret.insert(ret.end(), vec.begin(), vec.end());
		}
	}

	// Return matches
	return ret;
}

/* Archive::findModifiedEntries
 * Returns a list of modified entries, and set archive to unmodified
 * status if the list is empty
 *******************************************************************/
vector<ArchiveEntry*> Archive::findModifiedEntries(ArchiveTreeNode* dir)
{
	// Init search variables
	if (dir == nullptr) dir = &dir_root_;
	vector<ArchiveEntry*> ret;

	// Search entries
	for (unsigned a = 0; a < dir->numEntries(); a++)
	{
		ArchiveEntry* entry = dir->entryAt(a);

		// Add new and modified entries
		if (entry->getState() != 0)
			ret.push_back(entry);
	}

	// Search subdirectories
	for (unsigned a = 0; a < dir->nChildren(); a++)
	{
		vector<ArchiveEntry*> vec = findModifiedEntries((ArchiveTreeNode*)dir->getChild(a));
		ret.insert(ret.end(), vec.begin(), vec.end());
	}

	// If there aren't actually any, set archive to be unmodified
	if (ret.size() == 0)
		setModified(false);

	// Return matches
	return ret;
}


/*******************************************************************
 * ARCHIVE CLASS STATIC FUNCTIONS
 *******************************************************************/

/* Archive::loadFormats
 * Reads archive formats configuration file from [mc]
 *******************************************************************/
bool Archive::loadFormats(MemChunk& mc)
{
	Parser parser;
	if (!parser.parseText(mc))
		return false;

	auto root = parser.parseTreeRoot();
	auto formats_node = root->getChild("archive_formats");
	for (unsigned a = 0; a < formats_node->nChildren(); a++)
	{
		auto fmt_desc = (ParseTreeNode*)formats_node->getChild(a);
		ArchiveFormat fmt{ fmt_desc->getName() };

		for (unsigned p = 0; p < fmt_desc->nChildren(); p++)
		{
			auto prop = (ParseTreeNode*)fmt_desc->getChild(p);

			// Format name
			if (S_CMPNOCASE(prop->getName(), "name"))
				fmt.name = prop->stringValue();

			// Supports dirs
			else if (S_CMPNOCASE(prop->getName(), "supports_dirs"))
				fmt.supports_dirs = prop->boolValue();

			// Entry names have extensions
			else if (S_CMPNOCASE(prop->getName(), "names_extensions"))
				fmt.names_extensions = prop->boolValue();

			// Max entry name length
			else if (S_CMPNOCASE(prop->getName(), "max_name_length"))
				fmt.max_name_length = prop->intValue();

			// Entry format (id)
			else if (S_CMPNOCASE(prop->getName(), "entry_format"))
				fmt.entry_format = prop->stringValue();

			// Extensions
			else if (S_CMPNOCASE(prop->getName(), "extensions"))
			{
				for (unsigned e = 0; e < prop->nChildren(); e++)
				{
					auto ext = (ParseTreeNode*)prop->getChild(e);
					fmt.extensions.push_back({ ext->getName(), ext->stringValue() });
				}
			}
		}

		LOG_MESSAGE(3, "Read archive format %s: \"%s\"", fmt.id, fmt.name);
		if (fmt.supports_dirs) { LOG_MESSAGE(3, "  Supports folders"); }
		if (fmt.names_extensions) { LOG_MESSAGE(3, "  Entry names have extensions"); }
		if (fmt.max_name_length >= 0) { LOG_MESSAGE(3, "  Max entry name length: %d", fmt.max_name_length); }
		for (auto ext : fmt.extensions)
			LOG_MESSAGE(3, "  Extension \"%s\" = \"%s\"", ext.key, ext.value);

		formats.push_back(fmt);
	}

	// Add builtin 'folder' format
	ArchiveFormat fmt_folder("folder");
	fmt_folder.name = "Folder";
	fmt_folder.names_extensions = true;
	fmt_folder.supports_dirs = true;
	formats.push_back(fmt_folder);

	return true;
}


/*******************************************************************
 * TREELESSARCHIVE CLASS FUNCTIONS
 *******************************************************************/

/* TreelessArchive::paste
 * Treeless version of Archive::paste. Pastes all entries in [tree]
 * and its subdirectories straight into the root dir at [position]
 *******************************************************************/
bool TreelessArchive::paste(ArchiveTreeNode* tree, unsigned position, ArchiveTreeNode* base)
{
	// Check tree was given to paste
	if (!tree)
		return false;

	// Paste root entries only
	for (unsigned a = 0; a < tree->numEntries(); a++)
	{
		// Add entry to archive
		addEntry(tree->entryAt(a), position, nullptr, true);

		// Update [position] if needed
		if (position < 0xFFFFFFFF)
			position++;
	}

	// Go through paste tree subdirs and paste their entries recursively
	for (unsigned a = 0; a < tree->nChildren(); a++)
	{
		ArchiveTreeNode* dir = (ArchiveTreeNode*)tree->getChild(a);
		paste(dir, position);
	}

	// Set modified
	setModified(true);

	return true;
}
