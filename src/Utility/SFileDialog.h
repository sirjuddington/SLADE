#pragma once

namespace SFileDialog
{
struct FDInfo
{
	vector<std::string> filenames;
	std::string         extension;
	int                 ext_index;
	std::string         path;
};

bool openFile(
	FDInfo&          info,
	std::string_view caption,
	std::string_view extensions,
	wxWindow*        parent      = nullptr,
	std::string_view fn_default  = "",
	int              ext_default = 0);

bool openFiles(
	FDInfo&          info,
	std::string_view caption,
	std::string_view extensions,
	wxWindow*        parent      = nullptr,
	std::string_view fn_default  = "",
	int              ext_default = 0);

bool saveFile(
	FDInfo&          info,
	std::string_view caption,
	std::string_view extensions,
	wxWindow*        parent      = nullptr,
	std::string_view fn_default  = "",
	int              ext_default = 0);

bool saveFiles(
	FDInfo&          info,
	std::string_view caption,
	std::string_view extensions,
	wxWindow*        parent      = nullptr,
	int              ext_default = 0);

std::string executableExtensionString();
std::string executableFileName(std::string_view exe_name);
} // namespace SFileDialog
