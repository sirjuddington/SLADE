
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ThingType.cpp
 * Description: ThingType class, represents a thing type
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
#include "ThingType.h"
#include "Utility/Parser.h"
#include "MapEditor/GameConfiguration/GameConfiguration.h"


/*******************************************************************
 * THINGTYPE CLASS FUNCTIONS
 *******************************************************************/

/* ThingType::ThingType
 * ThingType class constructor
 *******************************************************************/
ThingType::ThingType(string name)
{
	// Init variables
	this->name = name;
	this->angled = true;
	this->hanging = false;
	this->shrink = false;
	this->colour = rgba_t(170, 170, 180, 255, 0);
	this->radius = 20;
	this->height = -1;
	this->scaleX = 1.0;
	this->scaleY = 1.0;
	this->fullbright = false;
	this->decoration = false;
	this->decorate = false;
	this->solid = false;
	this->zeth = -1;
	this->nexttype = 0;
	this->nextargs = 0;
	this->flags = 0;
	this->tagged = 0;
	this->arg_count = 0;

	// Init args
	args[0].name = "Arg1";
	args[1].name = "Arg2";
	args[2].name = "Arg3";
	args[3].name = "Arg4";
	args[4].name = "Arg5";
}

/* ThingType::copy
 * Copies values from [copy]
 *******************************************************************/
void ThingType::copy(ThingType* copy)
{
	// Check TT was given
	if (!copy) return;

	// Copy properties
	this->name = copy->name;
	this->group = copy->group;
	this->colour = copy->colour;
	this->radius = copy->radius;
	this->height = copy->height;
	this->scaleX = copy->scaleX;
	this->scaleY = copy->scaleY;
	this->angled = copy->angled;
	this->hanging = copy->hanging;
	this->fullbright = copy->fullbright;
	this->shrink = copy->shrink;
	this->sprite = copy->sprite;
	this->icon = copy->icon;
	this->translation = copy->translation;
	this->palette = copy->palette;
	this->decoration = copy->decoration;
	this->solid = copy->solid;
	this->zeth = copy->zeth;
	this->nexttype = copy->nexttype;
	this->nextargs = copy->nextargs;
	this->flags = copy->flags;
	this->tagged = copy->tagged;
	this->arg_count = copy->arg_count;

	// Copy args
	for (unsigned a = 0; a < 5; a++)
		this->args[a] = copy->args[a];
}

/* ThingType::getArgsString
 * Returns a string representation of the thing type's args given
 * the values in [args]
 *******************************************************************/
string ThingType::getArgsString(int args[5], string argstr[2])
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

/* ThingType::reset
 * Resets all values to defaults
 *******************************************************************/
void ThingType::reset()
{
	// Reset variables
	this->name = "Unknown";
	this->group = "";
	this->sprite = "";
	this->icon = "";
	this->translation = "";
	this->palette = "";
	this->angled = true;
	this->hanging = false;
	this->shrink = false;
	this->colour = COL_WHITE;
	this->radius = 20;
	this->height = -1;
	this->scaleX = 1.0;
	this->scaleY = 1.0;
	this->fullbright = false;
	this->decoration = false;
	this->solid = false;
	this->zeth = -1;
	this->nexttype = 0;
	this->nextargs = 0;
	this->flags = 0;
	this->tagged = 0;
	this->arg_count = 0;

	// Reset args
	for (unsigned a = 0; a < 5; a++)
	{
		args[a].name = S_FMT("Arg%d", a+1);
		args[a].type = ARGT_NUMBER;
		args[a].custom_flags.clear();
		args[a].custom_values.clear();
	}
}

/* ThingType::parse
 * Reads an thing type definition from a parsed tree [node]
 *******************************************************************/
