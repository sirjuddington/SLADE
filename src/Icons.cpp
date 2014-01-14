
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
#include "WxStuff.h"
#include "Icons.h"
#include "ArchiveManager.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
struct icon_t
{
	wxImage	image;
	wxImage image_large;
	string	name;
};
vector<icon_t>	icons;
wxBitmap icon_empty;


/*******************************************************************
 * FUNCTIONS
 *******************************************************************/

/* loadIcons
 * Loads all icons from slade.pk3 (in the icons/ dir)
 *******************************************************************/
bool loadIcons()
{
	string tempfile = appPath("sladetemp", DIR_TEMP);

	// Get slade.pk3
	Archive* res_archive = theArchiveManager->programResourceArchive();

	// Do nothing if it doesn't exist
	if (!res_archive)
		return false;

	// Get the icons directory of the archive
	ArchiveTreeNode* dir_icons = res_archive->getDir("icons/");

	// Go through each entry in the directory
	for (size_t a = 0; a < dir_icons->numEntries(false); a++)
	{
		ArchiveEntry* entry = dir_icons->getEntry(a);

		// Export entry data to a temporary file
		entry->exportFile(tempfile);

		// Create / setup icon
		icon_t n_icon;
		n_icon.image.LoadFile(tempfile);	// Load image from temp file
		n_icon.name = entry->getName(true);	// Set icon name

		// Add the icon
		icons.push_back(n_icon);

		// Delete the temporary file
		wxRemoveFile(tempfile);
	}

	// Go through large icons
	ArchiveTreeNode* dir_icons_large = res_archive->getDir("icons/large/");
	if (dir_icons_large)
	{
		for (size_t a = 0; a < dir_icons_large->numEntries(false); a++)
		{
			ArchiveEntry* entry = dir_icons_large->getEntry(a);

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
				n_icon.name = entry->getName(true);	// Set icon name

				// Add the icon
				icons.push_back(n_icon);
			}

			// Delete the temporary file
			wxRemoveFile(tempfile);
		}
	}

	return true;
}

/* getIcon
 * Returns the icon matching <name> as a wxBitmap (for toolbars etc),
 * or an empty bitmap if no icon matching <name> was found
 *******************************************************************/
wxBitmap getIcon(string name, bool large)
{
	for (size_t a = 0; a < icons.size(); a++)
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

	wxLogMessage("Icon \"%s\" does not exist", name.c_str());
	return wxNullBitmap;
}
