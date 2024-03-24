#pragma once

class wxMenu;

namespace slade
{
class Parser;

namespace executables
{
	struct GameExe
	{
		string             id;
		string             name;
		string             exe_name;
		string             path;
		vector<StringPair> run_configs;
		vector<StringPair> map_configs;
		bool               custom;
		vector<bool>       run_configs_custom;
		vector<bool>       map_configs_custom;
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
	void   parse(const Parser* p, bool custom);

	// Game executables
	GameExe* gameExe(string_view id);
	GameExe* gameExe(unsigned index);
	unsigned nGameExes();
	void     setGameExePath(string_view id, string_view path);
	void     parseGameExe(const ParseTreeNode* node, bool custom);
	void     addGameExe(string_view name);
	bool     removeGameExe(unsigned index);
	void     addGameExeRunConfig(
			unsigned    exe_index,
			string_view config_name,
			string_view config_params,
			bool        custom = true);
	bool removeGameExeRunConfig(unsigned exe_index, unsigned config_index);
	void addGameExeMapConfig(
		unsigned    exe_index,
		string_view config_name,
		string_view config_params,
		bool        custom = true);
	bool removeGameExeMapConfig(unsigned exe_index, unsigned config_index);

	// External executables
	int                 nExternalExes(string_view category = "");
	ExternalExe         externalExe(string_view name, string_view category = "");
	vector<ExternalExe> externalExes(string_view category = "");
	void                parseExternalExe(const ParseTreeNode* node);
	void                addExternalExe(string_view name, string_view path, string_view category);
	void                setExternalExeName(string_view name_old, string_view name_new, string_view category);
	void                setExternalExePath(string_view name, string_view path, string_view category);
	void                removeExternalExe(string_view name, string_view category);
} // namespace executables
} // namespace slade
