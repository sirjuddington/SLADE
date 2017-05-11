
#ifndef __ACTION_SPECIAL_H__
#define __ACTION_SPECIAL_H__

#include "Args.h"
using namespace Game; // TODO: Move this into Game namespace

class ParseTreeNode;
class ActionSpecial
{
	friend class GameConfiguration;
public:
	ActionSpecial(string name = "Unknown", string group = "");
	~ActionSpecial() {}

	void	copy(ActionSpecial* copy);

	string			getName() { return name; }
	string			getGroup() { return group; }
	int				needsTag() { return tagged; }
	const ArgSpec&	getArgspec() { return args; }

	void	setName(string name) { this->name = name; }
	void	setGroup(string group) { this->group = group; }
	void	setTagged(int tagged) { this->tagged = tagged; }

	string	getArgsString(int args[5], string argstr[2]);

	void	reset();
	void	parse(ParseTreeNode* node, Arg::SpecialMap* shared_args);
	string	stringDesc();

private:
	string	name;
	string	group;
	int		tagged;
	ArgSpec	args;
};

#endif//__ACTION_SPECIAL_H__
