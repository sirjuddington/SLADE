#pragma once

class Parser;
class ParseTreeNode;
class wxMenu;

namespace Executables
{
struct GameExe
{
	std::string        id;
	std::string        name;
	std::string        exe_name;
	std::string        path;
	vector<StringPair> configs;
	bool               custom;
	vector<bool>       configs_custom;
};

struct ExternalExe
{
	std::string category;
	std::string name;
	std::string path;
};

std::string writePaths();
std::string writeExecutables();
void        init();
void        parse(Parser* p, bool custom);

// Game executables
GameExe* gameExe(std::string_view id);
GameExe* gameExe(unsigned index);
unsigned nGameExes();
void     setGameExePath(std::string_view id, std::string_view path);
void     parseGameExe(ParseTreeNode* node, bool custom);
void     addGameExe(std::string_view name);
bool     removeGameExe(unsigned index);
void     addGameExeConfig(
		unsigned         exe_index,
		std::string_view config_name,
		std::string_view config_params,
		bool             custom = true);
bool removeGameExeConfig(unsigned exe_index, unsigned config_index);

// External executables
int                 nExternalExes(std::string_view category = "");
ExternalExe         externalExe(std::string_view name, std::string_view category = "");
vector<ExternalExe> externalExes(std::string_view category = "");
void                parseExternalExe(ParseTreeNode* node);
void                addExternalExe(std::string_view name, std::string_view path, std::string_view category);
void                setExternalExeName(std::string_view name_old, std::string_view name_new, std::string_view category);
void                setExternalExePath(std::string_view name, std::string_view path, std::string_view category);
void                removeExternalExe(std::string_view name, std::string_view category);
} // namespace Executables
