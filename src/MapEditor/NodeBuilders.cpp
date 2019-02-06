
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    NodeBuilders.cpp
// Description: NodeBuilders namespace - functions for handling node builder
//              definitions
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
#include "NodeBuilders.h"
#include "Archive/ArchiveManager.h"
#include "Utility/Parser.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace NodeBuilders
{
vector<Builder>  builders;
Builder          invalid;
Builder          none;
wxString         custom;
vector<wxString> builder_paths;
} // namespace NodeBuilders


// -----------------------------------------------------------------------------
//
// NodeBuilders Namespace Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Loads all node builder definitions from the program resource
// -----------------------------------------------------------------------------
void NodeBuilders::init()
{
	// Init default builders
	invalid.id = "invalid";
	none.id    = "none";
	none.name  = "Don't Build Nodes";
	builders.push_back(none);

	// Get nodebuilders configuration from slade.pk3
	auto archive = App::archiveManager().programResourceArchive();
	auto config  = archive->entryAtPath("config/nodebuilders.cfg");
	if (!config)
		return;

	// Parse it
	Parser parser;
	parser.parseText(config->data(), "nodebuilders.cfg");

	// Get 'nodebuilders' block
	auto root = parser.parseTreeRoot()->childPTN("nodebuilders");
	if (!root)
		return;

	// Go through child block
	for (unsigned a = 0; a < root->nChildren(); a++)
	{
		auto n_builder = root->childPTN(a);

		// Parse builder block
		Builder builder;
		builder.id = n_builder->name();
		for (unsigned b = 0; b < n_builder->nChildren(); b++)
		{
			auto node = n_builder->childPTN(b);

			// Option
			if (S_CMPNOCASE(node->type(), "option"))
			{
				builder.options.push_back(node->name());
				builder.option_desc.push_back(node->stringValue());
			}

			// Builder name
			else if (S_CMPNOCASE(node->name(), "name"))
				builder.name = node->stringValue();

			// Builder command
			else if (S_CMPNOCASE(node->name(), "command"))
				builder.command = node->stringValue();

			// Builder executable
			else if (S_CMPNOCASE(node->name(), "executable"))
				builder.exe = node->stringValue();
		}
		builders.push_back(builder);
	}

	// Set builder paths
	for (unsigned a = 0; a < builder_paths.size(); a += 2)
		builder(builder_paths[a]).path = builder_paths[a + 1];
}

// -----------------------------------------------------------------------------
// Adds [path] for [builder]
// -----------------------------------------------------------------------------
void NodeBuilders::addBuilderPath(wxString builder, wxString path)
{
	builder_paths.push_back(builder);
	builder_paths.push_back(path);
}

// -----------------------------------------------------------------------------
// Writes builder paths to [file]
// -----------------------------------------------------------------------------
void NodeBuilders::saveBuilderPaths(wxFile& file)
{
	file.Write("nodebuilder_paths\n{\n");
	for (unsigned a = 0; a < builders.size(); a++)
	{
		wxString path = builders[a].path;
		path.Replace("\\", "/");
		file.Write(S_FMT("\t%s \"%s\"\n", builders[a].id, path));
	}
	file.Write("}\n");
}

// -----------------------------------------------------------------------------
// Returns the number of node builders defined
// -----------------------------------------------------------------------------
unsigned NodeBuilders::nNodeBuilders()
{
	return builders.size();
}

// -----------------------------------------------------------------------------
// Returns the node builder definition matching [id]
// -----------------------------------------------------------------------------
NodeBuilders::Builder& NodeBuilders::builder(wxString id)
{
	for (unsigned a = 0; a < builders.size(); a++)
	{
		if (builders[a].id == id)
			return builders[a];
	}

	return invalid;
}

// -----------------------------------------------------------------------------
// Returns the node builder definition at [index]
// -----------------------------------------------------------------------------
NodeBuilders::Builder& NodeBuilders::builder(unsigned index)
{
	// Check index
	if (index >= builders.size())
		return invalid;

	return builders[index];
}
