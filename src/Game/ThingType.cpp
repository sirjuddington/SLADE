
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
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
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "ThingType.h"
#include "Game/Configuration.h"
#include "Utility/Parser.h"
#include "Utility/StringUtils.h"

using namespace slade;
using namespace game;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
ThingType ThingType::unknown_;


// -----------------------------------------------------------------------------
//
// ThingType Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ThingType class constructor
// -----------------------------------------------------------------------------
ThingType::ThingType(string_view name, string_view group, string_view class_name) :
	name_{ name }, group_{ group }, tagged_{ TagType::None }, class_name_{ class_name }
{
}

// -----------------------------------------------------------------------------
// Copies all properties from [copy]
// (excludes definition variables like name, number, etc.)
// -----------------------------------------------------------------------------
void ThingType::copy(const ThingType& copy)
{
	angled_            = copy.angled_;
	hanging_           = copy.hanging_;
	shrink_            = copy.shrink_;
	colour_            = copy.colour_;
	radius_            = copy.radius_;
	height_            = copy.height_;
	scale_             = copy.scale_;
	fullbright_        = copy.fullbright_;
	decoration_        = copy.decoration_;
	decorate_          = copy.decorate_;
	solid_             = copy.solid_;
	zeth_icon_         = copy.zeth_icon_;
	next_type_         = copy.next_type_;
	next_args_         = copy.next_args_;
	flags_             = copy.flags_;
	tagged_            = copy.tagged_;
	args_              = copy.args_;
	sprite_            = copy.sprite_;
	icon_              = copy.icon_;
	translation_       = copy.translation_;
	palette_           = copy.palette_;
	z_height_absolute_ = copy.z_height_absolute_;
	point_light_       = copy.point_light_;
}

// -----------------------------------------------------------------------------
// Defines this thing type's [number], [name] and [group]
// -----------------------------------------------------------------------------
void ThingType::define(int number, string_view name, string_view group)
{
	number_ = number;
	name_   = name;
	group_  = group;
}

// -----------------------------------------------------------------------------
// Resets all values to defaults
// -----------------------------------------------------------------------------
void ThingType::reset()
{
	// Reset variables
	name_              = "Unknown";
	group_             = "";
	sprite_            = "";
	icon_              = "";
	translation_       = "";
	palette_           = "";
	angled_            = true;
	hanging_           = false;
	shrink_            = false;
	colour_            = ColRGBA::WHITE;
	radius_            = 20;
	height_            = -1;
	scale_             = { 1.0, 1.0 };
	fullbright_        = false;
	decoration_        = false;
	solid_             = false;
	zeth_icon_         = -1;
	next_type_         = 0;
	next_args_         = 0;
	flags_             = 0;
	tagged_            = TagType::None;
	z_height_absolute_ = false;
	point_light_       = "";

	// Reset args
	args_.count = 0;
	for (unsigned a = 0; a < 5; a++)
	{
		args_[a].name = fmt::format("Arg{}", a + 1);
		args_[a].type = Arg::Type::Number;
		args_[a].custom_flags.clear();
		args_[a].custom_values.clear();
	}
}

