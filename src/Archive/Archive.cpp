
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
#include "MainApp.h"
#include "General/Misc.h"
#include "General/UndoRedo.h"
#include "General/Clipboard.h"

/* Archive Directory Layout:
 * ---------------------
 * [root entries]
 * [subdir 1]
 * [subdir 1 entries]
 * [subdir 1/subdir 1.1]
 * [subdir 1/subdir 1.1 entries]
 * [subdir 1/subdir 1.2]
 * [subdir 1/subdir 1.2 entries]
 * [subdir 2] (has no entries)
 * [subdir 2/subdir 2.1]
 * [subdir 2/subdir 2.1 entries]
 * etc...
 */


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Bool, archive_load_data, false, CVAR_SAVE)
bool Archive::save_backup = true;


/*******************************************************************
 * ARCHIVETREENODE CLASS FUNCTIONS
 *******************************************************************/

/* ArchiveTreeNode::ArchiveTreeNode
 * ArchiveTreeNode class constructor
 *******************************************************************/
ArchiveTreeNode::ArchiveTreeNode(ArchiveTreeNode* parent) : STreeNode(parent)
{
	// Init dir entry
	dir_entry = std::make_unique<ArchiveEntry>();
	dir_entry->type = EntryType::folderType();
	dir_entry->parent = parent;

	// Init variables
	archive = nullptr;
}

/* ArchiveTreeNode::~ArchiveTreeNode
 * ArchiveTreeNode class destructor
 *******************************************************************/
ArchiveTreeNode::~ArchiveTreeNode()
{
}

/* ArchiveTreeNode::addChild
 * Override of STreeNode::addChild to also set the child node's
 * directory entry parent to this node
 *******************************************************************/
void ArchiveTreeNode::addChild(STreeNode* child)
{
	// Do default child add
	STreeNode::addChild(child);

	// The child node's dir_entry should have this as parent
	((ArchiveTreeNode*)child)->dir_entry->parent = this;
}

/* ArchiveTreeNode::getArchive
 * Returns the parent archive of this node (gets the parent archive
 * of the *root* parent of this node)
 *******************************************************************/
Archive* ArchiveTreeNode::getArchive()
{
	if (parent)
		return ((ArchiveTreeNode*)parent)->getArchive();
	else
		return archive;
}

/* ArchiveTreeNode::getName
 * Returns the node (directory) name
 *******************************************************************/
string ArchiveTreeNode::getName()
{
	// Check dir entry exists
	if (!dir_entry)
		return "ERROR";

	return dir_entry->getName();
}

/* ArchiveTreeNode::entryIndex
 * Returns the index of [entry] within this directory, or -1 if
 * the entry doesn't exist
 *******************************************************************/
int ArchiveTreeNode::entryIndex(ArchiveEntry* entry, size_t startfrom)
{
	// Check entry was given
	if (!entry)
		return -1;

	// Search for it
	size_t size = entries.size();
	if (entry->index_guess < startfrom || entry->index_guess >= size)
	{
		for (unsigned a = startfrom; a < size; a++)
		{
			if (entries[a].get() == entry)
			{
				entry->index_guess = a;
				return (int)a;
			}
		}
	} else {
		for (unsigned a = entry->index_guess; a < size; a++)
		{
			if (entries[a].get() == entry)
			{
				entry->index_guess = a;
				return (int)a;
			}
		}
		for (unsigned a = startfrom; a < entry->index_guess; a++)
		{
			if (entries[a].get() == entry)
			{
				entry->index_guess = a;
				return (int)a;
			}
		}
	}

	// Not found
	return -1;
}

/* ArchiveTreeNode::getEntry
 * Returns the entry at [index] in this directory, or null if [index]
 * is out of bounds
 *******************************************************************/
ArchiveEntry* ArchiveTreeNode::getEntry(unsigned index)
{
	// Check index
	if (index >= entries.size())
		return nullptr;

	return entries[index].get();
}

/* ArchiveTreeNode::getEntryShared
 * Returns a shared pointer to the entry at [index] in this
 * directory, or null if [index] is out of bounds
 *******************************************************************/
