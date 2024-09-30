
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SFileDialog.cpp
// Description: Various file dialog related functions, to keep things consistent
//              where file open/save dialogs are used, and so that the last used
//              directory is saved correctly
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
#include "SFileDialog.h"
#include "App.h"
#include "StringUtils.h"
#include "UI/WxUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(String, dir_last)


// -----------------------------------------------------------------------------
//
// SFileDialog Namespace Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Shows a dialog to open a single file.
// Returns true and sets [info] if the user clicked ok, false otherwise
// -----------------------------------------------------------------------------
bool filedialog::openFile(
	FDInfo&     info,
	string_view caption,
	string_view extensions,
	wxWindow*   parent,
	string_view fn_default,
	int         ext_default)
{
	// Create file dialog
	wxFileDialog fd(
		parent,
		wxutil::strFromView(caption),
		dir_last,
		wxutil::strFromView(fn_default),
		wxutil::strFromView(extensions),
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	// Select default extension
	fd.SetFilterIndex(ext_default);

	// Run the dialog
	if (fd.ShowModal() == wxID_OK)
	{
		// Set file dialog info
		strutil::Path fn(fd.GetPath().ToStdString());
		info.filenames.push_back(fn.fullPath());
		info.extension = fn.extension();
		info.ext_index = fd.GetFilterIndex();
		info.path      = fn.path(true);

		// Set last dir
		dir_last = info.path;

		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Shows a dialog to open a single file.
// Returns the selected filename if the user clicked ok, or an empty string
// otherwise
// -----------------------------------------------------------------------------
string filedialog::openFile(
	string_view caption,
	string_view extensions,
	wxWindow*   parent,
	string_view fn_default,
	int         ext_default)
{
	// Create file dialog
	wxFileDialog fd(
		parent,
		wxutil::strFromView(caption),
		dir_last,
		wxutil::strFromView(fn_default),
		wxutil::strFromView(extensions),
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	// Select default extension
	fd.SetFilterIndex(ext_default);

	// Run the dialog
	if (fd.ShowModal() == wxID_OK)
	{
		auto filename = fd.GetPath().ToStdString();
		dir_last      = strutil::Path::pathOf(filename);

		return filename;
	}

	return {};
}

// -----------------------------------------------------------------------------
// Shows a dialog to open a single executable file.
// Returns true and sets [info] if the user clicked ok, false otherwise
// -----------------------------------------------------------------------------
bool filedialog::openExecutableFile(FDInfo& info, string_view caption, wxWindow* parent, string_view fn_default)
{
	static auto extensions = app::platform() == app::Platform::Windows ? "Executable files (*.exe)|*.exe;*.bat"
																	   : wxFileSelectorDefaultWildcardStr;

	return openFile(info, caption, extensions, parent, fn_default);
}

// -----------------------------------------------------------------------------
// Shows a dialog to open a single executable file.
// Returns the selected filename if the user clicked ok, or an empty string
// otherwise
// -----------------------------------------------------------------------------
string filedialog::openExecutableFile(string_view caption, wxWindow* parent, string_view fn_default)
{
	static auto extensions = app::platform() == app::Platform::Windows ? "Executable files (*.exe)|*.exe;*.bat"
																	   : wxFileSelectorDefaultWildcardStr;

	return openFile(caption, extensions, parent, fn_default);
}

// -----------------------------------------------------------------------------
// Shows a dialog to open multiple files.
// Returns true and sets [info] if the user clicked ok, false otherwise
// -----------------------------------------------------------------------------
bool filedialog::openFiles(
	FDInfo&     info,
	string_view caption,
	string_view extensions,
	wxWindow*   parent,
	string_view fn_default,
	int         ext_default)
{
	// Create file dialog
	wxFileDialog fd(
		parent,
		wxutil::strFromView(caption),
		dir_last,
		wxutil::strFromView(fn_default),
		wxutil::strFromView(extensions),
		wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);

	// Select default extension
	fd.SetFilterIndex(ext_default);

	// Run the dialog
	if (fd.ShowModal() == wxID_OK)
	{
		wxArrayString paths;
		fd.GetPaths(paths);

		// Set file dialog info
		for (const auto& path : paths)
			info.filenames.emplace_back(path);
		strutil::Path fn(info.filenames[0]);
		info.extension = fn.extension();
		info.ext_index = fd.GetFilterIndex();
		info.path      = fn.path(true);

		// Set last dir
		dir_last = info.path;

		return true;
	}
	else
		return false;
}

// -----------------------------------------------------------------------------
// Shows a dialog to open multiple files.
// Returns an FDInfo struct with information about the selected files.
// If the user cancelled, the FDInfo will contain no filenames
// -----------------------------------------------------------------------------
filedialog::FDInfo filedialog::openFiles(
	string_view caption,
	string_view extensions,
	wxWindow*   parent,
	string_view fn_default,
	int         ext_default)
{
	FDInfo info;
	openFiles(info, caption, extensions, parent, fn_default, ext_default);
	return info;
}

// -----------------------------------------------------------------------------
// Shows a dialog to save a single file.
// Returns true and sets [info] if the user clicked ok, false otherwise
// -----------------------------------------------------------------------------
bool filedialog::saveFile(
	FDInfo&     info,
	string_view caption,
	string_view extensions,
	wxWindow*   parent,
	string_view fn_default,
	int         ext_default)
{
	// Create file dialog
	wxFileDialog fd(
		parent,
		wxutil::strFromView(caption),
		dir_last,
		wxutil::strFromView(fn_default),
		wxutil::strFromView(extensions),
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	// Select default extension
	fd.SetFilterIndex(ext_default);

	// Run the dialog
	if (fd.ShowModal() == wxID_OK)
	{
		// Set file dialog info
		strutil::Path fn(fd.GetPath().ToStdString());
		info.filenames.push_back(fn.fullPath());
		info.extension = fn.extension();
		info.ext_index = fd.GetFilterIndex();
		info.path      = fn.path(true);

		// Set last dir
		dir_last = info.path;

		return true;
	}
	else
		return false;
}

// -----------------------------------------------------------------------------
// Shows a dialog to save a single file.
// Returns the filename to save if the user clicked ok, or an empty string
// otherwise
// -----------------------------------------------------------------------------
string filedialog::saveFile(
	string_view caption,
	string_view extensions,
	wxWindow*   parent,
	string_view fn_default,
	int         ext_default)
{
	// Create file dialog
	wxFileDialog fd(
		parent,
		wxutil::strFromView(caption),
		dir_last,
		wxutil::strFromView(fn_default),
		wxutil::strFromView(extensions),
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	// Select default extension
	fd.SetFilterIndex(ext_default);

	// Run the dialog
	if (fd.ShowModal() == wxID_OK)
	{
		auto filename = fd.GetPath().ToStdString();
		dir_last      = strutil::Path::pathOf(filename);
		return filename;
	}

	return {};
}

// -----------------------------------------------------------------------------
// Shows a dialog to save multiple files.
// Returns true and sets [info] if the user clicked ok, false otherwise.
// This is used to replace wxDirDialog, which sucks
// -----------------------------------------------------------------------------
bool filedialog::saveFiles(FDInfo& info, string_view caption, string_view extensions, wxWindow* parent, int ext_default)
{
	// Create file dialog
	wxFileDialog fd(
		parent,
		wxutil::strFromView(caption),
		dir_last,
		"ignored",
		wxutil::strFromView(extensions),
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	// Select default extension
	fd.SetFilterIndex(ext_default);

	// Run the dialog
	if (fd.ShowModal() == wxID_OK)
	{
		// Set file dialog info
		info.filenames.clear();
		info.extension = fd.GetWildcard().AfterLast('.');
		info.ext_index = fd.GetFilterIndex();
		info.path      = fd.GetDirectory();

		// Set last dir
		dir_last = info.path;

		return true;
	}
	else
		return false;
}

// -----------------------------------------------------------------------------
// Shows a dialog to save multiple files.
// Returns an FDInfo struct with information about the selected files.
// If the user cancelled, the FDInfo will contain no path
// -----------------------------------------------------------------------------
filedialog::FDInfo filedialog::saveFiles(string_view caption, string_view extensions, wxWindow* parent, int ext_default)
{
	FDInfo info;
	saveFiles(info, caption, extensions, parent, ext_default);
	return info;
}

// -----------------------------------------------------------------------------
// Shows a dialog to open a directory.
// Returns the path of the selected directory or an empty string if cancelled
// -----------------------------------------------------------------------------
string filedialog::openDirectory(string_view caption, wxWindow* parent)
{
	// Open a directory browser dialog
	wxDirDialog dialog_open(parent, wxutil::strFromView(caption), dir_last, wxDD_DIR_MUST_EXIST | wxDD_NEW_DIR_BUTTON);

	// Run the dialog
	if (dialog_open.ShowModal() == wxID_OK)
	{
		// Set last dir
		dir_last = wxutil::strToView(dialog_open.GetPath());

		return dialog_open.GetPath().ToStdString();
	}

	return {};
}

// -----------------------------------------------------------------------------
// Returns the executable file filter string depending on the current OS
// -----------------------------------------------------------------------------
string filedialog::executableExtensionString()
{
	if (app::platform() == app::Platform::Windows)
		return "Executable Files (*.exe)|*.exe";
	else
		return "Executable Files|*.*";
}

// -----------------------------------------------------------------------------
// Returns [exe_name] with a .exe extension if in Windows
// -----------------------------------------------------------------------------
string filedialog::executableFileName(string_view exe_name)
{
	if (app::platform() == app::Platform::Windows)
		return string{ exe_name } + ".exe";
	else
		return string{ exe_name };
}
