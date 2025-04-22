
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    FileUtils.cpp
// Description: Various filesystem utility functions. Also includes SFile, a
//              simple safe-ish wrapper around a c-style FILE with various
//              convenience functions
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "FileUtils.h"
#include "App.h"
#include "StringUtils.h"
#include <filesystem>
#include <fstream>

using namespace slade;
namespace fs = std::filesystem;


// -----------------------------------------------------------------------------
//
// FileUtil Namespace Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns true if a file at [path] exists
// -----------------------------------------------------------------------------
bool fileutil::fileExists(string_view path)
{
	try
	{
		auto fs_path = fs::u8path(path);
		return fs::exists(fs_path) && fs::is_regular_file(fs_path);
	}
	catch (std::exception& ex)
	{
		log::error("Error checking if file \"{}\" exists: {}", path, ex.what());
		return false;
	}
}

// -----------------------------------------------------------------------------
// Returns true if a directory at [path] exists
// -----------------------------------------------------------------------------
bool fileutil::dirExists(string_view path)
{
	try
	{
		auto fs_path = fs::u8path(path);
		return fs::exists(fs_path) && fs::is_directory(fs_path);
	}
	catch (std::exception& ex)
	{
		log::error("Error checking if dir \"{}\" exists: {}", path, ex.what());
		return false;
	}
}

// -----------------------------------------------------------------------------
// Returns true if [path] is a valid executable (platform dependant)
// -----------------------------------------------------------------------------
bool fileutil::validExecutable(string_view path)
{
	// Special handling for MacOS .app dir
	if (app::platform() == app::Platform::MacOS)
	{
		if (strutil::endsWithCI(path, ".app") && dirExists(path))
			return true;
	}

	// Invalid if file doesn't exist
	if (!fileExists(path))
		return false;

	// Check for .exe or .bat extension on Windows
	if (app::platform() == app::Platform::Windows)
	{
		if (!strutil::endsWithCI(path, ".exe") && !strutil::endsWithCI(path, ".bat"))
			return false;
	}

	// TODO: Check for executable permission on Linux/MacOS

	// Passed all checks, is valid executable
	return true;
}