ArchiveEntry::SPtr ArchiveTreeNode::getEntryShared(unsigned index)
{
	// Check index
	if (index >= entries.size())
		return nullptr;

	return entries[index];
}

/* ArchiveTreeNode::getEntry
 * Returns the entry matching [name] in this directory, or null if
 * no entries match
 *******************************************************************/
ArchiveEntry* ArchiveTreeNode::getEntry(string name, bool cut_ext)
{
	// Check name was given
	if (name == "")
		return nullptr;

	// Go through entries
	for (auto& entry : entries)
	{
		// Check for (non-case-sensitive) name match
		if (S_CMPNOCASE(entry->getName(cut_ext), name))
			return entry.get();
	}

	// Not found
	return nullptr;
}

/* ArchiveTreeNode::getEntryShared
 * Returns a shared pointer to the entry matching [name] in this
 * directory, or null if no entries match
 *******************************************************************/
ArchiveEntry::SPtr ArchiveTreeNode::getEntryShared(string name, bool cut_ext)
{
	// Check name was given
	if (name == "")
		return nullptr;

	// Go through entries
	for (auto& entry : entries)
	{
		// Check for (non-case-sensitive) name match
		if (S_CMPNOCASE(entry->getName(cut_ext), name))
			return entry;
	}

	// Not found
	return nullptr;
}

/* ArchiveTreeNode::getEntryShared
 * Returns a shared pointer to [entry] in this directory, or null if
 * no entries match
 *******************************************************************/
ArchiveEntry::SPtr ArchiveTreeNode::getEntryShared(ArchiveEntry* entry)
{
	// Find entry
	for (auto& e : entries)
		if (entry == e.get())
			return e;

	// Not in this ArchiveTreeNode
	return nullptr;
}

/* ArchiveTreeNode::numEntries
 * Returns the number of entries in this directory
 *******************************************************************/
unsigned ArchiveTreeNode::numEntries(bool inc_subdirs)
{
	if (!inc_subdirs)
		return entries.size();
	else
	{
		unsigned count = entries.size();
		for (unsigned a = 0; a < children.size(); a++)
			count += ((ArchiveTreeNode*)children[a])->numEntries(true);

		return count;
	}
}

/* ArchiveTreeNode::linkEntries
 * Links two entries. [first] must come before [second] in the list
 *******************************************************************/
void ArchiveTreeNode::linkEntries(ArchiveEntry* first, ArchiveEntry* second)
{
	if (first) first->next = second;
	if (second) second->prev = first;
}

/* ArchiveTreeNode::addEntry
 * Adds [entry] to this directory at [index], or at the end if
 * [index] is out of bounds
 *******************************************************************/
bool ArchiveTreeNode::addEntry(ArchiveEntry* entry, unsigned index)
{
	// Check entry
	if (!entry)
		return false;

	// Check index
	if (index >= entries.size())
	{
		// 'Invalid' index, add to end of list

		// Link entry
		if (entries.size() > 0)
		{
			entries.back()->next = entry;
			entry->prev = entries.back().get();
		}
		entry->next = nullptr;

		// Add it to end
		entries.push_back(ArchiveEntry::SPtr(entry));
	}
	else
	{
		// Link entry
		if (index > 0)
		{
			entries[index-1]->next = entry;
			entry->prev = entries[index-1].get();
		}
		entries[index]->prev = entry;
		entry->next = entries[index].get();

		// Add it at index
		entries.insert(entries.begin() + index, ArchiveEntry::SPtr(entry));
	}

	// Set entry's parent to this node
	entry->parent = this;

	return true;
}

/* ArchiveTreeNode::addEntry
 * Adds [entry] to this directory at [index], or at the end if
 * [index] is out of bounds
 *******************************************************************/
