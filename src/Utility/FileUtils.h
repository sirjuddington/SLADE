#pragma once

namespace FileUtil
{
bool fileExists(std::string_view path);
bool removeFile(std::string_view path);
bool copyFile(std::string_view from, std::string_view to);
}
