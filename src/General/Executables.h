#pragma once

class Parser;
class ParseTreeNode;
class wxMenu;

namespace Executables
{
struct GameExe
{
	wxString           id;
	wxString           name;
	wxString           exe_name;
	wxString           path;
	vector<StringPair> configs;
	bool               custom;
	vector<bool>       configs_custom;
};

struct ExternalExe
{
	wxString category;
	wxString name;
	wxString path;
};

wxString writePaths();
wxString writeExecutables();
void     init();
void     parse(Parser* p, bool custom);

// Game executables
GameExe* gameExe(const wxString& id);
GameExe* gameExe(unsigned index);
unsigned nGameExes();
void     setGameExePath(wxString id, wxString path);
void     parseGameExe(ParseTreeNode* node, bool custom);
void     addGameExe(wxString name);
bool     removeGameExe(unsigned index);
void     addGameExeConfig(unsigned exe_index, wxString config_name, wxString config_params, bool custom = true);
bool     removeGameExeConfig(unsigned exe_index, unsigned config_index);

// External executables
int                 nExternalExes(const wxString& category = "");
ExternalExe         externalExe(const wxString& name, const wxString& category = "");
vector<ExternalExe> externalExes(const wxString& category = "");
void                parseExternalExe(ParseTreeNode* node);
void                addExternalExe(const wxString& name, const wxString& path, const wxString& category);
void                setExternalExeName(const wxString& name_old, const wxString& name_new, const wxString& category);
void                setExternalExePath(const wxString& name, const wxString& path, const wxString& category);
void                removeExternalExe(const wxString& name, const wxString& category);
} // namespace Executables
