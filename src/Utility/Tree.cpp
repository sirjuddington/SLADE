
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Tree.cpp
// Description: STreeNode class, a generic container representing a 'node' in a
//              tree structure, where each 'node' has a name, child nodes and
//              can be subclassed to hold different data
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
#include "Tree.h"
#include "StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// STreeNode Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// STreeNode class constructor
// -----------------------------------------------------------------------------
STreeNode::STreeNode(STreeNode* parent)
{
	if (parent)
		parent->addChild(this);
	else
		parent_ = nullptr;

	// Disallow duplicate child names by default
	allow_dup_child_ = false;
}

// -----------------------------------------------------------------------------
// STreeNode class destructor
// -----------------------------------------------------------------------------
STreeNode::~STreeNode()
{
	// Delete children
	for (auto& child : children_)
		delete child;
}

// -----------------------------------------------------------------------------
// Returns the 'path' to this node, ie, the names of all its parent nodes each
// separated by a / (including the name of this node)
// -----------------------------------------------------------------------------
string STreeNode::path()
{
	return !parent_ ? name() + '/' : parent_->path() + name() + '/';
}

// -----------------------------------------------------------------------------
// Returns the child node at [index], or NULL if index is invalid
// -----------------------------------------------------------------------------
STreeNode* STreeNode::child(unsigned index) const
{
	// Check index
	if (index >= children_.size())
		return nullptr;

	return children_[index];
}

// -----------------------------------------------------------------------------
// Returns the child node matching [name].
// Can also find deeper child nodes if a path is given in [name].
// Returns null if no match is found
// -----------------------------------------------------------------------------
STreeNode* STreeNode::child(string_view name) const
{
	// Check name was given
	if (name.empty())
		return nullptr;

	// If name ends with /, remove it
	if (strutil::endsWith(name, '/') || strutil::endsWith(name, '\\'))
		name.remove_suffix(1);

	// If no directories were given
	auto first_sep = name.find_first_of("/\\");
	if (first_sep == string_view::npos)
	{
		// Find child of this node
		for (auto& child : children_)
		{
			if (strutil::equalCI(name, child->name()))
				return child;
		}

		// Child not found
		return nullptr;
	}

	// Directories were given, get the first directory
	auto dir = name.substr(0, first_sep);

	// See if it is a child of this node
	auto cnode = child(dir);
	if (cnode)
	{
		// It is, remove the first directory and continue searching that child
		name.remove_prefix(first_sep + 1);
		return cnode->child(name);
	}

	return nullptr; // Child doesn't exist
}

// -----------------------------------------------------------------------------
// Returns a list of all the node's children matching [name].
// Also handles paths as per getChild
// -----------------------------------------------------------------------------
vector<STreeNode*> STreeNode::children(string_view name) const
{
	// Init return vector
	vector<STreeNode*> ret;

	// Check name was given
	if (name.empty())
		return ret;

	// If name ends with /, remove it
	if (strutil::endsWith(name, '/') || strutil::endsWith(name, '\\'))
		name.remove_suffix(1);

	// If no directories were given
	auto first_sep = name.find_first_of("/\\");
	if (first_sep == string_view::npos)
	{
		// Find child of this node
		for (auto child : children_)
		{
			if (strutil::equalCI(name, child->name()))
				ret.push_back(child);
		}
	}
	else
	{
		// Directories were given, get the first directory
		auto dir = name.substr(0, first_sep);

		// See if it is a child of this node
		auto cnode = child({ dir.data(), dir.size() });
		if (cnode)
		{
			// It is, remove the first directory and continue searching that child
			name.remove_prefix(first_sep + 1);
			return cnode->children(name);
		}
	}

	return ret;
}

// -----------------------------------------------------------------------------
// Adds [child] to this node
// -----------------------------------------------------------------------------
void STreeNode::addChild(STreeNode* child)
{
	children_.push_back(child);
	child->parent_ = this;
}

// -----------------------------------------------------------------------------
// Creates a new child node matching [name] and adds it to the node's children.
// Also works recursively if a path is given
// -----------------------------------------------------------------------------
STreeNode* STreeNode::addChild(string_view name)
{
	// Check name was given
	if (name.empty())
		return nullptr;

	// If name ends with /, remove it
	if (strutil::endsWith(name, '/') || strutil::endsWith(name, '\\'))
		name.remove_suffix(1);

	// If no directories were given
	auto first_sep = name.find_first_of("/\\");
	if (first_sep == string_view::npos)
	{
		// If child name duplication is disallowed,
		// check if a child with this name exists
		STreeNode* cnode = nullptr;
		if (!allow_dup_child_)
			cnode = child(name);

		// If it doesn't exist, create it
		if (!cnode)
		{
			cnode = createChild(name);
			addChild(cnode);
		}

		// Return the created child
		return cnode;
	}
	else
	{
		// Directories were given, get the first directory
		auto dir = name.substr(0, first_sep);

		// If child name duplication is disallowed,
		// check if a child with this name exists
		STreeNode* cnode = nullptr;
		if (!allow_dup_child_)
			cnode = child(dir);

		// If it doesn't exist, create it
		if (!cnode)
		{
			cnode = createChild(dir);
			addChild(cnode);
		}

		// Continue adding child nodes
		name.remove_prefix(first_sep + 1);
		return cnode->addChild(name);
	}
}

// -----------------------------------------------------------------------------
// Removes [child] from this node's children.
// Returns false if [child] is not a child node, true otherwise
// -----------------------------------------------------------------------------
bool STreeNode::removeChild(STreeNode* child)
{
	// Find child node
	for (unsigned a = 0; a < children_.size(); a++)
	{
		if (children_[a] == child)
		{
			// Reset child's parent
			children_[a]->parent_ = nullptr;

			// Remove child from list
			children_.erase(children_.begin() + a);

			return true;
		}
	}

	// Child didn't exist
	return false;
}
