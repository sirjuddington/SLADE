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
GameExe* gameExe(const string& id);
GameExe* gameExe(unsigned index);
unsigned nGameExes();
void     setGameExePath(string id, string path);
void     parseGameExe(ParseTreeNode* node, bool custom);
void     addGameExe(string name);
bool     removeGameExe(unsigned index);
void     addGameExeConfig(unsigned exe_index, string config_name, string config_params, bool custom = true);
bool     removeGameExeConfig(unsigned exe_index, unsigned config_index);

// External executables
int                 nExternalExes(const string& category = "");
ExternalExe         externalExe(const string& name, const string& category = "");
vector<ExternalExe> externalExes(const string& category = "");
void                parseExternalExe(ParseTreeNode* node);
void                addExternalExe(const string& name, const string& path, const string& category);
void                setExternalExeName(const string& name_old, const string& name_new, const string& category);
void                setExternalExePath(const string& name, const string& path, const string& category);
void                removeExternalExe(const string& name, const string& category);
} // namespace Executables
