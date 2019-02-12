
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
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
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "ArchiveTreeNode.h"
#include "App.h"
#include "General/Misc.h"
#include "Utility/StringUtils.h"


// -----------------------------------------------------------------------------
//
// ArchiveTreeNode Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ArchiveTreeNode class constructor
// -----------------------------------------------------------------------------
ArchiveTreeNode::ArchiveTreeNode(ArchiveTreeNode* parent, Archive* archive) : STreeNode{ parent }, archive_{ archive }
{
	// Init dir entry
	dir_entry_          = std::make_unique<ArchiveEntry>();
	dir_entry_->type_   = EntryType::folderType();
	dir_entry_->parent_ = parent;

	if (parent)
		allow_duplicate_names_ = parent->allow_duplicate_names_;
}

// -----------------------------------------------------------------------------
// Override of STreeNode::addChild to also set the child node's directory entry
// parent to this node
// -----------------------------------------------------------------------------
void ArchiveTreeNode::addChild(STreeNode* child)
{
	// Do default child add
	STreeNode::addChild(child);

	// The child node's dir_entry should have this as parent
	((ArchiveTreeNode*)child)->dir_entry_->parent_ = this;
}

// -----------------------------------------------------------------------------
// Returns the parent archive of this node (gets the parent archive of the
// *root* parent of this node)
// -----------------------------------------------------------------------------
Archive* ArchiveTreeNode::archive() const
{
	if (parent_)
		return ((ArchiveTreeNode*)parent_)->archive();
	else
		return archive_;
}

// -----------------------------------------------------------------------------
// Returns the node (directory) name
// -----------------------------------------------------------------------------
wxString ArchiveTreeNode::name()
{
	// Check dir entry exists
	if (!dir_entry_)
		return "ERROR";

	return dir_entry_->name();
}