// -----------------------------------------------------------------------------
// Removes the file at [path], returns true if successful
// -----------------------------------------------------------------------------
bool fileutil::removeFile(string_view path)
{
	static std::error_code ec;
	if (!fs::remove(fs::u8path(path), ec))
	{
		log::warning("Unable to remove file \"{}\": {}", path, ec.message());
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------
// Copies the file at [from] to a file at [to]. If [overwrite] is true, it will
// overwrite the file if it already exists.
// -----------------------------------------------------------------------------
bool fileutil::copyFile(string_view from, string_view to, bool overwrite)
{
	static std::error_code ec;
	auto                   options = overwrite ? fs::copy_options::overwrite_existing : fs::copy_options::none;
	if (!fs::copy_file(fs::u8path(from), fs::u8path(to), options, ec))
	{
		log::warning("Unable to copy file from \"{}\" to \"{}\": {}", from, to, ec.message());
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------
// Reads all text from the file at [path] into [str]
// -----------------------------------------------------------------------------
bool fileutil::readFileToString(const string& path, string& str)
{
	std::ifstream file(path);
	if (!file.is_open())
	{
		log::warning("Unable to open file \"{}\" for reading", path);
		return false;
	}

	file.seekg(0, std::ios::end);
	str.reserve(file.tellg());
	file.seekg(0, std::ios::beg);
	str.assign(std::istreambuf_iterator(file), std::istreambuf_iterator<char>());
	file.close();

	return true;
}

// -----------------------------------------------------------------------------
// Writes [str] to a file at [path].
// Will overwrite the file if it already exists
// -----------------------------------------------------------------------------
bool fileutil::writeStringToFile(const string& str, const string& path)
{
	std::ofstream file(path);
	if (!file.is_open())
	{
		log::warning("Unable to open file \"{}\" for writing", path);
		return false;
	}

	file << str;
	file.close();

	return true;
}

// -----------------------------------------------------------------------------
// Creates a new directory at [path] if it doesn't aleady exist.
// Returns false if the directory doesn't exist and could not be created
// -----------------------------------------------------------------------------
bool fileutil::createDir(string_view path)
{
	static std::error_code ec;
	if (!fs::create_directory(fs::u8path(path), ec))
	{
		if (ec.value() != 0)
			log::warning("Unable to create directory \"{}\": {}", path, ec.message());

		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------
// Removes the directory at [path] and all its contents.
// Returns true if successful
// -----------------------------------------------------------------------------
bool fileutil::removeDir(string_view path)
{
	static std::error_code ec;
	if (!fs::remove_all(fs::u8path(path), ec))
	{
		log::warning("Unable to remove directory \"{}\": {}", path, ec.message());
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------
// Returns a list of all files in the directory at [path].
// If [include_subdirs] is true, it will also include all files in
// subdirectories (recursively).
// If [include_dir_paths] is true, each filename will be prefixed with the given
// [path]
// -----------------------------------------------------------------------------
vector<string> fileutil::allFilesInDir(string_view path, bool include_subdirs, bool include_dir_paths)
{
	vector<string> paths;

	if (!dirExists(path))
		return paths;

	if (include_subdirs)
	{
		for (const auto& item : fs::recursive_directory_iterator(fs::u8path(path)))
			if (item.is_regular_file() || item.is_directory() && include_dir_paths)
				paths.push_back(item.path().u8string());
	}
	else
	{
		for (const auto& item : fs::directory_iterator(fs::u8path(path)))
			if (item.is_regular_file() || item.is_directory() && include_dir_paths)
				paths.push_back(item.path().u8string());
	}

	return paths;
}

// -----------------------------------------------------------------------------
// Returns the modification time of the file at [path], or 0 if the file doesn't
// exist or can't be acessed
// -----------------------------------------------------------------------------
time_t fileutil::fileModifiedTime(string_view path)
{
#if 0
	// Use this whenever we update to C++20
	const auto file_time = std::filesystem::last_write_time(path);
	const auto sys_time  = std::chrono::clock_cast<std::chrono::system_clock>(file_time);
	return std::chrono::system_clock::to_time_t(sys_time);
#endif

	return wxFileModificationTime(wxString::FromUTF8(path.data(), path.size()));
}



// -----------------------------------------------------------------------------
//
// SFile Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SFile class constructor
// -----------------------------------------------------------------------------
SFile::SFile(string_view path, Mode mode)
{
	open(string{ path }, mode);
}

// -----------------------------------------------------------------------------
// Returns the current read/write position in the file
// -----------------------------------------------------------------------------
unsigned SFile::currentPos() const
{
	return handle_ ? ftell(handle_) : 0;
}

// -----------------------------------------------------------------------------
// Opens the file at [path] in [mode] (read/write/etc.)
// -----------------------------------------------------------------------------
bool SFile::open(const string& path, Mode mode)
{
	// Needs to be closed first if already open
	if (handle_)
		return false;

#ifdef __WXMSW__
	// Convert path to UTF-16 for Windows
	auto wpath = wxString::FromUTF8(path);
	switch (mode)
	{
	case Mode::ReadOnly: handle_ = _wfopen(wpath.wc_str(), L"rb"); break;
	case Mode::Write: handle_ = _wfopen(wpath.wc_str(), L"wb"); break;
	case Mode::ReadWite: handle_ = _wfopen(wpath.wc_str(), L"r+b"); break;
	case Mode::Append: handle_ = _wfopen(wpath.wc_str(), L"ab"); break;
	}
#else
	switch (mode)
	{
	case Mode::ReadOnly: handle_ = fopen(path.c_str(), "rb"); break;
	case Mode::Write: handle_ = fopen(path.c_str(), "wb"); break;
	case Mode::ReadWite: handle_ = fopen(path.c_str(), "r+b"); break;
	case Mode::Append: handle_ = fopen(path.c_str(), "ab"); break;
	}
#endif

	if (handle_)
		stat(path.c_str(), &stat_);

	return handle_ != nullptr;
}

// -----------------------------------------------------------------------------
// Closes the file
// -----------------------------------------------------------------------------
void SFile::close()
{
	if (handle_)
	{
		fclose(handle_);
		handle_ = nullptr;
	}
}

// -----------------------------------------------------------------------------
// Seeks ahead by [offset] bytes from the current position
// -----------------------------------------------------------------------------
bool SFile::seek(unsigned offset)
{
	return handle_ ? fseek(handle_, offset, SEEK_CUR) == 0 : false;
}

// -----------------------------------------------------------------------------
// Seeks to [offset] bytes from the beginning of the file
// -----------------------------------------------------------------------------
bool SFile::seekFromStart(unsigned offset)
{
	return handle_ ? fseek(handle_, offset, SEEK_SET) == 0 : false;
}

// -----------------------------------------------------------------------------
// Seeks to [offset] bytes back from the end of the file
// -----------------------------------------------------------------------------
bool SFile::seekFromEnd(unsigned offset)
{
	return handle_ ? fseek(handle_, offset, SEEK_END) == 0 : false;
}

// -----------------------------------------------------------------------------
// Reads [count] bytes from the file into [buffer]
// -----------------------------------------------------------------------------
bool SFile::read(void* buffer, unsigned count)
{
	if (handle_)
		return fread(buffer, count, 1, handle_) > 0;

	return false;
}

// -----------------------------------------------------------------------------
// Reads [count] bytes from the file into a MemChunk [mc]
// (replaces the existing contents of the MemChunk)
// -----------------------------------------------------------------------------
bool SFile::read(MemChunk& mc, unsigned count)
{
	return mc.importFileStream(*this, count);
}

// -----------------------------------------------------------------------------
// Reads [count] characters from the file into a string [str]
// (replaces the existing contents of the string)
// -----------------------------------------------------------------------------
bool SFile::read(string& str, unsigned count) const
{
	if (handle_)
	{
		str.resize(count);
		auto c = fread(&str[0], 1, count, handle_);
		str.push_back('\0');
		return c > 0;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Writes [count] bytes from [buffer] to the file
// -----------------------------------------------------------------------------
bool SFile::write(const void* buffer, unsigned count)
{
	if (handle_)
		return fwrite(buffer, count, 1, handle_) > 0;

	return false;
}

// -----------------------------------------------------------------------------
// Writes [str] to the file
// -----------------------------------------------------------------------------
bool SFile::writeStr(string_view str) const
{
	if (handle_)
		return fwrite(str.data(), 1, str.size(), handle_);

	return false;
}
