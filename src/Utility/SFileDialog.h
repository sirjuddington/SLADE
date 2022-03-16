#pragma once

namespace slade::filedialog
{
struct FDInfo
{
	vector<string> filenames;
	string         extension;
	int            ext_index;
	string         path;
};

bool openFile(
	FDInfo&     info,
	string_view caption,
	string_view extensions,
	wxWindow*   parent      = nullptr,
	string_view fn_default  = "",
	int         ext_default = 0);

bool openExecutableFile(
	FDInfo& info,
	string_view caption,
	wxWindow* parent = nullptr,
	string_view fn_default = "");

bool openFiles(
	FDInfo&     info,
	string_view caption,
	string_view extensions,
	wxWindow*   parent      = nullptr,
	string_view fn_default  = "",
	int         ext_default = 0);

bool saveFile(
	FDInfo&     info,
	string_view caption,
	string_view extensions,
	wxWindow*   parent      = nullptr,
	string_view fn_default  = "",
	int         ext_default = 0);

bool saveFiles(
	FDInfo&     info,
	string_view caption,
	string_view extensions,
	wxWindow*   parent      = nullptr,
	int         ext_default = 0);

string executableExtensionString();
string executableFileName(string_view exe_name);
} // namespace slade::filedialog