bool ArchiveTreeNode::addEntry(ArchiveEntry::SPtr& entry, unsigned index)
{
	// Check entry
	if (!entry)
		return false;

	// Check index
	if (index >= entries.size())
	{
		// 'Invalid' index, add to end of list

		// Link entry
		if (entries.size() > 0)
		{
			entries.back()->next = entry.get();
			entry->prev = entries.back().get();
		}
		entry->next = nullptr;

		// Add it to end
		entries.push_back(ArchiveEntry::SPtr(entry));
	}
	else
	{
		// Link entry
		if (index > 0)
		{
			entries[index-1]->next = entry.get();
			entry->prev = entries[index-1].get();
		}
		entries[index]->prev = entry.get();
		entry->next = entries[index].get();

		// Add it at index
		entries.insert(entries.begin() + index, ArchiveEntry::SPtr(entry));
	}

	// Set entry's parent to this node
	entry->parent = this;

	return true;
}

/* ArchiveTreeNode::removeEntry
 * Removes the entry at [index] in this directory . Returns false if
 * [index] was out of bounds, true otherwise
 *******************************************************************/
bool ArchiveTreeNode::removeEntry(unsigned index)
{
	// Check index
	if (index >= entries.size())
		return false;

	// De-parent entry
	entries[index]->parent = nullptr;

	// De-link entry
	entries[index]->prev = nullptr;
	entries[index]->next = nullptr;
	if (index > 0) entries[index-1]->next = getEntry(index+1);
	if (index < entries.size()-1) entries[index+1]->prev = getEntry(index-1);

	// Remove it from the entry list
	entries.erase(entries.begin() + index);

	return true;
}

/* ArchiveTreeNode::swapEntries
 * Swaps the entry at [index1] with the entry at [index2] within this
 * directory. Returns false if either index was invalid,
 * true otherwise
 *******************************************************************/
bool ArchiveTreeNode::swapEntries(unsigned index1, unsigned index2)
{
	// Check indices
	if (index1 >= entries.size() || index2 >= entries.size() || index1==index2)
		return false;

	// Get entries to swap
	ArchiveEntry* entry1 = entries[index1].get();
	ArchiveEntry* entry2 = entries[index2].get();

	// Swap entries
	//entries[index1] = entry2;
	//entries[index2] = entry1;
	entries[index1].swap(entries[index2]);

	// Update links
	linkEntries(getEntry(index1-1), entry2);
	linkEntries(entry2, getEntry(index1+1));
	linkEntries(getEntry(index2-1), entry1);
	linkEntries(entry1, getEntry(index2+1));

	return true;
}

/* ArchiveTreeNode::clone
 * Returns a clone of this node
 *******************************************************************/
ArchiveTreeNode* ArchiveTreeNode::clone()
{
	// Create copy
	ArchiveTreeNode* copy = new ArchiveTreeNode();
	copy->setName(dir_entry->getName());

	// Copy entries
	for (unsigned a = 0; a < entries.size(); a++)
		copy->addEntry(new ArchiveEntry(*(entries[a])));

	// Copy subdirectories
	for (unsigned a = 0; a < children.size(); a++)
		copy->addChild(((ArchiveTreeNode*)children[a])->clone());

	return copy;
}

/* ArchiveTreeNode::merge
 * Merges [node] with this node. Entries within [node] are added
 * at [position] within this node. Returns false if [node] is invalid,
 * true otherwise
 *******************************************************************/
bool ArchiveTreeNode::merge(ArchiveTreeNode* node, unsigned position, int state)
{
	// Check node was given to merge
	if (!node)
		return false;

	// Merge entries
	for (unsigned a = 0; a < node->numEntries(); a++)
	{
		if (node->getEntry(a))
		{
			string name = Misc::lumpNameToFileName(node->getEntry(a)->getName());
			node->getEntry(a)->setName(name);
		}
		ArchiveEntry* nentry = new ArchiveEntry(*(node->getEntry(a)));
		addEntry(nentry, position);
		nentry->setState(state);

		if (position < entries.size())
			position++;
	}

	// Merge subdirectories
	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		ArchiveTreeNode* child = (ArchiveTreeNode*)STreeNode::addChild(node->getChild(a)->getName());
		child->merge((ArchiveTreeNode*)node->getChild(a));
		child->getDirEntry()->setState(state);
	}

	return true;
}

