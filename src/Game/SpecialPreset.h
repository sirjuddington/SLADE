#pragma once

class ParseTreeNode;

namespace Game
{
struct SpecialPreset
{
	wxString         name;
	wxString         group;
	int              special = -1;
	int              args[5] = { 0, 0, 0, 0, 0 };
	vector<wxString> flags;

	void           parse(ParseTreeNode* node);
	ParseTreeNode* write(ParseTreeNode* parent);
};

const vector<SpecialPreset>& customSpecialPresets();
bool                         loadCustomSpecialPresets();
bool                         saveCustomSpecialPresets();
} // namespace Game
