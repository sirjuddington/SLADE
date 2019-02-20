#pragma once

namespace FileUtil
{
bool                fileExists(std::string_view path);
bool                dirExists(std::string_view path);
bool                removeFile(std::string_view path);
bool                copyFile(std::string_view from, std::string_view to);
bool                readFileToString(const std::string& path, std::string& str);
bool                writeStringToFile(std::string& str, const std::string& path);
bool                createDir(std::string_view path);
vector<std::string> allFilesInDir(std::string_view path, bool include_subdirs = false, bool include_dir_paths = false);
} // namespace FileUtil
