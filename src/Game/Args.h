#pragma once

class ParseTreeNode;

namespace Game
{
struct ArgValue
{
	wxString name;
	int      value;
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

	typedef std::map<wxString, Arg> SpecialMap;

	wxString         name;
	wxString         desc;
	int              type = Number;
	vector<ArgValue> custom_values;
	vector<ArgValue> custom_flags;

	Arg() {}
	Arg(const wxString& name) : name{ name } {}

	wxString valueString(int value) const;
	wxString speedLabel(int value) const;
	void     parse(ParseTreeNode* node, SpecialMap* shared_args);
};

struct ArgSpec
{
	Arg args[5];
	int count;

	ArgSpec() : args{ { "Arg1" }, { "Arg2" }, { "Arg3" }, { "Arg4" }, { "Arg5" } }, count{ 0 } {}

	Arg&       operator[](int index) { return args[index]; }
	const Arg& operator[](int index) const { return args[index]; }

	wxString stringDesc(const int values[5], wxString values_str[2]) const;
};
} // namespace Game
