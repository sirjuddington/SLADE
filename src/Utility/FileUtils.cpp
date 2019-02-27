
#include "Main.h"
#include "FileUtils.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

bool FileUtil::fileExists(std::string_view path)
{
	auto fs_path = fs::path{ path };
	return fs::exists(fs_path) && fs::is_regular_file(fs_path);
}

bool FileUtil::dirExists(std::string_view path)
{
	auto fs_path = fs::path{ path };
	return fs::exists(fs_path) && fs::is_directory(fs_path);
}

bool FileUtil::removeFile(std::string_view path)
{
	static std::error_code ec;
	if (!fs::remove(path, ec))
	{
		Log::warning("Unable to remove file \"{}\": {}", path, ec.message());
		return false;
	}

	return true;
}

bool FileUtil::copyFile(std::string_view from, std::string_view to)
{
	static std::error_code ec;
	if (!fs::copy_file(from, to, ec))
	{
		Log::warning("Unable to copy file from \"{}\" to \"{}\": {}", from, to, ec.message());
		return false;
	}

	return true;
}

bool FileUtil::readFileToString(const std::string& path, std::string& str)
{
	std::ifstream file(path);
	if (!file.is_open())
	{
		Log::warning("Unable to open file \"{}\" for reading", path);
		return false;
	}

	file.seekg(0, std::ios::end);
	str.reserve(file.tellg());
	file.seekg(0, std::ios::beg);
	str.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();

	return true;
}

bool FileUtil::writeStringToFile(std::string& str, const std::string& path)
{
	std::ofstream file(path);
	if (!file.is_open())
	{
		Log::warning("Unable to open file \"{}\" for writing", path);
		return false;
	}

	file << str;
	file.close();

	return true;
}

bool FileUtil::createDir(std::string_view path)
{
	return fs::create_directory(path);
}

vector<std::string> FileUtil::allFilesInDir(std::string_view path, bool include_subdirs, bool include_dir_paths)
{
	vector<std::string> paths;

	if (include_subdirs)
	{
		for (const auto& item : fs::recursive_directory_iterator(path))
			if (item.is_regular_file() || item.is_directory() && include_dir_paths)
				paths.push_back(item.path().string());
	}
	else
	{
		for (const auto& item : fs::directory_iterator(path))
			if (item.is_regular_file() || item.is_directory() && include_dir_paths)
				paths.push_back(item.path().string());
	}

	return paths;
}

time_t FileUtil::fileModifiedTime(std::string_view path)
{
	return static_cast<time_t>(fs::last_write_time(path).time_since_epoch().count());
}
