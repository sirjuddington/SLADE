#pragma once

class ParseTreeNode;

namespace Game
{
struct SpecialPreset
{
	std::string         name;
	std::string         group;
	int                 special = -1;
	int                 args[5] = { 0, 0, 0, 0, 0 };
	vector<std::string> flags;

	void           parse(ParseTreeNode* node);
	ParseTreeNode* write(ParseTreeNode* parent);
};

const vector<SpecialPreset>& customSpecialPresets();
bool                         loadCustomSpecialPresets();
bool                         saveCustomSpecialPresets();
} // namespace Game
