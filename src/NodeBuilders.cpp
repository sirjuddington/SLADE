
#include "Main.h"
#include "ArchiveManager.h"
#include "NodeBuilders.h"
#include "Parser.h"

namespace NodeBuilders
{
	vector<builder_t>	builders;
	builder_t			invalid;
	string				custom;
	vector<string>		builder_paths;
}

void NodeBuilders::init()
{
	// Init invalid builder
	invalid.id = "invalid";

	// Get nodebuilders configuration from slade.pk3
	Archive* archive = theArchiveManager->programResourceArchive();
	ArchiveEntry* config = archive->entryAtPath("config/nodebuilders.cfg");
	if (!config)
		return;

	// Parse it
	Parser parser;
	parser.parseText(config->getMCData(), "nodebuilders.cfg");

	// Get 'nodebuilders' block
	ParseTreeNode* root = (ParseTreeNode*)parser.parseTreeRoot()->getChild("nodebuilders");
	if (!root)
		return;

	// Go through child block
	for (unsigned a = 0; a < root->nChildren(); a++)
	{
		ParseTreeNode* n_builder = (ParseTreeNode*)root->getChild(a);

		// Parse builder block
		builder_t builder;
		builder.id = n_builder->getName();
		for (unsigned b = 0; b < n_builder->nChildren(); b++)
		{
			ParseTreeNode* node = (ParseTreeNode*)n_builder->getChild(b);

			// Option
			if (S_CMPNOCASE(node->getType(), "option"))
			{
				builder.options.push_back(node->getName());
				builder.option_desc.push_back(node->getStringValue());
			}

			// Builder name
			else if (S_CMPNOCASE(node->getName(), "name"))
				builder.name = node->getStringValue();

			// Builder command
			else if (S_CMPNOCASE(node->getName(), "command"))
				builder.command = node->getStringValue();

			// Builder executable
			else if (S_CMPNOCASE(node->getName(), "executable"))
				builder.exe = node->getStringValue();
		}
		builders.push_back(builder);
	}

	// Set builder paths
	for (unsigned a = 0; a < builder_paths.size(); a+=2)
		getBuilder(builder_paths[a]).path = builder_paths[a+1];
}

void NodeBuilders::addBuilderPath(string builder, string path)
{
	builder_paths.push_back(builder);
	builder_paths.push_back(path);
}

void NodeBuilders::saveBuilderPaths(wxFile& file)
{
	file.Write("nodebuilder_paths\n{\n");
	for (unsigned a = 0; a < builders.size(); a++)
		file.Write(S_FMT("\t%s \"%s\"\n", CHR(builders[a].id), CHR(builders[a].path)));
	file.Write("}\n");
}

unsigned NodeBuilders::nNodeBuilders()
{
	return builders.size();
}

NodeBuilders::builder_t& NodeBuilders::getBuilder(string id)
{
	for (unsigned a = 0; a < builders.size(); a++)
	{
		if (builders[a].id == id)
			return builders[a];
	}

	return invalid;
}

NodeBuilders::builder_t& NodeBuilders::getBuilder(unsigned index)
{
	// Check index
	if (index >= builders.size())
		return invalid;

	return builders[index];
}
