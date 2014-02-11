
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MapTextureBrowser.cpp
 * Description: Specialisation of BrowserWindow to show and browse
 *              for map textures/flats
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
#include "MapTextureBrowser.h"
#include "MapEditorWindow.h"
#include "ResourceManager.h"
#include "CTexture.h"
#include "GameConfiguration.h"
#include "SLADEMap.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Int, map_tex_sort, 2, CVAR_SAVE)


/*******************************************************************
 * MAPTEXBROWSERITEM CLASS FUNCTIONS
 *******************************************************************/

/* MapTexBrowserItem::MapTexBrowserItem
 * MapTexBrowserItem class constructor
 *******************************************************************/
MapTexBrowserItem::MapTexBrowserItem(string name, int type, unsigned index) : BrowserItem(name, index)
{
	if (type == 0)
		this->type = "texture";
	else if (type == 1)
		this->type = "flat";

	// Check for blank texture
	if (name == "-" && type == 0)
		blank = true;

	usage_count = 0;
}

/* MapTexBrowserItem::~MapTexBrowserItem
 * MapTexBrowserItem class destructor
 *******************************************************************/
MapTexBrowserItem::~MapTexBrowserItem()
{
}

/* MapTexBrowserItem::loadImage
 * Loads the item image
 *******************************************************************/
bool MapTexBrowserItem::loadImage()
{
	GLTexture* tex = NULL;

	// Get texture or flat depending on type
	if (type == "texture")
		tex = theMapEditor->textureManager().getTexture(name, false);
	else if (type == "flat")
		tex = theMapEditor->textureManager().getFlat(name, false);

	if (tex)
	{
		image = tex;
		return true;
	}
	else
		return false;
}

/* MapTexBrowserItem::itemInfo
 * Returns a string with extra information about the texture/flat
 *******************************************************************/
string MapTexBrowserItem::itemInfo()
{
	string info;

	// Check for blank texture
	if (name == "-")
		return "No Texture";

	// Add dimensions if known
	if (image || loadImage())
		info += S_FMT("%dx%d", image->getWidth(), image->getHeight());
	else
		info += "Unknown size";

	// Add type
	if (type == "texture")
		info += ", Texture";
	else
		info += ", Flat";

	// Add scaling info
	if (image->getScaleX() != 1.0 || image->getScaleY() != 1.0)
		info += ", Scaled";

	// Add usage count
	info += S_FMT(", Used %d times", usage_count);

	return info;
}


/*******************************************************************
 * MAPTEXTUREBROWSER CLASS FUNCTIONS
 *******************************************************************/

/* MapTextureBrowser::MapTextureBrowser
 * MapTextureBrowser class constructor
 *******************************************************************/
