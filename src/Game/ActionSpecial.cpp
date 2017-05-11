
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ActionSpecial.cpp
 * Description: ActionSpecial class, represents an action special
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
#include "ActionSpecial.h"
#include "Utility/Parser.h"
#include "GameConfiguration.h"


/*******************************************************************
 * ACTIONSPECIAL CLASS FUNCTIONS
 *******************************************************************/

/* ActionSpecial::ActionSpecial
 * ActionSpecial class constructor
 *******************************************************************/
ActionSpecial::ActionSpecial(string name, string group)
{
	// Init variables
	this->name = name;
	this->group = group;
	this->tagged = 0;
	this->arg_count = 0;

	// Init args
	args[0].name = "Arg1";
	args[1].name = "Arg2";
	args[2].name = "Arg3";
	args[3].name = "Arg4";
	args[4].name = "Arg5";
}

/* ActionSpecial::copy
 * Copies another ActionSpecial
 *******************************************************************/
void ActionSpecial::copy(ActionSpecial* copy)
{
	// Check AS to copy was given
	if (!copy) return;

	// Copy properties
	this->name = copy->name;
	this->group = copy->group;
	this->tagged = copy->tagged;
	this->arg_count = copy->arg_count;

	// Copy args
	for (unsigned a = 0; a < 5; a++)
		this->args[a] = copy->args[a];
}

/* ActionSpecial::reset
 * Resets all values to defaults
 *******************************************************************/
void ActionSpecial::reset()
{
	// Reset variables
	name = "Unknown";
	group = "";
	tagged = 0;

	// Reset args
	for (unsigned a = 0; a < 5; a++)
	{
		args[a].name = S_FMT("Arg%d", a+1);
		args[a].type = ARGT_NUMBER;
		args[a].custom_flags.clear();
		args[a].custom_values.clear();
	}
}

/* ActionSpecial::parse
 * Reads an action special definition from a parsed tree [node]
 *******************************************************************/
void ActionSpecial::parse(ParseTreeNode* node, SpecialArgMap* shared_args)
{
	// Check for simple definition
	if (node->isLeaf())
	{
		name = node->getStringValue();
		return;
	}

	// Go through all child nodes/values
	ParseTreeNode* child = NULL;
	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		child = (ParseTreeNode*)node->getChild(a);
		string name = child->getName();
		int argn = -1;

		// Name
		if (S_CMPNOCASE(name, "name"))
			this->name = child->getStringValue();

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
			this->tagged = GameConfiguration::parseTagged(child);

		// Parse arg definition if it was one
		if (argn >= 0)
		{
			// Update arg count
			if (argn + 1 > arg_count)
				arg_count = argn + 1;

			parseArg(child, shared_args, args[argn]);
		}
	}
}

/* ActionSpecial::parseArg
 * Reads an action special argument definition from a parsed tree [node]
 *******************************************************************/
void ActionSpecial::parseArg(ParseTreeNode* node, SpecialArgMap* shared_args, arg_t& arg)
{
	ParseTreeNode* custom = NULL;

	// Check for simple definition
	if (node->isLeaf())
	{
		string name = node->getStringValue();
		string shared_arg_name;

		// Names beginning with a dollar sign are references to predeclared args
		if (shared_args && name.StartsWith("$", &shared_arg_name))
		{
			SpecialArgMap::iterator it = shared_args->find(shared_arg_name);
			if (it == shared_args->end())
				// Totally bogus reference; silently ignore this arg
				return;

			arg = it->second;
		}
		else
		{
			// Set name
			arg.name = node->getStringValue();

			// Set description (if specified)
			if (node->nValues() > 1) arg.desc = node->getStringValue(1);
		}
	}
	else
	{
		// Extended arg definition

		// Name
		ParseTreeNode* val = (ParseTreeNode*)node->getChild("name");
		if (val) arg.name = val->getStringValue();

		// Description
		val = (ParseTreeNode*)node->getChild("desc");
		if (val) arg.desc = val->getStringValue();

		// Type
		val = (ParseTreeNode*)node->getChild("type");
		string atype;
		if (val) atype = val->getStringValue();
		if (S_CMPNOCASE(atype, "yesno"))
			arg.type = ARGT_YESNO;
		else if (S_CMPNOCASE(atype, "noyes"))
			arg.type = ARGT_NOYES;
		else if (S_CMPNOCASE(atype, "angle"))
			arg.type = ARGT_ANGLE;
		else if (S_CMPNOCASE(atype, "choice"))
			arg.type = ARGT_CHOICE;
		else if (S_CMPNOCASE(atype, "flags"))
			arg.type = ARGT_FLAGS;
		else if (S_CMPNOCASE(atype, "speed"))
			arg.type = ARGT_SPEED;
		else
			arg.type = ARGT_NUMBER;

		// Customs
		val = (ParseTreeNode*)node->getChild("custom_values");
		if (val) {
			for (unsigned b = 0; b < val->nChildren(); b++)
			{
				custom = (ParseTreeNode*)val->getChild(b);
				arg.custom_values.push_back(
					arg_val_t(custom->getStringValue(), wxAtoi(custom->getName())));
			}
		}

		val = (ParseTreeNode*)node->getChild("custom_flags");
		if (val) {
			for (unsigned b = 0; b < val->nChildren(); b++)
			{
				custom = (ParseTreeNode*)val->getChild(b);
				arg.custom_flags.push_back(
					arg_val_t(custom->getStringValue(), wxAtoi(custom->getName())));
			}
		}
	}
}

/* ActionSpecial::getArgsString
 * Returns a string representation of the action special's args
 * given the values in [args]
 *******************************************************************/
string ActionSpecial::getArgsString(int args[5], string argstr[2])
{
	string ret;

	// Add each arg to the string
	for (unsigned a = 0; a < 5; a++)
	{
		// Skip if the arg name is undefined and the arg value is 0
		if (args[a] == 0 && this->args[a].name.StartsWith("Arg"))
			continue;

		ret += this->args[a].name;
		ret += ": ";
		if (a < 2 && args[a] == 0 && !argstr[a].IsEmpty())
			ret += argstr[a];
		else
			ret += this->args[a].valueString(args[a]);
		ret += ", ";
	}

	// Cut ending ", "
	if (!ret.IsEmpty())
		ret.RemoveLast(2);

	return ret;
}

/* ActionSpecial::stringDesc
 * Returns the action special info as a string
 *******************************************************************/
string ActionSpecial::stringDesc()
{
	// Init string
	string ret = S_FMT("\"%s\" in group \"%s\"", name, group);

	// Add tagged info
	if (tagged)
		ret += " (tagged)";
	else
		ret += " (not tagged)";

	// Add args
	ret += "\nArgs: ";
	for (unsigned a = 0; a < 5; a++)
	{
		ret += args[a].name + ": ";

		if (args[a].type == ARGT_NUMBER)
			ret += "Number";
		else if (args[a].type == ARGT_YESNO)
			ret += "Yes/No";
		else if (args[a].type == ARGT_NOYES)
			ret += "No/Yes";
		else if (args[a].type == ARGT_ANGLE)
			ret += "Angle";
		else if (args[a].type == ARGT_CHOICE)
			ret += "Choice";
		else
			ret += "Unknown Type";

		ret += ", ";
	}

	return ret;
}
