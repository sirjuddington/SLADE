
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ArchiveTreeNode.cpp
// Description: ArchiveTreeNode class, an STreeNode specialisation for handling
//              archive entries
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
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#include "ArchiveTreeNode.h"
#include "General/Misc.h"


// ----------------------------------------------------------------------------
//
// ArchiveTreeNode Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// ArchiveTreeNode::ArchiveTreeNode
//
// ArchiveTreeNode class constructor
// ----------------------------------------------------------------------------
ArchiveTreeNode::ArchiveTreeNode(ArchiveTreeNode* parent, Archive* archive) :
	STreeNode{ parent },
	archive_{ archive }
{
	// Init dir entry
	dir_entry_ = std::make_unique<ArchiveEntry>();
	dir_entry_->type = EntryType::folderType();
	dir_entry_->parent = parent;
}

// ----------------------------------------------------------------------------
// ArchiveTreeNode::~ArchiveTreeNode
//
// ArchiveTreeNode class destructor
// ----------------------------------------------------------------------------
ArchiveTreeNode::~ArchiveTreeNode()
{
}

// ----------------------------------------------------------------------------
// ArchiveTreeNode::addChild
//
// Override of STreeNode::addChild to also set the child node's directory entry
// parent to this node
// ----------------------------------------------------------------------------
void ArchiveTreeNode::addChild(STreeNode* child)
{
	// Do default child add
	STreeNode::addChild(child);

	// The child node's dir_entry should have this as parent
	((ArchiveTreeNode*)child)->dir_entry_->parent = this;
}

// ----------------------------------------------------------------------------
// ArchiveTreeNode::archive
//
// Returns the parent archive of this node (gets the parent archive of the
// *root* parent of this node)
// ----------------------------------------------------------------------------
Archive* ArchiveTreeNode::archive()
{
	if (parent)
		return ((ArchiveTreeNode*)parent)->archive();
	else
		return archive_;
}

// ----------------------------------------------------------------------------
// ArchiveTreeNode::getName
//
// Returns the node (directory) name
// ----------------------------------------------------------------------------
string ArchiveTreeNode::getName()
{
	// Check dir entry exists
	if (!dir_entry_)
		return "ERROR";

	return dir_entry_->getName();
}

// ----------------------------------------------------------------------------
// ArchiveTreeNode::entryIndex
//
// Returns the index of [entry] within this directory, or -1 if the entry
// doesn't exist
// ----------------------------------------------------------------------------
int ArchiveTreeNode::entryIndex(ArchiveEntry* entry, size_t startfrom)
{
	// Check entry was given
	if (!entry)
		return -1;

	// Search for it
	size_t size = entries_.size();
	if (entry->index_guess < startfrom || entry->index_guess >= size)
	{
		for (unsigned a = startfrom; a < size; a++)
		{
			if (entries_[a].get() == entry)
			{
				entry->index_guess = a;
				return (int)a;
			}
		}
	} else {
		for (unsigned a = entry->index_guess; a < size; a++)
		{
			if (entries_[a].get() == entry)
			{
				entry->index_guess = a;
				return (int)a;
			}
		}
		for (unsigned a = startfrom; a < entry->index_guess; a++)
		{
			if (entries_[a].get() == entry)
			{
				entry->index_guess = a;
				return (int)a;
			}
		}
	}

	// Not found
	return -1;
}

vector<ArchiveEntry::SPtr> ArchiveTreeNode::allEntries()
{
	vector<ArchiveEntry::SPtr> entries;
	std::function<void(vector<ArchiveEntry::SPtr>&, ArchiveTreeNode*)> f =
	[&](vector<ArchiveEntry::SPtr>& list, ArchiveTreeNode* dir)
	{
		for (unsigned a = 0; a < dir->nChildren(); a++)
			f(list, (ArchiveTreeNode*)dir->getChild(a));

		for (auto& entry : dir->entries_)
			list.push_back(entry);
	};
	f(entries, this);
	return entries;
}

// ----------------------------------------------------------------------------
// ArchiveTreeNode::getEntry
//
// Returns the entry at [index] in this directory, or null if [index] is out of
// bounds
// ----------------------------------------------------------------------------
ArchiveEntry* ArchiveTreeNode::entryAt(unsigned index)
{
	// Check index
	if (index >= entries_.size())
		return nullptr;

	return entries_[index].get();
}

// ----------------------------------------------------------------------------
// ArchiveTreeNode::getEntryShared
//
// Returns a shared pointer to the entry at [index] in this directory, or null
// if [index] is out of bounds
// ----------------------------------------------------------------------------
ArchiveEntry::SPtr ArchiveTreeNode::sharedEntryAt(unsigned index)
{
	// Check index
	if (index >= entries_.size())
		return nullptr;

	return entries_[index];
}

