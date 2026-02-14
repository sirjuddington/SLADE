
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
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
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveManager.h"
#include "Utility/FileUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace slade::nodebuilders
{
vector<Builder> builders;
Builder         invalid;
Builder         none;
string          custom;
vector<string>  builder_paths;
} // namespace slade::nodebuilders


// -----------------------------------------------------------------------------
//
// NodeBuilders Namespace Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Loads all node builder definitions from the program resource
// -----------------------------------------------------------------------------
void nodebuilders::init()
{
	// Init default builders
	invalid.id = "invalid";
	none.id    = "none";
	none.name  = "Don't Build Nodes";
	builders.push_back(none);

	// Get nodebuilders configuration from slade.pk3
	auto archive = app::archiveManager().programResourceArchive();
	auto config  = archive->entryAtPath("config/nodebuilders.json");
	if (!config)
		return;

	// Parse it
	if (auto j = jsonutil::parse(config->data()); !j.is_discarded())
	{
		for (auto& [id, j_builder] : j.items())
		{
			Builder builder;
			builder.id      = id;
			builder.name    = j_builder["name"];
			builder.command = j_builder["command"];
			builder.exe     = j_builder["executable"];

			if (j_builder.contains("options"))
			{
				for (auto& option : j_builder["options"])
				{
					builder.options.push_back(option["parameter"]);
					builder.option_desc.push_back(option["description"]);
				}
			}

			builders.push_back(builder);
		}
	}

	// Set builder paths
	for (unsigned a = 0; a < builder_paths.size(); a += 2)
		builder(builder_paths[a]).path = builder_paths[a + 1];

	// Try to find any builder executables not already set
	for (auto& builder : builders)
	{
		if (builder.path.empty())
		{
			auto found_path = fileutil::findExecutable(builder.exe, "nodebuilders");
			if (!found_path.empty())
				builder.path = found_path;
		}
	}
}

// -----------------------------------------------------------------------------
// Adds [path] for [builder]
// -----------------------------------------------------------------------------
void nodebuilders::addBuilderPath(string_view builder, string_view path)
{
	builder_paths.emplace_back(builder);
	builder_paths.emplace_back(path);
}

// -----------------------------------------------------------------------------
// Writes builder paths to [json]
// -----------------------------------------------------------------------------
void nodebuilders::writeBuilderPaths(Json& json)
{
	for (auto& builder : builders)
	{
		auto path = builder.path;
		std::replace(path.begin(), path.end(), '\\', '/');
		json["nodebuilder_paths"][builder.id] = path;
	}
}

// -----------------------------------------------------------------------------
// Returns the number of node builders defined
// -----------------------------------------------------------------------------
unsigned nodebuilders::nNodeBuilders()
{
	return builders.size();
}

// -----------------------------------------------------------------------------
// Returns the node builder definition matching [id]
// -----------------------------------------------------------------------------
nodebuilders::Builder& nodebuilders::builder(string_view id)
{
	for (auto& builder : builders)
	{
		if (builder.id == id)
			return builder;
	}

	return invalid;
}

// -----------------------------------------------------------------------------
// Returns the node builder definition at [index]
// -----------------------------------------------------------------------------
nodebuilders::Builder& nodebuilders::builder(unsigned index)
{
	// Check index
	if (index >= builders.size())
		return invalid;

	return builders[index];
}
