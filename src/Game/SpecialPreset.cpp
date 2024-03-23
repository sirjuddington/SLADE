
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SpecialPreset.cpp
// Description: SpecialPreset struct and related functions
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "SpecialPreset.h"
#include "App.h"
#include "Utility/Parser.h"
#include "Utility/Property.h"
#include "Utility/StringUtils.h"

using namespace slade;
using namespace game;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace slade::game
{
vector<SpecialPreset> custom_presets;
}

// -----------------------------------------------------------------------------
//
// SpecialPreset Struct Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Reads a special preset definition from a parsed tree [node]
// -----------------------------------------------------------------------------
void SpecialPreset::parse(const ParseTreeNode* node)
{
	name = node->name();

	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		auto child = node->childPTN(a);

		// Group
		if (strutil::equalCI(child->name(), "group"))
			group = child->stringValue();

		// Special
		else if (strutil::equalCI(child->name(), "special"))
			special = child->intValue();

		// Flags
		else if (strutil::equalCI(child->name(), "flags"))
			for (auto& flag : child->values())
				flags.push_back(property::asString(flag));

		// Args
		else if (strutil::equalCI(child->name(), "arg1"))
			args[0] = child->intValue();
		else if (strutil::equalCI(child->name(), "arg2"))
			args[1] = child->intValue();
		else if (strutil::equalCI(child->name(), "arg3"))
			args[2] = child->intValue();
		else if (strutil::equalCI(child->name(), "arg4"))
			args[3] = child->intValue();
		else if (strutil::equalCI(child->name(), "arg5"))
			args[4] = child->intValue();
	}
}

// -----------------------------------------------------------------------------
// Writes the special preset to a new 'preset' ParseTreeNode under [parent] and
// returns it
// -----------------------------------------------------------------------------
ParseTreeNode* SpecialPreset::write(ParseTreeNode* parent) const
{
	auto node = new ParseTreeNode(parent, nullptr, nullptr, "preset");
	node->setName(name);

	// Group
	string_view ex_group = group;
	if (strutil::startsWith(ex_group, "Custom/"))
		ex_group.remove_prefix(7);
	if (ex_group != "Custom")
		node->addChildPTN("group")->addStringValue(ex_group);

	// Special
	node->addChildPTN("special")->addIntValue(special);

	// Args
	for (unsigned a = 0; a < 5; a++)
		if (args[a] != 0)
			node->addChildPTN(fmt::format("arg{}", a + 1))->addIntValue(args[a]);

	// Flags
	auto node_flags = node->addChildPTN("flags");
	for (auto& flag : flags)
		node_flags->addStringValue(flag);

	return node;
}


// -----------------------------------------------------------------------------
//
// Game Namespace Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns all loaded custom special presets
// -----------------------------------------------------------------------------
const vector<SpecialPreset>& game::customSpecialPresets()
{
	return custom_presets;
}

// -----------------------------------------------------------------------------
// Loads user defined (custom) special presets from special_presets.cfg in the
// user data directory.
// Returns true on success or false if loading failed
// -----------------------------------------------------------------------------
bool game::loadCustomSpecialPresets()
{
	// Check file exists
	auto file = app::path("special_presets.cfg", app::Dir::User);
	if (!wxFileExists(file))
		return true;

	// Load special presets file to memory
	MemChunk mc;
	if (!mc.importFile(file))
		return false;

	// Parse the file
	Parser parser;
	if (!parser.parseText(mc, "special_presets.cfg"))
		return false;

	auto node = parser.parseTreeRoot()->childPTN("special_presets");
	if (node)
	{
		for (unsigned a = 0; a < node->nChildren(); a++)
		{
			auto child = node->childPTN(a);
			if (strutil::equalCI(child->type(), "preset"))
			{
				custom_presets.emplace_back();
				custom_presets.back().parse(child);

				// Add 'Custom' to preset group
				if (!custom_presets.back().group.empty())
					custom_presets.back().group = fmt::format("Custom/{}", custom_presets.back().group);
				else
					custom_presets.back().group = "Custom";
			}
		}
	}

	return true;
}

// -----------------------------------------------------------------------------
// Saves all user defined (custom) special presets to special_presets.cfg in
// the user data directory.
// Returns true on success or false if writing failed
// -----------------------------------------------------------------------------
bool game::saveCustomSpecialPresets()
{
	if (custom_presets.empty())
		return true;

	// Open file
	wxFile file;
	if (!file.Open(app::path("special_presets.cfg", app::Dir::User), wxFile::write))
	{
		log::error("Unable to open special_presets.cfg file for writing");
		return false;
	}

	// Build presets tree
	ParseTreeNode root;
	root.setName("special_presets");
	for (auto& preset : custom_presets)
		preset.write(&root);

	// Write to file
	string presets;
	root.write(presets);
	if (!file.Write(presets))
	{
		log::error("Writing to special_presets.cfg failed");
		return false;
	}

	return true;
}


// Testing
#include "General/Console.h"

CONSOLE_COMMAND(test_preset_export, 0, false)
{
	ParseTreeNode root;
	root.setName("special_presets");
	for (auto& preset : custom_presets)
		preset.write(&root);

	string out;
	root.write(out);
	log::console(out);
}
