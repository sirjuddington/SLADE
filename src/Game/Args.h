#pragma once

class ParseTreeNode;

namespace Game
{
struct ArgValue
{
	std::string name;
	int         value;
};

struct Arg
{
	enum Type
	{
		Number = 0,
		YesNo,
		NoYes,
		Angle,
		Choice,
		Flags,
		Speed,
	};

	typedef std::map<std::string, Arg> SpecialMap;

	std::string      name;
	std::string      desc;
	int              type = Number;
	vector<ArgValue> custom_values;
	vector<ArgValue> custom_flags;

	Arg() {}
	Arg(std::string_view name) : name{ name } {}

	std::string valueString(int value) const;
	std::string speedLabel(int value) const;
	void        parse(ParseTreeNode* node, SpecialMap* shared_args);
};

struct ArgSpec
{
	Arg args[5];
	int count;

	ArgSpec() : args{ { "Arg1" }, { "Arg2" }, { "Arg3" }, { "Arg4" }, { "Arg5" } }, count{ 0 } {}

	Arg&       operator[](int index) { return args[index]; }
	const Arg& operator[](int index) const { return args[index]; }

	std::string stringDesc(const int values[5], std::string values_str[2]) const;
};
} // namespace Game
