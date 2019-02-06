
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         https://slade.mancubus.net
// Filename:    Icons.cpp
// Description: Functions to do with loading program icons from slade.pk3
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
#include "Icons.h"
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "General/UI.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(String, iconset_general, "Default", CVar::Flag::Save)
CVAR(String, iconset_entry_list, "Default", CVar::Flag::Save)

namespace Icons
{
struct Icon
{
	wxImage       image;
	wxImage       image_large;
	wxString      name;
	ArchiveEntry* resource_entry;
};

vector<Icon>     icons_general;
vector<Icon>     icons_text_editor;
vector<Icon>     icons_entry;
wxBitmap         icon_empty;
vector<wxString> iconsets_entry;
vector<wxString> iconsets_general;
} // namespace Icons


// -----------------------------------------------------------------------------
//
// Icons Namespace Functions
//
// -----------------------------------------------------------------------------
namespace Icons
{
// -----------------------------------------------------------------------------
// Returns a list of all icons of [type]
// -----------------------------------------------------------------------------
vector<Icon>& iconList(Type type)
{
	if (type == Entry)
		return icons_entry;
	else if (type == TextEditor)
		return icons_text_editor;
	else
		return icons_general;
}

// -----------------------------------------------------------------------------
// Loads all icons in [dir] to the list for [type]
// -----------------------------------------------------------------------------
bool loadIconsDir(Type type, ArchiveTreeNode* dir)
{
	if (!dir)
		return false;

	// Check for icon set dirs
	for (unsigned a = 0; a < dir->nChildren(); a++)
	{
		if (dir->child(a)->name() != "large")
		{
			if (type == General)
				iconsets_general.push_back(dir->child(a)->name());
			else if (type == Entry)
				iconsets_entry.push_back(dir->child(a)->name());
		}
	}

	// Get icon set dir
	wxString icon_set_dir = "Default";
	if (type == Entry)
		icon_set_dir = iconset_entry_list;
	if (type == General)
		icon_set_dir = iconset_general;
	if (icon_set_dir != "Default" && dir->child(icon_set_dir))
		dir = (ArchiveTreeNode*)dir->child(icon_set_dir);

	auto&    icons    = iconList(type);
	wxString tempfile = App::path("sladetemp", App::Dir::Temp);

	// Go through each entry in the directory
	for (size_t a = 0; a < dir->numEntries(false); a++)
	{
		auto entry = dir->entryAt(a);

		// Ignore anything not png format
		if (!entry->name().EndsWith("png"))
			continue;

		// Export entry data to a temporary file
		entry->exportFile(tempfile);

		// Create / setup icon
		Icon n_icon;
		n_icon.image.LoadFile(tempfile);           // Load image from temp file
		n_icon.name           = entry->name(true); // Set icon name
		n_icon.resource_entry = entry;

		// Add the icon
		icons.push_back(n_icon);

		// Delete the temporary file
		wxRemoveFile(tempfile);
	}

	// Go through large icons
	auto dir_large = (ArchiveTreeNode*)dir->child("large");
	if (dir_large)
	{
		for (size_t a = 0; a < dir_large->numEntries(false); a++)
		{
			auto entry = dir_large->entryAt(a);

			// Ignore anything not png format
			if (!entry->name().EndsWith("png"))
				continue;

			// Export entry data to a temporary file
			entry->exportFile(tempfile);

			// Create / setup icon
			bool     found = false;
			wxString name  = entry->name(true);
			for (auto& icon : icons)
			{
				if (icon.name == name)
				{
					icon.image_large.LoadFile(tempfile);
					found = true;
					break;
				}
			}

			if (!found)
			{
				Icon n_icon;
				n_icon.image_large.LoadFile(tempfile);     // Load image from temp file
				n_icon.name           = entry->name(true); // Set icon name
				n_icon.resource_entry = entry;

				// Add the icon
				icons.push_back(n_icon);
			}

			// Delete the temporary file
			wxRemoveFile(tempfile);
		}
	}

	// Generate any missing large icons
	for (auto& icon : icons)
	{
		if (!icon.image_large.IsOk())
		{
			icon.image_large = icon.image.Copy();
			icon.image_large.Rescale(32, 32, wxIMAGE_QUALITY_BICUBIC);
		}
	}

	return true;
}
} // namespace Icons

// -----------------------------------------------------------------------------
// Loads all icons from slade.pk3 (in the icons/ dir)
// -----------------------------------------------------------------------------
bool Icons::loadIcons()
{
	wxString tempfile = App::path("sladetemp", App::Dir::Temp);

	// Get slade.pk3
	auto res_archive = App::archiveManager().programResourceArchive();

	// Do nothing if it doesn't exist
	if (!res_archive)
		return false;

	// Get the icons directory of the archive
	auto dir_icons = res_archive->dir("icons/");

	// Load general icons
	iconsets_general.emplace_back("Default");
	loadIconsDir(General, (ArchiveTreeNode*)dir_icons->child("general"));

	// Load entry list icons
	iconsets_entry.emplace_back("Default");
	loadIconsDir(Entry, (ArchiveTreeNode*)dir_icons->child("entry_list"));

	// Load text editor icons
	loadIconsDir(TextEditor, (ArchiveTreeNode*)dir_icons->child("text_editor"));

	return true;
}

// -----------------------------------------------------------------------------
// Returns the icon matching [name] of [type] as a wxBitmap (for toolbars etc),
// or an empty bitmap if no icon was found.
// If [type] is less than 0, try all icon types.
// If [log_missing] is true, log an error message if the icon was not found
// -----------------------------------------------------------------------------
wxBitmap Icons::getIcon(Type type, const wxString& name, bool large, bool log_missing)
{
	// Check all types if [type] is < 0
	if (type == Any)
	{
		auto icon = getIcon(General, name, large, false);
		if (!icon.IsOk())
			icon = getIcon(Entry, name, large, false);
		if (!icon.IsOk())
			icon = getIcon(TextEditor, name, large, false);

		if (!icon.IsOk() && log_missing)
			Log::warning(2, S_FMT("Icon \"%s\" does not exist", name));

		return icon;
	}

	auto& icons = iconList(type);

	size_t icons_size = icons.size();
	for (size_t a = 0; a < icons_size; a++)
	{
		if (icons[a].name.Cmp(name) == 0)
		{
			if (large)
			{
				if (icons[a].image_large.IsOk())
					return wxBitmap(icons[a].image_large);
				else
					return wxBitmap(icons[a].image);
			}
			else
				return wxBitmap(icons[a].image);
		}
	}

	if (log_missing)
		Log::warning(2, S_FMT("Icon \"%s\" does not exist", name));

	return wxNullBitmap;
}

// -----------------------------------------------------------------------------
// Returns the icon [name] of [type]
// -----------------------------------------------------------------------------
wxBitmap Icons::getIcon(Type type, const wxString& name)
{
	return getIcon(type, name, UI::scaleFactor() > 1.25);
}

// -----------------------------------------------------------------------------
// Exports icon [name] of [type] to a png image file at [path]
// -----------------------------------------------------------------------------
bool Icons::exportIconPNG(Type type, const wxString& name, const wxString& path)
{
	auto& icons = iconList(type);

	for (auto& icon : icons)
	{
		if (icon.name.Cmp(name) == 0)
			return icon.resource_entry->exportFile(path);
	}

	return false;
}

// -----------------------------------------------------------------------------
// Returns a list of currently available icon sets for [type]
// -----------------------------------------------------------------------------
vector<wxString> Icons::iconSets(Type type)
{
	if (type == General)
		return iconsets_general;
	else if (type == Entry)
		return iconsets_entry;

	return {};
}
