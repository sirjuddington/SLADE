
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ThingType.cpp
// Description: ThingType class, represents a thing type
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
#include "ThingType.h"
#include "Utility/Parser.h"
#include "Game/Configuration.h"

using namespace Game;


// ----------------------------------------------------------------------------
//
// Variables
//
// ----------------------------------------------------------------------------
ThingType ThingType::unknown_;


// ----------------------------------------------------------------------------
//
// ThingType Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// ThingType::ThingType
//
// ThingType class constructor
// ----------------------------------------------------------------------------
ThingType::ThingType(const string& name, const string& group) :
	name_{ name },
	group_{ group },
	colour_{ 170, 170, 180, 255, 0 },
	radius_{ 20 },
	height_{ -1 },
	angled_{ true },
	hanging_{ false },
	shrink_{ false },
	fullbright_{ false },
	decoration_{ false },
	zeth_icon_{ -1 },
	decorate_{ false },
	solid_{ false },
	next_type_{ 0 },
	next_args_{ 0 },
	flags_{ 0 },
	tagged_{ TagType::None },
	number_{ -1 },
	scale_{ 1.0, 1.0 }
{
	// Init args
	args_.count = 0;
	args_[0].name = "Arg1";
	args_[1].name = "Arg2";
	args_[2].name = "Arg3";
	args_[3].name = "Arg4";
	args_[4].name = "Arg5";
}

// ----------------------------------------------------------------------------
// ThingType::copy
//
// Copies all properties from [copy]
// (excludes definition variables like name, number, etc.)
// ----------------------------------------------------------------------------
void ThingType::copy(const ThingType& copy)
{
	angled_ = copy.angled_;
	hanging_ = copy.hanging_;
	shrink_ = copy.shrink_;
	colour_ = copy.colour_;
	radius_ = copy.radius_;
	height_ = copy.height_;
	scale_ = copy.scale_;
	fullbright_ = copy.fullbright_;
	decoration_ = copy.decoration_;
	decorate_ = copy.decorate_;
	solid_ = copy.solid_;
	zeth_icon_ = copy.zeth_icon_;
	next_type_ = copy.next_type_;
	next_args_ = copy.next_args_;
	flags_ = copy.flags_;
	tagged_ = copy.tagged_;
	args_ = copy.args_;
}

// ----------------------------------------------------------------------------
// ThingType::define
//
// Defines this thing type's [number], [name] and [group]
// ----------------------------------------------------------------------------
void ThingType::define(int number, const string& name, const string& group)
{
	number_ = number;
	name_ = name;
	group_ = group;
}

// ----------------------------------------------------------------------------
// ThingType::reset
//
// Resets all values to defaults
// ----------------------------------------------------------------------------
void ThingType::reset()
{
	// Reset variables
	name_ = "Unknown";
	group_ = "";
	sprite_ = "";
	icon_ = "";
	translation_ = "";
	palette_ = "";
	angled_ = true;
	hanging_ = false;
	shrink_ = false;
	colour_ = COL_WHITE;
	radius_ = 20;
	height_ = -1;
	scale_ = { 1.0, 1.0 };
	fullbright_ = false;
	decoration_ = false;
	solid_ = false;
	zeth_icon_ = -1;
	next_type_ = 0;
	next_args_ = 0;
	flags_ = 0;
	tagged_ = TagType::None;

	// Reset args
	args_.count = 0;
	for (unsigned a = 0; a < 5; a++)
	{
		args_[a].name = S_FMT("Arg%d", a+1);
		args_[a].type = Arg::Type::Number;
		args_[a].custom_flags.clear();
		args_[a].custom_values.clear();
	}
}

