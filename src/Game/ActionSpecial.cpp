
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ActionSpecial.cpp
// Description: ActionSpecial class, represents an action special
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
#include "ActionSpecial.h"
#include "Utility/Parser.h"
#include "Configuration.h"

using namespace Game;


// ----------------------------------------------------------------------------
//
// Variables
//
// ----------------------------------------------------------------------------
ActionSpecial ActionSpecial::unknown_;
ActionSpecial ActionSpecial::gen_switched_;
ActionSpecial ActionSpecial::gen_manual_;


// ----------------------------------------------------------------------------
//
// ActionSpecial Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// ActionSpecial::ActionSpecial
//
// ActionSpecial class constructor
// ----------------------------------------------------------------------------
ActionSpecial::ActionSpecial(string name, string group) :
	name_{ name },
	group_{ group },
	tagged_{ TagType::None },
	number_{ -1 }
{
}

// ----------------------------------------------------------------------------
// ActionSpecial::reset
//
// Resets all values to defaults
// ----------------------------------------------------------------------------
void ActionSpecial::reset()
{
	// Reset variables
	name_ = "Unknown";
	group_ = "";
	tagged_ = TagType::None;
	number_ = -1;

	// Reset args
	for (unsigned a = 0; a < 5; a++)
	{
		args_[a].name = S_FMT("Arg%d", a + 1);
		args_[a].desc = wxEmptyString;
		args_[a].type = Arg::Number;
		args_[a].custom_flags.clear();
		args_[a].custom_values.clear();
	}
}

// ----------------------------------------------------------------------------
// ActionSpecial::parse
//
// Reads an action special definition from a parsed tree [node]
// ----------------------------------------------------------------------------
void ActionSpecial::parse(ParseTreeNode* node, Arg::SpecialMap* shared_args)
{
	// Check for simple definition
	if (node->isLeaf())
	{
		name_ = node->stringValue();
		return;
	}

	// Go through all child nodes/values
	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		auto child = node->getChildPTN(a);
		string name = child->getName();
		int argn = -1;

		// Name
		if (S_CMPNOCASE(name, "name"))
			name_ = child->stringValue();

		// Args
		else if (S_CMPNOCASE(name, "arg1"))
			argn = 0;
		else if (S_CMPNOCASE(name, "arg2"))
			argn = 1;
		else if (S_CMPNOCASE(name, "arg3"))
			argn = 2;
		else if (S_CMPNOCASE(name, "arg4"))
			argn = 3;
		else if (S_CMPNOCASE(name, "arg5"))
			argn = 4;

		// Tagged
		else if (S_CMPNOCASE(name, "tagged"))
			this->tagged_ = Game::parseTagged(child);

		// Parse arg definition if it was one
		if (argn >= 0)
		{
			// Update arg count
			if (argn + 1 > args_.count)
				args_.count = argn + 1;

			args_[argn].parse(child, shared_args);
		}
	}
}

// ----------------------------------------------------------------------------
// ActionSpecial::stringDesc
//
// Returns the action special info as a string
// ----------------------------------------------------------------------------
string ActionSpecial::stringDesc() const
{
	// Init string
	string ret = S_FMT("\"%s\" in group \"%s\"", name_, group_);

	// Add tagged info
	if (tagged_ != TagType::None)
		ret += " (tagged)";
	else
		ret += " (not tagged)";

	// Add args
	ret += "\nArgs: ";
	for (unsigned a = 0; a < 5; a++)
	{
		ret += args_[a].name + ": ";

		if (args_[a].type == Arg::Number)
			ret += "Number";
		else if (args_[a].type == Arg::YesNo)
			ret += "Yes/No";
		else if (args_[a].type == Arg::NoYes)
			ret += "No/Yes";
		else if (args_[a].type == Arg::Angle)
			ret += "Angle";
		else if (args_[a].type == Arg::Choice)
			ret += "Choice";
		else
			ret += "Unknown Type";

		ret += ", ";
	}

	return ret;
}

void Game::ActionSpecial::initGlobal()
{
	gen_switched_.name_ = "Boom Generalized Switched Special";
	gen_switched_.tagged_ = TagType::Sector;

	gen_manual_.name_ = "Boom Generalized Manual Special";
	gen_manual_.tagged_ = TagType::Back;
}
