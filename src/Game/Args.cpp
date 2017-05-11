#include "Main.h"
#include "Args.h"
#include "Utility/Parser.h"

using namespace Game;

namespace
{
	string customFlags(int value, const vector<ArgValue>& custom_flags)
	{
		// This has to go in REVERSE order to correctly handle multi-bit
		// enums (so we see 3 before 1 and 2)
		vector<string> flags;
		size_t final_length = 0;
		int last_group = 0;
		int original_value = value;
		bool has_flag;
		for (auto it = custom_flags.rbegin();
			it != custom_flags.rend();
			it++)
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

		if (value || !flags.size())
			flags.push_back(S_FMT("%d", value));

		// Join 'em, in reverse again, to restore the original order
		string out;
		out.Alloc(final_length);
		auto it = flags.rbegin();
		while (true)
		{
			out += *it;
			it++;
			if (it == flags.rend())
				break;
			out += " + ";
		}
		return out;
	}
}

string Arg::valueString(int value)
{
	switch (type)
	{
	case YesNo:
		return (value > 0) ? "Yes" : "No";
	case NoYes:
		return (value > 0) ? "No" : "Yes";

	// Custom list of choices
	case Choice:
	{
		for (auto& cv : custom_values)
			if (cv.value == value)
				return cv.name;
		break;
	}

	// Custom list of flags
	case Flags:
		return customFlags(value, custom_flags);

	// Angle
	case Angle:
		return S_FMT("%d Degrees", value);  // TODO: E/S/W/N/etc

	// Speed
	case Speed:
	{
		string speed_label = speedLabel(value);
		if (speed_label.empty())
			return S_FMT("%d", value);
		else
			return S_FMT("%d (%s)", value, speed_label);
	}

	default:
		break;
	}

	// Any other type
	return S_FMT("%d", value);
}

string Arg::speedLabel(int value)
{
	// Speed can optionally have a set of predefined values, most taken
	// from the Boom generalized values
	if (!custom_values.size())
		return "";
	if (value == 0)
		return "broken";
	if (value < custom_values.front().value)
		return S_FMT("< %s", custom_values.front().name);
	if (value > custom_values.back().value)
		return S_FMT("> %s", custom_values.back().name);
	for (unsigned a = 0; a < custom_values.size(); a++)
	{
		if (value == custom_values[a].value)
			return custom_values[a].name;
		if (a > 0 && value < custom_values[a].value)
			return S_FMT("%s ~ %s", custom_values[a - 1].name, custom_values[a].name);
	}
	return "";
}

void Arg::parse(ParseTreeNode* node, SpecialMap* shared_args)
{
	ParseTreeNode* custom = nullptr;

	// Check for simple definition
	if (node->isLeaf())
	{
		string name = node->getStringValue();
		string shared_arg_name;

		// Names beginning with a dollar sign are references to predeclared args
		if (shared_args && name.StartsWith("$", &shared_arg_name))
		{
			auto it = shared_args->find(shared_arg_name);
			if (it == shared_args->end())
				// Totally bogus reference; silently ignore this arg
				return;

			*this = it->second;
		}
		else
		{
			// Set name
			name = node->getStringValue();

			// Set description (if specified)
			if (node->nValues() > 1) desc = node->getStringValue(1);
		}
	}
	else
	{
		// Extended arg definition

		// Name
		auto val = (ParseTreeNode*)node->getChild("name");
		if (val) name = val->getStringValue();

		// Description
		val = (ParseTreeNode*)node->getChild("desc");
		if (val) desc = val->getStringValue();

		// Type
		val = (ParseTreeNode*)node->getChild("type");
		string atype;
		if (val) atype = val->getStringValue();
		if (S_CMPNOCASE(atype, "yesno"))
			type = YesNo;
		else if (S_CMPNOCASE(atype, "noyes"))
			type = NoYes;
		else if (S_CMPNOCASE(atype, "angle"))
			type = Angle;
		else if (S_CMPNOCASE(atype, "choice"))
			type = Choice;
		else if (S_CMPNOCASE(atype, "flags"))
			type = Flags;
		else if (S_CMPNOCASE(atype, "speed"))
			type = Speed;
		else
			type = Number;

		// Customs
		val = (ParseTreeNode*)node->getChild("custom_values");
		if (val)
		{
			for (unsigned b = 0; b < val->nChildren(); b++)
			{
				custom = (ParseTreeNode*)val->getChild(b);
				custom_values.push_back({
					custom->getStringValue(),
					atoi(CHR(custom->getName()))
				});
			}
		}

		val = (ParseTreeNode*)node->getChild("custom_flags");
		if (val)
		{
			for (unsigned b = 0; b < val->nChildren(); b++)
			{
				custom = (ParseTreeNode*)val->getChild(b);
				custom_flags.push_back({
					custom->getStringValue(),
					atoi(CHR(custom->getName()))
				});
			}
		}
	}
}

string ArgSpec::stringDesc(int values[5], string values_str[2])
{
	string ret;

	// Add each arg to the string
	for (unsigned a = 0; a < 5; a++)
	{
		// Skip if the arg name is undefined and the arg value is 0
		if (values[a] == 0 && args[a].name.StartsWith("Arg"))
			continue;

		ret += args[a].name;
		ret += ": ";
		if (a < 2 && values[a] == 0 && !values_str[a].IsEmpty())
			ret += values_str[a];
		else
			ret += args[a].valueString(values[a]);
		ret += ", ";
	}

	// Cut ending ", "
	if (!ret.IsEmpty())
		ret.RemoveLast(2);

	return ret;
}
