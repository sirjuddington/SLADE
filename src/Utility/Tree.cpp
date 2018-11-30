
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
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
		this->parent_ = nullptr;

	// Disallow duplicate child names by default
	allow_dup_child_ = false;
}

// -----------------------------------------------------------------------------
// STreeNode class destructor
// -----------------------------------------------------------------------------
STreeNode::~STreeNode()
{
	// Delete children
	for (unsigned a = 0; a < children_.size(); a++)
		delete children_[a];
}

// -----------------------------------------------------------------------------
// Returns the 'path' to this node, ie, the names of all its parent nodes each
// separated by a / (including the name of this node)
// -----------------------------------------------------------------------------
string STreeNode::getPath()
{
	if (!parent_)
		return getName() + "/";
	else
		return parent_->getPath() + getName() + "/";
}

// -----------------------------------------------------------------------------
// Returns the child node at [index], or NULL if index is invalid
// -----------------------------------------------------------------------------
STreeNode* STreeNode::getChild(unsigned index)
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
STreeNode* STreeNode::getChild(string name)
{
	// Check name was given
	if (name.IsEmpty())
		return nullptr;

	// If name ends with /, remove it
	if (name.EndsWith("/"))
		name.RemoveLast(1);

	// Convert name to path for processing
	wxFileName fn(name);

	// If no directories were given
	if (fn.GetDirCount() == 0)
	{
		// Find child of this node
		for (unsigned a = 0; a < children_.size(); a++)
		{
			if (S_CMPNOCASE(name, children_[a]->getName()))
				return children_[a];
		}

		// Child not found
		return nullptr;
	}
	else
	{
		// Directories were given, get the first directory
		string dir = fn.GetDirs()[0];

		// See if it is a child of this node
		STreeNode* child = getChild(dir);
		if (child)
		{
			// It is, remove the first directory and continue searching that child
			fn.RemoveDir(0);
			return child->getChild(fn.GetFullPath(wxPATH_UNIX));
		}
		else
			return nullptr; // Child doesn't exist
	}
}

// -----------------------------------------------------------------------------
// Returns a list of all the node's children matching [name].
// Also handles paths as per getChild
// -----------------------------------------------------------------------------
vector<STreeNode*> STreeNode::getChildren(string name)
{
	// Init return vector
	vector<STreeNode*> ret;

	// Check name was given
	if (name.IsEmpty())
		return ret;

	// If name ends with /, remove it
	if (name.EndsWith("/"))
		name.RemoveLast(1);

	// Convert name to path for processing
	wxFileName fn(name);

	// If no directories were given
	if (fn.GetDirCount() == 0)
	{
		// Find child of this node
		for (unsigned a = 0; a < children_.size(); a++)
		{
			if (S_CMPNOCASE(name, children_[a]->getName()))
				ret.push_back(children_[a]);
		}
	}
	else
	{
		// Directories were given, get the first directory
		string dir = fn.GetDirs()[0];

		// See if it is a child of this node
		STreeNode* child = getChild(dir);
		if (child)
		{
			// It is, remove the first directory and continue searching that child
			fn.RemoveDir(0);
			return child->getChildren(fn.GetFullPath(wxPATH_UNIX));
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
STreeNode* STreeNode::addChild(string name)
{
	// Check name was given
	if (name.IsEmpty())
		return nullptr;

	// If name ends with /, remove it
	if (name.EndsWith("/"))
		name.RemoveLast(1);

	// Convert name to path for processing
	wxFileName fn(name);

	// If no directories were given
	if (fn.GetDirCount() == 0)
	{
		// If child name duplication is disallowed,
		// check if a child with this name exists
		STreeNode* child = nullptr;
		if (!allow_dup_child_)
			child = getChild(name);

		// If it doesn't exist, create it
		if (!child)
		{
			child = createChild(name);
			addChild(child);
		}

		// Return the created child
		return child;
	}
	else
	{
		// Directories were given, get the first directory
		string dir = fn.GetDirs()[0];

		// If child name duplication is disallowed,
		// check if a child with this name exists
		STreeNode* child = nullptr;
		if (!allow_dup_child_)
			child = getChild(dir);

		// If it doesn't exist, create it
		if (!child)
		{
			child = createChild(dir);
			addChild(child);
		}

		// Continue adding child nodes
		fn.RemoveDir(0);
		return child->addChild(fn.GetFullPath(wxPATH_UNIX));
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