void ThingType::parse(ParseTreeNode* node)
{
	// Go through all child nodes/values
	ParseTreeNode* child = NULL;
	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		child = (ParseTreeNode*)node->getChild(a);
		string name = child->getName();
		int arg = -1;

		// Name
		if (S_CMPNOCASE(name, "name"))
			this->name = child->getStringValue();

		// Args
		else if (S_CMPNOCASE(name, "arg1"))
			arg = 0;
		else if (S_CMPNOCASE(name, "arg2"))
			arg = 1;
		else if (S_CMPNOCASE(name, "arg3"))
			arg = 2;
		else if (S_CMPNOCASE(name, "arg4"))
			arg = 3;
		else if (S_CMPNOCASE(name, "arg5"))
			arg = 4;

		// Sprite
		else if (S_CMPNOCASE(name, "sprite"))
			this->sprite = child->getStringValue();

		// Icon
		else if (S_CMPNOCASE(name, "icon"))
			this->icon = child->getStringValue();

		// Radius
		else if (S_CMPNOCASE(name, "radius"))
			this->radius = child->getIntValue();

		// Height
		else if (S_CMPNOCASE(name, "height"))
			this->height = child->getIntValue();

		// Scale
		else if (S_CMPNOCASE(name, "scale"))
			this->scaleX = this->scaleY = child->getFloatValue();

		// ScaleX
		else if (S_CMPNOCASE(name, "scalex"))
			this->scaleX = child->getFloatValue();

		// ScaleY
		else if (S_CMPNOCASE(name, "scaley"))
			this->scaleY = child->getFloatValue();

		// Colour
		else if (S_CMPNOCASE(name, "colour"))
			this->colour.set(child->getIntValue(0), child->getIntValue(1), child->getIntValue(2));

		// Show angle
		else if (S_CMPNOCASE(name, "angle"))
			this->angled = child->getBoolValue();

		// Hanging object
		else if (S_CMPNOCASE(name, "hanging"))
			this->hanging = child->getBoolValue();

		// Shrink on zoom
		else if (S_CMPNOCASE(name, "shrink"))
			this->shrink = child->getBoolValue();

		// Fullbright
		else if (S_CMPNOCASE(name, "fullbright"))
			this->fullbright = child->getBoolValue();

		// Decoration
		else if (S_CMPNOCASE(name, "decoration"))
			this->decoration = child->getBoolValue();

		// Solid
		else if (S_CMPNOCASE(name, "solid"))
			this->solid = child->getBoolValue();

		// Translation
		else if (S_CMPNOCASE(name, "translation"))
		{
			this->translation += "\"";
			size_t v = 0;
			do
			{
				this->translation += child->getStringValue(v++);
			}
			while ((v < child->nValues()) && ((this->translation += "\", \""), true));
			this->translation += "\"";
		}

		// Palette override
		else if (S_CMPNOCASE(name, "palette"))
			this->palette = child->getStringValue();

		// Zeth icon
		else if (S_CMPNOCASE(name, "zeth"))
			this->zeth = child->getIntValue();

		// Pathed things stuff
		else if (S_CMPNOCASE(name, "nexttype"))
		{
			this->nexttype = child->getIntValue();
			this->flags |= THING_PATHED;
		}
		else if (S_CMPNOCASE(name, "nextargs"))
		{
			this->nextargs = child->getIntValue();
			this->flags |= THING_PATHED;
		}

		// Handle player starts
		else if (S_CMPNOCASE(name, "player_coop"))
			this->flags |= THING_COOPSTART;
		else if (S_CMPNOCASE(name, "player_dm"))
			this->flags |= THING_DMSTART;
		else if (S_CMPNOCASE(name, "player_team"))
			this->flags |= THING_TEAMSTART;

		// Hexen's critters are weird
		else if (S_CMPNOCASE(name, "dragon"))
			this->flags |= THING_DRAGON;
		else if (S_CMPNOCASE(name, "script"))
			this->flags |= THING_SCRIPT;

		// Some things tag other things directly
		else if (S_CMPNOCASE(name, "tagged"))
			this->tagged = GameConfiguration::parseTagged(child);

		// Parse arg definition if it was one
		if (arg >= 0)
		{
			// Update arg count
			if (arg + 1 > arg_count)
				arg_count = arg + 1;

			// Check for simple definition
			if (child->isLeaf())
			{
				// Set name
				args[arg].name = child->getStringValue();

				// Set description (if specified)
				if (child->nValues() > 1) args[arg].desc = child->getStringValue(1);
			}
			else
			{
				// Extended arg definition

				// Name
				ParseTreeNode* val = (ParseTreeNode*)child->getChild("name");
				if (val) args[arg].name = val->getStringValue();

				// Description
				val = (ParseTreeNode*)child->getChild("desc");
				if (val) args[arg].desc = val->getStringValue();

				// Type
				val = (ParseTreeNode*)child->getChild("type");
				string atype;
				if (val) atype = val->getStringValue();
				if (S_CMPNOCASE(atype, "yesno"))
					args[arg].type = ARGT_YESNO;
				else if (S_CMPNOCASE(atype, "noyes"))
					args[arg].type = ARGT_NOYES;
				else if (S_CMPNOCASE(atype, "angle"))
					args[arg].type = ARGT_ANGLE;
				else
					args[arg].type = ARGT_NUMBER;
			}
		}
	}
}

/* ThingType::stringDesc
 * Returns the thing type info as a string
 *******************************************************************/
string ThingType::stringDesc()
{
	// Init return string
	string ret = S_FMT("\"%s\" in group \"%s\", colour %d,%d,%d, radius %d", name, group, colour.r, colour.g, colour.b, radius);

	// Add any extra info
	if (!sprite.IsEmpty()) ret += S_FMT(", sprite \"%s\"", sprite);
	if (!angled) ret += ", angle hidden";
	if (hanging) ret += ", hanging";
	if (fullbright) ret += ", fullbright";
	if (decoration) ret += ", decoration";
	if (decorate) ret += ", defined in DECORATE";

	return ret;
}
