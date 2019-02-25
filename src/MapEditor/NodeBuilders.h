#pragma once

namespace NodeBuilders
{
struct Builder
{
	std::string         id;
	std::string         name;
	std::string         path;
	std::string         command;
	std::string         exe;
	vector<std::string> options;
	vector<std::string> option_desc;
};

void     init();
void     addBuilderPath(std::string_view builder, std::string_view path);
void     saveBuilderPaths(wxFile& file);
unsigned nNodeBuilders();
Builder& builder(std::string_view id);
Builder& builder(unsigned index);
} // namespace NodeBuilders
