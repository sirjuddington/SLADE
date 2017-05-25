
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
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
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#include "App.h"
#include "SpecialPreset.h"
#include "Utility/Parser.h"

using namespace Game;


// ----------------------------------------------------------------------------
//
// Variables
//
// ----------------------------------------------------------------------------
namespace Game
{
	vector<SpecialPreset>	custom_presets;
}

// ----------------------------------------------------------------------------
//
// SpecialPreset Struct Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// SpecialPreset::parse
//
// Reads a special preset definition from a parsed tree [node]
// ----------------------------------------------------------------------------
void SpecialPreset::parse(ParseTreeNode* node)
{
	name = node->getName();

	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		auto child = node->getChildPTN(a);
		string name = child->getName();

		// Group
		if (S_CMPNOCASE(child->getName(), "group"))
			group = child->stringValue();

		// Special
		else if (S_CMPNOCASE(child->getName(), "special"))
			special = child->intValue();

		// Flags
		else if (S_CMPNOCASE(child->getName(), "flags"))
			for (auto& flag : child->values())
				flags.push_back(flag.getStringValue());

		// Args
		else if (S_CMPNOCASE(child->getName(), "arg1"))
			args[0] = child->intValue();
		else if (S_CMPNOCASE(child->getName(), "arg2"))
			args[1] = child->intValue();
		else if (S_CMPNOCASE(child->getName(), "arg3"))
			args[2] = child->intValue();
		else if (S_CMPNOCASE(child->getName(), "arg4"))
			args[3] = child->intValue();
		else if (S_CMPNOCASE(child->getName(), "arg5"))
			args[4] = child->intValue();
	}
}

// ----------------------------------------------------------------------------
// SpecialPreset::write
//
// Writes the special preset to a new 'preset' ParseTreeNode under [parent] and
// returns it
// ----------------------------------------------------------------------------
ParseTreeNode* SpecialPreset::write(ParseTreeNode* parent)
{
	auto node = new ParseTreeNode(parent, nullptr, nullptr, "preset");
	node->setName(name);
	
	// Group
	string ex_group = group;
	if (ex_group.StartsWith("Custom/"))
		ex_group = ex_group.Remove(0, 7);
	if (ex_group != "Custom")
		node->addChildPTN("group")->addStringValue(ex_group);
	
	// Special
	node->addChildPTN("special")->addIntValue(special);

	// Args
	for (unsigned a = 0; a < 5; a++)
		if (args[a] != 0)
			node->addChildPTN(S_FMT("arg%d", a + 1))->addIntValue(args[a]);
	
	// Flags
	auto node_flags = node->addChildPTN("flags");
	for (auto& flag : flags)
		node_flags->addStringValue(flag);

	return node;
}


// ----------------------------------------------------------------------------
//
// Game Namespace Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// Game::customSpecialPresets
//
// Returns all loaded custom special presets
// ----------------------------------------------------------------------------
const vector<SpecialPreset>& Game::customSpecialPresets()
{
	return custom_presets;
}

// ----------------------------------------------------------------------------
// Game::loadCustomSpecialPresets
//
// Loads user defined (custom) special presets from special_presets.cfg in the
// user data directory. Returns true on success or false if loading failed
// ----------------------------------------------------------------------------
bool Game::loadCustomSpecialPresets()
{
	// Check file exists
	string file = App::path("special_presets.cfg", App::Dir::User);
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

	auto node = parser.parseTreeRoot()->getChildPTN("special_presets");
	if (node)
	{
		for (unsigned a = 0; a < node->nChildren(); a++)
		{
			auto child = node->getChildPTN(a);
			if (S_CMPNOCASE(child->type(), "preset"))
			{
				custom_presets.push_back({});
				custom_presets.back().parse(child);

				// Add 'Custom' to preset group
				if (!custom_presets.back().group.empty())
					custom_presets.back().group = S_FMT("Custom/%s", CHR(custom_presets.back().group));
				else
					custom_presets.back().group = "Custom";
			}
		}
	}

	return true;
}

// ----------------------------------------------------------------------------
// Game::saveCustomSpecialPresets
//
// Saves all user defined (custom) special presets to special_presets.cfg in
// the user data directory. Returns true on success or false if writing failed
// ----------------------------------------------------------------------------
bool Game::saveCustomSpecialPresets()
{
	if (custom_presets.empty())
		return true;

	// Open file
	wxFile file;
	if (!file.Open(App::path("special_presets.cfg", App::Dir::User), wxFile::write))
	{
		Log::error("Unable to open special_presets.cfg file for writing");
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
		Log::error("Writing to special_presets.cfg failed");
		return false;
	}

	return true;
}


// Testing
#include "General/Console/Console.h"

CONSOLE_COMMAND(test_preset_export, 0, false)
{
	ParseTreeNode root;
	root.setName("special_presets");
	for (auto& preset : custom_presets)
		preset.write(&root);

	string out;
	root.write(out);
	Log::console(CHR(out));
}
