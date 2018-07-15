
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
#include "Game/Configuration.h"
#include "General/ResourceManager.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "MapEditor/SLADEMap/SLADEMap.h"
#include "MapTextureBrowser.h"


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
		this->type_ = "texture";
	else if (type == 1)
		this->type_ = "flat";

	// Check for blank texture
	if (name == "-" && type == 0)
		blank_ = true;

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
	GLTexture* tex = nullptr;

	// Get texture or flat depending on type
	if (type_ == "texture")
		tex = MapEditor::textureManager().getTexture(name_, false);
	else if (type_ == "flat")
		tex = MapEditor::textureManager().getFlat(name_, false);

	if (tex)
	{
		image_ = tex;
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
	if (name_ == "-")
		return "No Texture";

	// Add dimensions if known
	if (image_ || loadImage())
		info += S_FMT("%dx%d", image_->getWidth(), image_->getHeight());
	else
		info += "Unknown size";

	// Add type
	if (type_ == "texture")
		info += ", Texture";
	else
		info += ", Flat";

	// Add scaling info
	if (image_->getScaleX() != 1.0 || image_->getScaleY() != 1.0)
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
	this->truncate_names_ = true;

	// Init sorting
	addSortType("Usage Count");
	setSortType(map_tex_sort);

	// Set window title
	SetTitle("Browse Map Textures");

	int mapFormat = map->currentFormat();

	// Textures
	if (type == 0 || Game::configuration().featureSupported(Game::Feature::MixTexFlats))
	{
		addGlobalItem(new MapTexBrowserItem("-", 0, 0));

		vector<map_texinfo_t>& textures = MapEditor::textureManager().getAllTexturesInfo();
		for (unsigned a = 0; a < textures.size(); a++)
		{
			if ((mapFormat != MAP_UDMF ||
				!Game::configuration().featureSupported(Game::Feature::LongNames)) &&
				textures[a].shortName.Len() > 8)
			{
				// Only UDMF supports texture/flat names longer than 8 characters
				continue;
			}

			// Don't add two textures with the same name
			bool dontAdd = false;
			for (unsigned b = 0; b < textures.size(); b++)
			{
				if (textures[b].shortName.Cmp(textures[a].shortName) == 0 && b > a)
				{
					dontAdd = true;
					break;
				}
			}

			if (dontAdd)
				continue;
			// Add browser item
			addItem(new MapTexBrowserItem(textures[a].shortName, 0, textures[a].index),
				determineTexturePath(textures[a].archive, textures[a].category, "Textures", textures[a].path));
		}
	}

	// Flats
	if (type == 1 || Game::configuration().featureSupported(Game::Feature::MixTexFlats))
	{
		vector<map_texinfo_t>& flats = MapEditor::textureManager().getAllFlatsInfo();
		for (unsigned a = 0; a < flats.size(); a++)
		{
			if ((mapFormat != MAP_UDMF ||
				!Game::configuration().featureSupported(Game::Feature::LongNames)) &&
				flats[a].shortName.Len() > 8)
			{
				// Only UDMF supports texture/flat names longer than 8 characters
				continue;
			}

			// Don't add two flats with the same name
			bool dontAdd = false;
			for (unsigned b = 0; b < flats.size(); b++)
			{
				if (flats[b].shortName.Cmp(flats[a].shortName) == 0 && b > a)
				{
					dontAdd = true;
					break;
				}
			}

			if (dontAdd)
				continue;

			// Determine tree path
			string path = determineTexturePath(flats[a].archive, flats[a].category, "Flats", flats[a].path);

			// Add browser item
			if (flats[a].category == MapTextureManager::TC_TEXTURES)
				addItem(new MapTexBrowserItem(flats[a].shortName, 0, flats[a].index), path);
			else
				addItem(new MapTexBrowserItem(flats[a].shortName, 1, flats[a].index), path);
		}
	}

	// Full path textures
	if (mapFormat == MAP_UDMF &&
		Game::configuration().featureSupported(Game::Feature::LongNames))
	{
		// Textures
		vector<map_texinfo_t>& fpTextures = MapEditor::textureManager().getAllTexturesInfo();
		for (unsigned a = 0; a < fpTextures.size(); a++)
		{
			if (fpTextures[a].category != MapTextureManager::TC_TEXTURES &&
					fpTextures[a].category != MapTextureManager::TC_HIRES &&
					!fpTextures[a].path.IsEmpty() && fpTextures[a].path.Cmp("/") != 0) {
				// Add browser item
				addItem(new MapTexBrowserItem(fpTextures[a].longName, 0, fpTextures[a].index),
					determineTexturePath(fpTextures[a].archive, fpTextures[a].category, "Textures (Full Path)", fpTextures[a].path));
			}
		}

		// Flats
		vector<map_texinfo_t>& fpFlats = MapEditor::textureManager().getAllFlatsInfo();
		for (unsigned a = 0; a < fpFlats.size(); a++)
		{
			if (!fpFlats[a].path.IsEmpty() && fpFlats[a].path.Cmp("/") != 0) {
				// Add browser item
				//fpName.Remove(0, 1); // Remove leading slash
				addItem(new MapTexBrowserItem(fpFlats[a].longName, 1, fpFlats[a].index),
					determineTexturePath(fpFlats[a].archive, fpFlats[a].category, "Textures (Full Path)", fpFlats[a].path));
			}
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
			ret += archive->filename(false);
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
		return left->name() < right->name();
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
		vector<BrowserItem*>& items = canvas_->itemList();
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

	vector<BrowserItem*>& items = canvas_->itemList();
	for (unsigned i = 0; i < items.size(); i++)
	{
		MapTexBrowserItem* item = (MapTexBrowserItem*)items[i];
		if (type == 0)
			item->setUsage(map->texUsageCount(item->name()));
		else
			item->setUsage(map->flatUsageCount(item->name()));
	}
}
