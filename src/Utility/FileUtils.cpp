
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
#include <fstream>

using namespace slade;


// -----------------------------------------------------------------------------
//
// FileUtil Namespace Functions
//
// -----------------------------------------------------------------------------
namespace slade::fileutil
{
// -----------------------------------------------------------------------------
// Converts a string_view to wxString using UTF-8 encoding
// -----------------------------------------------------------------------------
inline wxString fromUtf8(string_view str)
{
#if wxCHECK_VERSION(3, 3, 0)
	return wxString::FromUTF8(str);
#else
	return wxString::FromUTF8(str.data(), str.size());
#endif
}

// -----------------------------------------------------------------------------
// Returns the platform-specific path separator
// -----------------------------------------------------------------------------
inline string pathSeparator()
{
#ifdef __WXMSW__
	return "\\";
#else
	return "/";
#endif
}
} // namespace slade::fileutil


// -----------------------------------------------------------------------------
// Returns true if a file at [path] exists
// -----------------------------------------------------------------------------
bool fileutil::fileExists(string_view path)
{
	return !path.empty() && wxFileExists(fromUtf8(path));
}

// -----------------------------------------------------------------------------
// Returns true if a directory at [path] exists
// -----------------------------------------------------------------------------
bool fileutil::dirExists(string_view path)
{
	return !path.empty() && wxDirExists(fromUtf8(path));
}

// -----------------------------------------------------------------------------
// Returns true if [path] is a valid executable (platform dependant)
// -----------------------------------------------------------------------------
bool fileutil::validExecutable(string_view path)
{
	if (path.empty())
		return false;

	// Special handling for macOS .app dir
	if (app::platform() == app::Platform::MacOS)
	{
		if (strutil::endsWithCI(path, ".app") && dirExists(path))
			return true;
	}

	// Check for .exe or .bat extension on Windows
	if (app::platform() == app::Platform::Windows)
	{
		if (!strutil::endsWithCI(path, ".exe") && !strutil::endsWithCI(path, ".bat"))
			return false;

		// Invalid if file doesn't exist
		if (!fileExists(path))
			return false;
	}

	// TODO: Check if file OR command exists on Linux/macOS
	// TODO: Check for executable permission on Linux/macOS

	// Passed all checks, is valid executable
	return true;
}

// -----------------------------------------------------------------------------
// Removes the file at [path], returns true if successful
// -----------------------------------------------------------------------------
bool fileutil::removeFile(string_view path)
{
	return !path.empty() && wxRemoveFile(fromUtf8(path));
}

// -----------------------------------------------------------------------------
// Copies the file at [from] to a file at [to]. If [overwrite] is true, it will
// overwrite the file if it already exists.
// -----------------------------------------------------------------------------
bool fileutil::copyFile(string_view from, string_view to, bool overwrite)
{
	if (from.empty() || to.empty())
		return false;

	return wxCopyFile(fromUtf8(from), fromUtf8(to), overwrite);
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
	return !path.empty() && wxMkdir(fromUtf8(path));
}

// -----------------------------------------------------------------------------
// Removes the directory at [path] and all its contents.
// Returns true if successful
// -----------------------------------------------------------------------------
bool fileutil::removeDir(string_view path)
{
	return !path.empty() && wxRmdir(fromUtf8(path));
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

	if (path.empty())
		return paths;

	wxArrayString all_files;
	wxDir::GetAllFiles(
		fromUtf8(path), &all_files, wxEmptyString, include_subdirs ? wxDIR_FILES | wxDIR_DIRS : wxDIR_FILES);

	for (const auto& file : all_files)
	{
		if (include_dir_paths)
			paths.push_back(file.utf8_string());
		else
			paths.push_back(strutil::replace(file.utf8_string(), path, ""));
	}

	return paths;
}

// -----------------------------------------------------------------------------
// Returns the modification time of the file at [path], or 0 if the file doesn't
// exist or can't be acessed
// -----------------------------------------------------------------------------
time_t fileutil::fileModifiedTime(string_view path)
{
	if (path.empty())
		return 0;

	return wxFileModificationTime(fromUtf8(path));
}

// -----------------------------------------------------------------------------
// Searches the system PATH for an executable named [exe_name].
// Returns the full path to the executable if found, or an empty string if not
// -----------------------------------------------------------------------------
string fileutil::findExecutable(string_view exe_name, string_view bundle_dir)
{
	if (exe_name.empty())
		return {};

	// Check for bundled tool executable
	if (!bundle_dir.empty() && app::platform() == app::Platform::Windows)
	{
		auto exe_path = app::path(fmt::format("tools/{}/{}", bundle_dir, exe_name), app::Dir::Executable);

		// Append .exe if not present
		if (!strutil::endsWithCI(exe_path, ".exe"))
			exe_path += ".exe";

		// Check if it exists
		if (fileExists(exe_path))
			return exe_path;
	}

	// Get system PATH environment variable
	auto path_env = std::getenv("PATH");
	if (!path_env)
		return {};

	// Remove * suffix or prefix from exe_name
	if (strutil::startsWith(exe_name, '*'))
		exe_name.remove_prefix(1);
	if (strutil::endsWith(exe_name, '*'))
		exe_name.remove_suffix(1);

	// Split PATH into individual paths
	auto path_str = string{ path_env };
	auto paths    = strutil::split(path_str, (app::platform() == app::Platform::Windows) ? ';' : ':');

	// Check each path for the executable
	for (const auto& p : paths)
	{
		auto path = fmt::format("{}{}{}", p, pathSeparator(), exe_name);

		// Append .exe on Windows if not present
		if (app::platform() == app::Platform::Windows && !strutil::endsWithCI(path, ".exe"))
			path += ".exe";

		// Check if file exists and is executable
		if (wxFileName::IsFileExecutable(fromUtf8(path)))
			return path;
	}

	return {};
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