// -----------------------------------------------------------------------------
// Returns the index of [entry] within this directory, or -1 if the entry
// doesn't exist
// -----------------------------------------------------------------------------
int ArchiveTreeNode::entryIndex(ArchiveEntry* entry, size_t startfrom)
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
vector<ArchiveEntry::SPtr> ArchiveTreeNode::allEntries()
{
	vector<ArchiveEntry::SPtr> entries;

	// Recursive lambda function to build entry list
	std::function<void(vector<ArchiveEntry::SPtr>&, ArchiveTreeNode*)> build_list =
		[&](vector<ArchiveEntry::SPtr>& list, ArchiveTreeNode* dir) {
			const auto n_subdirs = dir->nChildren();
			for (unsigned a = 0; a < n_subdirs; a++)
				build_list(list, (ArchiveTreeNode*)dir->child(a));

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
ArchiveEntry* ArchiveTreeNode::entryAt(unsigned index)
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
ArchiveEntry::SPtr ArchiveTreeNode::sharedEntryAt(unsigned index)
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
ArchiveEntry* ArchiveTreeNode::entry(const wxString& name, bool cut_ext)
{
	// Check name was given
	if (name.empty())
		return nullptr;

	// Go through entries
	for (auto& entry : entries_)
	{
		// Check for (non-case-sensitive) name match
		if (StrUtil::equalCI(cut_ext ? entry->nameNoExt() : entry->name(), name.ToStdString()))
			return entry.get();
	}

	// Not found
	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns a shared pointer to the entry matching [name] in this directory, or
// null if no entries match
// -----------------------------------------------------------------------------
ArchiveEntry::SPtr ArchiveTreeNode::sharedEntry(const wxString& name, bool cut_ext)
{
	// Check name was given
	if (name.empty())
		return nullptr;

	// Go through entries
	for (auto& entry : entries_)
	{
		// Check for (non-case-sensitive) name match
		if (StrUtil::equalCI(cut_ext ? entry->nameNoExt() : entry->name(), name.ToStdString()))
			return entry;
	}

	// Not found
	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns a shared pointer to [entry] in this directory, or null if no entries
// match
// -----------------------------------------------------------------------------
ArchiveEntry::SPtr ArchiveTreeNode::sharedEntry(ArchiveEntry* entry)
{
	// Find entry
	for (auto& e : entries_)
		if (entry == e.get())
			return e;

	// Not in this ArchiveTreeNode
	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the number of entries in this directory
// -----------------------------------------------------------------------------
unsigned ArchiveTreeNode::numEntries(bool inc_subdirs)
{
	if (!inc_subdirs)
		return entries_.size();
	else
	{
		unsigned count = entries_.size();
		for (auto& subdir : children_)
			count += ((ArchiveTreeNode*)subdir)->numEntries(true);

		return count;
	}
}

// -----------------------------------------------------------------------------
// Links two entries. [first] must come before [second] in the list
// -----------------------------------------------------------------------------
void ArchiveTreeNode::linkEntries(ArchiveEntry* first, ArchiveEntry* second)
{
	if (first)
		first->next_ = second;
	if (second)
		second->prev_ = first;
}

// -----------------------------------------------------------------------------
// Adds [entry] to this directory at [index], or at the end if [index] is out
// of bounds
// -----------------------------------------------------------------------------
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
		if (!entries_.empty())
		{
			entries_.back()->next_ = entry;
			entry->prev_           = entries_.back().get();
		}
		entry->next_ = nullptr;

		// Add it to end
		entries_.push_back(ArchiveEntry::SPtr(entry));
	}
	else
	{
		// Link entry
		if (index > 0)
		{
			entries_[index - 1]->next_ = entry;
			entry->prev_               = entries_[index - 1].get();
		}
		entries_[index]->prev_ = entry;
		entry->next_           = entries_[index].get();

		// Add it at index
		entries_.insert(entries_.begin() + index, ArchiveEntry::SPtr(entry));
	}

	// Set entry's parent to this node
	entry->parent_ = this;

	// Check entry name if duplicate names aren't allowed
	if (!allow_duplicate_names_)
		ensureUniqueName(entry);

	return true;
}

// -----------------------------------------------------------------------------
// Adds [entry] to this directory at [index], or at the end if [index] is out
// of bounds
// -----------------------------------------------------------------------------
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
		if (!entries_.empty())
		{
			entries_.back()->next_ = entry.get();
			entry->prev_           = entries_.back().get();
		}
		entry->next_ = nullptr;

		// Add it to end
		entries_.push_back(ArchiveEntry::SPtr(entry));
	}
	else
	{
		// Link entry
		if (index > 0)
		{
			entries_[index - 1]->next_ = entry.get();
			entry->prev_               = entries_[index - 1].get();
		}
		entries_[index]->prev_ = entry.get();
		entry->next_           = entries_[index].get();

		// Add it at index
		entries_.insert(entries_.begin() + index, ArchiveEntry::SPtr(entry));
	}

	// Set entry's parent to this node
	entry->parent_ = this;

	// Check entry name if duplicate names aren't allowed
	if (!allow_duplicate_names_)
		ensureUniqueName(entry.get());

	return true;
}

// -----------------------------------------------------------------------------
// Removes the entry at [index] in this directory . Returns false if [index]
// was out of bounds, true otherwise
// -----------------------------------------------------------------------------
bool ArchiveTreeNode::removeEntry(unsigned index)
{
	// Check index
	if (index >= entries_.size())
		return false;

	// De-parent entry
	entries_[index]->parent_ = nullptr;

	// De-link entry
	entries_[index]->prev_ = nullptr;
	entries_[index]->next_ = nullptr;
	if (index > 0)
		entries_[index - 1]->next_ = entryAt(index + 1);
	if (index < entries_.size() - 1)
		entries_[index + 1]->prev_ = entryAt(index - 1);

	// Remove it from the entry list
	entries_.erase(entries_.begin() + index);

	return true;
}

// -----------------------------------------------------------------------------
// Swaps the entry at [index1] with the entry at [index2] within this directory
// Returns false if either index was invalid, true otherwise
// -----------------------------------------------------------------------------
bool ArchiveTreeNode::swapEntries(unsigned index1, unsigned index2)
{
	// Check indices
	if (index1 >= entries_.size() || index2 >= entries_.size() || index1 == index2)
		return false;

	// Get entries to swap
	auto entry1 = entries_[index1].get();
	auto entry2 = entries_[index2].get();

	// Swap entries
	entries_[index1].swap(entries_[index2]);

	// Update links
	linkEntries(entryAt(index1 - 1), entry2);
	linkEntries(entry2, entryAt(index1 + 1));
	linkEntries(entryAt(index2 - 1), entry1);
	linkEntries(entry1, entryAt(index2 + 1));

	return true;
}

// -----------------------------------------------------------------------------
// Clears all entries and subdirs
// -----------------------------------------------------------------------------
void ArchiveTreeNode::clear()
{
	// Clear entries
	entries_.clear();

	// Clear subdirs
	for (auto& subdir : children_)
		delete subdir;
	children_.clear();
}

// -----------------------------------------------------------------------------
// Returns a clone of this node
// -----------------------------------------------------------------------------
ArchiveTreeNode* ArchiveTreeNode::clone()
{
	// Create copy
	auto copy = new ArchiveTreeNode();
	copy->setName(dir_entry_->name());

	// Copy entries
	for (auto& entry : entries_)
		copy->addEntry(new ArchiveEntry(*entry));

	// Copy subdirectories
	for (auto& subdir : children_)
		copy->addChild(((ArchiveTreeNode*)subdir)->clone());

	return copy;
}

// -----------------------------------------------------------------------------
// Merges [node] with this node. Entries within [node] are added at [position]
// within this node. Returns false if [node] is invalid, true otherwise
// -----------------------------------------------------------------------------
bool ArchiveTreeNode::merge(ArchiveTreeNode* node, unsigned position, ArchiveEntry::State state)
{
	// Check node was given to merge
	if (!node)
		return false;

	// Merge entries
	for (unsigned a = 0; a < node->numEntries(); a++)
	{
		if (node->entryAt(a))
			node->entryAt(a)->setName(node->entryAt(a)->name());

		auto nentry = new ArchiveEntry(*(node->entryAt(a)));
		addEntry(nentry, position);
		nentry->setState(state, true);

		if (position < entries_.size())
			position++;
	}

	// Merge subdirectories
	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		auto child = dynamic_cast<ArchiveTreeNode*>(STreeNode::addChild(node->child(a)->name()));
		child->merge(dynamic_cast<ArchiveTreeNode*>(node->child(a)), 0xFFFFFFFF, state);
		child->dirEntry()->setState(state, true);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Exports all entries and subdirs to the filesystem at [path]
// -----------------------------------------------------------------------------
bool ArchiveTreeNode::exportTo(const wxString& path)
{
	// Create directory if needed
	if (!wxDirExists(path))
		wxMkdir(path);

	// Export entries as files
	for (auto& entry : entries_)
	{
		// Setup entry filename
		wxFileName fn(entry->name());
		fn.SetPath(path);

		// Add file extension if it doesn't exist
		if (!fn.HasExt())
			fn.SetExt(entry->type()->extension());

		// Do export
		entry->exportFile(fn.GetFullPath().ToStdString());
	}

	// Export subdirectories
	for (auto& subdir : children_)
		((ArchiveTreeNode*)subdir)->exportTo(path + "/" + subdir->name());

	return true;
}

// -----------------------------------------------------------------------------
// Ensures [entry] has an unique name within this directory
// -----------------------------------------------------------------------------
void ArchiveTreeNode::ensureUniqueName(ArchiveEntry* entry)
{
	unsigned      index     = 0;
	unsigned      number    = 0;
	const auto    n_entries = entries_.size();
	StrUtil::Path fn(entry->name());
	auto          name = fn.fileName();
	while (index < n_entries)
	{
		if (entries_[index].get() == entry)
		{
			index++;
			continue;
		}

		if (StrUtil::equalCI(entries_[index]->name(), name))
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
