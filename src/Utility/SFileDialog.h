#pragma once

namespace SFileDialog
{
struct FDInfo
{
	wxArrayString filenames;
	string        extension;
	int           ext_index;
	string        path;
};

bool openFile(
	FDInfo&   info,
	string    caption,
	string    extensions,
	wxWindow* parent      = nullptr,
	string    fn_default  = "",
	int       ext_default = 0);
bool openFiles(
	FDInfo&   info,
	string    caption,
	string    extensions,
	wxWindow* parent      = nullptr,
	string    fn_default  = "",
	int       ext_default = 0);
bool saveFile(
	FDInfo&   info,
	string    caption,
	string    extensions,
	wxWindow* parent      = nullptr,
	string    fn_default  = "",
	int       ext_default = 0);
bool saveFiles(FDInfo& info, string caption, string extensions, wxWindow* parent = nullptr, int ext_default = 0);

string executableExtensionString();
string executableFileName(const string& exe_name);
} // namespace SFileDialog
