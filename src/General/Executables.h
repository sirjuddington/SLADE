#pragma once

class Parser;
class ParseTreeNode;
class wxMenu;

namespace Executables
{
struct GameExe
{
	string             id;
	string             name;
	string             exe_name;
	string             path;
	vector<StringPair> configs;
	bool               custom;
	vector<bool>       configs_custom;
};

struct ExternalExe
{
	string category;
	string name;
	string path;
};

string writePaths();
string writeExecutables();
void   init();
void   parse(Parser* p, bool custom);

// Game executables
GameExe* gameExe(string id);
GameExe* gameExe(unsigned index);
unsigned nGameExes();
void     setGameExePath(string id, string path);
void     parseGameExe(ParseTreeNode* node, bool custom);
void     addGameExe(string name);
bool     removeGameExe(unsigned index);
void     addGameExeConfig(unsigned exe_index, string config_name, string config_params, bool custom = true);
bool     removeGameExeConfig(unsigned exe_index, unsigned config_index);

// External executables
int                 nExternalExes(string category = "");
ExternalExe         externalExe(string name, string category = "");
vector<ExternalExe> externalExes(string category = "");
void                parseExternalExe(ParseTreeNode* node);
void                addExternalExe(string name, string path, string category);
void                setExternalExeName(string name_old, string name_new, string category);
void                setExternalExePath(string name, string path, string category);
void                removeExternalExe(string name, string category);
} // namespace Executables