bool ArchiveTreeNode::exportTo(string path)
{
	// Create directory if needed
	if (!wxDirExists(path))
		wxMkDir(path, 0);

	// Export entries as files
	for (unsigned a = 0; a < entries.size(); a++)
	{
		// Setup entry filename
		wxFileName fn(entries[a]->getName());
		fn.SetPath(path);

		// Add file extension if it doesn't exist
		if (!fn.HasExt())
			fn.SetExt(entries[a]->getType()->getExtension());

		// Do export
		entries[a]->exportFile(fn.GetFullPath());
	}

	// Export subdirectories
	for (unsigned a = 0; a < children.size(); a++)
		((ArchiveTreeNode*)children[a])->exportTo(path + "/" + children[a]->getName());

	return true;
}



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

	bool doUndo()
	{
		// Get entry parent dir
		ArchiveTreeNode* dir = archive->getDir(entry_path);
		if (dir)
		{
			// Rename entry
			ArchiveEntry* entry = dir->getEntry(entry_index);
			return archive->renameEntry(entry, old_name);
		}

		return false;
	}

	bool doRedo()
	{
		// Get entry parent dir
		ArchiveTreeNode* dir = archive->getDir(entry_path);
		if (dir)
		{
			// Rename entry
			ArchiveEntry* entry = dir->getEntry(entry_index);
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
		this->archive = dir->getArchive();
		this->new_name = new_name;
		this->old_name = dir->getName();
		this->path = dir->getParent()->getPath() + new_name;
		this->prev_state = dir->getDirEntry()->getState();
	}

	void swapNames()
	{
		ArchiveTreeNode* dir = archive->getDir(path);
		archive->renameDir(dir, old_name);
		old_name = new_name;
		new_name = dir->getName();
		path = dir->getPath();
	}

	bool doUndo()
	{
		swapNames();
		archive->getDir(path)->getDirEntry()->setState(prev_state);
		return true;
	}

	bool doRedo()
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
		archive = dir->getArchive();
		path = dir->getPath();
		this->index1 = index1;
		this->index2 = index2;
	}

	bool doSwap()
	{
		// Get parent dir
		ArchiveTreeNode* dir = archive->getDir(path);
		if (dir)
			return dir->swapEntries(index1, index2);
		return false;
	}

	bool doUndo() { return doSwap(); }
	bool doRedo() { return doSwap(); }
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

	bool deleteEntry()
	{
		// Get parent dir
		ArchiveTreeNode* dir = archive->getDir(path);
		if (dir)
			return archive->removeEntry(dir->getEntry(index));
		else
			return false;
	}

	bool createEntry()
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

	bool doUndo()
	{
		if (created)
			return deleteEntry();
		else
			return createEntry();
	}

	bool doRedo()
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
	unsigned				index;
	EntryTreeClipboardItem*	cb_tree;

public:
	DirCreateDeleteUS(bool created, ArchiveTreeNode* dir)
	{
		this->created = created;
		this->archive = dir->getArchive();
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
				entries.push_back(dir->getEntry(a));

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

	bool doUndo()
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

			dir->getDirEntry()->setState(0);

			return !!dir;
		}
	}

	bool doRedo()
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
Archive::Archive(uint8_t type)
{
	// Init variables
	desc.type = type;
	modified = true;
	on_disk = false;
	parent = nullptr;
	read_only = false;

	// Create root directory
	dir_root = new ArchiveTreeNode();
	dir_root->archive = this;
}

/* Archive::~Archive
 * Archive class destructor
 *******************************************************************/
Archive::~Archive()
{
	if (dir_root)
		delete dir_root;
	if (parent)
		parent->unlock();
}

/* Archive::getFilename
 * Returns the archive's filename, including the path if specified
 *******************************************************************/