// -----------------------------------------------------------------------------
// Reads an thing type definition from a parsed tree [node]
// -----------------------------------------------------------------------------
void ThingType::parse(ParseTreeNode* node)
{
	// Go through all child nodes/values
	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		auto child = node->childPTN(a);
		auto name  = strutil::lower(child->name());
		int  arg   = -1;

		// Name
		if (name == "name")
			name_ = child->stringValue();

		// Args
		else if (name == "arg1")
			arg = 0;
		else if (name == "arg2")
			arg = 1;
		else if (name == "arg3")
			arg = 2;
		else if (name == "arg4")
			arg = 3;
		else if (name == "arg5")
			arg = 4;

		// Sprite
		else if (name == "sprite")
			sprite_ = child->stringValue();

		// Icon
		else if (name == "icon")
			icon_ = child->stringValue();

		// Radius
		else if (name == "radius")
			radius_ = child->intValue();

		// Height
		else if (name == "height")
			height_ = child->intValue();

		// Scale
		else if (name == "scale")
		{
			float s = child->floatValue();
			scale_  = { s, s };
		}

		// ScaleX
		else if (name == "scalex")
			scale_.x = child->floatValue();

		// ScaleY
		else if (name == "scaley")
			scale_.y = child->floatValue();

		// Colour
		else if (name == "colour")
			colour_.set(child->intValue(0), child->intValue(1), child->intValue(2));

		// Show angle
		else if (name == "angle")
			angled_ = child->boolValue();

		// Hanging object
		else if (name == "hanging")
			hanging_ = child->boolValue();

		// Shrink on zoom
		else if (name == "shrink")
			shrink_ = child->boolValue();

		// Fullbright
		else if (name == "fullbright")
			fullbright_ = child->boolValue();

		// Decoration
		else if (name == "decoration")
			decoration_ = child->boolValue();

		// Solid
		else if (name == "solid")
			solid_ = child->boolValue();

		// Translation
		else if (name == "translation")
		{
			translation_ += "\"";
			size_t v = 0;
			do
			{
				translation_ += child->stringValue(v++);
			} while ((v < child->nValues()) && ((translation_ += "\", \""), true));
			translation_ += "\"";
		}

		// Palette override
		else if (name == "palette")
			palette_ = child->stringValue();

		// Zeth icon
		else if (name == "zeth")
			zeth_icon_ = child->intValue();

		// Pathed things stuff
		else if (name == "nexttype")
		{
			next_type_ = child->intValue();
			flags_ |= Pathed;
		}
		else if (name == "nextargs")
		{
			next_args_ = child->intValue();
			flags_ |= Pathed;
		}

		// Handle player starts
		else if (name == "player_coop")
			flags_ |= CoOpStart;
		else if (name == "player_dm")
			flags_ |= DMStart;
		else if (name == "player_team")
			flags_ |= TeamStart;

		// Hexen's critters are weird
		else if (name == "dragon")
			flags_ |= Dragon;
		else if (name == "script")
			flags_ |= Script;

		// Some things tag other things directly
		else if (name == "tagged")
			tagged_ = parseTagged(child);

		// Z Height is absolute rather than relative to the floor/ceiling
		else if (name == "z_height_absolute")
			z_height_absolute_ = child->boolValue();

		// Thing is a point light
		else if (name == "point_light")
			point_light_ = strutil::lower(child->stringValue());

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
				if (child->nValues() > 1)
					args_[arg].desc = child->stringValue(1);
			}
			else
			{
				// Extended arg definition

				// Name
				auto val = child->childPTN("name");
				if (val)
					args_[arg].name = val->stringValue();

				// Description
				val = child->childPTN("desc");
				if (val)
					args_[arg].desc = val->stringValue();

				// Type
				val = child->childPTN("type");
				string atype;
				if (val)
					atype = val->stringValue();
				if (strutil::equalCI(atype, "yesno"))
					args_[arg].type = Arg::Type::YesNo;
				else if (strutil::equalCI(atype, "noyes"))
					args_[arg].type = Arg::Type::NoYes;
				else if (strutil::equalCI(atype, "angle"))
					args_[arg].type = Arg::Type::Angle;
				else
					args_[arg].type = Arg::Type::Number;
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Returns the thing type info as a string
// -----------------------------------------------------------------------------
string ThingType::stringDesc() const
{
	// Init return string
	auto ret = fmt::format(
		R"("{}" in group "{}", colour {},{},{}, radius {})", name_, group_, colour_.r, colour_.g, colour_.b, radius_);

	// Add any extra info
	if (!sprite_.empty())
		ret += fmt::format(", sprite \"{}\"", sprite_);
	if (!angled_)
		ret += ", angle hidden";
	if (hanging_)
		ret += ", hanging";
	if (fullbright_)
		ret += ", fullbright";
	if (decoration_)
		ret += ", decoration";
	if (decorate_)
		ret += ", defined in DECORATE";

	return ret;
}

// -----------------------------------------------------------------------------
// Reads type properties from [props] and marks as a decorate type if [decorate]
// is true.
// If [zscript] is true, support zscript-only properties
// -----------------------------------------------------------------------------
void ThingType::loadProps(PropertyList& props, bool decorate, bool zscript)
{
	// Set decorate flag
	decorate_ = decorate;

	// Sprite
	if (auto sprite = props.getIf<string>("sprite"))
	{
		if (strutil::equalCI(*sprite, "tnt1a?"))
		{
			if (!props.contains("icon"))
				icon_ = "tnt1a0";
		}
		else
			sprite_ = *sprite;
	}

	// Colour
	if (auto colour = props.getIf<string>("colour"))
	{
		// SLADE Colour
		wxColour wxc(*colour);
		if (wxc.IsOk())
			colour_.set(wxc);
	}
	else if (auto color = props.getIf<int>("color"))
	{
		// Translate DB2 color indices to RGB values
		static vector<ColRGBA> db2_colours{
			{ 0x69, 0x69, 0x69, 0xFF }, // DimGray			ARGB value of #FF696969
			{ 0x41, 0x69, 0xE1, 0xFF }, // RoyalBlue		ARGB value of #FF4169E1
			{ 0x22, 0x8B, 0x22, 0xFF }, // ForestGreen		ARGB value of #FF228B22
			{ 0x20, 0xB2, 0xAA, 0xFF }, // LightSeaGreen	ARGB value of #FF20B2AA
			{ 0xB2, 0x22, 0x22, 0xFF }, // Firebrick		ARGB value of #FFB22222
			{ 0x94, 0x00, 0xD3, 0xFF }, // DarkViolet		ARGB value of #FF9400D3
			{ 0xB8, 0x86, 0x0B, 0xFF }, // DarkGoldenrod	ARGB value of #FFB8860B
			{ 0xC0, 0xC0, 0xC0, 0xFF }, // Silver			ARGB value of #FFC0C0C0
			{ 0x80, 0x80, 0x80, 0xFF }, // Gray				ARGB value of #FF808080
			{ 0x00, 0xBF, 0xFF, 0xFF }, // DeepSkyBlue		ARGB value of #FF00BFFF
			{ 0x32, 0xCD, 0x32, 0xFF }, // LimeGreen		ARGB value of #FF32CD32
			{ 0xAF, 0xEE, 0xEE, 0xFF }, // PaleTurquoise	ARGB value of #FFAFEEEE
			{ 0xFF, 0x63, 0x47, 0xFF }, // Tomato			ARGB value of #FFFF6347
			{ 0xEE, 0x82, 0xEE, 0xFF }, // Violet			ARGB value of #FFEE82EE
			{ 0xFF, 0xFF, 0x00, 0xFF }, // Yellow			ARGB value of #FFFFFF00
			{ 0xF5, 0xF5, 0xF5, 0xFF }, // WhiteSmoke		ARGB value of #FFF5F5F5
			{ 0xFF, 0xB6, 0xC1, 0xFF }, // LightPink		ARGB value of #FFFFB6C1
			{ 0xFF, 0x8C, 0x00, 0xFF }, // DarkOrange		ARGB value of #FFFF8C00
			{ 0xBD, 0xB7, 0x6B, 0xFF }, // DarkKhaki		ARGB value of #FFBDB76B
			{ 0xDA, 0xA5, 0x20, 0xFF }, // Goldenrod		ARGB value of #FFDAA520
		};

		if (*color < (int)db2_colours.size())
			colour_ = db2_colours[*color];
	}

	// Other props
	if (auto val = props.getIf<int>("radius"))
		radius_ = *val;
	if (auto val = props.getIf<int>("height"))
		height_ = *val;
	if (auto val = props.getIf<double>("scalex"))
		scale_.x = *val;
	if (auto val = props.getIf<double>("scaley"))
		scale_.y = *val;
	if (auto val = props.getIf<bool>("hanging"))
		hanging_ = *val;
	if (auto val = props.getIf<bool>("angled"))
		angled_ = *val;
	if (auto val = props.getIf<bool>("bright"))
		fullbright_ = *val;
	if (auto val = props.getIf<bool>("decoration"))
		decoration_ = *val;
	if (auto val = props.getIf<string>("icon"))
		icon_ = *val;
	if (auto val = props.getIf<string>("translation"))
		translation_ = *val;
	if (auto val = props.getIf<bool>("solid"))
		solid_ = *val;
	if (auto val = props.getIf<bool>("obsolete"))
		flags_ |= Obsolete;

	// ZScript-only props
	if (zscript)
	{
		if (auto val = props.getIf<double>("scale"))
			scale_.x = scale_.y = *val;
		if (auto val = props.getIf<double>("scale.x"))
			scale_.x = *val;
		if (auto val = props.getIf<double>("scale.y"))
			scale_.y = *val;
		if (auto val = props.getIf<bool>("spawnceiling"))
			hanging_ = *val;
	}
}


// -----------------------------------------------------------------------------
//
// ThingType Class Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Initialises global (static) ThingType objects
// -----------------------------------------------------------------------------
void ThingType::initGlobal()
{
	unknown_.shrink_ = true;
	unknown_.icon_   = "unknown";
}
