
#ifndef __ACTION_SPECIAL_H__
#define __ACTION_SPECIAL_H__

#include "Args.h"

WX_DECLARE_STRING_HASH_MAP(arg_t, SpecialArgMap);

class ParseTreeNode;
class ActionSpecial
{
	friend class GameConfiguration;
private:
	string	name;
	string	group;
	int		tagged;
	arg_t	args[5];
	int		arg_count;

public:
	ActionSpecial(string name = "Unknown", string group = "");
	~ActionSpecial() {}

	void	copy(ActionSpecial* copy);

	string	getName() { return name; }
	string	getGroup() { return group; }
	int		needsTag() { return tagged; }
	const argspec_t getArgspec() { return argspec_t(args, arg_count); }

	void	setName(string name) { this->name = name; }
	void	setGroup(string group) { this->group = group; }
	void	setTagged(int tagged) { this->tagged = tagged; }

	string	getArgsString(int args[5], string argstr[2]);

	void	reset();
	void	parse(ParseTreeNode* node, SpecialArgMap* shared_args);
	static void	parseArg(ParseTreeNode* node, SpecialArgMap* shared_args, arg_t& arg);
	string	stringDesc();
};

#endif//__ACTION_SPECIAL_H__
