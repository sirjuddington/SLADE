#pragma once

#include "General/JsonFwd.h"

namespace slade
{
namespace game
{
	struct SpecialPreset
	{
		string         name;
		string         group;
		int            special = -1;
		int            args[5] = { 0, 0, 0, 0, 0 };
		vector<string> flags;

		void           parse(const ParseTreeNode* node);
		ParseTreeNode* write(ParseTreeNode* parent) const;

		void fromJson(const Json& j);
		Json toJson() const;
	};

	const vector<SpecialPreset>& customSpecialPresets();
	bool                         loadCustomSpecialPresets();
	bool                         saveCustomSpecialPresets();
} // namespace game
} // namespace slade
