
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         https://slade.mancubus.net
// Filename:    Args.cpp
// Description: Arg and related structs for ActionSpecial and ThingType arg
//              definitions
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
#include "Args.h"
#include "Utility/Parser.h"
#include "Utility/StringUtils.h"

using namespace slade;
using namespace game;


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Returns a string representation of [value] for a 'custom flags' type
// arg, given [custom_flags]
// -----------------------------------------------------------------------------
string customFlags(int value, const vector<ArgValue>& custom_flags)
{
	// This has to go in REVERSE order to correctly handle multi-bit
	// enums (so we see 3 before 1 and 2)
	vector<string> flags;
	size_t         final_length   = 0;
	int            last_group     = 0;
	int            original_value = value;
	bool           has_flag;
	for (auto it = custom_flags.rbegin(); it != custom_flags.rend(); ++it)
	{
		if ((it->value & (it->value - 1)) != 0)
			// Not a power of two, so must be a group
			last_group = value;

		if (value == 0)
			// Zero is special: it only counts as a flag value if the
			// most recent "group" is empty
			has_flag = (last_group && (original_value & last_group) == 0);
		else
			has_flag = ((value & it->value) == it->value);

		if (has_flag)
		{
			value &= ~it->value;
			flags.push_back(it->name);
			final_length += it->name.size() + 3;
		}
	}

	if (value || flags.empty())
		flags.push_back(fmt::format("{}", value));

	// Join 'em, in reverse again, to restore the original order
	string out;
	out.reserve(final_length);
	auto it = flags.rbegin();
	while (true)
	{
		out += *it;
		++it;
		if (it == flags.rend())
			break;
		out += " + ";
	}
	return out;
}
} // namespace


// -----------------------------------------------------------------------------
//
// Arg Struct Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns a string representation of [value] depending on the arg's type
// -----------------------------------------------------------------------------
string Arg::valueString(int value) const
{
	switch (type)
	{
	case YesNo: return (value > 0) ? "Yes" : "No";
	case NoYes: return (value > 0) ? "No" : "Yes";

	// Custom list of choices
	case Choice:
	{
		for (auto& cv : custom_values)
			if (cv.value == value)
				return cv.name;
		break;
	}

	// Custom list of flags
	case Flags: return customFlags(value, custom_flags);

	// Angle
	case Angle:
		return fmt::format("{} Degrees", value); // TODO: E/S/W/N/etc

	// Speed
	case Speed:
	{
		auto speed_label = speedLabel(value);
		if (speed_label.empty())
			return fmt::format("{}", value);
		else
			return fmt::format("{} ({})", value, speed_label);
	}

	case Tics: return fmt::format("{:.2f} seconds", value / 35.0);

	case Octics: return fmt::format("{:.2f} seconds", value / 8.0);

	default: break;
	}

	// Any other type
	return fmt::format("{}", value);
}

// -----------------------------------------------------------------------------
// Returns a string representation of speed [value]
// -----------------------------------------------------------------------------
string Arg::speedLabel(int value) const
{
	// Speed can optionally have a set of predefined values, most taken
	// from the Boom generalized values
	if (custom_values.empty())
		return "";
	if (value == 0)
		return "broken";
	if (value < custom_values.front().value)
		return fmt::format("< {}", custom_values.front().name);
	if (value > custom_values.back().value)
		return fmt::format("> {}", custom_values.back().name);
	for (unsigned a = 0; a < custom_values.size(); a++)
	{
		if (value == custom_values[a].value)
			return custom_values[a].name;
		if (a > 0 && value < custom_values[a].value)
			return fmt::format("{} ~ {}", custom_values[a - 1].name, custom_values[a].name);
	}
	return "";
}

// -----------------------------------------------------------------------------
// Parses an arg definition from [node], using [shared_args] for predeclared
// args if it is given
// -----------------------------------------------------------------------------
void Arg::parse(ParseTreeNode* node, SpecialMap* shared_args)
{
	// Check for simple definition
	if (node->isLeaf())
	{
		auto name = node->stringValue();

		// Names beginning with a dollar sign are references to predeclared args
		if (shared_args && strutil::startsWith(name, '$'))
		{
			auto it = shared_args->find(name.substr(1));
			if (it == shared_args->end())
				// Totally bogus reference; silently ignore this arg
				return;

			*this = it->second;
		}
		else
		{
			// Set name
			this->name = node->stringValue();

			// Set description (if specified)
			if (node->nValues() > 1)
				desc = node->stringValue(1);
		}
	}
	else
	{
		// Extended arg definition

		// Name
		auto val = node->childPTN("name");
		if (val)
			name = val->stringValue();

		// Description
		val = node->childPTN("desc");
		if (val)
			desc = val->stringValue();

		// Type
		val = node->childPTN("type");
		string atype;
		if (val)
			atype = val->stringValue();
		if (strutil::equalCI(atype, "yesno"))
			type = YesNo;
		else if (strutil::equalCI(atype, "noyes"))
			type = NoYes;
		else if (strutil::equalCI(atype, "angle"))
			type = Angle;
		else if (strutil::equalCI(atype, "choice"))
			type = Choice;
		else if (strutil::equalCI(atype, "flags"))
			type = Flags;
		else if (strutil::equalCI(atype, "speed"))
			type = Speed;
		else if (strutil::equalCI(atype, "tics"))
			type = Tics;
		else if (strutil::equalCI(atype, "octics"))
			type = Octics;
		else
			type = Number;

		// Customs
		val = node->childPTN("custom_values");
		if (val)
		{
			for (auto cv : val->allChildren())
				custom_values.push_back({ Parser::node(cv)->stringValue(), strutil::asInt(cv->name()) });
		}

		val = node->childPTN("custom_flags");
		if (val)
		{
			for (auto cf : val->allChildren())
				custom_flags.push_back({ Parser::node(cf)->stringValue(), strutil::asInt(cf->name()) });
		}
	}
}


// -----------------------------------------------------------------------------
//
// ArgSpec Struct Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns a string representation of [values] depending on the spec arg types
// -----------------------------------------------------------------------------
string ArgSpec::stringDesc(const int values[5], string values_str[2]) const
{
	string ret;

	// Add each arg to the string
	for (unsigned a = 0; a < 5; a++)
	{
		// Skip if the arg name is undefined and the arg value is 0
		if (values[a] == 0 && strutil::startsWith(args[a].name, "Arg"))
			continue;

		ret += args[a].name;
		ret += ": ";
		if (a < 2 && values[a] == 0 && !values_str[a].empty())
			ret += values_str[a];
		else
			ret += args[a].valueString(values[a]);
		ret += ", ";
	}

	// Cut ending ", "
	if (!ret.empty())
		strutil::removeLastIP(ret, 2);

	return ret;
}
