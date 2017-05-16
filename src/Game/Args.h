#pragma once

class ParseTreeNode;

namespace Game
{
	struct ArgValue
	{
		string	name;
		int		value;
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

		typedef std::map<string, Arg> SpecialMap;

		string				name;
		string				desc;
		int					type = Number;
		vector<ArgValue>	custom_values;
		vector<ArgValue>	custom_flags;

		Arg() {}
		Arg(string name) { this->name = name; }

		string	valueString(int value) const;
		string	speedLabel(int value) const;
		void	parse(ParseTreeNode* node, SpecialMap* shared_args);
	};

	struct ArgSpec
	{
		Arg  args[5];
		int  count;

		ArgSpec() : args{ { "Arg1" },{ "Arg2" },{ "Arg3" },{ "Arg4" },{ "Arg5" } }, count{ 0 } {}

		Arg&		operator[](int index) { return args[index]; }
		const Arg&	operator[](int index) const { return args[index]; }

		string	stringDesc(int values[5], string values_str[2]) const;
	};
}