MapTextureBrowser::MapTextureBrowser(wxWindow* parent, int type, string texture, SLADEMap* map) : BrowserWindow(parent)
{
	// Init variables
	this->type = type;
	this->map = map;

	// Init sorting
	addSortType("Usage Count");
	setSortType(map_tex_sort);

	// Set window title
	SetTitle("Browse Map Textures");

	// Textures
	if (type == 0 || theGameConfiguration->mixTexFlats())
	{
		// No texture '-' (don't show if browsing for flats)
		if (type == 0)
			addItem(new MapTexBrowserItem("-", 0, 0), "Textures");

		// Composite textures
		vector<TextureResource::tex_res_t> textures;
		theResourceManager->getAllTextures(textures, NULL);
		for (unsigned a = 0; a < textures.size(); a++)
		{
			CTexture * tex = textures[a].tex;
			string parent = textures[a].parent->getFilename(false);
			if (tex->isExtended())
			{
				if (S_CMPNOCASE(tex->getType(), "texture") || S_CMPNOCASE(tex->getType(), "walltexture"))
					addItem(new MapTexBrowserItem(tex->getName(), 0, tex->getIndex()+1), S_FMT("Textures/TEXTURES/%s", parent));
				else if (S_CMPNOCASE(tex->getType(), "define"))
					addItem(new MapTexBrowserItem(tex->getName(), 0, tex->getIndex()+1), S_FMT("Textures/HIRESTEX/%s", parent));
				else if (S_CMPNOCASE(tex->getType(), "flat"))
					addItem(new MapTexBrowserItem(tex->getName(), 0, tex->getIndex()+1), S_FMT("Flats/TEXTURES/%s", parent));
				// Ignore graphics, patches and sprites
			}
			else
				addItem(new MapTexBrowserItem(tex->getName(), 0, tex->getIndex()+1), S_FMT("Textures/TEXTUREx/%s", parent));
		}

		// Texture namespace patches (TX_)
		if (theGameConfiguration->txTextures())
		{
			vector<ArchiveEntry*> patches;
			theResourceManager->getAllPatchEntries(patches, NULL);
			for (unsigned a = 0; a < patches.size(); a++)
			{
				if (patches[a]->isInNamespace("textures") || patches[a]->isInNamespace("hires"))
				{
					// Determine texture path if it's in a pk3
					string path = patches[a]->getPath();
					if (path.StartsWith("/textures/"))
						path.Remove(0, 9);
					else if (path.StartsWith("/hires/"))
						path.Remove(0, 6);
					else
						path = "";

					path = patches[a]->getParent()->getFilename(false) + path;

					addItem(new MapTexBrowserItem(patches[a]->getName(true), 0, a), "Textures/Single File (TX)/" + path);
				}
			}
		}
	}

	// Flats
	if (type == 1 || theGameConfiguration->mixTexFlats())
	{
		vector<ArchiveEntry*> flats;
		theResourceManager->getAllFlatEntries(flats, NULL);
		for (unsigned a = 0; a < flats.size(); a++)
		{
			ArchiveEntry* entry = flats[a];

			// Determine flat path if it's in a pk3
			string path = entry->getPath();
			if (path.StartsWith("/flats/") || path.StartsWith("/hires/"))
				path.Remove(0, 6);
			else
				path = "";

			path = flats[a]->getParent()->getFilename(false) + path;

			addItem(new MapTexBrowserItem(entry->getName(true), 1, entry->getParentDir()->entryIndex(entry)), "Flats/" + path);
		}
	}

	populateItemTree();

	// Select initial texture (if any)
	if (!texture.IsEmpty())
		selectItem(texture);
}

/* MapTextureBrowser::~MapTextureBrowser
 * MapTextureBrowser class destructor
 *******************************************************************/
MapTextureBrowser::~MapTextureBrowser()
{
}

/* sortBIUsage
 * Returns true if [left] has higher usage count than [right]. If
 * both are equal it will go alphabetically by name
 *******************************************************************/
bool sortBIUsage(BrowserItem* left, BrowserItem* right)
{
	// Sort alphabetically if usage counts are equal
	if (((MapTexBrowserItem*)left)->usageCount() == ((MapTexBrowserItem*)right)->usageCount())
		return left->getName() < right->getName();
	else
		return ((MapTexBrowserItem*)left)->usageCount() > ((MapTexBrowserItem*)right)->usageCount();
}

/* MapTextureBrowser::doSort
 * Sort the current items depending on [sort_type]
 *******************************************************************/
void MapTextureBrowser::doSort(unsigned sort_type)
{
	map_tex_sort = sort_type;

	// Default sorts
	if (sort_type < 2)
		return BrowserWindow::doSort(sort_type);

	// Sort by usage
	else if (sort_type == 2)
	{
		updateUsage();
		vector<BrowserItem*>& items = canvas->itemList();
		std::sort(items.begin(), items.end(), sortBIUsage);
	}
}

/* MapTextureBrowser::updateUsage
 * Updates usage counts for all browser items
 *******************************************************************/
void MapTextureBrowser::updateUsage()
{
	vector<BrowserItem*>& items = canvas->itemList();
	for (unsigned i = 0; i < items.size(); i++)
	{
		MapTexBrowserItem* item = (MapTexBrowserItem*)items[i];
		if (type == 0)
			item->setUsage(map->texUsageCount(item->getName()));
		else
			item->setUsage(map->flatUsageCount(item->getName()));
	}

	if (!map)
		return;
}
