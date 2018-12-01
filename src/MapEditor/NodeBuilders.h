#pragma once

namespace NodeBuilders
{
struct Builder
{
	string         id;
	string         name;
	string         path;
	string         command;
	string         exe;
	vector<string> options;
	vector<string> option_desc;
};

void     init();
void     addBuilderPath(string builder, string path);
void     saveBuilderPaths(wxFile& file);
unsigned nNodeBuilders();
Builder& builder(string id);
Builder& builder(unsigned index);
} // namespace NodeBuilders