// ----------------------------------------------------------------------------
// ArchiveTreeNode::getEntry
//
// Returns the entry matching [name] in this directory, or null if no entries
// match
// ----------------------------------------------------------------------------
ArchiveEntry* ArchiveTreeNode::entry(string name, bool cut_ext)
{
	// Check name was given
	if (name == "")
		return nullptr;

	// Go through entries
	for (auto& entry : entries_)
	{
		// Check for (non-case-sensitive) name match
		if (S_CMPNOCASE(entry->getName(cut_ext), name))
			return entry.get();
	}

	// Not found
	return nullptr;
}

// ----------------------------------------------------------------------------
// ArchiveTreeNode::getEntryShared
//
// Returns a shared pointer to the entry matching [name] in this directory, or
// null if no entries match
// ----------------------------------------------------------------------------
ArchiveEntry::SPtr ArchiveTreeNode::sharedEntry(string name, bool cut_ext)
{
	// Check name was given
	if (name == "")
		return nullptr;

	// Go through entries
	for (auto& entry : entries_)
	{
		// Check for (non-case-sensitive) name match
		if (S_CMPNOCASE(entry->getName(cut_ext), name))
			return entry;
	}

	// Not found
	return nullptr;
}

// ----------------------------------------------------------------------------
// ArchiveTreeNode::getEntryShared
//
// Returns a shared pointer to [entry] in this directory, or null if no entries
// match
// ----------------------------------------------------------------------------
ArchiveEntry::SPtr ArchiveTreeNode::sharedEntry(ArchiveEntry* entry)
{
	// Find entry
	for (auto& e : entries_)
		if (entry == e.get())
			return e;

	// Not in this ArchiveTreeNode
	return nullptr;
}

// ----------------------------------------------------------------------------
// ArchiveTreeNode::numEntries
//
// Returns the number of entries in this directory
// ----------------------------------------------------------------------------
unsigned ArchiveTreeNode::numEntries(bool inc_subdirs)
{
	if (!inc_subdirs)
		return entries_.size();
	else
	{
		unsigned count = entries_.size();
		for (unsigned a = 0; a < children.size(); a++)
			count += ((ArchiveTreeNode*)children[a])->numEntries(true);

		return count;
	}
}

// ----------------------------------------------------------------------------
// ArchiveTreeNode::linkEntries
//
// Links two entries. [first] must come before [second] in the list
// ----------------------------------------------------------------------------
void ArchiveTreeNode::linkEntries(ArchiveEntry* first, ArchiveEntry* second)
{
	if (first) first->next = second;
	if (second) second->prev = first;
}

// ----------------------------------------------------------------------------
// ArchiveTreeNode::addEntry
//
// Adds [entry] to this directory at [index], or at the end if [index] is out
// of bounds
// ----------------------------------------------------------------------------
bool ArchiveTreeNode::addEntry(ArchiveEntry* entry, unsigned index)
{
	// Check entry
	if (!entry)
		return false;

	// Check index
	if (index >= entries_.size())
	{
		// 'Invalid' index, add to end of list

		// Link entry
		if (entries_.size() > 0)
		{
			entries_.back()->next = entry;
			entry->prev = entries_.back().get();
		}
		entry->next = nullptr;

		// Add it to end
		entries_.push_back(ArchiveEntry::SPtr(entry));
	}
	else
	{
		// Link entry
		if (index > 0)
		{
			entries_[index-1]->next = entry;
			entry->prev = entries_[index-1].get();
		}
		entries_[index]->prev = entry;
		entry->next = entries_[index].get();

		// Add it at index
		entries_.insert(entries_.begin() + index, ArchiveEntry::SPtr(entry));
	}

	// Set entry's parent to this node
	entry->parent = this;

	return true;
}

// ----------------------------------------------------------------------------
// ArchiveTreeNode::addEntry
//
// Adds [entry] to this directory at [index], or at the end if [index] is out
// of bounds
// ----------------------------------------------------------------------------
bool ArchiveTreeNode::addEntry(ArchiveEntry::SPtr& entry, unsigned index)
{
	// Check entry
	if (!entry)
		return false;

	// Check index
	if (index >= entries_.size())
	{
		// 'Invalid' index, add to end of list

		// Link entry
		if (entries_.size() > 0)
		{
			entries_.back()->next = entry.get();
			entry->prev = entries_.back().get();
		}
		entry->next = nullptr;

		// Add it to end
		entries_.push_back(ArchiveEntry::SPtr(entry));
	}
	else
	{
		// Link entry
		if (index > 0)
		{
			entries_[index-1]->next = entry.get();
			entry->prev = entries_[index-1].get();
		}
		entries_[index]->prev = entry.get();
		entry->next = entries_[index].get();

		// Add it at index
		entries_.insert(entries_.begin() + index, ArchiveEntry::SPtr(entry));
	}

	// Set entry's parent to this node
	entry->parent = this;

	return true;
}