// ----------------------------------------------------------------------------
// ThingType::parse
//
// Reads an thing type definition from a parsed tree [node]
// ----------------------------------------------------------------------------
void ThingType::parse(ParseTreeNode* node)
{
	// Go through all child nodes/values
	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		auto child = node->getChildPTN(a);
		string name = child->getName();
		int arg = -1;

		// Name
		if (S_CMPNOCASE(name, "name"))
			name_ = child->stringValue();

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
			sprite_ = child->stringValue();

		// Icon
		else if (S_CMPNOCASE(name, "icon"))
			icon_ = child->stringValue();

		// Radius
		else if (S_CMPNOCASE(name, "radius"))
			radius_ = child->intValue();

		// Height
		else if (S_CMPNOCASE(name, "height"))
			height_ = child->intValue();

		// Scale
		else if (S_CMPNOCASE(name, "scale"))
		{
			float s = child->floatValue();
			scale_ = { s, s };
		}

		// ScaleX
		else if (S_CMPNOCASE(name, "scalex"))
			scale_.x = child->floatValue();

		// ScaleY
		else if (S_CMPNOCASE(name, "scaley"))
			scale_.y = child->floatValue();

		// Colour
		else if (S_CMPNOCASE(name, "colour"))
			colour_.set(child->intValue(0), child->intValue(1), child->intValue(2));

		// Show angle
		else if (S_CMPNOCASE(name, "angle"))
			angled_ = child->boolValue();

		// Hanging object
		else if (S_CMPNOCASE(name, "hanging"))
			hanging_ = child->boolValue();

		// Shrink on zoom
		else if (S_CMPNOCASE(name, "shrink"))
			shrink_ = child->boolValue();

		// Fullbright
		else if (S_CMPNOCASE(name, "fullbright"))
			fullbright_ = child->boolValue();

		// Decoration
		else if (S_CMPNOCASE(name, "decoration"))
			decoration_ = child->boolValue();

		// Solid
		else if (S_CMPNOCASE(name, "solid"))
			solid_ = child->boolValue();

		// Translation
		else if (S_CMPNOCASE(name, "translation"))
		{
			translation_ += "\"";
			size_t v = 0;
			do
			{
				translation_ += child->stringValue(v++);
			}
			while ((v < child->nValues()) && ((translation_ += "\", \""), true));
			translation_ += "\"";
		}

		// Palette override
		else if (S_CMPNOCASE(name, "palette"))
			palette_ = child->stringValue();

		// Zeth icon
		else if (S_CMPNOCASE(name, "zeth"))
			zeth_icon_ = child->intValue();

		// Pathed things stuff
		else if (S_CMPNOCASE(name, "nexttype"))
		{
			next_type_ = child->intValue();
			flags_ |= FLAG_PATHED;
		}
		else if (S_CMPNOCASE(name, "nextargs"))
		{
			next_args_ = child->intValue();
			flags_ |= FLAG_PATHED;
		}

		// Handle player starts
		else if (S_CMPNOCASE(name, "player_coop"))
			flags_ |= FLAG_COOPSTART;
		else if (S_CMPNOCASE(name, "player_dm"))
			flags_ |= FLAG_DMSTART;
		else if (S_CMPNOCASE(name, "player_team"))
			flags_ |= FLAG_TEAMSTART;

		// Hexen's critters are weird
		else if (S_CMPNOCASE(name, "dragon"))
			flags_ |= FLAG_DRAGON;
		else if (S_CMPNOCASE(name, "script"))
			flags_ |= FLAG_SCRIPT;

		// Some things tag other things directly
		else if (S_CMPNOCASE(name, "tagged"))
			tagged_ = Game::parseTagged(child);

		// Parse arg definition if it was one
		if (arg >= 0)
		{
			// Update arg count
			if (arg + 1 > args_.count)
				args_.count = arg + 1;

			// Check for simple definition
			if (child->isLeaf())
			{
				// Set name
				args_[arg].name = child->stringValue();

				// Set description (if specified)
				if (child->nValues() > 1) args_[arg].desc = child->stringValue(1);
			}
			else
			{
				// Extended arg definition

				// Name
				auto val = child->getChildPTN("name");
				if (val) args_[arg].name = val->stringValue();

				// Description
				val = child->getChildPTN("desc");
				if (val) args_[arg].desc = val->stringValue();

				// Type
				val = child->getChildPTN("type");
				string atype;
				if (val) atype = val->stringValue();
				if (S_CMPNOCASE(atype, "yesno"))
					args_[arg].type = Arg::Type::YesNo;
				else if (S_CMPNOCASE(atype, "noyes"))
					args_[arg].type = Arg::Type::NoYes;
				else if (S_CMPNOCASE(atype, "angle"))
					args_[arg].type = Arg::Type::Angle;
				else
					args_[arg].type = Arg::Type::Number;
			}
		}
	}
}

// ----------------------------------------------------------------------------
// ThingType::stringDesc
//
// Returns the thing type info as a string
// ----------------------------------------------------------------------------
string ThingType::stringDesc() const
{
	// Init return string
	string ret = S_FMT("\"%s\" in group \"%s\", colour %d,%d,%d, radius %d", name_, group_, colour_.r, colour_.g, colour_.b, radius_);

	// Add any extra info
	if (!sprite_.IsEmpty()) ret += S_FMT(", sprite \"%s\"", sprite_);
	if (!angled_) ret += ", angle hidden";
	if (hanging_) ret += ", hanging";
	if (fullbright_) ret += ", fullbright";
	if (decoration_) ret += ", decoration";
	if (decorate_) ret += ", defined in DECORATE";

	return ret;
}

