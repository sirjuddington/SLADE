
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    STree.cpp
 * Description: STreeNode class, a generic container representing a
 *              'node' in a tree structure, where each 'node' has a
 *              name, child nodes and can be subclassed to hold
 *              different data
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
#include "Tree.h"


/*******************************************************************
 * STREENODE CLASS FUNCTIONS
 *******************************************************************/

/* STreeNode::STreeNode
 * STreeNode class constructor
 *******************************************************************/
STreeNode::STreeNode(STreeNode* parent)
{
	if (parent)
		parent->addChild(this);
	else
		this->parent = NULL;

	// Disallow duplicate child names by default
	allow_dup_child = false;
}

/* STreeNode::~STreeNode
 * STreeNode class destructor
 *******************************************************************/
STreeNode::~STreeNode()
{
	// Delete children
	for (unsigned a = 0; a < children.size(); a++)
		delete children[a];
}

/* STreeNode::getPath
 * Returns the 'path' to this node, ie, the names of all its parent
 * nodes each separated by a / (including the name of this node)
 *******************************************************************/
string STreeNode::getPath()
{
	if (!parent)
		return getName() + "/";
	else
		return parent->getPath() + getName() + "/";
}

/* STreeNode::getChild
 * Returns the child node at [index], or NULL if index is invalid
 *******************************************************************/
STreeNode* STreeNode::getChild(unsigned index)
{
	// Check index
	if (index >= children.size())
		return NULL;

	return children[index];
}

/* STreeNode::getChild
 * Returns the child node matching [name]. Can also find deeper
 * child nodes if a path is given in [name]. Returns NULL if no
 * match is found
 *******************************************************************/
STreeNode* STreeNode::getChild(string name)
{
	// Check name was given
	if (name.IsEmpty())
		return NULL;

	// If name ends with /, remove it
	if (name.EndsWith("/"))
		name.RemoveLast(1);

	// Convert name to path for processing
	wxFileName fn(name);

	// If no directories were given
	if (fn.GetDirCount() == 0)
	{
		// Find child of this node
		for (unsigned a = 0; a < children.size(); a++)
		{
			if (S_CMPNOCASE(name, children[a]->getName()))
				return children[a];
		}

		// Child not found
		return NULL;
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
			return NULL;	// Child doesn't exist
	}
}

/* STreeNode::getChildren
 * Returns a list of all the node's children matching [name]. Also
 * handles paths as per getChild
 *******************************************************************/
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
		for (unsigned a = 0; a < children.size(); a++)
		{
			if (S_CMPNOCASE(name, children[a]->getName()))
				ret.push_back(children[a]);
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

/* STreeNode::addChild
 * Adds [child] to this node
 *******************************************************************/
void STreeNode::addChild(STreeNode* child)
{
	children.push_back(child);
	child->parent = this;
}

/* STreeNode::addChild
 * Creates a new child node matching [name] and adds it to the node's
 * children. Also works recursively if a path is given
 *******************************************************************/
STreeNode* STreeNode::addChild(string name)
{
	// Check name was given
	if (name.IsEmpty())
		return NULL;

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
		STreeNode* child = NULL;
		if (!allow_dup_child)
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
		STreeNode* child = NULL;
		if (!allow_dup_child)
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

/* STreeNode::removeChild
 * Removes [child] from this node's children. Returns false if
 * [child] is not a child node, true otherwise
 *******************************************************************/
bool STreeNode::removeChild(STreeNode* child)
{
	// Find child node
	for (unsigned a = 0; a < children.size(); a++)
	{
		if (children[a] == child)
		{
			// Reset child's parent
			children[a]->parent = NULL;

			// Remove child from list
			children.erase(children.begin() + a);

			return true;
		}
	}

	// Child didn't exist
	return false;
}
