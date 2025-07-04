#pragma once

namespace slade::nodebuilders
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
void     addBuilderPath(string_view builder, string_view path);
void     writeBuilderPaths(nlohmann::json& json);
unsigned nNodeBuilders();
Builder& builder(string_view id);
Builder& builder(unsigned index);
} // namespace slade::nodebuilders
