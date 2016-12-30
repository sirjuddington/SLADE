
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    Icons.cpp
 * Description: Functions to do with loading program icons from
 *              slade.pk3
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
#include "Icons.h"
#include "Archive/ArchiveManager.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(String, iconset_general, "Default", CVAR_SAVE)
CVAR(String, iconset_entry_list, "Default", CVAR_SAVE)

namespace Icons
{
	struct icon_t
	{
		wxImage			image;
		wxImage			image_large;
		string			name;
		ArchiveEntry*	resource_entry;
	};

	vector<icon_t>	icons_general;
	vector<icon_t>	icons_text_editor;
	vector<icon_t>	icons_entry;
	wxBitmap icon_empty;
	vector<string>	iconsets_entry;
	vector<string>	iconsets_general;
}


/*******************************************************************
 * ICONS NAMESPACE FUNCTIONS
 *******************************************************************/
namespace Icons
{
	vector<icon_t>& iconList(int type)
	{
		if (type == ENTRY)
			return icons_entry;
		else if (type == TEXT_EDITOR)
			return icons_text_editor;
		else
			return icons_general;
	}

	bool loadIconsDir(int type, ArchiveTreeNode* dir)
	{
		if (!dir)
			return false;

		// Check for icon set dirs
		for (unsigned a = 0; a < dir->nChildren(); a++)
		{
			if (dir->getChild(a)->getName() != "large")
			{
				if (type == GENERAL)
					iconsets_general.push_back(dir->getChild(a)->getName());
				else if (type == ENTRY)
					iconsets_entry.push_back(dir->getChild(a)->getName());
			}
		}

		// Get icon set dir
		string icon_set_dir = "Default";
		if (type == ENTRY) icon_set_dir = iconset_entry_list;
		if (type == GENERAL) icon_set_dir = iconset_general;
		if (icon_set_dir != "Default" && dir->getChild(icon_set_dir))
			dir = (ArchiveTreeNode*)dir->getChild(icon_set_dir);

		vector<icon_t>& icons = iconList(type);
		string tempfile = appPath("sladetemp", DIR_TEMP);

		// Go through each entry in the directory
		for (size_t a = 0; a < dir->numEntries(false); a++)
		{
			ArchiveEntry* entry = dir->getEntry(a);

			// Ignore anything not png format
			if (!entry->getName().EndsWith("png"))
				continue;

			// Export entry data to a temporary file
			entry->exportFile(tempfile);

			// Create / setup icon
			icon_t n_icon;
			n_icon.image.LoadFile(tempfile);	// Load image from temp file
			n_icon.name = entry->getName(true);	// Set icon name
			n_icon.resource_entry = entry;

			// Add the icon
			icons.push_back(n_icon);

			// Delete the temporary file
			wxRemoveFile(tempfile);
		}

		// Go through large icons
		ArchiveTreeNode* dir_large = (ArchiveTreeNode*)dir->getChild("large");
		if (dir_large)
		{
			for (size_t a = 0; a < dir_large->numEntries(false); a++)
			{
				ArchiveEntry* entry = dir_large->getEntry(a);

				// Ignore anything not png format
				if (!entry->getName().EndsWith("png"))
					continue;

				// Export entry data to a temporary file
				entry->exportFile(tempfile);

				// Create / setup icon
				bool found = false;
				string name = entry->getName(true);
				for (unsigned i = 0; i < icons.size(); i++)
				{
					if (icons[i].name == name)
					{
						icons[i].image_large.LoadFile(tempfile);
						found = true;
						break;
					}
				}

				if (!found)
				{
					icon_t n_icon;
					n_icon.image_large.LoadFile(tempfile);	// Load image from temp file
					n_icon.name = entry->getName(true);		// Set icon name
					n_icon.resource_entry = entry;

					// Add the icon
					icons.push_back(n_icon);
				}

				// Delete the temporary file
				wxRemoveFile(tempfile);
			}
		}

		return true;
	}
}

/* Icons::loadIcons
 * Loads all icons from slade.pk3 (in the icons/ dir)
 *******************************************************************/
bool Icons::loadIcons()
{
	string tempfile = appPath("sladetemp", DIR_TEMP);

	// Get slade.pk3
	Archive* res_archive = theArchiveManager->programResourceArchive();

	// Do nothing if it doesn't exist
	if (!res_archive)
		return false;

	// Get the icons directory of the archive
	ArchiveTreeNode* dir_icons = res_archive->getDir("icons/");

	// Load general icons
	iconsets_general.push_back("Default");
	loadIconsDir(GENERAL, (ArchiveTreeNode*)dir_icons->getChild("general"));

	// Load entry list icons
	iconsets_entry.push_back("Default");
	loadIconsDir(ENTRY, (ArchiveTreeNode*)dir_icons->getChild("entry_list"));

	// Load text editor icons
	loadIconsDir(TEXT_EDITOR, (ArchiveTreeNode*)dir_icons->getChild("text_editor"));

	return true;
}

/* Icons::getIcon
 * Returns the icon matching [name] of [type] as a wxBitmap (for
 * toolbars etc), or an empty bitmap if no icon was found. If [type]
 * is less than 0, try all icon types. If [log_missing] is true, log
 * an error message if the icon was not found
 *******************************************************************/
wxBitmap Icons::getIcon(int type, string name, bool large, bool log_missing)
{
	// Check all types if [type] is < 0
	if (type < 0)
	{
		wxBitmap icon = getIcon(GENERAL, name, large, false);
		if (!icon.IsOk())
			icon = getIcon(ENTRY, name, large, false);
		if (!icon.IsOk())
			icon = getIcon(TEXT_EDITOR, name, large, false);

		if (!icon.IsOk() && log_missing)
			LOG_MESSAGE(2, "Icon \"%s\" does not exist", name);

		return icon;
	}

	vector<icon_t>& icons = iconList(type);

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
		LOG_MESSAGE(2, "Icon \"%s\" does not exist", name);

	return wxNullBitmap;
}

/* Icons::exportIconPNG
 * Exports icon [name] of [type] to a png image file at [path]
 *******************************************************************/
bool Icons::exportIconPNG(int type, string name, string path)
{
	vector<icon_t>& icons = iconList(type);

	for (size_t a = 0; a < icons.size(); a++)
	{
		if (icons[a].name.Cmp(name) == 0)
			return icons[a].resource_entry->exportFile(path);
	}

	return false;
}

/* Icons::getIconSets
 * Returns a list of currently available icon sets for [type]
 *******************************************************************/
vector<string> Icons::getIconSets(int type)
{
	if (type == GENERAL)
		return iconsets_general;
	else if (type == ENTRY)
		return iconsets_entry;

	return vector<string>();
}
