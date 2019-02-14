
#include "Main.h"
#include "FileUtils.h"
#include <filesystem>

namespace fs = std::filesystem;

bool FileUtil::fileExists(std::string_view path)
{
	return fs::exists(path);
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
