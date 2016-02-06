
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    SFileDialog.cpp
 * Description: Various file dialog related functions, to keep things
 *              consistent where file open/save dialogs are used,
 *              and so that the last used directory is saved correctly
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "UI/WxStuff.h"
#include "SFileDialog.h"
#include <wx/filedlg.h>
#include <wx/filename.h>


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(String, dir_last)


/*******************************************************************
 * SFILEDIALOG NAMESPACE FUNCTIONS
 *******************************************************************/

/* SFileDialog::openFile
 * Shows a dialog to open a single file. Returns true and sets [info]
 * if the user clicked ok, false otherwise
 *******************************************************************/
bool SFileDialog::openFile(fd_info_t& info, string caption, string extensions, wxWindow* parent, string fn_default, int ext_default)
{
	// Create file dialog
	wxFileDialog fd(parent, caption, dir_last, fn_default, extensions, wxFD_OPEN|wxFD_FILE_MUST_EXIST);

	// Select default extension
	fd.SetFilterIndex(ext_default);

	// Run the dialog
	if (fd.ShowModal() == wxID_OK)
	{
		// Set file dialog info
		wxFileName fn(fd.GetPath());
		info.filenames.Add(fn.GetFullPath());
		info.extension = fn.GetExt();
		info.ext_index = fd.GetFilterIndex();
		info.path = fn.GetPath(true);

		// Set last dir
		dir_last = info.path;

		return true;
	}
	else
		return false;
}

/* SFileDialog::openFiles
 * Shows a dialog to open multiple files. Returns true and sets
 * [info] if the user clicked ok, false otherwise
 *******************************************************************/
bool SFileDialog::openFiles(fd_info_t& info, string caption, string extensions, wxWindow* parent, string fn_default, int ext_default)
{
	// Create file dialog
	wxFileDialog fd(parent, caption, dir_last, fn_default, extensions, wxFD_OPEN|wxFD_FILE_MUST_EXIST|wxFD_MULTIPLE);

	// Select default extension
	fd.SetFilterIndex(ext_default);

	// Run the dialog
	if (fd.ShowModal() == wxID_OK)
	{
		// Set file dialog info
		fd.GetPaths(info.filenames);
		wxFileName fn(info.filenames[0]);
		info.extension = fn.GetExt();
		info.ext_index = fd.GetFilterIndex();
		info.path = fn.GetPath(true);

		// Set last dir
		dir_last = info.path;

		return true;
	}
	else
		return false;
}

/* SFileDialog::saveFile
 * Shows a dialog to save a single file. Returns true and sets [info]
 * if the user clicked ok, false otherwise
 *******************************************************************/
bool SFileDialog::saveFile(fd_info_t& info, string caption, string extensions, wxWindow* parent, string fn_default, int ext_default)
{
	// Create file dialog
	wxFileDialog fd(parent, caption, dir_last, fn_default, extensions, wxFD_SAVE|wxFD_OVERWRITE_PROMPT);

	// Select default extension
	fd.SetFilterIndex(ext_default);

	// Run the dialog
	if (fd.ShowModal() == wxID_OK)
	{
		// Set file dialog info
		wxFileName fn(fd.GetPath());
		info.filenames.Add(fn.GetFullPath());
		info.extension = fn.GetExt();
		info.ext_index = fd.GetFilterIndex();
		info.path = fn.GetPath(true);

		// Set last dir
		dir_last = info.path;

		return true;
	}
	else
		return false;
}

/* SFileDialog::saveFiles
 * Shows a dialog to save multiple files. Returns true and sets
 * [info] if the user clicked ok, false otherwise. This is used to
 * replace wxDirDialog, which sucks
 *******************************************************************/
bool SFileDialog::saveFiles(fd_info_t& info, string caption, string extensions, wxWindow* parent, int ext_default)
{
	// Create file dialog
	wxFileDialog fd(parent, caption, dir_last, "ignored", extensions, wxFD_SAVE|wxFD_OVERWRITE_PROMPT);

	// Select default extension
	fd.SetFilterIndex(ext_default);

	// Run the dialog
	if (fd.ShowModal() == wxID_OK)
	{
		// Set file dialog info
		info.filenames.Clear();
		info.extension = fd.GetWildcard().AfterLast('.');
		info.ext_index = fd.GetFilterIndex();
		info.path = fd.GetDirectory();

		// Set last dir
		dir_last = info.path;

		return true;
	}
	else
		return false;
}