// ----------------------------------------------------------------------------
// ArchiveTreeNode::removeEntry
//
// Removes the entry at [index] in this directory . Returns false if [index]
// was out of bounds, true otherwise
// ----------------------------------------------------------------------------
bool ArchiveTreeNode::removeEntry(unsigned index)
{
	// Check index
	if (index >= entries_.size())
		return false;

	// De-parent entry
	entries_[index]->parent = nullptr;

	// De-link entry
	entries_[index]->prev = nullptr;
	entries_[index]->next = nullptr;
	if (index > 0) entries_[index-1]->next = entryAt(index+1);
	if (index < entries_.size()-1) entries_[index+1]->prev = entryAt(index-1);

	// Remove it from the entry list
	entries_.erase(entries_.begin() + index);

	return true;
}

// ----------------------------------------------------------------------------
// ArchiveTreeNode::swapEntries
//
// Swaps the entry at [index1] with the entry at [index2] within this directory
// Returns false if either index was invalid, true otherwise
// ----------------------------------------------------------------------------
bool ArchiveTreeNode::swapEntries(unsigned index1, unsigned index2)
{
	// Check indices
	if (index1 >= entries_.size() || index2 >= entries_.size() || index1==index2)
		return false;

	// Get entries to swap
	ArchiveEntry* entry1 = entries_[index1].get();
	ArchiveEntry* entry2 = entries_[index2].get();

	// Swap entries
	//entries[index1] = entry2;
	//entries[index2] = entry1;
	entries_[index1].swap(entries_[index2]);

	// Update links
	linkEntries(entryAt(index1-1), entry2);
	linkEntries(entry2, entryAt(index1+1));
	linkEntries(entryAt(index2-1), entry1);
	linkEntries(entry1, entryAt(index2+1));

	return true;
}

// ----------------------------------------------------------------------------
// ArchiveTreeNode::clear
//
// Clears all entries and subdirs
// ----------------------------------------------------------------------------
void ArchiveTreeNode::clear()
{
	// Clear entries
	entries_.clear();

	// Clear subdirs
	for (unsigned a = 0; a < children.size(); a++)
		delete children[a];
	children.clear();
}

// ----------------------------------------------------------------------------
// ArchiveTreeNode::clone
//
// Returns a clone of this node
// ----------------------------------------------------------------------------
ArchiveTreeNode* ArchiveTreeNode::clone()
{
	// Create copy
	ArchiveTreeNode* copy = new ArchiveTreeNode();
	copy->setName(dir_entry_->getName());

	// Copy entries
	for (unsigned a = 0; a < entries_.size(); a++)
		copy->addEntry(new ArchiveEntry(*(entries_[a])));

	// Copy subdirectories
	for (unsigned a = 0; a < children.size(); a++)
		copy->addChild(((ArchiveTreeNode*)children[a])->clone());

	return copy;
}

// ----------------------------------------------------------------------------
// ArchiveTreeNode::merge
//
// Merges [node] with this node. Entries within [node] are added at [position]
// within this node. Returns false if [node] is invalid, true otherwise
// ----------------------------------------------------------------------------
bool ArchiveTreeNode::merge(ArchiveTreeNode* node, unsigned position, int state)
{
	// Check node was given to merge
	if (!node)
		return false;

	// Merge entries
	for (unsigned a = 0; a < node->numEntries(); a++)
	{
		if (node->entryAt(a))
		{
			string name = Misc::lumpNameToFileName(node->entryAt(a)->getName());
			node->entryAt(a)->setName(name);
		}
		ArchiveEntry* nentry = new ArchiveEntry(*(node->entryAt(a)));
		addEntry(nentry, position);
		nentry->setState(state);

		if (position < entries_.size())
			position++;
	}

	// Merge subdirectories
	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		ArchiveTreeNode* child = (ArchiveTreeNode*)STreeNode::addChild(node->getChild(a)->getName());
		child->merge((ArchiveTreeNode*)node->getChild(a));
		child->dirEntry()->setState(state);
	}

	return true;
}

// ----------------------------------------------------------------------------
// ArchiveTreeNode::exportTo
//
// Exports all entries and subdirs to the filesystem at [path]
// ----------------------------------------------------------------------------
bool ArchiveTreeNode::exportTo(string path)
{
	// Create directory if needed
	if (!wxDirExists(path))
		wxMkDir(path, 0);

	// Export entries as files
	for (unsigned a = 0; a < entries_.size(); a++)
	{
		// Setup entry filename
		wxFileName fn(entries_[a]->getName());
		fn.SetPath(path);

		// Add file extension if it doesn't exist
		if (!fn.HasExt())
			fn.SetExt(entries_[a]->getType()->getExtension());

		// Do export
		entries_[a]->exportFile(fn.GetFullPath());
	}

	// Export subdirectories
	for (unsigned a = 0; a < children.size(); a++)
		((ArchiveTreeNode*)children[a])->exportTo(path + "/" + children[a]->getName());

	return true;
}
