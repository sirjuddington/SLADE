#pragma once

class ParseTreeNode;

namespace Game
{
	struct SpecialPreset
	{
		string			name;
		string			group;
		int				special = -1;
		int				args[5] = { 0, 0, 0, 0, 0 };
		vector<string>	flags;

		void			parse(ParseTreeNode* node);
		ParseTreeNode*	write(ParseTreeNode* parent);
	};

	const vector<SpecialPreset>&	customSpecialPresets();
	bool							loadCustomSpecialPresets();
	bool							saveCustomSpecialPresets();
}
