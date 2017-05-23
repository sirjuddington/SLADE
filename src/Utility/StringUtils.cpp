
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    StringUtils.cpp
// Description: Various string utility functions
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
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#include "StringUtils.h"
#include "Tokenizer.h"
#include "App.h"
#include "Archive/ArchiveManager.h"


// ----------------------------------------------------------------------------
//
// StringUtils Namespace Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// StringUtils::escapedString
//
// Returns a copy of [str] with escaped double quotes and backslashes.
// If [swap_backslash] is true, instead of escaping it will swap backslashes to
// forward slashes
// ----------------------------------------------------------------------------
string StringUtils::escapedString(const string& str, bool swap_backslash)
{
	string escaped = str;

	escaped.Replace(SLASH_BACK, swap_backslash ? SLASH_FORWARD : ESCAPED_SLASH_BACK);
	escaped.Replace(QUOTE_DOUBLE, ESCAPED_QUOTE_DOUBLE);

	return escaped;
}

// ----------------------------------------------------------------------------
// StringUtils::buildConfig
//
// Reads the text file at [filename], processing any #include statements in the
// file recursively. The resulting 'expanded' text is written to [out]
// ----------------------------------------------------------------------------
void StringUtils::processIncludes(string filename, string& out)
{
	// Open file
	wxTextFile file;
	if (!file.Open(filename))
		return;

	// Get file path
	wxFileName fn(filename);
	string path = fn.GetPath(true);

	// Go through line-by-line
	string line = file.GetNextLine();
	while (!file.Eof())
	{
		// Check for #include
		if (line.Lower().Trim().StartsWith("#include"))
		{
			// Get filename to include
			Tokenizer tz;
			tz.openString(line);
			tz.getToken();	// Skip #include
			string fn = tz.getToken();

			// Process the file
			processIncludes(path + fn, out);
		}
		else
			out.Append(line + "\n");

		line = file.GetNextLine();
	}
}

// ----------------------------------------------------------------------------
// StringUtils::processIncludes
//
// Reads the text entry [entry], processing any #include statements in the
// entry text recursively. This will search in the resource folder and archive
// as well as in the parent archive. The resulting 'expanded' text is written 
// to [out]
// ----------------------------------------------------------------------------
void StringUtils::processIncludes(ArchiveEntry* entry, string& out, bool use_res)
{
	// Check entry was given
	if (!entry)
		return;

	// Write entry to temp file
	string filename = App::path(entry->getName(), App::Dir::Temp);
	entry->exportFile(filename);

	// Open file
	wxTextFile file;
	if (!file.Open(filename))
		return;

	// Go through line-by-line
	string line = file.GetFirstLine();
	while (!file.Eof())
	{
		// Check for #include
		if (line.Lower().Trim().StartsWith("#include"))
		{
			// Get name of entry to include
			Tokenizer tz;
			tz.openString(line);
			tz.getToken();	// Skip #include
			tz.setSpecialCharacters("");
			string inc_name = tz.getToken();
			string name = entry->getPath() + inc_name;

			// Get the entry
			bool done = false;
			ArchiveEntry* entry_inc = entry->getParent()->entryAtPath(name);
			// DECORATE paths start from the root, not from the #including entry's directory
			if (!entry_inc)
				entry_inc = entry->getParent()->entryAtPath(inc_name);
			if (entry_inc)
			{
				processIncludes(entry_inc, out);
				done = true;
			}
			else
				Log::info(2, S_FMT("Couldn't find entry to #include: %s", name));

			// Look in resource pack
			if (use_res && !done && theArchiveManager->programResourceArchive())
			{
				name = "config/games/" + inc_name;
				entry_inc = theArchiveManager->programResourceArchive()->entryAtPath(name);
				if (entry_inc)
				{
					processIncludes(entry_inc, out);
					done = true;
				}
			}

			// Okay, we've exhausted all possibilities
			if (!done)
				Log::info(
					1,
					S_FMT("Error: Attempting to #include nonexistant entry \"%s\" from entry %s",
						name,
						entry->getName())
				);
		}
		else
			out.Append(line + "\n");

		line = file.GetNextLine();
	}

	// Delete temp file
	wxRemoveFile(filename);
}
