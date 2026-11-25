#pragma once

namespace slade::filedialog
{
struct FDInfo
{
	vector<string> filenames;
	string         extension;
	int            ext_index;
	string         path;

	bool empty() const { return path.empty(); }
};

bool openFile(
	FDInfo&     info,
	string_view caption,
	string_view extensions,
	wxWindow*   parent      = nullptr,
	string_view fn_default  = "",
	int         ext_default = 0);

string openFile(
	string_view caption,
	string_view extensions,
	wxWindow*   parent      = nullptr,
	string_view fn_default  = "",
	int         ext_default = 0);

bool   openExecutableFile(FDInfo& info, string_view caption, wxWindow* parent = nullptr, string_view fn_default = "");
string openExecutableFile(string_view caption, wxWindow* parent = nullptr, string_view fn_default = "");

bool openFiles(
	FDInfo&     info,
	string_view caption,
	string_view extensions,
	wxWindow*   parent      = nullptr,
	string_view fn_default  = "",
	int         ext_default = 0);

FDInfo openFiles(
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

string saveFile(
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

FDInfo saveFiles(string_view caption, string_view extensions, wxWindow* parent = nullptr, int ext_default = 0);

string openDirectory(string_view caption, wxWindow* parent = nullptr);

string executableExtensionString();
string executableFileName(string_view exe_name);

// A helper for saving multiple files in a single session, which handles
// checking if the file already exists and prompting the user for action.
//
// For each file to be saved, call processFile() with the filename.
// If it returns true, the file should be saved to the path in [file_path].
// If it returns false, the file should not be saved (the user chose to skip it)
struct MultiFileSaveSession
{
	string file_path;
	int    prev_response = -1;

	bool processFile(string_view filename, wxWindow* parent = nullptr);
};
} // namespace slade::filedialog