string Archive::getFilename(bool full)
{
	// If the archive is within another archive, return "<parent archive>/<entry name>"
	if (parent)
	{
		string parent_archive = "";
		if (getParentArchive())
			parent_archive = getParentArchive()->getFilename(false) + "/";

		wxFileName fn(parent->getName());
		return parent_archive + fn.GetName();
	}

	if (full)
		return filename;
	else
	{
		// Get the filename without the path
		wxFileName fn(filename);
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
	string backupname = this->filename;
	this->filename = filename;

	// Load from MemChunk
	sf::Clock timer;
	if (open(mc))
	{
		LOG_MESSAGE(2, "Archive::open took %dms", timer.getElapsedTime().asMilliseconds());
		this->on_disk = true;
		return true;
	}
	else
	{
		this->filename = backupname;
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
		parent = entry;
		parent->lock();
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
	this->modified = modified;

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
		dir = dir_root;	// None given, use root

	return dir->getEntry(name, cut_ext);
}

/* Archive::getEntry
 * Returns the entry at [index] within [dir]. If no dir is given the
 * root dir is used. Returns null if [index] is out of bounds
 *******************************************************************/
ArchiveEntry* Archive::getEntry(unsigned index, ArchiveTreeNode* dir)
{
	// Check if dir was given
	if (!dir)
		dir = dir_root;	// None given, use root

	return dir->getEntry(index);
}

/* Archive::entryIndex
 * Returns the index of [entry] within [dir]. IF no dir is given the
 * root dir is used. Returns -1 if [entry] doesn't exist within [dir]
 *******************************************************************/
int	Archive::entryIndex(ArchiveEntry* entry, ArchiveTreeNode* dir)
{
	// Check if dir was given
	if (!dir)
		dir = dir_root;	// None given, use root

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
		dir = getRoot();
	else
		dir = getDir(fn.GetPath(true, wxPATH_UNIX));

	// If dir doesn't exist, return nullptr
	if (!dir)
		return nullptr;

	// Return entry
	return dir->getEntry(fn.GetFullName());
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
		dir = getRoot();
	else
		dir = getDir(fn.GetPath(true, wxPATH_UNIX));

	// If dir doesn't exist, return nullptr
	if (!dir)
		return nullptr;

	// Return entry
	return dir->getEntryShared(fn.GetFullName());
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
	if (read_only)
	{
		Global::error = "Archive is read-only";
		return false;
	}

	// If the archive has a parent ArchiveEntry, just write it to that
	if (parent)
	{
		success = write(parent->getMCData());
		parent->setState(1);
	}
	else
	{
		// Otherwise, file stuff
		if (!filename.IsEmpty())
		{
			// New filename is given (ie 'save as'), write to new file and change archive filename accordingly
			success = write(filename);
			if (success) this->filename = filename;

			// Update variables
			this->on_disk = true;
		}
		else if (!this->filename.IsEmpty())
		{
			// No filename is given, but the archive has a filename, so overwrite it (and make a backup)

			// Create backup
			if (wxFileName::FileExists(this->filename) && save_backup)
			{
				// Copy current file contents to new backup file
				string bakfile = this->filename + ".bak";
				wxLogMessage("Creating backup %s", bakfile);
				wxCopyFile(this->filename, bakfile, true);
			}

			// Write it to the file
			success = write(this->filename);

			// Update variables
			this->on_disk = true;
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
	return dir_root->numEntries(true);
}

/* Archive::close
 * 'Closes' the archive
 *******************************************************************/
void Archive::close()
{
	// Announce
	announce("closing");

	// Delete root directory
	delete dir_root;

	// Recreate root directory
	dir_root = new ArchiveTreeNode();
	dir_root->archive = this;

	// Unlock parent entry if it exists
	if (parent)
		parent->unlock();

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
		start = dir_root;

	// Add the directory entry to the list if it isn't the root dir
	if (start != dir_root)
		list.push_back(start->dir_entry.get());

	// Add all entries to the list
	for (unsigned a = 0; a < start->numEntries(); a++)
		list.push_back(start->getEntry(a));

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
		start = dir_root;

	// Add the directory entry to the list if it isn't the root dir
	if (start != dir_root)
		list.push_back(start->dir_entry);

	// Add all entries to the list
	for (unsigned a = 0; a < start->numEntries(); a++)
		list.push_back(start->getEntryShared(a));

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
		base = getRoot();

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
		base = dir_root;

		// If empty directory, just return the root
		if (path == "/" || path == "")
			return dir_root;

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
	if (read_only)
		return dir_root;

	// If no base dir specified, set it to root
	if (!base)
		base = dir_root;

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
	if (read_only)
		return false;

	// Get the dir to remove
	ArchiveTreeNode* dir = getDir(path, base);

	// Check it exists (and that it isn't the root dir)
	if (!dir || dir == getRoot())
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
	if (read_only)
		return false;

	// Check the directory is part of this archive
	if (dir->getArchive() != this)
		return false;

	// Rename the directory if needed
	if (!(S_CMPNOCASE(dir->getName(), new_name)))
	{
		if (UndoRedo::currentlyRecording())
			UndoRedo::currentManager()->recordUndoStep(new DirRenameUS(dir, new_name));

		dir->setName(new_name);
		dir->getDirEntry()->setState(1);
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
	if (read_only)
		return nullptr;

	// Check valid entry
	if (!entry)
		return nullptr;

	// If no dir given, set it to the root dir
	if (!dir)
		dir = dir_root;

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
	if (read_only)
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
	if (read_only)
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
	if (read_only)
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
		dir = dir_root;

	// Check if either entry is locked
	if (dir->getEntry(index1)->isLocked() || dir->getEntry(index2)->isLocked())
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
	if (read_only)
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
		wxLogMessage("Error: Can't swap two entries in different directories");
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
	if (read_only)
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
		dir = dir_root;

	// Remove the entry from its current dir
	auto sptr = dir->getEntryShared(dir->entryIndex(entry)); // Get a shared pointer so it isn't deleted
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
	if (read_only)
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

	// Announce modification
	entryStateChanged(entry);

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
		dir->getDirEntry()->setState(0);
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
		return detectNamespace(dir->getEntry(index));
	}
	return "global";
}

string Archive::detectNamespace(ArchiveEntry* entry)
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

/* Archive::findFirst
 * Returns the first entry matching the search criteria in [options],
 * or null if no matching entry was found
 *******************************************************************/
ArchiveEntry* Archive::findFirst(search_options_t& options)
{
	// Init search variables
	ArchiveTreeNode* dir = options.dir;
	if (!dir) dir = dir_root;
	options.match_name.MakeLower();		// Force case-insensitive

	// Begin search

	// Search entries
	for (unsigned a = 0; a < dir->numEntries(); a++)
	{
		ArchiveEntry* entry = dir->getEntry(a);

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
			search_options_t opt = options;
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
ArchiveEntry* Archive::findLast(search_options_t& options)
{
	// Init search variables
	ArchiveTreeNode* dir = options.dir;
	if (!dir) dir = dir_root;
	options.match_name.MakeLower();		// Force case-insensitive

	// Begin search

	// Search subdirectories (if needed) (bottom-up)
	if (options.search_subdirs)
	{
		for (int a = dir->nChildren() - 1; a >= 0; a--)
		{
			search_options_t opt = options;
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
		ArchiveEntry* entry = dir->getEntry(a);

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
vector<ArchiveEntry*> Archive::findAll(search_options_t& options)
{
	// Init search variables
	ArchiveTreeNode* dir = options.dir;
	if (!dir) dir = dir_root;
	vector<ArchiveEntry*> ret;
	options.match_name.MakeLower();		// Force case-insensitive

	// Begin search

	// Search entries
	for (unsigned a = 0; a < dir->numEntries(); a++)
	{
		ArchiveEntry* entry = dir->getEntry(a);

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
			search_options_t opt = options;
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
	if (dir == nullptr) dir = dir_root;
	vector<ArchiveEntry*> ret;

	// Search entries
	for (unsigned a = 0; a < dir->numEntries(); a++)
	{
		ArchiveEntry* entry = dir->getEntry(a);

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
		ArchiveEntry* entry = addEntry(tree->getEntry(a), position, nullptr, true);

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