// ----------------------------------------------------------------------------
// ThingType::loadProps
//
// Reads type properties from [props] and marks as a decorate type if
// [decorate] is true
// ----------------------------------------------------------------------------
void ThingType::loadProps(PropertyList& props, bool decorate)
{
	// Set decorate flag
	decorate_ = decorate;

	// Sprite
	if (props["sprite"].hasValue())
	{
		if (S_CMPNOCASE(props["sprite"].getStringValue(), "tnt1a?"))
		{
			if ((!(props["icon"].hasValue())) && icon_.IsEmpty())
				icon_ = "tnt1a0";
		}
		else
			sprite_ = props["sprite"].getStringValue();
	}

	// Colour
	if (props["colour"].hasValue())
	{
		// SLADE Colour
		wxColour wxc(props["colour"].getStringValue());
		if (wxc.IsOk())
			colour_.set(COLWX(wxc));
	}
	else if (props["color"].hasValue())
	{
		// Translate DB2 color indices to RGB values
		static vector<rgba_t> db2_colours
		{
			{ 0x69, 0x69, 0x69, 0xFF },   // DimGray		ARGB value of #FF696969
			{ 0x41, 0x69, 0xE1, 0xFF },   // RoyalBlue		ARGB value of #FF4169E1
			{ 0x22, 0x8B, 0x22, 0xFF },   // ForestGreen	ARGB value of #FF228B22
			{ 0x20, 0xB2, 0xAA, 0xFF },   // LightSeaGreen	ARGB value of #FF20B2AA
			{ 0xB2, 0x22, 0x22, 0xFF },   // Firebrick		ARGB value of #FFB22222
			{ 0x94, 0x00, 0xD3, 0xFF },   // DarkViolet		ARGB value of #FF9400D3
			{ 0xB8, 0x86, 0x0B, 0xFF },   // DarkGoldenrod	ARGB value of #FFB8860B
			{ 0xC0, 0xC0, 0xC0, 0xFF },   // Silver			ARGB value of #FFC0C0C0
			{ 0x80, 0x80, 0x80, 0xFF },   // Gray			ARGB value of #FF808080
			{ 0x00, 0xBF, 0xFF, 0xFF },   // DeepSkyBlue	ARGB value of #FF00BFFF
			{ 0x32, 0xCD, 0x32, 0xFF },   // LimeGreen		ARGB value of #FF32CD32
			{ 0xAF, 0xEE, 0xEE, 0xFF },   // PaleTurquoise	ARGB value of #FFAFEEEE
			{ 0xFF, 0x63, 0x47, 0xFF },   // Tomato			ARGB value of #FFFF6347
			{ 0xEE, 0x82, 0xEE, 0xFF },   // Violet			ARGB value of #FFEE82EE
			{ 0xFF, 0xFF, 0x00, 0xFF },   // Yellow			ARGB value of #FFFFFF00
			{ 0xF5, 0xF5, 0xF5, 0xFF },   // WhiteSmoke		ARGB value of #FFF5F5F5
			{ 0xFF, 0xB6, 0xC1, 0xFF },   // LightPink		ARGB value of #FFFFB6C1
			{ 0xFF, 0x8C, 0x00, 0xFF },   // DarkOrange		ARGB value of #FFFF8C00
			{ 0xBD, 0xB7, 0x6B, 0xFF },   // DarkKhaki		ARGB value of #FFBDB76B
			{ 0xDA, 0xA5, 0x20, 0xFF },   // Goldenrod		ARGB value of #FFDAA520
		};

		int color = props["color"].getIntValue();
		if (color < db2_colours.size())
			colour_ = db2_colours[color];
	}

	// Other props
	if (props["radius"].hasValue()) radius_ = props["radius"].getIntValue();
	if (props["height"].hasValue()) height_ = props["height"].getIntValue();
	if (props["scalex"].hasValue()) scale_.x = props["scalex"].getFloatValue();
	if (props["scaley"].hasValue()) scale_.y = props["scaley"].getFloatValue();
	if (props["hanging"].hasValue()) hanging_ = props["hanging"].getBoolValue();
	if (props["angled"].hasValue()) angled_ = props["angled"].getBoolValue();
	if (props["bright"].hasValue()) fullbright_ = props["bright"].getBoolValue();
	if (props["decoration"].hasValue()) decoration_ = props["decoration"].getBoolValue();
	if (props["icon"].hasValue()) icon_ = props["icon"].getStringValue();
	if (props["translation"].hasValue()) translation_ = props["translation"].getStringValue();
	if (props["solid"].hasValue()) solid_ = props["solid"].getBoolValue();
	if (props["obsolete"].hasValue()) flags_ |= FLAG_OBSOLETE;
}


// ----------------------------------------------------------------------------
//
// ThingType Class Static Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// ThingType::initGlobal
//
// Initialises global (static) ThingType objects
// ----------------------------------------------------------------------------
void ThingType::initGlobal()
{
	unknown_.shrink_ = true;
	unknown_.icon_ = "unknown";
}
