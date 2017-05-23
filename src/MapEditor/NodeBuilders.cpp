
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    NodeBuilders.cpp
 * Description: NodeBuilders namespace - functions for handling
 *              node builder definitions
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "Archive/ArchiveManager.h"
#include "NodeBuilders.h"
#include "Utility/Parser.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
namespace NodeBuilders
{
	vector<builder_t>	builders;
	builder_t			invalid;
	builder_t			none;
	string				custom;
	vector<string>		builder_paths;
}


/*******************************************************************
 * NODEBUILDERS NAMESPACE FUNCTIONS
 *******************************************************************/

/* NodeBuilders::init
 * Loads all node builder definitions from the program resource
 *******************************************************************/
void NodeBuilders::init()
{
	// Init default builders
	invalid.id = "invalid";
	none.id = "none";
	none.name = "Don't Build Nodes";
	builders.push_back(none);

	// Get nodebuilders configuration from slade.pk3
	Archive* archive = theArchiveManager->programResourceArchive();
	ArchiveEntry* config = archive->entryAtPath("config/nodebuilders.cfg");
	if (!config)
		return;

	// Parse it
	Parser parser;
	parser.parseText(config->getMCData(), "nodebuilders.cfg");

	// Get 'nodebuilders' block
	auto root = parser.parseTreeRoot()->getChildPTN("nodebuilders");
	if (!root)
		return;

	// Go through child block
	for (unsigned a = 0; a < root->nChildren(); a++)
	{
		auto n_builder = root->getChildPTN(a);

		// Parse builder block
		builder_t builder;
		builder.id = n_builder->getName();
		for (unsigned b = 0; b < n_builder->nChildren(); b++)
		{
			auto node = n_builder->getChildPTN(b);

			// Option
			if (S_CMPNOCASE(node->type(), "option"))
			{
				builder.options.push_back(node->getName());
				builder.option_desc.push_back(node->stringValue());
			}

			// Builder name
			else if (S_CMPNOCASE(node->getName(), "name"))
				builder.name = node->stringValue();

			// Builder command
			else if (S_CMPNOCASE(node->getName(), "command"))
				builder.command = node->stringValue();

			// Builder executable
			else if (S_CMPNOCASE(node->getName(), "executable"))
				builder.exe = node->stringValue();
		}
		builders.push_back(builder);
	}

	// Set builder paths
	for (unsigned a = 0; a < builder_paths.size(); a+=2)
		getBuilder(builder_paths[a]).path = builder_paths[a+1];
}

/* NodeBuilders::addBUilderPath
 * Adds [path] for [builder]
 *******************************************************************/
void NodeBuilders::addBuilderPath(string builder, string path)
{
	builder_paths.push_back(builder);
	builder_paths.push_back(path);
}

/* NodeBuilders::saveBuilderPaths
 * Writes builder paths to [file]
 *******************************************************************/
void NodeBuilders::saveBuilderPaths(wxFile& file)
{
	file.Write("nodebuilder_paths\n{\n");
	for (unsigned a = 0; a < builders.size(); a++)
	{
		string path = builders[a].path;
		path.Replace("\\", "/");
		file.Write(S_FMT("\t%s \"%s\"\n", builders[a].id, path));
	}
	file.Write("}\n");
}

/* NodeBuilders::nNodeBuilders
 * Returns the number of node builders defined
 *******************************************************************/
unsigned NodeBuilders::nNodeBuilders()
{
	return builders.size();
}

/* NodeBuilders::getBuilder
 * Returns the node builder definition matching [id]
 *******************************************************************/
NodeBuilders::builder_t& NodeBuilders::getBuilder(string id)
{
	for (unsigned a = 0; a < builders.size(); a++)
	{
		if (builders[a].id == id)
			return builders[a];
	}

	return invalid;
}

/* NodeBuilders::getBuilder
 * Returns the node builder definition at [index]
 *******************************************************************/
NodeBuilders::builder_t& NodeBuilders::getBuilder(unsigned index)
{
	// Check index
	if (index >= builders.size())
		return invalid;

	return builders[index];
}
