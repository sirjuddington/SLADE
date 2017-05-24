
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SpecialPreset.cpp
// Description: SpecialPreset struct and related functions
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
#include "SpecialPreset.h"
#include "Utility/Parser.h"

using namespace Game;


// ----------------------------------------------------------------------------
//
// SpecialPreset Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// SpecialPreset::parse
//
// Reads a special preset definition from a parsed tree [node]
// ----------------------------------------------------------------------------
void SpecialPreset::parse(ParseTreeNode* node)
{
	name = node->getName();

	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		auto child = node->getChildPTN(a);
		string name = child->getName();

		// Group
		if (S_CMPNOCASE(child->getName(), "group"))
			group = child->stringValue();

		// Special
		else if (S_CMPNOCASE(child->getName(), "special"))
			special = child->intValue();

		// Flags
		else if (S_CMPNOCASE(child->getName(), "flags"))
			for (auto& flag : child->values())
				flags.push_back(flag.getStringValue());

		// Args
		else if (S_CMPNOCASE(child->getName(), "arg1"))
			args[0] = child->intValue();
		else if (S_CMPNOCASE(child->getName(), "arg2"))
			args[1] = child->intValue();
		else if (S_CMPNOCASE(child->getName(), "arg3"))
			args[2] = child->intValue();
		else if (S_CMPNOCASE(child->getName(), "arg4"))
			args[3] = child->intValue();
		else if (S_CMPNOCASE(child->getName(), "arg5"))
			args[4] = child->intValue();
	}
}
