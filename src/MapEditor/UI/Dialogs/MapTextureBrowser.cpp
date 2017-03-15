
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
#include "MapTextureBrowser.h"
#include "General/ResourceManager.h"
#include "Graphics/CTexture/CTexture.h"
#include "MapEditor/GameConfiguration/GameConfiguration.h"
#include "MapEditor/MapEditorWindow.h"
#include "MapEditor/SLADEMap/SLADEMap.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Int, map_tex_sort, 2, CVAR_SAVE)
CVAR(String, map_tex_treespec, "type,archive,category", CVAR_SAVE)


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
	this->truncate_names = true;

	// Init sorting
	addSortType("Usage Count");
	setSortType(map_tex_sort);

	// Set window title
	SetTitle("Browse Map Textures");

	// Textures
	if (type == 0 || theGameConfiguration->mixTexFlats())
	{
		addGlobalItem(new MapTexBrowserItem("-", 0, 0));

		vector<map_texinfo_t>& textures = theMapEditor->textureManager().getAllTexturesInfo();
		for (unsigned a = 0; a < textures.size(); a++)
		{
			// Add browser item
			addItem(new MapTexBrowserItem(textures[a].name, 0, textures[a].index),
				determineTexturePath(textures[a].archive, textures[a].category, "Textures", textures[a].path));
		}
	}

	// Flats
	if (type == 1 || theGameConfiguration->mixTexFlats())
	{
		vector<map_texinfo_t>& flats = theMapEditor->textureManager().getAllFlatsInfo();
		for (unsigned a = 0; a < flats.size(); a++)
		{
			// Determine tree path
			string path = determineTexturePath(flats[a].archive, flats[a].category, "Flats", flats[a].path);

			// Add browser item
			if (flats[a].category == MapTextureManager::TC_TEXTURES)
				addItem(new MapTexBrowserItem(flats[a].name, 0, flats[a].index), path);
			else
				addItem(new MapTexBrowserItem(flats[a].name, 1, flats[a].index), path);
		}
	}

	populateItemTree(false);

	// Select initial texture (if any)
	selectItem(texture);
}

/* MapTextureBrowser::~MapTextureBrowser
 * MapTextureBrowser class destructor
 *******************************************************************/
MapTextureBrowser::~MapTextureBrowser()
{
}

/* MapTextureBrowser::determineTexturePath
 * Builds and returns the tree item path for [info]
 *******************************************************************/
string MapTextureBrowser::determineTexturePath(Archive* archive, uint8_t category, string type, string path)
{
	wxArrayString tree_spec = wxSplit(map_tex_treespec, ',');
	string ret;
	for (unsigned b = 0; b < tree_spec.size(); b++)
	{
		if (tree_spec[b] == "archive")
			ret += archive->getFilename(false);
		else if (tree_spec[b] == "type")
			ret += type;
		else if (tree_spec[b] == "category")
		{
			switch (category)
			{
			case MapTextureManager::TC_TEXTUREX:
				ret += "TEXTUREx"; break;
			case MapTextureManager::TC_TEXTURES:
				ret += "TEXTURES"; break;
			case MapTextureManager::TC_HIRES:
				ret += "HIRESTEX"; break;
			case MapTextureManager::TC_TX:
				ret += "Single (TX)"; break;
			default:
				continue;
			}
		}

		ret += "/";
	}

	return ret + path;
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
	if (!map)
		return;

	vector<BrowserItem*>& items = canvas->itemList();
	for (unsigned i = 0; i < items.size(); i++)
	{
		MapTexBrowserItem* item = (MapTexBrowserItem*)items[i];
		if (type == 0)
			item->setUsage(map->texUsageCount(item->getName()));
		else
			item->setUsage(map->flatUsageCount(item->getName()));
	}
}
