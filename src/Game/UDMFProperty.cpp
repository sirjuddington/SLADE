
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    UDMFProperty.cpp
// Description: UDMFProperty class, contains info about a UDMF property
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
#include "UDMFProperty.h"
#include "Utility/Parser.h"
#include "Utility/StringUtils.h"

using namespace slade;
using namespace game;


// -----------------------------------------------------------------------------
//
// UDMFProperty Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Reads a UDMF property definition from a parsed tree [node]
// -----------------------------------------------------------------------------
void UDMFProperty::parse(ParseTreeNode* node, string_view group)
{
	// Set group and property name
	group_    = group;
	property_ = node->name();

	// Check for basic definition
	if (node->nChildren() == 0)
	{
		name_ = node->stringValue();
		return;
	}

	// Otherwise, read node data
	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		auto prop     = node->childPTN(a);
		auto pn_lower = strutil::lower(prop->name());

		// Property type
		if (pn_lower == "type")
		{
			auto val_lower = strutil::lower(prop->stringValue());

			if (val_lower == "bool")
				type_ = Type::Boolean;
			else if (val_lower == "int")
				type_ = Type::Int;
			else if (val_lower == "float")
				type_ = Type::Float;
			else if (val_lower == "string")
				type_ = Type::String;
			else if (val_lower == "colour")
				type_ = Type::Colour;
			else if (val_lower == "actionspecial")
				type_ = Type::ActionSpecial;
			else if (val_lower == "sectorspecial")
				type_ = Type::SectorSpecial;
			else if (val_lower == "thingtype")
				type_ = Type::ThingType;
			else if (val_lower == "angle")
				type_ = Type::Angle;
			else if (val_lower == "texture_wall")
				type_ = Type::TextureWall;
			else if (val_lower == "texture_flat")
				type_ = Type::TextureFlat;
			else if (val_lower == "id")
				type_ = Type::ID;
		}

		// Property name
		else if (pn_lower == "name")
			name_ = prop->stringValue();

		// Default value
		else if (pn_lower == "default")
		{
			switch (type_)
			{
			case Type::Boolean: default_value_ = prop->boolValue(); break;
			case Type::Int: default_value_ = prop->intValue(); break;
			case Type::Float: default_value_ = prop->floatValue(); break;
			case Type::String: default_value_ = prop->stringValue(); break;
			case Type::ActionSpecial: default_value_ = prop->intValue(); break;
			case Type::SectorSpecial: default_value_ = prop->intValue(); break;
			case Type::ThingType: default_value_ = prop->intValue(); break;
			case Type::Angle: default_value_ = prop->intValue(); break;
			case Type::TextureWall: default_value_ = prop->stringValue(); break;
			case Type::TextureFlat: default_value_ = prop->stringValue(); break;
			case Type::ID: default_value_ = prop->intValue(); break;
			default: default_value_ = prop->stringValue(); break;
			}

			// Not sure why I have to do this here, but for whatever reason prop->getIntValue() doesn't work
			// if the value parsed was hex (or it could be to do with the colour type? who knows)
			if (type_ == Type::Colour)
				default_value_ = strutil::asInt(prop->stringValue());

			has_default_ = true;
		}

		// Property is a flag
		else if (pn_lower == "flag")
			flag_ = true;

		// Property is a SPAC trigger
		else if (pn_lower == "trigger")
			trigger_ = true;

		// Possible values
		else if (pn_lower == "values")
		{
			switch (type_)
			{
			case Type::Boolean:
				for (unsigned b = 0; b < prop->nValues(); b++)
					values_.emplace_back(prop->boolValue(b));
				break;
			case Type::Int:
			case Type::ActionSpecial:
			case Type::SectorSpecial:
			case Type::ThingType:
				for (unsigned b = 0; b < prop->nValues(); b++)
					values_.emplace_back(prop->intValue(b));
				break;
			case Type::Float:
				for (unsigned b = 0; b < prop->nValues(); b++)
					values_.emplace_back(prop->floatValue(b));
				break;
			default:
				for (unsigned b = 0; b < prop->nValues(); b++)
					values_.emplace_back(prop->stringValue(b));
				break;
			}
		}

		// Show always
		else if (pn_lower == "show_always")
			show_always_ = prop->boolValue();
	}
}

// -----------------------------------------------------------------------------
// Returns a string representation of the UDMF property definition
// -----------------------------------------------------------------------------
string UDMFProperty::getStringRep()
{
	auto ret = fmt::format(R"(Property "{}": name = "{}", group = "{}")", property_, name_, group_);

	switch (type_)
	{
	case Type::Boolean: ret += ", type = bool"; break;
	case Type::Int: ret += ", type = int"; break;
	case Type::Float: ret += ", type = float"; break;
	case Type::String: ret += ", type = string"; break;
	case Type::Colour: ret += ", type = colour"; break;
	case Type::ActionSpecial: ret += ", type = actionspecial"; break;
	case Type::SectorSpecial: ret += ", type = sectorspecial"; break;
	case Type::ThingType: ret += ", type = thingtype"; break;
	case Type::Angle: ret += ", type = angle"; break;
	case Type::TextureWall: ret += ", type = wall texture"; break;
	case Type::TextureFlat: ret += ", type = flat texture"; break;
	case Type::ID: ret += ", type = id"; break;
	default: ret += ", ******unknown type********"; break;
	};

	if (has_default_)
	{
		if (type_ == Type::Boolean)
		{
			if ((bool)default_value_)
				ret += ", default = true";
			else
				ret += ", default = false";
		}
		else if (
			type_ == Type::Int || type_ == Type::ActionSpecial || type_ == Type::SectorSpecial
			|| type_ == Type::ThingType || type_ == Type::Colour)
			ret += fmt::format(", default = {}", default_value_.intValue());
		else if (type_ == Type::Float)
			ret += fmt::format(", default = {:1.2f}", (double)default_value_);
		else
			ret += fmt::format(", default = \"{}\"", default_value_.stringValue());
	}
	else
		ret += ", no valid default";

	if (flag_)
		ret += ", is flag";
	if (trigger_)
		ret += ", is trigger";

	if (!values_.empty())
	{
		ret += "\nPossible values: ";
		for (unsigned a = 0; a < values_.size(); a++)
		{
			ret += values_[a].stringValue();
			if (a < values_.size() - 1)
				ret += ", ";
		}
	}

	return ret;
}
